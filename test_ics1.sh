#!/bin/bash
./test_ics1 $srcdir/test/testim.ics result_v1.ics && cmp -s $srcdir/test/testim.ids result_v1.ids && (( `diff $srcdir/test/testim.ics result_v1.ics | grep -v filename | grep -v -- --- | grep -v 3c3 | wc -l` == 0 ))
