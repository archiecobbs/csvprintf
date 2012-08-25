#!/bin/sh

set -e

for INPUT_FILE in *.in; do
    OUTPUT_FILE=`echo "${INPUT_FILE}" | sed -n 's/\.in$/.out/gp'`
    echo "*** testing ${INPUT_FILE}..." 1>&2
    if ! ../csvprintf -x -f "${INPUT_FILE}" | diff -u "${OUTPUT_FILE}" -; then
        echo "*** FAILED: ${INPUT_FILE}" 1>&2
        exit 1
    fi
done
echo "*** all tests passed"

