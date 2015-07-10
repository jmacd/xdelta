#!/bin/sh

# Place C/C++ common flags here
COMMON="-g"

export CFLAGS
export CXXFLAGS
export LDFLAGS

LIBBASE=$HOME/lib

MINGW_CCFLAGS="-DEXTERNAL_COMPRESSION=0 -DVCDIFF_TOOLS=0 -DXD3_WIN32=1 -DSHELL_TESTS=0"
MINGW_LDFLAGS="-lmingw32"

# Location where mingw-w64-build script runs.
MINGW_BASE="/volume/home/jmacd/src/mingwb"

# Windows 32bit
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
    cargs=$4
    largs=$5
    BM="${host}${march}"
    D=build/$BM/xoff${offsetbits}
    CFLAGS="${COMMON} ${march} ${cargs} -I$LIBBASE/$BM/include"
    CXXFLAGS="${COMMON} ${march} ${cargs} -I$LIBBASE/$BM/include"
    LDFLAGS="${largs} -L$LIBBASE/$BM/lib"
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

    # echo "Building $D ..."
    # (cd $D && make all)
    # echo "Installing $D ..."    
    # (cd $D && make install)
    # echo "Testing $D ..."

    # TODO test if host matches mingw, can't run tests
    # (cd $D && ./bin/xdelta3regtest)
    # (cd $D && ./bin/xdelta3 test)
}

function buildall {
    buildit "$1" "$2" 32 "-DXD3_USE_LARGEFILE64=0 $3" "$4"
    buildit "$1" "$2" 64 "-DXD3_USE_LARGEFILE64=1 $3" "$4"
}

# Linux / OS X 64bit
#buildall x86_64-pc-linux-gnu -m64

# Linux / OS X 32bit
#buildall x86_64-pc-linux-gnu -m32

# Windows 32bit
buildall i686-w64-mingw32 -mconsole "${MINGW_CFLAGS}" "-L${MINGW_BASE}/mingw-w64-i686/lib ${MINGW_LDFLAGS}"

# Windows 64bit
#buildall x86_64-w64-mingw32 -mconsole "${MINGW_CFLAGS}" "-L${MINGW_BASE}/mingw-w64-x86_64/lib ${MINGW_LDFLAGS}"
