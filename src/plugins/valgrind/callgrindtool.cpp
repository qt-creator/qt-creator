// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "callgrindtool.h"

#include "callgrind/callgrindcallmodel.h"
#include "callgrind/callgrinddatamodel.h"
#include "callgrind/callgrindfunction.h"
#include "callgrind/callgrindfunctioncall.h"
#include "callgrind/callgrindparsedata.h"
#include "callgrind/callgrindparser.h"
#include "callgrind/callgrindproxymodel.h"
#include "callgrind/callgrindstackbrowser.h"
#include "callgrindcostdelegate.h"
#include "callgrindcostview.h"
#include "callgrindengine.h"
#include "callgrindtextmark.h"
#include "callgrindvisualisation.h"
#include "valgrindsettings.h"
#include "valgrindtr.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <cplusplus/LookupContext.h>
#include <cplusplus/Overview.h>
#include <cplusplus/Symbols.h>

#include <cppeditor/cppeditorconstants.h>

#include <debugger/debuggerconstants.h>
#include <debugger/debuggermainwindow.h>
#include <debugger/analyzer/analyzerutils.h>
#include <debugger/analyzer/startremotedialog.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/taskhub.h>

#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>

#include <utils/fileutils.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QActionGroup>
#include <QComboBox>
#include <QFile>
#include <QFileInfo>
#include <QLineEdit>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QTimer>
#include <QToolButton>

using namespace Debugger;
using namespace Core;
using namespace Valgrind::Callgrind;
using namespace TextEditor;
using namespace ProjectExplorer;
using namespace Utils;

namespace Valgrind::Internal {

const char CallgrindLocalActionId[]       = "Callgrind.Local.Action";
const char CallgrindRemoteActionId[]      = "Callgrind.Remote.Action";
const char CALLGRIND_RUN_MODE[]           = "CallgrindTool.CallgrindRunMode";

class CallgrindToolRunnerFactory final : public RunWorkerFactory
{
public:
    CallgrindToolRunnerFactory()
    {
        setProduct<CallgrindToolRunner>();
        addSupportedRunMode(CALLGRIND_RUN_MODE);
    }
};

class CallgrindTool final : public QObject
{
    Q_OBJECT

public:
    explicit CallgrindTool(QObject *parent);
    ~CallgrindTool() final;

    void setupRunner(CallgrindToolRunner *runner);

    CostDelegate::CostFormat costFormat() const;

    void doClear();
    void updateEventCombo();

signals:
    void dumpRequested();
    void resetRequested();
    void pauseToggled(bool checked);

public:
    void slotRequestDump();
    void loadExternalLogFile();

    void selectFunction(const Function *);
    void setCostFormat(CostDelegate::CostFormat format);
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

    void stackBrowserChanged();

    /// If \param busy is true, all widgets get a busy cursor when hovered
    void setBusyCursor(bool busy);

    void dataFunctionSelected(const QModelIndex &index);
    void calleeFunctionSelected(const QModelIndex &index);
    void callerFunctionSelected(const QModelIndex &index);
    void visualisationFunctionSelected(const Function *function);
    void showParserResults(const ParseDataPtr &data);

    void setParserData(const ParseDataPtr &data);
    void doSetParseData(const ParseDataPtr &data);
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
    QPointer<CostView> m_flatView;
    QPointer<CostView> m_callersView;
    QPointer<CostView> m_calleesView;
    QPointer<Visualization> m_visualization;

    QString m_lastFileName;

    // Navigation
    QAction *m_goBack = nullptr;
    QAction *m_goNext = nullptr;
    QPointer<QLineEdit> m_searchFilter = nullptr;

    // Cost formatting
    QAction *m_filterProjectCosts = nullptr;
    QAction *m_costAbsolute = nullptr;
    QAction *m_costRelative = nullptr;
    QAction *m_costRelativeToParent = nullptr;
    QPointer<QComboBox> m_eventCombo;

    QTimer m_updateTimer;

    QList<CallgrindTextMark *> m_textMarks;

