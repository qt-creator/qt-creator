// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "profilermode.h"

#include "profilertr.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/icontext.h>
#include <coreplugin/imode.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/navigationwidget.h>
#include <coreplugin/outputpane.h>
#include <coreplugin/perspective.h>
#include <coreplugin/rightpane.h>

#include <utils/fancymainwindow.h>
#include <utils/icon.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>

#include <QMenu>
#include <QPointer>
#include <QVBoxLayout>

using namespace Core;
using namespace Utils;

namespace Profiler::Internal {

const char MODE_PROFILER[]   = "Mode.Profiler";
const char C_PROFILERMODE[]  = "Profiler.ProfilerMode";
const char SETTINGS_GROUP[]  = "Profiler.MainWindow";
const int  P_MODE_PROFILER   = 84; // Between Debug (85) and Projects (83).

static PerspectivesView *theProfilerView = nullptr;
static QPointer<IMode> theProfilerMode;

PerspectivesView *profilerView()
{
    return theProfilerView;
}

// Layout mirrors the Debug mode's central widget, minus the debugger-specific
// engine switchers.
class ProfilerModeWidget final : public MiniSplitter
{
public:
    ProfilerModeWidget()
    {
        PerspectivesView *view = profilerView();
        QTC_ASSERT(view, return);

        auto editorHolderLayout = new QVBoxLayout;
        editorHolderLayout->setContentsMargins(0, 0, 0, 0);
        editorHolderLayout->setSpacing(0);

        auto editorAndFindWidget = new QWidget;
        editorAndFindWidget->setLayout(editorHolderLayout);
        editorHolderLayout->addWidget(view->centralWidgetStack());
        editorHolderLayout->addWidget(new FindToolBarPlaceHolder(editorAndFindWidget));

        auto documentAndRightPane = new MiniSplitter;
        documentAndRightPane->addWidget(editorAndFindWidget);
        documentAndRightPane->addWidget(new RightPanePlaceHolder(MODE_PROFILER));
        documentAndRightPane->setStretchFactor(0, 1);
        documentAndRightPane->setStretchFactor(1, 0);

        auto centralEditorWidget = view->mainWindow()->centralWidget();
        auto centralLayout = new QVBoxLayout(centralEditorWidget);
        centralEditorWidget->setLayout(centralLayout);
        centralLayout->setContentsMargins(0, 0, 0, 0);
        centralLayout->setSpacing(0);
        centralLayout->addWidget(documentAndRightPane);
        centralLayout->setStretch(0, 1);
        centralLayout->setStretch(1, 0);

        // Right-side window with editor, output etc.
        auto mainWindowSplitter = new MiniSplitter;
        mainWindowSplitter->addWidget(view->mainWindow());
        mainWindowSplitter->addWidget(new OutputPanePlaceHolder(MODE_PROFILER, mainWindowSplitter));
        auto outputPane = new OutputPanePlaceHolder(MODE_PROFILER, mainWindowSplitter);
        outputPane->setObjectName("ProfilerOutputPanePlaceHolder");
        mainWindowSplitter->addWidget(outputPane);
        mainWindowSplitter->setStretchFactor(0, 10);
        mainWindowSplitter->setStretchFactor(1, 0);
        mainWindowSplitter->setOrientation(Qt::Vertical);

        // Navigation and right-side window.
        setFocusProxy(view->centralWidgetStack());
        addWidget(new NavigationWidgetPlaceHolder(MODE_PROFILER, Side::Left));
        addWidget(mainWindowSplitter);
        setStretchFactor(0, 0);
        setStretchFactor(1, 1);
        setObjectName("ProfilerModeWidget");

        IContext::attach(this, Context(Core::Constants::C_EDITORMANAGER));
    }
};

class ProfilerMode final : public IMode
{
public:
    ProfilerMode()
    {
        setObjectName("ProfilerMode");
        setContext(Context(C_PROFILERMODE, Core::Constants::C_NAVIGATION_PANE));
        setDisplayName(Tr::tr("Profiler"));
        const Icon flat({{":/profiler/images/mode_profiler_mask.png", Theme::IconsBaseColor}});
        setIcon(Icon::sideBarIcon(flat, flat));
        setPriority(P_MODE_PROFILER);
        setId(MODE_PROFILER);

        setWidgetCreator([] { return new ProfilerModeWidget; });
        setMainWindow(profilerView()->mainWindow());

        setMenu([](QMenu *menu) { profilerView()->addPerspectiveMenu(menu); });
    }
};

void setupProfilerMode()
{
    QTC_ASSERT(!theProfilerView, return);
    theProfilerView = PerspectivesView::createView(QLatin1String(SETTINGS_GROUP),
                                                   C_PROFILERMODE,
                                                   "ProfilerStatusLabel");

    theProfilerMode = new ProfilerMode;

    // Selecting a profiler perspective switches to the Profiler mode ...
    QObject::connect(theProfilerView, &PerspectivesView::modeActivationRequested,
                     theProfilerMode, [] {
        ModeManager::activateMode(MODE_PROFILER);
    });

    // ... and entering/leaving the mode ramps the current perspective up/down.
    QObject::connect(ModeManager::instance(), &ModeManager::currentModeAboutToChange,
                     theProfilerMode, [] {
        if (ModeManager::currentModeId() == MODE_PROFILER)
            profilerView()->leave();
    });
    QObject::connect(ModeManager::instance(), &ModeManager::currentModeChanged,
                     theProfilerMode, [](Utils::Id mode, Utils::Id oldMode) {
        QTC_ASSERT(mode != oldMode, return);
        if (mode == MODE_PROFILER)
            profilerView()->enter();
    });
}

void destroyProfilerMode()
{
    // Delete the view before the mode: the mode widget embeds the view's
    // main window and central widget stack, which hold value-member widgets
    // of the view (chooser, status label, ...). The view's destructor must
    // remove them from the widget tree before the mode widget is deleted,
    // otherwise Qt's parent-child deletion would double-free them. This
    // mirrors the Debugger's shutdown order (doShutdown() before delete mode).
    PerspectivesView::destroyView(theProfilerView);
    theProfilerView = nullptr;
    delete theProfilerMode;
    theProfilerMode = nullptr;
}

} // namespace Profiler::Internal
