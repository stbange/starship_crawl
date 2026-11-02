#!/usr/bin/env bash
# Local tool installation for the Debian 13 managed cloud image.
set -euo pipefail
repo_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
tools_dir=/workspace/.starship-tools
mkdir -p "$tools_dir/src" "$tools_dir/apt/lists/partial" \
  "$tools_dir/apt/cache/archives/partial" "$tools_dir/apt/empty" "$tools_dir/sysroot"

if [[ ! -x "$tools_dir/python/cmake/data/bin/cmake" ]]; then
  python3 -m pip install --target "$tools_dir/python" cmake==3.31.6 ninja==1.11.1.4
fi

cat > "$tools_dir/apt/apt.conf" <<EOF
Dir::Etc::parts "$tools_dir/apt/empty";
Dir::Etc::sourcelist "$tools_dir/apt/sources.list";
Dir::Etc::sourceparts "$tools_dir/apt/empty";
Dir::State::lists "$tools_dir/apt/lists";
Dir::Cache "$tools_dir/apt/cache";
APT::Sandbox::User "$(id -un)";
EOF
cat > "$tools_dir/apt/sources.list" <<'EOF'
deb [signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] https://deb.debian.org/debian trixie main
EOF

# apt verifies the signed index and package hashes; download-only never installs
# into the system. Extracting development packages needs no root privileges.
if [[ ! -f "$tools_dir/cloud-packages-v1" ]]; then
  APT_CONFIG="$tools_dir/apt/apt.conf" apt-get update
  APT_CONFIG="$tools_dir/apt/apt.conf" apt-get --download-only --no-install-recommends \
    -y -o Debug::NoLocking=true install emscripten=3.1.69+dfsg-3 \
    libgl-dev libx11-dev libxext-dev libxi-dev libxcursor-dev libxrandr-dev
  python3 - <<'PY'
from pathlib import Path
import subprocess
root = Path('/workspace/.starship-tools/sysroot')
for archive in sorted(Path('/workspace/.starship-tools/apt/cache/archives').glob('*.deb')):
    if 'mingw' in archive.name:
        continue  # An optional cross-compiler is managed separately.
    subprocess.run(['dpkg-deb', '--extract', str(archive), str(root)], check=True)
# Development packages contain linker symlinks. Resolve their runtime targets
# against the matching libraries already supplied by this Debian image.
base = root / 'usr/lib/x86_64-linux-gnu'
system = Path('/usr/lib/x86_64-linux-gnu')
for link in base.glob('*.so'):
    if link.is_symlink() and not link.exists():
        target = link.parent / link.readlink()
        candidates = sorted(system.glob(link.name + '.*'), key=lambda p: len(p.name))
        if candidates and not target.exists():
            target.symlink_to(candidates[0])
PY
  touch "$tools_dir/cloud-packages-v1"
fi

archive="$tools_dir/src/sdl3.tar.gz"
if [[ ! -f "$archive" ]]; then
  curl --fail --location --show-error \
    https://codeload.github.com/libsdl-org/SDL/tar.gz/f5e5f6588921eed3d7d048ce43d9eb1ff0da0ffc \
    -o "$archive"
fi
printf '%s  %s\n' 7f80bce51b7179bab472e75b90ce8ac06b162b3da8e19feadecee5acea07dbfb "$archive" | sha256sum --check
if [[ ! -f "$tools_dir/src/sdl3/CMakeLists.txt" ]]; then
  mkdir -p "$tools_dir/src/sdl3"
  tar -xzf "$archive" --strip-components=1 -C "$tools_dir/src/sdl3"
fi

python3 - <<'PY'
from pathlib import Path
import shutil
root = Path('/workspace/.starship-tools')
config = {
    'LLVM_ROOT': str(root / 'sysroot/usr/lib/llvm-19/bin'),
    'BINARYEN_ROOT': str(root / 'sysroot/usr'),
    'NODE_JS': shutil.which('node'),
    'CACHE': str(root / 'sysroot/usr/share/emscripten/cache'),
    'FROZEN_CACHE': False,
}
if not config['NODE_JS']:
    raise SystemExit('A working Node.js runtime must be on PATH.')
(root / 'emscripten-config.py').write_text(''.join(f'{key} = {value!r}\n' for key, value in config.items()))
PY
cat > "$tools_dir/activate.sh" <<'EOF'
export PATH="/workspace/.starship-tools/python/cmake/data/bin:/workspace/.starship-tools/python/ninja/data/bin:/workspace/.starship-tools/sysroot/usr/share/emscripten:$PATH"
export EM_CONFIG=/workspace/.starship-tools/emscripten-config.py
export SC_SYSROOT=/workspace/.starship-tools/sysroot
export LD_LIBRARY_PATH="/workspace/.starship-tools/sysroot/usr/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export MESA_SHADER_CACHE_DIR=/workspace/.starship-tools/mesa-cache
EOF
source "$tools_dir/activate.sh"
cd "$repo_dir"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DFETCHCONTENT_SOURCE_DIR_SDL3="$tools_dir/src/sdl3" \
  -DCMAKE_INCLUDE_PATH="$SC_SYSROOT/usr/include" \
  -DCMAKE_LIBRARY_PATH="$SC_SYSROOT/usr/lib/x86_64-linux-gnu" \
  -DCMAKE_C_FLAGS="-I$SC_SYSROOT/usr/include" -DSDL_UNIX_CONSOLE_BUILD=ON
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
SDL_VIDEODRIVER=offscreen EGL_PLATFORM=surfaceless SDL_AUDIODRIVER=dummy \
  ./build/starship_crawl --seed 713 --smoke 10 --no-save
emcmake cmake -S . -B build-web -DCMAKE_BUILD_TYPE=Release \
  -DFETCHCONTENT_SOURCE_DIR_SDL3="$tools_dir/src/sdl3" -DBUILD_TESTING=OFF
cmake --build build-web --parallel 4
