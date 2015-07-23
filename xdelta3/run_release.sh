#!/bin/bash

# Place C/C++ common flags here
COMMON="-g"

export CFLAGS
export CXXFLAGS
export LDFLAGS

# TODO replace w/ wget
LZMA="xz-5.2.1"
LZMA_FILE="/volume/home/jmacd/src/xdelta-devel/xz-5.2.1.tar.xz"

MAKEFLAGS="-j 10"

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

LINUXTEST1=""
LINUXTEST2=""
WINTEST1=""
WINTEST2=""

find build -type f 2> /dev/null | xargs rm

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
    echo -n "	${w} ..."
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

function buildlzma {
    host=$1
    march=$2
    local target="${BUILDDIR}/lib-${host}${march}"

    echo "	... liblzma"
    
    mkdir -p $target

    try configure-lzma ${target} ${LZMASRC}/configure \
	--host=${host} \
	--prefix=${target} \
	--disable-shared \
	"CFLAGS=${march}" \
	"CXXFLAGS=${march}" \
	"LDFLAGS=${march}"
    if [ $? -ne 0 ]; then
	return
    fi

    try build-lzma ${target} make ${MAKEFLAGS}
    if [ $? -ne 0 ]; then
	return
    fi
    try install-lzma ${target} make install
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
    D="build/${BM}/xoff${offsetbits}"
    BMD="${BM}-${offsetbits}"
    FULLD="$PWD/$D"
    CFLAGS="${COMMON} ${march} ${cargs} -I${PWD}/build/lib-${BM}/include"
    CXXFLAGS="${COMMON} ${march} ${cargs} -I${PWD}/build/lib-${BM}/include"
    LDFLAGS="${largs} ${march} -L${PWD}/build/lib-${BM}/lib"
    mkdir -p ${D}

    echo "	... ${BMD}"
    
    cat >> Makefile.test <<EOF

# ${BMD}
.PHONY: regtest-${BMD}
regtest-${BMD}:
	(cd ${D} && ./xdelta3regtest 1> \${TMP}/regtest.${BMD}.stdout 2> \${TMP}/regtest.${BMD}.stderr)

.PHONY: selftest-${BMD}
selftest-${BMD}:
	(cd ${D} && ./bin/xdelta3 test 1> \${TMP}/selftest.${BMD}.stdout 2> \${TMP}/selftest.${BMD}.stderr)


EOF

    case ${host} in
	*linux*)
	    LINUXTEST1="${LINUXTEST1} selftest-${BMD}"
	    LINUXTEST2="${LINUXTEST2} regtest-${BMD}"
	    ;;
	*mingw*)
	    WINTEST1="${WINTEST1} selftest-${BMD}"
	    WINTEST2="${WINTEST2} regtest-${BMD}"
	    ;;
    esac

    try configure-xdelta $FULLD $SRCDIR/configure \
    		  --host=${host} \
    		  --prefix=${FULLD} \
    		  --enable-static \
    		  --disable-shared \
    		  --enable-debug-symbols
    if [ $? -ne 0 ]; then
	return
    fi

    try build-xdelta $FULLD make ${MAKEFLAGS} all
    if [ $? -ne 0 ]; then
	return
    fi

    try install-xdelta $FULLD make install
}

function buildall {
    echo ""
    echo "Host $1$2"
    echo ""

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

setup

try untar-lzma ${BUILDDIR} tar -xvf "${LZMA_FILE}"
if [ $? -ne 0 ]; then
    exit $?
fi

DATE=`date`
cat > Makefile.test <<EOF
# Auto-generated ${DATE} -*- Mode: Makefile -*-
EOF

# Linux
buildall x86_64-pc-linux-gnu -m32
buildall x86_64-pc-linux-gnu -m64

# Windows
buildall i686-w64-mingw32 -mconsole "${MINGW_CFLAGS}" ""
buildall x86_64-w64-mingw32 -mconsole "${MINGW_CFLAGS}" ""

cat >> Makefile.test <<EOF

linux: linux-selftest linux-regtest
windows: windows-selftest windows-regtest

linux-selftest: ${LINUXTEST1}
linux-regtest: ${LINUXTEST2}

windows-selftest: ${WINTEST1}
windows-regtest: ${WINTEST2}

EOF
