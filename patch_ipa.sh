#/bin/sh

set -e

if [ $# -ne 2 ]; then
    echo "usage: ./chimera_patch.sh <stock-ipa> <output-ipa>"
    exit 1
fi

TEMP_DIR="$(mktemp -d)"
CURRENT="$(pwd)"
CHIMERA="$TEMP_DIR/Payload/Chimera.app/Chimera"
FRAMEWORKS="$TEMP_DIR/Payload/Chimera.app/Frameworks"

echo "[*] temp dir at: $TEMP_DIR"

unzip $1 -d $TEMP_DIR >/dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "[-] failed to extract ipa file"
    exit 1
fi

echo "[*] applying patch file..."
ldid -r $CHIMERA
xxd -c1 -r patchfile $CHIMERA
echo "[*] adding dylib..."

echo "[*] moving files..."
mkdir $FRAMEWORKS
cp -a chimera4k.dylib $FRAMEWORKS/chimera4k.dylib

ldid -S $CHIMERA

cd $TEMP_DIR
zip -r $CURRENT/out.ipa Payload > /dev/null

rm -rf "$TEMP_DIR"
  