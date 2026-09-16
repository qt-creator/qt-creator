// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>
#include <QVersionNumber>

namespace CMakeProjectManager::Internal {

namespace PresetsDetails {
class TestPreset;
}

// The lowest ctest version that understands the "--" separator which
// "testPassthroughArguments" is passed after.
const QVersionNumber &ctestPassthroughArgumentsVersion();

QStringList presetToCTestArgs(const PresetsDetails::TestPreset &preset,
                              const QVersionNumber &ctestVersion);

} // namespace CMakeProjectManager::Internal
