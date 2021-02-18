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

#include "valgrindsettings.h"
#include "valgrindplugin.h"
#include "valgrindconfigwidget.h"

#include <coreplugin/icore.h>

#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <valgrind/xmlprotocol/error.h>

#include <QSettings>
#include <QDebug>

using namespace Utils;

namespace Valgrind {
namespace Internal {

//////////////////////////////////////////////////////////////////
//
// ValgrindBaseSettings
//
//////////////////////////////////////////////////////////////////

ValgrindBaseSettings::ValgrindBaseSettings()
{
    // Note that this is used twice, once for project settings in the .user files
    // and once for global settings in QtCreator.ini. This uses intentionally
    // the same key to facilitate copying using fromMap/toMap.
    QString base = "Analyzer.Valgrind.";

    group.registerAspect(&valgrindExecutable);
    valgrindExecutable.setSettingsKey(base + "ValgrindExecutable");
    valgrindExecutable.setDefaultValue("valgrind");
    valgrindExecutable.setDisplayStyle(StringAspect::PathChooserDisplay);
    valgrindExecutable.setExpectedKind(PathChooser::Command);
    valgrindExecutable.setHistoryCompleter("Valgrind.Command.History");
    valgrindExecutable.setDisplayName(tr("Valgrind Command"));
    valgrindExecutable.setLabelText(tr("Valgrind executable:"));
    if (Utils::HostOsInfo::isWindowsHost()) {
        // On Window we know that we don't have a local valgrind
        // executable, so having the "Browse" button in the path chooser
        // (which is needed for the remote executable) is confusing.
        // FIXME: not deadly, still...
        //valgrindExecutable. ... buttonAtIndex(0)->hide();
    }

    group.registerAspect(&valgrindArguments);
    valgrindArguments.setSettingsKey(base + "ValgrindArguments");
    valgrindArguments.setDisplayStyle(StringAspect::LineEditDisplay);
    valgrindArguments.setLabelText(tr("Valgrind arguments:"));

    group.registerAspect(&selfModifyingCodeDetection);
    selfModifyingCodeDetection.setSettingsKey(base + "SelfModifyingCodeDetection");
    selfModifyingCodeDetection.setDefaultValue(DetectSmcStackOnly);
    selfModifyingCodeDetection.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    selfModifyingCodeDetection.addOption("No");
    selfModifyingCodeDetection.addOption("Only on Stack");
    selfModifyingCodeDetection.addOption("Everywhere");
    selfModifyingCodeDetection.addOption("Everywhere Except in File-backend Mappings");
    selfModifyingCodeDetection.setLabelText(tr("Detect self-modifying code:"));

    // Memcheck
    group.registerAspect(&memcheckArguments);
    memcheckArguments.setSettingsKey(base + "Memcheck.Arguments");
    memcheckArguments.setDisplayStyle(StringAspect::LineEditDisplay);
    memcheckArguments.setLabelText(tr("Extra MemCheck arguments:"));

    group.registerAspect(&filterExternalIssues);
    filterExternalIssues.setSettingsKey(base + "FilterExternalIssues");
    filterExternalIssues.setDefaultValue(true);
    filterExternalIssues.setIcon(Icons::FILTER.icon());
    filterExternalIssues.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBoxWithoutDummyLabel);
    filterExternalIssues.setLabelText(tr("Show Project Costs Only"));
    filterExternalIssues.setToolTip(tr("Show only profiling info that originated from this project source."));

    group.registerAspect(&trackOrigins);
    trackOrigins.setSettingsKey(base + "TrackOrigins");
    trackOrigins.setDefaultValue(true);
    trackOrigins.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBoxWithoutDummyLabel);
    trackOrigins.setLabelText(tr("Track origins of uninitialized memory"));

