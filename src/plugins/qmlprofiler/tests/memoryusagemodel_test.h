/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <qmlprofiler/memoryusagemodel.h>
#include <qmlprofiler/qmlprofilermodelmanager.h>
#include <QObject>

namespace QmlProfiler {
namespace Internal {

class MemoryUsageModelTest : public QObject
{
    Q_OBJECT
public:
    MemoryUsageModelTest(QObject *parent = 0);

private slots:
    void initTestCase();
    void testRowMaxValue();
    void testTypeId();
    void testColor();
    void testLabels();
    void testDetails();
    void testExpandedRow();
    void testCollapsedRow();
    void testLocation();
    void testRelativeHeight();
    void cleanupTestCase();

private:
    QmlProfilerModelManager manager;
    Timeline::TimelineModelAggregator aggregator;
    MemoryUsageModel model;

    int heapPageTypeId = -1;
    int smallItemTypeId = -1;
    int largeItemTypeId = -1;
    int rangeTypeId = -1;
};

} // namespace Internal
} // namespace QmlProfiler
