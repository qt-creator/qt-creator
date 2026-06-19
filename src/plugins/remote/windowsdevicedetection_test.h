// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace Remote::Internal {

// Integration test for the remote Windows device: connects to a Windows-over-SSH host,
// runs the MSVC toolchain auto-detection and verifies that a single, complete kit (with a
// resolved compiler on the device) is created. Requires the QTC_SSH_TEST_* environment to
// point at a reachable Windows machine; skipped otherwise.
class WindowsDeviceDetectionTest : public QObject
{
    Q_OBJECT

private slots:
    void testDetectToolchainsAndCreateKit();
};

} // namespace Remote::Internal
