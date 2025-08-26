#!/bin/sh
mkdir -p fs
iwasm --map-dir=/::./fs build/filesystem-full.wasm
