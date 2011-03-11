#include "qmlprofilertool.h"
#include "qmlprofilerengine.h"
#include "qmlprofilerplugin.h"

#include "tracewindow.h"
#include <private/qdeclarativedebugclient_p.h>

#include <analyzerbase/analyzermanager.h>
#include <analyzerbase/analyzerconstants.h>

#include "timelineview.h"

#include "canvas/qdeclarativecanvas_p.h"
#include "canvas/qdeclarativecontext2d_p.h"
#include "canvas/qdeclarativetiledcanvas_p.h"

#include <qmlprojectmanager/qmlprojectrunconfiguration.h>
#include <utils/fileinprojectfinder.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/project.h>

#include <texteditor/itexteditor.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QtCore/QFile>


using namespace Analyzer;
using namespace Analyzer::Internal;

QString QmlProfilerTool::host = QLatin1String("localhost");
quint16 QmlProfilerTool::port = 33455;


// Adapter for output pane.
class QmlProfilerOutputPaneAdapter : public Analyzer::IAnalyzerOutputPaneAdapter
{
public:
    explicit QmlProfilerOutputPaneAdapter(QmlProfilerTool *mct) :
        IAnalyzerOutputPaneAdapter(mct), m_tool(mct) {}

    virtual QWidget *toolBarWidget() { return m_tool->createToolBarWidget(); }
    virtual QWidget *paneWidget() { return m_tool->createTimeLineWidget(); }
    virtual void clearContents() { /*TODO*/ }
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
    TraceWindow *m_traceWindow;
    QmlProfilerOutputPaneAdapter *m_outputPaneAdapter;
};

QmlProfilerTool::QmlProfilerTool(QObject *parent)
    : IAnalyzerTool(parent), d(new QmlProfilerToolPrivate(this))
{
     d->m_client = 0;
     d->m_traceWindow = 0;
     d->m_outputPaneAdapter = 0;
}

QmlProfilerTool::~QmlProfilerTool()
{
    if (d->m_client->isConnected())
        d->m_client->disconnectFromHost();
    delete d->m_traceWindow;
    delete d->m_outputPaneAdapter;
    delete d;
}

QString QmlProfilerTool::id() const
{
    return "QmlProfiler";
}

QString QmlProfilerTool::displayName() const
{
    return tr("Qml Performance Monitor");
}

IAnalyzerTool::ToolMode QmlProfilerTool::mode() const
{
    return DebugMode;
}


IAnalyzerEngine *QmlProfilerTool::createEngine(ProjectExplorer::RunConfiguration *runConfiguration)
{
    QmlProfilerEngine *engine = new QmlProfilerEngine(runConfiguration);

    connect(engine, SIGNAL(processRunning()), this, SLOT(connectClient()));
    connect(engine, SIGNAL(processTerminated()), this, SLOT(disconnectClient()));
    connect(engine, SIGNAL(stopRecording()), this, SLOT(stopRecording()));
    connect(d->m_traceWindow, SIGNAL(viewUpdated()), engine, SLOT(viewUpdated()));
    connect(d->m_traceWindow, SIGNAL(gotoSourceLocation(QString,int)), this, SLOT(gotoSourceLocation(QString,int)));

    return engine;

}

void QmlProfilerTool::initialize(ExtensionSystem::IPlugin */*plugin*/)
{
    qmlRegisterType<Canvas>("QtQuick",1,1, "Canvas");
    qmlRegisterType<TiledCanvas>("QtQuick",1,1, "TiledCanvas");
    qmlRegisterType<Context2D>();
    qmlRegisterType<CanvasImage>();
    qmlRegisterType<CanvasGradient>();

    qmlRegisterType<TimelineView>("Monitor",1,0,"TimelineView");

    d->m_client = new QDeclarativeDebugConnection;
    d->m_traceWindow = new TraceWindow();
    d->m_traceWindow->reset(d->m_client);
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
    return 0;
}

QWidget *QmlProfilerTool::createTimeLineWidget()
{
    return d->m_traceWindow;
}

void QmlProfilerTool::connectClient()
{
    QDeclarativeDebugConnection *newClient = new QDeclarativeDebugConnection;
    d->m_traceWindow->reset(newClient);
    delete d->m_client;
    d->m_client = newClient;

    d->m_client->connectToHost(host, port);
    d->m_client->waitForConnected();

    if (d->m_client->state() == QDeclarativeDebugConnection::ConnectedState) {
        d->m_traceWindow->setRecording(true);
        if (QmlProfilerPlugin::debugOutput)
            qWarning("QmlProfiler: connected and running");
    } else {
        if (QmlProfilerPlugin::debugOutput)
            qWarning("QmlProfiler: Failed to connect: %s", qPrintable(d->m_client->errorString()));
    }
}

void QmlProfilerTool::disconnectClient()
{
    d->m_client->disconnectFromHost();
}

void QmlProfilerTool::stopRecording()
{
    d->m_traceWindow->setRecording(false);
}

void QmlProfilerTool::gotoSourceLocation(const QString &fileName, int lineNumber)
{
    if (lineNumber < 0 || !QFile::exists(QUrl(fileName).toLocalFile()))
        return;

    Utils::FileInProjectFinder m_projectFinder;
    ProjectExplorer::Project *m_debugProject;
    m_debugProject = ProjectExplorer::ProjectExplorerPlugin::instance()->startupProject();
    m_projectFinder.setProjectDirectory(m_debugProject->projectDirectory());

    QString projectFileName = m_projectFinder.findFile(fileName);


    Core::EditorManager *editorManager = Core::EditorManager::instance();
    Core::IEditor *editor = editorManager->openEditor(projectFileName);
    TextEditor::ITextEditor *textEditor = qobject_cast<TextEditor::ITextEditor*>(editor);

    if (textEditor) {
        editorManager->addCurrentPositionToNavigationHistory();
        textEditor->gotoLine(lineNumber);
        textEditor->widget()->setFocus();
    }
}
