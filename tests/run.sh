#!/bin/sh

set -e

for INPUT_FILE in *.in; do
    OUTPUT_FILE1=`echo "${INPUT_FILE}" | sed -n 's/\.in$/.out1/gp'`
    OUTPUT_FILE2=`echo "${INPUT_FILE}" | sed -n 's/\.in$/.out2/gp'`
    OUTPUT_FILE3=`echo "${INPUT_FILE}" | sed -n 's/\.in$/.out3/gp'`
    echo "*** testing ${INPUT_FILE}..." 1>&2
    if ! ../csvprintf -x -f "${INPUT_FILE}" | diff -u "${OUTPUT_FILE1}" -; then
        echo "*** FAILED: [1] ${INPUT_FILE}" 1>&2
        exit 1
    fi
    if ! ../csvprintf -X -f "${INPUT_FILE}" | diff -u "${OUTPUT_FILE2}" -; then
        echo "*** FAILED: [2] ${INPUT_FILE}" 1>&2
        exit 1
    fi
    if ! ../csvprintf -e iso-8859-1 -x -f "${INPUT_FILE}" | xsltproc ../csv.xsl - | ../csvprintf -e UTF-8 -x | diff -u "${OUTPUT_FILE1}" -; then
        echo "*** FAILED: [3] ${INPUT_FILE}" 1>&2
        exit 1
    fi
    if ! ../csvprintf -ij -f "${INPUT_FILE}" | diff -u "${OUTPUT_FILE3}" -; then
        echo "*** FAILED: [3] ${INPUT_FILE}" 1>&2
        exit 1
    fi
done
echo "*** all tests passed"

