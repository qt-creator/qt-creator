// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmlprofiler/pixmapcachemodel.h>
#include <qmlprofiler/qmlprofilermodelmanager.h>
#include <QObject>

namespace QmlProfiler {
namespace Internal {

class PixmapCacheModelTest : public QObject
{
    Q_OBJECT
public:
    PixmapCacheModelTest(QObject *parent = nullptr);

private slots:
    void initTestCase();
    void testConsistency();
    void testRowMaxValue();
    void testColor();
    void testLabels();
    void cleanupTestCase();

private:
    QmlProfilerModelManager manager;
    Timeline::TimelineModelAggregator aggregator;
    PixmapCacheModel model;

    int eventTypeIndices[2 * MaximumPixmapEventType];
};

} // namespace Internal
} // namespace QmlProfiler
