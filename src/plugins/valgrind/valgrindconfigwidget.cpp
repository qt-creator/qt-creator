// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "valgrindconfigwidget.h"
#include "valgrindsettings.h"
#include "valgrindtr.h"

#include <debugger/analyzer/analyzericons.h>

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

using namespace Utils;

namespace Valgrind {
namespace Internal {

class ValgrindConfigWidget : public Core::IOptionsPageWidget
{
public:
    explicit ValgrindConfigWidget(ValgrindBaseSettings *settings);

    void apply() final
    {
        ValgrindGlobalSettings::instance()->apply();
        ValgrindGlobalSettings::instance()->writeSettings();
    }

    void finish() final
    {
        ValgrindGlobalSettings::instance()->finish();
    }
};

ValgrindConfigWidget::ValgrindConfigWidget(ValgrindBaseSettings *settings)
{
    using namespace Layouting;
    ValgrindBaseSettings &s = *settings;

    Grid generic {
        s.valgrindExecutable, br,
        s.valgrindArguments, br,
        s.selfModifyingCodeDetection, br
    };

    Grid memcheck {
        s.memcheckArguments, br,
        s.trackOrigins, br,
        s.showReachable, br,
        s.leakCheckOnFinish, br,
        s.numCallers, br,
        s.filterExternalIssues, br,
        s.suppressions
    };

    Grid callgrind {
        s.callgrindArguments, br,
        s.kcachegrindExecutable, br,
        s.minimumInclusiveCostRatio, br,
        s.visualizationMinimumInclusiveCostRatio, br,
        s.enableEventToolTips, br,
        Span {
            2,
            Group {
                Column {
                    s.enableCacheSim,
                    s.enableBranchSim,
                    s.collectSystime,
                    s.collectBusEvents,
                }
            }
        }
    };

    Column {
        Group { title(Tr::tr("Valgrind Generic Settings")), generic },
        Group { title(Tr::tr("MemCheck Memory Analysis Options")), memcheck },
        Group { title(Tr::tr("CallGrind Profiling Options")), callgrind },
        st,
    }.attachTo(this);
}

// ValgrindOptionsPage

ValgrindOptionsPage::ValgrindOptionsPage()
{
    setId(ANALYZER_VALGRIND_SETTINGS);
    setDisplayName(Tr::tr("Valgrind"));
    setCategory("T.Analyzer");
    setDisplayCategory(Tr::tr("Analyzer"));
    setCategoryIconPath(Analyzer::Icons::SETTINGSCATEGORY_ANALYZER);
    setWidgetCreator([] { return new ValgrindConfigWidget(ValgrindGlobalSettings::instance()); });
}

QWidget *ValgrindOptionsPage::createSettingsWidget(ValgrindBaseSettings *settings)
{
    return new ValgrindConfigWidget(settings);
}

} // namespace Internal
} // namespace Valgrind
