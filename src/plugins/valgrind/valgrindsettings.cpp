// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "valgrindsettings.h"

#include "callgrindcostdelegate.h"
#include "valgrindconfigwidget.h"
#include "valgrindtr.h"

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>
#include <utils/treemodel.h>
#include <utils/utilsicons.h>

#include <valgrind/xmlprotocol/error.h>

#include <QDebug>
#include <QListView>
#include <QPushButton>
#include <QSettings>
#include <QStandardItemModel>

using namespace Utils;

namespace Valgrind::Internal {

//
// SuppressionAspect
//

class SuppressionAspectPrivate : public QObject
{
public:
    SuppressionAspectPrivate(SuppressionAspect *q, bool global) : q(q), isGlobal(global) {}

    void slotAddSuppression();
    void slotRemoveSuppression();
    void slotSuppressionSelectionChanged();

    SuppressionAspect *q;
    const bool isGlobal;

    QPointer<QPushButton> addEntry;
    QPointer<QPushButton> removeEntry;
    QPointer<QListView> entryList;

    QStandardItemModel m_model; // The volatile value of this aspect.
};

void SuppressionAspect::addSuppressionFile(const FilePath &suppression)
{
    FilePaths val = value();
    val.append(suppression);
    setValue(val);
}

void SuppressionAspectPrivate::slotAddSuppression()
{
    ValgrindGlobalSettings *conf = ValgrindGlobalSettings::instance();
    QTC_ASSERT(conf, return);
    const FilePaths files =
            FileUtils::getOpenFilePaths(nullptr,
                      Tr::tr("Valgrind Suppression Files"),
                      conf->lastSuppressionDirectory.filePath(),
                      Tr::tr("Valgrind Suppression File (*.supp);;All Files (*)"));
    //dialog.setHistory(conf->lastSuppressionDialogHistory());
    if (!files.isEmpty()) {
        for (const FilePath &file : files)
            m_model.appendRow(new QStandardItem(file.toString()));
        conf->lastSuppressionDirectory.setFilePath(files.at(0).absolutePath());
        //conf->setLastSuppressionDialogHistory(dialog.history());
        if (!isGlobal)
            q->apply();
    }
}

void SuppressionAspectPrivate::slotRemoveSuppression()
{
    // remove from end so no rows get invalidated
    QList<int> rows;

    QStringList removed;
    const QModelIndexList selected = entryList->selectionModel()->selectedIndexes();
    for (const QModelIndex &index : selected) {
        rows << index.row();
        removed << index.data().toString();
    }

    Utils::sort(rows, std::greater<int>());

    for (int row : std::as_const(rows))
        m_model.removeRow(row);

    if (!isGlobal)
        q->apply();
}

void SuppressionAspectPrivate::slotSuppressionSelectionChanged()
{
    removeEntry->setEnabled(entryList->selectionModel()->hasSelection());
}

//
// SuppressionAspect
//

SuppressionAspect::SuppressionAspect(bool global)
{
    d = new SuppressionAspectPrivate(this, global);
    setSettingsKey("Analyzer.Valgrind.SuppressionFiles");
}

SuppressionAspect::~SuppressionAspect()
{
    delete d;
}

FilePaths SuppressionAspect::value() const
{
    return FileUtils::toFilePathList(BaseAspect::value().toStringList());
}

void SuppressionAspect::setValue(const FilePaths &val)
{
    BaseAspect::setValue(Utils::transform<QStringList>(val, &FilePath::toString));
}

void SuppressionAspect::addToLayout(LayoutBuilder &builder)
{
    QTC_CHECK(!d->addEntry);
    QTC_CHECK(!d->removeEntry);
    QTC_CHECK(!d->entryList);

    using namespace Layouting;

    d->addEntry = new QPushButton(Tr::tr("Add..."));
    d->removeEntry = new QPushButton(Tr::tr("Remove"));

    d->entryList = createSubWidget<QListView>();
    d->entryList->setModel(&d->m_model);
    d->entryList->setSelectionMode(QAbstractItemView::MultiSelection);

    connect(d->addEntry, &QPushButton::clicked,
            d, &SuppressionAspectPrivate::slotAddSuppression);
    connect(d->removeEntry, &QPushButton::clicked,
            d, &SuppressionAspectPrivate::slotRemoveSuppression);
    connect(d->entryList->selectionModel(), &QItemSelectionModel::selectionChanged,
            d, &SuppressionAspectPrivate::slotSuppressionSelectionChanged);

    builder.addItem(Column { Tr::tr("Suppression files:"), st });
    Row group {
        d->entryList.data(),
        Column { d->addEntry.data(), d->removeEntry.data(), st }
    };
    builder.addItem(Span { 2, group });

    setVolatileValue(BaseAspect::value());
}

void SuppressionAspect::fromMap(const QVariantMap &map)
{
    BaseAspect::fromMap(map);
}

void SuppressionAspect::toMap(QVariantMap &map) const
{
    BaseAspect::toMap(map);
}

QVariant SuppressionAspect::volatileValue() const
{
    QStringList ret;

    for (int i = 0; i < d->m_model.rowCount(); ++i)
        ret << d->m_model.item(i)->text();

    return ret;
}

void SuppressionAspect::setVolatileValue(const QVariant &val)
{
    const QStringList files = val.toStringList();
    d->m_model.clear();
    for (const QString &file : files)
        d->m_model.appendRow(new QStandardItem(file));
}

//////////////////////////////////////////////////////////////////
//
// ValgrindBaseSettings
//
//////////////////////////////////////////////////////////////////

ValgrindBaseSettings::ValgrindBaseSettings(bool global)
    : suppressions(global)
{
    // Note that this is used twice, once for project settings in the .user files
    // and once for global settings in QtCreator.ini. This uses intentionally
    // the same key to facilitate copying using fromMap/toMap.
    QString base = "Analyzer.Valgrind.";

    registerAspect(&suppressions);

    registerAspect(&valgrindExecutable);
    valgrindExecutable.setSettingsKey(base + "ValgrindExecutable");
    valgrindExecutable.setDefaultValue("valgrind");
    valgrindExecutable.setDisplayStyle(StringAspect::PathChooserDisplay);
    valgrindExecutable.setExpectedKind(PathChooser::Command);
    valgrindExecutable.setHistoryCompleter("Valgrind.Command.History");
    valgrindExecutable.setDisplayName(Tr::tr("Valgrind Command"));
    valgrindExecutable.setLabelText(Tr::tr("Valgrind executable:"));
    if (Utils::HostOsInfo::isWindowsHost()) {
        // On Window we know that we don't have a local valgrind
        // executable, so having the "Browse" button in the path chooser
        // (which is needed for the remote executable) is confusing.
        // FIXME: not deadly, still...
        //valgrindExecutable. ... buttonAtIndex(0)->hide();
    }

    registerAspect(&valgrindArguments);
    valgrindArguments.setSettingsKey(base + "ValgrindArguments");
    valgrindArguments.setDisplayStyle(StringAspect::LineEditDisplay);
    valgrindArguments.setLabelText(Tr::tr("Valgrind arguments:"));

    registerAspect(&selfModifyingCodeDetection);
    selfModifyingCodeDetection.setSettingsKey(base + "SelfModifyingCodeDetection");
    selfModifyingCodeDetection.setDefaultValue(DetectSmcStackOnly);
    selfModifyingCodeDetection.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    selfModifyingCodeDetection.addOption("No");
    selfModifyingCodeDetection.addOption("Only on Stack");
    selfModifyingCodeDetection.addOption("Everywhere");
    selfModifyingCodeDetection.addOption("Everywhere Except in File-backend Mappings");
    selfModifyingCodeDetection.setLabelText(Tr::tr("Detect self-modifying code:"));

    // Memcheck
    registerAspect(&memcheckArguments);
    memcheckArguments.setSettingsKey(base + "Memcheck.Arguments");
    memcheckArguments.setDisplayStyle(StringAspect::LineEditDisplay);
    memcheckArguments.setLabelText(Tr::tr("Extra MemCheck arguments:"));

    registerAspect(&filterExternalIssues);
    filterExternalIssues.setSettingsKey(base + "FilterExternalIssues");
    filterExternalIssues.setDefaultValue(true);
    filterExternalIssues.setIcon(Icons::FILTER.icon());
    filterExternalIssues.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBoxWithoutDummyLabel);
    filterExternalIssues.setLabelText(Tr::tr("Show Project Costs Only"));
    filterExternalIssues.setToolTip(Tr::tr("Show only profiling info that originated from this project source."));

