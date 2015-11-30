/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmlprofilertool.h"
#include "qmlprofilerstatemanager.h"
#include "qmlprofilerruncontrol.h"
#include "qmlprofilerconstants.h"
#include "qmlprofilerattachdialog.h"
#include "qmlprofilerviewmanager.h"
#include "qmlprofilerclientmanager.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilerdetailsrewriter.h"
#include "qmlprofilernotesmodel.h"
#include "qmlprofilerrunconfigurationaspect.h"
#include "qmlprofilersettings.h"
#include "qmlprofilerplugin.h"

#include <analyzerbase/analyzermanager.h>
#include <analyzerbase/analyzerruncontrol.h>

#include <utils/fancymainwindow.h>
#include <utils/fileinprojectfinder.h>
#include <utils/qtcassert.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/session.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/localapplicationrunconfiguration.h>
#include <texteditor/texteditor.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/coreicons.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/findplugin.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/imode.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>

#include <qtsupport/qtkitinformation.h>

#include <QApplication>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QTcpServer>
#include <QTime>
#include <QTimer>
#include <QToolButton>

using namespace Core;
using namespace Core::Constants;
using namespace Analyzer;
using namespace Analyzer::Constants;
using namespace QmlProfiler::Constants;
using namespace QmlDebug;
using namespace ProjectExplorer;

namespace QmlProfiler {
namespace Internal {

class QmlProfilerTool::QmlProfilerToolPrivate
{
public:
    QmlProfilerStateManager *m_profilerState;
    QmlProfilerClientManager *m_profilerConnections;
    QmlProfilerModelManager *m_profilerModelManager;

    QmlProfilerViewManager *m_viewContainer;
    Utils::FileInProjectFinder m_projectFinder;
    QToolButton *m_recordButton;
    QMenu *m_recordFeaturesMenu;

    QToolButton *m_clearButton;

    // elapsed time display
    QTimer m_recordingTimer;
    QTime m_recordingElapsedTime;
    QLabel *m_timeLabel;

    // open search
    QToolButton *m_searchButton;

    // hide and show categories
    QToolButton *m_displayFeaturesButton;
    QMenu *m_displayFeaturesMenu;

