#!/bin/sh

set -e
set -o pipefail

FAILED_TESTS=''
for INPUT_FILE in *.in; do
    OUTPUT_FILE1=`echo "${INPUT_FILE}" | sed -n 's/\.in$/.out1/gp'`
    OUTPUT_FILE2=`echo "${INPUT_FILE}" | sed -n 's/\.in$/.out2/gp'`
    OUTPUT_FILE3A=`echo "${INPUT_FILE}" | sed -n 's/\.in$/.out3a/gp'`
    OUTPUT_FILE3B=`echo "${INPUT_FILE}" | sed -n 's/\.in$/.out3b/gp'`
    OUTPUT_FILE4=`echo "${INPUT_FILE}" | sed -n 's/\.in$/.out4/gp'`
    OUTPUT_FILE5A=`echo "${INPUT_FILE}" | sed -n 's/\.in$/.out5a/gp'`
    OUTPUT_FILE5B=`echo "${INPUT_FILE}" | sed -n 's/\.in$/.out5b/gp'`
    echo "*** testing ${INPUT_FILE}..." 1>&2
    if ! ../csvprintf -x -f "${INPUT_FILE}" | diff -u "${OUTPUT_FILE1}" -; then
        echo "*** FAILED: [1] ${INPUT_FILE}" 1>&2
        FAILED_TESTS="${FAILED_TESTS} ${INPUT_FILE}/${OUTPUT_FILE1}"
    fi
    if ! ../csvprintf -X -f "${INPUT_FILE}" | diff -u "${OUTPUT_FILE2}" -; then
        echo "*** FAILED: [2] ${INPUT_FILE}" 1>&2
        FAILED_TESTS="${FAILED_TESTS} ${INPUT_FILE}/${OUTPUT_FILE2}"
    fi
    if ! ../csvprintf -e iso-8859-1 -x -f "${INPUT_FILE}" | xsltproc ../csv.xsl - | ../csvprintf -e UTF-8 -x | diff -u "${OUTPUT_FILE1}" -; then
        echo "*** FAILED: [1x] ${INPUT_FILE}" 1>&2
        FAILED_TESTS="${FAILED_TESTS} ${INPUT_FILE}/csv2xml"
    fi
    if ! ../csvprintf -j -f "${INPUT_FILE}" | diff -u "${OUTPUT_FILE3A}" -; then
        echo "*** FAILED: [3a] ${INPUT_FILE}" 1>&2
        FAILED_TESTS="${FAILED_TESTS} ${INPUT_FILE}/${OUTPUT_FILE3A}"
    fi
    if ! ../csvprintf -ij -f "${INPUT_FILE}" | diff -u "${OUTPUT_FILE3B}" -; then
        echo "*** FAILED: [3b] ${INPUT_FILE}" 1>&2
        FAILED_TESTS="${FAILED_TESTS} ${INPUT_FILE}/${OUTPUT_FILE3B}"
    fi
    if ! ../csvprintf -ix -f "${INPUT_FILE}" | diff -u "${OUTPUT_FILE4}" -; then
        echo "*** FAILED: [4] ${INPUT_FILE}" 1>&2
        FAILED_TESTS="${FAILED_TESTS} ${INPUT_FILE}/${OUTPUT_FILE4}"
    fi
    if ! ../csvprintf -b -f "${INPUT_FILE}" | diff -u "${OUTPUT_FILE5A}" -; then
        echo "*** FAILED: [5a] ${INPUT_FILE}" 1>&2
        FAILED_TESTS="${FAILED_TESTS} ${INPUT_FILE}/${OUTPUT_FILE5A}"
    fi
    if ! ../csvprintf -ib -f "${INPUT_FILE}" | diff -u "${OUTPUT_FILE5B}" -; then
        echo "*** FAILED: [5b] ${INPUT_FILE}" 1>&2
        FAILED_TESTS="${FAILED_TESTS} ${INPUT_FILE}/${OUTPUT_FILE5B}"
    fi
done

if [ -z "${FAILED_TESTS}" ]; then
    echo "*** all tests passed"
else
    echo "*** test(s) failed:${FAILED_TESTS}"
    exit 1
fi