    group.registerAspect(&showReachable);
    showReachable.setSettingsKey(base + "ShowReachable");
    showReachable.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBoxWithoutDummyLabel);
    showReachable.setLabelText(tr("Show reachable and indirectly lost blocks"));

    group.registerAspect(&leakCheckOnFinish);
    leakCheckOnFinish.setSettingsKey(base + "LeakCheckOnFinish");
    leakCheckOnFinish.setDefaultValue(LeakCheckOnFinishSummaryOnly);
    leakCheckOnFinish.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    leakCheckOnFinish.addOption(tr("No"));
    leakCheckOnFinish.addOption(tr("Summary Only"));
    leakCheckOnFinish.addOption(tr("Full"));
    leakCheckOnFinish.setLabelText(tr("Check for leaks on finish:"));

    group.registerAspect(&numCallers);
    numCallers.setSettingsKey(base + "NumCallers");
    numCallers.setDefaultValue(25);
    numCallers.setLabelText(tr("Backtrace frame count:"));

    // Callgrind

    group.registerAspect(&kcachegrindExecutable);
    kcachegrindExecutable.setSettingsKey(base + "KCachegrindExecutable");
    kcachegrindExecutable.setDefaultValue("kcachegrind");
    kcachegrindExecutable.setDisplayStyle(StringAspect::PathChooserDisplay);
    kcachegrindExecutable.setLabelText(tr("KCachegrind executable:"));
    kcachegrindExecutable.setExpectedKind(Utils::PathChooser::Command);
    kcachegrindExecutable.setDisplayName(tr("KCachegrind Command"));

    group.registerAspect(&callgrindArguments);
    callgrindArguments.setSettingsKey(base + "Callgrind.Arguments");
    callgrindArguments.setDisplayStyle(StringAspect::LineEditDisplay);
    callgrindArguments.setLabelText(tr("Extra CallGrind arguments:"));

    group.registerAspect(&enableEventToolTips);
    enableEventToolTips.setDefaultValue(true);
    enableEventToolTips.setSettingsKey(base + "Callgrind.EnableEventToolTips");
    enableEventToolTips.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBoxWithoutDummyLabel);
    enableEventToolTips.setLabelText(tr("Show additional information for events in tooltips"));

    group.registerAspect(&enableCacheSim);
    enableCacheSim.setSettingsKey(base + "Callgrind.EnableCacheSim");
    enableCacheSim.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBoxWithoutDummyLabel);
    enableCacheSim.setLabelText(tr("Enable cache simulation"));
    enableCacheSim.setToolTip("<html><head/><body>" + tr(
        "<p>Does full cache simulation.</p>\n"
        "<p>By default, only instruction read accesses will be counted (\"Ir\").</p>\n"
        "<p>\n"
        "With cache simulation, further event counters are enabled:\n"
        "<ul><li>Cache misses on instruction reads (\"I1mr\"/\"I2mr\").</li>\n"
        "<li>Data read accesses (\"Dr\") and related cache misses (\"D1mr\"/\"D2mr\").</li>\n"
        "<li>Data write accesses (\"Dw\") and related cache misses (\"D1mw\"/\"D2mw\").</li></ul>\n"
        "</p>") + "</body></html>");

    group.registerAspect(&enableBranchSim);
    enableBranchSim.setSettingsKey(base + "Callgrind.EnableBranchSim");
    enableBranchSim.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBoxWithoutDummyLabel);
    enableBranchSim.setLabelText(tr("Enable branch prediction simulation"));
    enableBranchSim.setToolTip("<html><head/><body>\n" + tr(
        "<p>Does branch prediction simulation.</p>\n"
        "<p>Further event counters are enabled: </p>\n"
        "<ul><li>Number of executed conditional branches and related predictor misses (\n"
        "\"Bc\"/\"Bcm\").</li>\n"
        "<li>Executed indirect jumps and related misses of the jump address predictor (\n"
        "\"Bi\"/\"Bim\").)</li></ul>") + "</body></html>");

    group.registerAspect(&collectSystime);
    collectSystime.setSettingsKey(base + "Callgrind.CollectSystime");
    collectSystime.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBoxWithoutDummyLabel);
    collectSystime.setLabelText(tr("Collect system call time"));
    collectSystime.setToolTip(tr("Collects information for system call times."));

    group.registerAspect(&collectBusEvents);
    collectBusEvents.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBoxWithoutDummyLabel);
    collectBusEvents.setSettingsKey(base + "Callgrind.CollectBusEvents");
    collectBusEvents.setLabelText(tr("Collect global bus events"));
    collectBusEvents.setToolTip(tr("Collect the number of global bus events that are executed. "
        "The event type \"Ge\" is used for these events."));

    group.registerAspect(&minimumInclusiveCostRatio);
    minimumInclusiveCostRatio.setSettingsKey(base + "Callgrind.MinimumCostRatio");
    minimumInclusiveCostRatio.setDefaultValue(0.01);
    minimumInclusiveCostRatio.setSuffix(tr("%"));
    minimumInclusiveCostRatio.setLabelText(tr("Result view: Minimum event cost:"));
    minimumInclusiveCostRatio.setToolTip(tr("Limits the amount of results the profiler gives you. "
         "A lower limit will likely increase performance."));

    group.registerAspect(&visualizationMinimumInclusiveCostRatio);
    visualizationMinimumInclusiveCostRatio.setSettingsKey(base + "Callgrind.VisualisationMinimumCostRatio");
    visualizationMinimumInclusiveCostRatio.setDefaultValue(10.0);
    visualizationMinimumInclusiveCostRatio.setLabelText(tr("Visualization: Minimum event cost:"));
    visualizationMinimumInclusiveCostRatio.setSuffix(tr("%"));

    group.registerAspect(&visibleErrorKinds);
    visibleErrorKinds.setSettingsKey(base + "VisibleErrorKinds");
    QList<int> defaultErrorKinds;
    for (int i = 0; i < Valgrind::XmlProtocol::MemcheckErrorKindCount; ++i)
        defaultErrorKinds << i;
    visibleErrorKinds.setDefaultValue(defaultErrorKinds);
}

