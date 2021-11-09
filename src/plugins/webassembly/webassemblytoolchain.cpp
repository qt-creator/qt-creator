/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "webassemblytoolchain.h"
#include "webassemblyconstants.h"
#include "webassemblyemsdk.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmacro.h>
#include <projectexplorer/toolchainmanager.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
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
    const ToolChain *toolChain = ToolChainManager::toolChain([](const ToolChain *t){
        return t->typeId() == ProjectExplorer::Constants::MINGW_TOOLCHAIN_TYPEID;
    });
    if (toolChain)
        env.appendOrSetPath(toolChain->compilerCommand().parentDir());
}

void WebAssemblyToolChain::addToEnvironment(Environment &env) const
{
    WebAssemblyEmSdk::addToEnvironment(WebAssemblyEmSdk::registeredEmSdk(), env);
    if (env.osType() == OsTypeWindows)
        addRegisteredMinGWToEnvironment(env);
}

WebAssemblyToolChain::WebAssemblyToolChain() :
    GccToolChain(Constants::WEBASSEMBLY_TOOLCHAIN_TYPEID)
{
    setSupportedAbis({toolChainAbi()});
    setTargetAbi(toolChainAbi());
    setTypeDisplayName(tr("Emscripten Compiler"));
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

void WebAssemblyToolChain::registerToolChains()
{
    // Remove old toolchains
    for (ToolChain *tc : ToolChainManager::findToolChains(toolChainAbi())) {
         if (tc->detection() != ToolChain::AutoDetection)
             continue;
         ToolChainManager::deregisterToolChain(tc);
    };

    // Create new toolchains and register them
    ToolChainFactory *factory =
            findOrDefault(ToolChainFactory::allToolChainFactories(), [](ToolChainFactory *f){
            return f->supportedToolChainType() == Constants::WEBASSEMBLY_TOOLCHAIN_TYPEID;
    });
    QTC_ASSERT(factory, return);
    for (auto toolChain : factory->autoDetect({}, {}))
        ToolChainManager::registerToolChain(toolChain);

    // Let kits pick up the new toolchains
    for (Kit *kit : KitManager::kits()) {
        if (!kit->isAutoDetected())
            continue;
        const BaseQtVersion *qtVersion = QtKitAspect::qtVersion(kit);
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
    setDisplayName(WebAssemblyToolChain::tr("Emscripten"));
    setSupportedToolChainType(Constants::WEBASSEMBLY_TOOLCHAIN_TYPEID);
    setSupportedLanguages({ProjectExplorer::Constants::C_LANGUAGE_ID,
                           ProjectExplorer::Constants::CXX_LANGUAGE_ID});
    setToolchainConstructor([] { return new WebAssemblyToolChain; });
    setUserCreatable(true);
}

QList<ToolChain *> WebAssemblyToolChainFactory::autoDetect(
        const QList<ToolChain *> &alreadyKnown,
        const IDevice::Ptr &device)
{
    Q_UNUSED(alreadyKnown)

    const FilePath sdk = WebAssemblyEmSdk::registeredEmSdk();
    if (!WebAssemblyEmSdk::isValid(sdk))
        return {};

    if (device) {
        // Only detect toolchains from the emsdk installation device
        const FilePath deviceRoot = device->mapToGlobalPath({});
        if (deviceRoot.host() != sdk.host())
            return {};
    }

    Environment env = sdk.deviceEnvironment();
    WebAssemblyEmSdk::addToEnvironment(sdk, env);

    QList<ToolChain *> result;
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

        const QString displayName = WebAssemblyToolChain::tr("Emscripten Compiler %1 for %2")
                .arg(toolChain->version(), QLatin1String(cLanguage ? "C" : "C++"));
        toolChain->setDisplayName(displayName);
        result.append(toolChain);
    }

    return result;
}

} // namespace Internal
} // namespace WebAssembly
