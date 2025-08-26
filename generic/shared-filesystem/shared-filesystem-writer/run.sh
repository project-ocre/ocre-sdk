#!/bin/sh

# Create shared filesystem directory
mkdir -p ./shared

# Run the writer container with shared filesystem mounted
echo "Running shared filesystem writer container..."
iwasm --map-dir=/::./ build/shared-filesystem-writer.wasm

echo "Writer container completed. Check shared/shared_data.txt for output." 