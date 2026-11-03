// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cpuflamegraphview.h"

#include "cpuflamegraphroles.h"
#include "profilertr.h"

namespace Profiler::Internal {

/*!
    \class Profiler::Internal::CpuFlameGraphView
    \internal

    The flame graph of sampled call stacks, shared by the CPU profiler
    backends. It reads its \a model through the roles in CpuFlameGraph::Role.
    With Measures::SamplesAndResources it also offers to size the graph by the
    resources the functions used, for a backend that records them.
*/
CpuFlameGraphView::CpuFlameGraphView(QAbstractItemModel *model, Measures measures,
                                     QWidget *parent)
    : Timeline::FlameGraphWidget(model, parent)
{
    setObjectName("CpuFlameGraphView");
    setWindowTitle(Tr::tr("CPU Flame Graph"));

    using namespace CpuFlameGraph;
    setTypeIdRole(TypeIdRole);
    setSourceRoles(SourceFileRole, SourceLineRole);
    setSummaryRole(FunctionRole);
    setDetailsTitleRole(DisplayNameRole);

    QList<QPair<int, QString>> sizeRoles{{SamplesRole, Tr::tr("Samples")}};
    QList<QPair<int, QString>> detailsRoles{
        {FunctionRole, Tr::tr("Function")},
        {SamplesRole, Tr::tr("Samples")},
        {SelfRole, Tr::tr("Self")},
        {SourceFileRole, Tr::tr("Source")},
        {BinaryRole, Tr::tr("Binary")},
    };
    if (measures == Measures::SamplesAndResources) {
        const QList<QPair<int, QString>> resourceRoles{
            {ResourcePeakRole, Tr::tr("Peak Usage")},
            {ResourceAllocationsRole, Tr::tr("Allocations")},
            {ResourceReleasesRole, Tr::tr("Releases")},
        };
        sizeRoles += resourceRoles;
        detailsRoles += resourceRoles;
    }
    setSizeRoles(sizeRoles);
    setDetailsRoles(detailsRoles);
    setOthersText(Tr::tr("Various"));
}

} // namespace Profiler::Internal
