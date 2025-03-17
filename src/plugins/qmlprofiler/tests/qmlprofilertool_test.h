// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace QmlProfiler::Internal {

class QmlProfilerToolTest : public QObject
{
    Q_OBJECT

private slots:
    void testAttachToWaitingApplication();
    void testClearEvents();
};

} // namespace QmlProfiler::Internal
