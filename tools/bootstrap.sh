#!/usr/bin/env bash
# Rebuild the sandbox toolchain after a context reset (build/ + layer tools are
# NOT persisted by workspace snapshots; this restores cmake + X11/GL dev bits).
set -e
export PATH=$HOME/.local/bin:$PATH
if ! command -v cmake >/dev/null; then pip install --quiet cmake; fi
PREFIX=/tmp/x11prefix
if [ ! -f $PREFIX/usr/include/X11/extensions/Xrandr.h ]; then
  mkdir -p /tmp/debs $PREFIX && cd /tmp/debs
  apt-get download libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
    libxfixes-dev libxkbcommon-dev libwayland-dev wayland-protocols \
    libgl-dev libglx-dev libopengl-dev libgl1 >/dev/null
  for d in *.deb; do dpkg -x "$d" $PREFIX; done
  cd - >/dev/null
fi
export PKG_CONFIG_PATH=$PREFIX/usr/lib/x86_64-linux-gnu/pkgconfig:/usr/lib/x86_64-linux-gnu/pkgconfig
export LIBRARY_PATH=$PREFIX/usr/lib/x86_64-linux-gnu
if [ ! -f build/CMakeCache.txt ]; then
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INCLUDE_PATH=$PREFIX/usr/include -DCMAKE_LIBRARY_PATH=$PREFIX/usr/lib/x86_64-linux-gnu
fi
mkdir -p build/generated/protocol
python3 tools/protogen/protogen.py shared/protocol/messages.md build/generated/protocol
cmake --build build -j"$(nproc)"
./build/tests/bh_tests --no-skip | tail -2
