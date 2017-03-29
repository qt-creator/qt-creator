/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmlprofilertraceview.h"
#include "qmlprofilertool.h"
#include "qmlprofilerstatemanager.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilernotesmodel.h"
#include "qmlprofileranimationsmodel.h"
#include "qmlprofilerrangemodel.h"
#include "qmlprofilerplugin.h"

#include "inputeventsmodel.h"
#include "pixmapcachemodel.h"
#include "debugmessagesmodel.h"
#include "flamegraphview.h"
#include "memoryusagemodel.h"
#include "scenegraphtimelinemodel.h"

// Communication with the other views (limit events to range)
#include "qmlprofilerviewmanager.h"

#include "timeline/timelinezoomcontrol.h"
#include "timeline/timelinemodelaggregator.h"
#include "timeline/timelinerenderer.h"
#include "timeline/timelineoverviewrenderer.h"
#include "timeline/timelinetheme.h"
#include "timeline/timelineformattime.h"

#include <aggregation/aggregate.h>
// Needed for the load&save actions in the context menu
#include <debugger/analyzer/analyzermanager.h>
#include <coreplugin/findplaceholder.h>
#include <utils/styledbar.h>
#include <utils/algorithm.h>

#include <QQmlContext>
#include <QToolButton>
#include <QEvent>
#include <QVBoxLayout>
#include <QGraphicsObject>
#include <QScrollBar>
#include <QSlider>
#include <QMenu>
#include <QQuickItem>
#include <QQuickWidget>
#include <QApplication>
#include <QRegExp>
#include <QTextCursor>

#include <math.h>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerTraceView::QmlProfilerTraceViewPrivate
{
public:
    QmlProfilerTraceViewPrivate(QmlProfilerTraceView *qq) : q(qq) {}
    QmlProfilerTraceView *q;
    QmlProfilerViewManager *m_viewContainer;
    QQuickWidget *m_mainView;
    QmlProfilerModelManager *m_modelManager;
    QVariantList m_suspendedModels;
    Timeline::TimelineModelAggregator *m_modelProxy;
    Timeline::TimelineZoomControl *m_zoomControl;
};

