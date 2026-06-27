#!/usr/bin/env bash

# CRC XOR Testing Script (CRC3/4/8)
# Supports: crcxor.c, crcxor.cpp, crcxor.rs or built executable

set -euo pipefail

# Check if source file argument is provided
if [ $# -ne 1 ]; then
    echo "Usage: $0 <source_file>"
    echo "Supported files: crcxor.c, crcxor.cpp, crcxor.rs"
    echo "You may also pass a compiled executable."
    exit 1
fi

# Color codes for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

print_status() {
    local color=$1
    local message=$2
    echo -e "${color}${message}${NC}"
}

now_seconds() {
    if [ -x /usr/bin/perl ]; then
        /usr/bin/perl -MTime::HiRes -e 'print Time::HiRes::time()'
        return 0
    fi
    date +%s
}

format_seconds() {
    local value=$1
    if command -v awk >/dev/null 2>&1; then
        awk -v v="$value" 'BEGIN { printf "%.3f", v }'
    else
        echo "$value"
    fi
}

run_with_timeout() {
    local timeout=$1
    shift

    if command -v gtimeout >/dev/null 2>&1; then
        gtimeout "$timeout" "$@"
        return $?
    fi

    if command -v timeout >/dev/null 2>&1; then
        timeout "$timeout" "$@"
        return $?
    fi

    if [ -x /usr/bin/perl ]; then
        /usr/bin/perl -e 'alarm shift; exec @ARGV' "$timeout" "$@"
        return $?
    fi

    print_status $YELLOW "WARN: Timeout tool not found; running without a timeout."
    "$@"
}

is_timeout_status() {
    case $1 in
        124|137|142|143) return 0 ;;
        *) return 1 ;;
    esac
}

cleanup() {
    if [ -n "$TEMP_EXE" ] && [ -f "$TEMP_EXE" ]; then
        rm -f "$TEMP_EXE"
    fi
}
trap cleanup EXIT

INPUT_PATH="$1"
if [ ! -e "$INPUT_PATH" ]; then
    echo "Error: input not found: $INPUT_PATH"
    exit 1
fi

BASE_DIR="$(cd "$(dirname "$0")" && pwd)"
EXPECTED_DIR="$BASE_DIR/expected_outputs"
TEST_CASES_DIR="$BASE_DIR/test_cases"
STUDENT_OUTPUT_DIR="$BASE_DIR/student_output"
FAILED_OUTPUT_DIR="$BASE_DIR/failed_cases"
COMPILE_TIMEOUT_SEC=20
RUN_TIMEOUT_SEC=5
SOURCE_BASENAME="crcxor"

if [ ! -d "$EXPECTED_DIR" ]; then
    print_status $RED "FAIL: expected_outputs directory not found!"
    echo "Please run the expected output generator first."
    exit 1
fi

if [ ! -d "$TEST_CASES_DIR" ]; then
    print_status $RED "FAIL: test_cases directory not found!"
    exit 1
fi

if [ "${INPUT_PATH: -2}" = ".c" ] && [ "$(basename -- "$INPUT_PATH")" != "$SOURCE_BASENAME.c" ] && [ "$(basename -- "$INPUT_PATH")" != "$SOURCE_BASENAME.cpp" ] && [ "$(basename -- "$INPUT_PATH")" != "$SOURCE_BASENAME.rs" ]; then
    print_status $RED "FAIL: Unsupported source file name: $(basename -- "$INPUT_PATH")"
    echo "Expected $SOURCE_BASENAME.c, $SOURCE_BASENAME.cpp, or $SOURCE_BASENAME.rs"
    exit 1
fi

BASE_NAME="$(basename -- "$INPUT_PATH")"
EXT="${BASE_NAME##*.}"

EXE=""
TEMP_EXE=""
LANG=""

