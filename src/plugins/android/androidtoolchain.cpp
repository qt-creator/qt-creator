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

#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/projectexplorer.h>

#include <utils/environment.h>

#include <QFileInfo>
#include <QLoggingCategory>
#include <QRegExp>


namespace {
Q_LOGGING_CATEGORY(androidTCLog, "qtc.android.toolchainmanagement", QtWarningMsg);
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

static const QList<Core::Id> LanguageIds = {ProjectExplorer::Constants::CXX_LANGUAGE_ID,
                                            ProjectExplorer::Constants::C_LANGUAGE_ID};

static ToolChain *findToolChain(Utils::FileName &compilerPath, Core::Id lang, const QString &target,
                                CToolChainList &alreadyKnown)
{
    ToolChain * tc = Utils::findOrDefault(alreadyKnown, [target, compilerPath, lang](ToolChain *tc) {
        return tc->typeId() == Constants::ANDROID_TOOLCHAIN_ID
                && tc->language() == lang
                && tc->targetAbi() == ClangTargets[target]
                && tc->compilerCommand() == compilerPath;
    });
    return tc;
}

AndroidToolChain::~AndroidToolChain() = default;

QString AndroidToolChain::typeDisplayName() const
{
    return AndroidToolChainFactory::tr("Android Clang");
}

bool AndroidToolChain::isValid() const
{
    return ClangToolChain::isValid()
            && typeId() == Constants::ANDROID_TOOLCHAIN_ID
            && targetAbi().isValid()
            && compilerCommand().isChildOf(AndroidConfigurations::currentConfig().ndkLocation())
            && !originalTargetTriple().isEmpty();
}

void AndroidToolChain::addToEnvironment(Environment &env) const
{
    env.set(QLatin1String("ANDROID_NDK_HOST"),
            AndroidConfigurations::currentConfig().toolchainHost());
    const Utils::FileName javaHome = AndroidConfigurations::currentConfig().openJDKLocation();
    if (!javaHome.isEmpty() && javaHome.toFileInfo().exists()) {
        env.set(QLatin1String("JAVA_HOME"), javaHome.toString());
        Utils::FileName javaBin = javaHome;
        javaBin.appendPath(QLatin1String("bin"));
        if (!Utils::contains(env.path(), [&javaBin](const Utils::FileName &p) { return p == javaBin; }))
            env.prependOrSetPath(javaBin.toUserOutput());
    }
    env.set(QLatin1String("ANDROID_HOME"),
            AndroidConfigurations::currentConfig().sdkLocation().toString());
    env.set(QLatin1String("ANDROID_SDK_ROOT"),
            AndroidConfigurations::currentConfig().sdkLocation().toString());
}

FileName AndroidToolChain::suggestedDebugger() const
{
    // TODO: Make use of LLDB if available.
    return AndroidConfigurations::currentConfig().gdbPath(targetAbi());
}

FileName AndroidToolChain::suggestedGdbServer() const
{
    return AndroidConfigurations::currentConfig().gdbServer(targetAbi());
}

bool AndroidToolChain::fromMap(const QVariantMap &data)
{
    if (!ClangToolChain::fromMap(data))
        return false;
    return isValid();
}

FileNameList AndroidToolChain::suggestedMkspecList() const
{
    return FileNameList() << FileName::fromLatin1("android-g++")
                          << FileName::fromLatin1("android-clang");
}

QString AndroidToolChain::makeCommand(const Environment &env) const
{
    Q_UNUSED(env);
    FileName makePath = AndroidConfigurations::currentConfig().makePath();
    return makePath.exists() ? makePath.toString() : "make";
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
    setDisplayName(tr("Android Clang"));
}

QSet<Core::Id> Android::Internal::AndroidToolChainFactory::supportedLanguages() const
{
    return {ProjectExplorer::Constants::CXX_LANGUAGE_ID};
}

ToolChainList AndroidToolChainFactory::autoDetect(CToolChainList &alreadyKnown)
{
    return autodetectToolChainsForNdk(alreadyKnown);
}

bool AndroidToolChainFactory::canRestore(const QVariantMap &data)
{
    return typeIdFromMap(data) == Constants::ANDROID_TOOLCHAIN_ID;
}

ToolChain *AndroidToolChainFactory::restore(const QVariantMap &data)
{
    auto tc = new AndroidToolChain();
    if (tc->fromMap(data))
        return tc;

    delete tc;
    return nullptr;
}

static FileName clangPlusPlusPath(const FileName &clangPath)
{
    return clangPath.parentDir().appendPath(
                HostOsInfo::withExecutableSuffix(
                    QFileInfo(clangPath.toString()).baseName() + "++"));
}

ToolChainList AndroidToolChainFactory::autodetectToolChainsForNdk(CToolChainList &alreadyKnown)
{
    QList<ToolChain *> result;
    FileName clangPath = AndroidConfigurations::currentConfig().clangPath();
    if (!clangPath.exists()) {
        qCDebug(androidTCLog) << "Clang toolchains detection fails. Can not find Clang"<< clangPath;
        return result;
    }

    qCDebug(androidTCLog) << "Detecting toolchains from Android NDK:"
                          << AndroidConfigurations::currentConfig().ndkLocation();

    for (const Core::Id &lang : LanguageIds) {
        FileName compilerCommand = clangPath;
        if (lang == ProjectExplorer::Constants::CXX_LANGUAGE_ID)
            compilerCommand = clangPlusPlusPath(clangPath);

        if (!compilerCommand.exists()) {
            qCDebug(androidTCLog) << "Skipping Clang toolchain. Can not find compiler"
                                  << compilerCommand;
            continue;
        }

        auto targetItr = ClangTargets.constBegin();
        while (targetItr != ClangTargets.constEnd()) {
            const Abi &abi = targetItr.value();
            ToolChain *tc = findToolChain(compilerCommand, lang, targetItr.key(), alreadyKnown);
            if (tc) {
                qCDebug(androidTCLog) << "Tool chain already known" << abi.toString() << lang;
            } else {
                qCDebug(androidTCLog) << "New Clang toolchain found" << abi.toString() << lang;
                auto atc = new AndroidToolChain(targetItr.key(), lang);
                atc->resetToolChain(compilerCommand);
                tc = atc;
            }
            result << tc;
            ++targetItr;
        }
    }

    return result;
}

// for fromMap
AndroidToolChain::AndroidToolChain()
    : ClangToolChain(Constants::ANDROID_TOOLCHAIN_ID, ToolChain::ManualDetection)
{
}


AndroidToolChain::AndroidToolChain(const QString& target, Core::Id languageId)
    : ClangToolChain(Constants::ANDROID_TOOLCHAIN_ID, ToolChain::AutoDetection)
{
    setOriginalTargetTriple(target);
    setLanguage(languageId);
    setTargetAbi(ClangTargets[target]);
    setPlatformCodeGenFlags({"-target", target});
    setPlatformLinkerFlags({"-target", target});
    setDisplayName(QString::fromLatin1("Android Clang (%1, %2)")
                   .arg(ToolChainManager::displayNameOfLanguageId(languageId),
                        AndroidConfig::displayName(targetAbi())));
}

} // namespace Internal
} // namespace Android
