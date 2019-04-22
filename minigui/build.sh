#! /bin/sh
# which can be reset
# INSTALL_DIR
# EXTRA_LIBS
# EXTRA_INC
#-----------------------------------------------------------------------------------------
NAME=`whoami`
PWD=`pwd`

CROSS="${LICHEE_DIR}/out/external-toolchain/gcc-arm/bin/arm-linux-gnueabi-"
SYSROOT="$(${CROSS}gcc -print-sysroot)"
ROOTFS=${TARGET_OUT}/target
STAGING=${TARGET_OUT}/staging
EXTRA_LDFLAG="-L$SYSROOT/lib -L$SYSROOT/usr/lib -L$ROOTFS/lib -L$ROOTFS/usr/lib -lpng -ljpeg -lts -lz"
#-----------------------------------------------------------------------------------------
LD_FLAG="-lm -lstdc++ -lc -ldl -Wl,-Bdynamic"
#-----------------------------------------------------------------------------------------
CPPFLAGS="--sysroot=$SYSROOT -I$STAGING/usr/include -I$STAGING/usr/lib/include"
LDFLAGS="$LD_FLAG $EXTRA_LDFLAG"
#-----------------------------------------------------------------------------------------
export LDFLAGS="$LDFLAGS"
export CPPFLAGS="$CPPFLAGS"

export CC="$CROSS"gcc
export CXX="$CROSS"g++
export LD="$CROSS"ld
export AS="$CROSS"as
export AR="$CROSS"ar
export STRIP="$CROSS"strip
export INSTALL_DIR="$PWD/tmp"

mkdir -p $INSTALL_DIR/lib
mkdir -p $INSTALL_DIR/include/minigui

# if Makefile.am updated, should do this
if [ ! -x "Makefile.in" ];then
    echo "aclocal..."
    aclocal
    echo "autoconf..."
    autoconf
    echo "autoheader..."
    autoheader
    echo "automake..."
    automake --add-missing
fi
#-----------------------------------------------------------------------------------------
./configure \
    --prefix=${INSTALL_DIR} \
    --host=arm-linux \
    --build=i386-linux \
    --with-targetname=fbcon \
    --enable-sunximin \
    --enable-jpgsupport \
    --enable-pngsupport \
    --enable-adv2dapi \
    --enable-videoshadow \
    --enable-tslibial \
    --enable-sunxikeytslibial
    # --enable-debug \
#-----------------------------------------------------------------------------------------
make -j22
make install-strip
#-----------------------------------------------------------------------------------------
cp ${INSTALL_DIR}/lib/libminigui_ths-3.0.so.12.0.0 $TARGET_TOP/custom_aw/lib/$TARGET_PRODUCT
cp -rf ${INSTALL_DIR}/include/minigui $TARGET_TOP/custom_aw/include/$TARGET_PRODUCT
# cp -rf ${INSTALL_DIR}/lib/* $LICHEE_EYESEE_BR_OUT/staging/usr/lib/
# cp -rf ${INSTALL_DIR}/include/minigui $LICHEE_EYESEE_BR_OUT/staging/usr/include
#-----------------------------------------------------------------------------------------
