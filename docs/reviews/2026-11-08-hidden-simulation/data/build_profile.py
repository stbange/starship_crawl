"""Relink the ordinary web objects with review-only timing wrappers; leave Pages intact."""
from pathlib import Path
import hashlib
import json
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[4]
HERE = Path(__file__).resolve().parent
build = ROOT / 'build-web'
output = ROOT / 'build-web-profile'
output.mkdir(exist_ok=True)
command = shlex.split((build / 'CMakeFiles/starship_crawl.dir/link.txt').read_text())
obj = output / 'profile_browser.o'
subprocess.run([command[0], '-O3', '-std=gnu17', '-I' + str(ROOT / 'src'), '-c',
                str(HERE / 'profile_browser.c'), '-o', str(obj)], check=True)
command[command.index('-o') + 1] = str(output / 'starship_crawl.html')
command += [str(obj), '-Wl,--wrap=factions_step', '-Wl,--wrap=factions_forces_step',
            '-Wl,--wrap=hidden_director_step', '-Wl,--wrap=factions_tactics', '-Wl,--wrap=game_step']
subprocess.run(command, cwd=build, check=True)
subprocess.run(['python3', 'scripts/package-web.py', '--build-dir', str(output),
                '--output', 'build-pages-profile/starship_crawl'],
               cwd=ROOT, check=True)
(HERE / 'profile-build.json').write_text(json.dumps({
    'command': command, 'ordinary_core_sha256': hashlib.sha256((build / 'libsc_core.a').read_bytes()).hexdigest(),
    'wrapper_sha256': hashlib.sha256((HERE / 'profile_browser.c').read_bytes()).hexdigest(),
    'wasm_sha256': hashlib.sha256((output / 'starship_crawl.wasm').read_bytes()).hexdigest(),
    'published': False}, indent=2) + '\n')
