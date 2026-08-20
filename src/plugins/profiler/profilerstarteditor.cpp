// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "profilerstarteditor.h"

#include "profilermode.h"
#include "profilerrecorder.h"
#include "profilersamplerruncontrol.h"
#include "profilertr.h"
#include "profilertracedocument.h"
#include "profilertraceeditor.h"
#include "qmlprofilerconstants.h"
#include "recordingpage.h"
#include "welcomepage.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/idocument.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/runconfiguration.h>

#include <utils/qtcassert.h>
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
            m_recordingPage->start(target);
            setCurrentWidget(m_recordingPage);
        });
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
    // The run mode that records the startup project with the selected backend.
    Id currentRunMode() const
    {
        const int backend = profilerRecorder()->currentBackend();
        return backend >= 0 && backend < m_backendIds.size() ? samplerRunMode(m_backendIds[backend])
                                                             : Id();
    }

    // Describes what the current backend and target would profile, and whether
    // it could run at all.
    void showTarget()
    {
        // The settings that pick a target are the user's to edit only when the
        // target is theirs to choose; the backend's own options -- a sampling
        // interval, the features to record -- apply either way and stay.
        profilerRecorder()->setTargetChosenElsewhere(m_target == StartupProject);
        QWidget *config = profilerRecorder()->createConfigWidget();

        if (m_target == ChosenExecutable) {
            m_welcomePage->setActiveBackend(config);
            m_welcomePage->setStartEnabled(true);
            return;
        }

        // The run control brings the kit and its device along, so it is that
        // which decides whether the project can be profiled this way.
        const Result<> canRun = ProjectExplorerPlugin::canRunStartupProject(currentRunMode());
        RunConfiguration *runConfig = activeRunConfigForActiveProject();

        auto description = new QLabel;
        description->setWordWrap(true);
        if (!canRun)
            description->setText(canRun.error());
        else if (runConfig)
            description->setText(Tr::tr("Profiles \"%1\".").arg(runConfig->displayName()));
        else
            description->setText(Tr::tr("No active project."));

        QWidget *page = description;
        if (config) {
            page = new QWidget;
            auto layout = new QVBoxLayout(page);
            layout->setContentsMargins(0, 0, 0, 0);
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

bool isProfilerStartPageOpen()
{
    const QList<IDocument *> documents = DocumentModel::openedDocuments();
    for (IDocument *document : documents) {
        if (document->id() == Constants::START_EDITOR_ID)
            return true;
    }
    return false;
}

bool hasOpenTrace()
{
    const QList<IDocument *> documents = DocumentModel::openedDocuments();
    for (IDocument *document : documents) {
        if (qobject_cast<ProfilerTraceDocument *>(document))
            return true;
    }
    return false;
}

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
