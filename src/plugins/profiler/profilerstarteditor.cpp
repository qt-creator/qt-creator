// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "profilerstarteditor.h"

#include "profilermode.h"
#include "profilerrecorder.h"
#include "profilersamplerruncontrol.h"
#include "profilertr.h"
#include "profilertraceeditor.h"
#include "qmlprofilerconstants.h"
#include "recordingpage.h"
#include "sampler.h"
#include "welcomepage.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/idocument.h>

#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runcontrol.h>

#include <utils/infolabel.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QLabel>
#include <QStackedWidget>
#include <QVBoxLayout>

#include <memory>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace Profiler::Internal {

// What a recording runs against. The startup project is what the run button
// would run; the other is an executable the user points the backend at.
enum Target { StartupProject, ChosenExecutable };

// The samplers' run workers record on the desktop only. Devices provide run
// workers for the live profilers' run modes, so a backend that has one of
// those can still record a target on a device through it.
static Id liveRunModeFor(Id backendId)
{
    if (backendId == SamplerIds::Qml)
        return ProjectExplorer::Constants::QML_PROFILER_RUN_MODE;
    if (backendId == SamplerIds::Perf)
        return ProjectExplorer::Constants::PERFPROFILER_RUN_MODE;
    return {};
}

// Whether the startup project's device has a run worker for `runMode`. Unlike
// ProjectExplorerPlugin::canRunStartupProject() this does not change while a
// run is being started or a build is going on.
static bool deviceSupports(Id runMode)
{
    RunConfiguration *runConfig = activeRunConfigForActiveProject();
    return runConfig
           && RunControl::canRun(runMode, RunDeviceTypeKitAspect::deviceTypeId(runConfig->kit()),
                                 runConfig->id(), runConfig->executionType());
}

// The page that starts a run, and the progress of one that is running. They
// share a widget because a recording begins on the first and continues on the
// second; the trace it produces opens as a document of its own.
class ProfilerStartWidget : public QStackedWidget
{
public:
    ProfilerStartWidget()
        : m_backendIds(profilerRecorder()->backendIds())
        , m_welcomePage(new WelcomePage)
        , m_recordingPage(new RecordingPage)
    {
        addWidget(m_welcomePage);
        addWidget(m_recordingPage);

        m_welcomePage->setBackends(profilerRecorder()->backendNames(),
                                   profilerRecorder()->currentBackend());
        // Profiling the project that is open is the common case, so it leads.
        m_welcomePage->setTargets({Tr::tr("The startup project"),
                                   Tr::tr("An executable I choose")}, StartupProject);
        showTarget();

        connect(m_welcomePage, &WelcomePage::backendChanged,
                profilerRecorder(), &ProfilerRecorder::setCurrentBackend);
        connect(profilerRecorder(), &ProfilerRecorder::currentBackendChanged,
                this, [this] { showTarget(); });
        connect(m_welcomePage, &WelcomePage::targetChanged, this, [this](int index) {
            m_target = Target(index);
            showTarget();
        });
        connect(m_welcomePage, &WelcomePage::startRecordingRequested, this, [this] {
            // The startup project goes through Qt Creator's run machinery, which
            // is what brings the run configuration's arguments and environment,
            // the kit's device and any deployment along. A target the user names
            // here has none of that, and the backend launches it itself.
            if (m_target == StartupProject)
                ProjectExplorerPlugin::runStartupProject(currentRunMode());
            else
                profilerRecorder()->start();
        });
        connect(m_recordingPage, &RecordingPage::stopRequested,
                profilerRecorder(), &ProfilerRecorder::stop);

        connect(profilerRecorder(), &ProfilerRecorder::started, this, [this](const QString &target) {
            m_recordingPage->showWaiting(target);
            setCurrentWidget(m_recordingPage);
        });
        connect(profilerRecorder(), &ProfilerRecorder::captureStarted,
                m_recordingPage, &RecordingPage::captureStarted);
        connect(profilerRecorder(), &ProfilerRecorder::processingStarted,
                m_recordingPage, &RecordingPage::setProcessing);
        connect(profilerRecorder(), &ProfilerRecorder::progressChanged,
                m_recordingPage, &RecordingPage::setProgress);
        connect(profilerRecorder(), &ProfilerRecorder::statusChanged,
                m_recordingPage, &RecordingPage::setStatus);
        // Only this page's widgets are driven here. Opening the finished trace
        // and reporting errors is wired up in setupProfilerMode(): the page is
        // closable while a recording runs, so those must not depend on it.
        connect(profilerRecorder(), &ProfilerRecorder::finished, this, [this] {
            m_recordingPage->stop();
            setCurrentWidget(m_welcomePage);
        });
        connect(profilerRecorder(), &ProfilerRecorder::error, this, [this] {
            m_recordingPage->stop();
            setCurrentWidget(m_welcomePage);
        });

        // The page can be closed and reopened while a recording runs, and the
        // recorder outlives it; come back to the recording rather than to a
        // Start button that would do nothing.
        if (profilerRecorder()->isRecording())
            setCurrentWidget(m_recordingPage);

        // What the run button would run may change while this page is open, and
        // with it what profiling the startup project would do.
        connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::runActionsUpdated,
                this, [this] {
            if (m_target == StartupProject)
                showTarget();
        });
    }

