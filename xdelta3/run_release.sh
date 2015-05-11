#!/bin/sh

EXTRA=$*

# Choose
CC=clang 
CXX=clang++
# or
#CC=gcc
#CXX=g++

# Place C/C++ common flags here
COMMON="-g"

export CFLAGS
export CXXFLAGS
export CC
export CXX
export LDFLAGS

LIBBASE=$HOME/lib

# Run from the source dir.
SRCDIR=$PWD

rm -rf build

function setup {
    aclocal -I m4
    automake
    automake --add-missing
    autoconf
}

function buildit {
    machine=$1
    offsetbits=$2
    args=$3
    D=build/${machine}/xoff${offsetbits}
    CFLAGS="$COMMON -${machine} ${args} -I$LIBBASE/${machine}/include"
    CXXFLAGS="$COMMON -${machine} ${args} -I$LIBBASE/${machine}/include"
    LDFLAGS="$COMMON -${machine} ${args} -L$LIBBASE/${machine}/lib"
    echo CFLAGS=$CFLAGS
    echo CXXFLAGS=$CXXFLAGS
    echo LDFLAGS=$LDFLAGS    
    mkdir -p $D
    echo For machine=${machine}
    echo For xoff_t=${offsetbits} bits
    
    echo "Configuring $D $EXTRA ..."
    (cd $D && $SRCDIR/configure --prefix=$PWD/bin --enable-debug-symbols $EXTRA)
    echo "Building $D ..."
    (cd $D && make all)
    echo "Testing $D ..."
    (cd $D && ./xdelta3regtest)
}

function buildall {
    buildit $1 32 "-DXD3_USE_LARGEFILE64=0"
    buildit $1 64 "-DXD3_USE_LARGEFILE64=1"
}

buildall m64
buildall m32



