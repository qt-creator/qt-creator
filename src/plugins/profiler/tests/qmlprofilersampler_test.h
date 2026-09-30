// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace Profiler::Internal {

class QmlProfilerSamplerTest : public QObject
{
    Q_OBJECT

private slots:
    void testPauseResumeStop();
    void testStartPausedThenStop();
    void testPauseResumeStopBeforeTheServerAnswers();
};

} // namespace Profiler::Internal