    QAction *m_startAction = nullptr;
    QAction *m_stopAction = nullptr;
    QAction *m_loadExternalLogFile = nullptr;
    QAction *m_startKCachegrind = nullptr;
    QAction *m_dumpAction = nullptr;
    QAction *m_resetAction = nullptr;
    QAction *m_pauseAction = nullptr;
    QAction *m_discardAction = nullptr;

    QString m_toggleCollectFunction;
    bool m_toolBusy = false;

    Perspective m_perspective{"Callgrind.Perspective", Tr::tr("Callgrind")};

    CallgrindToolRunnerFactory callgrindRunWorkerFactory;
};

CallgrindTool::CallgrindTool(QObject *parent)
    : QObject(parent)
{
    setObjectName("CallgrindTool");

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

    ActionContainer *menu = ActionManager::actionContainer(Debugger::Constants::M_DEBUG_ANALYZER);
    QString toolTip = Tr::tr("Valgrind Function Profiler uses the "
        "Callgrind tool to record function calls when a program runs.");

    if (!Utils::HostOsInfo::isWindowsHost()) {
        m_startAction->setText(Tr::tr("Valgrind Function Profiler"));
        m_startAction->setParent(this);
        m_startAction->setToolTip(toolTip);
        menu->addAction(ActionManager::registerAction(m_startAction, CallgrindLocalActionId),
                        Debugger::Constants::G_ANALYZER_TOOLS);
        QObject::connect(m_startAction, &QAction::triggered, this, [this] {
            if (!Debugger::wantRunTool(OptimizedMode, m_startAction->text()))
                return;
            m_perspective.select();
            ProjectExplorerPlugin::runStartupProject(CALLGRIND_RUN_MODE);
        });
    }

    auto action = new QAction(Tr::tr("Valgrind Function Profiler (External Application)"), this);
    action->setToolTip(toolTip);
    menu->addAction(ActionManager::registerAction(action, CallgrindRemoteActionId),
                    Debugger::Constants::G_ANALYZER_REMOTE_TOOLS);
    QObject::connect(action, &QAction::triggered, this, [this, action] {
        auto runConfig = ProjectManager::startupRunConfiguration();
        if (!runConfig) {
            showCannotStartDialog(action->text());
            return;
        }
        StartRemoteDialog dlg;
        if (dlg.exec() != QDialog::Accepted)
            return;
        m_perspective.select();
        auto runControl = new RunControl(CALLGRIND_RUN_MODE);
        runControl->copyDataFromRunConfiguration(runConfig);
        runControl->createMainWorker();
        runControl->setCommandLine(dlg.commandLine());
        runControl->setWorkingDirectory(dlg.workingDirectory());
        runControl->start();
    });

    // If there is a CppEditor context menu add our own context menu actions.
    if (ActionContainer *editorContextMenu =
            ActionManager::actionContainer(CppEditor::Constants::M_CONTEXT)) {
        Context analyzerContext = Context(Debugger::Constants::C_DEBUGMODE);
        editorContextMenu->addSeparator(analyzerContext);

        auto action = new QAction(Tr::tr("Profile Costs of This Function and Its Callees"), this);
        action->setIcon(ProjectExplorer::Icons::ANALYZER_START_SMALL.icon());
        connect(action, &QAction::triggered, this,
                &CallgrindTool::handleShowCostsOfFunction);
        Command *cmd = ActionManager::registerAction(action, "Analyzer.Callgrind.ShowCostsOfFunction",
            analyzerContext);
        editorContextMenu->addAction(cmd);
        cmd->setAttribute(Command::CA_Hide);
        cmd->setAttribute(Command::CA_NonConfigurable);
    }

    QtcSettings *coreSettings = ICore::settings();

    //
    // DockWidgets
    //
    m_visualization = new Visualization;
    m_visualization->setFrameStyle(QFrame::NoFrame);
    m_visualization->setObjectName("Valgrind.CallgrindTool.Visualisation");
    m_visualization->setWindowTitle(Tr::tr("Visualization"));
    m_visualization->setModel(&m_dataModel);
    connect(m_visualization, &Visualization::functionActivated,
            this, &CallgrindTool::visualisationFunctionSelected);

    m_callersView = new CostView;
    m_callersView->setObjectName("Valgrind.CallgrindTool.CallersView");
    m_callersView->setWindowTitle(Tr::tr("Callers"));
    m_callersView->setSettings(coreSettings, "Valgrind.CallgrindTool.CallersView");
    m_callersView->sortByColumn(CallModel::CostColumn, Qt::DescendingOrder);
    m_callersView->setFrameStyle(QFrame::NoFrame);
    // enable sorting
    m_callersProxy.setSourceModel(&m_callersModel);
    m_callersView->setModel(&m_callersProxy);
    m_callersView->hideColumn(CallModel::CalleeColumn);
    connect(m_callersView, &QAbstractItemView::activated,
            this, &CallgrindTool::callerFunctionSelected);

    m_calleesView = new CostView;
    m_calleesView->setObjectName("Valgrind.CallgrindTool.CalleesView");
    m_calleesView->setWindowTitle(Tr::tr("Callees"));
    m_calleesView->setSettings(coreSettings, "Valgrind.CallgrindTool.CalleesView");
    m_calleesView->sortByColumn(CallModel::CostColumn, Qt::DescendingOrder);
    m_calleesView->setFrameStyle(QFrame::NoFrame);
    // enable sorting
    m_calleesProxy.setSourceModel(&m_calleesModel);
    m_calleesView->setModel(&m_calleesProxy);
    m_calleesView->hideColumn(CallModel::CallerColumn);
    connect(m_calleesView, &QAbstractItemView::activated,
            this, &CallgrindTool::calleeFunctionSelected);

    m_flatView = new CostView;
    m_flatView->setObjectName("Valgrind.CallgrindTool.FlatView");
    m_flatView->setWindowTitle(Tr::tr("Functions"));
    m_flatView->setSettings(coreSettings, "Valgrind.CallgrindTool.FlatView");
    m_flatView->sortByColumn(DataModel::SelfCostColumn, Qt::DescendingOrder);
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
    action = m_loadExternalLogFile = new QAction(this);
    action->setIcon(Utils::Icons::OPENFILE_TOOLBAR.icon());
    action->setToolTip(Tr::tr("Load External Log File"));
    connect(action, &QAction::triggered, this, &CallgrindTool::loadExternalLogFile);

    action = m_startKCachegrind = new QAction(this);
    action->setEnabled(false);
    const Utils::Icon kCachegrindIcon({{":/valgrind/images/kcachegrind.png",
                                        Theme::IconsBaseColor}});
    action->setIcon(kCachegrindIcon.icon());
    action->setToolTip(Tr::tr("Open results in KCachegrind."));
    connect(action, &QAction::triggered, this, [this] {
        Process::startDetached({globalSettings().kcachegrindExecutable(), { m_lastFileName }});
    });

    // dump action
    m_dumpAction = action = new QAction(this);
    action->setDisabled(true);
    action->setIcon(Utils::Icons::REDO.icon());
    //action->setText(Tr::tr("Dump"));
    action->setToolTip(Tr::tr("Request the dumping of profile information. This will update the Callgrind visualization."));
    connect(action, &QAction::triggered, this, &CallgrindTool::slotRequestDump);

    // reset action
    m_resetAction = action = new QAction(this);
    action->setDisabled(true);
    action->setIcon(Utils::Icons::RELOAD_TOOLBAR.icon());
    //action->setText(Tr::tr("Reset"));
    action->setToolTip(Tr::tr("Reset all event counters."));
    connect(action, &QAction::triggered, this, &CallgrindTool::resetRequested);

    // pause action
    m_pauseAction = action = new QAction(this);
    action->setCheckable(true);
    action->setIcon(Utils::Icons::INTERRUPT_SMALL_TOOLBAR.icon());
    //action->setText(Tr::tr("Ignore"));
    action->setToolTip(Tr::tr("Pause event logging. No events are counted which will speed up program execution during profiling."));
    connect(action, &QAction::toggled, this, &CallgrindTool::pauseToggled);

    // discard data action
    m_discardAction = action = new QAction(this);
    action->setIcon(Utils::Icons::CLEAN_TOOLBAR.icon());
    action->setToolTip(Tr::tr("Discard Data"));
    connect(action, &QAction::triggered, this, [this](bool) {
        clearTextMarks();
        doClear();
    });

    // navigation
    // go back
    m_goBack = action = new QAction(this);
    action->setDisabled(true);
    action->setIcon(Utils::Icons::PREV_TOOLBAR.icon());
    action->setToolTip(Tr::tr("Go back one step in history. This will select the previously selected item."));
    connect(action, &QAction::triggered, &m_stackBrowser, &StackBrowser::goBack);

    // go forward
    m_goNext = action = new QAction(this);
    action->setDisabled(true);
    action->setIcon(Utils::Icons::NEXT_TOOLBAR.icon());
    action->setToolTip(Tr::tr("Go forward one step in history."));
    connect(action, &QAction::triggered, &m_stackBrowser, &StackBrowser::goNext);

    // event selection
    m_eventCombo = new QComboBox;
    m_eventCombo->setToolTip(Tr::tr("Selects which events from the profiling data are shown and visualized."));
    connect(m_eventCombo, &QComboBox::currentIndexChanged,
            this, &CallgrindTool::setCostEvent);
    updateEventCombo();

    m_perspective.addToolBarAction(m_startAction);
    m_perspective.addToolBarAction(m_stopAction);
    m_perspective.addToolBarAction(m_loadExternalLogFile);
    m_perspective.addToolBarAction(m_startKCachegrind);
    m_perspective.addToolBarAction(m_dumpAction);
    m_perspective.addToolBarAction(m_resetAction);
    m_perspective.addToolBarAction(m_pauseAction);
    m_perspective.addToolBarAction(m_discardAction);
    m_perspective.addToolBarAction(m_goBack);
    m_perspective.addToolBarAction(m_goNext);
    m_perspective.addToolbarSeparator();
    m_perspective.addToolBarWidget(m_eventCombo);
    m_perspective.registerNextPrevShortcuts(m_goNext, m_goBack);

    // Cost formatting
    {
    auto group = new QActionGroup(this);

    // Show costs as absolute numbers
    m_costAbsolute = new QAction(Tr::tr("Absolute Costs"), this);
    m_costAbsolute->setToolTip(Tr::tr("Show costs as absolute numbers."));
    m_costAbsolute->setCheckable(true);
    m_costAbsolute->setChecked(true);
    connect(m_costAbsolute, &QAction::toggled, this, &CallgrindTool::updateCostFormat);
    group->addAction(m_costAbsolute);

    // Show costs in percentages
    m_costRelative = new QAction(Tr::tr("Relative Costs"), this);
    m_costRelative->setToolTip(Tr::tr("Show costs relative to total inclusive cost."));
    m_costRelative->setCheckable(true);
    connect(m_costRelative, &QAction::toggled, this, &CallgrindTool::updateCostFormat);
    group->addAction(m_costRelative);

    // Show costs relative to parent
    m_costRelativeToParent = new QAction(Tr::tr("Relative Costs to Parent"), this);
    m_costRelativeToParent->setToolTip(Tr::tr("Show costs relative to parent function's inclusive cost."));
    m_costRelativeToParent->setCheckable(true);
    connect(m_costRelativeToParent, &QAction::toggled, this, &CallgrindTool::updateCostFormat);
    group->addAction(m_costRelativeToParent);

    auto button = new QToolButton;
    button->addActions(group->actions());
    button->setPopupMode(QToolButton::InstantPopup);
    button->setText("$");
    button->setToolTip(Tr::tr("Cost Format"));
    m_perspective.addToolBarWidget(button);
    }

    // Filtering
    action = m_filterProjectCosts = globalSettings().filterExternalIssues.action();
    connect(action, &QAction::toggled, this, &CallgrindTool::handleFilterProjectCosts);

    // Filter
    m_searchFilter = new QLineEdit;
    m_searchFilter->setPlaceholderText(Tr::tr("Filter..."));
    connect(m_searchFilter, &QLineEdit::textChanged,
            &m_updateTimer, QOverload<>::of(&QTimer::start));

    setCostFormat(CostDelegate::CostFormat(globalSettings().costFormat()));

    m_perspective.addToolBarAction(globalSettings().detectCycles.action());
    m_perspective.addToolBarAction(globalSettings().shortenTemplates.action());
    m_perspective.addToolBarAction(m_filterProjectCosts);
    m_perspective.addToolBarWidget(m_searchFilter);

    m_perspective.addWindow(m_flatView, Perspective::SplitVertical, nullptr);
    m_perspective.addWindow(m_calleesView, Perspective::SplitVertical, nullptr);
    m_perspective.addWindow(m_callersView, Perspective::SplitHorizontal, m_calleesView);
    m_perspective.addWindow(m_visualization, Perspective::SplitVertical, nullptr,
                           false, Qt::RightDockWidgetArea);

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::runActionsUpdated,
            this, &CallgrindTool::updateRunActions);
}

