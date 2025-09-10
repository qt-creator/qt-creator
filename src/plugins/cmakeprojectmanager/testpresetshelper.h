// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

namespace CMakeProjectManager::Internal {

namespace PresetsDetails {
class TestPreset;
}

QStringList presetToCTestArgs(const PresetsDetails::TestPreset &preset);

} // namespace CMakeProjectManager::Internal