    registerAspect(&trackOrigins);
    trackOrigins.setSettingsKey(base + "TrackOrigins");
    trackOrigins.setDefaultValue(true);
    trackOrigins.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBoxWithoutDummyLabel);
    trackOrigins.setLabelText(Tr::tr("Track origins of uninitialized memory"));

    registerAspect(&showReachable);
    showReachable.setSettingsKey(base + "ShowReachable");
    showReachable.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBoxWithoutDummyLabel);
    showReachable.setLabelText(Tr::tr("Show reachable and indirectly lost blocks"));

    registerAspect(&leakCheckOnFinish);
    leakCheckOnFinish.setSettingsKey(base + "LeakCheckOnFinish");
    leakCheckOnFinish.setDefaultValue(LeakCheckOnFinishSummaryOnly);
    leakCheckOnFinish.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    leakCheckOnFinish.addOption(Tr::tr("No"));
    leakCheckOnFinish.addOption(Tr::tr("Summary Only"));
    leakCheckOnFinish.addOption(Tr::tr("Full"));
    leakCheckOnFinish.setLabelText(Tr::tr("Check for leaks on finish:"));

    registerAspect(&numCallers);
    numCallers.setSettingsKey(base + "NumCallers");
    numCallers.setDefaultValue(25);
    numCallers.setLabelText(Tr::tr("Backtrace frame count:"));

    // Callgrind

    registerAspect(&kcachegrindExecutable);
    kcachegrindExecutable.setSettingsKey(base + "KCachegrindExecutable");
    kcachegrindExecutable.setDefaultValue("kcachegrind");
    kcachegrindExecutable.setDisplayStyle(StringAspect::PathChooserDisplay);
    kcachegrindExecutable.setLabelText(Tr::tr("KCachegrind executable:"));
    kcachegrindExecutable.setExpectedKind(Utils::PathChooser::Command);
    kcachegrindExecutable.setDisplayName(Tr::tr("KCachegrind Command"));

    registerAspect(&callgrindArguments);
    callgrindArguments.setSettingsKey(base + "Callgrind.Arguments");
    callgrindArguments.setDisplayStyle(StringAspect::LineEditDisplay);
    callgrindArguments.setLabelText(Tr::tr("Extra CallGrind arguments:"));

    registerAspect(&enableEventToolTips);
    enableEventToolTips.setDefaultValue(true);
    enableEventToolTips.setSettingsKey(base + "Callgrind.EnableEventToolTips");
    enableEventToolTips.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBoxWithoutDummyLabel);
    enableEventToolTips.setLabelText(Tr::tr("Show additional information for events in tooltips"));

    registerAspect(&enableCacheSim);
    enableCacheSim.setSettingsKey(base + "Callgrind.EnableCacheSim");
    enableCacheSim.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBoxWithoutDummyLabel);
    enableCacheSim.setLabelText(Tr::tr("Enable cache simulation"));
    enableCacheSim.setToolTip("<html><head/><body>" + Tr::tr(
        "<p>Does full cache simulation.</p>\n"
        "<p>By default, only instruction read accesses will be counted (\"Ir\").</p>\n"
        "<p>\n"
        "With cache simulation, further event counters are enabled:\n"
        "<ul><li>Cache misses on instruction reads (\"I1mr\"/\"I2mr\").</li>\n"
        "<li>Data read accesses (\"Dr\") and related cache misses (\"D1mr\"/\"D2mr\").</li>\n"
        "<li>Data write accesses (\"Dw\") and related cache misses (\"D1mw\"/\"D2mw\").</li></ul>\n"
        "</p>") + "</body></html>");

    registerAspect(&enableBranchSim);
    enableBranchSim.setSettingsKey(base + "Callgrind.EnableBranchSim");
    enableBranchSim.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBoxWithoutDummyLabel);
    enableBranchSim.setLabelText(Tr::tr("Enable branch prediction simulation"));
    enableBranchSim.setToolTip("<html><head/><body>\n" + Tr::tr(
        "<p>Does branch prediction simulation.</p>\n"
        "<p>Further event counters are enabled: </p>\n"
        "<ul><li>Number of executed conditional branches and related predictor misses (\n"
        "\"Bc\"/\"Bcm\").</li>\n"
        "<li>Executed indirect jumps and related misses of the jump address predictor (\n"
        "\"Bi\"/\"Bim\").)</li></ul>") + "</body></html>");

    registerAspect(&collectSystime);
    collectSystime.setSettingsKey(base + "Callgrind.CollectSystime");
    collectSystime.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBoxWithoutDummyLabel);
    collectSystime.setLabelText(Tr::tr("Collect system call time"));
    collectSystime.setToolTip(Tr::tr("Collects information for system call times."));

    registerAspect(&collectBusEvents);
    collectBusEvents.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBoxWithoutDummyLabel);
    collectBusEvents.setSettingsKey(base + "Callgrind.CollectBusEvents");
    collectBusEvents.setLabelText(Tr::tr("Collect global bus events"));
    collectBusEvents.setToolTip(Tr::tr("Collect the number of global bus events that are executed. "
        "The event type \"Ge\" is used for these events."));

    registerAspect(&minimumInclusiveCostRatio);
    minimumInclusiveCostRatio.setSettingsKey(base + "Callgrind.MinimumCostRatio");
    minimumInclusiveCostRatio.setDefaultValue(0.01);
    minimumInclusiveCostRatio.setSuffix(Tr::tr("%"));
    minimumInclusiveCostRatio.setLabelText(Tr::tr("Result view: Minimum event cost:"));
    minimumInclusiveCostRatio.setToolTip(Tr::tr("Limits the amount of results the profiler gives you. "
         "A lower limit will likely increase performance."));

    registerAspect(&visualizationMinimumInclusiveCostRatio);
    visualizationMinimumInclusiveCostRatio.setSettingsKey(base + "Callgrind.VisualisationMinimumCostRatio");
    visualizationMinimumInclusiveCostRatio.setDefaultValue(10.0);
    visualizationMinimumInclusiveCostRatio.setLabelText(Tr::tr("Visualization: Minimum event cost:"));
    visualizationMinimumInclusiveCostRatio.setSuffix(Tr::tr("%"));

    registerAspect(&visibleErrorKinds);
    visibleErrorKinds.setSettingsKey(base + "VisibleErrorKinds");
    QList<int> defaultErrorKinds;
    for (int i = 0; i < Valgrind::XmlProtocol::MemcheckErrorKindCount; ++i)
        defaultErrorKinds << i;
    visibleErrorKinds.setDefaultValue(defaultErrorKinds);
}


