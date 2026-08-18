// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilertool.h"

#include "profilermode.h"
#include "profilertr.h"
#include "profilertracedocument.h"
#include "profilertraceeditor.h"
#include "qmlprofilerattachdialog.h"
#include "qmlprofilerclientmanager.h"
#include "qmlprofilerconstants.h"
#include "qmlprofilerrunconfigurationaspect.h"
#include "qmlprofilerruncontrol.h"
#include "qmlprofilersettings.h"
#include "qmlprofilerstatemanager.h"
#include "qmlprofilertracebackend.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/runcontrol.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/shutdownguard.h>
#include <utils/url.h>

#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QPointer>

using namespace Core;
using namespace Core::Constants;
using namespace Profiler::Constants;
using namespace ProjectExplorer;
using namespace Utils;

namespace Profiler::Internal {

static QmlProfilerTool *m_instance = nullptr;

class QmlProfilerTool::QmlProfilerToolPrivate
{
public:
    // The document a run records into. A new run opens another one, unless the
    // aggregate-traces setting asks for the recordings to be merged.
    QPointer<ProfilerTraceDocument> liveDocument;
    int runCount = 0;

    QAction startAction;
    QAction runAction;
    QAction attachAction;
    QAction loadQmlTrace;
    QAction saveQmlTrace;
    ActionContainer *options = nullptr;
};

QmlProfilerTool::QmlProfilerTool()
    : d(new QmlProfilerToolPrivate)
{
    m_instance = this;
    setObjectName("QmlProfilerTool");

    d->startAction.setText(Tr::tr("Start"));
    d->startAction.setIcon(ProjectExplorer::Icons::ANALYZER_START_SMALL_TOOLBAR.icon());
    connect(&d->startAction, &QAction::triggered, this, &QmlProfilerTool::profileStartupProject);

    const QString description = Tr::tr("The QML Profiler can be used to find performance "
                                       "bottlenecks in applications using QML.");

    d->runAction.setText(Tr::tr("QML Profiler"));
    d->runAction.setToolTip(description);
    connect(&d->runAction, &QAction::triggered, this, &QmlProfilerTool::profileStartupProject);
    connect(&d->startAction, &QAction::changed, this, [this] {
        d->runAction.setEnabled(d->startAction.isEnabled());
    });

    d->attachAction.setText(Tr::tr("QML Profiler (Attach to Waiting Application)"));
    d->attachAction.setToolTip(description);
    connect(&d->attachAction, &QAction::triggered,
            this, &QmlProfilerTool::attachToWaitingApplication);

    d->loadQmlTrace.setText(Tr::tr("Load QML Trace"));
    connect(&d->loadQmlTrace, &QAction::triggered,
            this, &QmlProfilerTool::showLoadDialog, Qt::QueuedConnection);

    d->saveQmlTrace.setText(Tr::tr("Save QML Trace"));
    connect(&d->saveQmlTrace, &QAction::triggered,
            this, &QmlProfilerTool::showSaveDialog, Qt::QueuedConnection);
    // Saving acts on the trace being shown, so it only applies to a profiler
    // editor.
    const auto updateSaveAction = [this] {
        IDocument *document = EditorManager::currentDocument();
        auto trace = qobject_cast<ProfilerTraceDocument *>(document);
        // Any saveable profiler document would qualify here, but this action
        // writes a QML trace: a perf document must not end up in a .qzt.
        d->saveQmlTrace.setEnabled(trace && trace->format() == TraceFormat::Qml
                                   && trace->isSaveAsAllowed());
    };
    connect(EditorManager::instance(), &EditorManager::currentDocumentStateChanged,
            this, updateSaveAction);
    connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
            this, updateSaveAction);
    updateSaveAction();

    d->options = ActionManager::createMenu("Analyzer.Menu.QMLOptions");
    d->options->menu()->setTitle(Tr::tr("QML Profiler Options"));
    d->options->menu()->setEnabled(true);
    ActionContainer *menu = ActionManager::actionContainer(M_DEBUG_ANALYZER);

    menu->addAction(ActionManager::registerAction(&d->runAction, "QmlProfiler.Internal"),
                    Core::Constants::G_ANALYZER_TOOLS);
    menu->addAction(ActionManager::registerAction(&d->attachAction,
                                                  "QmlProfiler.AttachToWaitingApplication"),
                    Core::Constants::G_ANALYZER_REMOTE_TOOLS);

