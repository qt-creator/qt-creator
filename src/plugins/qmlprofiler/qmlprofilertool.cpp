/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qmlprofilertool.h"
#include "qmlprofilerengine.h"
#include "qmlprofilerplugin.h"
#include "qmlprofilerconstants.h"
#include "qmlprofilerattachdialog.h"

#include "tracewindow.h"
#include <qmljsdebugclient/qdeclarativedebugclient_p.h>

#include <analyzerbase/analyzermanager.h>
#include <analyzerbase/analyzerconstants.h>
#include <analyzerbase/ianalyzeroutputpaneadapter.h>

#include "timelineview.h"

#include "canvas/qdeclarativecanvas_p.h"
#include "canvas/qdeclarativecontext2d_p.h"
#include "canvas/qdeclarativetiledcanvas_p.h"

#include <qmlprojectmanager/qmlprojectrunconfiguration.h>
#include <utils/fileinprojectfinder.h>
#include <utils/qtcassert.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>

#include <texteditor/itexteditor.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <QtCore/QFile>

#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QToolButton>

#include <QtGui/QTabWidget>
#include "qmlprofilersummaryview.h"

using namespace Analyzer;
using namespace QmlProfiler::Internal;

// Adapter for output pane.
class QmlProfilerOutputPaneAdapter : public Analyzer::IAnalyzerOutputPaneAdapter
{
public:
    explicit QmlProfilerOutputPaneAdapter(QmlProfilerTool *mct) :
        IAnalyzerOutputPaneAdapter(mct), m_tool(mct) {}

    virtual QWidget *toolBarWidget() { return m_tool->createToolBarWidget(); }
    virtual QWidget *paneWidget() { return m_tool->createTimeLineWidget(); }
    virtual void clearContents() { m_tool->clearDisplay(); }
    virtual void setFocus() { /*TODO*/ }
    virtual bool hasFocus() const { return false; /*TODO*/ }
    virtual bool canFocus() const { return false; /*TODO*/ }
    virtual bool canNavigate() const { return false; /*TODO*/ }
    virtual bool canNext() const { return false; /*TODO*/ }
    virtual bool canPrevious() const { return false; /*TODO*/ }
    virtual void goToNext() { /*TODO*/ }
    virtual void goToPrev() { /*TODO*/ }


private:
    QmlProfilerTool *m_tool;
};

class QmlProfilerTool::QmlProfilerToolPrivate
{
public:
    QmlProfilerToolPrivate(QmlProfilerTool *qq) : q(qq) {}
    ~QmlProfilerToolPrivate() {}

    QmlProfilerTool *q;

    QDeclarativeDebugConnection *m_client;
    QTimer m_connectionTimer;
    int m_connectionAttempts;
    TraceWindow *m_traceWindow;
    QTabWidget *m_tabbed;
    QmlProfilerSummaryView *m_summary;
    QmlProfilerOutputPaneAdapter *m_outputPaneAdapter;
    ProjectExplorer::Project *m_project;
    Utils::FileInProjectFinder m_projectFinder;
    ProjectExplorer::RunConfiguration *m_runConfiguration;
    bool m_isAttached;
    QAction *m_attachAction;
    QToolButton *m_recordButton;
    bool m_recordingEnabled;
    QString m_host;
    quint64 m_port;
};

QmlProfilerTool::QmlProfilerTool(QObject *parent)
    : IAnalyzerTool(parent), d(new QmlProfilerToolPrivate(this))
{
     d->m_client = 0;
     d->m_connectionAttempts = 0;
     d->m_traceWindow = 0;
     d->m_outputPaneAdapter = 0;
     d->m_project = 0;
     d->m_runConfiguration = 0;
     d->m_isAttached = false;
     d->m_attachAction = 0;
     d->m_recordingEnabled = true;

     d->m_connectionTimer.setInterval(200);
     connect(&d->m_connectionTimer, SIGNAL(timeout()), SLOT(tryToConnect()));
}