CallgrindTool::~CallgrindTool()
{
    qDeleteAll(m_textMarks);
    delete m_flatView;
    delete m_callersView;
    delete m_calleesView;
    delete m_visualization;
}

void CallgrindTool::doClear()
{
    doSetParseData({});

    // clear filters
    if (m_filterProjectCosts)
        m_filterProjectCosts->setChecked(false);
    m_proxyModel.setFilterBaseDir(QString());
    if (m_searchFilter)
        m_searchFilter->clear();
    m_proxyModel.setFilterRegularExpression(QRegularExpression());
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
        if (m_flatView)
            m_flatView->clearSelection();
        if (m_visualization)
            m_visualization->setFunction(nullptr);
        m_callersModel.clear();
        m_calleesModel.clear();
        return;
    }

    const QModelIndex index = m_dataModel.indexForObject(func);
    const QModelIndex proxyIndex = m_proxyModel.mapFromSource(index);
    if (m_flatView) {
        m_flatView->selectionModel()->clearSelection();
        m_flatView->selectionModel()->setCurrentIndex(proxyIndex,
                                                      QItemSelectionModel::ClearAndSelect |
                                                      QItemSelectionModel::Rows);
        m_flatView->scrollTo(proxyIndex);
    }

    m_callersModel.setCalls(func->incomingCalls(), func);
    m_calleesModel.setCalls(func->outgoingCalls(), func);
    if (m_visualization)
        m_visualization->setFunction(func);

    const Function *item = m_stackBrowser.current();
    if (!item || item != func)
        m_stackBrowser.select(func);

    const auto filePath = FilePath::fromString(func->file());
    if (filePath.exists()) {
        ///TODO: custom position support?
        int line = func->lineNumber();
        EditorManager::openEditorAt({filePath, qMax(line, 0)});
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
    m_proxyModel.setFilterRegularExpression(QRegularExpression::escape(m_searchFilter->text()));
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
    globalSettings().costFormat.setValue(format);
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
    auto func = index.data(DataModel::FunctionRole).value<const Function *>();
    QTC_ASSERT(func, return);

    selectFunction(func);
}

