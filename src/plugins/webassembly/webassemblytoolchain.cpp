/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>

#include <projectexplorer/abiwidget.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmacro.h>
#include <projectexplorer/toolchainmanager.h>

#include <QDir>
#include <QSettings>

namespace WebAssembly {
namespace Internal {

// See https://emscripten.org/docs/tools_reference/emsdk.html#compiler-configuration-file
struct CompilerConfiguration
{
    Utils::FilePath emSdk;
    Utils::FilePath llvmRoot;
    Utils::FilePath emConfig;
    Utils::FilePath emscriptenNativeOptimizer;
    Utils::FilePath binaryEnRoot;
    Utils::FilePath emSdkNode;
    Utils::FilePath emSdkPython;
    Utils::FilePath javaHome;
    Utils::FilePath emScripten;
};

static Utils::FilePath compilerConfigurationFile()
{
    return Utils::FilePath::fromString(QDir::homePath() + "/.emscripten");
}

static CompilerConfiguration compilerConfiguration()
{
    const QSettings configuration(compilerConfigurationFile().toString(), QSettings::IniFormat);
    auto configPath = [&configuration](const QString &key){
        return Utils::FilePath::fromString(configuration.value(key).toString().remove('\''));
    };
    const Utils::FilePath llvmRoot = configPath("LLVM_ROOT");
    return {
        llvmRoot.parentDir().parentDir(),
        llvmRoot,
        compilerConfigurationFile(),
        configPath("EMSCRIPTEN_NATIVE_OPTIMIZER"),
        configPath("BINARYEN_ROOT"),
        configPath("NODE_JS"),
        configPath("PYTHON"),
        configPath("JAVA").parentDir().parentDir(),
        configPath("EMSCRIPTEN_ROOT")
    };
}

static ProjectExplorer::Abi toolChainAbi()
{
    return {
        ProjectExplorer::Abi::AsmJsArchitecture,
        ProjectExplorer::Abi::UnknownOS,
        ProjectExplorer::Abi::UnknownFlavor,
        ProjectExplorer::Abi::EmscriptenFormat,
        32
    };
}

static void addEmscriptenToEnvironment(Utils::Environment &env)
{
    const CompilerConfiguration configuration = compilerConfiguration();

    env.prependOrSetPath(configuration.emScripten.toUserOutput());
    env.prependOrSetPath(configuration.javaHome.toUserOutput() + "/bin");
    env.prependOrSetPath(configuration.emSdkPython.parentDir().toUserOutput());
    env.prependOrSetPath(configuration.emSdkNode.parentDir().toUserOutput());
    env.prependOrSetPath(configuration.llvmRoot.toUserOutput());
    env.prependOrSetPath(configuration.emSdk.toUserOutput());

    env.set("EMSDK", configuration.emSdk.toUserOutput());
    env.set("EM_CONFIG", configuration.emConfig.toUserOutput());
    env.set("LLVM_ROOT", configuration.llvmRoot.toUserOutput());
    env.set("EMSCRIPTEN_NATIVE_OPTIMIZER", configuration.emscriptenNativeOptimizer.toUserOutput());
    env.set("BINARYEN_ROOT", configuration.binaryEnRoot.toUserOutput());
    env.set("EMSDK_NODE", configuration.emSdkNode.toUserOutput());
    env.set("EMSDK_PYTHON", configuration.emSdkPython.toUserOutput());
    env.set("JAVA_HOME", configuration.javaHome.toUserOutput());
    env.set("EMSCRIPTEN", configuration.emScripten.toUserOutput());
}

static void addRegisteredMinGWToEnvironment(Utils::Environment &env)
{
    using namespace ProjectExplorer;
    const ToolChain *toolChain = ToolChainManager::toolChain([](const ToolChain *t){
        return t->typeId() == ProjectExplorer::Constants::MINGW_TOOLCHAIN_TYPEID;
    });
    if (toolChain) {
        const QString mingwPath = toolChain->compilerCommand().parentDir().toUserOutput();
        env.appendOrSetPath(mingwPath);
    }
}

void WebAssemblyToolChain::addToEnvironment(Utils::Environment &env) const
{
    addEmscriptenToEnvironment(env);
    if (Utils::HostOsInfo::isWindowsHost())
        addRegisteredMinGWToEnvironment(env);
}

WebAssemblyToolChain::WebAssemblyToolChain() :
    ClangToolChain(Constants::WEBASSEMBLY_TOOLCHAIN_TYPEID)
{
    const CompilerConfiguration configuration = compilerConfiguration();
    const QString command = configuration.llvmRoot.toString()
            + Utils::HostOsInfo::withExecutableSuffix("/clang");
    setLanguage(ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    setCompilerCommand(Utils::FilePath::fromString(command));
    setSupportedAbis({toolChainAbi()});
    setTargetAbi(toolChainAbi());
    const QString typeAndDisplayName = tr("Emscripten Compiler");
    setDisplayName(typeAndDisplayName);
    setTypeDisplayName(typeAndDisplayName);
}

WebAssemblyToolChainFactory::WebAssemblyToolChainFactory()
{
    setDisplayName(WebAssemblyToolChain::tr("WebAssembly"));
    setSupportedToolChainType(Constants::WEBASSEMBLY_TOOLCHAIN_TYPEID);
    setSupportedLanguages({ProjectExplorer::Constants::C_LANGUAGE_ID,
                           ProjectExplorer::Constants::CXX_LANGUAGE_ID});
    setToolchainConstructor([] { return new WebAssemblyToolChain; });
    setUserCreatable(true);
}

QList<ProjectExplorer::ToolChain *> WebAssemblyToolChainFactory::autoDetect(
        const QList<ProjectExplorer::ToolChain *> &alreadyKnown)
{
    Q_UNUSED(alreadyKnown)

    auto cToolChain = new WebAssemblyToolChain;
    cToolChain->setLanguage(ProjectExplorer::Constants::C_LANGUAGE_ID);
    cToolChain->setDetection(ProjectExplorer::ToolChain::AutoDetection);

    auto cxxToolChain = new WebAssemblyToolChain;
    cxxToolChain->setLanguage(ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    cxxToolChain->setDetection(ProjectExplorer::ToolChain::AutoDetection);

    return {cToolChain, cxxToolChain};
}

} // namespace Internal
} // namespace WebAssembly
