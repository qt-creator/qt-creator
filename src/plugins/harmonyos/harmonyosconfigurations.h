// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

namespace ProjectExplorer { class Abi; }

namespace HarmonyOs::Internal {

// The HarmonyOS ABI name, as used by the Qt installation directories, the ohos mkspec
// and the SDK's CMake toolchain file.
QString ohosAbiName(const ProjectExplorer::Abi &abi);

// Re-detect toolchains and rebuild the automatic kit list from the configured SDK.
void applyConfig();

void registerNewToolchains();
void updateAutomaticKitList();

} // namespace HarmonyOs::Internal
