/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "callgrindtool.h"

#include "callgrindconstants.h"
#include "callgrindcostview.h"
#include "callgrindengine.h"
#include "callgrindwidgethandler.h"
#include "callgrindtextmark.h"
#include "callgrindvisualisation.h"
#include "callgrindsettings.h"

#include <analyzerbase/analyzermanager.h>
#include <analyzerbase/analyzersettings.h>
#include <analyzerbase/analyzerutils.h>
#include <analyzerbase/ianalyzeroutputpaneadapter.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/Overview.h>
#include <cppeditor/cppeditorconstants.h>
#include <extensionsystem/iplugin.h>
#include <texteditor/basetexteditor.h>
#include <utils/qtcassert.h>
#include <utils/fancymainwindow.h>
#include <utils/styledbar.h>

#include <valgrind/callgrind/callgrinddatamodel.h>
#include <valgrind/callgrind/callgrindparsedata.h>
#include <valgrind/callgrind/callgrindcostitem.h>
#include <valgrind/callgrind/callgrindproxymodel.h>
#include <valgrind/callgrind/callgrindfunction.h>
#include <valgrind/callgrind/callgrindstackbrowser.h>

#include <QtGui/QDockWidget>
#include <QtGui/QHBoxLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QGraphicsItem>
#include <QtGui/QMenu>
#include <QtGui/QToolButton>
#include <QtGui/QAction>
#include <QtGui/QLineEdit>
#include <QtGui/QToolBar>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

// shared/cplusplus includes
#include <Symbols.h>

using namespace Callgrind;

using namespace Analyzer;
using namespace Core;
using namespace Valgrind::Callgrind;