void CallgrindTool::calleeFunctionSelected(const QModelIndex &index)
{
    auto call = index.data(CallModel::FunctionCallRole).value<const FunctionCall *>();
    QTC_ASSERT(call, return);

    selectFunction(call->callee());
}

void CallgrindTool::callerFunctionSelected(const QModelIndex &index)
{
    auto call = index.data(CallModel::FunctionCallRole).value<const FunctionCall *>();
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

void CallgrindTool::doSetParseData(const ParseDataPtr &data)
{
    // we have new parse data, invalidate filters in the proxy model
    if (m_visualization)
        m_visualization->setFunction(nullptr);

    // might happen if the user cancelled the profile run
    // callgrind then sometimes produces empty callgrind.out.PID files
    const ParseDataPtr newData = data && !data->events().isEmpty() ? data : ParseDataPtr();

    m_lastFileName = newData ? newData->fileName() : QString();
    m_dataModel.setParseData(newData);
    m_calleesModel.setParseData(newData);
    m_callersModel.setParseData(newData);

    if (m_eventCombo)
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

    const ParseDataPtr data = m_dataModel.parseData();
    if (!data || data->events().isEmpty()) {
        m_eventCombo->hide();
        return;
    }

    m_eventCombo->show();
    const QStringList events = data->events();
    for (const QString &event : events)
        m_eventCombo->addItem(ParseData::prettyStringForEvent(event));
}

void CallgrindTool::setupRunner(CallgrindToolRunner *toolRunner)
{
    RunControl *runControl = toolRunner->runControl();

    connect(toolRunner, &CallgrindToolRunner::parserDataReady, this, &CallgrindTool::setParserData);
    connect(runControl, &RunControl::stopped, this, &CallgrindTool::engineFinished);

    connect(this, &CallgrindTool::dumpRequested, toolRunner, &CallgrindToolRunner::dump);
    connect(this, &CallgrindTool::resetRequested, toolRunner, &CallgrindToolRunner::reset);
    connect(this, &CallgrindTool::pauseToggled, toolRunner, &CallgrindToolRunner::setPaused);

    connect(m_stopAction, &QAction::triggered, toolRunner, [runControl] { runControl->initiateStop(); });

    // initialize run control
    toolRunner->setPaused(m_pauseAction->isChecked());

    // we may want to toggle collect for one function only in this run
    toolRunner->setToggleCollectFunction(m_toggleCollectFunction);
    m_toggleCollectFunction.clear();

    QTC_ASSERT(m_visualization, return);

    // apply project settings
    ValgrindSettings settings{false};
    settings.fromMap(runControl->settingsData(ANALYZER_VALGRIND_SETTINGS));
    m_visualization->setMinimumInclusiveCostRatio(settings.visualizationMinimumInclusiveCostRatio() / 100.0);
    m_proxyModel.setMinimumInclusiveCostRatio(settings.minimumInclusiveCostRatio() / 100.0);
    m_dataModel.setVerboseToolTipsEnabled(settings.enableEventToolTips());

    m_toolBusy = true;
    updateRunActions();

    // enable/disable actions
    m_resetAction->setEnabled(true);
    m_dumpAction->setEnabled(true);
    m_loadExternalLogFile->setEnabled(false);
    clearTextMarks();
    doClear();
}

void CallgrindTool::updateRunActions()
{
    if (m_toolBusy) {
        m_startAction->setEnabled(false);
        m_startKCachegrind->setEnabled(false);
        m_startAction->setToolTip(Tr::tr("A Valgrind Callgrind analysis is still in progress."));
        m_stopAction->setEnabled(true);
    } else {
        const auto canRun = ProjectExplorerPlugin::canRunStartupProject(CALLGRIND_RUN_MODE);
        m_startAction->setToolTip(canRun ? Tr::tr("Start a Valgrind Callgrind analysis.")
                                         : canRun.error());
        m_startAction->setEnabled(canRun);
        m_stopAction->setEnabled(false);
    }
}
void CallgrindTool::clearTextMarks()
{
    qDeleteAll(m_textMarks);
    m_textMarks.clear();
}

void CallgrindTool::engineFinished()
{
    if (m_flatView == nullptr) // Happens on shutdown when memcheck is still running.
        return;

    m_toolBusy = false;
    updateRunActions();

    // Enable/disable actions
    m_resetAction->setEnabled(false);
    m_dumpAction->setEnabled(false);
    m_loadExternalLogFile->setEnabled(true);

    const ParseDataPtr data = m_dataModel.parseData();
    if (data)
        showParserResults(data);
    else
        Debugger::showPermanentStatusMessage(Tr::tr("Profiling aborted."));

    setBusyCursor(false);
}

void CallgrindTool::showParserResults(const ParseDataPtr &data)
{
    QString msg;
    if (data) {
        // be careful, the list of events might be empty
        if (data->events().isEmpty()) {
            msg = Tr::tr("Parsing finished, no data.");
        } else {
            const QString costStr = QString::fromLatin1("%1 %2")
                    .arg(QString::number(data->totalCost(0)), data->events().constFirst());
            msg = Tr::tr("Parsing finished, total cost of %1 reported.").arg(costStr);
        }
    } else {
        msg = Tr::tr("Parsing failed.");
    }
    Debugger::showPermanentStatusMessage(msg);
}

void CallgrindTool::editorOpened(IEditor *editor)
{
    if (auto widget = TextEditorWidget::fromEditor(editor)) {
        connect(widget, &TextEditorWidget::markContextMenuRequested,
                this, &CallgrindTool::requestContextMenu);
    }
}

void CallgrindTool::requestContextMenu(TextEditorWidget *widget, int line, QMenu *menu)
{
    // Find callgrind text mark that corresponds to this editor's file and line number
    for (CallgrindTextMark *textMark : std::as_const(m_textMarks)) {
        if (textMark->filePath() == widget->textDocument()->filePath() && textMark->lineNumber() == line) {
            const Function *func = textMark->function();
            QAction *action = menu->addAction(Tr::tr("Select This Function in the Analyzer Output"));
            connect(action, &QAction::triggered, this, [this, func] { selectFunction(func); });
            break;
        }
    }
}

void CallgrindTool::handleShowCostsOfFunction()
{
    CPlusPlus::Symbol *symbol = Debugger::findSymbolUnderCursor();
    if (!symbol)
        return;

    if (!symbol->asFunction() && !symbol->type()->asFunctionType())
        return;

    CPlusPlus::Overview view;
    const QString qualifiedFunctionName = view.prettyName(CPlusPlus::LookupContext::fullyQualifiedName(symbol));

    m_toggleCollectFunction = qualifiedFunctionName + "()";
    m_startAction->trigger();
}

void CallgrindTool::slotRequestDump()
{
    //setBusy(true);
    m_visualization->setText(Tr::tr("Populating..."));
    emit dumpRequested();
}

void CallgrindTool::loadExternalLogFile()
{
    const FilePath filePath = FileUtils::getOpenFilePath(
                Tr::tr("Open Callgrind Log File"),
                {},
                Tr::tr("Callgrind Output (callgrind.out*);;All Files (*)"));
    if (filePath.isEmpty())
        return;

    QFile logFile(filePath.toString());
    if (!logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString msg = Tr::tr("Callgrind: Failed to open file for reading: %1")
                .arg(filePath.toUserOutput());
        TaskHub::addTask(Task::Error, msg, Debugger::Constants::ANALYZERTASK_ID);
        TaskHub::requestPopup();
        return;
    }

    Debugger::showPermanentStatusMessage(Tr::tr("Parsing Profile Data..."));
    QCoreApplication::processEvents();

    setParserData(parseDataFile(filePath));
}

void CallgrindTool::setParserData(const ParseDataPtr &data)
{
    showParserResults(data);

    if (!data)
        return;

    // clear first
    clearTextMarks();
    doClear();
    doSetParseData(data);

    const FilePath kcachegrindExecutable = globalSettings().kcachegrindExecutable();
    const FilePath found = kcachegrindExecutable.searchInPath();
    const bool kcachegrindExists = found.isExecutableFile();
    m_startKCachegrind->setEnabled(kcachegrindExists && !m_lastFileName.isEmpty());
    createTextMarks();
}

void CallgrindTool::createTextMarks()
{
    QList<QString> locations;
    for (int row = 0; row < m_dataModel.rowCount(); ++row) {
        const QModelIndex index = m_dataModel.index(row, DataModel::InclusiveCostColumn);

        QString fileName = index.data(DataModel::FileNameRole).toString();
        if (fileName.isEmpty() || fileName == "???")
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

        m_textMarks.append(new CallgrindTextMark(index, FilePath::fromString(fileName), lineNumber));
    }
}


// CallgrindTool

static CallgrindTool *dd = nullptr;

void setupCallgrindRunner(CallgrindToolRunner *toolRunner)
{
    dd->setupRunner(toolRunner);
}

void setupCallgrindTool(QObject *guard)
{
    dd = new CallgrindTool(guard);
}

} // Valgrind::Internal

#include "callgrindtool.moc"
