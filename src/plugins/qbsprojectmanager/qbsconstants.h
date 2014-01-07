/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QBSCONSTANTS_H
#define QBSCONSTANTS_H

namespace QbsProjectManager {
namespace Constants {

// Qt related settings:
const char QTCORE_BINPATH[] = "Qt.core.binPath";
const char QTCORE_BUILDVARIANT[] = "Qt.core.buildVariant";
const char QTCORE_DOCPATH[] = "Qt.core.docPath";
const char QTCORE_INCPATH[] = "Qt.core.incPath";
const char QTCORE_LIBPATH[] = "Qt.core.libPath";
const char QTCORE_PLUGINPATH[] = "Qt.core.pluginPath";
const char QTCORE_VERSION[] = "Qt.core.version";
const char QTCORE_NAMESPACE[] = "Qt.core.namespace";
const char QTCORE_LIBINFIX[] = "Qt.core.libInfix";
const char QTCORE_MKSPEC[] = "Qt.core.mkspecPath";
const char QTCORE_FRAMEWORKBUILD[] = "Qt.core.frameworkBuild";
const char QTCORE_CONFIG[] = "Qt.core.config";
const char QTCORE_QTCONFIG[] = "Qt.core.qtConfig";


// Toolchain related settings:
const char QBS_TARGETOS[] = "qbs.targetOS";
const char QBS_SYSROOT[] = "qbs.sysroot";
const char QBS_ARCHITECTURE[] = "qbs.architecture";
const char QBS_ENDIANNESS[] = "qbs.endianness";
const char QBS_TOOLCHAIN[] = "qbs.toolchain";
const char CPP_TOOLCHAINPATH[] = "cpp.toolchainInstallPath";
const char CPP_COMPILERNAME[] = "cpp.compilerName";

} // namespace Constants
} // namespace QbsProjectManager

#endif // QBSCONSTANTS_H
