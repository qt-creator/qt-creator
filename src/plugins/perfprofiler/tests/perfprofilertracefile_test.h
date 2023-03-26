// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace PerfProfiler {
namespace Internal {

class PerfProfilerTraceFileTest : public QObject
{
    Q_OBJECT
public:
    explicit PerfProfilerTraceFileTest(QObject *parent = nullptr);

private slots:
    void testSaveLoadTraceData();
};

} // namespace Internal
} // namespace PerfProfiler
