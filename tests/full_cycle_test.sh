#!/bin/bash

# TODO: 
# - Store created files in tmp dir

assert_equals() {
  if [[ "$1" == "$2" ]]; then
      echo "Test passed"
      return 0
  else
      echo "Test failed!"
      colordiff  <(echo "$1") <(echo "$2")
      return 1
  fi
}

cd "${0%/*}"/..
curdir="$(pwd)"
echo "Working directory: $curdir"

trap "rm -f ./tests/files/testdict.zip ./tests/files/data.mdb" EXIT

echo "Zipping test dictionary..."
zip -j -r ./tests/files/testdict.zip ./tests/files/testdictionary/dictionary_content/*
echo "Creating index..."
dictpopup-create -d ./tests/files/ -i ./tests/files/
echo "Calling cli..."
output=$(DICTPOPUP_CONFIG_DIR="$curdir" ./dictpopup-cli -d ./tests/files/ "面白い")
echo "Reading expected output.."
expected_output="$(cat ./tests/files/testdictionary/expected_cli_output.txt)"
echo "Asserting equality:"
assert_equals "$output" "$expected_output"
