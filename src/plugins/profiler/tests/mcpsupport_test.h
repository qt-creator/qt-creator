// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace Profiler::Internal {

class ProfilerMcpSupportTest : public QObject
{
    Q_OBJECT

private slots:
    void testReadOnlyHints_data();
    void testReadOnlyHints();
    void testRejectsInvalidRecordingOptions_data();
    void testRejectsInvalidRecordingOptions();
    void testAttachRecordSaveAndSummarize();
};

} // namespace Profiler::Internal
