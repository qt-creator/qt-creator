/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "defaultpropertyprovider.h"

#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/toolchain.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/qtcassert.h>

#include <QFileInfo>

// Qt related settings:
const char QTCORE_BINPATH[] = "Qt.core.binPath";
const char QTCORE_BUILDVARIANT[] = "Qt.core.buildVariant";
const char QTCORE_DOCPATH[] = "Qt.core.docPath";
const char QTCORE_INCPATH[] = "Qt.core.incPath";
const char QTCORE_LIBPATH[] = "Qt.core.libPath";
const char QTCORE_VERSION[] = "Qt.core.version";
const char QTCORE_NAMESPACE[] = "Qt.core.namespace";
const char QTCORE_LIBINFIX[] = "Qt.core.libInfix";
const char QTCORE_MKSPEC[] = "Qt.core.mkspecPath";
const char QTCORE_FRAMEWORKBUILD[] = "Qt.core.frameworkBuild";


// Toolchain related settings:
const char QBS_TARGETOS[] = "qbs.targetOS";
const char QBS_SYSROOT[] = "qbs.sysroot";
const char QBS_ARCHITECTURE[] = "qbs.architecture";
const char QBS_ENDIANNESS[] = "qbs.endianness";
const char QBS_TOOLCHAIN[] = "qbs.toolchain";
const char CPP_TOOLCHAINPATH[] = "cpp.toolchainInstallPath";
const char CPP_COMPILERNAME[] = "cpp.compilerName";

namespace QbsProjectManager {

QVariantMap DefaultPropertyProvider::properties(const ProjectExplorer::Kit *k, const QVariantMap &defaultData) const
{
    QTC_ASSERT(k, return defaultData);
    QVariantMap data = defaultData;
    QtSupport::BaseQtVersion *qt = QtSupport::QtKitInformation::qtVersion(k);
    if (qt) {
        data.insert(QLatin1String(QTCORE_BINPATH), qt->binPath().toUserOutput());
        QStringList builds;
        if (qt->hasDebugBuild())
            builds << QLatin1String("debug");
        if (qt->hasReleaseBuild())
            builds << QLatin1String("release");
        data.insert(QLatin1String(QTCORE_BUILDVARIANT), builds);
        data.insert(QLatin1String(QTCORE_DOCPATH), qt->docsPath().toUserOutput());
        data.insert(QLatin1String(QTCORE_INCPATH), qt->headerPath().toUserOutput());
        data.insert(QLatin1String(QTCORE_LIBPATH), qt->libraryPath().toUserOutput());
        Utils::FileName mkspecPath = qt->mkspecsPath();
        mkspecPath.appendPath(qt->mkspec().toString());
        data.insert(QLatin1String(QTCORE_MKSPEC), mkspecPath.toUserOutput());
        data.insert(QLatin1String(QTCORE_NAMESPACE), qt->qtNamespace());
        data.insert(QLatin1String(QTCORE_LIBINFIX), qt->qtLibInfix());
        data.insert(QLatin1String(QTCORE_VERSION), qt->qtVersionString());
        if (qt->isFrameworkBuild())
            data.insert(QLatin1String(QTCORE_FRAMEWORKBUILD), true);
    }

    if (ProjectExplorer::SysRootKitInformation::hasSysRoot(k))
        data.insert(QLatin1String(QBS_SYSROOT), ProjectExplorer::SysRootKitInformation::sysRoot(k).toUserOutput());

    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(k);
    if (tc) {
        // FIXME/CLARIFY: How to pass the sysroot?
        ProjectExplorer::Abi targetAbi = tc->targetAbi();
        QString architecture = ProjectExplorer::Abi::toString(targetAbi.architecture());
        if (targetAbi.wordWidth() == 64)
            architecture.append(QLatin1String("_64"));
        data.insert(QLatin1String(QBS_ARCHITECTURE), architecture);

        if (targetAbi.endianness() == ProjectExplorer::Abi::BigEndian)
            data.insert(QLatin1String(QBS_ENDIANNESS), QLatin1String("big"));
        else
            data.insert(QLatin1String(QBS_ENDIANNESS), QLatin1String("little"));

        if (targetAbi.os() == ProjectExplorer::Abi::WindowsOS) {
            data.insert(QLatin1String(QBS_TARGETOS), QLatin1String("windows"));
            data.insert(QLatin1String(QBS_TOOLCHAIN),
                               targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMSysFlavor
                                   ? QStringList() << QLatin1String("mingw") << QLatin1String("gcc")
                                   : QStringList() << QLatin1String("msvc"));
        } else if (targetAbi.os() == ProjectExplorer::Abi::MacOS) {
            data.insert(QLatin1String(QBS_TARGETOS), QStringList() << QLatin1String("osx")
                        << QLatin1String("darwin") << QLatin1String("unix"));
            if (tc->type() != QLatin1String("clang")) {
                data.insert(QLatin1String(QBS_TOOLCHAIN), QLatin1String("gcc"));
            } else {
                data.insert(QLatin1String(QBS_TOOLCHAIN),
                            QStringList() << QLatin1String("clang")
                            << QLatin1String("llvm")
                            << QLatin1String("gcc"));
            }
        } else if (targetAbi.os() == ProjectExplorer::Abi::LinuxOS) {
            data.insert(QLatin1String(QBS_TARGETOS), QStringList() << QLatin1String("linux")
                        << QLatin1String("unix"));
            if (tc->type() != QLatin1String("clang")) {
                data.insert(QLatin1String(QBS_TOOLCHAIN), QLatin1String("gcc"));
            } else {
                data.insert(QLatin1String(QBS_TOOLCHAIN),
                            QStringList() << QLatin1String("clang")
                            << QLatin1String("llvm")
                            << QLatin1String("gcc"));
            }
        }
        Utils::FileName cxx = tc->compilerCommand();
        data.insert(QLatin1String(CPP_TOOLCHAINPATH), cxx.toFileInfo().absolutePath());
        data.insert(QLatin1String(CPP_COMPILERNAME), cxx.toFileInfo().fileName());
    }
    return data;
}

} // namespace QbsProjectManager