void ValgrindBaseSettings::fromMap(const QVariantMap &map)
{
    group.fromMap(map);
    if (ValgrindGlobalSettings::instance() != this) {
        // FIXME: Update project page e.g. on "Restore Global", aspects
        // there are 'autoapply', and Aspect::cancel() is normally part of
        // the 'manual apply' machinery.
        group.setAutoApply(false);
        group.cancel();
        group.setAutoApply(true);
    }
}

void ValgrindBaseSettings::toMap(QVariantMap &map) const
{
    group.toMap(map);
}


//////////////////////////////////////////////////////////////////
//
// ValgrindGlobalSettings
//
//////////////////////////////////////////////////////////////////

static ValgrindGlobalSettings *theGlobalSettings = nullptr;

ValgrindGlobalSettings::ValgrindGlobalSettings()
{
    theGlobalSettings = this;

    const QString base = "Analyzer.Valgrind";

    group.registerAspect(&suppressionFiles_);
    suppressionFiles_.setSettingsKey(base + "SupressionFiles");

    group.registerAspect(&lastSuppressionDirectory);
    lastSuppressionDirectory.setSettingsKey(base + "LastSuppressionDirectory");

    group.registerAspect(&lastSuppressionHistory);
    lastSuppressionHistory.setSettingsKey(base + "LastSuppressionHistory");

    group.registerAspect(&detectCycles);
    detectCycles.setSettingsKey(base + "Callgrind.CycleDetection");
    detectCycles.setDefaultValue(true);
    detectCycles.setLabelText("O"); // FIXME: Create a real icon
    detectCycles.setToolTip(tr("Enable cycle detection to properly handle recursive "
        "or circular function calls."));

    group.registerAspect(&costFormat);
    costFormat.setSettingsKey(base + "Callgrind.CostFormat");
    costFormat.setDefaultValue(CostDelegate::FormatRelative);
    costFormat.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);

    group.registerAspect(&shortenTemplates);
    shortenTemplates.setSettingsKey(base + "Callgrind.ShortenTemplates");
    shortenTemplates.setDefaultValue(true);
    shortenTemplates.setLabelText("<>"); // FIXME: Create a real icon
    shortenTemplates.setToolTip(tr("Remove template parameter lists when displaying function names."));

    setConfigWidgetCreator([this] { return ValgrindOptionsPage::createSettingsWidget(this); });
    readSettings();

    group.forEachAspect([](BaseAspect *aspect) { aspect->setAutoApply(false); });
}

