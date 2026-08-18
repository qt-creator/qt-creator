// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "profilermode.h"

#include "profilerrecorder.h"
#include "profilerstarteditor.h"
#include "profilertr.h"
#include "profilertracedocument.h"
#include "profilertraceeditor.h"
#include "sampler.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/icontext.h>
#include <coreplugin/imode.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/navigationwidget.h>
#include <coreplugin/outputpane.h>
#include <coreplugin/rightpane.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/runconfiguration.h>

#include <utils/environment.h>
#include <utils/icon.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/stylehelper.h>
#include <utils/widgets.h>
#include <utils/theme/theme.h>

#include <QHBoxLayout>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace Profiler::Internal {

const char MODE_PROFILER[]  = "Mode.Profiler";
const char C_PROFILERMODE[] = "Profiler.ProfilerMode";
const int P_MODE_PROFILER   = 84; // Between Debug (85) and Projects (83).

static QPointer<IMode> theProfilerMode;
static ProfilerRecorder *theRecorder = nullptr;

// Laid out like the Edit mode: the traces, and the page that starts one, are
// documents in the shared editor area, so nothing here has to make room for
// them.
class ProfilerModeWidget final : public MiniSplitter
{
public:
    ProfilerModeWidget()
    {
        auto editorArea = new QWidget;
        auto editorLayout = new QVBoxLayout(editorArea);
        editorLayout->setContentsMargins(0, 0, 0, 0);
        editorLayout->setSpacing(0);
        editorLayout->addWidget(createToolBar());
        editorLayout->addWidget(new EditorManagerPlaceHolder);
        editorLayout->addWidget(new FindToolBarPlaceHolder(editorArea));

        auto editorAndRightPane = new MiniSplitter;
        editorAndRightPane->addWidget(editorArea);
        editorAndRightPane->addWidget(new RightPanePlaceHolder(MODE_PROFILER));
        editorAndRightPane->setStretchFactor(0, 1);
        editorAndRightPane->setStretchFactor(1, 0);

        auto outputPane = new OutputPanePlaceHolder(MODE_PROFILER);
        outputPane->setObjectName("ProfilerOutputPanePlaceHolder");
        auto editorAndOutputPane = new MiniSplitter;
        editorAndOutputPane->setOrientation(Qt::Vertical);
        editorAndOutputPane->addWidget(editorAndRightPane);
        editorAndOutputPane->addWidget(outputPane);
        editorAndOutputPane->setStretchFactor(0, 3);
        editorAndOutputPane->setStretchFactor(1, 0);

        addWidget(new NavigationWidgetPlaceHolder(MODE_PROFILER, Side::Left));
        addWidget(editorAndOutputPane);
        setStretchFactor(0, 0);
        setStretchFactor(1, 1);
        setObjectName("ProfilerModeWidget");
        setFocusProxy(editorArea);

        IContext::attach(this, Context(Core::Constants::C_EDITORMANAGER));
    }

private:
    // The mode's own actions, above the documents rather than in one of them.
    // The mode button only reaches the start page from another mode, and the
    // traces' toolbars belong to a trace, so without this there is no way back
    // to the page from here.
    static QWidget *createToolBar()
    {
        QAction *startPage = profilerStartPageAction();
        QTC_ASSERT(startPage, return new StyledBar);
        auto startPageButton = new QToolButton;
        startPageButton->setDefaultAction(startPage);
        startPageButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        StyleHelper::setPanelWidget(startPageButton);

        auto toolBar = new StyledBar;
        auto layout = new QHBoxLayout(toolBar);
        using namespace StyleHelper::SpacingTokens;
        layout->setContentsMargins(PaddingHS, 0, PaddingHS, 0);
        layout->setSpacing(PrimitiveS);
        layout->addWidget(startPageButton);
        layout->addStretch();
        return toolBar;
    }
};

class ProfilerMode final : public IMode
{
public:
    ProfilerMode()
    {
        setObjectName("ProfilerMode");
        setContext(Context(C_PROFILERMODE, Core::Constants::C_NAVIGATION_PANE));
        setDisplayName(Tr::tr("Profile"));
        const Icon flat({{":/profiler/images/mode_profiler_mask.png", Theme::IconsBaseColor}});
        setIcon(Icon::sideBarIcon(flat, flat));
        setPriority(P_MODE_PROFILER);
        setId(MODE_PROFILER);
        setWidgetCreator([] { return new ProfilerModeWidget; });
    }
};

void activateProfilerMode()
{
    ModeManager::activateMode(MODE_PROFILER);
}

ProfilerRecorder *profilerRecorder()
{
    return theRecorder;
}

// Points the backends at what the run button would run, so starting a
// recording needs no configuration in the common case. Values the user has
// edited are left alone (see ProfilerRecorder::seedLaunchTarget).
static void seedLaunchTarget()
{
    RunConfiguration *runConfig = activeRunConfigForActiveProject();
    if (!runConfig)
        return;
    const ProcessRunData runnable = runConfig->runnable();
    theRecorder->seedLaunchTarget(runnable.command, runnable.workingDirectory,
                                  runnable.environment);
}

