#!/usr/bin/env bash
tar -cf  temp.tar main.cpp Makefile
gzip -c -1 temp.tar > xbures29.tar.gz
rm temp.tar
#tar -xvzf ../tst.tar.gz