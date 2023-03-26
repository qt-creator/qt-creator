// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfconfigwidget.h"
#include "perfoptionspage.h"
#include "perfprofilerconstants.h"
#include "perfprofilertr.h"

#include <debugger/analyzer/analyzericons.h>
#include <debugger/debuggertr.h>

namespace PerfProfiler {
namespace Internal {

PerfOptionsPage::PerfOptionsPage(PerfSettings *settings)
{
    setId(Constants::PerfSettingsId);
    setDisplayName(Tr::tr("CPU Usage"));
    setCategory("T.Analyzer");
    setDisplayCategory(::Debugger::Tr::tr("Analyzer"));
    setCategoryIconPath(Analyzer::Icons::SETTINGSCATEGORY_ANALYZER);
    setWidgetCreator([settings] { return new PerfConfigWidget(settings); });
}

} // namespace Internal
} // namespace PerfProfiler