ValgrindGlobalSettings *ValgrindGlobalSettings::instance()
{
    return theGlobalSettings;
}

//
// Memcheck
//
QStringList ValgrindGlobalSettings::suppressionFiles() const
{
    return suppressionFiles_.value();
}

void ValgrindGlobalSettings::addSuppressionFiles(const QStringList &suppressions)
{
    suppressionFiles_.appendValues(suppressions);
}

void ValgrindGlobalSettings::removeSuppressionFiles(const QStringList &suppressions)
{
    suppressionFiles_.removeValues(suppressions);
}

QVariantMap ValgrindBaseSettings::defaultSettings() const
{
    QVariantMap defaults;
    group.forEachAspect([&defaults](BaseAspect *aspect) {
        defaults.insert(aspect->settingsKey(), aspect->defaultValue());
    });
    return defaults;
}

static const char groupC[] = "Analyzer";

void ValgrindGlobalSettings::readSettings()
{
    QVariantMap defaults = defaultSettings();

    // Read stored values
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(groupC);
    QVariantMap map = defaults;
    for (QVariantMap::ConstIterator it = defaults.constBegin(); it != defaults.constEnd(); ++it)
        map.insert(it.key(), settings->value(it.key(), it.value()));
    settings->endGroup();

    fromMap(map);
}

void ValgrindGlobalSettings::writeSettings() const
{
    const QVariantMap defaults = defaultSettings();

    Utils::QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup(groupC);
    QVariantMap map;
    toMap(map);
    for (QVariantMap::ConstIterator it = map.constBegin(); it != map.constEnd(); ++it)
        settings->setValueWithDefault(it.key(), it.value(), defaults.value(it.key()));
    settings->endGroup();
}

//////////////////////////////////////////////////////////////////
//
// ValgrindProjectSettings
//
//////////////////////////////////////////////////////////////////

ValgrindProjectSettings::ValgrindProjectSettings()
{
    setConfigWidgetCreator([this] { return ValgrindOptionsPage::createSettingsWidget(this); });

    group.registerAspect(&disabledGlobalSuppressionFiles);
    disabledGlobalSuppressionFiles.setSettingsKey("Analyzer.Valgrind.RemovedSuppressionFiles");

    group.registerAspect(&addedSuppressionFiles);
    addedSuppressionFiles.setSettingsKey("Analyzer.Valgrind.AddedSuppressionFiles");
}

//
// Memcheck
//

void ValgrindProjectSettings::addSuppressionFiles(const QStringList &suppressions)
{
    const QStringList globalSuppressions = ValgrindGlobalSettings::instance()->suppressionFiles();
    for (const QString &s : suppressions) {
        if (addedSuppressionFiles.value().contains(s))
            continue;
        disabledGlobalSuppressionFiles.removeValue(s);
        if (!globalSuppressions.contains(s))
            addedSuppressionFiles.appendValue(s);
    }
}

void ValgrindProjectSettings::removeSuppressionFiles(const QStringList &suppressions)
{
    const QStringList globalSuppressions = ValgrindGlobalSettings::instance()->suppressionFiles();
    for (const QString &s : suppressions) {
        addedSuppressionFiles.removeValue(s);
        if (globalSuppressions.contains(s))
            disabledGlobalSuppressionFiles.appendValue(s);
    }
}

QStringList ValgrindProjectSettings::suppressionFiles() const
{
    QStringList ret = ValgrindGlobalSettings::instance()->suppressionFiles();
    for (const QString &s : disabledGlobalSuppressionFiles.value())
        ret.removeAll(s);
    ret.append(addedSuppressionFiles.value());
    return ret;
}

} // namespace Internal
} // namespace Valgrind
