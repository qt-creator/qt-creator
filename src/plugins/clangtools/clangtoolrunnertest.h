// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace ClangTools::Internal {

class ClangToolRunnerTest : public QObject
{
    Q_OBJECT

private slots:
    void testRun();
    void testRun_data();
    void testWithoutOutputHandler();
    void testChecksSource();
    void testChecksSource_data();
};

} // namespace ClangTools::Internal
