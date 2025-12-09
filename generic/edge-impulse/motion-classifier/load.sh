#!/usr/bin/env bash
set -euo pipefail

# Usage: ./copy-largest-blob.sh <container-name>
# Copies the largest file in ~/.atym/<container-name>/blobs/sha256
# to dan@192.168.0.120:~/atym/atym-runtime/build/ocre_data/images/<file>.bin

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 <container-name>" >&2
  exit 1
fi

CONTAINER_NAME="$1"
SRC_DIR="$HOME/.atym/${CONTAINER_NAME}/blobs/sha256"
DEST_HOST="dan@192.168.0.120"
DEST_DIR="~/atym/atym-runtime/build/ocre_data/images"

# Ensure the source directory exists
if [[ ! -d "$SRC_DIR" ]]; then
  echo "Error: directory not found: $SRC_DIR" >&2
  exit 1
fi

# Find the largest file in the directory.
# Assumes filenames have no spaces (true for hash filenames).
if ! cd "$SRC_DIR"; then
  echo "Error: failed to cd into $SRC_DIR" >&2
  exit 1
fi

if ! largest_file=$(ls -S 2>/dev/null | head -n 1); then
  echo "Error: failed to list files in $SRC_DIR" >&2
  exit 1
fi

if [[ -z "$largest_file" ]]; then
  echo "Error: no files found in $SRC_DIR" >&2
  exit 1
fi

SRC_PATH="${SRC_DIR}/${largest_file}"
DEST_PATH="${DEST_HOST}:${DEST_DIR}/${largest_file}.bin"

echo "Source:      ${SRC_PATH}"
echo "Destination: ${DEST_PATH}"
echo

scp "${SRC_PATH}" "${DEST_PATH}"
