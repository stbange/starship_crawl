#!/usr/bin/env python3
"""Stage the built game as a static site, including its default index page."""
import argparse
from pathlib import Path
import shutil


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=Path("build-web"))
    parser.add_argument("--output", type=Path, default=Path("build-pages"))
    args = parser.parse_args()
    source = args.build_dir.resolve()
    destination = args.output.resolve()
    files = {
        "starship_crawl.html": "index.html",
        "starship_crawl.js": "starship_crawl.js",
        "starship_crawl.wasm": "starship_crawl.wasm",
    }
    for name in files:
        path = source / name
        if not path.is_file() or path.stat().st_size == 0:
            raise SystemExit(f"Missing web build: {path}. Build with Emscripten first.")
    if not (source / "starship_crawl.wasm").read_bytes().startswith(b"\0asm\x01\0\0\0"):
        raise SystemExit("The web build does not contain a valid WebAssembly module.")
    destination.mkdir(parents=True, exist_ok=True)
    for name, target in files.items():
        shutil.copyfile(source / name, destination / target)
    (destination / ".nojekyll").write_text("")
    root = Path(__file__).resolve().parents[1]
    shutil.copyfile(root / "THIRD_PARTY_NOTICES.md", destination / "THIRD_PARTY_NOTICES.md")
    print(f"Staged GitHub Pages site at {destination}")


if __name__ == "__main__":
    main()
