/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidtoolchain.h"
#include "androidconstants.h"
#include "androidconfigurations.h"

#include <projectexplorer/kitmanager.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/projectexplorer.h>

#include <utils/environment.h>

#include <QFileInfo>
#include <QLoggingCategory>


namespace {
static Q_LOGGING_CATEGORY(androidTCLog, "qtc.android.toolchainmanagement", QtWarningMsg);
}

namespace Android {
namespace Internal {

using namespace ProjectExplorer;
using namespace Utils;

static const QHash<QString, Abi> ClangTargets = {
    {"arm-linux-androideabi",
     Abi(Abi::ArmArchitecture, Abi::LinuxOS, Abi::AndroidLinuxFlavor, Abi::ElfFormat, 32)},
    {"i686-linux-android",
     Abi(Abi::X86Architecture, Abi::LinuxOS, Abi::AndroidLinuxFlavor, Abi::ElfFormat, 32)},
    {"x86_64-linux-android",
     Abi(Abi::X86Architecture, Abi::LinuxOS, Abi::AndroidLinuxFlavor, Abi::ElfFormat, 64)},
    {"aarch64-linux-android",
     Abi(Abi::ArmArchitecture, Abi::LinuxOS, Abi::AndroidLinuxFlavor, Abi::ElfFormat, 64)}};

static const QList<Utils::Id> LanguageIds = {ProjectExplorer::Constants::CXX_LANGUAGE_ID,
                                            ProjectExplorer::Constants::C_LANGUAGE_ID};

static ToolChain *findToolChain(Utils::FilePath &compilerPath, Utils::Id lang, const QString &target,
                                const ToolChainList &alreadyKnown)
{
    ToolChain * tc = Utils::findOrDefault(alreadyKnown, [target, compilerPath, lang](ToolChain *tc) {
        return tc->typeId() == Constants::ANDROID_TOOLCHAIN_TYPEID
                && tc->language() == lang
                && tc->targetAbi() == ClangTargets[target]
                && tc->compilerCommand() == compilerPath;
    });
    return tc;
}

AndroidToolChain::AndroidToolChain()
    : ClangToolChain(Constants::ANDROID_TOOLCHAIN_TYPEID)
{
    setTypeDisplayName(AndroidToolChain::tr("Android Clang"));
}

Utils::FilePath AndroidToolChain::ndkLocation() const
{
    return m_ndkLocation;
}

void AndroidToolChain::setNdkLocation(const Utils::FilePath &ndkLocation)
{
    m_ndkLocation = ndkLocation;
}

AndroidToolChain::~AndroidToolChain() = default;

bool AndroidToolChain::isValid() const
{
    if (m_ndkLocation.isEmpty()) {
        QStringList ndkParts(compilerCommand().toString().split("toolchains/llvm/prebuilt/"));
        if (ndkParts.size() > 1) {
            QString ndkLocation(ndkParts.first());
            if (ndkLocation.endsWith('/'))
                ndkLocation.chop(1);
            m_ndkLocation = FilePath::fromString(ndkLocation);
        }
    }

    const bool isChildofNdk = compilerCommand().isChildOf(m_ndkLocation);
    const bool isChildofSdk = compilerCommand().isChildOf(
        AndroidConfigurations::currentConfig().sdkLocation());

    return ClangToolChain::isValid() && typeId() == Constants::ANDROID_TOOLCHAIN_TYPEID
           && targetAbi().isValid() && (isChildofNdk || isChildofSdk)
           && !originalTargetTriple().isEmpty();
}

void AndroidToolChain::addToEnvironment(Environment &env) const
{
    AndroidConfig config = AndroidConfigurations::currentConfig();
    env.set(QLatin1String("ANDROID_NDK_HOST"), config.toolchainHostFromNdk(m_ndkLocation));
    const Utils::FilePath javaHome = config.openJDKLocation();
    if (javaHome.exists()) {
        env.set(QLatin1String("JAVA_HOME"), javaHome.toString());
        const FilePath javaBin = javaHome.pathAppended("bin");
        const FilePath currentJavaFilePath = env.searchInPath("java");
        if (!currentJavaFilePath.isChildOf(javaBin))
            env.prependOrSetPath(javaBin.toUserOutput());
    }
    env.set(QLatin1String("ANDROID_HOME"), config.sdkLocation().toString());
    env.set(QLatin1String("ANDROID_SDK_ROOT"), config.sdkLocation().toString());
}

bool AndroidToolChain::fromMap(const QVariantMap &data)
{
    if (!ClangToolChain::fromMap(data))
        return false;
    return isValid();
}

QStringList AndroidToolChain::suggestedMkspecList() const
{
    return {"android-g++", "android-clang"};
}

FilePath AndroidToolChain::makeCommand(const Environment &env) const
{
    Q_UNUSED(env)
    FilePath makePath = AndroidConfigurations::currentConfig().makePathFromNdk(m_ndkLocation);
    return makePath.exists() ? makePath : FilePath::fromString("make");
}

GccToolChain::DetectedAbisResult AndroidToolChain::detectSupportedAbis() const
{
    for (auto itr = ClangTargets.constBegin();itr != ClangTargets.constEnd(); ++itr) {
        if (itr.value() == targetAbi())
            return GccToolChain::DetectedAbisResult({targetAbi()}, itr.key());
    }
    return GccToolChain::DetectedAbisResult({targetAbi()}, "");
}


// --------------------------------------------------------------------------
// ToolChainFactory
// --------------------------------------------------------------------------

AndroidToolChainFactory::AndroidToolChainFactory()
{
    setDisplayName(AndroidToolChain::tr("Android Clang"));
    setSupportedToolChainType(Constants::ANDROID_TOOLCHAIN_TYPEID);
    setSupportedLanguages({ProjectExplorer::Constants::CXX_LANGUAGE_ID});
    setToolchainConstructor([] { return new AndroidToolChain; });
}

static FilePath clangPlusPlusPath(const FilePath &clangPath)
{
    return clangPath.parentDir().pathAppended(
                HostOsInfo::withExecutableSuffix(
                    QFileInfo(clangPath.toString()).baseName() + "++"));
}

static QList<FilePath> uniqueNdksForCurrentQtVersions()
{
    AndroidConfig config = AndroidConfigurations::currentConfig();

    auto androidQtVersions = QtSupport::QtVersionManager::versions(
        [](const QtSupport::BaseQtVersion *v) {
            return v->targetDeviceTypes().contains(Android::Constants::ANDROID_DEVICE_TYPE);
        });

    QList<FilePath> uniqueNdks;
    for (const QtSupport::BaseQtVersion *version : androidQtVersions) {
        FilePath ndk = config.ndkLocation(version);
        if (!uniqueNdks.contains(ndk))
            uniqueNdks.append(ndk);
    }

    return uniqueNdks;
}

ToolChainList AndroidToolChainFactory::autodetectToolChains(const ToolChainList &alreadyKnown)
{
    const QList<Utils::FilePath> uniqueNdks = uniqueNdksForCurrentQtVersions();
    return autodetectToolChainsFromNdks(alreadyKnown, uniqueNdks);
}

ToolChainList AndroidToolChainFactory::autodetectToolChainsFromNdks(
    const ToolChainList &alreadyKnown,
    const QList<Utils::FilePath> &ndkLocations,
    const bool isCustom)
{
    QList<ToolChain *> result;
    const AndroidConfig config = AndroidConfigurations::currentConfig();

    for (const Utils::FilePath &ndkLocation : ndkLocations) {
        FilePath clangPath = config.clangPathFromNdk(ndkLocation);
        if (!clangPath.exists()) {
            qCDebug(androidTCLog) << "Clang toolchains detection fails. Can not find Clang"
                                  << clangPath;
            continue;
        }

        for (const Utils::Id &lang : LanguageIds) {
            FilePath compilerCommand = clangPath;
            if (lang == ProjectExplorer::Constants::CXX_LANGUAGE_ID)
                compilerCommand = clangPlusPlusPath(clangPath);

            if (!compilerCommand.exists()) {
                qCDebug(androidTCLog)
                    << "Skipping Clang toolchain. Can not find compiler" << compilerCommand;
                continue;
            }

            auto targetItr = ClangTargets.constBegin();
            while (targetItr != ClangTargets.constEnd()) {
                const Abi &abi = targetItr.value();
                const QString target = targetItr.key();
                ToolChain *tc = findToolChain(compilerCommand, lang, target, alreadyKnown);

                QLatin1String customStr = isCustom ? QLatin1String("Custom ") : QLatin1String();
                const QString displayName(customStr + QString("Android Clang (%1, %2, NDK %3)")
                                              .arg(ToolChainManager::displayNameOfLanguageId(lang),
                                                   AndroidConfig::displayName(abi),
                                                   config.ndkVersion(ndkLocation).toString()));
                if (tc) {
                    // make sure to update the toolchain with current name format
                    if (tc->displayName() != displayName)
                        tc->setDisplayName(displayName);
                } else {
                    qCDebug(androidTCLog) << "New Clang toolchain found" << abi.toString() << lang
                                          << "for NDK" << ndkLocation;
                    auto atc = new AndroidToolChain();
                    atc->setNdkLocation(ndkLocation);
                    atc->setOriginalTargetTriple(target);
                    atc->setLanguage(lang);
                    atc->setTargetAbi(ClangTargets[target]);
                    atc->setPlatformCodeGenFlags({"-target", target});
                    atc->setPlatformLinkerFlags({"-target", target});
                    atc->setDisplayName(displayName);
                    atc->resetToolChain(compilerCommand);
                    tc = atc;
                }
                tc->setDetection(ToolChain::AutoDetection);
                result << tc;
                ++targetItr;
            }
        }
    }

    return result;
}

} // namespace Internal
} // namespace Android
