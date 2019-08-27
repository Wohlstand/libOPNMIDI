#!/bin/bash

WOPN=wopn
gcc program2wopn.c wopn/wopn_file.c -I${WOPN} -o program2wopn
./program2wopn
rm program2wopn

