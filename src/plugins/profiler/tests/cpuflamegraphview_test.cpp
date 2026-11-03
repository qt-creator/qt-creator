// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cpuflamegraphview_test.h"

#include <profiler/cpuflamegraphroles.h>
#include <profiler/cpuflamegraphview.h>
#include <profiler/perfprofilerflamegraphmodel.h>
#include <profiler/perfprofilertracemanager.h>

#include <QComboBox>
#include <QStandardItemModel>
#include <QTest>

namespace Profiler::Internal {

static QStringList measures(const CpuFlameGraphView &view)
{
    const auto combo = view.findChild<QComboBox *>();
    if (!combo)
        return {};
    QStringList result;
    for (int i = 0; i < combo->count(); ++i)
        result.append(combo->itemText(i));
    return result;
}

void CpuFlameGraphViewTest::testMeasures()
{
    QStandardItemModel model;
    const CpuFlameGraphView samples(&model, CpuFlameGraphView::Measures::Samples);
    QCOMPARE(samples.objectName(), QString("CpuFlameGraphView"));
    QCOMPARE(samples.windowTitle(), QString("CPU Flame Graph"));
    QCOMPARE(measures(samples), QStringList({"Samples"}));

    const CpuFlameGraphView resources(&model, CpuFlameGraphView::Measures::SamplesAndResources);
    QCOMPARE(measures(resources),
             QStringList({"Samples", "Peak Usage", "Allocations", "Releases"}));
}

void CpuFlameGraphViewTest::testPerfModelAnswersSharedRoles()
{
    // The root of an empty perf flame graph answers the roles the view reads,
    // with no samples and no resources yet.
    PerfProfilerTraceManager traceManager;
    PerfProfilerFlameGraphModel model(&traceManager);
    using namespace CpuFlameGraph;
    for (const int role : {TypeIdRole, SamplesRole, ResourcePeakRole, ResourceAllocationsRole,
                           ResourceReleasesRole}) {
        QVERIFY2(model.data({}, role).isValid(), qPrintable(QString::number(role)));
    }
    QCOMPARE(model.data({}, TypeIdRole).toInt(), -1);
    QCOMPARE(model.data({}, SamplesRole).toUInt(), 0u);
}

} // namespace Profiler::Internal
