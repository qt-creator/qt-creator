// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "harmonyostoolchain.h"

#include "harmonyosconstants.h"
#include "harmonyossdk.h"
#include "harmonyossettings.h"
#include "harmonyostr.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchainconfigwidget.h>
#include <projectexplorer/toolchainmanager.h>

#include <utils/algorithm.h>
#include <utils/environment.h>

#include <QLoggingCategory>

using namespace ProjectExplorer;
using namespace Utils;

namespace HarmonyOs::Internal {

static Q_LOGGING_CATEGORY(harmonyTCLog, "qtc.harmonyos.toolchainmanagement", QtWarningMsg)

using ClangTargetsType = QHash<QString, Abi>;

// Maps the LLVM target triples that the ohos-clang mkspec passes via "--target" to the ABIs
// that Qt for HarmonyOS reports. The OpenHarmony flavor makes these match strictly against a
// HarmonyOS Qt version's ABI (see ProjectExplorer::Abi::isCompatibleWith).
static const ClangTargetsType &clangTargets()
{
    static const ClangTargetsType targets {
        {"aarch64-linux-ohos",
         Abi(Abi::ArmArchitecture, Abi::LinuxOS, Abi::OpenHarmonyLinuxFlavor, Abi::ElfFormat, 64)},
        {"armv7-linux-gnueabihf-ohos",
         Abi(Abi::ArmArchitecture, Abi::LinuxOS, Abi::OpenHarmonyLinuxFlavor, Abi::ElfFormat, 32)},
        {"x86_64-linux-ohos",
         Abi(Abi::X86Architecture, Abi::LinuxOS, Abi::OpenHarmonyLinuxFlavor, Abi::ElfFormat, 64)},
    };
    return targets;
}

static QString archDisplayName(const Abi &abi)
{
    if (abi.architecture() == Abi::ArmArchitecture)
        return abi.wordWidth() == 64 ? QString("aarch64") : QString("armv7");
    if (abi.architecture() == Abi::X86Architecture)
        return QString("x86_64");
    return QString("unknown");
}

static Toolchain *findToolchain(const FilePath &compilerPath, Id lang, const QString &target,
                                const ToolchainList &alreadyKnown)
{
    return Utils::findOrDefault(alreadyKnown, [&](Toolchain *tc) {
        return tc->typeId() == Constants::HARMONYOS_TOOLCHAIN_TYPEID
               && tc->language() == lang
               && tc->targetAbi() == clangTargets().value(target)
               && tc->compilerCommand() == compilerPath;
    });
}

HarmonyOsToolchain::HarmonyOsToolchain()
    : GccToolchain(Constants::HARMONYOS_TOOLCHAIN_TYPEID, Clang)
{
    setTypeDisplayName(Tr::tr("HarmonyOS Clang"));
}

bool HarmonyOsToolchain::isValid() const
{
    const FilePath native = Sdk::nativeSdkPath(settings().sdkLocation());
    const bool isChildOfSdk = !native.isEmpty() && compilerCommand().isChildOf(native);

    return GccToolchain::isValid() && typeId() == Constants::HARMONYOS_TOOLCHAIN_TYPEID
           && targetAbi().isValid() && isChildOfSdk && !originalTargetTriple().isEmpty();
}

void HarmonyOsToolchain::addToEnvironment(Environment &env) const
{
    // The single place that injects the SDK environment into a kit build: the
    // ohos-clang mkspec reads NATIVE_OHOS_SDK, and the deploy tools need hvigor.
    Sdk::addToEnvironment(settings().sdkLocation(), env);
}

void HarmonyOsToolchain::fromMap(const Store &data)
{
    GccToolchain::fromMap(data);
    if (hasError())
        return;
    if (!isValid())
        reportError();
}

QStringList HarmonyOsToolchain::suggestedMkspecList() const
{
    return {"ohos-clang"};
}

GccToolchain::DetectedAbisResult HarmonyOsToolchain::detectSupportedAbis() const
{
    for (auto it = clangTargets().constBegin(); it != clangTargets().constEnd(); ++it) {
        if (it.value() == targetAbi())
            return GccToolchain::DetectedAbisResult({targetAbi()}, it.key());
    }
    return GccToolchain::DetectedAbisResult({targetAbi()}, "");
}

ToolchainList autodetectToolchains(const ToolchainList &alreadyKnown)
{
    const FilePath sdkRoot = settings().sdkLocation();
    if (sdkRoot.isEmpty())
        return {};
    const FilePath clang = Sdk::clangCompiler(sdkRoot, /*cxx=*/false);
    const FilePath clangxx = Sdk::clangCompiler(sdkRoot, /*cxx=*/true);
    if (clang.isEmpty() || !clang.exists() || clangxx.isEmpty() || !clangxx.exists()) {
        qCDebug(harmonyTCLog) << "Clang toolchain detection failed. Cannot find clang in" << sdkRoot;
        return {};
    }

    const Id languageIds[] {
        ProjectExplorer::Constants::CXX_LANGUAGE_ID,
        ProjectExplorer::Constants::C_LANGUAGE_ID,
    };

    ToolchainList newToolchains;
    for (const Id &lang : languageIds) {
        const FilePath compiler = lang == ProjectExplorer::Constants::CXX_LANGUAGE_ID ? clangxx
                                                                                      : clang;
        for (auto it = clangTargets().constBegin(); it != clangTargets().constEnd(); ++it) {
            const Abi &abi = it.value();
            const QString target = it.key();
            const QString displayName = QString("HarmonyOS Clang (%1)").arg(archDisplayName(abi));

            Toolchain *tc = findToolchain(compiler, lang, target, alreadyKnown);
            if (tc) {
                if (tc->displayName() != displayName)
                    tc->setDisplayName(displayName);
            } else {
                qCDebug(harmonyTCLog) << "New HarmonyOS Clang toolchain" << abi.toString() << lang;
                auto htc = new HarmonyOsToolchain();
                htc->setOriginalTargetTriple(target);
                htc->setLanguage(lang);
                htc->setTargetAbi(abi);
                htc->setPlatformCodeGenFlags({"-target", target});
                htc->setPlatformLinkerFlags({"-target", target});
                htc->setDisplayName(displayName);
                tc = htc;
                newToolchains << tc;
            }

            // Also re-detect known toolchains, so a changed compiler gets picked up.
            if (auto gccTc = dynamic_cast<GccToolchain *>(tc))
                gccTc->resetToolchain(compiler);
            tc->setDetectionSource(DetectionSource::FromSystem);
        }
    }

    return newToolchains;
}

class HarmonyOsToolchainFactory final : public ToolchainFactory
{
public:
    HarmonyOsToolchainFactory()
    {
        setDisplayName(Tr::tr("HarmonyOS Clang"));
        setSupportedToolchainType(Constants::HARMONYOS_TOOLCHAIN_TYPEID);
        setSupportedLanguages({ProjectExplorer::Constants::C_LANGUAGE_ID,
                               ProjectExplorer::Constants::CXX_LANGUAGE_ID});
        setToolchainConstructor([] { return new HarmonyOsToolchain; });
    }

private:
    std::unique_ptr<ToolchainConfigWidget> createConfigurationWidget(
        const ToolchainBundle &bundle) const final
    {
        return GccToolchain::createConfigurationWidget(bundle);
    }

    FilePath correspondingCompilerCommand(const FilePath &srcPath, Id targetLang) const override
    {
        return GccToolchain::correspondingCompilerCommand(srcPath, targetLang, "clang", "clang++");
    }
};

void setupHarmonyOsToolchain()
{
    static HarmonyOsToolchainFactory theHarmonyOsToolchainFactory;
}

} // namespace HarmonyOs::Internal
