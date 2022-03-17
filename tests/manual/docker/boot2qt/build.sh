#!/bin/sh

############################################################################
#
# Copyright (C) 2021 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3 as published by the Free Software
# Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#
############################################################################

uid=$(id -u)
gid=$(id -g)

repo=b2qt-qemuarm64
tag=6.0.0
tcdir=/opt/toolchain
envfile=environment-setup-skylake-64-poky-linux
qtcconf=configure-qtcreator.sh

if [ ! -e toolchain.sh ]; then
    host=http://ci-files02-hki.intra.qt.io
    dir=/packages/jenkins/enterprise/b2qt/yocto/dev/intel-skylake-64
    file=b2qt-x86_64-meta-toolchain-b2qt-embedded-qt6-sdk-intel-skylake-64.sh
    curl $host/$dir/$file -o $file || exit 1
    ln -s $file toolchain.sh
fi

sed -e "s/<UID>/${uid}/" -e "s/<GID>/${gid}/" Dockerfile.in > Dockerfile

docker build -f Dockerfile . -t $repo:$tag


ABI="x86-linux-poky-elf-64bit"

OECORE_NATIVE_SYSROOT=/opt/toolchain/sysroots/x86_64-pokysdk-linux
PREFIX="docker://$repo:$tag/opt/toolchain/sysroots/x86_64-pokysdk-linux"

CC="${PREFIX}/usr/bin/x86_64-poky-linux/x86_64-poky-linux-gcc"
CXX="${PREFIX}/usr/bin/x86_64-poky-linux/x86_64-poky-linux-g++"
GDB="${PREFIX}/usr/bin/x86_64-poky-linux/x86_64-poky-linux-gdb"
QMAKE="${PREFIX}/usr/bin/qmake"
CMAKE="${PREFIX}/usr/bin/cmake"

SDKTOOL="../../../../libexec/qtcreator/sdktool"

MACHINE="intel-skylake-64"

if [ ! -x ${SDKTOOL} ]; then
    echo "Cannot find 'sdktool' from QtCreator"
    exit 1
fi


#MKSPEC=$(qmake -query QMAKE_XSPEC)
MKSPEC=linux-oe-g++

#RELEASE=$(qmake -query QT_VERSION)
RELEASE=$tag

NAME=${NAME:-"Custom Qt ${RELEASE} ${MACHINE}"}
BASEID="byos.${RELEASE}.${MACHINE}"

${SDKTOOL} rmKit --id ${BASEID}.kit 2>/dev/null || true
${SDKTOOL} rmQt --id ${BASEID}.qt || true
${SDKTOOL} rmTC --id ProjectExplorer.ToolChain.Gcc:${BASEID}.gcc || true
${SDKTOOL} rmTC --id ProjectExplorer.ToolChain.Gcc:${BASEID}.g++ || true
${SDKTOOL} rmDebugger --id ${BASEID}.gdb 2>/dev/null || true
${SDKTOOL} rmCMake --id ${BASEID}.cmake 2>/dev/null || true
${SDKTOOL} rmDev --id "$repo:$tag" 2>/dev/null || true

if [ -n "${REMOVEONLY}" ]; then
    echo "Kit removed: ${NAME}"
    exit 0
fi

${SDKTOOL} addAbiFlavor \
    --flavor poky \
    --oses linux 2>/dev/null || true

${SDKTOOL} addDev \
    --id "$repo:$tag" \
    --type 1 \
    --name "Docker $repo:$tag" \
    --dockerRepo $repo \
    --dockerTag $tag \
    --dockerMappedPaths '/data' \
    --osType DockerDeviceType

${SDKTOOL} addTC \
    --id "ProjectExplorer.ToolChain.Gcc:${BASEID}.gcc" \
    --name "GCC ${NAME}" \
    --path "${CC}" \
    --abi "${ABI}" \
    --language C

${SDKTOOL} addTC \
    --id "ProjectExplorer.ToolChain.Gcc:${BASEID}.g++" \
    --name "G++ ${NAME}" \
    --path "${CXX}" \
    --abi "${ABI}" \
    --language Cxx

${SDKTOOL} addDebugger \
    --id "${BASEID}.gdb" \
    --name "GDB ${NAME}" \
    --engine 1 \
    --binary "${GDB}" \
    --abis "${ABI}"

${SDKTOOL} addQt \
    --id "${BASEID}.qt" \
    --name "${NAME}" \
    --type "Qdb.EmbeddedLinuxQt" \
    --qmake ${QMAKE} \
    --abis "${ABI}"

${SDKTOOL} addCMake \
    --id "${BASEID}.cmake" \
    --name "CMake ${NAME}" \
    --path ${CMAKE}

${SDKTOOL} addKit \
    --id "${BASEID}.kit" \
    --name "${NAME}" \
    --qt "${BASEID}.qt" \
    --debuggerid "${BASEID}.gdb" \
    --sysroot "docker://$repo:$tag/opt/toolchain/sysroots/skylake-64-poky-linux" \
    --devicetype "DockerDeviceType" \
    --device "$repo:$tag" \
    --builddevice "$repo:$tag" \
    --Ctoolchain "ProjectExplorer.ToolChain.Gcc:${BASEID}.gcc" \
    --Cxxtoolchain "ProjectExplorer.ToolChain.Gcc:${BASEID}.g++" \
    --icon ":/boot2qt/images/B2Qt_QtC_icon.png" \
    --mkspec "" \
    --cmake "${BASEID}.cmake" \
    --cmake-config "CMAKE_CXX_COMPILER:STRING=%{Compiler:Executable:Cxx}" \
    --cmake-config "CMAKE_C_COMPILER:STRING=%{Compiler:Executable:C}" \
    --cmake-config "CMAKE_PREFIX_PATH:STRING=%{Qt:QT_INSTALL_PREFIX}" \
    --cmake-config "QT_QMAKE_EXECUTABLE:STRING=%{Qt:qmakeExecutable}" \
    --cmake-config "CMAKE_TOOLCHAIN_FILE:FILEPATH=${OECORE_NATIVE_SYSROOT}/usr/lib/cmake/Qt6/qt.toolchain.cmake" \
    --cmake-config "CMAKE_MAKE_PROGRAM:FILEPATH=ninja" \
    --cmake-generator "Ninja"

echo "Configured Qt Creator with new kit: ${NAME}"
