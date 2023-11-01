// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "webassemblytoolchain.h"

#include "webassemblyconstants.h"
#include "webassemblyemsdk.h"
#include "webassemblysettings.h"
#include "webassemblytr.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmacro.h>
#include <projectexplorer/toolchainmanager.h>

#include <qtsupport/qtkitaspect.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;

namespace WebAssembly {
namespace Internal {

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

static void addRegisteredMinGWToEnvironment(Environment &env)
{
    if (!ToolChainManager::isLoaded()) {
        // Avoid querying the ToolChainManager before it is loaded, which is the case during
        // toolchain restoration. The compiler version can be determined without MinGW in path.
        return;
    }

    const ToolChain *toolChain = ToolChainManager::toolChain([](const ToolChain *t){
        return t->typeId() == ProjectExplorer::Constants::MINGW_TOOLCHAIN_TYPEID;
    });
    if (toolChain)
        env.appendOrSetPath(toolChain->compilerCommand().parentDir());
}

void WebAssemblyToolChain::addToEnvironment(Environment &env) const
{
    const FilePath emSdk = settings().emSdk();
    WebAssemblyEmSdk::addToEnvironment(emSdk, env);
    if (env.osType() == OsTypeWindows)
        addRegisteredMinGWToEnvironment(env); // qmake based builds require [mingw32-]make.exe
}

WebAssemblyToolChain::WebAssemblyToolChain() :
    GccToolChain(Constants::WEBASSEMBLY_TOOLCHAIN_TYPEID)
{
    setSupportedAbis({toolChainAbi()});
    setTargetAbi(toolChainAbi());
    setTypeDisplayName(Tr::tr("Emscripten Compiler"));
}

FilePath WebAssemblyToolChain::makeCommand(const Environment &environment) const
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

bool WebAssemblyToolChain::isValid() const
{
    return GccToolChain::isValid()
            && QVersionNumber::fromString(version()) >= minimumSupportedEmSdkVersion();
}

const QVersionNumber &WebAssemblyToolChain::minimumSupportedEmSdkVersion()
{
    static const QVersionNumber number(1, 39);
    return number;
}

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
    for (auto languageId : {ProjectExplorer::Constants::C_LANGUAGE_ID,
                            ProjectExplorer::Constants::CXX_LANGUAGE_ID}) {
        auto toolChain = new WebAssemblyToolChain;
        toolChain->setLanguage(languageId);
        toolChain->setDetection(ToolChain::AutoDetection);
        const bool cLanguage = languageId == ProjectExplorer::Constants::C_LANGUAGE_ID;
        const QString script = QLatin1String(cLanguage ? "emcc" : "em++")
                + QLatin1String(sdk.osType() == OsTypeWindows ? ".bat" : "");
        const FilePath scriptFile = sdk.withNewPath(script).searchInDirectories(env.path());
        toolChain->setCompilerCommand(scriptFile);

        const QString displayName = Tr::tr("Emscripten Compiler %1 for %2")
                .arg(toolChain->version(), QLatin1String(cLanguage ? "C" : "C++"));
        toolChain->setDisplayName(displayName);
        result.append(toolChain);
    }

    return result;
}

void WebAssemblyToolChain::registerToolChains()
{
    // Remove old toolchains
    for (ToolChain *tc : ToolChainManager::findToolChains(toolChainAbi())) {
         if (tc->detection() != ToolChain::AutoDetection)
             continue;
         ToolChainManager::deregisterToolChain(tc);
    };

    // Create new toolchains and register them
    ToolchainDetector detector({}, {}, {});
    const Toolchains toolchains = doAutoDetect(detector);
    for (auto toolChain : toolchains)
        ToolChainManager::registerToolChain(toolChain);

    // Let kits pick up the new toolchains
    for (Kit *kit : KitManager::kits()) {
        if (!kit->isAutoDetected())
            continue;
        const QtVersion *qtVersion = QtKitAspect::qtVersion(kit);
        if (!qtVersion || qtVersion->type() != Constants::WEBASSEMBLY_QT_VERSION)
            continue;
        kit->fix();
    }
}

bool WebAssemblyToolChain::areToolChainsRegistered()
{
    return !ToolChainManager::findToolChains(toolChainAbi()).isEmpty();
}

WebAssemblyToolChainFactory::WebAssemblyToolChainFactory()
{
    setDisplayName(Tr::tr("Emscripten"));
    setSupportedToolChainType(Constants::WEBASSEMBLY_TOOLCHAIN_TYPEID);
    setSupportedLanguages({ProjectExplorer::Constants::C_LANGUAGE_ID,
                           ProjectExplorer::Constants::CXX_LANGUAGE_ID});
    setToolchainConstructor([] { return new WebAssemblyToolChain; });
    setUserCreatable(true);
}

Toolchains WebAssemblyToolChainFactory::autoDetect(const ToolchainDetector &detector) const
{
    return doAutoDetect(detector);
}

} // namespace Internal
} // namespace WebAssembly
