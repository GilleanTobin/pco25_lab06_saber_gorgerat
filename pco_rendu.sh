#!/bin/bash

REPORT_FILE=rapport.pdf
ARCHIVE=rendu.tar.gz

# Vérif du rapport
if [ ! -f "$REPORT_FILE" ]; then
    echo "Could not find $REPORT_FILE" >&2
    exit 1
fi

echo "The following files are archived in $ARCHIVE :"

tar -czvf "$ARCHIVE" \
    CMakeLists.txt \
    "$REPORT_FILE" \
    $(find src test include -name "*.cpp" -o -name "*.h")
