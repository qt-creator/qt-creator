// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "valgrindsettings.h"

#include "callgrindcostdelegate.h"
#include "valgrindtr.h"
#include "xmlprotocol/error.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <debugger/analyzer/analyzericons.h>
#include <debugger/debuggertr.h>

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QListView>
#include <QMetaEnum>
#include <QPushButton>
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
    const FilePaths files =
            FileUtils::getOpenFilePaths(nullptr,
                      Tr::tr("Valgrind Suppression Files"),
                      globalSettings().lastSuppressionDirectory(),
                      Tr::tr("Valgrind Suppression File (*.supp);;All Files (*)"));
    //dialog.setHistory(conf->lastSuppressionDialogHistory());
    if (!files.isEmpty()) {
        for (const FilePath &file : files)
            m_model.appendRow(new QStandardItem(file.toString()));
        globalSettings().lastSuppressionDirectory.setValue(files.at(0).absolutePath());
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

SuppressionAspect::SuppressionAspect(AspectContainer *container, bool global)
    : TypedAspect(container)
{
    d = new SuppressionAspectPrivate(this, global);
    setSettingsKey("Analyzer.Valgrind.SuppressionFiles");
}

SuppressionAspect::~SuppressionAspect()
{
    delete d;
}

void SuppressionAspect::addToLayout(Layouting::LayoutItem &parent)
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

    parent.addItem(Column { Tr::tr("Suppression files:"), st });
    Row group {
        d->entryList.data(),
        Column { d->addEntry.data(), d->removeEntry.data(), st }
    };
    parent.addItem(Span { 2, group });
}

void SuppressionAspect::fromMap(const Store &map)
{
    BaseAspect::fromMap(map); // FIXME Looks wrong, as it skips the intermediate level
}

void SuppressionAspect::toMap(Store &map) const
{
    BaseAspect::toMap(map);
}

bool SuppressionAspect::guiToBuffer()
{
    const FilePaths old = m_buffer;
    m_buffer.clear();
    for (int i = 0; i < d->m_model.rowCount(); ++i)
        m_buffer.append(FilePath::fromUserInput(d->m_model.item(i)->text()));
    return m_buffer != old;
}

void SuppressionAspect::bufferToGui()
{
    d->m_model.clear();
    for (const FilePath &file : m_buffer)
        d->m_model.appendRow(new QStandardItem(file.toUserOutput()));
}

//////////////////////////////////////////////////////////////////
//
// ValgrindBaseSettings
//
//////////////////////////////////////////////////////////////////

