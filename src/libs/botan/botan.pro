TEMPLATE = aux

DISTFILES = update-botan.sh

include(botan.pri)
include(../../../qtcreator.pri)
BOTAN_BUILD_DIR = $$OUT_PWD/$$BOTAN_BUILD_DIR
BOTAN_FILE_PATH = $$BOTAN_BUILD_DIR/$$BOTAN_FULL_NAME
BOTAN_BUILD_DIR_FOR_SHELL = $$shell_quote($$shell_path($$BOTAN_BUILD_DIR))
BOTAN_SOURCE_DIR = $$PWD/../3rdparty/botan

TARGET = $$BOTAN_LIB_NAME
PRECOMPILED_HEADER =
CONFIG -= qt

msvc: BOTAN_CC_TYPE = msvc
else: clang: BOTAN_CC_TYPE = clang
else: BOTAN_CC_TYPE = gcc
contains(QT_ARCH, i386): BOTAN_ARCH_SWITCH = "--cpu=x86"
else: contains(QT_ARCH, x86_64): BOTAN_ARCH_SWITCH = "--cpu=x86_64"
BOTAN_MODULES = aes aes_ssse3 auto_rng bigint block cbc ctr des dh dsa ec_group ecdh ecdsa entropy \
                filters hmac mode_pad pubkey rsa sha1 sha1_sse2 sha1_x86 sha2_32 sha2_32_x86 \
                sha2_64 simd system_rng,emsa_pkcs1,pbes2,pbkdf2
OTHER_FLAGS = --amalgamation --minimized-build  --disable-shared \
              --enable-modules=$$join(BOTAN_MODULES,",",,) --without-documentation
mingw {
    BOTAN_OS_SWITCH = "--os=mingw"
    OTHER_FLAGS += --without-stack-protector
}
BOTAN_CXX_FLAGS = $$QMAKE_CXXFLAGS
msvc: BOTAN_CXX_FLAGS += /wd4100 /wd4800 /wd4127 /wd4244 /wd4250 /wd4267 /wd4334 /wd4702 /wd4996 \
                         /D_ENABLE_EXTENDED_ALIGNED_STORAGE
else: BOTAN_CXX_FLAGS += -Wno-unused-parameter
macos: BOTAN_CXX_FLAGS += -mmacosx-version-min=$$QMAKE_MACOSX_DEPLOYMENT_TARGET -isysroot $$shell_quote($$QMAKE_MAC_SDK_PATH)
unix: BOTAN_CXX_FLAGS += -fPIC
win32: OTHER_FLAGS += --link-method=hardlink
CONFIG(debug, debug|release) {
    OTHER_FLAGS += --debug-mode
} else {
    msvc: BOTAN_CXX_FLAGS += /O2
    else: BOTAN_CXX_FLAGS += -O3
}
!isEmpty(BOTAN_CXX_FLAGS): OTHER_FLAGS += --cxxflags=$$shell_quote($$BOTAN_CXX_FLAGS)
CONFIGURE_FILE_PATH_FOR_SHELL = $$shell_quote($$shell_path($$BOTAN_SOURCE_DIR/configure.py))

configure_inputs = $$BOTAN_SOURCE_DIR/configure.py

configure.input = configure_inputs
configure.output = $$BOTAN_BUILD_DIR/Makefile
configure.variable_out = BOTAN_MAKEFILE
configure.commands = cd $$BOTAN_BUILD_DIR_FOR_SHELL && \
                     python $$CONFIGURE_FILE_PATH_FOR_SHELL \
                     --cc=$$BOTAN_CC_TYPE --cc-bin=$$shell_quote($$QMAKE_CXX) \
                     $$BOTAN_ARCH_SWITCH $$BOTAN_OS_SWITCH $$OTHER_FLAGS
QMAKE_EXTRA_COMPILERS += configure

make.input = BOTAN_MAKEFILE
make.output = $$BOTAN_FILE_PATH
make.CONFIG += target_predeps
make.commands = cd $$BOTAN_BUILD_DIR_FOR_SHELL && $(MAKE) libs
QMAKE_EXTRA_COMPILERS += make