QmlProfilerTool::~QmlProfilerTool()
{
    if (d->m_client)
        delete d->m_client;
    delete d->m_tabbed;

    delete d->m_outputPaneAdapter;
    delete d;
}

QString QmlProfilerTool::id() const
{
    return "QmlProfiler";
}

QString QmlProfilerTool::displayName() const
{
    return tr("QML Performance Monitor");
}

IAnalyzerTool::ToolMode QmlProfilerTool::mode() const
{
    return AnyMode;
}

IAnalyzerEngine *QmlProfilerTool::createEngine(const AnalyzerStartParameters &sp,
                                               ProjectExplorer::RunConfiguration *runConfiguration)
{
    QmlProfilerEngine *engine = new QmlProfilerEngine(sp, runConfiguration);

    d->m_host = sp.connParams.host;
    d->m_port = sp.connParams.port;

    d->m_runConfiguration = runConfiguration;
    d->m_project = runConfiguration->target()->project();
    if (d->m_project) {
        d->m_projectFinder.setProjectDirectory(d->m_project->projectDirectory());
        updateProjectFileList();
        connect(d->m_project, SIGNAL(fileListChanged()), this, SLOT(updateProjectFileList()));
    }

    connect(engine, SIGNAL(processRunning()), this, SLOT(connectClient()));
    connect(engine, SIGNAL(finished()), this, SLOT(disconnectClient()));
    connect(engine, SIGNAL(stopRecording()), this, SLOT(stopRecording()));
    connect(d->m_traceWindow, SIGNAL(viewUpdated()), engine, SLOT(dataReceived()));
    connect(this, SIGNAL(connectionFailed()), engine, SLOT(finishProcess()));
    connect(this, SIGNAL(fetchingData(bool)), engine, SLOT(setFetchingData(bool)));
    emit fetchingData(d->m_recordButton->isChecked());

    return engine;
}

void QmlProfilerTool::initialize()
{
    qmlRegisterType<Canvas>("Monitor", 1, 0, "Canvas");
    qmlRegisterType<TiledCanvas>("Monitor", 1, 0, "TiledCanvas");
    qmlRegisterType<Context2D>();
    qmlRegisterType<CanvasImage>();
    qmlRegisterType<CanvasGradient>();

    qmlRegisterType<TimelineView>("Monitor", 1, 0,"TimelineView");

    d->m_tabbed = new QTabWidget();

    d->m_traceWindow = new TraceWindow(d->m_tabbed);
    d->m_traceWindow->reset(d->m_client);

    connect(d->m_traceWindow, SIGNAL(gotoSourceLocation(QString,int)), this, SLOT(gotoSourceLocation(QString,int)));
    connect(d->m_traceWindow, SIGNAL(timeChanged(qreal)), this, SLOT(updateTimer(qreal)));

    d->m_summary = new QmlProfilerSummaryView(d->m_tabbed);
    d->m_tabbed->addTab(d->m_traceWindow, "timeline");
    d->m_tabbed->addTab(d->m_summary, "summary");

    connect(d->m_traceWindow,SIGNAL(range(int,qint64,qint64,QStringList,QString,int)),
            d->m_summary,SLOT(addRangedEvent(int,qint64,qint64,QStringList,QString,int)));
    connect(d->m_traceWindow,SIGNAL(viewUpdated()), d->m_summary, SLOT(complete()));
    connect(d->m_summary,SIGNAL(gotoSourceLocation(QString,int)), this, SLOT(gotoSourceLocation(QString,int)));

    Core::ICore *core = Core::ICore::instance();
    Core::ActionManager *am = core->actionManager();
    Core::ActionContainer *manalyzer = am->actionContainer(Analyzer::Constants::M_DEBUG_ANALYZER);
    const Core::Context globalcontext(Core::Constants::C_GLOBAL);

    d->m_attachAction = new QAction(tr("Attach..."), manalyzer);
    Core::Command *command = am->registerAction(d->m_attachAction,
                                                Constants::ATTACH, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    manalyzer->addAction(command, Analyzer::Constants::G_ANALYZER_STARTSTOP);
    connect(d->m_attachAction, SIGNAL(triggered()), this, SLOT(attach()));

    Analyzer::AnalyzerManager *analyzerMgr = Analyzer::AnalyzerManager::instance();
    connect(analyzerMgr, SIGNAL(currentToolChanged(Analyzer::IAnalyzerTool*)),
            this, SLOT(updateAttachAction()));

    updateAttachAction();
}

void QmlProfilerTool::extensionsInitialized()
{
}

IAnalyzerOutputPaneAdapter *QmlProfilerTool::outputPaneAdapter()
{
    if (!d->m_outputPaneAdapter)
        d->m_outputPaneAdapter = new QmlProfilerOutputPaneAdapter(this);
    return d->m_outputPaneAdapter;
}

QWidget *QmlProfilerTool::createToolBarWidget()
{
    // custom toolbar (TODO)
    QWidget *toolbarWidget = new QWidget;
    toolbarWidget->setObjectName(QLatin1String("QmlProfilerToolBarWidget"));

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);

    d->m_recordButton = new QToolButton(toolbarWidget);

    d->m_recordButton->setIcon(QIcon(QLatin1String(":/qmlprofiler/analyzer_category_small.png")));
    d->m_recordButton->setCheckable(true);

    connect(d->m_recordButton,SIGNAL(toggled(bool)), this, SLOT(setRecording(bool)));
    d->m_recordButton->setChecked(true);
    layout->addWidget(d->m_recordButton);

    QLabel *timeLabel = new QLabel(tr("elapsed:      0 s"));
    QPalette palette = timeLabel->palette();
    palette.setColor(QPalette::WindowText, Qt::white);
    timeLabel->setPalette(palette);

    connect(this,SIGNAL(setTimeLabel(QString)),timeLabel,SLOT(setText(QString)));
    layout->addWidget(timeLabel);
    toolbarWidget->setLayout(layout);

    return toolbarWidget;
}