// Applies the system change a backend offered next to its failure. pkexec
// prompts for the password in its own dialog, the only elevation that works
// from a GUI with no terminal attached.
static void applySamplerFix(const SamplerFix &fix)
{
    constexpr int pkexecDismissed = 126;

    const Utils::FilePath pkexec = Utils::Environment::systemEnvironment().searchInPath("pkexec");
    if (pkexec.isEmpty()) {
        QMessageBox::warning(Core::ICore::dialogParent(), Tr::tr("Cannot Elevate Privileges"),
                             Tr::tr("\"pkexec\" was not found, so the change cannot be applied "
                                    "from here. Run this as root instead:\n\n    sudo %1")
                                 .arg(fix.command.toUserOutput()));
        return;
    }

    auto *process = new Utils::Process(theRecorder);
    Utils::CommandLine elevated(pkexec);
    elevated.addCommandLineAsArgs(fix.command);
    process->setCommand(elevated);
    QObject::connect(process, &Utils::Process::done, theRecorder, [process, fix] {
        process->deleteLater();
        // The setting is live now; starting the recording again is the only
        // confirmation the user needs.
        if (process->result() == Utils::ProcessResult::FinishedWithSuccess) {
            theRecorder->start();
            return;
        }
        // Dismissing the prompt is a deliberate "no", not a fault worth an
        // error box.
        if (process->exitCode() == pkexecDismissed)
            return;
        const QString details = process->cleanedStdErr().trimmed();
        QMessageBox::warning(
            Core::ICore::dialogParent(), Tr::tr("Could Not Apply Change"),
            details.isEmpty() ? Tr::tr("Running \"%1\" failed.").arg(fix.command.toUserOutput())
                              : Tr::tr("Running \"%1\" failed:\n\n%2")
                                    .arg(fix.command.toUserOutput(), details));
    });
    process->start();
}

void setupProfilerMode()
{
    QTC_ASSERT(!theProfilerMode, return);
    theProfilerMode = new ProfilerMode;

    // Owned here rather than by the mode's widget or the start page: a
    // recording has to survive closing either of them.
    theRecorder = new ProfilerRecorder;

    // For the same reason, what the recorder reports has to land somewhere that
    // lives as long as it does: the page's own handlers only drive its widgets,
    // and a recording that ends while the page is closed would otherwise strand
    // its trace in the temporary directory, and its errors unseen.
    QObject::connect(theRecorder, &ProfilerRecorder::finished, theProfilerMode,
                     [](const Utils::FilePath &tracePath) { openTraceFile(tracePath); });
    QObject::connect(theRecorder, &ProfilerRecorder::error, theProfilerMode,
                     [](const QString &message, const std::optional<SamplerFix> &fix) {
        if (!fix) {
            QMessageBox::warning(Core::ICore::dialogParent(), Tr::tr("Profiler"), message);
            return;
        }
        // The backend named the system change that would make recording work;
        // put it one click away instead of a dead-end warning. Cancel stays the
        // default: pressing Enter should not walk into a password prompt.
        QMessageBox box(QMessageBox::Warning, Tr::tr("Profiler"), message, QMessageBox::Cancel,
                        Core::ICore::dialogParent());
        box.setInformativeText(fix->detail);
        QPushButton *fixButton = box.addButton(fix->buttonText, QMessageBox::ActionRole);
        box.setDefaultButton(QMessageBox::Cancel);
        box.exec();
        if (box.clickedButton() == fixButton)
            applySamplerFix(*fix);
    });

    // Nothing in Creator drives the recorder's settings page the viewer has, so
    // what the start page configures is only written back at shutdown.
    QObject::connect(Core::ICore::instance(), &Core::ICore::saveSettingsRequested,
                     theRecorder, &ProfilerRecorder::writeSettings);
    QObject::connect(ProjectManager::instance(), &ProjectManager::startupProjectChanged,
                     theRecorder, &seedLaunchTarget);
    QObject::connect(ProjectExplorerPlugin::instance(),
                     &ProjectExplorerPlugin::runActionsUpdated, theRecorder, &seedLaunchTarget);
    seedLaunchTarget();

    // Entering the mode raises the page that starts a recording, so the mode
    // button is a reliable way back to it however many traces are open. It is
    // only opened when there is no trace to show instead, so recording one does
    // not bring back a page that was deliberately closed.
    QObject::connect(ModeManager::instance(), &ModeManager::currentModeChanged,
                     theProfilerMode, [](Id mode, Id) {
        if (mode == MODE_PROFILER && (isProfilerStartPageOpen() || !hasOpenTrace()))
            openProfilerStartPage();
    });

    // Closing the last trace leaves nothing to look at; offer a recording again.
    // Closing the page itself is left alone, or it could not be closed at all.
    QObject::connect(EditorManager::instance(), &EditorManager::documentClosed,
                     theProfilerMode, [](IDocument *document) {
        if (qobject_cast<ProfilerTraceDocument *>(document)
                && ModeManager::currentModeId() == MODE_PROFILER && !hasOpenTrace()) {
            openProfilerStartPage();
        }
    });
}

void destroyProfilerMode()
{
    theRecorder->writeSettings();
    delete theRecorder;
    theRecorder = nullptr;
    delete theProfilerMode;
    theProfilerMode = nullptr;
}

} // namespace Profiler::Internal
