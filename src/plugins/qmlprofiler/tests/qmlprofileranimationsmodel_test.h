// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmlprofiler/qmlprofileranimationsmodel.h>
#include <qmlprofiler/qmlprofilermodelmanager.h>
#include <QObject>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerAnimationsModelTest : public QObject
{
    Q_OBJECT
public:
    explicit QmlProfilerAnimationsModelTest(QObject *parent = nullptr);

private slots:
    void initTestCase();
    void testRowMaxValue();
    void testRowNumbers();
    void testTypeId();
    void testColor();
    void testRelativeHeight();
    void testLabels();
    void testDetails();
    void cleanupTestCase();

private:
    QmlProfilerModelManager manager;
    Timeline::TimelineModelAggregator aggregator;
    QmlProfilerAnimationsModel model;
};

} // namespace Internal
} // namespace QmlProfiler