QmlProfilerTraceView::QmlProfilerTraceView(QWidget *parent, QmlProfilerViewManager *container,
                                           QmlProfilerModelManager *modelManager)
    : QWidget(parent), d(new QmlProfilerTraceViewPrivate(this))
{
    setWindowTitle(tr("Timeline"));
    setObjectName("QmlProfiler.Timeline.Dock");

    d->m_zoomControl = new Timeline::TimelineZoomControl(this);
    connect(modelManager, &QmlProfilerModelManager::stateChanged, this, [modelManager, this]() {
        switch (modelManager->state()) {
        case QmlProfilerModelManager::Done: {
            qint64 start = modelManager->traceTime()->startTime();
            qint64 end = modelManager->traceTime()->endTime();
            d->m_zoomControl->setTrace(start, end);
            d->m_zoomControl->setRange(start, start + (end - start) / 10);
            // Fall through
        }
        case QmlProfilerModelManager::Empty:
            d->m_modelProxy->setModels(d->m_suspendedModels);
            d->m_suspendedModels.clear();
            d->m_modelManager->notesModel()->loadData();
            break;
        case QmlProfilerModelManager::ProcessingData:
            break;
        case QmlProfilerModelManager::ClearingData:
            d->m_zoomControl->clear();
            // Fall through
        case QmlProfilerModelManager::AcquiringData:
            // Temporarily remove the models, while we're changing them
            d->m_suspendedModels = d->m_modelProxy->models();
            d->m_modelProxy->setModels(QVariantList());
            break;
        }
    });

    QVBoxLayout *groupLayout = new QVBoxLayout;
    groupLayout->setContentsMargins(0, 0, 0, 0);
    groupLayout->setSpacing(0);

    qmlRegisterType<Timeline::TimelineRenderer>("TimelineRenderer", 1, 0, "TimelineRenderer");
    qmlRegisterType<Timeline::TimelineOverviewRenderer>("TimelineOverviewRenderer", 1, 0,
                                                        "TimelineOverviewRenderer");
    qmlRegisterType<Timeline::TimelineZoomControl>();
    qmlRegisterType<Timeline::TimelineModel>();
    qmlRegisterType<Timeline::TimelineNotesModel>();

    d->m_mainView = new QQuickWidget(this);
    d->m_mainView->setResizeMode(QQuickWidget::SizeRootObjectToView);
    d->m_mainView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setFocusProxy(d->m_mainView);

    Aggregation::Aggregate *agg = new Aggregation::Aggregate;
    agg->add(d->m_mainView);
    agg->add(new TraceViewFindSupport(this, modelManager));

    groupLayout->addWidget(d->m_mainView);
    groupLayout->addWidget(new Core::FindToolBarPlaceHolder(this));
    setLayout(groupLayout);

    d->m_viewContainer = container;
    d->m_modelProxy = new Timeline::TimelineModelAggregator(modelManager->notesModel(), this);
    d->m_modelManager = modelManager;

    QVariantList models;
    models.append(QVariant::fromValue(new PixmapCacheModel(modelManager, d->m_modelProxy)));
    models.append(QVariant::fromValue(new SceneGraphTimelineModel(modelManager, d->m_modelProxy)));
    models.append(QVariant::fromValue(new MemoryUsageModel(modelManager, d->m_modelProxy)));
    models.append(QVariant::fromValue(new InputEventsModel(modelManager, d->m_modelProxy)));
    models.append(QVariant::fromValue(new DebugMessagesModel(modelManager, d->m_modelProxy)));
    models.append(QVariant::fromValue(new QmlProfilerAnimationsModel(modelManager,
                                                                     d->m_modelProxy)));
    for (int i = 0; i < MaximumRangeType; ++i) {
        models.append(QVariant::fromValue(new QmlProfilerRangeModel(modelManager, (RangeType)i,
                                                                    d->m_modelProxy)));
    }
    d->m_modelProxy->setModels(models);

    // Minimum height: 5 rows of 20 pixels + scrollbar of 50 pixels + 20 pixels margin
    setMinimumHeight(170);

    Timeline::TimelineTheme::setupTheme(d->m_mainView->engine());
    Timeline::TimeFormatter::setupTimeFormatter();

    d->m_mainView->rootContext()->setContextProperty(QLatin1String("timelineModelAggregator"),
                                                     d->m_modelProxy);
    d->m_mainView->rootContext()->setContextProperty(QLatin1String("zoomControl"),
                                                     d->m_zoomControl);
    d->m_mainView->setSource(QUrl(QLatin1String("qrc:/timeline/MainView.qml")));

    QQuickItem *rootObject = d->m_mainView->rootObject();
    connect(rootObject, SIGNAL(updateCursorPosition()), this, SLOT(updateCursorPosition()));
}

QmlProfilerTraceView::~QmlProfilerTraceView()
{
    delete d->m_mainView;
    delete d;
}

bool QmlProfilerTraceView::hasValidSelection() const
{
    QQuickItem *rootObject = d->m_mainView->rootObject();
    if (rootObject)
        return rootObject->property("selectionRangeReady").toBool();
    return false;
}

qint64 QmlProfilerTraceView::selectionStart() const
{
    return d->m_zoomControl->selectionStart();
}

qint64 QmlProfilerTraceView::selectionEnd() const
{
    return d->m_zoomControl->selectionEnd();
}

void QmlProfilerTraceView::clear()
{
    QMetaObject::invokeMethod(d->m_mainView->rootObject(), "clear");
}

void QmlProfilerTraceView::selectByTypeId(int typeId)
{
    QQuickItem *rootObject = d->m_mainView->rootObject();
    if (!rootObject)
        return;
    QMetaObject::invokeMethod(rootObject, "selectByTypeId", Q_ARG(QVariant,QVariant(typeId)));
}

void QmlProfilerTraceView::selectByEventIndex(int modelId, int eventIndex)
{
    QQuickItem *rootObject = d->m_mainView->rootObject();
    if (!rootObject)
        return;

    const int modelIndex = d->m_modelProxy->modelIndexById(modelId);
    QTC_ASSERT(modelIndex != -1, return);
    QMetaObject::invokeMethod(rootObject, "selectByIndices",
                              Q_ARG(QVariant, QVariant(modelIndex)),
                              Q_ARG(QVariant, QVariant(eventIndex)));
}

