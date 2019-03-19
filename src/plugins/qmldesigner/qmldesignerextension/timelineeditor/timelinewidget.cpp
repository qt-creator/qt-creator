/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "timelinewidget.h"
#include "easingcurvedialog.h"
#include "timelineconstants.h"
#include "timelinegraphicsscene.h"
#include "timelineicons.h"
#include "timelinepropertyitem.h"
#include "timelinetoolbar.h"
#include "timelineview.h"

#include <qmldesignerplugin.h>
#include <qmlstate.h>
#include <qmltimeline.h>

#include <theme.h>
#include <utils/algorithm.h>
#include <utils/fileutils.h>

#include <QApplication>
#include <QComboBox>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMargins>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollBar>
#include <QShowEvent>
#include <QSlider>
#include <QVBoxLayout>
#include <QtGlobal>

namespace QmlDesigner {

class Eventfilter : public QObject
{
public:
    Eventfilter(QObject *parent)
        : QObject(parent)
    {}

    bool eventFilter(QObject *, QEvent *event) override
    {
        if (event->type() == QEvent::Wheel) {
            event->accept();
            return true;
        }
        return false;
    }
};

static qreal next(const QVector<qreal> &vector, qreal current)
{
    auto iter = std::find_if(vector.cbegin(), vector.cend(), [&](qreal val) {
        return val > current;
    });
    if (iter != vector.end())
        return *iter;
    return current;
}

static qreal previous(const QVector<qreal> &vector, qreal current)
{
    auto iter = std::find_if(vector.rbegin(), vector.rend(), [&](qreal val) {
        return val < current;
    });
    if (iter != vector.rend())
        return *iter;
    return current;
}

static qreal getcurrentFrame(const QmlTimeline &timeline)
{
    if (!timeline.isValid())
        return 0;

    if (timeline.modelNode().hasAuxiliaryData("currentFrame@NodeInstance"))
        return timeline.modelNode().auxiliaryData("currentFrame@NodeInstance").toReal();
    return timeline.currentKeyframe();
}

TimelineWidget::TimelineWidget(TimelineView *view)
    : QWidget()
    , m_toolbar(new TimelineToolBar(this))
    , m_rulerView(new QGraphicsView(this))
    , m_graphicsView(new QGraphicsView(this))
    , m_scrollbar(new QScrollBar(this))
    , m_statusBar(new QLabel(this))
    , m_timelineView(view)
    , m_graphicsScene(new TimelineGraphicsScene(this))
    , m_addButton(new QPushButton(this))
{
    setWindowTitle(tr("Timeline", "Title of timeline view"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    const QString css = Theme::replaceCssColors(QString::fromUtf8(
        Utils::FileReader::fetchQrc(QLatin1String(":/qmldesigner/scrollbar.css"))));

    m_scrollbar->setStyleSheet(css);
    m_scrollbar->setOrientation(Qt::Horizontal);

    QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Preferred);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(m_graphicsView->sizePolicy().hasHeightForWidth());

    m_rulerView->setObjectName("RulerView");
    m_rulerView->setScene(graphicsScene());
    m_rulerView->setFixedHeight(TimelineConstants::rulerHeight);
    m_rulerView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_rulerView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_rulerView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_rulerView->viewport()->installEventFilter(new Eventfilter(this));
    m_rulerView->viewport()->setFocusPolicy(Qt::NoFocus);

    m_graphicsView->setStyleSheet(css);
    m_graphicsView->setObjectName("SceneView");
    m_graphicsView->setFrameShape(QFrame::NoFrame);
    m_graphicsView->setFrameShadow(QFrame::Plain);
    m_graphicsView->setLineWidth(0);
    m_graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_graphicsView->setSizePolicy(sizePolicy1);
    m_graphicsView->setScene(graphicsScene());
    m_graphicsView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_graphicsView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    auto *scrollBarLayout = new QHBoxLayout;
    scrollBarLayout->addSpacing(TimelineConstants::sectionWidth);
    scrollBarLayout->addWidget(m_scrollbar);

    QMargins margins(0, 0, 0, QApplication::style()->pixelMetric(QStyle::PM_LayoutBottomMargin));

    auto *contentLayout = new QVBoxLayout;
    contentLayout->setContentsMargins(margins);
    contentLayout->addWidget(m_rulerView);
    contentLayout->addWidget(m_graphicsView);
    contentLayout->addLayout(scrollBarLayout);
    contentLayout->addWidget(m_statusBar);
    m_statusBar->setIndent(2);
    m_statusBar->setFixedHeight(TimelineConstants::rulerHeight);

    auto *widgetLayout = new QVBoxLayout;
    widgetLayout->setContentsMargins(0, 0, 0, 0);
    widgetLayout->setSpacing(0);
    widgetLayout->addWidget(m_toolbar);
    widgetLayout->addWidget(m_addButton);

    m_addButton->setIcon(TimelineIcons::ADD_TIMELINE.icon());
    m_addButton->setToolTip(tr("Add Timeline"));
    m_addButton->setFlat(true);
    m_addButton->setFixedSize(32, 32);

    widgetLayout->addLayout(contentLayout);
    this->setLayout(widgetLayout);

    connectToolbar();

    auto setScrollOffset = [this]() { graphicsScene()->setScrollOffset(m_scrollbar->value()); };
    connect(m_scrollbar, &QSlider::valueChanged, this, setScrollOffset);

    connect(graphicsScene(),
            &TimelineGraphicsScene::statusBarMessageChanged,
            this,
            [this](const QString &message) { m_statusBar->setText(message); });

    connect(m_addButton, &QPushButton::clicked, this, [this]() {
        m_timelineView->addNewTimelineDialog();
    });
}

void TimelineWidget::connectToolbar()
{
    connect(graphicsScene(),
            &TimelineGraphicsScene::selectionChanged,
            this,
            &TimelineWidget::selectionChanged);

    connect(graphicsScene(), &TimelineGraphicsScene::scroll, this, &TimelineWidget::scroll);

    auto setRulerScaling = [this](int val) { m_graphicsScene->setRulerScaling(val); };
    connect(m_toolbar, &TimelineToolBar::scaleFactorChanged, setRulerScaling);

    auto setToFirstFrame = [this]() {
        graphicsScene()->setCurrentFrame(graphicsScene()->startFrame());
    };
    connect(m_toolbar, &TimelineToolBar::toFirstFrameTriggered, setToFirstFrame);

    auto setToLastFrame = [this]() {
        graphicsScene()->setCurrentFrame(graphicsScene()->endFrame());
    };
    connect(m_toolbar, &TimelineToolBar::toLastFrameTriggered, setToLastFrame);

    auto setToPreviousFrame = [this]() {
        graphicsScene()->setCurrentFrame(adjacentFrame(&previous));
    };
    connect(m_toolbar, &TimelineToolBar::previousFrameTriggered, setToPreviousFrame);

    auto setToNextFrame = [this]() { graphicsScene()->setCurrentFrame(adjacentFrame(&next)); };
    connect(m_toolbar, &TimelineToolBar::nextFrameTriggered, setToNextFrame);

    auto setCurrentFrame = [this](int frame) { graphicsScene()->setCurrentFrame(frame); };
    connect(m_toolbar, &TimelineToolBar::currentFrameChanged, setCurrentFrame);

    auto setStartFrame = [this](int start) { graphicsScene()->setStartFrame(start); };
    connect(m_toolbar, &TimelineToolBar::startFrameChanged, setStartFrame);

    auto setEndFrame = [this](int end) { graphicsScene()->setEndFrame(end); };
    connect(m_toolbar, &TimelineToolBar::endFrameChanged, setEndFrame);


    connect(m_toolbar, &TimelineToolBar::recordToggled,
            this,
            &TimelineWidget::setTimelineRecording);

    connect(m_toolbar,
            &TimelineToolBar::openEasingCurveEditor,
            this,
            &TimelineWidget::openEasingCurveEditor);

    connect(m_toolbar,
            &TimelineToolBar::settingDialogClicked,
            m_timelineView,
            &TimelineView::openSettingsDialog);

    for (auto action : QmlDesignerPlugin::instance()->designerActionManager().designerActions()) {
        if (action->menuId() == "LivePreview") {
            QObject::connect(m_toolbar,
                             &TimelineToolBar::playTriggered,
                             action->action(),
                             [action]() {
                                 action->action()->setChecked(false);
                                 action->action()->triggered(true);
                             });
        }
    }

    setTimelineActive(false);
}

int TimelineWidget::adjacentFrame(const std::function<qreal(const QVector<qreal> &, qreal)> &fun) const
{
    auto positions = graphicsScene()->keyframePositions();
    std::sort(positions.begin(), positions.end());
    return qRound(fun(positions, graphicsScene()->currentFramePosition()));
}

void TimelineWidget::changeScaleFactor(int factor)
{
    m_toolbar->setScaleFactor(factor);
}

void TimelineWidget::scroll(const TimelineUtils::Side &side)
{
    if (side == TimelineUtils::Side::Left)
        m_scrollbar->setValue(m_scrollbar->value() - m_scrollbar->singleStep());
    else if (side == TimelineUtils::Side::Right)
        m_scrollbar->setValue(m_scrollbar->value() + m_scrollbar->singleStep());
}

void TimelineWidget::selectionChanged()
{
    if (graphicsScene()->hasSelection())
        m_toolbar->setActionEnabled("Curve Picker", true);
    else
        m_toolbar->setActionEnabled("Curve Picker", false);
}

void TimelineWidget::openEasingCurveEditor()
{
    if (graphicsScene()->hasSelection()) {
        QList<ModelNode> frames;
        for (auto *item : graphicsScene()->selectedKeyframes())
            frames.append(item->frameNode());
        EasingCurveDialog::runDialog(frames);
    }
}

void TimelineWidget::setTimelineRecording(bool value)
{
    ModelNode node = timelineView()->modelNodeForId(m_toolbar->currentTimelineId());

    if (value) {
        timelineView()->activateTimelineRecording(node);
    } else {
        timelineView()->deactivateTimelineRecording();
        timelineView()->activateTimeline(node);
    }

    graphicsScene()->invalidateRecordButtonsStatus();
}

void TimelineWidget::contextHelp(const Core::IContext::HelpCallback &callback) const
{
    if (timelineView())
        timelineView()->contextHelp(callback);
    else
        callback({});
}

void TimelineWidget::init()
{
    QmlTimeline currentTimeline = m_timelineView->timelineForState(m_timelineView->currentState());
    if (currentTimeline.isValid())
        setTimelineId(currentTimeline.modelNode().id());
    else
        setTimelineId({});

    invalidateTimelineDuration(graphicsScene()->currentTimeline());

    graphicsScene()->setWidth(m_graphicsView->viewport()->width());

    // setScaleFactor uses QSignalBlocker.
    m_toolbar->setScaleFactor(0);
    m_graphicsScene->setRulerScaling(0);
}

void TimelineWidget::reset()
{
    graphicsScene()->clearTimeline();
    m_toolbar->reset();
    m_statusBar->clear();
}

TimelineGraphicsScene *TimelineWidget::graphicsScene() const
{
    return m_graphicsScene;
}

TimelineToolBar *TimelineWidget::toolBar() const
{
    return m_toolbar;
}

void TimelineWidget::invalidateTimelineDuration(const QmlTimeline &timeline)
{
    if (timelineView() && timelineView()->model()) {
        QmlTimeline currentTimeline = graphicsScene()->currentTimeline();
        if (currentTimeline.isValid() && currentTimeline == timeline) {
            m_toolbar->setCurrentTimeline(timeline);
            graphicsScene()->setTimeline(timeline);
            graphicsScene()->setCurrenFrame(timeline, getcurrentFrame(timeline));
        }
    }
}

void TimelineWidget::invalidateTimelinePosition(const QmlTimeline &timeline)
{
    if (timelineView() && timelineView()->model()) {
        QmlTimeline currentTimeline = graphicsScene()->currentTimeline();
        if (currentTimeline.isValid() && currentTimeline == timeline) {
            qreal frame = getcurrentFrame(timeline);
            m_toolbar->setCurrentFrame(frame);
            graphicsScene()->setCurrenFrame(timeline, frame);
        }
    }
}

void TimelineWidget::setupScrollbar(int min, int max, int current)
{
    bool b = m_scrollbar->blockSignals(true);
    m_scrollbar->setMinimum(min);
    m_scrollbar->setMaximum(max);
    m_scrollbar->setValue(current);
    m_scrollbar->setSingleStep((max - min) / 10);
    m_scrollbar->blockSignals(b);
}

void TimelineWidget::setTimelineId(const QString &id)
{
    setTimelineActive(!m_timelineView->getTimelines().isEmpty());
    if (m_timelineView->isAttached()) {
        m_toolbar->setCurrentTimeline(m_timelineView->modelNodeForId(id));
        m_toolbar->setCurrentState(m_timelineView->currentState().name());
        m_timelineView->setTimelineRecording(false);
    }
}

void TimelineWidget::setTimelineActive(bool b)
{
    if (b) {
        m_toolbar->setVisible(true);
        m_graphicsView->setVisible(true);
        m_rulerView->setVisible(true);
        m_scrollbar->setVisible(true);
        m_addButton->setVisible(false);
        m_graphicsView->update();
        m_rulerView->update();
    } else {
        m_toolbar->setVisible(false);
        m_graphicsView->setVisible(false);
        m_rulerView->setVisible(false);
        m_scrollbar->setVisible(false);
        m_addButton->setVisible(true);
    }
}

void TimelineWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    graphicsScene()->setWidth(m_graphicsView->viewport()->width());
    graphicsScene()->invalidateLayout();
    graphicsScene()->invalidate();
    graphicsScene()->onShow();
}

void TimelineWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    graphicsScene()->setWidth(m_graphicsView->viewport()->width());
}

TimelineView *TimelineWidget::timelineView() const
{
    return m_timelineView;
}

} // namespace QmlDesigner
