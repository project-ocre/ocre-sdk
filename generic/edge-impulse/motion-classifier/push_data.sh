#!/usr/bin/env bash
set -euo pipefail

# Usage: ./copy-largest-blob.sh <container-name>
# Copies the largest file in ~/.atym/<container-name>/blobs/sha256
# to dan@192.168.0.120:~/atym/atym-runtime/build/ocre_data/images/<file>.bin

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 <folder_path>" >&2
  exit 1
fi

SRC_DIR="$1"
DEST_HOST="dan@192.168.0.120"
DEST_DIR="~/atym/atym-runtime/build/ocre_data/cfs"

# Ensure the source directory exists
if [[ ! -d "$SRC_DIR" ]]; then
  echo "Error: directory not found: $SRC_DIR" >&2
  exit 1
fi

DEST_PATH="${DEST_HOST}:${DEST_DIR}/."

echo "Source:      ${SRC_DIR}"
echo

scp -r "${SRC_DIR}" "${DEST_PATH}"
