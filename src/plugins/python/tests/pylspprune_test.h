// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace Python::Internal {

class PylspPruneTest final : public QObject
{
    Q_OBJECT

private slots:
    void testRemovesInstallationsWithoutInterpreter();
    void testRemovesEverythingWithoutVersions();
    void testIgnoresAMissingRoot();
    void testReportsNoVersionForAnUnreadableInterpreter();
    void testReportsTheVersionOfAReadableInterpreter();
};

QObject *createPylspPruneTest();

} // namespace Python::Internal