private:
    Id currentBackendId() const
    {
        const int backend = profilerRecorder()->currentBackend();
        return backend >= 0 && backend < m_backendIds.size() ? m_backendIds[backend] : Id();
    }

    QString currentBackendName() const
    {
        const QStringList names = profilerRecorder()->backendNames();
        const int backend = profilerRecorder()->currentBackend();
        return backend >= 0 && backend < names.size() ? names[backend] : QString();
    }

    // The run mode that records the startup project with the selected backend.
    Id currentRunMode() const
    {
        const Id backendId = currentBackendId();
        if (!backendId.isValid())
            return {};
        const Id samplerMode = samplerRunMode(backendId);
        const Id liveMode = liveRunModeFor(backendId);
        if (liveMode.isValid() && !deviceSupports(samplerMode) && deviceSupports(liveMode))
            return liveMode;
        return samplerMode;
    }

    // Describes what the current backend and target would profile, and whether
    // it could run at all.
    void showTarget()
    {
        // The settings that pick a target are the user's to edit only when the
        // target is theirs to choose; the backend's own options -- a sampling
        // interval, the features to record -- apply either way and stay, unless
        // a live profiler records the target instead (see below).
        profilerRecorder()->setTargetChosenElsewhere(m_target == StartupProject);

        if (m_target == ChosenExecutable) {
            profilerRecorder()->setOptionsChosenElsewhere(false);
            m_welcomePage->setActiveBackend(profilerRecorder()->createConfigWidget());
            m_welcomePage->setStartEnabled(true);
            return;
        }

        // The run control brings the kit and its device along, so it is that
        // which decides whether the project can be profiled this way.
        const Id backendId = currentBackendId();
        const Id runMode = currentRunMode();
        Result<> canRun = ProjectExplorerPlugin::canRunStartupProject(runMode);
        RunConfiguration *runConfig = activeRunConfigForActiveProject();

        // The run machinery only says that it cannot run the project, not that
        // it is the device that the backend cannot record on.
        if (!canRun && runConfig
            && RunDeviceTypeKitAspect::deviceTypeId(runConfig->kit())
                   != ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE
            && !deviceSupports(runMode)) {
            canRun = ResultError(
                liveRunModeFor(backendId).isValid()
                    ? Tr::tr("\"%1\" cannot record applications on the device of the "
                             "kit \"%2\".")
                          .arg(currentBackendName(), runConfig->kit()->displayName())
                    : Tr::tr("\"%1\" records only applications that run on the desktop.")
                          .arg(currentBackendName()));
        }

        // The live profilers that record on a device have settings of their
        // own and would ignore the backend's.
        const bool live = runMode.isValid() && runMode == liveRunModeFor(backendId);
        profilerRecorder()->setOptionsChosenElsewhere(live);
        QWidget *config = profilerRecorder()->createConfigWidget();

        auto description = new InfoLabel;
        description->setElideMode(Qt::ElideNone);
        description->setType(InfoLabelType::Warning);
        description->setWordWrap(true);
        if (!canRun) {
            description->setText(canRun.error());
        } else if (runConfig) {
            description->setText(Tr::tr("Profiles \"%1\".").arg(runConfig->displayName()));
            description->setType(InfoLabelType::None);
        } else {
            description->setText(Tr::tr("No active project."));
        }

        QWidget *page = description;
        if (config) {
            page = new QWidget;
            auto layout = new QVBoxLayout(page);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(StyleHelper::SpacingTokens::GapVL);
            layout->addWidget(description);
            layout->addWidget(config);
        }
        m_welcomePage->setActiveBackend(page);
        m_welcomePage->setStartEnabled(canRun.has_value() && runConfig,
                                       canRun ? QString() : canRun.error());
    }

    const QList<Id> m_backendIds; // Parallel to the backend selector.
    Target m_target = StartupProject;
    WelcomePage *m_welcomePage = nullptr;
    RecordingPage *m_recordingPage = nullptr;
};

