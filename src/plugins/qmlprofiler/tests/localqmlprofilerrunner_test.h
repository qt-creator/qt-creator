// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmlprofiler/qmlprofilermodelmanager.h>

namespace QmlProfiler {
namespace Internal {

class LocalQmlProfilerRunnerTest : public QObject
{
    Q_OBJECT

public:
    LocalQmlProfilerRunnerTest(QObject *parent = nullptr);

private slots:
    void testRunner();
    void testFindFreePort();
    void testFindFreeSocket();
};

} // namespace Internal
} // namespace QmlProfiler
