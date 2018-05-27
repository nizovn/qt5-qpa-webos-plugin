SYSROOT_PKG=/home/nizovn/webos/qt5/qt5.9/device/sysroot
export PKG_CONFIG_LIBDIR=$SYSROOT_PKG/usr/lib/pkgconfig
export PKG_CONFIG_SYSROOT_DIR=$SYSROOT_PKG

MY_PREFIX=/home/nizovn/webos/qt5/qt5.9/runtime
WEBOS_PREFIX=/media/cryptofs/apps/usr/palm/applications/com.nizovn.qt5

./configure \
-no-gcc-sysroot \
-sysroot ${SYSROOT_PKG} \
-prefix ${WEBOS_PREFIX} \
-extprefix ${MY_PREFIX} \
-hostprefix ${MY_PREFIX}/host \
-opensource \
-confirm-license \
-qt-freetype \
-openssl \
-openssl-linked \
-feature-accessibility \
-accessibility \
-skip qtandroidextras -skip qtactiveqt -skip qtdoc  \
-skip qtserialport  -skip qtwayland \
-skip qtmacextras -skip qtlocation \
-skip qtx11extras -skip qtwinextras \
-skip qttools \
-skip qtenginio -skip qtactiveqt \
-qt-pcre \
-release \
-no-cups \
-no-dbus \
-no-sm \
-no-pch \
-no-glib \
-no-harfbuzz \
-no-qml-debug \
-no-gstreamer \
-no-xkbcommon-evdev \
-no-xcb \
-no-xkbcommon-evdev \
-no-kms \
-no-system-proxies  \
-no-journald  \
-no-mtdev  \
-no-separate-debug-info \
-reduce-exports \
-no-android-style-assets \
-no-compile-examples \
-nomake examples -nomake tests \
-make tools \
-platform linux-g++ \
-make libs \
-xplatform linux-webos-arm-gnueabi-g++ \
-no-evdev \
-no-rpath \
-force-pkg-config \
-eglfs \
-alsa \
-proprietary-codecs \
-pepper-plugins \
-printing-and-pdf \
-webrtc \
-qpa webos \
-opengl es2
