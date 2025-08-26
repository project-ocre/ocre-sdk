#!/bin/sh

# Check if shared filesystem directory exists
if [ ! -d "../shared-filesystem-writer/shared" ]; then
    echo "Error: Shared filesystem directory not found."
    echo "Please run the writer container first: cd ../shared-filesystem-writer && ./run.sh"
    exit 1
fi

# Run the simple reader container with the same shared filesystem mounted
echo "Running simple shared filesystem reader container..."
iwasm --map-dir=/shared::../shared-filesystem-writer/shared build/shared-filesystem-reader.wasm

echo "Simple reader container completed. Check ../shared-filesystem-writer/shared/shared_data.txt for final output." 
