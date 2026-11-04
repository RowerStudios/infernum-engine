#!/bin/sh
set -e
cd "$(dirname "$0")"
qmake editor.pro
make -j"$(nproc)"
printf '>> editor build successful ..\n'