ValgrindSettings::ValgrindSettings(bool global)
    : suppressions(this, global)
{
    setSettingsGroup("Analyzer");
    setAutoApply(false);

    // Note that this is used twice, once for project settings in the .user files
    // and once for global settings in QtCreator.ini. This uses intentionally
    // the same key to facilitate copying using fromMap/toMap.
    Key base = "Analyzer.Valgrind.";

    valgrindExecutable.setSettingsKey(base + "ValgrindExecutable");
    valgrindExecutable.setDefaultValue("valgrind");
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

    valgrindArguments.setSettingsKey(base + "ValgrindArguments");
    valgrindArguments.setDisplayStyle(StringAspect::LineEditDisplay);
    valgrindArguments.setLabelText(Tr::tr("Valgrind arguments:"));

    selfModifyingCodeDetection.setSettingsKey(base + "SelfModifyingCodeDetection");
    selfModifyingCodeDetection.setDefaultValue(DetectSmcStackOnly);
    selfModifyingCodeDetection.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    selfModifyingCodeDetection.addOption("No");
    selfModifyingCodeDetection.addOption("Only on Stack");
    selfModifyingCodeDetection.addOption("Everywhere");
    selfModifyingCodeDetection.addOption("Everywhere Except in File-backend Mappings");
    selfModifyingCodeDetection.setLabelText(Tr::tr("Detect self-modifying code:"));

    // Memcheck
    memcheckArguments.setSettingsKey(base + "Memcheck.Arguments");
    memcheckArguments.setDisplayStyle(StringAspect::LineEditDisplay);
    memcheckArguments.setLabelText(Tr::tr("Extra Memcheck arguments:"));

    filterExternalIssues.setSettingsKey(base + "FilterExternalIssues");
    filterExternalIssues.setDefaultValue(true);
    filterExternalIssues.setIcon(Icons::FILTER.icon());
    filterExternalIssues.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBox);
    filterExternalIssues.setLabelText(Tr::tr("Show Project Costs Only"));
    filterExternalIssues.setToolTip(Tr::tr("Show only profiling info that originated from this project source."));

    trackOrigins.setSettingsKey(base + "TrackOrigins");
    trackOrigins.setDefaultValue(true);
    trackOrigins.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBox);
    trackOrigins.setLabelText(Tr::tr("Track origins of uninitialized memory"));

    showReachable.setSettingsKey(base + "ShowReachable");
    showReachable.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBox);
    showReachable.setLabelText(Tr::tr("Show reachable and indirectly lost blocks"));

    leakCheckOnFinish.setSettingsKey(base + "LeakCheckOnFinish");
    leakCheckOnFinish.setDefaultValue(LeakCheckOnFinishSummaryOnly);
    leakCheckOnFinish.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    leakCheckOnFinish.addOption(Tr::tr("No"));
    leakCheckOnFinish.addOption(Tr::tr("Summary Only"));
    leakCheckOnFinish.addOption(Tr::tr("Full"));
    leakCheckOnFinish.setLabelText(Tr::tr("Check for leaks on finish:"));

    numCallers.setSettingsKey(base + "NumCallers");
    numCallers.setDefaultValue(25);
    numCallers.setLabelText(Tr::tr("Backtrace frame count:"));

    lastSuppressionDirectory.setSettingsKey(base + "LastSuppressionDirectory");
    lastSuppressionDirectory.setVisible(global);

    lastSuppressionHistory.setSettingsKey(base + "LastSuppressionHistory");
    lastSuppressionHistory.setVisible(global);

    // Callgrind

    kcachegrindExecutable.setSettingsKey(base + "KCachegrindExecutable");
    kcachegrindExecutable.setDefaultValue("kcachegrind");
    kcachegrindExecutable.setLabelText(Tr::tr("KCachegrind executable:"));
    kcachegrindExecutable.setExpectedKind(Utils::PathChooser::Command);
    kcachegrindExecutable.setDisplayName(Tr::tr("KCachegrind Command"));

    callgrindArguments.setSettingsKey(base + "Callgrind.Arguments");
    callgrindArguments.setDisplayStyle(StringAspect::LineEditDisplay);
    callgrindArguments.setLabelText(Tr::tr("Extra Callgrind arguments:"));

    enableEventToolTips.setDefaultValue(true);
    enableEventToolTips.setSettingsKey(base + "Callgrind.EnableEventToolTips");
    enableEventToolTips.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBox);
    enableEventToolTips.setLabelText(Tr::tr("Show additional information for events in tooltips"));

    enableCacheSim.setSettingsKey(base + "Callgrind.EnableCacheSim");
    enableCacheSim.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBox);
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

    enableBranchSim.setSettingsKey(base + "Callgrind.EnableBranchSim");
    enableBranchSim.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBox);
    enableBranchSim.setLabelText(Tr::tr("Enable branch prediction simulation"));
    enableBranchSim.setToolTip("<html><head/><body>\n" + Tr::tr(
        "<p>Does branch prediction simulation.</p>\n"
        "<p>Further event counters are enabled: </p>\n"
        "<ul><li>Number of executed conditional branches and related predictor misses (\n"
        "\"Bc\"/\"Bcm\").</li>\n"
        "<li>Executed indirect jumps and related misses of the jump address predictor (\n"
        "\"Bi\"/\"Bim\").)</li></ul>") + "</body></html>");

    collectSystime.setSettingsKey(base + "Callgrind.CollectSystime");
    collectSystime.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBox);
    collectSystime.setLabelText(Tr::tr("Collect system call time"));
    collectSystime.setToolTip(Tr::tr("Collects information for system call times."));

    collectBusEvents.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBox);
    collectBusEvents.setSettingsKey(base + "Callgrind.CollectBusEvents");
    collectBusEvents.setLabelText(Tr::tr("Collect global bus events"));
    collectBusEvents.setToolTip(Tr::tr("Collect the number of global bus events that are executed. "
        "The event type \"Ge\" is used for these events."));

    minimumInclusiveCostRatio.setSettingsKey(base + "Callgrind.MinimumCostRatio");
    minimumInclusiveCostRatio.setDefaultValue(0.01);
    minimumInclusiveCostRatio.setSuffix(Tr::tr("%"));
    minimumInclusiveCostRatio.setLabelText(Tr::tr("Result view: Minimum event cost:"));
    minimumInclusiveCostRatio.setToolTip(Tr::tr("Limits the amount of results the profiler gives you. "
         "A lower limit will likely increase performance."));

    visualizationMinimumInclusiveCostRatio.setSettingsKey(base + "Callgrind.VisualisationMinimumCostRatio");
    visualizationMinimumInclusiveCostRatio.setDefaultValue(10.0);
    visualizationMinimumInclusiveCostRatio.setLabelText(Tr::tr("Visualization: Minimum event cost:"));
    visualizationMinimumInclusiveCostRatio.setSuffix(Tr::tr("%"));

    visibleErrorKinds.setSettingsKey(base + "VisibleErrorKinds");
    QList<int> defaultErrorKinds;
    const QMetaEnum memcheckErrorEnum = QMetaEnum::fromType<XmlProtocol::MemcheckError>();
    for (int i = 0; i < memcheckErrorEnum.keyCount(); ++i)
        defaultErrorKinds << memcheckErrorEnum.value(i);
    visibleErrorKinds.setDefaultValue(defaultErrorKinds);

    detectCycles.setSettingsKey(base + "Callgrind.CycleDetection");
    detectCycles.setDefaultValue(true);
    detectCycles.setLabelText("O"); // FIXME: Create a real icon
    detectCycles.setToolTip(Tr::tr("Enable cycle detection to properly handle recursive "
        "or circular function calls."));
    detectCycles.setVisible(global);

    costFormat.setSettingsKey(base + "Callgrind.CostFormat");
    costFormat.setDefaultValue(CostDelegate::FormatRelative);
    costFormat.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    costFormat.setVisible(global);

    shortenTemplates.setSettingsKey(base + "Callgrind.ShortenTemplates");
    shortenTemplates.setDefaultValue(true);
    shortenTemplates.setLabelText("<>"); // FIXME: Create a real icon
    shortenTemplates.setToolTip(Tr::tr("Remove template parameter lists when displaying function names."));
    shortenTemplates.setVisible(global);

    setLayouter([this] {
        using namespace Layouting;

        // clang-format off
        Grid generic {
            valgrindExecutable, br,
            valgrindArguments, br,
            selfModifyingCodeDetection, br
        };

        Grid memcheck {
            memcheckArguments, br,
            trackOrigins, br,
            showReachable, br,
            leakCheckOnFinish, br,
            numCallers, br,
            filterExternalIssues, br,
            suppressions
        };

        Grid callgrind {
            callgrindArguments, br,
            kcachegrindExecutable, br,
            minimumInclusiveCostRatio, br,
            visualizationMinimumInclusiveCostRatio, br,
            enableEventToolTips, br,
            Span {
                2,
                Group {
                    Column {
                        enableCacheSim,
                        enableBranchSim,
                        collectSystime,
                        collectBusEvents,
                    }
                }
            }
        };

        return Column {
            Group { title(Tr::tr("Valgrind Generic Settings")), generic },
            Group { title(Tr::tr("Memcheck Memory Analysis Options")), memcheck },
            Group { title(Tr::tr("Callgrind Profiling Options")), callgrind },
            st,
        };
        // clang-format on
    });

    if (global) {
        readSettings();
    } else {
        // FIXME: Is this needed?
        connect(this, &AspectContainer::fromMapFinished, [this] {
            // FIXME: Update project page e.g. on "Restore Global", aspects
            // there are 'autoapply', and Aspect::cancel() is normally part of
            // the 'manual apply' machinery.
            setAutoApply(false);
            cancel();
            setAutoApply(true);
        });
    }
}

ValgrindSettings &globalSettings()
{
    static ValgrindSettings theSettings{true};
    return theSettings;
}

// ValgrindSettingsPage

class ValgrindSettingsPage final : public Core::IOptionsPage
{
public:
    ValgrindSettingsPage()
    {
        setId(ANALYZER_VALGRIND_SETTINGS);
        setDisplayName(Tr::tr("Valgrind"));
        setCategory("T.Analyzer");
        setDisplayCategory(::Debugger::Tr::tr("Analyzer"));
        setCategoryIconPath(Analyzer::Icons::SETTINGSCATEGORY_ANALYZER);
        setSettingsProvider([] { return &globalSettings(); });
    }
};

const ValgrindSettingsPage settingsPage;

} // Valgrind::Internal
