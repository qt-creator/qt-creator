// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace Remote::Internal {

// The translations the Flatpak host device is built on, checked without a
// sandbox: they are pure, and flatpak-spawn is stood in for by a script.
class FlatpakApiTest : public QObject
{
    Q_OBJECT

private slots:
    void testHostPathToSandboxPath_data();
    void testHostPathToSandboxPath();
    void testSandboxPathToHostPath_data();
    void testSandboxPathToHostPath();
    void testRoundTrip_data();
    void testRoundTrip();
    void testHostCommandLine_data();
    void testHostCommandLine();
    void testHostCommandLineRuns();
    void testDetectHostEnvironment();
    void testHandlesFile();
    void testLocalSource();
};

} // namespace Remote::Internal