QWidget *QmlProfilerTool::createTimeLineWidget()
{
    return d->m_tabbed;
}

void QmlProfilerTool::connectClient()
{
    QTC_ASSERT(!d->m_client, return;)
    d->m_client = new QDeclarativeDebugConnection;
    d->m_traceWindow->reset(d->m_client);
    connect(d->m_client, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            this, SLOT(connectionStateChanged()));
    d->m_connectionTimer.start();
}

void QmlProfilerTool::connectToClient()
{
    if (!d->m_client || d->m_client->state() != QAbstractSocket::UnconnectedState)
        return;
    if (QmlProfilerPlugin::debugOutput)
        qWarning("QmlProfiler: Connecting to %s:%lld ...", qPrintable(d->m_host), d->m_port);


    d->m_client->connectToHost(d->m_host, d->m_port);
}

void QmlProfilerTool::disconnectClient()
{
    // this might be actually be called indirectly by QDDConnectionPrivate::readyRead(), therefore allow
    // method to complete before deleting object
    if (d->m_client) {
        d->m_client->deleteLater();
        d->m_client = 0;
    }
}

void QmlProfilerTool::startRecording()
{
    if (d->m_client && d->m_client->isConnected()) {
        clearDisplay();
        d->m_traceWindow->setRecording(true);
    }
    emit fetchingData(true);
}

void QmlProfilerTool::stopRecording()
{
    d->m_traceWindow->setRecording(false);
    emit fetchingData(false);
}

void QmlProfilerTool::setRecording(bool recording)
{
    d->m_recordingEnabled = recording;
    if (recording)
        startRecording();
    else
        stopRecording();
}

void QmlProfilerTool::gotoSourceLocation(const QString &fileUrl, int lineNumber)
{
    if (lineNumber < 0 || fileUrl.isEmpty())
        return;

    const QString fileName = QUrl(fileUrl).toLocalFile();
    const QString projectFileName = d->m_projectFinder.findFile(fileName);

    Core::EditorManager *editorManager = Core::EditorManager::instance();
    Core::IEditor *editor = editorManager->openEditor(projectFileName);
    TextEditor::ITextEditor *textEditor = qobject_cast<TextEditor::ITextEditor*>(editor);

    if (textEditor) {
        editorManager->addCurrentPositionToNavigationHistory();
        textEditor->gotoLine(lineNumber);
        textEditor->widget()->setFocus();
    }
}

