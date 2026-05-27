// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfprofilerflamegraphmodel.h"
#include "perfprofilerflamegraphview.h"
#include "perfprofilertr.h"

namespace PerfProfiler::Internal {

PerfProfilerFlameGraphView::PerfProfilerFlameGraphView(QWidget *parent)
    : Timeline::FlameGraphWidget(
          new PerfProfilerFlameGraphModel(&traceManager()),
          parent)
{
    setObjectName(QLatin1String("PerfProfilerFlameGraphView"));

    model()->setParent(this);

    using Role = PerfProfilerFlameGraphModel;
    setTypeIdRole(Role::TypeIdRole);
    setSourceRoles(Role::SourceFileRole, Role::LineRole);
    setSummaryRole(Role::FunctionRole);
    setDetailsTitleRole(Role::DisplayNameRole);
    setSizeRoles({
        {Role::SamplesRole,              Tr::tr("Samples")},
        {Role::ResourcePeakRole,         Tr::tr("Peak Usage")},
        {Role::ResourceAllocationsRole,  Tr::tr("Allocations")},
        {Role::ResourceReleasesRole,     Tr::tr("Releases")},
    });
    setDetailsRoles({
        {Role::FunctionRole,            Tr::tr("Function")},
        {Role::SamplesRole,             Tr::tr("Samples")},
        {Role::SourceFileRole,          Tr::tr("Source")},
        {Role::ElfFileRole,             Tr::tr("Binary")},
        {Role::ResourcePeakRole,        Tr::tr("Peak Usage")},
        {Role::ResourceAllocationsRole, Tr::tr("Allocations")},
        {Role::ResourceReleasesRole,    Tr::tr("Releases")},
    });
    setOthersText(Tr::tr("Various"));
}

} // namespace PerfProfiler::Internal