    menu->addMenu(d->options, G_ANALYZER_OPTIONS);
    d->options->addAction(ActionManager::registerAction(&d->loadQmlTrace,
                                                        Constants::QmlProfilerLoadActionId));
    d->options->addAction(ActionManager::registerAction(&d->saveQmlTrace,
                                                        Constants::QmlProfilerSaveActionId));

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::runActionsUpdated,
            this, &QmlProfilerTool::updateRunActions);
    updateRunActions();
}

QmlProfilerTool::~QmlProfilerTool()
{
    delete d;
    m_instance = nullptr;
}

QmlProfilerTool *QmlProfilerTool::instance()
{
    return m_instance;
}

QmlProfilerTraceBackend *QmlProfilerTool::liveBackend() const
{
    if (!d->liveDocument || d->liveDocument->backends().isEmpty())
        return nullptr;
    return qobject_cast<QmlProfilerTraceBackend *>(d->liveDocument->backends().first());
}

// Opens the trace a run records into. Successive runs each get their own
// editor, unless the trace being recorded aggregates them.
static ProfilerTraceDocument *openRunDocument(int runNumber)
{
    return openLiveTrace(TraceFormat::Qml, Tr::tr("QML Profile %1").arg(runNumber),
                         QString("QmlProfiler.Run.%1").arg(runNumber));
}

void QmlProfilerTool::finalizeRunControl(RunControl *runControl)
{
    bool aggregateTraces = false;
    int flushInterval = 0;
    if (auto aspect = runControl->aspectData<QmlProfilerRunConfigurationAspect>()) {
        if (auto settings = static_cast<const QmlProfilerSettings *>(aspect->currentSettings)) {
            flushInterval = settings->flushEnabled() ? settings->flushInterval() : 0;
            aggregateTraces = settings->aggregateTraces();
        }
    }

    QmlProfilerTraceBackend *backend = liveBackend();
    if (!backend || !aggregateTraces) {
        d->liveDocument = openRunDocument(++d->runCount);
        backend = liveBackend();
    }
    QTC_ASSERT(backend, return);
    emit liveBackendChanged(backend);

    connect(backend->stopAction(), &QAction::triggered, runControl, &RunControl::initiateStop);
    backend->prepareRun(runControl->buildConfiguration(), aggregateTraces, flushInterval);
    updateRunActions();
}

void QmlProfilerTool::handleStop(QmlProfilerTraceBackend *backend)
{
    // The caller names the run's own backend: resolving liveBackend() here
    // would act on whatever run is live by now.
    if (backend) {
        disconnect(backend->stopAction(), &QAction::triggered, nullptr, nullptr);
        backend->handleStop();
    }
    updateRunActions();
}

