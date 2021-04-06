/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
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

#include "valgrindconfigwidget.h"
#include "valgrindsettings.h"
#include "valgrindplugin.h"

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
    Q_DECLARE_TR_FUNCTIONS(Valgrind::Internal::ValgrindConfigWidget)

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
    const Break nl;
    ValgrindBaseSettings &s = *settings;

    Grid generic {
        s.valgrindExecutable, nl,
        s.valgrindArguments, nl,
        s.selfModifyingCodeDetection, nl
    };

    Grid memcheck {
        s.memcheckArguments, nl,
        s.trackOrigins, nl,
        s.showReachable, nl,
        s.leakCheckOnFinish, nl,
        s.numCallers, nl,
        s.filterExternalIssues, nl,
        s.suppressions
    };

    Grid callgrind {
        s.callgrindArguments, nl,
        s.kcachegrindExecutable, nl,
        s.minimumInclusiveCostRatio, nl,
        s.visualizationMinimumInclusiveCostRatio, nl,
        s.enableEventToolTips, nl,
        Span {
            2,
            Group {
                s.enableCacheSim,
                s.enableBranchSim,
                s.collectSystime,
                s.collectBusEvents,
            }
        }
    };

    Column {
        Group { Title(tr("Valgrind Generic Settings")), generic },
        Group { Title(tr("MemCheck Memory Analysis Options")), memcheck },
        Group { Title(tr("CallGrind Profiling Options")), callgrind },
        Stretch(),
    }.attachTo(this);
}

// ValgrindOptionsPage

ValgrindOptionsPage::ValgrindOptionsPage()
{
    setId(ANALYZER_VALGRIND_SETTINGS);
    setDisplayName(ValgrindConfigWidget::tr("Valgrind"));
    setCategory("T.Analyzer");
    setDisplayCategory(QCoreApplication::translate("Analyzer", "Analyzer"));
    setCategoryIconPath(Analyzer::Icons::SETTINGSCATEGORY_ANALYZER);
    setWidgetCreator([] { return new ValgrindConfigWidget(ValgrindGlobalSettings::instance()); });
}

QWidget *ValgrindOptionsPage::createSettingsWidget(ValgrindBaseSettings *settings)
{
    return new ValgrindConfigWidget(settings);
}

} // namespace Internal
} // namespace Valgrind