    // save and load actions
    QAction *m_saveQmlTrace;
    QAction *m_loadQmlTrace;
};

QmlProfilerTool::QmlProfilerTool(QObject *parent)
    : QObject(parent), d(new QmlProfilerToolPrivate)
{
    setObjectName(QLatin1String("QmlProfilerTool"));

    d->m_profilerState = 0;
    d->m_viewContainer = 0;
    d->m_recordButton = 0;
    d->m_recordFeaturesMenu = 0;
    d->m_clearButton = 0;
    d->m_timeLabel = 0;
    d->m_searchButton = 0;
    d->m_displayFeaturesButton = 0;
    d->m_displayFeaturesMenu = 0;

    d->m_profilerState = new QmlProfilerStateManager(this);
    connect(d->m_profilerState, &QmlProfilerStateManager::stateChanged,
            this, &QmlProfilerTool::profilerStateChanged);
    connect(d->m_profilerState, &QmlProfilerStateManager::clientRecordingChanged,
            this, &QmlProfilerTool::clientRecordingChanged);
    connect(d->m_profilerState, &QmlProfilerStateManager::serverRecordingChanged,
            this, &QmlProfilerTool::serverRecordingChanged);
    connect(d->m_profilerState, &QmlProfilerStateManager::recordedFeaturesChanged,
            this, &QmlProfilerTool::setRecordedFeatures);

    d->m_profilerConnections = new QmlProfilerClientManager(this);
    d->m_profilerConnections->registerProfilerStateManager(d->m_profilerState);
    connect(d->m_profilerConnections, &QmlProfilerClientManager::connectionClosed,
            this, &QmlProfilerTool::clientsDisconnected);

    d->m_profilerModelManager = new QmlProfilerModelManager(&d->m_projectFinder, this);
    connect(d->m_profilerModelManager, &QmlProfilerModelManager::stateChanged,
            this, &QmlProfilerTool::profilerDataModelStateChanged);
    connect(d->m_profilerModelManager, &QmlProfilerModelManager::error,
            this, &QmlProfilerTool::showErrorDialog);
    connect(d->m_profilerModelManager, &QmlProfilerModelManager::availableFeaturesChanged,
            this, &QmlProfilerTool::setAvailableFeatures);
    connect(d->m_profilerModelManager, &QmlProfilerModelManager::saveFinished,
            this, &QmlProfilerTool::onLoadSaveFinished);
    connect(d->m_profilerModelManager, &QmlProfilerModelManager::loadFinished,
            this, &QmlProfilerTool::onLoadSaveFinished);

    d->m_profilerConnections->setModelManager(d->m_profilerModelManager);
    Command *command = 0;

    ActionContainer *menu = ActionManager::actionContainer(M_DEBUG_ANALYZER);
    ActionContainer *options = ActionManager::createMenu(M_DEBUG_ANALYZER_QML_OPTIONS);
    options->menu()->setTitle(tr("QML Profiler Options"));
    menu->addMenu(options, G_ANALYZER_OPTIONS);
    options->menu()->setEnabled(true);

    QAction *act = d->m_loadQmlTrace = new QAction(tr("Load QML Trace"), options);
    command = ActionManager::registerAction(act, "Analyzer.Menu.StartAnalyzer.QMLProfilerOptions.LoadQMLTrace");
    connect(act, &QAction::triggered, this, &QmlProfilerTool::showLoadDialog);
    options->addAction(command);

    act = d->m_saveQmlTrace = new QAction(tr("Save QML Trace"), options);
    d->m_saveQmlTrace->setEnabled(false);
    command = ActionManager::registerAction(act, "Analyzer.Menu.StartAnalyzer.QMLProfilerOptions.SaveQMLTrace");
    connect(act, &QAction::triggered, this, &QmlProfilerTool::showSaveDialog);
    options->addAction(command);

    d->m_recordingTimer.setInterval(100);
    connect(&d->m_recordingTimer, &QTimer::timeout, this, &QmlProfilerTool::updateTimeDisplay);
}

QmlProfilerTool::~QmlProfilerTool()
{
    delete d;
}

AnalyzerRunControl *QmlProfilerTool::createRunControl(const AnalyzerStartParameters &sp,
    RunConfiguration *runConfiguration)
{
    QmlProfilerRunConfigurationAspect *aspect = static_cast<QmlProfilerRunConfigurationAspect *>(
                runConfiguration->extraAspect(Constants::SETTINGS));
    QTC_ASSERT(aspect, return 0);

    QmlProfilerSettings *settings = static_cast<QmlProfilerSettings *>(aspect->currentSettings());
    QTC_ASSERT(settings, return 0);

    d->m_profilerConnections->setFlushInterval(settings->flushEnabled() ?
                                                   settings->flushInterval() : 0);
    d->m_profilerConnections->setAggregateTraces(settings->aggregateTraces());

    QmlProfilerRunControl *engine = new QmlProfilerRunControl(sp, runConfiguration);

    engine->registerProfilerStateManager(d->m_profilerState);

    // FIXME: Check that there's something sensible in sp.connParams
    if (!sp.analyzerSocket.isEmpty())
        d->m_profilerConnections->setLocalSocket(sp.analyzerSocket);
    else
        d->m_profilerConnections->setTcpConnection(sp.analyzerHost, sp.analyzerPort);

    //
    // Initialize m_projectFinder
    //

    QString projectDirectory;
    if (runConfiguration) {
        Project *project = runConfiguration->target()->project();
        projectDirectory = project->projectDirectory().toString();
    }

    populateFileFinder(projectDirectory, sp.sysroot);

    if (sp.analyzerSocket.isEmpty())
        connect(engine, &QmlProfilerRunControl::processRunning,
                d->m_profilerConnections, &QmlProfilerClientManager::connectTcpClient);
    connect(d->m_profilerConnections, &QmlProfilerClientManager::connectionFailed,
            engine, &QmlProfilerRunControl::cancelProcess);

    return engine;
}

static QString sysroot(RunConfiguration *runConfig)
{
    QTC_ASSERT(runConfig, return QString());
    Kit *k = runConfig->target()->kit();
    if (k && SysRootKitInformation::hasSysRoot(k))
        return SysRootKitInformation::sysRoot(runConfig->target()->kit()).toString();
    return QString();
}

QWidget *QmlProfilerTool::createWidgets()
{
    QTC_ASSERT(!d->m_viewContainer, return 0);


    d->m_viewContainer = new QmlProfilerViewManager(this,
                                                    this,
                                                    d->m_profilerModelManager,
                                                    d->m_profilerState);
    connect(d->m_viewContainer, &QmlProfilerViewManager::gotoSourceLocation,
            this, &QmlProfilerTool::gotoSourceLocation);

    //
    // Toolbar
    //
    QWidget *toolbarWidget = new QWidget;
    toolbarWidget->setObjectName(QLatin1String("QmlProfilerToolBarWidget"));

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);

