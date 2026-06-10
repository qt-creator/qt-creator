// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilertraceview.h"

#include "profilertr.h"
#include "qmlprofileranimationsmodel.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilerrangemodel.h"
#include "qmlprofilertool.h"

#include "quick3dmodel.h"
#include "inputeventsmodel.h"
#include "pixmapcachemodel.h"
#include "debugmessagesmodel.h"
#include "memoryusagemodel.h"
#include "scenegraphtimelinemodel.h"

#include <aggregation/aggregate.h>

#include <coreplugin/find/ifindsupport.h>
#include <coreplugin/findplaceholder.h>
#include <tracing/timelinezoomcontrol.h>
#include <tracing/timelinemodelaggregator.h>
#include <tracing/timelinewidget.h>
#include <tracing/timelineformattime.h>

#include <utils/styledbar.h>
#include <utils/algorithm.h>

#include <QApplication>
#include <QContextMenuEvent>
#include <QEvent>
#include <QMenu>
#include <QRegularExpression>
#include <QTextCursor>
#include <QToolButton>
#include <QVBoxLayout>

using namespace QmlDebug;

namespace Profiler::Internal {

class TraceViewFindSupport : public Core::IFindSupport
{
public:
    TraceViewFindSupport(QmlProfilerTraceView *view, QmlProfilerModelManager *manager);

    bool supportsReplace() const override;
    Utils::FindFlags supportedFindFlags() const override;
    void resetIncrementalSearch() override;
    void clearHighlights() override;
    QString currentFindString() const override;
    QString completedFindString() const override;
    Result findIncremental(const QString &txt, Utils::FindFlags findFlags) override;
    Result findStep(const QString &txt, Utils::FindFlags findFlags) override;

private:
    bool find(const QString &txt, Utils::FindFlags findFlags, int start, bool *wrapped);
    bool findOne(const QString &txt, Utils::FindFlags findFlags, int start);

    QmlProfilerTraceView *m_view;
    QmlProfilerModelManager *m_modelManager;
    int m_incrementalStartPos = -1;
    bool m_incrementalWrappedState = false;
    int m_currentPosition = -1;
};

class QmlProfilerTraceView::QmlProfilerTraceViewPrivate
{
public:
    Timeline::TimelineWidget *m_mainView; // Not owned
    QmlProfilerModelManager *m_modelManager; // Not owned

