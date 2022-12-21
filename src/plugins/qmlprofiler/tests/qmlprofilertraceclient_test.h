// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmlprofiler/qmlprofilermodelmanager.h>
#include <qmlprofiler/qmlprofilertraceclient.h>

#include <QObject>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerTraceClientTest : public QObject
{
    Q_OBJECT
public:
    QmlProfilerTraceClient traceClient;
    explicit QmlProfilerTraceClientTest(QObject *parent = nullptr);

private slots:
    void testMessageReceived();

private:
    QmlProfilerModelManager modelManager;
};

} // namespace Internal
} // namespace QmlProfiler
