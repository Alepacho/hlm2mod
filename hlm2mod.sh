#!/bin/bash
cd "${0%/*}"
# DYLD_PRINT_LIBRARIES=1 
# exec > output.log 2>&1
# DYLD_PRINT_APIS=1 
DYLD_FORCE_FLAT_NAMESPACE=1 DYLD_INSERT_LIBRARIES=./hook.dylib ./HotlineMiami2 "$@"