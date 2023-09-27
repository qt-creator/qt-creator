// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidconfigurations.h"
#include "androidconstants.h"
#include "androidtoolchain.h"
#include "androidtr.h"

#include <projectexplorer/kitmanager.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/environment.h>

#include <QLoggingCategory>

using namespace ProjectExplorer;
using namespace Utils;

namespace Android {
namespace Internal {

static Q_LOGGING_CATEGORY(androidTCLog, "qtc.android.toolchainmanagement", QtWarningMsg);

using ClangTargetsType = QHash<QString, Abi>;
Q_GLOBAL_STATIC_WITH_ARGS(ClangTargetsType, ClangTargets, ({
        {"arm-linux-androideabi",
         Abi(Abi::ArmArchitecture, Abi::LinuxOS, Abi::AndroidLinuxFlavor, Abi::ElfFormat, 32)},
        {"i686-linux-android",
         Abi(Abi::X86Architecture, Abi::LinuxOS, Abi::AndroidLinuxFlavor, Abi::ElfFormat, 32)},
        {"x86_64-linux-android",
         Abi(Abi::X86Architecture, Abi::LinuxOS, Abi::AndroidLinuxFlavor, Abi::ElfFormat, 64)},
        {"aarch64-linux-android",
         Abi(Abi::ArmArchitecture, Abi::LinuxOS, Abi::AndroidLinuxFlavor, Abi::ElfFormat, 64)}}
));

static ToolChain *findToolChain(FilePath &compilerPath, Id lang, const QString &target,
                                const ToolChainList &alreadyKnown)
{
    ToolChain *tc = Utils::findOrDefault(alreadyKnown, [target, compilerPath, lang](ToolChain *tc) {
        return tc->typeId() == Constants::ANDROID_TOOLCHAIN_TYPEID
                && tc->language() == lang
                && tc->targetAbi() == ClangTargets->value(target)
                && tc->compilerCommand() == compilerPath;
    });
    return tc;
}

AndroidToolChain::AndroidToolChain()
    : GccToolChain(Constants::ANDROID_TOOLCHAIN_TYPEID, Clang)
{
    setTypeDisplayName(Tr::tr("Android Clang"));
}

FilePath AndroidToolChain::ndkLocation() const
{
    return m_ndkLocation;
}

void AndroidToolChain::setNdkLocation(const FilePath &ndkLocation)
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