namespace Callgrind {
namespace Internal {

static QToolButton *createToolButton(QAction *action)
{
    QToolButton *button = new QToolButton;
    button->setDefaultAction(action);
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    return button;
}

CallgrindTool::CallgrindTool(QObject *parent)
: Analyzer::IAnalyzerTool(parent)
, m_callgrindWidgetHandler(0)
, m_dumpAction(0)
, m_resetAction(0)
, m_pauseAction(0)
, m_showCostsOfFunctionAction(0)
, m_toolbarWidget(0)
{
    Core::ICore *core = Core::ICore::instance();

    // EditorManager
    QObject *editorManager = core->editorManager();
    connect(editorManager, SIGNAL(editorOpened(Core::IEditor*)),
        SLOT(editorOpened(Core::IEditor*)));
}

CallgrindTool::~CallgrindTool()
{
    qDeleteAll(m_textMarks);
}

QString CallgrindTool::id() const
{
    return "Callgrind";
}

QString CallgrindTool::displayName() const
{
    return tr("Profile");
}

IAnalyzerTool::ToolMode CallgrindTool::mode() const
{
    return ReleaseMode;
}

void CallgrindTool::initialize(ExtensionSystem::IPlugin */*plugin*/)
{
    AnalyzerManager *am = AnalyzerManager::instance();

    CallgrindWidgetHandler *handler = new CallgrindWidgetHandler(am->mainWindow());
    m_callgrindWidgetHandler = handler;

    connect(m_callgrindWidgetHandler, SIGNAL(functionSelected(const Valgrind::Callgrind::Function*)),
            this, SLOT(slotFunctionSelected(const Valgrind::Callgrind::Function*)));
}

void CallgrindTool::initializeDockWidgets()
{
    AnalyzerManager *am = AnalyzerManager::instance();

    //QDockWidget *callersDock =
        am->createDockWidget(this, tr("Callers"),
                             m_callgrindWidgetHandler->callersView(),
                             Qt::BottomDockWidgetArea);

    QDockWidget *flatDock =
        am->createDockWidget(this, tr("Functions"),
                             m_callgrindWidgetHandler->flatView(),
                             Qt::LeftDockWidgetArea);

    QDockWidget *calleesDock =
        am->createDockWidget(this, tr("Callees"),
                             m_callgrindWidgetHandler->calleesView(),
                             Qt::BottomDockWidgetArea);

    //QDockWidget *visDock =
        am->createDockWidget(this, tr("Visualization"),
                             m_callgrindWidgetHandler->visualisation(),
                             Qt::LeftDockWidgetArea);

    am->mainWindow()->splitDockWidget(flatDock, calleesDock, Qt::Vertical);
    am->mainWindow()->tabifyDockWidget(flatDock, calleesDock);

    m_toolbarWidget = new QWidget;
    m_toolbarWidget->setObjectName("CallgrindToolBarWidget");
    QHBoxLayout *layout = new QHBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    m_toolbarWidget->setLayout(layout);

    m_callgrindWidgetHandler->populateActions(layout);

    CallgrindGlobalSettings *settings = AnalyzerGlobalSettings::instance()->subConfig<CallgrindGlobalSettings>();
    m_callgrindWidgetHandler->setCostFormat(settings->costFormat());
    m_callgrindWidgetHandler->enableCycleDetection(settings->detectCycles());
    connect(m_callgrindWidgetHandler, SIGNAL(costFormatChanged(Callgrind::Internal::CostDelegate::CostFormat)),
            settings, SLOT(setCostFormat(Callgrind::Internal::CostDelegate::CostFormat)));
    connect(m_callgrindWidgetHandler, SIGNAL(cycleDetectionEnabled(bool)),
            settings, SLOT(setDetectCycles(bool)));
}

void CallgrindTool::extensionsInitialized()
{
    Core::ICore *core = Core::ICore::instance();
    Core::ActionManager *actionManager = core->actionManager();

    Core::Context analyzerContext = Core::Context(Analyzer::Constants::C_ANALYZEMODE);

    // check if there is a CppEditor context menu, if true, add our own context menu actions
    if (Core::ActionContainer *editorContextMenu =
            actionManager->actionContainer(CppEditor::Constants::M_CONTEXT)) {
        QAction *action = 0;
        Core::Command *cmd = 0;

        action = new QAction(this);
        action->setSeparator(true);
        cmd = actionManager->registerAction(action, "Analyzer.Callgrind.ContextMenu.Sep",
            analyzerContext);
        editorContextMenu->addAction(cmd);

        action = new QAction(tr("Profile Costs of this Function and its Callees"), this);
        action->setIcon(QIcon(Analyzer::Constants::ANALYZER_CONTROL_START_ICON));
        connect(action, SIGNAL(triggered()), SLOT(handleShowCostsOfFunction()));
        cmd = actionManager->registerAction(action, Callgrind::Constants::A_SHOWCOSTSOFFUNCTION,
            analyzerContext);
        editorContextMenu->addAction(cmd);
        cmd->setAttribute(Core::Command::CA_Hide);
        cmd->setAttribute(Core::Command::CA_NonConfigurable);
        m_showCostsOfFunctionAction = action;
    }
}

IAnalyzerEngine *CallgrindTool::createEngine(const AnalyzerStartParameters &sp,
                                             ProjectExplorer::RunConfiguration *runConfiguration)
{
    CallgrindEngine *engine = new CallgrindEngine(sp, runConfiguration);

    connect(engine, SIGNAL(parserDataReady(CallgrindEngine *)), SLOT(takeParserData(CallgrindEngine *)));

    connect(engine, SIGNAL(starting(const Analyzer::IAnalyzerEngine*)),
            this, SLOT(engineStarting(const Analyzer::IAnalyzerEngine*)));
    connect(engine, SIGNAL(finished()),
            this, SLOT(engineFinished()));

    connect(this, SIGNAL(dumpRequested()), engine, SLOT(dump()));
    connect(this, SIGNAL(resetRequested()), engine, SLOT(reset()));
    connect(this, SIGNAL(pauseToggled(bool)), engine, SLOT(setPaused(bool)));

    // initialize engine
    engine->setPaused(m_pauseAction->isChecked());

    // we may want to toggle collect for one function only in this run
    engine->setToggleCollectFunction(m_toggleCollectFunction);
    m_toggleCollectFunction.clear();

    AnalyzerManager::instance()->showStatusMessage(AnalyzerManager::msgToolStarted(displayName()));

    // apply project settings
    AnalyzerProjectSettings *analyzerSettings = runConfiguration->extraAspect<AnalyzerProjectSettings>();
    CallgrindProjectSettings *settings = analyzerSettings->subConfig<CallgrindProjectSettings>();
    QTC_ASSERT(settings, return engine)

    m_callgrindWidgetHandler->visualisation()->setMinimumInclusiveCostRatio(settings->visualisationMinimumInclusiveCostRatio() / 100.0);
    m_callgrindWidgetHandler->proxyModel()->setMinimumInclusiveCostRatio(settings->minimumInclusiveCostRatio() / 100.0);
    m_callgrindWidgetHandler->dataModel()->setVerboseToolTipsEnabled(settings->enableEventToolTips());

    return engine;
}

QWidget *CallgrindTool::createControlWidget()
{
    QWidget *widget = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    widget->setLayout(layout);

    // dump action
    m_dumpAction = new QAction(this);
    m_dumpAction->setDisabled(true);
    m_dumpAction->setIcon(QIcon(QLatin1String(Core::Constants::ICON_REDO)));
    m_dumpAction->setText(tr("Dump"));
    m_dumpAction->setToolTip(tr("Request the dumping of profile information. This will update the callgrind visualization."));
    connect(m_dumpAction, SIGNAL(triggered()), this, SLOT(slotRequestDump()));
    layout->addWidget(createToolButton(m_dumpAction));

    // reset action
    m_resetAction = new QAction(this);
    m_resetAction->setDisabled(true);
    m_resetAction->setIcon(QIcon(QLatin1String(Core::Constants::ICON_CLEAR)));
    m_resetAction->setText(tr("Reset"));
    m_resetAction->setToolTip(tr("Zero all event counters."));
    connect(m_resetAction, SIGNAL(triggered()), this, SIGNAL(resetRequested()));
    layout->addWidget(createToolButton(m_resetAction));

    // pause action
    m_pauseAction = new QAction(this);
    m_pauseAction->setCheckable(true);
    m_pauseAction->setIcon(QIcon(QLatin1String(":/qml/images/pause-small.png")));
    m_pauseAction->setText(tr("Ignore"));
    m_pauseAction->setToolTip(tr("If enabled, no events are counted which will speed up program execution during profiling."));
    connect(m_pauseAction, SIGNAL(toggled(bool)), this, SIGNAL(pauseToggled(bool)));
    layout->addWidget(createToolButton(m_pauseAction));

    layout->addWidget(new Utils::StyledSeparator);
    layout->addStretch();

    return widget;
}

CallgrindWidgetHandler *CallgrindTool::callgrindWidgetHandler() const
{
  return m_callgrindWidgetHandler;
}

void CallgrindTool::clearErrorView()
{
    clearTextMarks();

    m_callgrindWidgetHandler->slotClear();
}

void CallgrindTool::clearTextMarks()
{
    qDeleteAll(m_textMarks);
    m_textMarks.clear();
}

void CallgrindTool::engineStarting(const Analyzer::IAnalyzerEngine *)
{
    // enable/disable actions
    m_resetAction->setEnabled(true);
    m_dumpAction->setEnabled(true);

    clearErrorView();
}

void CallgrindTool::engineFinished()
{
    // enable/disable actions
    m_resetAction->setEnabled(false);
    m_dumpAction->setEnabled(false);

    const ParseData *data = m_callgrindWidgetHandler->dataModel()->parseData();
    if (data)
        showParserResults(data);
    else
        AnalyzerManager::instance()->showStatusMessage(tr("Profiling aborted."));
}

void CallgrindTool::showParserResults(const ParseData *data)
{
    QString msg;
    if (data) {
        // be careful, the list of events might be empty
        if (data->events().isEmpty()) {
            msg = tr("Parsing finished, no data.");
        }
        else {
            const QString costStr = QString("%1 %2").arg(QString::number(data->totalCost(0)), data->events().first());
            msg = tr("Parsing finished, total cost of %1 reported.").arg(costStr);
        }
    } else {
        msg = tr("Parsing failed.");
    }
    AnalyzerManager::instance()->showStatusMessage(msg);
}

void CallgrindTool::editorOpened(Core::IEditor *editor)
{
    TextEditor::ITextEditor *textEditor = qobject_cast<TextEditor::ITextEditor *>(editor);
    if (!textEditor)
        return;

    connect(textEditor,
        SIGNAL(markContextMenuRequested(TextEditor::ITextEditor*,int,QMenu*)),
        SLOT(requestContextMenu(TextEditor::ITextEditor*,int,QMenu*)));
}

void CallgrindTool::requestContextMenu(TextEditor::ITextEditor *editor, int line, QMenu *menu)
{
    // find callgrind text mark that corresponds to this editor's file and line number
    const Function *func = 0;
    foreach (CallgrindTextMark *textMark, m_textMarks) {
        if (textMark->fileName() == editor->file()->fileName() && textMark->lineNumber() == line) {
            func = textMark->function();
            break;
        }
    }
    if (!func)
        return; // no callgrind text mark under cursor, return

    // add our action to the context menu
    QAction *action = new QAction(tr("Select this Function in the Analyzer Output"), menu);
    connect(action, SIGNAL(triggered()), this, SLOT(handleShowCostsAction()));
    action->setData(QVariant::fromValue<const Function *>(func));
    menu->addAction(action);
}

void CallgrindTool::handleShowCostsAction()
{
    const QAction *action = qobject_cast<QAction *>(sender());
    QTC_ASSERT(action, return)

    const Function *func = action->data().value<const Function *>();
    QTC_ASSERT(func, return)

    m_callgrindWidgetHandler->selectFunction(func);
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

    m_toggleCollectFunction = QString("%1()").arg(qualifiedFunctionName);

    AnalyzerManager::instance()->selectTool(this);
    AnalyzerManager::instance()->startTool();
}


void CallgrindTool::slotRequestDump()
{
    m_callgrindWidgetHandler->slotRequestDump();
    emit dumpRequested();
}

void CallgrindTool::slotFunctionSelected(const Function *func)
{
    if (func && QFile::exists(func->file())) {
        ///TODO: custom position support?
        int line = func->lineNumber();
        TextEditor::BaseTextEditorWidget::openEditorAt(func->file(), qMax(line, 0));
    }
}

void CallgrindTool::takeParserData(CallgrindEngine *engine)
{
    ParseData *data = engine->takeParserData();
    showParserResults(data);

    if (!data)
        return;

    // clear first
    clearErrorView();

    m_callgrindWidgetHandler->setParseData(data);
    createTextMarks();
}

void CallgrindTool::createTextMarks()
{
    DataModel *model = m_callgrindWidgetHandler->dataModel();
    QTC_ASSERT(model, return)

    QList<QString> locations;
    for (int row = 0; row < model->rowCount(); ++row)
    {
        const QModelIndex index = model->index(row, DataModel::InclusiveCostColumn);

        QString fileName = index.data(DataModel::FileNameRole).toString();
        if (fileName.isEmpty() || fileName == "???")
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
        const QString location = QString("%1:%2").arg(fileName, QString::number(lineNumber));
        if (locations.contains(location))
            continue;
        locations << location;

        CallgrindTextMark *mark = new CallgrindTextMark(index, fileName, lineNumber);
        m_textMarks << mark;
    }
}

bool CallgrindTool::canRunRemotely() const
{
    return true;
}

}
}
