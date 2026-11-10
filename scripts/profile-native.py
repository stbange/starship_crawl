#!/usr/bin/env python3
"""Measure software-rendered native frame intervals without shipping profiling hooks.

Requires an existing Linux CMake Makefiles game build. Relinks its exact objects
with a test-only SDL swap wrapper, then runs separate processes per scene. No
source, gameplay state, graphics setting or ordinary executable is replaced.
"""
import argparse
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
WRAPPER = r'''
#define _POSIX_C_SOURCE 200809L
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/resource.h>
typedef struct SDL_Window SDL_Window;
bool __real_SDL_GL_SwapWindow(SDL_Window *window);
static double times[8192], previous;
static unsigned count, frames;
static double clock_seconds(void) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t);
    return t.tv_sec + t.tv_nsec * 1e-9;
}
static int compare(const void *a, const void *b) {
    double x=*(const double*)a,y=*(const double*)b; return (x>y)-(x<y);
}
static void report(void) {
    struct rusage usage; getrusage(RUSAGE_SELF,&usage);
    if(!count) return;
    qsort(times,count,sizeof(double),compare);
    printf("PROFILE {\"samples\":%u,\"warmup_frames\":30,\"p50_ms\":%.4f,\"p95_ms\":%.4f,\"p99_ms\":%.4f,\"max_ms\":%.4f,\"max_rss_kib\":%ld}\n",
      count,times[count/2],times[count*95/100],times[count*99/100],times[count-1],usage.ru_maxrss);
}
bool __wrap_SDL_GL_SwapWindow(SDL_Window *window) {
    bool ok=__real_SDL_GL_SwapWindow(window); double current=clock_seconds();
    if(!frames) atexit(report);
    if(frames>=30 && count<8192) times[count++]=(current-previous)*1000;
    previous=current;frames++; return ok;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--suite', choices=('common', 'all'), default='all',
                        help='Common arrival/bay views also exist in the pre-change build')
    args = parser.parse_args()
    build = args.build_dir.resolve()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    environment = dict(os.environ, SDL_VIDEODRIVER='offscreen', SDL_AUDIODRIVER='dummy', LIBGL_ALWAYS_SOFTWARE='1')
    records = []
    with tempfile.TemporaryDirectory(prefix='starship-render-profile-') as temporary:
        temp = Path(temporary)
        source, obj, binary = temp / 'swap.c', temp / 'swap.o', temp / 'profile-game'
        source.write_text(WRAPPER)
        command = shlex.split((build / 'CMakeFiles/starship_crawl.dir/link.txt').read_text())
        subprocess.run([command[0], '-O2', '-c', str(source), '-o', str(obj)], check=True)
        command[command.index('-o') + 1] = str(binary)
        command.insert(1, str(obj))
        command.append('-Wl,--wrap=SDL_GL_SwapWindow')
        subprocess.run(command, cwd=build, check=True)
        cases = [(713, 'arrival', False), (713, 'bay', False), (713, 'bay', True)]
        if args.suite == 'all':
            cases += [(27, 'foundry6', False), (7, 'foundry6', False),
                      (713, 'guardian6warn', False), (713, 'build8', False),
                      (27, 'foundry6', True)]
        for seed, scene, split in cases:
            label = f'{seed}-{scene}' + ('-split' if split else '')
            run = [str(binary), '--seed', str(seed), '--scene', scene, '--no-save', '--smoke', '600',
                   '--capture', str(output / f'{label}.ppm')]
            if split:
                run.append('--split')
            result = subprocess.run(run, env=environment, cwd=ROOT, text=True, capture_output=True, check=True)
            (output / f'{label}.log').write_text(result.stdout + result.stderr)
            measurement = next(json.loads(line[8:]) for line in result.stdout.splitlines() if line.startswith('PROFILE '))
            records.append(dict(seed=seed, scene=scene, split=split, frames=600, **measurement))
            print(label, measurement, flush=True)
    (output / 'summary.json').write_text(json.dumps(records, indent=2) + '\n')


if __name__ == '__main__':
    main()
