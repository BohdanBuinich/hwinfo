#!/usr/bin/env bash
#
# This script checks (and optionally fixes) code style using clang-format
# on C/C++ source files in specified directories.

set -euo pipefail  # Exit on errors, unset variables, or pipeline errors.

###############################################################################
# Global Variables
###############################################################################
APPLY_CHANGES=false  # If true, automatically applies clang-format changes.
ERROR_FOUND=0        # Tracks how many files fail the style check.

###############################################################################
# Helper Functions
###############################################################################

usage() {
    cat <<EOF
Usage: $(basename "$0") [--apply]

Optional Arguments:
  --apply    Automatically apply clang-format changes to any files with issues.
EOF
}

check_clang_format_installed() {
    echo -e "\x1b[34mChecking clang-format version...\x1b[m"
    if ! clang-format --version; then
        echo -e "\x1b[31mclang-format is not installed or not in the PATH. Please install it and try again.\x1b[m"
        exit 1
    fi
}

collect_source_files() {
    # Collect source files: .h, .cpp, .hpp, .cxx, etc.
    # Using a null-delimited approach to safely handle any filenames with spaces.
    local -a files
    while IFS= read -r -d '' file; do
        files+=( "$file" )
    done < <(find ./src/ ./test/ ./include/ -regextype egrep -regex '.*\.(h|c)(pp|xx)?$' -print0)
    echo "${files[@]}"
}

format_and_check_file() {
    local source_file="$1"

    # Obtain the clang-format proposed changes in XML format:
    local diff_output
    diff_output="$(clang-format -output-replacements-xml "$source_file")"

    # If there's a <replacement> tag in the XML, then changes are needed.
    if echo "$diff_output" | grep -q "<replacement "; then
        if [ "$APPLY_CHANGES" = true ]; then
            # Apply the changes automatically
            clang-format -i "$source_file"
            echo -e "Checking $source_file: \x1b[33mFIXED & UPDATED\x1b[m"
        else
            echo -e "Checking $source_file: \x1b[31mFAILED!\x1b[m"
            echo -e "Proposed changes by clang-format for file: $source_file"

            # "|| true" so diff's non-zero exit won't kill the script
            diff -u "$source_file" <(clang-format "$source_file") || true

            # Increment the count of files with style issues
            ERROR_FOUND=$(( ERROR_FOUND + 1 ))
        fi
    else
        echo -e "Checking $source_file: \x1b[32mPASSED\x1b[m"
    fi
}

###############################################################################
# Main Script
###############################################################################

# Parse command-line arguments
if [[ $# -gt 0 ]]; then
    case "$1" in
        --apply)
            APPLY_CHANGES=true
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            echo -e "\x1b[31mUnknown option: $1\x1b[m"
            usage
            exit 1
            ;;
    esac
fi

check_clang_format_installed

echo -e "\n\x1b[34mChecking sources for code style...\x1b[m"
SOURCE_FILES=( $(collect_source_files) )

# Log how many files we are about to check
echo -e "Found ${#SOURCE_FILES[@]} source files to check."

# Iterate over each source file and check style
for source_file in "${SOURCE_FILES[@]}"; do
    format_and_check_file "$source_file"
done

# Summary of results
if [ "$APPLY_CHANGES" = false ]; then
    if [ "$ERROR_FOUND" -eq 0 ]; then
        echo -e "\n\x1b[32m✅ Congratulations! All sources match the code style.\x1b[m"
    else
        echo -e "\n\x1b[31m❌ clang-format discovered style issues in $ERROR_FOUND file(s).\x1b[m"
        echo -e "\x1b[34mSuggestion: To apply changes automatically, rerun with the '--apply' flag.\x1b[m"
        exit 1
    fi
else
    echo -e "\n\x1b[32m✅ All formatting issues have been fixed and applied successfully.\x1b[m"
fi
