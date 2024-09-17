#!/usr/bin/env bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
OUT_DIR="$SCRIPT_DIR/docs/doxygen/tags"

if [ ! -d "$OUT_DIR" ]; then
	mkdir -p "$OUT_DIR"
fi

pushd "$OUT_DIR"
curl -z "qtcore.tags"    --remote-time -O "https://doc.qt.io/qt-6/qtcore.tags"
curl -z "qtgui.tags"     --remote-time -O "https://doc.qt.io/qt-6/qtgui.tags"
curl -z "qtwidgets.tags" --remote-time -O "https://doc.qt.io/qt-6/qtwidgets.tags"
curl -z "libstdc++.tag"  --remote-time -O "https://gcc.gnu.org/onlinedocs/libstdc++/latest-doxygen/libstdc++.tag"
popd