# Determine source file type and compile accordingly
case "$BASE_NAME" in
    "$SOURCE_BASENAME.c")
        TEMP_EXE="$(mktemp /tmp/crcxor_test_XXXXXX)"
        print_status $BLUE "INFO: Compiling C source file: $INPUT_PATH"
        run_with_timeout "$COMPILE_TIMEOUT_SEC" gcc -Wall -Wextra -std=c99 -O2 "$INPUT_PATH" -o "$TEMP_EXE" -lm
        compile_status=$?
        if is_timeout_status "$compile_status"; then
            print_status $RED "FAIL: Compilation timed out after ${COMPILE_TIMEOUT_SEC}s"
            exit 1
        fi
        if [ $compile_status -ne 0 ]; then
            print_status $RED "FAIL: Compilation of $SOURCE_BASENAME.c failed"
            exit 1
        fi
        EXE="$TEMP_EXE"
        LANG="C"
        ;;
    "$SOURCE_BASENAME.cpp")
        TEMP_EXE="$(mktemp /tmp/crcxor_test_XXXXXX)"
        print_status $BLUE "INFO: Compiling C++ source file: $INPUT_PATH"
        run_with_timeout "$COMPILE_TIMEOUT_SEC" g++ -Wall -Wextra -std=c++17 -O2 "$INPUT_PATH" -o "$TEMP_EXE"
        compile_status=$?
        if is_timeout_status "$compile_status"; then
            print_status $RED "FAIL: Compilation timed out after ${COMPILE_TIMEOUT_SEC}s"
            exit 1
        fi
        if [ $compile_status -ne 0 ]; then
            print_status $RED "FAIL: Compilation of $SOURCE_BASENAME.cpp failed"
            exit 1
        fi
        EXE="$TEMP_EXE"
        LANG="C++"
        ;;
    "$SOURCE_BASENAME.rs")
        TEMP_EXE="$(mktemp /tmp/crcxor_test_XXXXXX)"
        print_status $BLUE "INFO: Compiling Rust source file: $INPUT_PATH"
        run_with_timeout "$COMPILE_TIMEOUT_SEC" rustc -O "$INPUT_PATH" -o "$TEMP_EXE"
        compile_status=$?
        if is_timeout_status "$compile_status"; then
            print_status $RED "FAIL: Compilation timed out after ${COMPILE_TIMEOUT_SEC}s"
            exit 1
        fi
        if [ $compile_status -ne 0 ]; then
            print_status $RED "FAIL: Compilation of $SOURCE_BASENAME.rs failed"
            exit 1
        fi
        EXE="$TEMP_EXE"
        LANG="Rust"
        ;;
    *)
        if [ -x "$INPUT_PATH" ]; then
            EXE="$INPUT_PATH"
            LANG="Binary"
        else
            print_status $RED "FAIL: Unsupported input. Expected $SOURCE_BASENAME.c, $SOURCE_BASENAME.cpp, $SOURCE_BASENAME.rs, or a compiled executable"
            exit 1
        fi
        ;;
esac

print_status $GREEN "PASS: Compilation/Input validation succeeded using $LANG"
echo ""

# Create output directories (fresh each run)
rm -rf "$STUDENT_OUTPUT_DIR" "$FAILED_OUTPUT_DIR"
mkdir -p "$STUDENT_OUTPUT_DIR" "$FAILED_OUTPUT_DIR"

# Initialize test counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

MESSAGE_FILES=()
while IFS= read -r file; do
    MESSAGE_FILES+=("$file")
done < <(find "$TEST_CASES_DIR" -type f -name 'message_*.txt' | sort -V)