    QList<Timeline::TimelineModel *> m_suspendedModels;
    Timeline::TimelineModelAggregator m_modelProxy;
    Timeline::TimelineZoomControl m_zoomControl;
};

QmlProfilerTraceView::QmlProfilerTraceView(QWidget *parent, QmlProfilerModelManager *modelManager)
    : QWidget(parent), d(new QmlProfilerTraceViewPrivate)
{
    setWindowTitle(Tr::tr("Timeline"));
    setObjectName("QmlProfiler.Timeline.Dock");

    modelManager->registerFeatures(0, [this] {
        if (d->m_suspendedModels.isEmpty()) {
            // Temporarily remove the models, while we're changing them
            d->m_suspendedModels = d->m_modelProxy.models();
            d->m_modelProxy.setModels({});
        }
        // Otherwise models are suspended already. This can happen if either acquiring was
        // aborted or we're doing a "restrict to range" which consists of a partial clearing and
        // then re-acquiring of data.
    }, [this, modelManager]() {
        const qint64 start = modelManager->traceStart();
        const qint64 end = modelManager->traceEnd();
        d->m_zoomControl.setTrace(start, end);
        d->m_zoomControl.setRange(start, start + (end - start) / 10);
        d->m_modelProxy.setModels(d->m_suspendedModels);
        d->m_suspendedModels.clear();
    }, [this] {
        d->m_zoomControl.clear();
        if (!d->m_suspendedModels.isEmpty()) {
            d->m_modelProxy.setModels(d->m_suspendedModels);
            d->m_suspendedModels.clear();
        }
    });

    d->m_modelProxy.setNotes(modelManager->notesModel());
    d->m_modelManager = modelManager;

    QList<Timeline::TimelineModel *> models;
    models.append(new PixmapCacheModel(modelManager, &d->m_modelProxy));
    models.append(new SceneGraphTimelineModel(modelManager, &d->m_modelProxy));
    models.append(new MemoryUsageModel(modelManager, &d->m_modelProxy));
    models.append(new InputEventsModel(modelManager, &d->m_modelProxy));
    models.append(new DebugMessagesModel(modelManager, &d->m_modelProxy));
    models.append(new Quick3DModel(modelManager, &d->m_modelProxy));
    models.append(new QmlProfilerAnimationsModel(modelManager, &d->m_modelProxy));
    for (int i = 0; i < MaximumRangeType; ++i)
        models.append(new QmlProfilerRangeModel(modelManager, (RangeType)i, &d->m_modelProxy));
    d->m_modelProxy.setModels(models);

    d->m_mainView = new Timeline::TimelineWidget(&d->m_modelProxy, &d->m_zoomControl, this);
    setFocusProxy(d->m_mainView);

    Aggregation::aggregate({d->m_mainView, new TraceViewFindSupport(this, modelManager)});

    auto groupLayout = new QVBoxLayout;
    groupLayout->setContentsMargins(0, 0, 0, 0);
    groupLayout->setSpacing(0);
    groupLayout->addWidget(d->m_mainView);
    groupLayout->addWidget(new Core::FindToolBarPlaceHolder(this));
    setLayout(groupLayout);

    connect(d->m_mainView, &Timeline::TimelineWidget::gotoSourceLocation,
            this, &QmlProfilerTraceView::gotoSourceLocation);
    connect(d->m_mainView, &Timeline::TimelineWidget::typeSelected,
            this, &QmlProfilerTraceView::typeSelected);
}

QmlProfilerTraceView::~QmlProfilerTraceView()
{
    delete d->m_mainView;
    delete d;
}

Timeline::RangeDetailsWidget *QmlProfilerTraceView::rangeDetailsWidget() const
{
    return d->m_mainView->rangeDetailsWidget();
}

bool QmlProfilerTraceView::hasValidSelection() const
{
    return d->m_mainView->hasValidSelection();
}

qint64 QmlProfilerTraceView::selectionStart() const
{
    return d->m_mainView->selectionStart();
}

qint64 QmlProfilerTraceView::selectionEnd() const
{
    return d->m_mainView->selectionEnd();
}

void QmlProfilerTraceView::clear()
{
    d->m_mainView->clear();
}

void QmlProfilerTraceView::selectByTypeId(int typeId)
{
    d->m_mainView->selectByTypeId(typeId);
}

void QmlProfilerTraceView::selectByEventIndex(int modelId, int eventIndex)
{
    const int modelIndex = d->m_modelProxy.modelIndexById(modelId);
    QTC_ASSERT(modelIndex != -1, return);
    d->m_mainView->selectByIndices(modelIndex, eventIndex);
}

void QmlProfilerTraceView::updateCursorPosition()
{
    const QString file = d->m_mainView->currentFile();
    if (!file.isEmpty())
        emit gotoSourceLocation(file, d->m_mainView->currentLine(), d->m_mainView->currentColumn());
    emit typeSelected(d->m_mainView->currentTypeId());
}

void QmlProfilerTraceView::mousePressEvent(QMouseEvent *event)
{
    d->m_zoomControl.setWindowLocked(true);
    QWidget::mousePressEvent(event);
}

void QmlProfilerTraceView::mouseReleaseEvent(QMouseEvent *event)
{
    d->m_zoomControl.setWindowLocked(false);
    QWidget::mouseReleaseEvent(event);
}

void QmlProfilerTraceView::contextMenuEvent(QContextMenuEvent *ev)
{
    showContextMenu(ev->globalPos());
}

void QmlProfilerTraceView::showContextMenu(QPoint position)
{
    QMenu menu;
    QAction *viewAllAction = nullptr;

    menu.addActions(QmlProfilerTool::profilerContextMenuActions());
    menu.addSeparator();

    QAction *getLocalStatsAction = menu.addAction(Tr::tr("Analyze Current Range"));
    if (!hasValidSelection())
        getLocalStatsAction->setEnabled(false);

    QAction *getGlobalStatsAction = menu.addAction(Tr::tr("Analyze Full Range"));
    if (!d->m_modelManager->isRestrictedToRange())
        getGlobalStatsAction->setEnabled(false);

    if (d->m_zoomControl.traceDuration() > 0) {
        menu.addSeparator();
        viewAllAction = menu.addAction(Tr::tr("Reset Zoom"));
    }

    QAction *selectedAction = menu.exec(position);

    if (selectedAction) {
        if (selectedAction == viewAllAction) {
            d->m_zoomControl.setRange(d->m_zoomControl.traceStart(),
                                       d->m_zoomControl.traceEnd());
        }
        if (selectedAction == getLocalStatsAction) {
            d->m_modelManager->restrictToRange(selectionStart(), selectionEnd());
        }
        if (selectedAction == getGlobalStatsAction)
            d->m_modelManager->restrictToRange(-1, -1);
    }
}

bool QmlProfilerTraceView::isSuspended() const
{
    return !d->m_suspendedModels.isEmpty();
}


TraceViewFindSupport::TraceViewFindSupport(QmlProfilerTraceView *view,
                                           QmlProfilerModelManager *manager)
    : m_view(view), m_modelManager(manager)
{
}

bool TraceViewFindSupport::supportsReplace() const
{
    return false;
}

Utils::FindFlags TraceViewFindSupport::supportedFindFlags() const
{
    return Utils::FindBackward | Utils::FindCaseSensitively | Utils::FindRegularExpression
            | Utils::FindWholeWords;
}

void TraceViewFindSupport::resetIncrementalSearch()
{
    m_incrementalStartPos = -1;
    m_incrementalWrappedState = false;
}

void TraceViewFindSupport::clearHighlights()
{
}

QString TraceViewFindSupport::currentFindString() const
{
    return QString();
}

QString TraceViewFindSupport::completedFindString() const
{
    return QString();
}

Core::IFindSupport::Result TraceViewFindSupport::findIncremental(const QString &txt,
                                                                 Utils::FindFlags findFlags)
{
    if (m_incrementalStartPos < 0)
        m_incrementalStartPos = qMax(m_currentPosition, 0);
    bool wrapped = false;
    bool found = find(txt, findFlags, m_incrementalStartPos, &wrapped);
    if (wrapped != m_incrementalWrappedState && found) {
        m_incrementalWrappedState = wrapped;
        showWrapIndicator(m_view);
    }
    return found ? Core::IFindSupport::Found : Core::IFindSupport::NotFound;
}

Core::IFindSupport::Result TraceViewFindSupport::findStep(const QString &txt,
                                                          Utils::FindFlags findFlags)
{
    int start = (findFlags & Utils::FindBackward) ? m_currentPosition : m_currentPosition + 1;
    bool wrapped;
    bool found = find(txt, findFlags, start, &wrapped);
    if (wrapped)
        showWrapIndicator(m_view);
    if (found) {
        m_incrementalStartPos = m_currentPosition;
        m_incrementalWrappedState = false;
    }
    return found ? Core::IFindSupport::Found : Core::IFindSupport::NotFound;
}

// "start" is the model index that is searched first in a forward search, i.e. as if the
// "cursor" were between start-1 and start
bool TraceViewFindSupport::find(const QString &txt, Utils::FindFlags findFlags, int start,
                                bool *wrapped)
{
    if (wrapped)
        *wrapped = false;
    if (!findOne(txt, findFlags, start)) {
        int secondStart;
        if (findFlags & Utils::FindBackward)
            secondStart = m_modelManager->notesModel()->count();
        else
            secondStart = 0;
        if (!findOne(txt, findFlags, secondStart))
            return false;
        if (wrapped)
            *wrapped = true;
    }
    return true;
}

// "start" is the model index that is searched first in a forward search, i.e. as if the
// "cursor" were between start-1 and start
bool TraceViewFindSupport::findOne(const QString &txt, Utils::FindFlags findFlags, int start)
{
    bool caseSensitiveSearch = (findFlags & Utils::FindCaseSensitively);
    bool regexSearch = (findFlags & Utils::FindRegularExpression);
    QRegularExpression regexp(regexSearch ? txt : QRegularExpression::escape(txt),
                              caseSensitiveSearch ? QRegularExpression::NoPatternOption
                                                  : QRegularExpression::CaseInsensitiveOption);
    QTextDocument::FindFlags flags;
    if (caseSensitiveSearch)
        flags |= QTextDocument::FindCaseSensitively;
    if (findFlags & Utils::FindWholeWords)
        flags |= QTextDocument::FindWholeWords;
    bool forwardSearch = !(findFlags & Utils::FindBackward);
    int increment = forwardSearch ? +1 : -1;
    int current = forwardSearch ? start : start - 1;
    Timeline::TimelineNotesModel *model = m_modelManager->notesModel();
    while (current >= 0 && current < model->count()) {
        QTextDocument doc(model->text(current)); // for automatic handling of WholeWords option
        if (!doc.find(regexp, 0, flags).isNull()) {
            m_currentPosition = current;
            m_view->selectByEventIndex(model->timelineModel(m_currentPosition),
                                       model->timelineIndex(m_currentPosition));
            QWidget *findBar = QApplication::focusWidget();
            m_view->updateCursorPosition(); // open file/line that belongs to event
            QTC_ASSERT(findBar, return true);
            findBar->setFocus();
            return true;
        }
        current += increment;
    }
    return false;
}

} // namespace Profiler::Internal
