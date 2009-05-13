#!/bin/zsh
for i in **/*.cpp **/*.h; do cat $i | grep -v 'bla Fortuna' >| TMP_FILE; echo mv TMP_FILE $i; done