if [ ${#MESSAGE_FILES[@]} -eq 0 ]; then
    print_status $RED "FAIL: no message_*.txt files found in $TEST_CASES_DIR"
    exit 1
fi

record_failure() {
    local test_name=$1
    local reason=$2
    local msg_file=$3
    local expected_file=$4
    local student_output=$5

    local case_dir="$FAILED_OUTPUT_DIR/$test_name"
    mkdir -p "$case_dir"
    echo "Test Case: $test_name" > "$case_dir/info.txt"
    echo "Reason: $reason" >> "$case_dir/info.txt"
    echo "Message File: $msg_file" >> "$case_dir/info.txt"
    echo "Expected: $expected_file" >> "$case_dir/info.txt"
    echo "Student Output: $student_output" >> "$case_dir/info.txt"
    if [ -f "$student_output" ]; then
        cp "$student_output" "$case_dir/output.txt"
    fi
}

run_test_case() {
    local msg_file=$1
    local crc=$2

    local msg_name
    msg_name="$(basename -- "$msg_file")"
    local msg_id="${msg_name#message_}"
    msg_id="${msg_id%.txt}"
    local test_name="message${msg_id}_crc${crc}"

    local expected_file="$EXPECTED_DIR/${test_name}.txt"
    local student_output="$STUDENT_OUTPUT_DIR/${test_name}_output.txt"

    TOTAL_TESTS=$((TOTAL_TESTS + 1))

    print_status $PURPLE "INFO: Test Case: $test_name"

    if [ ! -f "$expected_file" ]; then
        print_status $RED "   FAIL: expected output missing: $expected_file"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        record_failure "$test_name" "Missing expected result file" "$msg_file" "$expected_file" "$student_output"
        return 1
    fi

    run_with_timeout "$RUN_TIMEOUT_SEC" "$EXE" "$msg_file" "$crc" > "$student_output" 2> "$student_output.err"
    run_status=$?

    if is_timeout_status $run_status; then
        print_status $RED "   FAIL: Program timed out after ${RUN_TIMEOUT_SEC}s"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        record_failure "$test_name" "Program timed out" "$msg_file" "$expected_file" "$student_output"
        return 1
    fi

    if [ $run_status -ne 0 ]; then
        print_status $RED "   FAIL: Program execution failed"
        if [ -s "$student_output.err" ]; then
            cat "$student_output.err" > "$FAILED_OUTPUT_DIR/$test_name/stderr.txt"
        fi
        FAILED_TESTS=$((FAILED_TESTS + 1))
        record_failure "$test_name" "Program execution failed" "$msg_file" "$expected_file" "$student_output"
        return 1
    fi

    if diff -B -w "$expected_file" "$student_output" >/dev/null 2>&1; then
        print_status $GREEN "   PASS: Output matches expected"
        PASSED_TESTS=$((PASSED_TESTS + 1))
        return 0
    else
        print_status $RED "   FAIL: Output differs from expected"
        echo "   First few differences:"
        diff -B -w -u "$expected_file" "$student_output" | sed -n '1,10p'
        FAILED_TESTS=$((FAILED_TESTS + 1))
        record_failure "$test_name" "Output mismatch" "$msg_file" "$expected_file" "$student_output"
        diff -B -w -u "$expected_file" "$student_output" > "$FAILED_OUTPUT_DIR/$test_name/diff.txt"
        return 1
    fi
}

print_status $CYAN "INFO: Starting CRC3/4/8 test evaluation..."
echo "============================================================"

START_TIME=$(now_seconds)
for msg_file in "${MESSAGE_FILES[@]}"; do
    run_test_case "$msg_file" 3
    run_test_case "$msg_file" 4
    run_test_case "$msg_file" 8
done
END_TIME=$(now_seconds)

TOTAL_TIME_RAW=$(awk -v end="$END_TIME" -v start="$START_TIME" 'BEGIN { printf "%.6f", (end - start) }')
if [ "$TOTAL_TESTS" -gt 0 ]; then
    PASS_RATE_RAW=$(awk -v passed="$PASSED_TESTS" -v total="$TOTAL_TESTS" 'BEGIN { printf "%.2f", (passed / total) * 100 }')
    AVG_TIME_RAW=$(awk -v total="$TOTAL_TIME_RAW" -v count="$TOTAL_TESTS" 'BEGIN { printf "%.6f", (total / count) }')
else
    PASS_RATE_RAW="0.00"
    AVG_TIME_RAW="0"
fi

echo "============================================================"
SUMMARY_COLOR=$GREEN
if [ $FAILED_TESTS -ne 0 ]; then
    SUMMARY_COLOR=$RED
fi
print_status $SUMMARY_COLOR "Test Summary"
echo "   Total Tests:       $TOTAL_TESTS"
echo "   Passed:            $PASSED_TESTS"
echo "   Failed:            $FAILED_TESTS"
echo "   Pass Rate:         ${PASS_RATE_RAW}%"
echo "   Grade:             ${PASS_RATE_RAW}%"
echo "   Total Time:        $(format_seconds "$TOTAL_TIME_RAW") s"
echo "   Avg Time per Test: $(format_seconds "$AVG_TIME_RAW") s"

echo ""
if [ "$FAILED_TESTS" -ne 0 ]; then
    print_status $RED "Result: TESTS FAILED"
    exit 1
fi

print_status $GREEN "Result: ALL TESTS PASSED"
exit 0