// Nothing to load, save or reload: the page is a control surface, not a file.
class ProfilerStartDocument : public IDocument
{
public:
    ProfilerStartDocument()
    {
        setId(Constants::START_EDITOR_ID);
        setPreferredDisplayName(Tr::tr("Profile"));
        // It says nothing about a file, so bringing it back in the next session
        // would only add a tab nobody asked for.
        setTemporary(true);
    }

    Result<> setContents(const QByteArray &contents) override
    {
        QTC_CHECK(contents.isEmpty());
        return ResultOk;
    }

    ReloadBehavior reloadBehavior(ChangeTrigger, ChangeType) const override
    {
        return BehaviorSilent;
    }
};

class ProfilerStartEditor : public IEditor
{
public:
    ProfilerStartEditor()
        : m_document(std::make_unique<ProfilerStartDocument>())
        , m_widget(new ProfilerStartWidget)
    {
        setWidget(m_widget);
        setContext(Context(Constants::C_PROFILER_TRACE_EDITOR, Core::Constants::C_EDITORMANAGER));
        setDuplicateSupported(false);
    }

    ~ProfilerStartEditor() override { delete m_widget; }

    IDocument *document() const override { return m_document.get(); }
    QWidget *toolBar() override { return nullptr; }

private:
    std::unique_ptr<ProfilerStartDocument> m_document;
    ProfilerStartWidget *m_widget = nullptr;
};

class ProfilerStartEditorFactory final : public IEditorFactory
{
public:
    ProfilerStartEditorFactory()
    {
        setId(Constants::START_EDITOR_ID);
        setDisplayName(Tr::tr("Profiler Start Page"));
        setEditorCreator([] { return new ProfilerStartEditor; });
    }
};

static ProfilerStartEditorFactory *s_factory = nullptr;
static QAction *s_openAction = nullptr;

IEditor *openProfilerStartPage()
{
    activateProfilerMode();
    QString title = Tr::tr("Profile");
    // One start page is enough: the unique id raises the open one instead of
    // adding another.
    return EditorManager::openEditorWithContents(Constants::START_EDITOR_ID, &title, {},
                                                 Constants::START_EDITOR_ID,
                                                 EditorManager::DoNotSwitchToDesignMode
                                                     | EditorManager::DoNotSwitchToEditMode);
}

QAction *profilerStartPageAction()
{
    return s_openAction;
}

void setupProfilerStartEditor()
{
    QTC_ASSERT(!s_factory, return);
    s_factory = new ProfilerStartEditorFactory;

    // Entering the mode raises the page, but only reopens it while no trace is
    // open, and the mode button says nothing once the mode is already current.
    // This is what reaches the page in either case.
    s_openAction = new QAction(Utils::Icons::PLUS_TOOLBAR.icon(), Tr::tr("New Recording"));
    s_openAction->setToolTip(Tr::tr("Open the page that starts a profiling run."));
    QObject::connect(s_openAction, &QAction::triggered, &openProfilerStartPage);
    ActionManager::actionContainer(Core::Constants::M_DEBUG_ANALYZER)
        ->addAction(ActionManager::registerAction(s_openAction, Constants::START_EDITOR_ID),
                    Core::Constants::G_ANALYZER_TOOLS);
}

void destroyProfilerStartEditor()
{
    delete s_factory;
    s_factory = nullptr;
    delete s_openAction;
    s_openAction = nullptr;
}

} // namespace Profiler::Internal
