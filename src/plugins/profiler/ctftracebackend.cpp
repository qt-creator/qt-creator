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
    connect(&d->viewManager, &CtfPlainViewManager::loadFinished, this, [this] {
        updateThreadMenu();
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
}

QList<QWidget *> CtfTraceBackend::toolBarWidgets()
{
    return {&d->restrictToThreadsButton};
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
