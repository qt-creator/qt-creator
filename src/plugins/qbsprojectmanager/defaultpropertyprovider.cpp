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

#include "defaultpropertyprovider.h"
#include "qbsconstants.h"

#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/toolchain.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/qtcassert.h>

#include <QFileInfo>

namespace QbsProjectManager {
using namespace Constants;

QVariantMap DefaultPropertyProvider::properties(const ProjectExplorer::Kit *k, const QVariantMap &defaultData) const
{
    QTC_ASSERT(k, return defaultData);
    QVariantMap data = defaultData;
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
            const char IOSQT[] = "Qt4ProjectManager.QtVersion.Ios"; // from Ios::Constants (include header?)
            const QtSupport::BaseQtVersion * const qt = QtSupport::QtKitInformation::qtVersion(k);
            if (qt && qt->type() == QLatin1String(IOSQT)) {
                QStringList targetOS;
                if (targetAbi.architecture() == ProjectExplorer::Abi::X86Architecture)
                    targetOS << QLatin1String("ios-simulator");
                targetOS << QLatin1String("ios") << QLatin1String("darwin")
                         << QLatin1String("bsd4") << QLatin1String("bsd") << QLatin1String("unix");
                data.insert(QLatin1String(QBS_TARGETOS), targetOS);
            } else {
                data.insert(QLatin1String(QBS_TARGETOS), QStringList() << QLatin1String("osx")
                            << QLatin1String("darwin") << QLatin1String("bsd4")
                            << QLatin1String("bsd") << QLatin1String("unix"));
            }
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
        } else {
            // TODO: Factor out toolchain type setting.
            data.insert(QLatin1String(QBS_TARGETOS), QStringList() << QLatin1String("unix"));
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
        if (targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2013Flavor) {
            const QLatin1String flags("/FS");
            data.insert(QLatin1String(CPP_PLATFORMCFLAGS), flags);
            data.insert(QLatin1String(CPP_PLATFORMCXXFLAGS), flags);
        }
    }
    return data;
}

} // namespace QbsProjectManager
