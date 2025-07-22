// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "webassemblytoolchain.h"

#include "webassemblyconstants.h"
#include "webassemblyemsdk.h"
#include "webassemblysettings.h"
#include "webassemblytr.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/gcctoolchain.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmacro.h>
#include <projectexplorer/toolchainconfigwidget.h>
#include <projectexplorer/toolchainmanager.h>

#include <qtsupport/qtkitaspect.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QVersionNumber>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;

namespace WebAssembly::Internal {

static const Abi &toolChainAbi()
{
    static const Abi abi(
                Abi::AsmJsArchitecture,
                Abi::UnknownOS,
                Abi::UnknownFlavor,
                Abi::EmscriptenFormat,
                32);
    return abi;
}

const QVersionNumber &minimumSupportedEmSdkVersion()
{
    static const QVersionNumber number(1, 39);
    return number;
}

static void addRegisteredMinGWToEnvironment(Environment &env)
{
    if (!ToolchainManager::isLoaded()) {
        // Avoid querying the ToolchainManager before it is loaded, which is the case during
        // toolchain restoration. The compiler version can be determined without MinGW in path.
        return;
    }

    const Toolchain *toolChain = ToolchainManager::toolchain([](const Toolchain *t){
        return t->typeId() == ProjectExplorer::Constants::MINGW_TOOLCHAIN_TYPEID;
    });
    if (toolChain)
        env.appendOrSetPath(toolChain->compilerCommand().parentDir());
}

class WebAssemblyToolChain final : public GccToolchain
{
public:
    WebAssemblyToolChain() :
        GccToolchain(Constants::WEBASSEMBLY_TOOLCHAIN_TYPEID)
    {
        setSupportedAbis({toolChainAbi()});
        setTargetAbi(toolChainAbi());
        setTypeDisplayName(Tr::tr("Emscripten Compiler"));
    }

    void addToEnvironment(Environment &env) const final
    {
        const FilePath emSdk = settings().emSdk();
        WebAssemblyEmSdk::addToEnvironment(emSdk, env);
        if (env.osType() == OsTypeWindows)
            addRegisteredMinGWToEnvironment(env); // qmake based builds require [mingw32-]make.exe
    }

    FilePath makeCommand(const Environment &environment) const final
    {
        // Diverged duplicate of ClangToolChain::makeCommand and MingwToolChain::makeCommand
        const QStringList makes = environment.osType() == OsTypeWindows
                                      ? QStringList({"mingw32-make.exe", "make.exe"})
                                      : QStringList({"make"});

        FilePath tmp;
        for (const QString &make : makes) {
            tmp = environment.searchInPath(make);
            if (!tmp.isEmpty())
                return tmp;
        }
        return FilePath::fromString(makes.first());
    }

    bool isValid() const final
    {
        return GccToolchain::isValid()
               && QVersionNumber::fromString(version()) >= minimumSupportedEmSdkVersion();
    }
};

static Toolchains doAutoDetect(const ToolchainDetector &detector)
{
    const FilePath sdk = settings().emSdk();
    if (!WebAssemblyEmSdk::isValid(sdk))
        return {};

    if (detector.device
        && detector.device->type() != ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
        // Only detect toolchains from the emsdk installation device
        const FilePath deviceRoot = detector.device->rootPath();
        if (deviceRoot.host() != sdk.host())
            return {};
    }

    Environment env = sdk.deviceEnvironment();
    WebAssemblyEmSdk::addToEnvironment(sdk, env);

    Toolchains result;
    for (auto languageId : {Id(ProjectExplorer::Constants::C_LANGUAGE_ID),
                            Id(ProjectExplorer::Constants::CXX_LANGUAGE_ID)}) {
        auto toolChain = new WebAssemblyToolChain;
        toolChain->setLanguage(languageId);
        toolChain->setDetectionSource(DetectionSource::FromSystem);
        const bool cLanguage = languageId == ProjectExplorer::Constants::C_LANGUAGE_ID;
        const QString script = QLatin1String(cLanguage ? "emcc" : "em++")
                + QLatin1String(sdk.osType() == OsTypeWindows ? ".bat" : "");
        const FilePath scriptFile = sdk.withNewPath(script).searchInDirectories(env.path());
        toolChain->setCompilerCommand(scriptFile);

        const QString displayName = Tr::tr("Emscripten Compiler %1").arg(toolChain->version());
        toolChain->setDisplayName(displayName);
        result.append(toolChain);
    }

    return result;
}

void registerToolChains()
{
    // Remove old toolchains
    const Toolchains oldToolchains = Utils::filtered(
        ToolchainManager::findToolchains(toolChainAbi()), [](Toolchain *tc) {
            return tc->detectionSource().type == DetectionSource::FromSystem;
        });
    ToolchainManager::deregisterToolchains(oldToolchains);

    // Create new toolchains and register them
    ToolchainManager::registerToolchains(
        doAutoDetect(ToolchainDetector({}, DeviceManager::defaultDesktopDevice(), {})));

    // Let kits pick up the new toolchains
    for (Kit *kit : KitManager::kits()) {
        if (!kit->detectionSource().isAutoDetected())
            continue;
        const QtVersion *qtVersion = QtKitAspect::qtVersion(kit);
        if (!qtVersion || qtVersion->type() != Constants::WEBASSEMBLY_QT_VERSION)
            continue;
        kit->fix();
    }
}

bool areToolChainsRegistered()
{
    return !ToolchainManager::findToolchains(toolChainAbi()).isEmpty();
}

class WebAssemblyToolchainFactory final : public ToolchainFactory
{
public:
    WebAssemblyToolchainFactory()
    {
        setDisplayName(Tr::tr("Emscripten"));
        setSupportedToolchainType(Constants::WEBASSEMBLY_TOOLCHAIN_TYPEID);
        setSupportedLanguages({ProjectExplorer::Constants::C_LANGUAGE_ID,
                               ProjectExplorer::Constants::CXX_LANGUAGE_ID});
        setToolchainConstructor([] { return new WebAssemblyToolChain; });
        setUserCreatable(true);
    }

private:
    Toolchains autoDetect(const ToolchainDetector &detector) const override
    {
        return doAutoDetect(detector);
    }

    std::unique_ptr<ToolchainConfigWidget> createConfigurationWidget(
        const ToolchainBundle &bundle) const override
    {
        return GccToolchain::createConfigurationWidget(bundle);
    }

    FilePath correspondingCompilerCommand(const FilePath &srcPath, Id targetLang) const override
    {
        return GccToolchain::correspondingCompilerCommand(srcPath, targetLang, "emcc", "em++");
    }
};

void setupWebAssemblyToolchain()
{
    static WebAssemblyToolchainFactory theWebAssemblyToolchainFactory;
}

} // WebAssembly::Internal
