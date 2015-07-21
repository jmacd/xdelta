#!/bin/sh

# TODO build liblzma for each host

# Place C/C++ common flags here
COMMON="-g"

export CFLAGS
export CXXFLAGS
export LDFLAGS

# TODO replace w/ wget
LZMA="xz-5.2.1"
LZMA_FILE="/volume/home/jmacd/src/xdelta-devel/xz-5.2.1.tar.xz"

MYOS=`uname`

BUILDDIR=${PWD}/build
LZMASRC=${BUILDDIR}/${LZMA}

LIBBASE=$HOME/lib

MINGW_CFLAGS="-DEXTERNAL_COMPRESSION=0 -DVCDIFF_TOOLS=0 -DXD3_WIN32=1 -DSHELL_TESTS=0"

MINGW_LDFLAGS=""

# Location where mingw-w64-build script runs.
MINGW_BASE="/volume/home/jmacd/src/mingwb"

# Windows 32bit
# Run from the source dir.
SRCDIR=$PWD

rm -rf build/*/*/*

function setup {
    aclocal -I m4
    automake
    automake --add-missing
    autoconf
}

function try {
    local w=$1
    shift
    local dir=$1
    shift
    echo -n "Running ${w} in ${dir}"
    (cd "${dir}" && "$@" >$w.stdout 2>$w.stderr)
    local s=$?
    if [ $s -eq 0 ]; then
	echo " success"
    else
	echo " failed!"
	echo "Error $1 in ${dir}" >&2
    fi
    return $s
}

try untar-xz ${BUILDDIR} tar -xvf "${LZMA_FILE}"
if [ $? -ne 0 ]; then
    return
fi

function buildlzma {
    host=$1
    march=$2
    local target="${BUILDDIR}/lib-${host}${march}"

    mkdir -p $target

    try configure ${target} ${LZMASRC}/configure \
	--host=${host} \
	--prefix=${target} \
	"CFLAGS=${march}" \
	"CXXFLAGS=${march}" \
	"LDFLAGS=${march}"
    if [ $? -ne 0 ]; then
	return
    fi

    try build-xz ${target} make
    if [ $? -ne 0 ]; then
	return
    fi
    try install-xz ${target} make install
    if [ $? -ne 0 ]; then
	return
    fi
}

function buildit {
    host=$1
    march=$2
    offsetbits=$3
    cargs=$4
    largs=$5
    BM="${host}${march}"
    D="build/$BM/xoff${offsetbits}"
    FULLD="$PWD/$D"
    CFLAGS="${COMMON} ${march} ${cargs} -I${PWD}/build/lib-${BM}/include"
    CXXFLAGS="${COMMON} ${march} ${cargs} -I${PWD}/build/lib-${BM}/include"
    LDFLAGS="${largs} ${march} -L${PWD}/build/lib-${BM}/lib"
    #echo CFLAGS=$CFLAGS
    #echo CXXFLAGS=$CXXFLAGS
    #echo LDFLAGS=$LDFLAGS
    mkdir -p $D

    echo "For ${BM}-xoff${offsetbits}"

    try configure $FULLD $SRCDIR/configure \
		  --host=${host} \
		  --prefix=${FULLD} \
		  --enable-static \
		  --disable-shared \
		  --enable-debug-symbols
    if [ $? -ne 0 ]; then
	return
    fi

    try build $FULLD make all
    if [ $? -ne 0 ]; then
	return
    fi

    if echo "$host" | grep -i "$MYOS" >/dev/null; then
	try install $FULLD make install
	if [ $? -ne 0 ]; then
	    return
	fi

	(cd $D && ./xdelta3regtest 1> regtest.stdout 2> regtest.stderr&)
	(cd $D && ./bin/xdelta3 test 1> selftest.stdout 2> selftest.stderr&)
    else
	echo "To test:"
	echo "cd ${FULLD} && ./xdelta3 test"
    fi
}

function buildall {
    echo "Building for $1 $2 ..."
    buildlzma "$1" "$2"
    if [ $? -ne 0 ]; then
	return
    fi
    buildit "$1" "$2" 32 "-DXD3_USE_LARGEFILE64=0 $3" "$4"
    if [ $? -ne 0 ]; then
	return
    fi
    buildit "$1" "$2" 64 "-DXD3_USE_LARGEFILE64=1 $3" "$4"
    if [ $? -ne 0 ]; then
	return
    fi
}

# Linux
buildall x86_64-pc-linux-gnu -m32
buildall x86_64-pc-linux-gnu -m64

# Windows
buildall i686-w64-mingw32 -mconsole \
	 "${MINGW_CFLAGS}" \
	 "-L${MINGW_BASE}/mingw-w64-i686/lib ${MINGW_LDFLAGS}"
buildall x86_64-w64-mingw32 -mconsole \
	 "${MINGW_CFLAGS}" \
	 "-L${MINGW_BASE}/mingw-w64-x86_64/lib ${MINGW_LDFLAGS}"