    d->m_recordButton = new QToolButton(toolbarWidget);
    d->m_recordButton->setCheckable(true);

    connect(d->m_recordButton,&QAbstractButton::clicked,
            this, &QmlProfilerTool::recordingButtonChanged);
    d->m_recordButton->setChecked(true);
    d->m_recordFeaturesMenu = new QMenu(d->m_recordButton);
    d->m_recordButton->setMenu(d->m_recordFeaturesMenu);
    d->m_recordButton->setPopupMode(QToolButton::MenuButtonPopup);
    connect(d->m_recordFeaturesMenu, &QMenu::triggered,
            this, &QmlProfilerTool::toggleRequestedFeature);

    setRecording(d->m_profilerState->clientRecording());
    layout->addWidget(d->m_recordButton);

    d->m_clearButton = new QToolButton(toolbarWidget);
    d->m_clearButton->setIcon(Icons::CLEAN_PANE.icon());
    d->m_clearButton->setToolTip(tr("Discard data"));

    connect(d->m_clearButton, &QAbstractButton::clicked, [this](){
        if (checkForUnsavedNotes())
            clearData();
    });

    layout->addWidget(d->m_clearButton);

    d->m_searchButton = new QToolButton;
    d->m_searchButton->setIcon(Icons::ZOOM.icon());
    d->m_searchButton->setToolTip(tr("Search timeline event notes."));
    layout->addWidget(d->m_searchButton);

    connect(d->m_searchButton, &QToolButton::clicked, this, &QmlProfilerTool::showTimeLineSearch);

    d->m_displayFeaturesButton = new QToolButton;
    d->m_displayFeaturesButton->setIcon(Icons::FILTER.icon());
    d->m_displayFeaturesButton->setToolTip(tr("Hide or show event categories."));
    d->m_displayFeaturesButton->setPopupMode(QToolButton::InstantPopup);
    d->m_displayFeaturesButton->setProperty("noArrow", true);
    d->m_displayFeaturesMenu = new QMenu(d->m_displayFeaturesButton);
    d->m_displayFeaturesButton->setMenu(d->m_displayFeaturesMenu);
    connect(d->m_displayFeaturesMenu, &QMenu::triggered,
            this, &QmlProfilerTool::toggleVisibleFeature);
    layout->addWidget(d->m_displayFeaturesButton);

    d->m_timeLabel = new QLabel();
    QPalette palette;
    palette.setColor(QPalette::WindowText, Qt::white);
    d->m_timeLabel->setPalette(palette);
    d->m_timeLabel->setIndent(10);
    updateTimeDisplay();
    layout->addWidget(d->m_timeLabel);

    layout->addStretch();

    toolbarWidget->setLayout(layout);
    setAvailableFeatures(d->m_profilerModelManager->availableFeatures());
    setRecordedFeatures(0);

