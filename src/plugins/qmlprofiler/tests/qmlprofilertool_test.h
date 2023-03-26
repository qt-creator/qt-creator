// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <qmlprofiler/qmlprofilertool.h>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerToolTest : public QObject
{
    Q_OBJECT
private slots:
    void testAttachToWaitingApplication();
    void testClearEvents();
};

} // namespace Internal
} // namespace QmlProfiler
