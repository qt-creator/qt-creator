/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
#include <valgrind/callgrind/callgrindproxymodel.h>
#include <valgrind/callgrind/callgrindstackbrowser.h>
#include <valgrind/valgrindplugin.h>
#include <valgrind/valgrindsettings.h>

#include <analyzerbase/analyzermanager.h>
#include <analyzerbase/analyzersettings.h>
#include <analyzerbase/analyzerutils.h>
#include <analyzerbase/analyzerconstants.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

#include <cplusplus/LookupContext.h>
#include <cplusplus/Overview.h>
#include <cppeditor/cppeditorconstants.h>
#include <extensionsystem/iplugin.h>
#include <texteditor/basetexteditor.h>

#include <utils/qtcassert.h>
#include <utils/fancymainwindow.h>
#include <utils/styledbar.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>

#include <QFile>
#include <QFileInfo>
#include <QTimer>

#include <QAction>
#include <QActionGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QGraphicsItem>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

// shared/cplusplus includes
#include <Symbols.h>

using namespace Analyzer;
using namespace Core;
using namespace Valgrind::Callgrind;

namespace Valgrind {
namespace Internal {

class CallgrindToolPrivate : public QObject
{
    Q_OBJECT

public:
    friend class CallgrindTool;

    explicit CallgrindToolPrivate(CallgrindTool *parent);
    virtual ~CallgrindToolPrivate();

    void setParseData(ParseData *data);

    CostDelegate::CostFormat costFormat() const;
    QWidget *createWidgets();

    void doClear(bool clearParseData);
    void updateEventCombo();

    IAnalyzerEngine *createEngine(const AnalyzerStartParameters &sp,
        ProjectExplorer::RunConfiguration *runConfiguration = 0);

signals:
    void cycleDetectionEnabled(bool enabled);
    void dumpRequested();
    void resetRequested();
    void pauseToggled(bool checked);

public slots:
    void slotClear();
    void slotRequestDump();

    void selectFunction(const Valgrind::Callgrind::Function *);
    void setCostFormat(Valgrind::Internal::CostDelegate::CostFormat format);
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
    void handleShowCostsAction();
    void handleShowCostsOfFunction();

    void slotGoToOverview();
    void stackBrowserChanged();

    /// If \param busy is true, all widgets get a busy cursor when hovered
    void setBusyCursor(bool busy);

    void dataFunctionSelected(const QModelIndex &index);
    void calleeFunctionSelected(const QModelIndex &index);
    void callerFunctionSelected(const QModelIndex &index);
    void visualisationFunctionSelected(const Valgrind::Callgrind::Function *function);
    void showParserResults(const Valgrind::Callgrind::ParseData *data);

    void takeParserData(CallgrindEngine *engine);
    void engineStarting(const Analyzer::IAnalyzerEngine *);
    void engineFinished();

    void editorOpened(Core::IEditor *);
    void requestContextMenu(TextEditor::ITextEditor *editor, int line, QMenu *menu);

public:
    CallgrindTool *q;

    DataModel *m_dataModel;
    DataProxyModel *m_proxyModel;
    StackBrowser *m_stackBrowser;

    CallModel *m_callersModel;
    CallModel *m_calleesModel;

    // callgrind widgets
    CostView *m_flatView;
    CostView *m_callersView;
    CostView *m_calleesView;
    Visualisation *m_visualisation;

    // navigation
    QAction *m_goBack;
    QAction *m_goNext;
    QLineEdit *m_searchFilter;

    // cost formatting
    QAction *m_filterProjectCosts;
    QAction *m_costAbsolute;
    QAction *m_costRelative;
    QAction *m_costRelativeToParent;
    QAction *m_cycleDetection;
    QAction *m_shortenTemplates;
    QComboBox *m_eventCombo;

    QTimer *m_updateTimer;

    QVector<CallgrindTextMark *> m_textMarks;

    QAction *m_dumpAction;
    QAction *m_resetAction;
    QAction *m_pauseAction;
    QAction *m_showCostsOfFunctionAction;

