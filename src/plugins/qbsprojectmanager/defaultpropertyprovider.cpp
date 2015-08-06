/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "defaultpropertyprovider.h"
#include "qbsconstants.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <qbs.h>

#include <ios/iosconstants.h>
#include <qnx/qnxconstants.h>
#include <winrt/winrtconstants.h>

#include <QDir>
#include <QFileInfo>
#include <QSettings>

namespace QbsProjectManager {
using namespace Constants;

namespace Internal {
using namespace ProjectExplorer::Constants;
using namespace Ios::Constants;
using namespace Qnx::Constants;
using namespace WinRt::Internal::Constants;

static QString extractToolchainPrefix(QString *compilerName)
{
    QString prefix;
    if (compilerName->endsWith(QLatin1String("-g++"))
            || compilerName->endsWith(QLatin1String("-clang++"))
            || compilerName->endsWith(QLatin1String("-gcc"))
            || compilerName->endsWith(QLatin1String("-clang"))) {
        const int idx = compilerName->lastIndexOf(QLatin1Char('-')) + 1;
        prefix = compilerName->left(idx);
        compilerName->remove(0, idx);
    }
    return prefix;
}

static QStringList targetOSList(const ProjectExplorer::Abi &abi, const ProjectExplorer::Kit *k)
{
    const Core::Id device = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(k);
    QStringList os;
    switch (abi.os()) {
    case ProjectExplorer::Abi::WindowsOS:
        if (device == WINRT_DEVICE_TYPE_LOCAL ||
                device == WINRT_DEVICE_TYPE_PHONE ||
                device == WINRT_DEVICE_TYPE_EMULATOR) {
            os << QLatin1String("winrt");
        } else if (abi.osFlavor() == ProjectExplorer::Abi::WindowsCEFlavor) {
            os << QLatin1String("windowsce");
        }
        os << QLatin1String("windows");
        break;
    case ProjectExplorer::Abi::MacOS:
        if (device == DESKTOP_DEVICE_TYPE)
            os << QLatin1String("osx");
        else if (device == IOS_DEVICE_TYPE)
            os << QLatin1String("ios");
        else if (device == IOS_SIMULATOR_TYPE)
            os << QLatin1String("ios-simulator") << QLatin1String("ios");
        os << QLatin1String("darwin") << QLatin1String("bsd") << QLatin1String("unix");
        break;
    case ProjectExplorer::Abi::LinuxOS:
        if (abi.osFlavor() == ProjectExplorer::Abi::AndroidLinuxFlavor)
            os << QLatin1String("android");
        os << QLatin1String("linux") << QLatin1String("unix");
        break;
    case ProjectExplorer::Abi::BsdOS:
        switch (abi.osFlavor()) {
        case ProjectExplorer::Abi::FreeBsdFlavor:
            os << QLatin1String("freebsd");
            break;
        case ProjectExplorer::Abi::NetBsdFlavor:
            os << QLatin1String("netbsd");
            break;
        case ProjectExplorer::Abi::OpenBsdFlavor:
            os << QLatin1String("openbsd");
            break;
        default:
            break;
        }
        os << QLatin1String("bsd") << QLatin1String("unix");
        break;
    case ProjectExplorer::Abi::UnixOS:
        if (device == QNX_QNX_OS_TYPE)
            os << QLatin1String("qnx");
        else if (abi.osFlavor() == ProjectExplorer::Abi::SolarisUnixFlavor)
            os << QLatin1String("solaris");
        os << QLatin1String("unix");
        break;
    default:
        break;
    }
    return os;
}

static QStringList toolchainList(const ProjectExplorer::ToolChain *tc)
{
    QStringList list;
    if (tc->typeId() == ProjectExplorer::Constants::CLANG_TOOLCHAIN_TYPEID)
        list << QLatin1String("clang") << QLatin1String("llvm") << QLatin1String("gcc");
    else if (tc->typeId() == ProjectExplorer::Constants::GCC_TOOLCHAIN_TYPEID)
        list << QLatin1String("gcc"); // TODO: Detect llvm-gcc
    else if (tc->typeId() == ProjectExplorer::Constants::MINGW_TOOLCHAIN_TYPEID)
        list << QLatin1String("mingw") << QLatin1String("gcc");
    else if (tc->typeId() == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID)
        list << QLatin1String("msvc");
    return list;
}

QVariantMap DefaultPropertyProvider::properties(const ProjectExplorer::Kit *k,
                                                const QVariantMap &defaultData) const
{
    QTC_ASSERT(k, return defaultData);
    QVariantMap data = autoGeneratedProperties(k, defaultData);
    const QVariantMap customProperties = k->value(Core::Id(QBS_PROPERTIES_KEY_FOR_KITS)).toMap();
    for (QVariantMap::ConstIterator it = customProperties.constBegin();
         it != customProperties.constEnd(); ++it) {
        data.insert(it.key(), it.value());
    }
    return data;
}

QVariantMap DefaultPropertyProvider::autoGeneratedProperties(const ProjectExplorer::Kit *k,
                                                             const QVariantMap &defaultData) const
{
    QVariantMap data = defaultData;

    const QString sysroot = ProjectExplorer::SysRootKitInformation::sysRoot(k).toUserOutput();
    if (ProjectExplorer::SysRootKitInformation::hasSysRoot(k))
        data.insert(QLatin1String(QBS_SYSROOT), sysroot);

    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(k);
    if (!tc)
        return data;

    ProjectExplorer::Abi targetAbi = tc->targetAbi();
    if (targetAbi.architecture() != ProjectExplorer::Abi::UnknownArchitecture) {
        QString architecture = ProjectExplorer::Abi::toString(targetAbi.architecture());

        // We have to be conservative tacking on suffixes to arch names because an arch that is
        // already 64-bit may get an incorrect name as a result (i.e. Itanium)
        if (targetAbi.wordWidth() == 64) {
            switch (targetAbi.architecture()) {
            case ProjectExplorer::Abi::X86Architecture:
                architecture.append(QLatin1Char('_'));
                // fall through
            case ProjectExplorer::Abi::ArmArchitecture:
            case ProjectExplorer::Abi::MipsArchitecture:
            case ProjectExplorer::Abi::PowerPCArchitecture:
                architecture.append(QString::number(targetAbi.wordWidth()));
                break;
            default:
                break;
            }
        } else if (targetAbi.architecture() == ProjectExplorer::Abi::ArmArchitecture &&
                   targetAbi.os() == ProjectExplorer::Abi::MacOS) {
            architecture.append(QLatin1String("v7"));
        }

        data.insert(QLatin1String(QBS_ARCHITECTURE), qbs::canonicalArchitecture(architecture));
    }

    QStringList targetOS = targetOSList(targetAbi, k);
    if (!targetOS.isEmpty())
        data.insert(QLatin1String(QBS_TARGETOS), targetOS);

    QStringList toolchain = toolchainList(tc);
    if (!toolchain.isEmpty())
        data.insert(QLatin1String(QBS_TOOLCHAIN), toolchain);

    if (targetAbi.os() == ProjectExplorer::Abi::MacOS) {
        // Set Xcode SDK name and version - required by Qbs if a sysroot is present
        // Ideally this would be done in a better way...
        const QRegExp sdkNameRe(QLatin1String("(macosx|iphoneos|iphonesimulator)([0-9]+\\.[0-9]+)"));
        const QRegExp sdkVersionRe(QLatin1String("([0-9]+\\.[0-9]+)"));
        QDir sysrootdir(sysroot);
        const QSettings sdkSettings(sysrootdir.absoluteFilePath(QLatin1String("SDKSettings.plist")), QSettings::NativeFormat);
        const QString sdkName(sdkSettings.value(QLatin1String("CanonicalName")).toString());
        const QString sdkVersion(sdkSettings.value(QLatin1String("Version")).toString());
        if (sdkNameRe.exactMatch(sdkName) && sdkVersionRe.exactMatch(sdkVersion)) {
            for (int i = 3; i > 0; --i)
                sysrootdir.cdUp();
            data.insert(QLatin1String(CPP_PLATFORMPATH), sysrootdir.absolutePath());
            data.insert(QLatin1String(CPP_XCODESDKNAME), sdkName);
            data.insert(QLatin1String(CPP_XCODESDKVERSION), sdkVersion);
        }
    }

    Utils::FileName cxx = tc->compilerCommand();
    const QFileInfo cxxFileInfo = cxx.toFileInfo();
    QString compilerName = cxxFileInfo.fileName();
    const QString toolchainPrefix = extractToolchainPrefix(&compilerName);
    if (!toolchainPrefix.isEmpty())
        data.insert(QLatin1String(CPP_TOOLCHAINPREFIX), toolchainPrefix);
    if (toolchain.contains(QLatin1String("msvc")))
        data.insert(QLatin1String(CPP_COMPILERNAME), compilerName);
    else
        data.insert(QLatin1String(CPP_CXXCOMPILERNAME), compilerName);
    if (targetAbi.os() != ProjectExplorer::Abi::WindowsOS
            || targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMSysFlavor) {
        data.insert(QLatin1String(CPP_LINKERNAME), compilerName);
    }
    data.insert(QLatin1String(CPP_TOOLCHAINPATH), cxxFileInfo.absolutePath());

    // TODO: Remove this once compiler version properties are set for MSVC
    if (targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2013Flavor
            || targetAbi.osFlavor() == ProjectExplorer::Abi::WindowsMsvc2015Flavor) {
        const QLatin1String flags("/FS");
        data.insert(QLatin1String(CPP_PLATFORMCFLAGS), flags);
        data.insert(QLatin1String(CPP_PLATFORMCXXFLAGS), flags);
    }
    return data;
}

} // namespace Internal
} // namespace QbsProjectManager
