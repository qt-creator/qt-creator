// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

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

// The sysroot of the native SDK.
Utils::FilePath sysrootPath(const Utils::FilePath &sdkRoot);

// The CMake toolchain file that Qt's qt.toolchain.cmake chain-loads for HarmonyOS.
Utils::FilePath cmakeToolchainFile(const Utils::FilePath &sdkRoot);

// Directory holding the hvigorw launcher, to be prepended to PATH for deployment.
Utils::FilePath hvigorBinPath(const Utils::FilePath &sdkRoot);

// Directory holding the bundled Node.js, which hvigor requires.
Utils::FilePath nodeBinPath(const Utils::FilePath &sdkRoot);

// Value for the DEVECO_SDK_HOME environment variable.
Utils::FilePath devEcoSdkHome(const Utils::FilePath &sdkRoot);

// A configured SDK root is valid when the native SDK and its clang compiler exist.
bool isValidSdk(const Utils::FilePath &sdkRoot);

// Sets NATIVE_OHOS_SDK and DEVECO_SDK_HOME and prepends hvigor and node to PATH.
void addToEnvironment(const Utils::FilePath &sdkRoot, Utils::Environment &env);

// Best-effort location of an installed DevEco Studio SDK (Windows and macOS only).
// Returns an empty path if none is found.
Utils::FilePath detectDevEcoSdk();

} // namespace HarmonyOs::Internal::Sdk
