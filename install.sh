#!/bin/bash

SRC_DIR="$(dirname "$(realpath "$0")")"
DEST_DIR="$HOME/.local/bin/headsetcontrol-qt"
DESKTOP_ENTRY_FILE="$HOME/.local/share/applications/headsetcontrol-qt.desktop"

mkdir -p "$DEST_DIR"
cp -r "$SRC_DIR/src/"* "$DEST_DIR/"

EXEC_CMD="$DEST_DIR/headsetcontrol-qt.py"

cat > "$DESKTOP_ENTRY_FILE" << EOL
[Desktop Entry]
Name=HeadsetControl-Qt
Exec=$EXEC_CMD
Icon=$DEST_DIR/icons/icon.png
Path=$DEST_DIR/
Terminal=false
Type=Application
EOL

chmod +x "$DESKTOP_ENTRY_FILE"
echo ""
echo "Setup complete. HeadsetControl-Qt should now be available in your applications menu."