    return GccToolChain::isValid() && typeId() == Constants::ANDROID_TOOLCHAIN_TYPEID
           && targetAbi().isValid() && (isChildofNdk || isChildofSdk)
           && !originalTargetTriple().isEmpty();
}

void AndroidToolChain::addToEnvironment(Environment &env) const
{
    const AndroidConfig &config = AndroidConfigurations::currentConfig();
    env.set(QLatin1String("ANDROID_NDK_HOST"), config.toolchainHostFromNdk(m_ndkLocation));
    const FilePath javaHome = config.openJDKLocation();
    if (javaHome.exists()) {
        env.set(Constants::JAVA_HOME_ENV_VAR, javaHome.toUserOutput());
        const FilePath javaBin = javaHome.pathAppended("bin");
        const FilePath currentJavaFilePath
            = env.searchInPath("java", {}, {}, FilePath::WithExeSuffix);
        if (!currentJavaFilePath.isChildOf(javaBin))
            env.prependOrSetPath(javaBin);
    }
    env.set(QLatin1String("ANDROID_HOME"), config.sdkLocation().toUserOutput());
    env.set(QLatin1String("ANDROID_SDK_ROOT"), config.sdkLocation().toUserOutput());
}

void AndroidToolChain::fromMap(const Store &data)
{
    GccToolChain::fromMap(data);
    if (hasError())
        return;
    if (!isValid())
        reportError();
}

QStringList AndroidToolChain::suggestedMkspecList() const
{
    return {"android-g++", "android-clang"};
}

FilePath AndroidToolChain::makeCommand(const Environment &env) const
{
    Q_UNUSED(env)
    FilePath makePath = AndroidConfigurations::currentConfig().makePathFromNdk(m_ndkLocation);
    return makePath.exists() ? makePath : FilePath("make");
}

GccToolChain::DetectedAbisResult AndroidToolChain::detectSupportedAbis() const
{
    for (auto itr = ClangTargets->constBegin(); itr != ClangTargets->constEnd(); ++itr) {
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
    setDisplayName(Tr::tr("Android Clang"));
    setSupportedToolChainType(Constants::ANDROID_TOOLCHAIN_TYPEID);
    setSupportedLanguages({ProjectExplorer::Constants::CXX_LANGUAGE_ID});
    setToolchainConstructor([] { return new AndroidToolChain; });
}

static FilePath clangPlusPlusPath(const FilePath &clangPath)
{
    return clangPath.parentDir().pathAppended(clangPath.baseName() + "++").withExecutableSuffix();
}

static FilePaths uniqueNdksForCurrentQtVersions()
{
    const AndroidConfig &config = AndroidConfigurations::currentConfig();

    auto androidQtVersions = QtSupport::QtVersionManager::versions(
        [](const QtSupport::QtVersion *v) {
            return v->targetDeviceTypes().contains(Android::Constants::ANDROID_DEVICE_TYPE);
        });

    FilePaths uniqueNdks;
    for (const QtSupport::QtVersion *version : androidQtVersions) {
        FilePath ndk = config.ndkLocation(version);
        if (!uniqueNdks.contains(ndk))
            uniqueNdks.append(ndk);
    }

    return uniqueNdks;
}

ToolChainList AndroidToolChainFactory::autodetectToolChains(const ToolChainList &alreadyKnown)
{
    const QList<FilePath> uniqueNdks = uniqueNdksForCurrentQtVersions();
    return autodetectToolChainsFromNdks(alreadyKnown, uniqueNdks);
}

ToolChainList AndroidToolChainFactory::autodetectToolChainsFromNdks(
    const ToolChainList &alreadyKnown,
    const QList<FilePath> &ndkLocations,
    const bool isCustom)
{
    QList<ToolChain *> result;
    const AndroidConfig config = AndroidConfigurations::currentConfig();

    const Id LanguageIds[] {
        ProjectExplorer::Constants::CXX_LANGUAGE_ID,
        ProjectExplorer::Constants::C_LANGUAGE_ID
    };

    for (const FilePath &ndkLocation : ndkLocations) {
        FilePath clangPath = config.clangPathFromNdk(ndkLocation);
        if (!clangPath.exists()) {
            qCDebug(androidTCLog) << "Clang toolchains detection fails. Can not find Clang"
                                  << clangPath;
            continue;
        }

        for (const Id &lang : LanguageIds) {
            FilePath compilerCommand = clangPath;
            if (lang == ProjectExplorer::Constants::CXX_LANGUAGE_ID)
                compilerCommand = clangPlusPlusPath(clangPath);

            if (!compilerCommand.exists()) {
                qCDebug(androidTCLog)
                    << "Skipping Clang toolchain. Can not find compiler" << compilerCommand;
                continue;
            }

            auto targetItr = ClangTargets->constBegin();
            while (targetItr != ClangTargets->constEnd()) {
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
                    atc->setTargetAbi(ClangTargets->value(target));
                    atc->setPlatformCodeGenFlags({"-target", target});
                    atc->setPlatformLinkerFlags({"-target", target});
                    atc->setDisplayName(displayName);
                    tc = atc;
                }

                // Do not only reset newly created toolchains. This triggers call to
                // addToEnvironment, so that e.g. JAVA_HOME gets updated.
                if (auto gccTc = dynamic_cast<GccToolChain*>(tc))
                    gccTc->resetToolChain(compilerCommand);

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
