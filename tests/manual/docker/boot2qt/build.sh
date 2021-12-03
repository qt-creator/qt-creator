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

if [ ! -e toolchain.sh ]; then
    host=http://ci-files02-hki.intra.qt.io
    dir=/packages/jenkins/enterprise/b2qt/yocto/6.2/
    file=b2qt-x86_64-meta-toolchain-b2qt-embedded-qt6-sdk-intel-skylake-64.sh
    curl $host/$dir/$file -o $file || exit 1
    ln -s $file toolchain.sh
fi

sed -e "s/<UID>/${uid}/" -e "s/<GID>/${gid}/" Dockerfile.in > Dockerfile

docker build -f Dockerfile . -t b2qt-qemuarm64:6.0.0
