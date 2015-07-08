#!/bin/sh

# Place C/C++ common flags here
COMMON="-g"

export CFLAGS
export CXXFLAGS
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
    host=$1
    march=$2
    offsetbits=$3
    args=$4
    BM="${host}${march}"
    D=build/$BM/xoff${offsetbits}
    CFLAGS="$COMMON ${march} ${args} -I$LIBBASE/$BM/include"
    CXXFLAGS="$COMMON ${march} ${args} -I$LIBBASE/$BM/include"
    LDFLAGS="$COMMON ${march} ${args} -L$LIBBASE/$BM/lib"
    echo CFLAGS=$CFLAGS
    echo CXXFLAGS=$CXXFLAGS
    echo LDFLAGS=$LDFLAGS
    mkdir -p $D
    echo For build=$BM
    echo For xoff_t=${offsetbits} bits

    echo "Configuring $D ..."
    (cd $D && $SRCDIR/configure \
		  --host=${host} \
		  --prefix=$PWD \
		  --enable-debug-symbols)
    echo "Building $D ..."
    (cd $D && make all)
    echo "Installing $D ..."    
    (cd $D && make install)
    echo "Testing $D ..."

    # TODO test if host matches mingw, can't run tests
    # (cd $D && ./bin/xdelta3regtest)
    (cd $D && ./bin/xdelta3 test)
}

function buildall {
    buildit "$1" "$2" 32 "-DXD3_USE_LARGEFILE64=0 $3"
    buildit "$1" "$2" 64 "-DXD3_USE_LARGEFILE64=1 $3"
}

#buildall x86_64-pc-linux-gnu -m64
#buildall x86_64-pc-linux-gnu -m32

MINGW_FLAGS="-DEXTERNAL_COMPRESSION=0 -DVCDIFF_TOOLS=0 -DXD3_WIN32=1 -DSHELL_TESTS=0"

buildall i686-w64-mingw32 -m32 "${MINGW_FLAGS}"
buildall x86_64-w64-mingw32 -m64 "${MINGW_FLAGS}"
