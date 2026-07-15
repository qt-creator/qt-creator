// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "harmonyossdk.h"
#include "harmonyosconstants.h"

#include <utils/environment.h>
#include <utils/hostosinfo.h>

using namespace Utils;

namespace HarmonyOs::Internal::Sdk {

FilePath nativeSdkPath(const FilePath &sdkRoot)
{
    if (sdkRoot.isEmpty())
        return {};

    // Candidate locations of the "openharmony/native" folder, relative to the configured
    // SDK root. Covers a DevEco Studio installation ("<root>/sdk/..."), a command-line-tools
    // installation ("<root>/sdk/...") and pointing directly at the "sdk" or "native" folder.
    static const QStringList candidates = {
        "sdk/default/openharmony/native",
        "default/openharmony/native",
        "openharmony/native",
        "native",
        "",
    };

    for (const QString &candidate : candidates) {
        const FilePath native = candidate.isEmpty() ? sdkRoot : sdkRoot.pathAppended(candidate);
        if (native.pathAppended("llvm").isDir() && native.pathAppended("sysroot").isDir())
            return native;
    }
    return {};
}

FilePath clangCompiler(const FilePath &sdkRoot, bool cxx)
{
    const FilePath native = nativeSdkPath(sdkRoot);
    if (native.isEmpty())
        return {};
    const QString compiler = cxx ? QString("clang++") : QString("clang");
    return native.pathAppended("llvm/bin").pathAppended(compiler).withExecutableSuffix();
}

FilePath lldbCommand(const FilePath &sdkRoot)
{
    const FilePath native = nativeSdkPath(sdkRoot);
    if (native.isEmpty())
        return {};
    return native.pathAppended("llvm/bin/lldb").withExecutableSuffix();
}

FilePath cmakeToolchainFile(const FilePath &sdkRoot)
{
    const FilePath native = nativeSdkPath(sdkRoot);
    if (native.isEmpty())
        return {};
    const FilePath toolchainFile = native.pathAppended("build/cmake/ohos.toolchain.cmake");
    return toolchainFile.exists() ? toolchainFile : FilePath();
}

FilePath hdcCommand(const FilePath &sdkRoot)
{
    const FilePath native = nativeSdkPath(sdkRoot);
    if (native.isEmpty())
        return {};
    // hdc lives in the "toolchains" folder next to the "native" folder.
    const FilePath hdc = native.parentDir().pathAppended("toolchains/hdc").withExecutableSuffix();
    return hdc.exists() ? hdc : FilePath();
}

FilePath sysrootPath(const FilePath &sdkRoot)
{
    const FilePath native = nativeSdkPath(sdkRoot);
    if (native.isEmpty())
        return {};
    const FilePath sysroot = native.pathAppended("sysroot");
    return sysroot.isDir() ? sysroot : FilePath();
}

FilePath hvigorBinPath(const FilePath &sdkRoot)
{
    if (sdkRoot.isEmpty())
        return {};

    // DevEco Studio keeps hvigor under "tools/hvigor/bin"; the standalone command-line-tools
    // package keeps the hvigorw launcher directly in "bin".
    static const QStringList candidates = {"tools/hvigor/bin", "bin"};
    for (const QString &candidate : candidates) {
        const FilePath binDir = sdkRoot.pathAppended(candidate);
        if (binDir.pathAppended("hvigorw").withExecutableSuffix().exists()
            || binDir.pathAppended("hvigorw.bat").exists()) {
            return binDir;
        }
    }
    return {};
}

FilePath hvigorCommand(const FilePath &sdkRoot)
{
    const FilePath bin = hvigorBinPath(sdkRoot);
    if (bin.isEmpty())
        return {};
    // harmonydeployqt runs the launcher directly, so hand it the platform variant.
    const FilePath launcher = HostOsInfo::isWindowsHost() ? bin.pathAppended("hvigorw.bat")
                                                          : bin.pathAppended("hvigorw");
    return launcher.exists() ? launcher : FilePath();
}

FilePath nodeBinPath(const FilePath &sdkRoot)
{
    if (sdkRoot.isEmpty())
        return {};

    static const QStringList candidates = {"tools/node", "node"};
    for (const QString &candidate : candidates) {
        const FilePath nodeDir = sdkRoot.pathAppended(candidate);
        if (nodeDir.pathAppended("node").withExecutableSuffix().exists())
            return nodeDir;
    }
    return {};
}

FilePath devEcoSdkHome(const FilePath &sdkRoot)
{
    if (sdkRoot.isEmpty())
        return {};
    const FilePath sdk = sdkRoot.pathAppended("sdk");
    return sdk.isDir() ? sdk : sdkRoot;
}

bool isValidSdk(const FilePath &sdkRoot)
{
    const FilePath compiler = clangCompiler(sdkRoot, /*cxx=*/true);
    return !compiler.isEmpty() && compiler.exists();
}

void addToEnvironment(const FilePath &sdkRoot, Environment &env)
{
    const FilePath native = nativeSdkPath(sdkRoot);
    if (!native.isEmpty())
        env.set(Constants::NATIVE_OHOS_SDK_ENV_VAR, native.toUserOutput());

    const FilePath devEco = devEcoSdkHome(sdkRoot);
    if (!devEco.isEmpty())
        env.set(Constants::DEVECO_SDK_HOME_ENV_VAR, devEco.toUserOutput());

    const FilePath hvigor = hvigorBinPath(sdkRoot);
    if (!hvigor.isEmpty())
        env.prependOrSetPath(hvigor);

    // harmonydeployqt does not search PATH for hvigor; it needs this env var.
    const FilePath hvigorw = hvigorCommand(sdkRoot);
    if (!hvigorw.isEmpty())
        env.set(Constants::HVIGOR_ENV_VAR, hvigorw.toUserOutput());

    const FilePath node = nodeBinPath(sdkRoot);
    if (!node.isEmpty())
        env.prependOrSetPath(node);
}

FilePath detectDevEcoSdk()
{
    FilePaths candidates;

    // An explicitly configured SDK home wins. It points at the "sdk" folder, so the
    // installation root that also holds the build tools is its parent.
    const QString envSdkHome = qtcEnvironmentVariable(Constants::DEVECO_SDK_HOME_ENV_VAR);
    if (!envSdkHome.isEmpty()) {
        const FilePath sdkHome = FilePath::fromUserInput(envSdkHome);
        candidates << sdkHome.parentDir() << sdkHome;
    }

    if (HostOsInfo::isWindowsHost()) {
        const QString localAppData = qtcEnvironmentVariable("LOCALAPPDATA");
        if (!localAppData.isEmpty()) {
            candidates << FilePath::fromUserInput(localAppData)
                              .pathAppended("Huawei/DevEco Studio");
        }
    } else if (HostOsInfo::isMacHost()) {
        candidates << FilePath::fromUserInput("/Applications/DevEco-Studio.app/Contents");
    }

    for (const FilePath &candidate : candidates) {
        if (isValidSdk(candidate))
            return candidate;
    }
    return {};
}

} // namespace HarmonyOs::Internal::Sdk
