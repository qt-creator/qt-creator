// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

#include <optional>

namespace Core::Internal {

QObject *createSecretAspectTest();

// Overrides whether a keychain is available, std::nullopt restores the probe.
void setKeychainAvailableForTest(std::optional<bool> available);

} // namespace Core::Internal
