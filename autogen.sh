#!/bin/sh

set -xe

autoreconf -vi
mkdir -p build
cd build
../configure
