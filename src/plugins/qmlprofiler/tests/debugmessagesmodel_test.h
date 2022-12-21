// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmlprofiler/debugmessagesmodel.h>
#include <qmlprofiler/qmlprofilermodelmanager.h>
#include <QObject>

namespace QmlProfiler {
namespace Internal {

class DebugMessagesModelTest : public QObject
{
    Q_OBJECT
public:
    DebugMessagesModelTest(QObject *parent = nullptr);

private slots:
    void initTestCase();
    void testTypeId();
    void testColor();
    void testLabels();
    void testDetails();
    void testExpandedRow();
    void testCollapsedRow();
    void testLocation();
    void cleanupTestCase();

private:
    QmlProfilerModelManager manager;
    Timeline::TimelineModelAggregator aggregator;
    DebugMessagesModel model;
};

} // namespace Internal
} // namespace QmlProfiler
