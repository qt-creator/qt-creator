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

#include "callgrindtool.h"

#include "callgrindcostdelegate.h"
#include "callgrindcostview.h"
#include "callgrindengine.h"
#include "callgrindtextmark.h"
#include "callgrindvisualisation.h"

#include <valgrind/callgrind/callgrindcallmodel.h>
#include <valgrind/callgrind/callgrindcostitem.h>
#include <valgrind/callgrind/callgrinddatamodel.h>
#include <valgrind/callgrind/callgrindfunction.h>
#include <valgrind/callgrind/callgrindfunctioncall.h>
#include <valgrind/callgrind/callgrindparsedata.h>
#include <valgrind/callgrind/callgrindparser.h>
#include <valgrind/callgrind/callgrindproxymodel.h>
#include <valgrind/callgrind/callgrindstackbrowser.h>
#include <valgrind/valgrindplugin.h>
#include <valgrind/valgrindruncontrolfactory.h>
#include <valgrind/valgrindsettings.h>

#include <debugger/debuggerconstants.h>
#include <debugger/analyzer/analyzerconstants.h>
#include <debugger/analyzer/analyzermanager.h>
#include <debugger/analyzer/analyzerstartparameters.h>
#include <debugger/analyzer/analyzerutils.h>
#include <debugger/analyzer/startremotedialog.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <cplusplus/LookupContext.h>
#include <cplusplus/Overview.h>
#include <cplusplus/Symbols.h>

#include <cppeditor/cppeditorconstants.h>

#include <extensionsystem/pluginmanager.h>

#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/session.h>
#include <projectexplorer/taskhub.h>

#include <utils/fancymainwindow.h>
#include <utils/qtcassert.h>
#include <utils/styledbar.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QActionGroup>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>

using namespace Debugger;
using namespace Core;
using namespace Valgrind::Callgrind;
using namespace TextEditor;
using namespace ProjectExplorer;
using namespace Utils;

namespace Valgrind {
namespace Internal {

const char CallgrindPerspectiveId[]       = "Callgrind.Perspective";
const char CallgrindLocalActionId[]       = "Callgrind.Local.Action";
const char CallgrindRemoteActionId[]      = "Callgrind.Remote.Action";
const char CallgrindCallersDockId[]       = "Callgrind.Callers.Dock";
const char CallgrindCalleesDockId[]       = "Callgrind.Callees.Dock";
const char CallgrindFlatDockId[]          = "Callgrind.Flat.Dock";
const char CallgrindVisualizationDockId[] = "Callgrind.Visualization.Dock";

const char CALLGRIND_RUN_MODE[]           = "CallgrindTool.CallgrindRunMode";

class CallgrindTool : public QObject
{
    Q_OBJECT

public:
    CallgrindTool(QObject *parent);
    ~CallgrindTool();

    ValgrindRunControl *createRunControl(RunConfiguration *runConfiguration, Id runMode);

    void setParseData(ParseData *data);
    CostDelegate::CostFormat costFormat() const;

    void doClear(bool clearParseData);
    void updateEventCombo();

signals:
    void dumpRequested();
    void resetRequested();
    void pauseToggled(bool checked);

public:
    void slotClear();
    void slotRequestDump();
    void loadExternalLogFile();

    void selectFunction(const Function *);
    void setCostFormat(CostDelegate::CostFormat format);
    void enableCycleDetection(bool enabled);
    void shortenTemplates(bool enabled);
    void setCostEvent(int index);

    /// This function will add custom text marks to the editor
    /// \note Call this after the data model has been populated
    void createTextMarks();

    /// This function will clear all text marks from the editor
    void clearTextMarks();

    void updateFilterString();
    void updateCostFormat();

    void handleFilterProjectCosts();
    void handleShowCostsOfFunction();

    void slotGoToOverview();
    void stackBrowserChanged();

    /// If \param busy is true, all widgets get a busy cursor when hovered
    void setBusyCursor(bool busy);

    void dataFunctionSelected(const QModelIndex &index);
    void calleeFunctionSelected(const QModelIndex &index);
    void callerFunctionSelected(const QModelIndex &index);
    void visualisationFunctionSelected(const Function *function);
    void showParserResults(const ParseData *data);

    void takeParserDataFromRunControl(CallgrindRunControl *rc);
    void takeParserData(ParseData *data);
    void engineStarting();
    void engineFinished();

    void editorOpened(IEditor *);
    void requestContextMenu(TextEditorWidget *widget, int line, QMenu *menu);
    void updateRunActions();

public:
    DataModel m_dataModel;
    DataProxyModel m_proxyModel;
    StackBrowser m_stackBrowser;

