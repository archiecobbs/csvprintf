#!/bin/bash

# Bail on error
set -e

# Setup temporary files
TMP_STDOUT_EXPECTED='csvprintf-test-out-expected.tmp'
TMP_STDERR_EXPECTED='csvprintf-test-err-expected.tmp'
TMP_STDOUT_ACTUAL='csvprintf-test-out-actual.tmp'
TMP_STDERR_ACTUAL='csvprintf-test-err-actual.tmp'
TMP_SWAP_FILE=''csvprintf-test-hexdump.tmp
trap "rm -f \
    ${TMP_STDOUT_EXPECTED} \
    ${TMP_STDERR_EXPECTED} \
    ${TMP_STDOUT_ACTUAL} \
    ${TMP_STDERR_ACTUAL} \
    ${TMP_SWAP_FILE}" 0 2 3 5 10 13 15

# Convert a file to hexdump version
hexdumpify()
{
    FILE="${1}"
    hexdump -C < "${FILE}" > "${TMP_SWAP_FILE}"
    mv "${TMP_SWAP_FILE}" "${FILE}"
}

# Compare files, on failure set ${DIFF_FAIL}
checkdiff()
{
    if [ "${1}" = '-h' ]; then
        HEXDUMPIFY='true'
        shift
    else
        HEXDUMPIFY='false'
    fi
    TESTFILE="${1}"
    WHAT="${2}"
    EXPECTED="${3}"
    ACTUAL="${4}"
    if diff -q "${EXPECTED}" "${ACTUAL}" >/dev/null; then
        return 0
    fi
    echo "test: ${TESTFILE}: ${WHAT} mismatch"
    echo '------------------------------------------------------'
    if [ "${HEXDUMPIFY}" = 'true' ]; then
        hexdumpify "${EXPECTED}"
        hexdumpify "${ACTUAL}"
    fi
    diff -u "${EXPECTED}" "${ACTUAL}" || true
    echo '------------------------------------------------------'
    DIFF_FAIL='true'
}

# Execute one test, on failure set ${TEST_FAIL}
runtest()
{
    # Read test data
    unset FLAGS
    unset STDIN
    unset STDOUT
    unset STDERR
    unset EXITVAL
    . "${TESTFILE}"
    if [ -z "${FLAGS+x}" \
      -o -z "${STDIN+x}" \
      -o -z "${STDOUT+x}" \
      -o -z "${STDERR+x}" \
      -o -z "${EXITVAL+x}" ]; then
        echo "test: ${TESTFILE}: invalid test file"
        exit 1
    fi

    # Set up files
    echo -en "${STDOUT}" > "${TMP_STDOUT_EXPECTED}"
    echo -en "${STDERR}" > "${TMP_STDERR_EXPECTED}"
    set +e
    echo -en "${STDIN}" | ../csvprintf ${FLAGS} >"${TMP_STDOUT_ACTUAL}" 2>"${TMP_STDERR_ACTUAL}"
    ACTUAL_EXITVAL="$?"
    set -e

    # Special hacks
    if [ "${STDERR}" = '!USAGE!' ]; then
        ../csvprintf --help 2>"${TMP_STDERR_EXPECTED}"
    fi

    # Check result
    DIFF_FAIL='false'
    if [ "${STDOUT}" != '!IGNORE!' ]; then
        checkdiff -h "${TESTFILE}" "standard output" "${TMP_STDOUT_EXPECTED}" "${TMP_STDOUT_ACTUAL}"
    fi
    checkdiff "${TESTFILE}" "standard error" "${TMP_STDERR_EXPECTED}" "${TMP_STDERR_ACTUAL}"
    if [ "${DIFF_FAIL}" != 'false' ]; then
        TEST_FAIL='true'
    fi
    if [ "${ACTUAL_EXITVAL}" -ne "${EXITVAL}" ]; then
        echo "test: ${TESTFILE}: exit value ${ACTUAL_EXITVAL} != ${EXITVAL}"
        TEST_FAIL='true'
    fi

    # Print success or if failure show params
    if [ "${TEST_FAIL}" = 'false' ]; then
        echo "test: ${TESTFILE}: success"
    else
        echo "******************************************************"
        echo "test: ${TESTFILE} FAILED with:"
        echo "  FLAGS='${FLAGS}'"
        echo "  STDIN='${STDIN}'"
        echo "******************************************************"
    fi
}

# Find all tests and run them
ANY_FAIL='false'
for TESTFILE in `find . -maxdepth 1 -type f -name 'test-*.tst' | sort | sed 's|^./||g'`; do
    TEST_FAIL='false'
    runtest "${TESTFILE}"
    if [ "${TEST_FAIL}" != 'false' ]; then
        ANY_FAIL='true'
    fi
done

# Exit with error if any test failed
if [ "${ANY_FAIL}" != 'false' ]; then
    exit 1
fi