//////////////////////////////////////////////////////////////////
//
// ValgrindGlobalSettings
//
//////////////////////////////////////////////////////////////////

static ValgrindGlobalSettings *theGlobalSettings = nullptr;

ValgrindGlobalSettings::ValgrindGlobalSettings()
    : ValgrindBaseSettings(true)
{
    theGlobalSettings = this;

    const QString base = "Analyzer.Valgrind";

    registerAspect(&lastSuppressionDirectory);
    lastSuppressionDirectory.setSettingsKey(base + "LastSuppressionDirectory");

    registerAspect(&lastSuppressionHistory);
    lastSuppressionHistory.setSettingsKey(base + "LastSuppressionHistory");

    registerAspect(&detectCycles);
    detectCycles.setSettingsKey(base + "Callgrind.CycleDetection");
    detectCycles.setDefaultValue(true);
    detectCycles.setLabelText("O"); // FIXME: Create a real icon
    detectCycles.setToolTip(Tr::tr("Enable cycle detection to properly handle recursive "
        "or circular function calls."));

    registerAspect(&costFormat);
    costFormat.setSettingsKey(base + "Callgrind.CostFormat");
    costFormat.setDefaultValue(CostDelegate::FormatRelative);
    costFormat.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);

    registerAspect(&shortenTemplates);
    shortenTemplates.setSettingsKey(base + "Callgrind.ShortenTemplates");
    shortenTemplates.setDefaultValue(true);
    shortenTemplates.setLabelText("<>"); // FIXME: Create a real icon
    shortenTemplates.setToolTip(Tr::tr("Remove template parameter lists when displaying function names."));

    setConfigWidgetCreator([this] { return ValgrindOptionsPage::createSettingsWidget(this); });
    readSettings();

    setAutoApply(false);
}

ValgrindGlobalSettings *ValgrindGlobalSettings::instance()
{
    return theGlobalSettings;
}

//
// Memcheck
//

QVariantMap ValgrindBaseSettings::defaultSettings() const
{
    QVariantMap defaults;
    forEachAspect([&defaults](BaseAspect *aspect) {
        defaults.insert(aspect->settingsKey(), aspect->defaultValue());
    });
    return defaults;
}

static const char groupC[] = "Analyzer";

void ValgrindGlobalSettings::readSettings()
{
    // Read stored values
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(groupC);
    QVariantMap map;
    const QStringList childKey = settings->childKeys();
    for (const QString &key : childKey)
        map.insert(key, settings->value(key));
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
    : ValgrindBaseSettings(false)
{
    setConfigWidgetCreator([this] { return ValgrindOptionsPage::createSettingsWidget(this); });

    connect(this, &AspectContainer::fromMapFinished, [this] {
        // FIXME: Update project page e.g. on "Restore Global", aspects
        // there are 'autoapply', and Aspect::cancel() is normally part of
        // the 'manual apply' machinery.
        setAutoApply(false);
        cancel();
        setAutoApply(true);
    });
}

} // Valgrind::Internal
