// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ctftracebackend.h"

#include "ctfplainviewmanager.h"
#include "ctftimelinemodel.h"
#include "ctftracemanager.h"
#include "profilertr.h"

#include <coreplugin/progressmanager/taskprogress.h>

#include <tracing/timelinewidget.h>

#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QMenu>
#include <QPointer>
#include <QStringList>
#include <QToolButton>

using namespace Utils;

using namespace std::chrono;

namespace Profiler::Internal {

class CtfTraceBackendPrivate
{
public:
    explicit CtfTraceBackendPrivate(Timeline::RangeDetailsWidget *details)
        : viewManager(details)
    {}

    CtfPlainViewManager viewManager;
    QToolButton restrictToThreadsButton;
    QMenu *restrictToThreadsMenu = new QMenu(&restrictToThreadsButton);
    QToolButton providersButton;
    QMenu *providersMenu = new QMenu(&providersButton);
    QPointer<Timeline::TimelineWidget> traceView;
};

CtfTraceBackend::CtfTraceBackend(Timeline::RangeDetailsWidget *details, QObject *parent)
    : ProfilerTraceBackend(parent)
    , d(new CtfTraceBackendPrivate(details))
{
    d->viewManager.setTaskTreeSetup([](QtTaskTree::QTaskTree &taskTree) {
        (new Core::TaskProgress(&taskTree))->setDisplayName(Tr::tr("Loading Trace"));
    });

    connect(&d->viewManager, &CtfPlainViewManager::error, this, &CtfTraceBackend::error);
    connect(&d->viewManager, &CtfPlainViewManager::gotoSourceLocation, this,
            [this](const QString &file, int line, int column) {
        // The path is the one the machine that produced the trace saw, and a
        // trace recorded elsewhere names files this one does not have.
        const FilePath path = FilePath::fromUserInput(file);
        if (!path.isAbsolutePath() || !path.isReadableFile())
            return;
        emit gotoSourceLocation({path, line, column});
    });
    connect(&d->viewManager, &CtfPlainViewManager::loadFinished, this, [this] {
        updateThreadMenu();
        updateProviderMenu();
        emit loadFinished();
        emit traceChanged();
    });

    StyleHelper::setPanelWidget(&d->restrictToThreadsButton);
    d->restrictToThreadsButton.setIcon(Icons::FILTER.icon());
    d->restrictToThreadsButton.setToolTip(Tr::tr("Restrict to Threads"));
    d->restrictToThreadsButton.setPopupMode(QToolButton::InstantPopup);
    d->restrictToThreadsButton.setProperty(StyleHelper::C_NO_ARROW, true);
    d->restrictToThreadsButton.setMenu(d->restrictToThreadsMenu);
    connect(d->restrictToThreadsMenu, &QMenu::triggered,
            this, &CtfTraceBackend::toggleThreadRestriction);

    // Named rather than given the filter icon the threads have: the two sit
    // next to each other, and one icon twice says which is which to nobody.
    // Shown only for a trace that states providers at all.
    StyleHelper::setPanelWidget(&d->providersButton);
    d->providersButton.setText(Tr::tr("Providers"));
    d->providersButton.setToolTip(Tr::tr("Tracepoint Providers to Show"));
    d->providersButton.setPopupMode(QToolButton::InstantPopup);
    d->providersButton.setMenu(d->providersMenu);
    d->providersButton.hide();
    connect(d->providersMenu, &QMenu::triggered,
            this, &CtfTraceBackend::toggleShownProviders);
}

QList<QWidget *> CtfTraceBackend::toolBarWidgets()
{
    return {&d->restrictToThreadsButton, &d->providersButton};
}

void CtfTraceBackend::updateThreadMenu()
{
    d->restrictToThreadsMenu->clear();
    const QList<CtfTimelineModel *> threads = d->viewManager.traceManager()->getSortedThreads();
    for (CtfTimelineModel *model : threads) {
        QAction *action = d->restrictToThreadsMenu->addAction(model->displayName());
        action->setCheckable(true);
        action->setData(model->tid());
        action->setChecked(d->viewManager.traceManager()->isRestrictedTo(model->tid()));
        action->setEnabled(model->count());
    }
}

void CtfTraceBackend::toggleThreadRestriction(QAction *action)
{
    // Deselect any current event first: next/previous would otherwise act on a
    // different -- or removed -- model.
    if (d->traceView)
        d->traceView->selectByIndices(-1, -1);
    d->viewManager.traceManager()->setThreadRestriction(action->data().toString(),
                                                        action->isChecked());
}

void CtfTraceBackend::updateProviderMenu()
{
    d->providersMenu->clear();
    const QStringList providers = d->viewManager.traceProviders();
    const QStringList shown = d->viewManager.shownProviders();
    // A check mark is what the timeline holds, so a trace as it was opened has
    // all of them. Clearing the last one would leave nothing to look at, and
    // would put the menu back in the state it started in without meaning it,
    // so the only provider left is not offered for clearing.
    const bool theOnlyOne = shown.size() == 1;
    for (const QString &provider : providers) {
        QAction *action = d->providersMenu->addAction(provider);
        action->setCheckable(true);
        action->setData(provider);
        action->setChecked(shown.contains(provider));
        action->setEnabled(!(theOnlyOne && action->isChecked()));
    }
    // A Chrome trace, or a kernel recording, states no provider for any of its
    // events, and a trace of a single provider offers the one entry that can
    // never be cleared: neither is anything to pick from, and a menu that can
    // only say what it says already is a control that does nothing.
    d->providersButton.setVisible(providers.size() > 1);
}

void CtfTraceBackend::toggleShownProviders()
{
    // The whole trace is read again, so nothing that is selected now survives.
    if (d->traceView)
        d->traceView->selectByIndices(-1, -1);

    QStringList providers;
    const QList<QAction *> actions = d->providersMenu->actions();
    for (const QAction *action : actions) {
        if (action->isChecked())
            providers.append(action->data().toString());
    }
    d->viewManager.setShownProviders(providers);
    // A change that is not taken -- one arriving while the trace is being read
    // -- would leave the menu saying something the timeline does not. A change
    // that is taken rebuilds the menu when the trace has been read.
    if (d->viewManager.shownProviders() != providers)
        updateProviderMenu();
}

CtfTraceBackend::~CtfTraceBackend()
{
    delete d;
}

QWidgetList CtfTraceBackend::views(QWidget *parent)
{
    const QWidgetList views = d->viewManager.views(parent);
    d->traceView = qobject_cast<Timeline::TimelineWidget *>(views.value(0));
    return views;
}

void CtfTraceBackend::load(const FilePath &path)
{
    if (path.isDir())
        d->viewManager.loadCtf2(path);
    else
        d->viewManager.loadJson(path);
}

void CtfTraceBackend::clear()
{
    d->viewManager.clear();
}

milliseconds CtfTraceBackend::traceDuration() const
{
    return d->viewManager.traceDuration();
}

} // namespace Profiler::Internal
