/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "defaultpropertyprovider.h"

#include "qbskitinformation.h"
#include "qbsprojectmanagerconstants.h"

#include <coreplugin/messagemanager.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/gcctoolchain.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <qbs.h>

#include <ios/iosconstants.h>
#include <winrt/winrtconstants.h>

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSettings>

namespace QbsProjectManager {
using namespace Constants;

namespace Internal {
using namespace ProjectExplorer::Constants;
using namespace Ios::Constants;
using namespace WinRt::Internal::Constants;

static QString extractToolchainPrefix(QString *compilerName)
{
    QString prefix;
    const QStringList candidates = {QLatin1String("g++"), QLatin1String("clang++"),
                                    QLatin1String("gcc"), QLatin1String("clang")};
    foreach (const QString &candidate, candidates) {
        const QString suffix = Utils::HostOsInfo::withExecutableSuffix(QLatin1Char('-')
                                                                       + candidate);
        if (compilerName->endsWith(suffix)) {
            const int idx = compilerName->lastIndexOf(QLatin1Char('-')) + 1;
            prefix = compilerName->left(idx);
            compilerName->remove(0, idx);
        }
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
    case ProjectExplorer::Abi::DarwinOS:
        if (device == DESKTOP_DEVICE_TYPE)
            os << QLatin1String("macos") << QLatin1String("osx");
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
    case ProjectExplorer::Abi::QnxOS:
        os << QLatin1String("qnx") << QLatin1String("unix");
        break;
    case ProjectExplorer::Abi::UnixOS:
        if (abi.osFlavor() == ProjectExplorer::Abi::SolarisUnixFlavor)
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

static QString architecture(const ProjectExplorer::Abi &targetAbi)
{
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
                // ARM sub-architectures are currently not handled, which is kind of problematic
            case ProjectExplorer::Abi::MipsArchitecture:
            case ProjectExplorer::Abi::PowerPCArchitecture:
                architecture.append(QString::number(targetAbi.wordWidth()));
                break;
            default:
                break;
            }
        }

        return architecture;
    }

