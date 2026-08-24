#!/bin/bash
set -euo pipefail

QT_ROOT=/home/suo/Qt/6.9.1/gcc_64
SRC=/home/suo/Desktop/work/MyFolder/v1.1.1/Client/MyFolder
STAGE=/tmp/myfolder-deb
APPDIR=$STAGE/opt/myfolder
PATCHELF=${PATCHELF:-$(command -v patchelf || true)}

if [ -z "$PATCHELF" ] || [ ! -x "$PATCHELF" ]; then
  echo "patchelf is required (install it or set PATCHELF=/path/to/patchelf)" >&2
  exit 1
fi

rm -rf "$STAGE"
mkdir -p "$STAGE/DEBIAN" "$APPDIR/lib" "$APPDIR/plugins" "$APPDIR/qml"
install -m 0755 "$SRC/packaging/linux/postinst" "$STAGE/DEBIAN/postinst"
install -m 0755 "$SRC/packaging/linux/postrm" "$STAGE/DEBIAN/postrm"

# 1. Binary
cp "$SRC/build-linux-deb/appMyFolder" "$APPDIR/appMyFolder"
chmod 755 "$APPDIR/appMyFolder"

# 1b. Desktop integration. Keep the shell-facing icon in the hicolor theme;
# the application itself uses the same PNG from its Qt resources at runtime.
mkdir -p "$STAGE/usr/bin" \
  "$STAGE/usr/share/applications" \
  "$STAGE/usr/share/icons/hicolor/128x128/apps" \
  "$STAGE/usr/share/icons/hicolor/512x512/apps"
cat > "$STAGE/usr/bin/myfolder" <<'EOF'
#!/bin/sh
set -eu
export MYFOLDER_BASE_URL="${MYFOLDER_BASE_URL:-https://api.myfolder.com.cn}"
exec /opt/myfolder/appMyFolder "$@"
EOF
chmod 755 "$STAGE/usr/bin/myfolder"
cat > "$STAGE/usr/share/applications/myfolder.desktop" <<'EOF'
[Desktop Entry]
Type=Application
Name=MyFolder
GenericName=File Transfer
Comment=MyFolder 跨设备文件传输客户端
# %u forwards the myfolder:// OAuth callback URL to the client.
Exec=/usr/bin/myfolder %u
Icon=myfolder
Terminal=false
Categories=Network;FileTransfer;Utility;
StartupNotify=true
StartupWMClass=appMyFolder
MimeType=x-scheme-handler/myfolder;
EOF
chmod 644 "$STAGE/usr/share/applications/myfolder.desktop"
if command -v desktop-file-validate >/dev/null 2>&1; then
  desktop-file-validate "$STAGE/usr/share/applications/myfolder.desktop"
fi
if ! grep -Eq '^Exec=.*%u([[:space:]]|$)' "$STAGE/usr/share/applications/myfolder.desktop"; then
  echo "OAuth protocol handler must forward the callback URL with %u" >&2
  exit 1
fi
if command -v convert >/dev/null 2>&1; then
  convert "$SRC/Icons/app.png" -resize 128x128 "$STAGE/usr/share/icons/hicolor/128x128/apps/myfolder.png"
else
  cp "$SRC/Icons/app.png" "$STAGE/usr/share/icons/hicolor/128x128/apps/myfolder.png"
fi
cp "$SRC/Icons/app.png" "$STAGE/usr/share/icons/hicolor/512x512/apps/myfolder.png"

# 2. Qt plugins (selected categories)
for d in platforms xcbglintegrations imageformats iconengines tls platforminputcontexts platformthemes generic networkinformation; do
  mkdir -p "$APPDIR/plugins/$d"
  cp -a "$QT_ROOT/plugins/$d/." "$APPDIR/plugins/$d/"
done

# 3. Qt QML modules (selected)
for d in QtQml QtQuick QtCore QtNetwork QtWebSockets; do
  cp -a "$QT_ROOT/qml/$d" "$APPDIR/qml/"
done
mkdir -p "$APPDIR/qml/Qt"
cp -a "$QT_ROOT/qml/Qt/labs" "$APPDIR/qml/Qt/"
cp -a "$QT_ROOT/qml/Assets" "$APPDIR/qml/"
cp -a "$QT_ROOT/qml/builtins.qmltypes" "$APPDIR/qml/" 2>/dev/null || true

# 4. Dependency closure: copy every Qt-side shared lib referenced by any staged ELF
for i in $(seq 1 12); do
  changed=0
  find "$APPDIR" -type f \( -name 'appMyFolder' -o -name '*.so' -o -name '*.so.*' \) > /tmp/elf-list.txt
  : > /tmp/qt-libs-needed.txt
  : > /tmp/qt-libs-missing.txt
  while IFS= read -r elf; do
    # libs resolved inside the Qt tree
    ldd "$elf" 2>/dev/null | awk '{for(j=1;j<=NF;j++) if ($j ~ /^\/home\/suo\/Qt\/6\.9\.1\/gcc_64\/lib\//) print $j}' >> /tmp/qt-libs-needed.txt || true
    # libs not found (or wrongly resolved to system Qt): record SONAME
    ldd "$elf" 2>/dev/null | awk '/=> not found/ {print $1}' >> /tmp/qt-libs-missing.txt || true
    ldd "$elf" 2>/dev/null | awk '{for(j=1;j<=NF;j++) if ($j ~ /^\/(lib|usr\/lib)/ && $1 ~ /^libQt6|^libicu/) print $1}' >> /tmp/qt-libs-missing.txt || true
  done < /tmp/elf-list.txt
  sort -u /tmp/qt-libs-needed.txt -o /tmp/qt-libs-needed.txt
  sort -u /tmp/qt-libs-missing.txt -o /tmp/qt-libs-missing.txt

  while IFS= read -r libpath; do
    [ -z "$libpath" ] && continue
    base=$(basename "$libpath")
    if [ ! -e "$APPDIR/lib/$base" ]; then
      # dereference: ldd reports e.g. libQt6Core.so.6 which is a symlink in Qt's lib dir
      cp -L "$libpath" "$APPDIR/lib/"
      changed=1
    fi
  done < /tmp/qt-libs-needed.txt

  while IFS= read -r soname; do
    [ -z "$soname" ] && continue
    if [ -e "$QT_ROOT/lib/$soname" ] && [ ! -e "$APPDIR/lib/$soname" ]; then
      cp -L "$QT_ROOT/lib/$soname" "$APPDIR/lib/"
      changed=1
    fi
  done < /tmp/qt-libs-missing.txt

  [ "$changed" -eq 0 ] && break
done

# 5. Fix binary RUNPATH -> $ORIGIN/lib
"$PATCHELF" --set-rpath '$ORIGIN/lib' "$APPDIR/appMyFolder"

# 6. qt.conf
cat > "$APPDIR/qt.conf" <<'EOF'
[Paths]
Prefix = .
Plugins = plugins
QmlImports = qml
Libraries = lib
EOF

# 7. Strip
strip --strip-unneeded "$APPDIR/appMyFolder" 2>/dev/null || true
find "$APPDIR/lib" "$APPDIR/plugins" "$APPDIR/qml" -type f -name '*.so*' -exec strip --strip-unneeded {} \; 2>/dev/null || true

echo "=== staged size ==="
du -sh "$APPDIR"
echo "=== lib count ==="
ls "$APPDIR/lib" | wc -l
echo "=== not-found libs in ldd ==="
while IFS= read -r elf; do ldd "$elf" 2>/dev/null; done < /tmp/elf-list.txt | grep -i 'not found' | sort -u || echo "(none)"
