#!/bin/sh

# Choose
CC=clang 
CXX=clang++
# or
CC=gcc
CXX=g++

# These are updated below, 
CFLAGS=     # Do not set here
CXXFLAGS=   # Do not set here

# Place C/C++ common flags here
COMMON=-O3

export CFLAGS
export CXXFLAGS
export CC
export CXX

LIBBASE=$HOME/lib

# Run from the source dir.
# Looks for liblzma to be installed in $LIBBASE/$MACH.
MACH=
SRCDIR=$PWD

rm -rf build

# aclocal -I m4
# automake --force-missing
# automake
# autoconf

function resetflag {
    CFLAGS="$COMMON -$1 -I$LIBBASE/$MACH/include"
    CXXFLAGS="$COMMON -$1 -I$LIBBASE/$MACH/include"
    LDFLAGS="$COMMON -$1 -L$LIBBASE/$MACH/lib"
}    

function addflag {
    CFLAGS="$CFLAGS $1"
    CXXFLAGS="$CXXFLAGS $1"
}

function buildit {
    sizebits=$1
    offsetbits=$2
    echo Configuration for usize_t=${sizebits} bits, xoff_t=${offsetbits} bits.
    echo CFLAGS=$CFLAGS
    echo CXXFLAGS=$CXXFLAGS
    D=build/$MACH/${sizebits}size-${offsetbits}off
    mkdir -p $D
    (cd $D && $SRCDIR/configure --prefix=$PWD/bin --enable-debug-symbols)
    (cd $D && make xdelta3checksum)
}

function buildall {
    MACH=$1
    resetflag $MACH
    addflag -DXD3_USE_LARGEFILE64=0
    addflag -DXD3_USE_LARGEWINDOW64=0
    buildit 32 32

    resetflag $MACH
    addflag -DXD3_USE_LARGEFILE64=1
    addflag -DXD3_USE_LARGEWINDOW64=0
    buildit 32 64

    resetflag $MACH
    addflag -DXD3_USE_LARGEFILE64=1
    addflag -DXD3_USE_LARGEWINDOW64=1
    buildit 64 64
}

buildall m64
buildall m32
