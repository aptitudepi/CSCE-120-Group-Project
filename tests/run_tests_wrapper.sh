#!/bin/bash
# Wrapper script to run tests with system libraries instead of conda's
# This ensures system libstdc++ is used, which is compatible with Qt6

# Remove conda from LD_LIBRARY_PATH and prepend system library paths
if [ -n "$CONDA_PREFIX" ]; then
    # Remove conda/lib from LD_LIBRARY_PATH
    export LD_LIBRARY_PATH=$(echo "$LD_LIBRARY_PATH" | tr ':' '\n' | grep -v "$CONDA_PREFIX/lib" | tr '\n' ':' | sed 's/:$//')
    # Prepend system library paths
    export LD_LIBRARY_PATH="/usr/lib/x86_64-linux-gnu:/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH"
fi

# Run the test executable with all arguments
exec "$@"