    CallModel m_callersModel;
    CallModel m_calleesModel;

    QSortFilterProxyModel m_callersProxy;
    QSortFilterProxyModel m_calleesProxy;

    // Callgrind widgets
    CostView *m_flatView = 0;
    CostView *m_callersView = 0;
    CostView *m_calleesView = 0;
    Visualisation *m_visualization = 0;

    // Navigation
    QAction *m_goBack = 0;
    QAction *m_goNext = 0;
    QLineEdit *m_searchFilter = 0;

    // Cost formatting
    QAction *m_filterProjectCosts = 0;
    QAction *m_costAbsolute = 0;
    QAction *m_costRelative = 0;
    QAction *m_costRelativeToParent = 0;
    QAction *m_cycleDetection;
    QAction *m_shortenTemplates;
    QComboBox *m_eventCombo = 0;

    QTimer m_updateTimer;

    QVector<CallgrindTextMark *> m_textMarks;

    QAction *m_startAction = 0;
    QAction *m_stopAction = 0;
    QAction *m_loadExternalLogFile;
    QAction *m_dumpAction = 0;
    QAction *m_resetAction = 0;
    QAction *m_pauseAction = 0;

    QString m_toggleCollectFunction;
    bool m_toolBusy = false;
};

CallgrindTool::CallgrindTool(QObject *parent)
    : QObject(parent)
{
    setObjectName(QLatin1String("CallgrindTool"));

    m_updateTimer.setInterval(200);
    m_updateTimer.setSingleShot(true);

    m_proxyModel.setSourceModel(&m_dataModel);
    m_proxyModel.setDynamicSortFilter(true);
    m_proxyModel.setSortCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel.setFilterKeyColumn(DataModel::NameColumn);
    m_proxyModel.setFilterCaseSensitivity(Qt::CaseInsensitive);

    connect(&m_stackBrowser, &StackBrowser::currentChanged,
            this, &CallgrindTool::stackBrowserChanged);
    connect(&m_updateTimer, &QTimer::timeout,
            this, &CallgrindTool::updateFilterString);

    connect(EditorManager::instance(), &EditorManager::editorOpened,
            this, &CallgrindTool::editorOpened);

    m_startAction = Debugger::createStartAction();
    m_stopAction = Debugger::createStopAction();

    ActionDescription desc;
    desc.setToolTip(tr("Valgrind Function Profiler uses the "
        "Callgrind tool to record function calls when a program runs."));

    if (!Utils::HostOsInfo::isWindowsHost()) {
        desc.setText(tr("Valgrind Function Profiler"));
        desc.setPerspectiveId(CallgrindPerspectiveId);
        desc.setRunControlCreator([this](RunConfiguration *runConfiguration, Id) {
            return createRunControl(runConfiguration, CALLGRIND_RUN_MODE);
        });
        desc.setToolMode(OptimizedMode);
        desc.setRunMode(CALLGRIND_RUN_MODE);
        desc.setMenuGroup(Debugger::Constants::G_ANALYZER_TOOLS);
        Debugger::registerAction(CallgrindLocalActionId, desc, m_startAction);
    }

    desc.setText(tr("Valgrind Function Profiler (External Application)"));
    desc.setPerspectiveId(CallgrindPerspectiveId);
    desc.setCustomToolStarter([this](RunConfiguration *runConfig) {
        StartRemoteDialog dlg;
        if (dlg.exec() != QDialog::Accepted)
            return;
        ValgrindRunControl *rc = createRunControl(runConfig, CALLGRIND_RUN_MODE);
        QTC_ASSERT(rc, return);
        const auto runnable = dlg.runnable();
        rc->setRunnable(runnable);
        AnalyzerConnection connection;
        connection.connParams = dlg.sshParams();
        rc->setConnection(connection);
        rc->setDisplayName(runnable.executable);
        ProjectExplorerPlugin::startRunControl(rc, CALLGRIND_RUN_MODE);
    });
    desc.setMenuGroup(Debugger::Constants::G_ANALYZER_REMOTE_TOOLS);
    Debugger::registerAction(CallgrindRemoteActionId, desc);

    // If there is a CppEditor context menu add our own context menu actions.
    if (ActionContainer *editorContextMenu =
            ActionManager::actionContainer(CppEditor::Constants::M_CONTEXT)) {
        Context analyzerContext = Context(Debugger::Constants::C_DEBUGMODE);
        editorContextMenu->addSeparator(analyzerContext);

        auto action = new QAction(tr("Profile Costs of This Function and Its Callees"), this);
        action->setIcon(ProjectExplorer::Icons::ANALYZER_START_SMALL.icon());
        connect(action, &QAction::triggered, this,
                &CallgrindTool::handleShowCostsOfFunction);
        Command *cmd = ActionManager::registerAction(action, "Analyzer.Callgrind.ShowCostsOfFunction",
            analyzerContext);
        editorContextMenu->addAction(cmd);
        cmd->setAttribute(Command::CA_Hide);
        cmd->setAttribute(Command::CA_NonConfigurable);
    }

    QSettings *coreSettings = ICore::settings();

    //
    // DockWidgets
    //
    m_visualization = new Visualisation;
    m_visualization->setFrameStyle(QFrame::NoFrame);
    m_visualization->setObjectName(QLatin1String("Valgrind.CallgrindTool.Visualisation"));
    m_visualization->setWindowTitle(tr("Visualization"));
    m_visualization->setModel(&m_dataModel);
    connect(m_visualization, &Visualisation::functionActivated,
            this, &CallgrindTool::visualisationFunctionSelected);

    m_callersView = new CostView;
    m_callersView->setObjectName(QLatin1String("Valgrind.CallgrindTool.CallersView"));
    m_callersView->setWindowTitle(tr("Callers"));
    m_callersView->setSettings(coreSettings, "Valgrind.CallgrindTool.CallersView");
    m_callersView->sortByColumn(CallModel::CostColumn);
    m_callersView->setFrameStyle(QFrame::NoFrame);
    // enable sorting
    m_callersProxy.setSourceModel(&m_callersModel);
    m_callersView->setModel(&m_callersProxy);
    m_callersView->hideColumn(CallModel::CalleeColumn);
    connect(m_callersView, &QAbstractItemView::activated,
            this, &CallgrindTool::callerFunctionSelected);

    m_calleesView = new CostView;
    m_calleesView->setObjectName(QLatin1String("Valgrind.CallgrindTool.CalleesView"));
    m_calleesView->setWindowTitle(tr("Callees"));
    m_calleesView->setSettings(coreSettings, "Valgrind.CallgrindTool.CalleesView");
    m_calleesView->sortByColumn(CallModel::CostColumn);
    m_calleesView->setFrameStyle(QFrame::NoFrame);
    // enable sorting
    m_calleesProxy.setSourceModel(&m_calleesModel);
    m_calleesView->setModel(&m_calleesProxy);
    m_calleesView->hideColumn(CallModel::CallerColumn);
    connect(m_calleesView, &QAbstractItemView::activated,
            this, &CallgrindTool::calleeFunctionSelected);

    m_flatView = new CostView;
    m_flatView->setObjectName(QLatin1String("Valgrind.CallgrindTool.FlatView"));
    m_flatView->setWindowTitle(tr("Functions"));
    m_flatView->setSettings(coreSettings, "Valgrind.CallgrindTool.FlatView");
    m_flatView->sortByColumn(DataModel::SelfCostColumn);
    m_flatView->setFrameStyle(QFrame::NoFrame);
    m_flatView->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_flatView->setModel(&m_proxyModel);
    connect(m_flatView, &QAbstractItemView::activated,
            this, &CallgrindTool::dataFunctionSelected);

    updateCostFormat();

    //
    // Control Widget
    //

    // load external log file
    auto action = m_loadExternalLogFile = new QAction(this);
    action->setIcon(Utils::Icons::OPENFILE.icon());
    action->setToolTip(tr("Load External Log File"));
    connect(action, &QAction::triggered, this, &CallgrindTool::loadExternalLogFile);

    // dump action
    m_dumpAction = action = new QAction(this);
    action->setDisabled(true);
    action->setIcon(Utils::Icons::REDO.icon());
    //action->setText(tr("Dump"));
    action->setToolTip(tr("Request the dumping of profile information. This will update the Callgrind visualization."));
    connect(action, &QAction::triggered, this, &CallgrindTool::slotRequestDump);

    // reset action
    m_resetAction = action = new QAction(this);
    action->setDisabled(true);
    action->setIcon(Utils::Icons::RELOAD.icon());
    //action->setText(tr("Reset"));
    action->setToolTip(tr("Reset all event counters."));
    connect(action, &QAction::triggered, this, &CallgrindTool::resetRequested);

    // pause action
    m_pauseAction = action = new QAction(this);
    action->setCheckable(true);
    action->setIcon(Utils::Icons::INTERRUPT_SMALL_TOOLBAR.icon());
    //action->setText(tr("Ignore"));
    action->setToolTip(tr("Pause event logging. No events are counted which will speed up program execution during profiling."));
    connect(action, &QAction::toggled, this, &CallgrindTool::pauseToggled);

    // navigation
    // go back
    m_goBack = action = new QAction(this);
    action->setDisabled(true);
    action->setIcon(Utils::Icons::PREV_TOOLBAR.icon());
    action->setToolTip(tr("Go back one step in history. This will select the previously selected item."));
    connect(action, &QAction::triggered, &m_stackBrowser, &StackBrowser::goBack);

    // go forward
    m_goNext = action = new QAction(this);
    action->setDisabled(true);
    action->setIcon(Utils::Icons::NEXT_TOOLBAR.icon());
    action->setToolTip(tr("Go forward one step in history."));
    connect(action, &QAction::triggered, &m_stackBrowser, &StackBrowser::goNext);

    // event selection
    m_eventCombo = new QComboBox;
    m_eventCombo->setToolTip(tr("Selects which events from the profiling data are shown and visualized."));
    connect(m_eventCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &CallgrindTool::setCostEvent);
    updateEventCombo();

    ToolbarDescription toolbar;
    toolbar.addAction(m_startAction);
    toolbar.addAction(m_stopAction);
    toolbar.addAction(m_loadExternalLogFile);
    toolbar.addAction(m_dumpAction);
    toolbar.addAction(m_resetAction);
    toolbar.addAction(m_pauseAction);
    toolbar.addAction(m_goBack);
    toolbar.addAction(m_goNext);
    toolbar.addWidget(new Utils::StyledSeparator);
    toolbar.addWidget(m_eventCombo);

    // Cost formatting
    {
    auto group = new QActionGroup(this);

    // Show costs as absolute numbers
    m_costAbsolute = new QAction(tr("Absolute Costs"), this);
    m_costAbsolute->setToolTip(tr("Show costs as absolute numbers."));
    m_costAbsolute->setCheckable(true);
    m_costAbsolute->setChecked(true);
    connect(m_costAbsolute, &QAction::toggled, this, &CallgrindTool::updateCostFormat);
    group->addAction(m_costAbsolute);

    // Show costs in percentages
    m_costRelative = new QAction(tr("Relative Costs"), this);
    m_costRelative->setToolTip(tr("Show costs relative to total inclusive cost."));
    m_costRelative->setCheckable(true);
    connect(m_costRelative, &QAction::toggled, this, &CallgrindTool::updateCostFormat);
    group->addAction(m_costRelative);

    // Show costs relative to parent
    m_costRelativeToParent = new QAction(tr("Relative Costs to Parent"), this);
    m_costRelativeToParent->setToolTip(tr("Show costs relative to parent function's inclusive cost."));
    m_costRelativeToParent->setCheckable(true);
    connect(m_costRelativeToParent, &QAction::toggled, this, &CallgrindTool::updateCostFormat);
    group->addAction(m_costRelativeToParent);

    auto button = new QToolButton;
    button->addActions(group->actions());
    button->setPopupMode(QToolButton::InstantPopup);
    button->setText(QLatin1String("$"));
    button->setToolTip(tr("Cost Format"));
    toolbar.addWidget(button);
    }

    ValgrindGlobalSettings *settings = ValgrindPlugin::globalSettings();

    // Cycle detection
    //action = new QAction(QLatin1String("Cycle Detection"), this); ///FIXME: icon
    action = m_cycleDetection = new QAction(QLatin1String("O"), this); ///FIXME: icon
    action->setToolTip(tr("Enable cycle detection to properly handle recursive or circular function calls."));
    action->setCheckable(true);
    connect(action, &QAction::toggled, &m_dataModel, &DataModel::enableCycleDetection);
    connect(action, &QAction::toggled, settings, &ValgrindGlobalSettings::setDetectCycles);

    // Shorter template signature
    action = m_shortenTemplates = new QAction(QLatin1String("<>"), this);
    action->setToolTip(tr("Remove template parameter lists when displaying function names."));
    action->setCheckable(true);
    connect(action, &QAction::toggled, &m_dataModel, &DataModel::setShortenTemplates);
    connect(action, &QAction::toggled, settings, &ValgrindGlobalSettings::setShortenTemplates);

    // Filtering
    action = m_filterProjectCosts = new QAction(tr("Show Project Costs Only"), this);
    action->setIcon(Utils::Icons::FILTER.icon());
    action->setToolTip(tr("Show only profiling info that originated from this project source."));
    action->setCheckable(true);
    connect(action, &QAction::toggled, this, &CallgrindTool::handleFilterProjectCosts);

    // Filter
    ///FIXME: find workaround for https://bugreports.qt.io/browse/QTCREATORBUG-3247
    m_searchFilter = new QLineEdit;
    m_searchFilter->setPlaceholderText(tr("Filter..."));
    connect(m_searchFilter, &QLineEdit::textChanged,
            &m_updateTimer, static_cast<void(QTimer::*)()>(&QTimer::start));

    setCostFormat(settings->costFormat());
    enableCycleDetection(settings->detectCycles());

    toolbar.addAction(m_cycleDetection);
    toolbar.addAction(m_shortenTemplates);
    toolbar.addAction(m_filterProjectCosts);
    toolbar.addWidget(m_searchFilter);
    Debugger::registerToolbar(CallgrindPerspectiveId, toolbar);

    Debugger::registerPerspective(CallgrindPerspectiveId, new Perspective(tr("Callgrind"), {
        { CallgrindFlatDockId, m_flatView, {}, Perspective::SplitVertical },
        { CallgrindCalleesDockId, m_calleesView, {}, Perspective::SplitVertical },
        { CallgrindCallersDockId, m_callersView, CallgrindCalleesDockId, Perspective::SplitHorizontal },
        { CallgrindVisualizationDockId, m_visualization, {}, Perspective::SplitVertical,
          false, Qt::RightDockWidgetArea }
    }));

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::updateRunActions,
            this, &CallgrindTool::updateRunActions);
}

CallgrindTool::~CallgrindTool()
{
    qDeleteAll(m_textMarks);
}

void CallgrindTool::slotGoToOverview()
{
    selectFunction(0);
}

void CallgrindTool::slotClear()
{
    doClear(true);
}

void CallgrindTool::doClear(bool clearParseData)
{
    if (clearParseData) // Crashed when done from destructor.
        setParseData(0);

    // clear filters
    if (m_filterProjectCosts)
        m_filterProjectCosts->setChecked(false);
    m_proxyModel.setFilterBaseDir(QString());
    if (m_searchFilter)
        m_searchFilter->clear();
    m_proxyModel.setFilterFixedString(QString());
}

void CallgrindTool::setBusyCursor(bool busy)
{
    QCursor cursor(busy ? Qt::BusyCursor : Qt::ArrowCursor);
    m_flatView->setCursor(cursor);
    m_calleesView->setCursor(cursor);
    m_callersView->setCursor(cursor);
    m_visualization->setCursor(cursor);
}

void CallgrindTool::selectFunction(const Function *func)
{
    if (!func) {
        m_flatView->clearSelection();
        m_visualization->setFunction(0);
        m_callersModel.clear();
        m_calleesModel.clear();
        return;
    }

    const QModelIndex index = m_dataModel.indexForObject(func);
    const QModelIndex proxyIndex = m_proxyModel.mapFromSource(index);
    m_flatView->selectionModel()->clearSelection();
    m_flatView->selectionModel()->setCurrentIndex(proxyIndex,
                                                  QItemSelectionModel::ClearAndSelect |
                                                  QItemSelectionModel::Rows);
    m_flatView->scrollTo(proxyIndex);

    m_callersModel.setCalls(func->incomingCalls(), func);
    m_calleesModel.setCalls(func->outgoingCalls(), func);
    m_visualization->setFunction(func);

    const Function *item = m_stackBrowser.current();
    if (!item || item != func)
        m_stackBrowser.select(func);

    if (QFile::exists(func->file())) {
        ///TODO: custom position support?
        int line = func->lineNumber();
        EditorManager::openEditorAt(func->file(), qMax(line, 0));
    }
}

void CallgrindTool::stackBrowserChanged()
{
    m_goBack->setEnabled(m_stackBrowser.hasPrevious());
    m_goNext->setEnabled(m_stackBrowser.hasNext());
    const Function *item = m_stackBrowser.current();
    selectFunction(item);
}

void CallgrindTool::updateFilterString()
{
    m_proxyModel.setFilterFixedString(m_searchFilter->text());
}

void CallgrindTool::setCostFormat(CostDelegate::CostFormat format)
{
    switch (format) {
        case CostDelegate::FormatAbsolute:
            m_costAbsolute->setChecked(true);
            break;
        case CostDelegate::FormatRelative:
            m_costRelative->setChecked(true);
            break;
        case CostDelegate::FormatRelativeToParent:
            m_costRelativeToParent->setChecked(true);
            break;
    }
}

void CallgrindTool::setCostEvent(int index)
{
    // prevent assert in model, don't try to set event to -1
    // happens when we clear the eventcombo
    if (index == -1)
        index = 0;

    m_dataModel.setCostEvent(index);
    m_calleesModel.setCostEvent(index);
    m_callersModel.setCostEvent(index);
}

void CallgrindTool::enableCycleDetection(bool enabled)
{
    m_cycleDetection->setChecked(enabled);
}

void CallgrindTool::shortenTemplates(bool enabled)
{
    m_shortenTemplates->setChecked(enabled);
}

// Following functions can be called with actions=0 or widgets=0
// depending on initialization sequence (whether callgrind was current).
CostDelegate::CostFormat CallgrindTool::costFormat() const
{
    if (m_costRelativeToParent && m_costRelativeToParent->isChecked())
        return CostDelegate::FormatRelativeToParent;
    if (m_costRelative && m_costRelative->isChecked())
        return CostDelegate::FormatRelative;
    return CostDelegate::FormatAbsolute;
}

void CallgrindTool::updateCostFormat()
{
    const CostDelegate::CostFormat format = costFormat();
    if (m_flatView)
        m_flatView->setCostFormat(format);
    if (m_calleesView) {
        m_calleesView->setCostFormat(format);
        m_callersView->setCostFormat(format);
    }
    if (ValgrindGlobalSettings *settings = ValgrindPlugin::globalSettings())
        settings->setCostFormat(format);
}

void CallgrindTool::handleFilterProjectCosts()
{
    Project *pro = ProjectTree::currentProject();

    if (pro && m_filterProjectCosts->isChecked()) {
        const QString projectDir = pro->projectDirectory().toString();
        m_proxyModel.setFilterBaseDir(projectDir);
    } else {
        m_proxyModel.setFilterBaseDir(QString());
    }
}

void CallgrindTool::dataFunctionSelected(const QModelIndex &index)
{
    const Function *func = index.data(DataModel::FunctionRole).value<const Function *>();
    QTC_ASSERT(func, return);

    selectFunction(func);
}

void CallgrindTool::calleeFunctionSelected(const QModelIndex &index)
{
    const FunctionCall *call = index.data(CallModel::FunctionCallRole).value<const FunctionCall *>();
    QTC_ASSERT(call, return);

    selectFunction(call->callee());
}

void CallgrindTool::callerFunctionSelected(const QModelIndex &index)
{
    const FunctionCall *call = index.data(CallModel::FunctionCallRole).value<const FunctionCall *>();
    QTC_ASSERT(call, return);

    selectFunction(call->caller());
}

void CallgrindTool::visualisationFunctionSelected(const Function *function)
{
    if (function && function == m_visualization->function())
        // up-navigation when the initial function was activated
        m_stackBrowser.goBack();
    else
        selectFunction(function);
}

void CallgrindTool::setParseData(ParseData *data)
{
    // we have new parse data, invalidate filters in the proxy model
    m_visualization->setFunction(0);

    // invalidate parse data in the data model
    delete m_dataModel.parseData();

    if (data && data->events().isEmpty()) {
        // might happen if the user cancelled the profile run
        // callgrind then sometimes produces empty callgrind.out.PID files
        delete data;
        data = 0;
    }
    m_dataModel.setParseData(data);
    m_calleesModel.setParseData(data);
    m_callersModel.setParseData(data);

    updateEventCombo();

    // clear history for new data
    m_stackBrowser.clear();

    // unset busy state
    //setBusy(false);
}

void CallgrindTool::updateEventCombo()
{
    QTC_ASSERT(m_eventCombo, return);

    m_eventCombo->clear();

    const ParseData *data = m_dataModel.parseData();
    if (!data || data->events().isEmpty()) {
        m_eventCombo->hide();
        return;
    }

    m_eventCombo->show();
    foreach (const QString &event, data->events())
        m_eventCombo->addItem(ParseData::prettyStringForEvent(event));
}

ValgrindRunControl *CallgrindTool::createRunControl(RunConfiguration *runConfiguration, Id runMode)
{
    auto runControl = new CallgrindRunControl(runConfiguration, runMode);

    connect(runControl, &CallgrindRunControl::parserDataReady, this, &CallgrindTool::takeParserDataFromRunControl);
    connect(runControl, &AnalyzerRunControl::starting, this, &CallgrindTool::engineStarting);
    connect(runControl, &RunControl::finished, this, &CallgrindTool::engineFinished);

    connect(this, &CallgrindTool::dumpRequested, runControl, &CallgrindRunControl::dump);
    connect(this, &CallgrindTool::resetRequested, runControl, &CallgrindRunControl::reset);
    connect(this, &CallgrindTool::pauseToggled, runControl, &CallgrindRunControl::setPaused);

    connect(m_stopAction, &QAction::triggered, runControl, [runControl] { runControl->stop(); });

    // initialize run control
    runControl->setPaused(m_pauseAction->isChecked());

    // we may want to toggle collect for one function only in this run
    runControl->setToggleCollectFunction(m_toggleCollectFunction);
    m_toggleCollectFunction.clear();

    QTC_ASSERT(m_visualization, return runControl);

    // apply project settings
    if (runConfiguration) {
        if (IRunConfigurationAspect *analyzerAspect = runConfiguration->extraAspect(ANALYZER_VALGRIND_SETTINGS)) {
            if (const ValgrindBaseSettings *settings = qobject_cast<ValgrindBaseSettings *>(analyzerAspect->currentSettings())) {
                m_visualization->setMinimumInclusiveCostRatio(settings->visualisationMinimumInclusiveCostRatio() / 100.0);
                m_proxyModel.setMinimumInclusiveCostRatio(settings->minimumInclusiveCostRatio() / 100.0);
                m_dataModel.setVerboseToolTipsEnabled(settings->enableEventToolTips());
            }
        }
    }

    m_toolBusy = true;
    updateRunActions();

    return runControl;
}

void CallgrindTool::updateRunActions()
{
    if (m_toolBusy) {
        m_startAction->setEnabled(false);
        m_startAction->setToolTip(tr("A Valgrind Callgrind analysis is still in progress."));
        m_stopAction->setEnabled(true);
    } else {
        QString whyNot = tr("Start a Valgrind Callgrind analysis.");
        bool canRun = ProjectExplorerPlugin::canRunStartupProject(CALLGRIND_RUN_MODE, &whyNot);
        m_startAction->setToolTip(whyNot);
        m_startAction->setEnabled(canRun);
        m_stopAction->setEnabled(false);
    }
}
void CallgrindTool::clearTextMarks()
{
    qDeleteAll(m_textMarks);
    m_textMarks.clear();
}

void CallgrindTool::engineStarting()
{
    // enable/disable actions
    m_resetAction->setEnabled(true);
    m_dumpAction->setEnabled(true);
    m_loadExternalLogFile->setEnabled(false);
    clearTextMarks();
    slotClear();
}

void CallgrindTool::engineFinished()
{
    m_toolBusy = false;
    updateRunActions();

    // Enable/disable actions
    m_resetAction->setEnabled(false);
    m_dumpAction->setEnabled(false);
    m_loadExternalLogFile->setEnabled(true);

    const ParseData *data = m_dataModel.parseData();
    if (data)
        showParserResults(data);
    else
        Debugger::showPermanentStatusMessage(tr("Profiling aborted."));

    setBusyCursor(false);
}

void CallgrindTool::showParserResults(const ParseData *data)
{
    QString msg;
    if (data) {
        // be careful, the list of events might be empty
        if (data->events().isEmpty()) {
            msg = tr("Parsing finished, no data.");
        } else {
            const QString costStr = QString::fromLatin1("%1 %2").arg(QString::number(data->totalCost(0)), data->events().first());
            msg = tr("Parsing finished, total cost of %1 reported.").arg(costStr);
        }
    } else {
        msg = tr("Parsing failed.");
    }
    Debugger::showPermanentStatusMessage(msg);
}

void CallgrindTool::editorOpened(IEditor *editor)
{
    if (auto widget = qobject_cast<TextEditorWidget *>(editor->widget())) {
        connect(widget, &TextEditorWidget::markContextMenuRequested,
                this, &CallgrindTool::requestContextMenu);
    }
}

void CallgrindTool::requestContextMenu(TextEditorWidget *widget, int line, QMenu *menu)
{
    // Find callgrind text mark that corresponds to this editor's file and line number
    foreach (CallgrindTextMark *textMark, m_textMarks) {
        if (textMark->fileName() == widget->textDocument()->filePath().toString() && textMark->lineNumber() == line) {
            const Function *func = textMark->function();
            QAction *action = menu->addAction(tr("Select This Function in the Analyzer Output"));
            connect(action, &QAction::triggered, this, [this, func] { selectFunction(func); });
            break;
        }
    }
}

void CallgrindTool::handleShowCostsOfFunction()
{
    CPlusPlus::Symbol *symbol = AnalyzerUtils::findSymbolUnderCursor();
    if (!symbol)
        return;

    if (!symbol->isFunction())
        return;

    CPlusPlus::Overview view;
    const QString qualifiedFunctionName = view.prettyName(CPlusPlus::LookupContext::fullyQualifiedName(symbol));

    m_toggleCollectFunction = qualifiedFunctionName + QLatin1String("()");
    m_startAction->trigger();
}

void CallgrindTool::slotRequestDump()
{
    //setBusy(true);
    m_visualization->setText(tr("Populating..."));
    dumpRequested();
}

void CallgrindTool::loadExternalLogFile()
{
    const QString filePath = QFileDialog::getOpenFileName(
                ICore::mainWindow(),
                tr("Open Callgrind Log File"),
                QString(),
                tr("Callgrind Output (callgrind.out*);;All Files (*)"));
    if (filePath.isEmpty())
        return;

    QFile logFile(filePath);
    if (!logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString msg = tr("Callgrind: Failed to open file for reading: %1").arg(filePath);
        TaskHub::addTask(Task::Error, msg, Debugger::Constants::ANALYZERTASK_ID);
        TaskHub::requestPopup();
        return;
    }

    Debugger::showPermanentStatusMessage(tr("Parsing Profile Data..."));
    QCoreApplication::processEvents();

    Parser parser;
    parser.parse(&logFile);
    takeParserData(parser.takeData());
}

void CallgrindTool::takeParserDataFromRunControl(CallgrindRunControl *rc)
{
    takeParserData(rc->takeParserData());
}

void CallgrindTool::takeParserData(ParseData *data)
{
    showParserResults(data);

    if (!data)
        return;

    // clear first
    clearTextMarks();
    slotClear();

    setParseData(data);
    createTextMarks();
}

void CallgrindTool::createTextMarks()
{
    QList<QString> locations;
    for (int row = 0; row < m_dataModel.rowCount(); ++row) {
        const QModelIndex index = m_dataModel.index(row, DataModel::InclusiveCostColumn);

        QString fileName = index.data(DataModel::FileNameRole).toString();
        if (fileName.isEmpty() || fileName == QLatin1String("???"))
            continue;

        bool ok = false;
        const int lineNumber = index.data(DataModel::LineNumberRole).toInt(&ok);
        QTC_ASSERT(ok, continue);
        // avoid creating invalid text marks
        if (lineNumber <= 0)
            continue;

        // sanitize filename, text marks need a canonical (i.e. no ".."s) path
        // BaseTextMark::editorOpened(Core::IEditor *editor) compares file names on string basis
        QFileInfo info(fileName);
        fileName = info.canonicalFilePath();
        if (fileName.isEmpty())
            continue; // isEmpty == true => file does not exist, continue then

        // create only one text mark per location
        const QString location = QString::fromLatin1("%1:%2").arg(fileName, QString::number(lineNumber));
        if (locations.contains(location))
            continue;
        locations << location;

        m_textMarks.append(new CallgrindTextMark(index, fileName, lineNumber));
    }
}


class CallgrindRunControlFactory : public IRunControlFactory
{
public:
    CallgrindRunControlFactory() : m_tool(new CallgrindTool(this)) {}

    bool canRun(RunConfiguration *runConfiguration, Core::Id runMode) const override
    {
        Q_UNUSED(runConfiguration);
        return runMode == CALLGRIND_RUN_MODE;
    }

    RunControl *create(RunConfiguration *runConfiguration, Core::Id runMode, QString *errorMessage) override
    {
        Q_UNUSED(errorMessage);
        return m_tool->createRunControl(runConfiguration, runMode);
    }

    IRunConfigurationAspect *createRunConfigurationAspect(ProjectExplorer::RunConfiguration *rc) override
    {
        return createValgrindRunConfigurationAspect(rc);
    }

public:
    CallgrindTool *m_tool;
};


static CallgrindRunControlFactory *theCallgrindRunControlFactory;

void initCallgrindTool()
{
    theCallgrindRunControlFactory = new CallgrindRunControlFactory;
    ExtensionSystem::PluginManager::addObject(theCallgrindRunControlFactory);
}

void destroyCallgrindTool()
{
    ExtensionSystem::PluginManager::removeObject(theCallgrindRunControlFactory);
    delete theCallgrindRunControlFactory;
    theCallgrindRunControlFactory = 0;
}

} // namespace Internal
} // namespace Valgrind

#include "callgrindtool.moc"
