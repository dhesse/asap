#!/bin/bash
rm -f configure
mkdir -p m4
autoreconf --install || exit 1