    QString m_toggleCollectFunction;
    ValgrindGlobalSettings *m_settings; // Not owned
};


CallgrindToolPrivate::CallgrindToolPrivate(CallgrindTool *parent)
    : q(parent)
    , m_dataModel(new DataModel(this))
    , m_proxyModel(new DataProxyModel(this))
    , m_stackBrowser(new StackBrowser(this))
    , m_callersModel(new CallModel(this))
    , m_calleesModel(new CallModel(this))
    , m_flatView(0)
    , m_callersView(0)
    , m_calleesView(0)
    , m_visualisation(0)
    , m_goBack(0)
    , m_goNext(0)
    , m_searchFilter(0)
    , m_filterProjectCosts(0)
    , m_costAbsolute(0)
    , m_costRelative(0)
    , m_costRelativeToParent(0)
    , m_eventCombo(0)
    , m_updateTimer(new QTimer(this))
    , m_dumpAction(0)
    , m_resetAction(0)
    , m_pauseAction(0)
    , m_showCostsOfFunctionAction(0)
    , m_settings(0)
{
    m_updateTimer->setInterval(200);
    m_updateTimer->setSingleShot(true);

    m_proxyModel->setSourceModel(m_dataModel);
    m_proxyModel->setDynamicSortFilter(true);
    m_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setFilterKeyColumn(DataModel::NameColumn);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    m_settings = AnalyzerGlobalSettings::instance()->subConfig<ValgrindGlobalSettings>();

    connect(m_stackBrowser, SIGNAL(currentChanged()), SLOT(stackBrowserChanged()));
    connect(m_updateTimer, SIGNAL(timeout()), SLOT(updateFilterString()));
}

CallgrindToolPrivate::~CallgrindToolPrivate()
{
    qDeleteAll(m_textMarks);
    doClear(false);
}

void CallgrindToolPrivate::slotGoToOverview()
{
    selectFunction(0);
}

void CallgrindToolPrivate::slotClear()
{
    doClear(true);
}

void CallgrindToolPrivate::doClear(bool clearParseData)
{
    if (clearParseData) // Crashed when done from destructor.
        setParseData(0);

    // clear filters
    if (m_filterProjectCosts)
        m_filterProjectCosts->setChecked(false);
    m_proxyModel->setFilterBaseDir(QString());
    if (m_searchFilter)
        m_searchFilter->setText(QString());
    m_proxyModel->setFilterFixedString(QString());
}

void CallgrindToolPrivate::setBusyCursor(bool busy)
{
    QCursor cursor(busy ? Qt::BusyCursor : Qt::ArrowCursor);
    m_flatView->setCursor(cursor);
    m_calleesView->setCursor(cursor);
    m_callersView->setCursor(cursor);
    m_visualisation->setCursor(cursor);
}

void CallgrindToolPrivate::selectFunction(const Function *func)
{
    if (!func) {
        m_flatView->clearSelection();
        m_visualisation->setFunction(0);
        m_callersModel->clear();
        m_calleesModel->clear();
        return;
    }

    const QModelIndex index = m_dataModel->indexForObject(func);
    const QModelIndex proxyIndex = m_proxyModel->mapFromSource(index);
    m_flatView->selectionModel()->clearSelection();
    m_flatView->selectionModel()->setCurrentIndex(proxyIndex,
                                                  QItemSelectionModel::ClearAndSelect |
                                                  QItemSelectionModel::Rows);
    m_flatView->scrollTo(proxyIndex);

    m_callersModel->setCalls(func->incomingCalls(), func);
    m_calleesModel->setCalls(func->outgoingCalls(), func);
    m_visualisation->setFunction(func);

    const Function *item = m_stackBrowser->current();
    if (!item || item != func)
        m_stackBrowser->select(func);

    if (QFile::exists(func->file())) {
        ///TODO: custom position support?
        int line = func->lineNumber();
        TextEditor::BaseTextEditorWidget::openEditorAt(func->file(), qMax(line, 0));
    }
}

void CallgrindToolPrivate::stackBrowserChanged()
{
    m_goBack->setEnabled(m_stackBrowser->hasPrevious());
    m_goNext->setEnabled(m_stackBrowser->hasNext());
    const Function *item = m_stackBrowser->current();
    selectFunction(item);
}

void CallgrindToolPrivate::updateFilterString()
{
    m_proxyModel->setFilterFixedString(m_searchFilter->text());
}

void CallgrindToolPrivate::setCostFormat(CostDelegate::CostFormat format)
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

void CallgrindToolPrivate::setCostEvent(int index)
{
    // prevent assert in model, don't try to set event to -1
    // happens when we clear the eventcombo
    if (index == -1)
        index = 0;

    m_dataModel->setCostEvent(index);
    m_calleesModel->setCostEvent(index);
    m_callersModel->setCostEvent(index);
}

void CallgrindToolPrivate::enableCycleDetection(bool enabled)
{
    m_cycleDetection->setChecked(enabled);
}

void CallgrindToolPrivate::shortenTemplates(bool enabled)
{
    m_shortenTemplates->setChecked(enabled);
}

// Following functions can be called with actions=0 or widgets=0
// depending on initialization sequence (whether callgrind was current).
CostDelegate::CostFormat CallgrindToolPrivate::costFormat() const
{
    if (m_costRelativeToParent && m_costRelativeToParent->isChecked())
        return CostDelegate::FormatRelativeToParent;
    if (m_costRelative && m_costRelative->isChecked())
        return CostDelegate::FormatRelative;
    return CostDelegate::FormatAbsolute;
}

void CallgrindToolPrivate::updateCostFormat()
{
    const CostDelegate::CostFormat format = costFormat();
    if (m_flatView)
        m_flatView->setCostFormat(format);
    if (m_calleesView) {
        m_calleesView->setCostFormat(format);
        m_callersView->setCostFormat(format);
    }
    if (m_settings)
        m_settings->setCostFormat(format);
}

void CallgrindToolPrivate::handleFilterProjectCosts()
{
    ProjectExplorer::Project *pro = ProjectExplorer::ProjectExplorerPlugin::currentProject();
    QTC_ASSERT(pro, return);

    if (m_filterProjectCosts->isChecked()) {
        const QString projectDir = pro->projectDirectory();
        m_proxyModel->setFilterBaseDir(projectDir);
    }
    else {
        m_proxyModel->setFilterBaseDir(QString());
    }
}

void CallgrindToolPrivate::dataFunctionSelected(const QModelIndex &index)
{
    const Function *func = index.data(DataModel::FunctionRole).value<const Function *>();
    QTC_ASSERT(func, return);

    selectFunction(func);
}

void CallgrindToolPrivate::calleeFunctionSelected(const QModelIndex &index)
{
    const FunctionCall *call = index.data(CallModel::FunctionCallRole).value<const FunctionCall *>();
    QTC_ASSERT(call, return);

    selectFunction(call->callee());
}

void CallgrindToolPrivate::callerFunctionSelected(const QModelIndex &index)
{
    const FunctionCall *call = index.data(CallModel::FunctionCallRole).value<const FunctionCall *>();
    QTC_ASSERT(call, return);

    selectFunction(call->caller());
}

void CallgrindToolPrivate::visualisationFunctionSelected(const Function *function)
{
    if (function && function == m_visualisation->function())
        // up-navigation when the initial function was activated
        m_stackBrowser->goBack();
    else
        selectFunction(function);
}

void CallgrindToolPrivate::setParseData(ParseData *data)
{
    // we have new parse data, invalidate filters in the proxy model
    m_visualisation->setFunction(0);

    // invalidate parse data in the data model
    delete m_dataModel->parseData();

    if (data && data->events().isEmpty()) {
        // might happen if the user cancelled the profile run
        // callgrind then sometimes produces empty callgrind.out.PID files
        delete data;
        data = 0;
    }
    m_dataModel->setParseData(data);
    m_calleesModel->setParseData(data);
    m_callersModel->setParseData(data);

    updateEventCombo();

    // clear history for new data
    m_stackBrowser->clear();

    // unset busy state
    //setBusy(false);
}

void CallgrindToolPrivate::updateEventCombo()
{
    QTC_ASSERT(m_eventCombo, return);

    m_eventCombo->clear();

    const ParseData *data = m_dataModel->parseData();
    if (!data || data->events().isEmpty()) {
        m_eventCombo->hide();
        return;
    }

    m_eventCombo->show();
    foreach (const QString &event, data->events())
        m_eventCombo->addItem(ParseData::prettyStringForEvent(event));
}

static QToolButton *createToolButton(QAction *action)
{
    QToolButton *button = new QToolButton;
    button->setDefaultAction(action);
    //button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    return button;
}

CallgrindTool::CallgrindTool(QObject *parent)
    : ValgrindTool(parent)
{
    d = new CallgrindToolPrivate(this);
    setObjectName(QLatin1String("CallgrindTool"));

    connect(Core::ICore::editorManager(), SIGNAL(editorOpened(Core::IEditor*)),
        d, SLOT(editorOpened(Core::IEditor*)));
}

CallgrindTool::~CallgrindTool()
{
    delete d;
}

Core::Id CallgrindTool::id() const
{
    return Core::Id("Callgrind");
}

ProjectExplorer::RunMode CallgrindTool::runMode() const
{
    return ProjectExplorer::CallgrindRunMode;
}

QString CallgrindTool::displayName() const
{
    return tr("Valgrind Function Profiler");
}

QString CallgrindTool::description() const
{
    return tr("Valgrind Profile uses the \"callgrind\" tool to "
              "record function calls when a program runs.");
}

IAnalyzerTool::ToolMode CallgrindTool::toolMode() const
{
    return ReleaseMode;
}

void CallgrindTool::extensionsInitialized()
{
    Core::Context analyzerContext = Core::Context(Analyzer::Constants::C_ANALYZEMODE);

    // check if there is a CppEditor context menu, if true, add our own context menu actions
    if (Core::ActionContainer *editorContextMenu =
            Core::ActionManager::actionContainer(CppEditor::Constants::M_CONTEXT)) {
        QAction *action = 0;
        Core::Command *cmd = 0;

        editorContextMenu->addSeparator(analyzerContext);

        action = new QAction(tr("Profile Costs of this Function and its Callees"), this);
        action->setIcon(QIcon(QLatin1String(Analyzer::Constants::ANALYZER_CONTROL_START_ICON)));
        connect(action, SIGNAL(triggered()), d, SLOT(handleShowCostsOfFunction()));
        cmd = Core::ActionManager::registerAction(action, "Analyzer.Callgrind.ShowCostsOfFunction",
            analyzerContext);
        editorContextMenu->addAction(cmd);
        cmd->setAttribute(Core::Command::CA_Hide);
        cmd->setAttribute(Core::Command::CA_NonConfigurable);
        d->m_showCostsOfFunctionAction = action;
    }
}

IAnalyzerEngine *CallgrindTool::createEngine(const AnalyzerStartParameters &sp,
    ProjectExplorer::RunConfiguration *runConfiguration)
{
    return d->createEngine(sp, runConfiguration);
}

IAnalyzerEngine *CallgrindToolPrivate::createEngine(const AnalyzerStartParameters &sp,
    ProjectExplorer::RunConfiguration *runConfiguration)
{
    CallgrindEngine *engine = new CallgrindEngine(q, sp, runConfiguration);

    connect(engine, SIGNAL(parserDataReady(CallgrindEngine*)),
            SLOT(takeParserData(CallgrindEngine*)));
    connect(engine, SIGNAL(starting(const Analyzer::IAnalyzerEngine*)),
            SLOT(engineStarting(const Analyzer::IAnalyzerEngine*)));
    connect(engine, SIGNAL(finished()),
            SLOT(engineFinished()));

    connect(this, SIGNAL(dumpRequested()), engine, SLOT(dump()));
    connect(this, SIGNAL(resetRequested()), engine, SLOT(reset()));
    connect(this, SIGNAL(pauseToggled(bool)), engine, SLOT(setPaused(bool)));

    // initialize engine
    engine->setPaused(m_pauseAction->isChecked());

    // we may want to toggle collect for one function only in this run
    engine->setToggleCollectFunction(m_toggleCollectFunction);
    m_toggleCollectFunction.clear();

    AnalyzerManager::showStatusMessage(AnalyzerManager::msgToolStarted(q->displayName()));

    QTC_ASSERT(m_visualisation, return engine);

    // apply project settings
    if (runConfiguration) {
        if (const AnalyzerRunConfigurationAspect *analyzerSettings = runConfiguration->extraAspect<AnalyzerRunConfigurationAspect>()) {
            if (const ValgrindProjectSettings *settings = analyzerSettings->subConfig<ValgrindProjectSettings>()) {
                m_visualisation->setMinimumInclusiveCostRatio(settings->visualisationMinimumInclusiveCostRatio() / 100.0);
                m_proxyModel->setMinimumInclusiveCostRatio(settings->minimumInclusiveCostRatio() / 100.0);
                m_dataModel->setVerboseToolTipsEnabled(settings->enableEventToolTips());
            }
        }
    }
    return engine;
}

void CallgrindTool::startTool(StartMode mode)
{
    ValgrindPlugin::startValgrindTool(this, mode);
    d->setBusyCursor(true);
}

QWidget *CallgrindTool::createWidgets()
{
    return d->createWidgets();
}

QWidget *CallgrindToolPrivate::createWidgets()
{
    QTC_ASSERT(!m_visualisation, return 0);

    //
    // DockWidgets
    //
    Utils::FancyMainWindow *mw = AnalyzerManager::mainWindow();
    m_visualisation = new Visualisation(mw);
    m_visualisation->setFrameStyle(QFrame::NoFrame);
    m_visualisation->setObjectName(QLatin1String("Valgrind.CallgrindTool.Visualisation"));
    m_visualisation->setModel(m_dataModel);
    connect(m_visualisation, SIGNAL(functionActivated(const Valgrind::Callgrind::Function*)),
            this, SLOT(visualisationFunctionSelected(const Valgrind::Callgrind::Function*)));

    m_callersView = new CostView(mw);
    m_callersView->setObjectName(QLatin1String("Valgrind.CallgrindTool.CallersView"));
    m_callersView->sortByColumn(CallModel::CostColumn);
    m_callersView->setFrameStyle(QFrame::NoFrame);
    // enable sorting
    QSortFilterProxyModel *callerProxy = new QSortFilterProxyModel(m_callersModel);
    callerProxy->setSourceModel(m_callersModel);
    m_callersView->setModel(callerProxy);
    m_callersView->hideColumn(CallModel::CalleeColumn);
    connect(m_callersView, SIGNAL(activated(QModelIndex)),
            this, SLOT(callerFunctionSelected(QModelIndex)));

    m_calleesView = new CostView(mw);
    m_calleesView->setObjectName(QLatin1String("Valgrind.CallgrindTool.CalleesView"));
    m_calleesView->sortByColumn(CallModel::CostColumn);
    m_calleesView->setFrameStyle(QFrame::NoFrame);
    // enable sorting
    QSortFilterProxyModel *calleeProxy = new QSortFilterProxyModel(m_calleesModel);
    calleeProxy->setSourceModel(m_calleesModel);
    m_calleesView->setModel(calleeProxy);
    m_calleesView->hideColumn(CallModel::CallerColumn);
    connect(m_calleesView, SIGNAL(activated(QModelIndex)),
            this, SLOT(calleeFunctionSelected(QModelIndex)));

    m_flatView = new CostView(mw);
    m_flatView->setObjectName(QLatin1String("Valgrind.CallgrindTool.FlatView"));
    m_flatView->sortByColumn(DataModel::SelfCostColumn);
    m_flatView->setFrameStyle(QFrame::NoFrame);
    m_flatView->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_flatView->setModel(m_proxyModel);
    connect(m_flatView, SIGNAL(activated(QModelIndex)),
            this, SLOT(dataFunctionSelected(QModelIndex)));

    updateCostFormat();

    QDockWidget *callersDock = AnalyzerManager::createDockWidget
        (q, tr("Callers"), m_callersView, Qt::BottomDockWidgetArea);

    QDockWidget *flatDock = AnalyzerManager::createDockWidget
        (q, tr("Functions"), m_flatView, Qt::BottomDockWidgetArea);

    QDockWidget *calleesDock = AnalyzerManager::createDockWidget
        (q, tr("Callees"), m_calleesView, Qt::BottomDockWidgetArea);

    QDockWidget *visualizationDock = AnalyzerManager::createDockWidget
        (q, tr("Visualization"), m_visualisation, Qt::RightDockWidgetArea);

    callersDock->show();
    calleesDock->show();
    flatDock->show();
    visualizationDock->hide();

    mw->splitDockWidget(mw->toolBarDockWidget(), flatDock, Qt::Vertical);
    mw->splitDockWidget(mw->toolBarDockWidget(), calleesDock, Qt::Vertical);
    mw->splitDockWidget(calleesDock, callersDock, Qt::Horizontal);

    //
    // Control Widget
    //
    QAction *action = 0;
    QWidget *widget = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout;

    layout->setMargin(0);
    layout->setSpacing(0);
    widget->setLayout(layout);

    // dump action
    action = new QAction(this);
    action->setDisabled(true);
    action->setIcon(QIcon(QLatin1String(Core::Constants::ICON_REDO)));
    //action->setText(tr("Dump"));
    action->setToolTip(tr("Request the dumping of profile information. This will update the callgrind visualization."));
    connect(action, SIGNAL(triggered()), this, SLOT(slotRequestDump()));
    layout->addWidget(createToolButton(action));
    m_dumpAction = action;

    // reset action
    action = new QAction(this);
    action->setDisabled(true);
    action->setIcon(QIcon(QLatin1String(Core::Constants::ICON_CLEAR)));
    //action->setText(tr("Reset"));
    action->setToolTip(tr("Reset all event counters."));
    connect(action, SIGNAL(triggered()), this, SIGNAL(resetRequested()));
    layout->addWidget(createToolButton(action));
    m_resetAction = action;

    // pause action
    action = new QAction(this);
    action->setCheckable(true);
    action->setIcon(QIcon(QLatin1String(":/qml/images/pause-small.png")));
    //action->setText(tr("Ignore"));
    action->setToolTip(tr("Pause event logging. No events are counted which will speed up program execution during profiling."));
    connect(action, SIGNAL(toggled(bool)), this, SIGNAL(pauseToggled(bool)));
    layout->addWidget(createToolButton(action));
    m_pauseAction = action;

    // navigation
    // go back
    action = new QAction(this);
    action->setDisabled(true);
    action->setIcon(QIcon(QLatin1String(Core::Constants::ICON_PREV)));
    action->setToolTip(tr("Go back one step in history. This will select the previously selected item."));
    connect(action, SIGNAL(triggered(bool)), m_stackBrowser, SLOT(goBack()));
    layout->addWidget(createToolButton(action));
    m_goBack = action;

    // go forward
    action = new QAction(this);
    action->setDisabled(true);
    action->setIcon(QIcon(QLatin1String(Core::Constants::ICON_NEXT)));
    action->setToolTip(tr("Go forward one step in history."));
    connect(action, SIGNAL(triggered(bool)), m_stackBrowser, SLOT(goNext()));
    layout->addWidget(createToolButton(action));
    m_goNext = action;

    layout->addWidget(new Utils::StyledSeparator);

    // event selection
    m_eventCombo = new QComboBox;
    m_eventCombo->setToolTip(tr("Selects which events from the profiling data are shown and visualized."));
    connect(m_eventCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(setCostEvent(int)));
    updateEventCombo();
    layout->addWidget(m_eventCombo);

    // cost formatting
    {
    QMenu *menu = new QMenu(layout->parentWidget());
    QActionGroup *group = new QActionGroup(this);

    // show costs as absolute numbers
    m_costAbsolute = new QAction(tr("Absolute Costs"), this);
    m_costAbsolute->setToolTip(tr("Show costs as absolute numbers."));
    m_costAbsolute->setCheckable(true);
    m_costAbsolute->setChecked(true);
    connect(m_costAbsolute, SIGNAL(toggled(bool)), SLOT(updateCostFormat()));
    group->addAction(m_costAbsolute);
    menu->addAction(m_costAbsolute);

    // show costs in percentages
    m_costRelative = new QAction(tr("Relative Costs"), this);
    m_costRelative->setToolTip(tr("Show costs relative to total inclusive cost."));
    m_costRelative->setCheckable(true);
    connect(m_costRelative, SIGNAL(toggled(bool)), SLOT(updateCostFormat()));
    group->addAction(m_costRelative);
    menu->addAction(m_costRelative);

    // show costs relative to parent
    m_costRelativeToParent = new QAction(tr("Relative Costs to Parent"), this);
    m_costRelativeToParent->setToolTip(tr("Show costs relative to parent functions inclusive cost."));
    m_costRelativeToParent->setCheckable(true);
    connect(m_costRelativeToParent, SIGNAL(toggled(bool)), SLOT(updateCostFormat()));
    group->addAction(m_costRelativeToParent);
    menu->addAction(m_costRelativeToParent);

    QToolButton *button = new QToolButton;
    button->setMenu(menu);
    button->setPopupMode(QToolButton::InstantPopup);
    button->setText(QLatin1String("$"));
    button->setToolTip(tr("Cost Format"));
    layout->addWidget(button);
    }

    // cycle detection
    //action = new QAction(QLatin1String("Cycle Detection"), this); ///FIXME: icon
    action = new QAction(QLatin1String("O"), this); ///FIXME: icon
    action->setToolTip(tr("Enable cycle detection to properly handle recursive or circular function calls."));
    action->setCheckable(true);
    connect(action, SIGNAL(toggled(bool)), m_dataModel, SLOT(enableCycleDetection(bool)));
    connect(action, SIGNAL(toggled(bool)), m_settings, SLOT(setDetectCycles(bool)));
    layout->addWidget(createToolButton(action));
    m_cycleDetection = action;

    // shorter template signature
    action = new QAction(QLatin1String("<>"), this);
    action->setToolTip(tr("This removes template parameter lists when displaying function names."));
    action->setCheckable(true);
    connect(action, SIGNAL(toggled(bool)), m_dataModel, SLOT(setShortenTemplates(bool)));
    connect(action, SIGNAL(toggled(bool)), m_settings, SLOT(setShortenTemplates(bool)));
    layout->addWidget(createToolButton(action));
    m_shortenTemplates = action;

    // filtering
    action = new QAction(tr("Show Project Costs Only"), this);
    action->setIcon(QIcon(QLatin1String(Core::Constants::ICON_FILTER)));
    action->setToolTip(tr("Show only profiling info that originated from this project source."));
    action->setCheckable(true);
    connect(action, SIGNAL(toggled(bool)), this, SLOT(handleFilterProjectCosts()));
    layout->addWidget(createToolButton(action));
    m_filterProjectCosts = action;

    // filter
    ///FIXME: find workaround for https://bugreports.qt-project.org/browse/QTCREATORBUG-3247
    QLineEdit *filter = new QLineEdit;
    filter->setPlaceholderText(tr("Filter..."));
    connect(filter, SIGNAL(textChanged(QString)), m_updateTimer, SLOT(start()));
    layout->addWidget(filter);
    m_searchFilter = filter;

    setCostFormat(m_settings->costFormat());
    enableCycleDetection(m_settings->detectCycles());

    layout->addWidget(new Utils::StyledSeparator);
    layout->addStretch();

    return widget;
}

void CallgrindToolPrivate::clearTextMarks()
{
    qDeleteAll(m_textMarks);
    m_textMarks.clear();
}

void CallgrindToolPrivate::engineStarting(const Analyzer::IAnalyzerEngine *)
{
    // enable/disable actions
    m_resetAction->setEnabled(true);
    m_dumpAction->setEnabled(true);
    clearTextMarks();
    slotClear();
}

void CallgrindToolPrivate::engineFinished()
{
    // enable/disable actions
    m_resetAction->setEnabled(false);
    m_dumpAction->setEnabled(false);

    const ParseData *data = m_dataModel->parseData();
    if (data)
        showParserResults(data);
    else
        AnalyzerManager::showStatusMessage(tr("Profiling aborted."));

    setBusyCursor(false);
}

void CallgrindToolPrivate::showParserResults(const ParseData *data)
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
    AnalyzerManager::showStatusMessage(msg);
}

void CallgrindToolPrivate::editorOpened(Core::IEditor *editor)
{
    TextEditor::ITextEditor *textEditor = qobject_cast<TextEditor::ITextEditor *>(editor);
    if (!textEditor)
        return;

    connect(textEditor,
        SIGNAL(markContextMenuRequested(TextEditor::ITextEditor*,int,QMenu*)),
        SLOT(requestContextMenu(TextEditor::ITextEditor*,int,QMenu*)));
}

void CallgrindToolPrivate::requestContextMenu(TextEditor::ITextEditor *editor, int line, QMenu *menu)
{
    // find callgrind text mark that corresponds to this editor's file and line number
    const Function *func = 0;
    foreach (CallgrindTextMark *textMark, m_textMarks) {
        if (textMark->fileName() == editor->document()->fileName() && textMark->lineNumber() == line) {
            func = textMark->function();
            break;
        }
    }
    if (!func)
        return; // no callgrind text mark under cursor, return

    // add our action to the context menu
    QAction *action = new QAction(tr("Select this Function in the Analyzer Output"), menu);
    connect(action, SIGNAL(triggered()), SLOT(handleShowCostsAction()));
    action->setData(QVariant::fromValue<const Function *>(func));
    menu->addAction(action);
}

void CallgrindToolPrivate::handleShowCostsAction()
{
    const QAction *action = qobject_cast<QAction *>(sender());
    QTC_ASSERT(action, return);

    const Function *func = action->data().value<const Function *>();
    QTC_ASSERT(func, return);

    selectFunction(func);
}

void CallgrindToolPrivate::handleShowCostsOfFunction()
{
    CPlusPlus::Symbol *symbol = AnalyzerUtils::findSymbolUnderCursor();
    if (!symbol)
        return;

    if (!symbol->isFunction())
        return;

    CPlusPlus::Overview view;
    const QString qualifiedFunctionName = view.prettyName(CPlusPlus::LookupContext::fullyQualifiedName(symbol));

    m_toggleCollectFunction = qualifiedFunctionName + QLatin1String("()");

    AnalyzerManager::selectTool(q, StartMode(StartLocal));
    AnalyzerManager::startTool(q, StartMode(StartLocal));
}

void CallgrindToolPrivate::slotRequestDump()
{
    //setBusy(true);
    m_visualisation->setText(tr("Populating..."));
    dumpRequested();
}

void CallgrindToolPrivate::takeParserData(CallgrindEngine *engine)
{
    ParseData *data = engine->takeParserData();
    showParserResults(data);

    if (!data)
        return;

    // clear first
    clearTextMarks();
    slotClear();

    setParseData(data);
    createTextMarks();
}

void CallgrindToolPrivate::createTextMarks()
{
    DataModel *model = m_dataModel;
    QTC_ASSERT(model, return);

    QList<QString> locations;
    for (int row = 0; row < model->rowCount(); ++row) {
        const QModelIndex index = model->index(row, DataModel::InclusiveCostColumn);

        QString fileName = index.data(DataModel::FileNameRole).toString();
        if (fileName.isEmpty() || fileName == QLatin1String("???"))
            continue;

        bool ok = false;
        const int lineNumber = index.data(DataModel::LineNumberRole).toInt(&ok);
        QTC_ASSERT(ok, continue);

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

        CallgrindTextMark *mark = new CallgrindTextMark(index, fileName, lineNumber);
        mark->init();
        m_textMarks.append(mark);
    }
}

} // namespace Internal
} // namespace Valgrind

#include "callgrindtool.moc"
