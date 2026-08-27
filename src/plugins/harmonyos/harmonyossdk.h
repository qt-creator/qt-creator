// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QString>
#include <QStringList>

namespace Utils { class Environment; }

namespace HarmonyOs::Internal::Sdk {

// The "OpenHarmony native" folder (contains "llvm" and "sysroot"), which the ohos-clang
// mkspec expects in the NATIVE_OHOS_SDK environment variable. Derived from the SDK root
// the user configures (a DevEco Studio or command-line-tools installation).
Utils::FilePath nativeSdkPath(const Utils::FilePath &sdkRoot);

// The clang / clang++ compiler shipped in the native SDK.
Utils::FilePath clangCompiler(const Utils::FilePath &sdkRoot, bool cxx);

// The lldb debugger shipped in the native SDK.
Utils::FilePath lldbCommand(const Utils::FilePath &sdkRoot);

// The hdc device connector shipped in the SDK toolchains.
Utils::FilePath hdcCommand(const Utils::FilePath &sdkRoot);

Utils::FilePath hapSignToolJar(const Utils::FilePath &sdkRoot);

Utils::FilePath binarySignTool(const Utils::FilePath &sdkRoot);

// The version of the configured SDK, spelled as build-profile.json5 wants it.
QString sdkVersion(const Utils::FilePath &sdkRoot);

// The permissions a device only grants when the provisioning profile allows them.
QStringList restrictedPermissions(const Utils::FilePath &sdkRoot);

// The tool that packs a directory into an .hnp native package.
Utils::FilePath hnpcliCommand(const Utils::FilePath &sdkRoot);

// The aarch64 lldb-server to ship in a package, so that a debugged application can
// start it in its own context.
Utils::FilePath lldbServerForDevice(const Utils::FilePath &sdkRoot);

// The sysroot of the native SDK.
Utils::FilePath sysrootPath(const Utils::FilePath &sdkRoot);

// The library that holds an application at startup, built for the device from the source
// shipped beside Qt Creator. It is built once and kept beside the settings, because the
// kits name it as something to link against and every build resolves that same path.
// Returns an empty path when the SDK does not have what it takes to build it.
Utils::FilePath waitLibrary(const Utils::FilePath &sdkRoot);

// The CMake toolchain file that Qt's qt.toolchain.cmake chain-loads for HarmonyOS.
Utils::FilePath cmakeToolchainFile(const Utils::FilePath &sdkRoot);

// Directory holding the hvigorw launcher, to be prepended to PATH for deployment.
Utils::FilePath hvigorBinPath(const Utils::FilePath &sdkRoot);

// The hvigorw launcher itself, which harmonydeployqt runs via QT_HARMONYOS_HVIGOR.
Utils::FilePath hvigorCommand(const Utils::FilePath &sdkRoot);

// Directory holding the bundled Node.js, which hvigor requires.
Utils::FilePath nodeBinPath(const Utils::FilePath &sdkRoot);

// Value for the DEVECO_SDK_HOME environment variable.
Utils::FilePath devEcoSdkHome(const Utils::FilePath &sdkRoot);

// A configured SDK root is valid when the native SDK and its clang compiler exist.
bool isValidSdk(const Utils::FilePath &sdkRoot);

// Sets NATIVE_OHOS_SDK, DEVECO_SDK_HOME and QT_HARMONYOS_HVIGOR, and prepends
// hvigor and node to PATH.
void addToEnvironment(const Utils::FilePath &sdkRoot, Utils::Environment &env);

// Best-effort location of an installed DevEco Studio SDK (Windows and macOS only).
// Returns an empty path if none is found.
Utils::FilePath detectDevEcoSdk();

} // namespace HarmonyOs::Internal::Sdk