    // When the widgets are requested we assume that the session data
    // is available, then we can populate the file finder
    populateFileFinder();

    return toolbarWidget;
}

void QmlProfilerTool::populateFileFinder(QString projectDirectory, QString activeSysroot)
{
    // Initialize filefinder with some sensible default
    QStringList sourceFiles;
    QList<Project *> projects = SessionManager::projects();
    if (Project *startupProject = SessionManager::startupProject()) {
        // startup project first
        projects.removeOne(startupProject);
        projects.insert(0, startupProject);
    }
    foreach (Project *project, projects)
        sourceFiles << project->files(Project::ExcludeGeneratedFiles);

    if (!projects.isEmpty()) {
        if (projectDirectory.isEmpty())
            projectDirectory = projects.first()->projectDirectory().toString();

        if (activeSysroot.isEmpty()) {
            if (Target *target = projects.first()->activeTarget())
                if (RunConfiguration *rc = target->activeRunConfiguration())
                    activeSysroot = sysroot(rc);
        }
    }

    d->m_projectFinder.setProjectDirectory(projectDirectory);
    d->m_projectFinder.setProjectFiles(sourceFiles);
    d->m_projectFinder.setSysroot(activeSysroot);
}

void QmlProfilerTool::recordingButtonChanged(bool recording)
{
    // clientRecording is our intention for new sessions. That may differ from the state of the
    // current session, as indicated by the button. To synchronize it, toggle once.

    if (recording && d->m_profilerState->currentState() == QmlProfilerStateManager::AppRunning) {
        if (checkForUnsavedNotes()) {
            if (!d->m_profilerConnections->aggregateTraces() ||
                    d->m_profilerModelManager->state() == QmlProfilerModelManager::Done)
                clearData(); // clear before the recording starts, unless we aggregate recordings
            if (d->m_profilerState->clientRecording())
                d->m_profilerState->setClientRecording(false);
            d->m_profilerState->setClientRecording(true);
        } else {
            d->m_recordButton->setChecked(false);
        }
    } else {
        if (d->m_profilerState->clientRecording() == recording)
            d->m_profilerState->setClientRecording(!recording);
        d->m_profilerState->setClientRecording(recording);
    }
}

void QmlProfilerTool::setRecording(bool recording)
{
    // update display
    d->m_recordButton->setToolTip( recording ? tr("Disable profiling") : tr("Enable profiling"));
    d->m_recordButton->setIcon(QIcon(recording ? QLatin1String(":/qmlprofiler/recordOn.png") :
                                                 QLatin1String(":/qmlprofiler/recordOff.png")));

    d->m_recordButton->setChecked(recording);

    // manage timer
    if (d->m_profilerState->currentState() == QmlProfilerStateManager::AppRunning) {
        if (recording) {
            d->m_recordingTimer.start();
            d->m_recordingElapsedTime.start();
        } else {
            d->m_recordingTimer.stop();
        }
    }
}

void QmlProfilerTool::gotoSourceLocation(const QString &fileUrl, int lineNumber, int columnNumber)
{
    if (lineNumber < 0 || fileUrl.isEmpty())
        return;

    const QString projectFileName = d->m_projectFinder.findFile(fileUrl);

    QFileInfo fileInfo(projectFileName);
    if (!fileInfo.exists() || !fileInfo.isReadable())
        return;

    // The text editors count columns starting with 0, but the ASTs store the
    // location starting with 1, therefore the -1.
    EditorManager::openEditorAt(projectFileName, lineNumber, columnNumber - 1, Id(),
                                EditorManager::DoNotSwitchToDesignMode);
}

void QmlProfilerTool::updateTimeDisplay()
{
    double seconds = 0;
    switch (d->m_profilerState->currentState()) {
    case QmlProfilerStateManager::AppStopRequested:
    case QmlProfilerStateManager::AppDying:
        return; // Transitional state: don't update the display.
    case QmlProfilerStateManager::AppRunning:
        if (d->m_profilerState->serverRecording()) {
            seconds = d->m_recordingElapsedTime.elapsed() / 1000.0;
            break;
        } // else fall through
    case QmlProfilerStateManager::Idle:
        if (d->m_profilerModelManager->state() != QmlProfilerModelManager::Empty &&
               d->m_profilerModelManager->state() != QmlProfilerModelManager::ClearingData)
            seconds = d->m_profilerModelManager->traceTime()->duration() / 1.0e9;
        break;
    }
    QString timeString = QString::number(seconds,'f',1);
    QString profilerTimeStr = QmlProfilerTool::tr("%1 s").arg(timeString, 6);
    d->m_timeLabel->setText(tr("Elapsed: %1").arg(profilerTimeStr));
}

void QmlProfilerTool::showTimeLineSearch()
{
    d->m_viewContainer->raiseTimeline();
    Core::FindPlugin::instance()->openFindToolBar(Core::FindPlugin::FindForwardDirection);
}

void QmlProfilerTool::clearData()
{
    d->m_profilerModelManager->clear();
    d->m_profilerConnections->discardPendingData();
    setRecordedFeatures(0);
}

void QmlProfilerTool::clearDisplay()
{
    d->m_profilerConnections->clearBufferedData();
    d->m_viewContainer->clear();
    updateTimeDisplay();
}

void QmlProfilerTool::setButtonsEnabled(bool enable)
{
    d->m_clearButton->setEnabled(enable);
    d->m_displayFeaturesButton->setEnabled(enable);
    d->m_searchButton->setEnabled(enable);
    d->m_recordFeaturesMenu->setEnabled(enable);
}

bool QmlProfilerTool::prepareTool()
{
    if (d->m_recordButton->isChecked()) {
        if (checkForUnsavedNotes()) {
            clearData(); // clear right away to suppress second warning on server recording change
            return true;
        } else {
            return false;
        }
    }
    return true;
}

void QmlProfilerTool::startRemoteTool()
{
    AnalyzerManager::showMode();

    Id kitId;
    quint16 port;
    Kit *kit = 0;

    {
        QSettings *settings = ICore::settings();

        kitId = Id::fromSetting(settings->value(QLatin1String("AnalyzerQmlAttachDialog/kitId")));
        port = settings->value(QLatin1String("AnalyzerQmlAttachDialog/port"), 3768).toUInt();

        QmlProfilerAttachDialog dialog;

        dialog.setKitId(kitId);
        dialog.setPort(port);

        if (dialog.exec() != QDialog::Accepted)
            return;

        kit = dialog.kit();
        port = dialog.port();

        settings->setValue(QLatin1String("AnalyzerQmlAttachDialog/kitId"), kit->id().toSetting());
        settings->setValue(QLatin1String("AnalyzerQmlAttachDialog/port"), port);
    }

    AnalyzerStartParameters sp;

    IDevice::ConstPtr device = DeviceKitInformation::device(kit);
    if (device) {
        sp.connParams = device->sshParameters();
        sp.analyzerHost = device->qmlProfilerHost();
    }
    sp.sysroot = SysRootKitInformation::sysRoot(kit).toString();
    sp.analyzerPort = port;

    AnalyzerRunControl *rc = createRunControl(sp, 0);
    ProjectExplorerPlugin::startRunControl(rc, ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
}

void QmlProfilerTool::logState(const QString &msg)
{
    MessageManager::write(msg, MessageManager::Flash);
}

void QmlProfilerTool::logError(const QString &msg)
{
    MessageManager::write(msg);
}

void QmlProfilerTool::showErrorDialog(const QString &error)
{
    QMessageBox *errorDialog = new QMessageBox(ICore::mainWindow());
    errorDialog->setIcon(QMessageBox::Warning);
    errorDialog->setWindowTitle(tr("QML Profiler"));
    errorDialog->setText(error);
    errorDialog->setStandardButtons(QMessageBox::Ok);
    errorDialog->setDefaultButton(QMessageBox::Ok);
    errorDialog->setModal(false);
    errorDialog->show();
}

void QmlProfilerTool::showLoadOption()
{
    d->m_loadQmlTrace->setEnabled(!d->m_profilerState->serverRecording());
}

void QmlProfilerTool::showSaveOption()
{
    d->m_saveQmlTrace->setEnabled(!d->m_profilerModelManager->isEmpty());
}

void saveLastTraceFile(const QString &filename)
{
    QmlProfilerSettings *settings = QmlProfilerPlugin::globalSettings();
    if (filename != settings->lastTraceFile()) {
        settings->setLastTraceFile(filename);
        settings->writeGlobalSettings();
    }
}

void QmlProfilerTool::showSaveDialog()
{
    QString filename = QFileDialog::getSaveFileName(
                ICore::mainWindow(), tr("Save QML Trace"),
                QmlProfilerPlugin::globalSettings()->lastTraceFile(),
                tr("QML traces (*%1)").arg(QLatin1String(TraceFileExtension)));
    if (!filename.isEmpty()) {
        if (!filename.endsWith(QLatin1String(TraceFileExtension)))
            filename += QLatin1String(TraceFileExtension);
        saveLastTraceFile(filename);
        AnalyzerManager::mainWindow()->setEnabled(false);
        d->m_profilerModelManager->save(filename);
    }
}

void QmlProfilerTool::showLoadDialog()
{
    if (!checkForUnsavedNotes())
        return;

    if (ModeManager::currentMode()->id() != MODE_ANALYZE)
        AnalyzerManager::showMode();

    AnalyzerManager::selectAction(QmlProfilerRemoteActionId);

    QString filename = QFileDialog::getOpenFileName(
                ICore::mainWindow(), tr("Load QML Trace"),
                QmlProfilerPlugin::globalSettings()->lastTraceFile(),
                tr("QML traces (*%1)").arg(QLatin1String(TraceFileExtension)));

    if (!filename.isEmpty()) {
        saveLastTraceFile(filename);
        AnalyzerManager::mainWindow()->setEnabled(false);
        connect(d->m_profilerModelManager, &QmlProfilerModelManager::recordedFeaturesChanged,
                this, &QmlProfilerTool::setRecordedFeatures);
        d->m_profilerModelManager->load(filename);
    }
}

void QmlProfilerTool::onLoadSaveFinished()
{
    disconnect(d->m_profilerModelManager, &QmlProfilerModelManager::recordedFeaturesChanged,
               this, &QmlProfilerTool::setRecordedFeatures);
    AnalyzerManager::mainWindow()->setEnabled(true);
}

/*!
    Checks if we have unsaved notes. If so, shows a warning dialog. Returns true if we can continue
    with a potentially destructive operation and discard the warnings, or false if not. We don't
    want to show a save/discard dialog here because that will often result in a confusing series of
    different dialogs: first "save" and then immediately "load" or "connect".
 */
bool QmlProfilerTool::checkForUnsavedNotes()
{
    if (!d->m_profilerModelManager->notesModel()->isModified())
        return true;
    return QMessageBox::warning(QApplication::activeWindow(), tr("QML Profiler"),
                                tr("You are about to discard the profiling data, including unsaved "
                                   "notes. Do you want to continue?"),
                                QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes;
}

void QmlProfilerTool::restoreFeatureVisibility()
{
    // Restore the shown/hidden state of features to what the user selected. When clearing data the
    // the model manager sets its features to 0, and models get automatically shown, for the mockup.
    quint64 features = 0;
    foreach (const QAction *action, d->m_displayFeaturesMenu->actions()) {
        if (action->isChecked())
            features |= (1ULL << action->data().toUInt());
    }
    d->m_profilerModelManager->setVisibleFeatures(features);
}

void QmlProfilerTool::clientsDisconnected()
{
    // If the application stopped by itself, check if we have all the data
    if (d->m_profilerState->currentState() == QmlProfilerStateManager::AppDying ||
            d->m_profilerState->currentState() == QmlProfilerStateManager::Idle) {
        if (d->m_profilerModelManager->state() == QmlProfilerModelManager::AcquiringData) {
            if (d->m_profilerConnections->aggregateTraces()) {
                d->m_profilerModelManager->acquiringDone();
            } else {
                showNonmodalWarning(tr("Application finished before loading profiled data.\n"
                                       "Please use the stop button instead."));
                d->m_profilerModelManager->clear();
            }
        }

        // ... and return to the "base" state
        if (d->m_profilerState->currentState() == QmlProfilerStateManager::AppDying)
            d->m_profilerState->setCurrentState(QmlProfilerStateManager::Idle);
    }
    // If the connection is closed while the app is still running, no special action is needed
}

void addFeatureToMenu(QMenu *menu, ProfileFeature feature, quint64 enabledFeatures)
{
    QAction *action =
            menu->addAction(QmlProfilerTool::tr(QmlProfilerModelManager::featureName(feature)));
    action->setCheckable(true);
    action->setData(static_cast<uint>(feature));
    action->setChecked(enabledFeatures & (1ULL << (feature)));
}

template<ProfileFeature feature>
void QmlProfilerTool::updateFeatures(quint64 features)
{
    if (features & (1ULL << (feature))) {
        addFeatureToMenu(d->m_recordFeaturesMenu, feature,
                         d->m_profilerState->requestedFeatures());
        addFeatureToMenu(d->m_displayFeaturesMenu, feature,
                         d->m_profilerModelManager->visibleFeatures());
    }
    updateFeatures<static_cast<ProfileFeature>(feature + 1)>(features);
}

template<>
void QmlProfilerTool::updateFeatures<MaximumProfileFeature>(quint64 features)
{
    Q_UNUSED(features);
    return;
}

void QmlProfilerTool::setAvailableFeatures(quint64 features)
{
    if (features != d->m_profilerState->requestedFeatures())
        d->m_profilerState->setRequestedFeatures(features); // by default, enable them all.
    if (d->m_recordFeaturesMenu && d->m_displayFeaturesMenu) {
        d->m_recordFeaturesMenu->clear();
        d->m_displayFeaturesMenu->clear();
        updateFeatures<static_cast<ProfileFeature>(0)>(features);
    }
}

void QmlProfilerTool::setRecordedFeatures(quint64 features)
{
    foreach (QAction *action, d->m_displayFeaturesMenu->actions())
        action->setEnabled(features & (1ULL << action->data().toUInt()));
}

void QmlProfilerTool::profilerDataModelStateChanged()
{
    switch (d->m_profilerModelManager->state()) {
    case QmlProfilerModelManager::Empty :
        d->m_recordButton->setEnabled(true);
        setButtonsEnabled(true);
        break;
    case QmlProfilerModelManager::ClearingData :
        d->m_recordButton->setEnabled(false);
        setButtonsEnabled(false);
        clearDisplay();
        break;
    case QmlProfilerModelManager::AcquiringData :
        d->m_recordButton->setEnabled(true); // Press recording button to stop recording
        setButtonsEnabled(false);            // Other buttons disabled
        break;
    case QmlProfilerModelManager::ProcessingData :
        d->m_recordButton->setEnabled(false);
        setButtonsEnabled(false);
        break;
    case QmlProfilerModelManager::Done :
        if (d->m_profilerState->currentState() == QmlProfilerStateManager::AppStopRequested)
            d->m_profilerState->setCurrentState(QmlProfilerStateManager::Idle);
        showSaveOption();
        updateTimeDisplay();
        restoreFeatureVisibility();
        d->m_recordButton->setEnabled(true);
        setButtonsEnabled(true);
    break;
    default:
        break;
    }
}

QList <QAction *> QmlProfilerTool::profilerContextMenuActions() const
{
    QList <QAction *> commonActions;
    commonActions << d->m_loadQmlTrace << d->m_saveQmlTrace;
    return commonActions;
}

void QmlProfilerTool::showNonmodalWarning(const QString &warningMsg)
{
    QMessageBox *noExecWarning = new QMessageBox(ICore::mainWindow());
    noExecWarning->setIcon(QMessageBox::Warning);
    noExecWarning->setWindowTitle(tr("QML Profiler"));
    noExecWarning->setText(warningMsg);
    noExecWarning->setStandardButtons(QMessageBox::Ok);
    noExecWarning->setDefaultButton(QMessageBox::Ok);
    noExecWarning->setModal(false);
    noExecWarning->show();
}

QMessageBox *QmlProfilerTool::requestMessageBox()
{
    return new QMessageBox(ICore::mainWindow());
}

void QmlProfilerTool::handleHelpRequest(const QString &link)
{
    HelpManager::handleHelpRequest(link);
}

void QmlProfilerTool::profilerStateChanged()
{
    switch (d->m_profilerState->currentState()) {
    case QmlProfilerStateManager::AppDying : {
        // If already disconnected when dying, check again that all data was read
        if (!d->m_profilerConnections->isConnected())
            QTimer::singleShot(0, this, &QmlProfilerTool::clientsDisconnected);
        break;
    }
    case QmlProfilerStateManager::Idle :
        // when the app finishes, set recording display to client status
        setRecording(d->m_profilerState->clientRecording());
        break;
    case QmlProfilerStateManager::AppStopRequested:
        // Don't allow toggling the recording while data is loaded when application quits
        if (d->m_profilerState->serverRecording())
            d->m_recordButton->setEnabled(false);
        break;
    default:
        // no special action needed for other states
        break;
    }
}

void QmlProfilerTool::clientRecordingChanged()
{
    // if application is running, display server record changes
    // if application is stopped, display client record changes
    if (d->m_profilerState->currentState() != QmlProfilerStateManager::AppRunning)
        setRecording(d->m_profilerState->clientRecording());
}

void QmlProfilerTool::serverRecordingChanged()
{
    showLoadOption();
    if (d->m_profilerState->currentState() == QmlProfilerStateManager::AppRunning) {
        // clear the old data each time we start a new profiling session
        if (d->m_profilerState->serverRecording()) {
            // We cannot stop it here, so we cannot give the usual yes/no dialog. Show a dialog
            // offering to immediately save the data instead.
            if (d->m_profilerModelManager->notesModel()->isModified() &&
                    QMessageBox::warning(QApplication::activeWindow(), tr("QML Profiler"),
                                         tr("Starting a new profiling session will discard the "
                                            "previous data, including unsaved notes.\nDo you want "
                                            "to save the data first?"),
                                         QMessageBox::Save, QMessageBox::Discard) ==
                    QMessageBox::Save)
                showSaveDialog();

            setRecording(true);
            if (!d->m_profilerConnections->aggregateTraces() ||
                    d->m_profilerModelManager->state() == QmlProfilerModelManager::Done)
                clearData();
            d->m_profilerModelManager->prepareForWriting();
        } else {
            setRecording(false);

            // changes back once loading is finished, see profilerDataModelStateChanged()
            if (!d->m_profilerConnections->aggregateTraces())
                d->m_recordButton->setEnabled(false);
        }
    }
}

void QmlProfilerTool::toggleRequestedFeature(QAction *action)
{
    uint feature = action->data().toUInt();
    if (action->isChecked())
        d->m_profilerState->setRequestedFeatures(
                    d->m_profilerState->requestedFeatures() | (1ULL << feature));
    else
        d->m_profilerState->setRequestedFeatures(
                    d->m_profilerState->requestedFeatures() & (~(1ULL << feature)));
}

void QmlProfilerTool::toggleVisibleFeature(QAction *action)
{
    uint feature = action->data().toUInt();
    if (action->isChecked())
        d->m_profilerModelManager->setVisibleFeatures(
                    d->m_profilerModelManager->visibleFeatures() | (1ULL << feature));
    else
        d->m_profilerModelManager->setVisibleFeatures(
                    d->m_profilerModelManager->visibleFeatures() & (~(1ULL << feature)));
}

}
}