    return QString();
}

static bool isMultiTargetingToolchain(const ProjectExplorer::ToolChain *tc)
{
    // Clang and QCC are multi-targeting compilers; others (GCC/MinGW, MSVC, ICC) are not
    return tc->targetAbi().os() == ProjectExplorer::Abi::QnxOS
            || tc->typeId() == ProjectExplorer::Constants::CLANG_TOOLCHAIN_TYPEID;
}

static QStringList architectures(const ProjectExplorer::ToolChain *tc)
{
    // For platforms which can have builds for multiple architectures in a single configuration
    // (Darwin, Android), regardless of whether the toolchain is multi-targeting or not (Clang
    // always is, but Android GCC is not), let qbs automatically determine the list of architectures
    // to build for by default. Similarly, if the underlying toolchain only targets a single
    // architecture there's no reason to duplicate the detection logic here.
    // Handles: GCC/MinGW, ICC, MSVC, Clang (Darwin, Android)
    if (tc->targetAbi().os() == ProjectExplorer::Abi::DarwinOS
            || tc->targetAbi().osFlavor() == ProjectExplorer::Abi::AndroidLinuxFlavor
            || !isMultiTargetingToolchain(tc))
        return { };

    // This attempts to use the preferred architecture for toolchains which are multi-targeting.
    // Handles: Clang (Linux/UNIX), QCC
    const auto arch = architecture(tc->targetAbi());
    if (!arch.isEmpty())
        return { arch };
    return { };
}

QVariantMap DefaultPropertyProvider::properties(const ProjectExplorer::Kit *k,
                                                const QVariantMap &defaultData) const
{
    QTC_ASSERT(k, return defaultData);
    QVariantMap data = autoGeneratedProperties(k, defaultData);
    const QVariantMap customProperties = QbsKitInformation::properties(k);
    for (QVariantMap::ConstIterator it = customProperties.constBegin();
         it != customProperties.constEnd(); ++it) {
        data.insert(it.key(), it.value());
    }
    return data;
}

static void filterCompilerLinkerFlags(const ProjectExplorer::Abi &targetAbi, QStringList &flags)
{
    for (int i = 0; i < flags.size(); ) {
        if (targetAbi.architecture() != ProjectExplorer::Abi::UnknownArchitecture
                && flags[i] == QStringLiteral("-arch")
                && i + 1 < flags.size()) {
            flags.removeAt(i);
            flags.removeAt(i);
        } else {
            ++i;
        }
    }
}

QVariantMap DefaultPropertyProvider::autoGeneratedProperties(const ProjectExplorer::Kit *k,
                                                             const QVariantMap &defaultData) const
{
    QVariantMap data = defaultData;

    const QString sysroot = ProjectExplorer::SysRootKitInformation::sysRoot(k).toUserOutput();
    if (ProjectExplorer::SysRootKitInformation::hasSysRoot(k))
        data.insert(QLatin1String(QBS_SYSROOT), sysroot);

    ProjectExplorer::ToolChain *tcC
            = ProjectExplorer::ToolChainKitInformation::toolChain(k, ProjectExplorer::Constants::C_LANGUAGE_ID);
    ProjectExplorer::ToolChain *tcCxx
            = ProjectExplorer::ToolChainKitInformation::toolChain(k, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    if (!tcC && !tcCxx)
        return data;

    ProjectExplorer::ToolChain *mainTc = tcCxx ? tcCxx : tcC;

    ProjectExplorer::Abi targetAbi = mainTc->targetAbi();

    auto archs = architectures(mainTc);
    if (!archs.isEmpty())
        data.insert(QLatin1String(QBS_ARCHITECTURES), archs);
    if (mainTc->targetAbi() !=
            ProjectExplorer::Abi::abiFromTargetTriplet(mainTc->originalTargetTriple())) {
        data.insert(QLatin1String(QBS_ARCHITECTURE), architecture(mainTc->targetAbi()));
    }
    data.insert(QLatin1String(QBS_TARGETOS), targetOSList(targetAbi, k));

    QStringList toolchain = toolchainList(mainTc);

    Utils::FileName cCompilerPath;
    if (tcC)
        cCompilerPath = tcC->compilerCommand();

    Utils::FileName cxxCompilerPath;
    if (tcCxx)
        cxxCompilerPath = tcCxx->compilerCommand();

    const QFileInfo cFileInfo = cCompilerPath.toFileInfo();
    const QFileInfo cxxFileInfo = cxxCompilerPath.toFileInfo();
    QString cCompilerName = cFileInfo.fileName();
    QString cxxCompilerName = cxxFileInfo.fileName();
    const QString cToolchainPrefix = extractToolchainPrefix(&cCompilerName);
    const QString cxxToolchainPrefix = extractToolchainPrefix(&cxxCompilerName);

    QFileInfo mainFileInfo;
    QString mainCompilerName;
    QString mainToolchainPrefix;
    if (tcCxx) {
        mainFileInfo = cxxFileInfo;
        mainCompilerName = cxxCompilerName;
        mainToolchainPrefix = cxxToolchainPrefix;
    } else {
        mainFileInfo = cFileInfo;
        mainCompilerName = cCompilerName;
        mainToolchainPrefix = cToolchainPrefix;
    }

    if (!mainToolchainPrefix.isEmpty())
        data.insert(QLatin1String(CPP_TOOLCHAINPREFIX), mainToolchainPrefix);

    if (toolchain.contains(QLatin1String("msvc"))) {
        data.insert(QLatin1String(CPP_COMPILERNAME), mainCompilerName);
    } else {
        if (!cCompilerName.isEmpty())
            data.insert(QLatin1String(CPP_COMPILERNAME), cCompilerName);
        if (!cxxCompilerName.isEmpty())
            data.insert(QLatin1String(CPP_CXXCOMPILERNAME), cxxCompilerName);
    }

    if (tcC && tcCxx && cFileInfo.absolutePath() != cxxFileInfo.absolutePath()) {
        Core::MessageManager::write(tr("C and C++ compiler paths differ. C compiler may not work."),
                                    Core::MessageManager::ModeSwitch);
    }
    data.insert(QLatin1String(CPP_TOOLCHAINPATH), mainFileInfo.absolutePath());

    if (ProjectExplorer::GccToolChain *gcc = dynamic_cast<ProjectExplorer::GccToolChain *>(mainTc)) {
        QStringList compilerFlags = gcc->platformCodeGenFlags();
        filterCompilerLinkerFlags(targetAbi, compilerFlags);
        data.insert(QLatin1String(CPP_PLATFORMCOMMONCOMPILERFLAGS), compilerFlags);

        QStringList linkerFlags = gcc->platformLinkerFlags();
        filterCompilerLinkerFlags(targetAbi, linkerFlags);
        data.insert(QLatin1String(CPP_PLATFORMLINKERFLAGS), linkerFlags);
    }

    if (targetAbi.os() == ProjectExplorer::Abi::DarwinOS) {
        // Reverse engineer the Xcode developer path from the compiler path
        const QRegularExpression compilerRe(
            QStringLiteral("^(?<developerpath>.*)/Toolchains/(?:.+)\\.xctoolchain/usr/bin$"));
        const QRegularExpressionMatch compilerReMatch = compilerRe.match(cxxFileInfo.absolutePath());
        if (compilerReMatch.hasMatch()) {
            const QString developerPath = compilerReMatch.captured(QStringLiteral("developerpath"));
            data.insert(QLatin1String(XCODE_DEVELOPERPATH), developerPath);
            toolchain.insert(0, QStringLiteral("xcode"));

            // If the sysroot is part of this developer path, set the canonical SDK name
            const QDir sysrootdir(QDir::cleanPath(sysroot));
            const QString sysrootAbs = sysrootdir.absolutePath();
            const QSettings sdkSettings(
                sysrootdir.absoluteFilePath(QStringLiteral("SDKSettings.plist")),
                QSettings::NativeFormat);
            const QString version(
                sdkSettings.value(QStringLiteral("Version")).toString());
            QString canonicalName(
                sdkSettings.value(QStringLiteral("CanonicalName")).toString());
            canonicalName.chop(version.size());
            if (!canonicalName.isEmpty() && !version.isEmpty()
                    && sysrootAbs.startsWith(developerPath)) {
                if (sysrootAbs.endsWith(QStringLiteral("/%1.sdk").arg(canonicalName + version),
                                        Qt::CaseInsensitive)) {
                    data.insert(QLatin1String(XCODE_SDK), QString(canonicalName + version));
                }
                if (sysrootAbs.endsWith(QStringLiteral("/%1.sdk").arg(canonicalName),
                                        Qt::CaseInsensitive)) {
                    data.insert(QLatin1String(XCODE_SDK), canonicalName);
                }
            }
        }
    }

    if (!toolchain.isEmpty())
        data.insert(QLatin1String(QBS_TOOLCHAIN), toolchain);

    return data;
}

} // namespace Internal
} // namespace QbsProjectManager
