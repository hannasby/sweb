#!/bin/sh

SOURCE_DIR="$1"
OUTPUT_FILE="$2"

generate () {
  echo "// Automatically generated by gen_to_string.sh at $(date)"
  echo "// DO NOT EDIT"
  echo
  echo '#include "internal.hh"'
  echo
  echo 'DWARFPP_BEGIN_NAMESPACE'
  echo
  PYTHON=python
  command -v $PYTHON > /dev/null 2>&1
  if [ $? -ne 0 ]; then
    PYTHON=python3
  fi
  $PYTHON "$SOURCE_DIR"/../elf/enum-print.py < "$SOURCE_DIR"/dwarf++.hh
  $PYTHON "$SOURCE_DIR"/../elf/enum-print.py -s _ -u --hex -x hi_user -x lo_user < "$SOURCE_DIR"/data.hh
  echo 'DWARFPP_END_NAMESPACE'
}

generate > "$OUTPUT_FILE"
