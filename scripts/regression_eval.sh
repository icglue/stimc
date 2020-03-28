#!/bin/bash

n_pass=0
n_test=1

while read line ; do
    # nothing
    if echo $line | grep -e '^PASSED: [0-9]\+ out of [0-9]\+$' > /dev/null ; then
        n_pass=$(echo $line | sed -e 's/^PASSED: \([0-9]\+\) out of \([0-9]\+\)$/\1/')
        n_test=$(echo $line | sed -e 's/^PASSED: \([0-9]\+\) out of \([0-9]\+\)$/\2/')
    fi
done

#echo "passed: $n_pass"
#echo "total:  $n_test"

if [ "$n_pass" -eq "$n_test" ] ; then
    echo "Regression test passed"
    exit 0
else
    echo "Regression test failed"
    exit 1
fi
