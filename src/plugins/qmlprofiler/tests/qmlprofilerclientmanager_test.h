// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <qmlprofiler/qmlprofilerclientmanager.h>
#include <qmlprofiler/qmlprofilermodelmanager.h>
#include <qmlprofiler/qmlprofilerstatemanager.h>

#include <QObject>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerClientManagerTest : public QObject
{
    Q_OBJECT
public:
    explicit QmlProfilerClientManagerTest(QObject *parent = nullptr);

private slots:
    void testConnectionFailure_data();
    void testConnectionFailure();

    void testUnresponsiveTcp();
    void testUnresponsiveLocal();

    void testResponsiveTcp_data();
    void testResponsiveTcp();

    void testResponsiveLocal_data();
    void testResponsiveLocal();

    void testInvalidData();
    void testStopRecording();
    void testConnectionDrop();

private:
    QmlProfilerClientManager clientManager;
    QmlProfilerModelManager modelManager;
    QmlProfilerStateManager stateManager;

};

} // namespace Internal
} // namespace QmlProfiler