void QmlProfilerTool::updateTimer(qreal elapsedSeconds)
{
    QString timeString = QString::number(elapsedSeconds,'f',1);
    timeString = QString("      ").left(6-timeString.length()) + timeString;
    emit setTimeLabel(tr("elapsed: ")+timeString+QLatin1String(" s"));
}

void QmlProfilerTool::updateProjectFileList()
{
    d->m_projectFinder.setProjectFiles(
                d->m_project->files(ProjectExplorer::Project::ExcludeGeneratedFiles));
}

bool QmlProfilerTool::canRunRemotely() const
{
    // TODO: Is this correct?
    return true;
}

void QmlProfilerTool::clearDisplay()
{
    d->m_traceWindow->clearDisplay();
    d->m_summary->clean();
}

void QmlProfilerTool::attach()
{
    if (!d->m_isAttached) {
        QmlProfilerAttachDialog dialog;
        int result = dialog.exec();

        if (result == QDialog::Rejected)
            return;

        d->m_port = dialog.port();
        d->m_host = dialog.address();

        connectClient();
        AnalyzerManager::instance()->showMode();
    } else {
        stopRecording();
    }

    d->m_isAttached = !d->m_isAttached;
    updateAttachAction();
}

void QmlProfilerTool::updateAttachAction()
{
    if (d->m_attachAction) {
        if (d->m_isAttached) {
            d->m_attachAction->setText(tr("Detach"));
        } else {
            d->m_attachAction->setText(tr("Attach..."));
        }
    }

    d->m_attachAction->setEnabled(Analyzer::AnalyzerManager::instance()->currentTool() == this);
}

void QmlProfilerTool::tryToConnect()
{
    ++d->m_connectionAttempts;

    if (d->m_client && d->m_client->isConnected()) {
        d->m_connectionTimer.stop();
        d->m_connectionAttempts = 0;
    } else if (d->m_connectionAttempts == 50) {
        d->m_connectionTimer.stop();
        d->m_connectionAttempts = 0;
        if (QmlProfilerPlugin::debugOutput) {
            if (d->m_client) {
                qWarning("QmlProfiler: Failed to connect: %s", qPrintable(d->m_client->errorString()));
            } else {
                qWarning("QmlProfiler: Failed to connect.");
            }
        }
        emit connectionFailed();
    } else {
        connectToClient();
    }
}

void QmlProfilerTool::connectionStateChanged()
{
    if (!d->m_client)
        return;
    switch (d->m_client->state()) {
    case QAbstractSocket::UnconnectedState:
    {
        if (QmlProfilerPlugin::debugOutput)
            qWarning("QmlProfiler: disconnected");
        break;
    }
    case QAbstractSocket::HostLookupState:
        break;
    case QAbstractSocket::ConnectingState: {
        if (QmlProfilerPlugin::debugOutput)
            qWarning("QmlProfiler: Connecting to debug server ...");
        break;
    }
    case QAbstractSocket::ConnectedState:
    {
        if (QmlProfilerPlugin::debugOutput)
            qWarning("QmlProfiler: connected and running");
        updateRecordingState();
        break;
    }
    case QAbstractSocket::ClosingState:
        if (QmlProfilerPlugin::debugOutput)
            qWarning("QmlProfiler: closing ...");
        break;
    case QAbstractSocket::BoundState:
    case QAbstractSocket::ListeningState:
        break;
    }
}

void QmlProfilerTool::updateRecordingState()
{
    if (d->m_client->isConnected()) {
        d->m_traceWindow->setRecording(d->m_recordingEnabled);
    } else {
        d->m_traceWindow->setRecording(false);
    }

    if (d->m_traceWindow->isRecording())
        clearDisplay();
}
