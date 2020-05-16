#!/bin/bash

die() {
    echo $@
    exit 1
}

YEAR=$(date '+%Y')
VERSION=$1

VERSION_MAJOR=$(echo $VERSION | sed -n -r -e 's/^([0-9]+)\.([0-9]+)/\1/p')
VERSION_MINOR=$(echo $VERSION | sed -n -r -e 's/^([0-9]+)\.([0-9]+)/\2/p')

test -n "$VERSION_MAJOR" && test -n "$VERSION_MINOR" || die "invalid version $VERSION"

echo "updating to version ${VERSION_MAJOR}.${VERSION_MINOR} // year ${YEAR}"

FILES_DATE=$(find . -type f -and '(' -name \*.c -or -name \*.h -or -name \*.cpp -or -name \*.inl -or -iname Makefile -or -iname README\* ')' )
FILES_VERSION1=$(find . -type f -and -name Makefile)
FILES_VERSION2=$(find . -type f -and -name \*.c)

# copyright ...-<current> date
for f in $FILES_DATE ; do
    sed -r -e 's#(Copyright[^0-9]*[0-9]{4})(-?)([0-9]{4})?#\1-'${YEAR}'#' -i $f
done

# makefiles 'VERSION'
for f in $FILES_VERSION1 ; do
    sed -r -e 's/^(\s*VERSION\s*[?:]=\s*)[0-9].*?/\1'$VERSION'/' -i $f
done

# library version defines
for f in $FILES_VERSION2 ; do
    sed -r \
        -e 's/^(\s*#define\s+.*_VERSION_MAJOR\s+)[0-9]+\s*$/\1'$VERSION_MAJOR'/' \
        -e 's/^(\s*#define\s+.*_VERSION_MINOR\s+)[0-9]+\s*$/\1'$VERSION_MINOR'/' \
        -i $f
done
