// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace HarmonyOs::Internal {

// Re-detect toolchains and rebuild the automatic kit list from the configured SDK.
void applyConfig();

void registerNewToolchains();
void updateAutomaticKitList();

} // namespace HarmonyOs::Internal
