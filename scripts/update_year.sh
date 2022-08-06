#!/bin/bash

die() {
    echo $@
    exit 1
}

YEAR=$(date '+%Y')

echo "updating year ${YEAR}"

FILES_DATE=$(find . -type f -and '(' -name \*.c -or -name \*.h -or -name \*.cpp -or -name \*.inl -or -iname Makefile -or -iname README\* ')' )

# copyright ...-<current> date
for f in $FILES_DATE ; do
    sed -r -e 's#(Copyright[^0-9]*[0-9]{4})(-?)([0-9]{4})?#\1-'${YEAR}'#' -i $f
done

