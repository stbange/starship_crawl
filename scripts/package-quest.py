#!/usr/bin/env python3
"""Stage only the opt-in Quest WebXR application in its own directory."""
import argparse
import hashlib
from pathlib import Path
import shutil


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=Path('build-quest'))
    parser.add_argument('--output', type=Path, default=Path('build-pages/quest'))
    args = parser.parse_args()
    source, destination = args.build_dir.resolve(), args.output.resolve()
    stem = 'starship_crawl_quest'
    for suffix in ('.html', '.js', '.wasm'):
        path = source / (stem + suffix)
        if not path.is_file() or not path.stat().st_size:
            raise SystemExit(f'Missing Quest build: {path}. Build starship_crawl_quest first.')
    wasm = (source / (stem + '.wasm')).read_bytes()
    if not wasm.startswith(b'\0asm\x01\0\0\0'):
        raise SystemExit('The Quest build does not contain a valid WebAssembly module.')
    html = (source / (stem + '.html')).read_text()
    js = (source / (stem + '.js')).read_text()
    if stem + '.js' not in html or 'immersive-vr' not in js or '/quest-persist' not in js:
        raise SystemExit('The selected build is not the isolated Quest application.')
    if stem + '.wasm' not in js:
        raise SystemExit('The Quest loader does not reference its WebAssembly module.')
    # Refuse to replace the regular index even if called with the wrong output.
    if (destination / 'starship_crawl.js').exists():
        raise SystemExit('Quest output must be a separate directory from the regular web game.')
    destination.mkdir(parents=True, exist_ok=True)
    # Keep JS and WASM on the same build across Pages/browser caches. In
    # particular, an updated XR bridge must never call an older C export ABI.
    revision = hashlib.sha256(js.encode() + b'\0' + wasm).hexdigest()[:16]
    loader_name, wasm_name = stem + '.' + revision + '.js', stem + '.' + revision + '.wasm'
    (destination / 'index.html').write_text(html.replace(stem + '.js', loader_name))
    (destination / loader_name).write_text(js.replace(stem + '.wasm', wasm_name))
    shutil.copyfile(source / (stem + '.wasm'), destination / wasm_name)
    (destination / '.nojekyll').write_text('')
    root = Path(__file__).resolve().parents[1]
    shutil.copyfile(root / 'THIRD_PARTY_NOTICES.md', destination / 'THIRD_PARTY_NOTICES.md')
    print(f'Staged standalone Quest site at {destination}')


if __name__ == '__main__':
    main()
