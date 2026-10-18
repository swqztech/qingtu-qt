#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
STAGE="$ROOT/packaging/stage"
PACKAGING="$ROOT/packaging"
DIST="$ROOT/dist"
VERSION="$(grep -E 'project\(.*VERSION' "$ROOT/CMakeLists.txt" | sed -E 's/.*VERSION\s+([0-9.]+).*/\1/')"
NAME="轻图-qt-${VERSION}-win64-绿色版"
INSTALLER_MODE="${INSTALLER:-both}"

find_qt() {
  [[ -n "${CMAKE_PREFIX_PATH:-}" && -d "$CMAKE_PREFIX_PATH" ]] && { echo "$CMAKE_PREFIX_PATH"; return; }
  local repo_qt="$ROOT/../.qt"
  if [[ -d "$repo_qt" ]]; then
    find "$repo_qt" -maxdepth 2 -type d -name 'msvc*_64' 2>/dev/null | sort -r | head -1
  fi
}

CMAKE="$(command -v cmake 2>/dev/null || true)"
if [[ -z "$CMAKE" ]]; then
  for p in "/c/Program Files/CMake/bin/cmake.exe" \
    "/c/Users/Administrator/AppData/Local/Microsoft/WinGet/Packages/Kitware.CMake_Microsoft.Winget.Source_8wekyb3d8bbwe/cmake-"*/bin/cmake.exe; do
    [[ -x "$p" ]] && CMAKE="$p" && break
  done
fi
QT="$(find_qt || true)"
[[ -n "$CMAKE" && -n "$QT" ]] || { echo "需要 cmake 与 Qt6 (CMAKE_PREFIX_PATH)"; exit 1; }

(cd "$ROOT" && "$CMAKE" -B build -DCMAKE_PREFIX_PATH="$QT" && "$CMAKE" --build build --config Release)
EXE_SRC="$ROOT/build/Release/qingtu-qt.exe"
[[ -f "$EXE_SRC" ]] || { echo "未找到 exe"; exit 1; }

PORTABLE_DIR="$DIST/$NAME"
rm -rf "$DIST"
mkdir -p "$PORTABLE_DIR"
cp -f "$EXE_SRC" "$PORTABLE_DIR/qingtu-qt.exe"
cp -f "$STAGE/"* "$PORTABLE_DIR/"
"$QT/bin/windeployqt.exe" --no-compiler-runtime --no-translations "$PORTABLE_DIR/qingtu-qt.exe"

(cd "$DIST" && tar -a -cf "${NAME}.zip" "$NAME")
echo "绿色版: $DIST/${NAME}.zip ($(du -sh "$PORTABLE_DIR" | cut -f1))"

if [[ "$INSTALLER_MODE" == "nsis" || "$INSTALLER_MODE" == "both" ]]; then
  INSTALLER_STAGE="$PACKAGING/installer-stage"
  rm -rf "$INSTALLER_STAGE"
  mkdir -p "$INSTALLER_STAGE"
  cp -r "$PORTABLE_DIR"/* "$INSTALLER_STAGE/"
  SETUP_EXE="$DIST/轻图-qt-${VERSION}-win64-setup.exe"
  SETUP_ASCII="$DIST/Qingtu-Qt-${VERSION}-win64-setup.exe"
  for p in "/c/Program Files (x86)/NSIS/makensis.exe" "/c/Program Files/NSIS/makensis.exe"; do
    if [[ -f "$p" ]]; then
      "$p" "//INPUTCHARSET" "UTF8" "//DVERSION=$VERSION" \
        "//DSTAGE_DIR=$(cygpath -w "$INSTALLER_STAGE")" \
        "//DOUTFILE=$(cygpath -w "$SETUP_ASCII")" \
        "$(cygpath -w "$PACKAGING/nsis/setup.nsi")"
      mv -f "$SETUP_ASCII" "$SETUP_EXE"
      echo "安装包: $SETUP_EXE ($(du -sh "$SETUP_EXE" | cut -f1))"
      break
    fi
  done
  if [[ ! -f "$SETUP_EXE" ]]; then
    echo "未找到 NSIS (makensis)，跳过安装包。可安装 NSIS 后设置 INSTALLER=both 重试。"
  fi
fi