// Goto source location
void QmlProfilerTraceView::updateCursorPosition()
{
    QQuickItem *rootObject = d->m_mainView->rootObject();
    QString file = rootObject->property("fileName").toString();
    if (!file.isEmpty())
        emit gotoSourceLocation(file, rootObject->property("lineNumber").toInt(),
                                rootObject->property("columnNumber").toInt());

    emit typeSelected(rootObject->property("typeId").toInt());
}

void QmlProfilerTraceView::mousePressEvent(QMouseEvent *event)
{
    d->m_zoomControl->setWindowLocked(true);
    QWidget::mousePressEvent(event);
}

void QmlProfilerTraceView::mouseReleaseEvent(QMouseEvent *event)
{
    d->m_zoomControl->setWindowLocked(false);
    QWidget::mouseReleaseEvent(event);
}

void QmlProfilerTraceView::contextMenuEvent(QContextMenuEvent *ev)
{
    showContextMenu(ev->globalPos());
}

void QmlProfilerTraceView::showContextMenu(QPoint position)
{
    QMenu menu;
    QAction *viewAllAction = 0;

    menu.addActions(QmlProfilerTool::profilerContextMenuActions());
    menu.addSeparator();

    QAction *getLocalStatsAction = menu.addAction(tr("Analyze Current Range"));
    if (!hasValidSelection())
        getLocalStatsAction->setEnabled(false);

    QAction *getGlobalStatsAction = menu.addAction(tr("Analyze Full Range"));
    if (!d->m_modelManager->isRestrictedToRange())
        getGlobalStatsAction->setEnabled(false);

    if (d->m_zoomControl->traceDuration() > 0) {
        menu.addSeparator();
        viewAllAction = menu.addAction(tr("Reset Zoom"));
    }

    QAction *selectedAction = menu.exec(position);

    if (selectedAction) {
        if (selectedAction == viewAllAction) {
            d->m_zoomControl->setRange(d->m_zoomControl->traceStart(),
                                       d->m_zoomControl->traceEnd());
        }
        if (selectedAction == getLocalStatsAction) {
            d->m_modelManager->restrictToRange(selectionStart(), selectionEnd());
        }
        if (selectedAction == getGlobalStatsAction)
            d->m_modelManager->restrictToRange(-1, -1);
    }
}

bool QmlProfilerTraceView::isUsable() const
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
    return d->m_mainView->quickWindow()->rendererInterface()->graphicsApi()
            == QSGRendererInterface::OpenGL;
#else
    return true;
#endif
}

void QmlProfilerTraceView::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::EnabledChange) {
        QQuickItem *rootObject = d->m_mainView->rootObject();
        rootObject->setProperty("enabled", isEnabled());
    }
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

Core::FindFlags TraceViewFindSupport::supportedFindFlags() const
{
    return Core::FindBackward | Core::FindCaseSensitively | Core::FindRegularExpression
            | Core::FindWholeWords;
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
                                                                 Core::FindFlags findFlags)
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
                                                          Core::FindFlags findFlags)
{
    int start = (findFlags & Core::FindBackward) ? m_currentPosition : m_currentPosition + 1;
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
bool TraceViewFindSupport::find(const QString &txt, Core::FindFlags findFlags, int start,
                                bool *wrapped)
{
    if (wrapped)
        *wrapped = false;
    if (!findOne(txt, findFlags, start)) {
        int secondStart;
        if (findFlags & Core::FindBackward)
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
bool TraceViewFindSupport::findOne(const QString &txt, Core::FindFlags findFlags, int start)
{
    bool caseSensitiveSearch = (findFlags & Core::FindCaseSensitively);
    QRegExp regexp(txt);
    regexp.setPatternSyntax((findFlags & Core::FindRegularExpression) ? QRegExp::RegExp : QRegExp::FixedString);
    regexp.setCaseSensitivity(caseSensitiveSearch ? Qt::CaseSensitive : Qt::CaseInsensitive);
    QTextDocument::FindFlags flags;
    if (caseSensitiveSearch)
        flags |= QTextDocument::FindCaseSensitively;
    if (findFlags & Core::FindWholeWords)
        flags |= QTextDocument::FindWholeWords;
    bool forwardSearch = !(findFlags & Core::FindBackward);
    int increment = forwardSearch ? +1 : -1;
    int current = forwardSearch ? start : start - 1;
    QmlProfilerNotesModel *model = m_modelManager->notesModel();
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

} // namespace Internal
} // namespace QmlProfiler