void QmlProfilerTool::updateRunActions()
{
    QmlProfilerTraceBackend *backend = liveBackend();
    const bool busy = backend
                      && backend->stateManager()->currentState() != QmlProfilerStateManager::Idle;
    if (busy) {
        d->startAction.setEnabled(false);
        d->startAction.setToolTip(Tr::tr("A QML Profiler analysis is still in progress."));
    } else {
        const auto canRun = ProjectExplorerPlugin::canRunStartupProject(
            ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
        d->startAction.setToolTip(canRun ? Tr::tr("Start QML Profiler analysis.")
                                         : canRun.error());
        d->startAction.setEnabled(canRun.has_value());
    }
}

void QmlProfilerTool::logState(const QString &msg)
{
    MessageManager::writeFlashing(msg);
}

static void saveLastTraceFile(const FilePath &filePath)
{
    QmlProfilerSettings &s = globalSettings();
    if (filePath != s.lastTraceFile()) {
        s.lastTraceFile.setValue(filePath);
        s.writeSettings();
    }
}

QString QmlProfilerTool::fileDialogTraceFilesFilter()
{
    QString qmlTraceFiles = Tr::tr("QML traces (*%1 *%2)")
                                .arg(QtdFileExtension).arg(QztFileExtension);
    return qmlTraceFiles.append(";;").append(Core::DocumentManager::allFilesFilterString());
}

void QmlProfilerTool::showSaveDialog()
{
    auto trace = qobject_cast<ProfilerTraceDocument *>(EditorManager::currentDocument());
    if (!trace || trace->format() != TraceFormat::Qml)
        return;

    FilePath filePath = FileUtils::getSaveFilePath(Tr::tr("Save QML Trace"),
                                                   globalSettings().lastTraceFile(),
                                                   fileDialogTraceFilesFilter());
    if (filePath.isEmpty())
        return;
    if (!filePath.endsWith(QtdFileExtension) && !filePath.endsWith(QztFileExtension))
        filePath = filePath.stringAppended(QztFileExtension);
    saveLastTraceFile(filePath);
    trace->save(filePath);
}

void QmlProfilerTool::loadFile(const FilePath &filePath)
{
    saveLastTraceFile(filePath);
    openTraceFile(filePath);
}

void QmlProfilerTool::showLoadDialog()
{
    const FilePath filePath = FileUtils::getOpenFilePath(Tr::tr("Load QML Trace"),
                                                         globalSettings().lastTraceFile(),
                                                         fileDialogTraceFilesFilter());
    if (!filePath.isEmpty())
        loadFile(filePath);
}

void QmlProfilerTool::profileStartupProject()
{
    activateProfilerMode();
    ProjectExplorerPlugin::runStartupProject(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
}

RunControl *QmlProfilerTool::attachToWaitingApplication()
{
    Kit *kit = nullptr;
    int port = 0;
    {
        QtcSettings *settings = ICore::settings();
        QmlProfilerAttachDialog dialog;
        dialog.setKitId(Id::fromSetting(settings->value("AnalyzerQmlAttachDialog/kitId")));
        dialog.setPort(settings->value("AnalyzerQmlAttachDialog/port", 3768).toInt());

        if (dialog.exec() != QDialog::Accepted)
            return nullptr;

        kit = dialog.kit();
        port = dialog.port();
        QTC_ASSERT(port >= 0, return nullptr);
        QTC_ASSERT(port <= std::numeric_limits<quint16>::max(), return nullptr);

        settings->setValue("AnalyzerQmlAttachDialog/kitId", kit->id().toSetting());
        settings->setValue("AnalyzerQmlAttachDialog/port", port);
    }

    IDevice::ConstPtr device = RunDeviceKitAspect::device(kit);
    QTC_ASSERT(device, return nullptr);
    const QUrl toolControl = device->toolControlChannel(IDevice::QmlControlChannel);
    QUrl serverUrl;
    serverUrl.setScheme(Utils::urlTcpScheme());
    serverUrl.setHost(toolControl.host());
    serverUrl.setPort(port);

    auto runControl = new RunControl(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
    RunConfiguration *activeRunConfig = activeRunConfigForActiveProject();
    if (activeRunConfig && activeRunConfig->kit() == kit)
        runControl->copyDataFromRunConfiguration(activeRunConfig);
    else
        runControl->setKit(kit);
    runControl->setQmlChannel(serverUrl);
    runControl->setRunRecipe(qmlProfilerRecipe(runControl));
    runControl->start();

    if (QmlProfilerTraceBackend *backend = liveBackend()) {
        connect(backend->clientManager(), &QmlProfilerClientManager::connectionClosed,
                runControl, &RunControl::initiateStop);
    }
    return runControl;
}

QList<QAction *> QmlProfilerTool::profilerContextMenuActions()
{
    // QmlProfilerTool is also used outside Qt Creator, by the standalone
    // profiler, where the ActionManager is not initialized. Avoid a crash.
    if (!ActionManager::instance())
        return {};

    QList<QAction *> commonActions;
    if (Command *command = ActionManager::command(Constants::QmlProfilerLoadActionId))
        commonActions << command->action();
    if (Command *command = ActionManager::command(Constants::QmlProfilerSaveActionId))
        commonActions << command->action();
    return commonActions;
}

void QmlProfilerTool::showNonmodalWarning(const QString &warningMsg)
{
    auto noExecWarning = new QMessageBox(ICore::dialogParent());
    noExecWarning->setIcon(QMessageBox::Warning);
    noExecWarning->setWindowTitle(Tr::tr("QML Profiler"));
    noExecWarning->setText(warningMsg);
    noExecWarning->setStandardButtons(QMessageBox::Ok);
    noExecWarning->setDefaultButton(QMessageBox::Ok);
    noExecWarning->setModal(false);
    noExecWarning->show();
}

void setupQmlProfilerTool()
{
    static GuardedObject<QmlProfilerTool> theQmlProfilerTool;
}

} // namespace Profiler::Internal
