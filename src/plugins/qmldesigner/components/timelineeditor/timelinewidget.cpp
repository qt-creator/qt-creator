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
#include "bindingproperty.h"
#include "curvesegment.h"
#include "easingcurve.h"
#include "easingcurvedialog.h"
#include "timelineconstants.h"
#include "timelinegraphicsscene.h"
#include "timelineicons.h"
#include "timelinepropertyitem.h"
#include "timelinetoolbar.h"
#include "timelineview.h"
#include "navigation2d.h"

#include <qmldesignerplugin.h>
#include <qmlstate.h>
#include <qmltimeline.h>

#include <coreplugin/icore.h>

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
#include <QSpacerItem>
#include <QVariantAnimation>

#include <cmath>

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
    , m_scrollbar(new Navigation2dScrollBar(this))
    , m_statusBar(new QLabel(this))
    , m_timelineView(view)
    , m_graphicsScene(new TimelineGraphicsScene(this))
    , m_addButton(new QPushButton(this))
    , m_onboardingContainer(new QWidget(this))
    , m_loopPlayback(false)
    , m_playbackSpeed(1)
    , m_playbackAnimation(new QVariantAnimation(this))
{
    setWindowTitle(tr("Timeline", "Title of timeline view"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    const QString css = Theme::replaceCssColors(QString::fromUtf8(
        Utils::FileReader::fetchQrc(":/qmldesigner/scrollbar.css")));

    m_scrollbar->setStyleSheet(css);
    m_scrollbar->setOrientation(Qt::Horizontal);

    QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Preferred);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(m_graphicsView->sizePolicy().hasHeightForWidth());

    m_rulerView->setObjectName("RulerView");
    m_rulerView->setFixedHeight(TimelineConstants::rulerHeight);
    m_rulerView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_rulerView->viewport()->installEventFilter(new Eventfilter(this));
    m_rulerView->viewport()->setFocusPolicy(Qt::NoFocus);
    m_rulerView->setStyleSheet(css);
    m_rulerView->setFrameShape(QFrame::NoFrame);
    m_rulerView->setFrameShadow(QFrame::Plain);
    m_rulerView->setLineWidth(0);
    m_rulerView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_rulerView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_rulerView->setScene(graphicsScene());

    m_graphicsView->setStyleSheet(css);
    m_graphicsView->setObjectName("SceneView");
    m_graphicsView->setFrameShape(QFrame::NoFrame);
    m_graphicsView->setFrameShadow(QFrame::Plain);
    m_graphicsView->setLineWidth(0);
    m_graphicsView->setVerticalScrollBar(new Navigation2dScrollBar);
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

    m_addButton->setIcon(TimelineIcons::ADD_TIMELINE_TOOLBAR.icon());
    m_addButton->setToolTip(tr("Add Timeline"));
    m_addButton->setFlat(true);
    m_addButton->setFixedSize(32, 32);


    widgetLayout->addWidget(m_onboardingContainer);

    auto *onboardingTopLabel = new QLabel(m_onboardingContainer);
    auto *onboardingBottomLabel = new QLabel(m_onboardingContainer);
    auto *onboardingBottomIcon = new QLabel(m_onboardingContainer);

    auto *onboardingLayout = new QVBoxLayout;
    auto *onboardingSublayout = new QHBoxLayout;
    auto *leftSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    auto *rightSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    auto *topSpacer = new QSpacerItem(40, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);
    auto *bottomSpacer = new QSpacerItem(40, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);

    const QString labelText =
            tr("This file does not contain a timeline. <br><br>"
               "To create an animation, add a timeline by clicking the + button.");

    onboardingTopLabel->setText(labelText);
    onboardingTopLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    m_onboardingContainer->setLayout(onboardingLayout);
    onboardingLayout->setContentsMargins(0, 0, 0, 0);
    onboardingLayout->setSpacing(0);
    onboardingLayout->addSpacerItem(topSpacer);
    onboardingLayout->addWidget(onboardingTopLabel);
    onboardingLayout->addLayout(onboardingSublayout);

    onboardingSublayout->setContentsMargins(0, 0, 0, 0);
    onboardingSublayout->setSpacing(0);
    onboardingSublayout->addSpacerItem(leftSpacer);

    onboardingBottomLabel->setAlignment(Qt::AlignRight | Qt::AlignTop);
    onboardingBottomLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    onboardingSublayout->addWidget(onboardingBottomLabel);
    onboardingBottomLabel->setText(tr("To edit the timeline settings, click "));

    onboardingBottomIcon->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    onboardingBottomIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    onboardingSublayout->addWidget(onboardingBottomIcon);
    onboardingBottomIcon->setPixmap(TimelineIcons::ANIMATION.pixmap());

    onboardingSublayout->addSpacerItem(rightSpacer);
    onboardingLayout->addSpacerItem(bottomSpacer);

    widgetLayout->addLayout(contentLayout);
    this->setLayout(widgetLayout);

    {
        QPalette timelinePalette;
        timelinePalette.setColor(QPalette::Text, Utils::creatorTheme()->color(
                                     Utils::Theme::DStextColor));
        timelinePalette.setColor(QPalette::WindowText, timelinePalette.color(QPalette::Text));
        timelinePalette.setColor(QPalette::Window, Utils::creatorTheme()->color(
                                     Utils::Theme::QmlDesigner_BackgroundColorDarkAlternate));

        onboardingTopLabel->setPalette(timelinePalette);
        onboardingBottomLabel->setPalette(timelinePalette);
        setPalette(timelinePalette);
        setAutoFillBackground(true);
        m_graphicsView->setPalette(timelinePalette);
        m_graphicsView->setBackgroundRole(QPalette::Window);
        m_statusBar->setPalette(timelinePalette);
    }

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

    Navigation2dFilter *filter = new Navigation2dFilter(this, m_scrollbar);
    connect(filter, &Navigation2dFilter::zoomChanged, [this](double scale, const QPointF& pos) {
        int s = static_cast<int>(std::round(scale*100.));
        double ps = m_graphicsScene->mapFromScene(pos.x());
        m_graphicsScene->setZoom(std::clamp(m_graphicsScene->zoom() + s, 0, 100), ps);
    });
    installEventFilter(filter);

    m_playbackAnimation->stop();
    auto playAnimation = [this](QVariant frame) { graphicsScene()->setCurrentFrame(qRound(frame.toDouble())); };
    connect(m_playbackAnimation, &QVariantAnimation::valueChanged, playAnimation);

    auto updatePlaybackLoopValues = [this]() {
        updatePlaybackValues();
    };
    connect(graphicsScene()->layoutRuler(), &TimelineRulerSectionItem::playbackLoopValuesChanged, updatePlaybackLoopValues);

    auto setPlaybackState = [this](QAbstractAnimation::State newState, QAbstractAnimation::State oldState) {
        Q_UNUSED(oldState)
        m_toolbar->setPlayState(newState == QAbstractAnimation::State::Running);
    };
    connect(m_playbackAnimation, &QVariantAnimation::stateChanged, setPlaybackState);

    auto onFinish = [this]() { graphicsScene()->setCurrentFrame(m_playbackAnimation->startValue().toInt()); };
    connect(m_playbackAnimation, &QVariantAnimation::finished, onFinish);
}

void TimelineWidget::connectToolbar()
{
    connect(graphicsScene(),
            &TimelineGraphicsScene::selectionChanged,
            this,
            &TimelineWidget::selectionChanged);

    connect(graphicsScene(), &TimelineGraphicsScene::scroll, this, &TimelineWidget::scroll);

    auto setZoomFactor = [this](int val) { m_graphicsScene->setZoom(val); };
    connect(m_toolbar, &TimelineToolBar::scaleFactorChanged, setZoomFactor);

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

    connect(m_toolbar, &TimelineToolBar::recordToggled, this, &TimelineWidget::setTimelineRecording);

    connect(m_toolbar, &TimelineToolBar::playTriggered, this, &TimelineWidget::toggleAnimationPlayback);

    auto setLoopAnimation = [this](bool loop) {
        graphicsScene()->layoutRuler()->setPlaybackLoopEnabled(loop);
        if (m_playbackAnimation->state() == QAbstractAnimation::Running)
            m_playbackAnimation->pause();
        m_loopPlayback = loop;
    };
    connect(m_toolbar, &TimelineToolBar::loopPlaybackToggled, setLoopAnimation);
    auto setPlaybackSpeed = [this](float val) {
        m_playbackSpeed = val;
        updatePlaybackValues();
    };
    connect(m_toolbar, &TimelineToolBar::playbackSpeedChanged, setPlaybackSpeed);

    connect(m_toolbar,
            &TimelineToolBar::openEasingCurveEditor,
            this,
            &TimelineWidget::openEasingCurveEditor);

    connect(m_toolbar,
            &TimelineToolBar::settingDialogClicked,
            m_timelineView,
            &TimelineView::openSettingsDialog);
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
        EasingCurveDialog::runDialog(frames, Core::ICore::dialogParent());
    }
}

void TimelineWidget::updatePlaybackValues()
{
    QmlTimeline currentTimeline = graphicsScene()->currentTimeline();
    qreal endFrame = currentTimeline.endKeyframe();
    qreal startFrame = currentTimeline.startKeyframe();
    qreal duration = currentTimeline.duration();

    if (m_loopPlayback) {
        m_playbackAnimation->setLoopCount(-1);
        qreal loopStart = graphicsScene()->layoutRuler()->playbackLoopStart();
        qreal loopEnd = graphicsScene()->layoutRuler()->playbackLoopEnd();
        startFrame = qRound(startFrame + loopStart);
        endFrame = qRound(startFrame + (loopEnd - loopStart));
        duration = endFrame - startFrame;
    } else {
        m_playbackAnimation->setLoopCount(1);
    }

    if (duration > 0.0f) {
        qreal animationDuration = duration * (1.0 / m_playbackSpeed);
        m_playbackAnimation->setDuration(animationDuration);
    } else {
        if (m_playbackAnimation->state() == QAbstractAnimation::Running)
            m_playbackAnimation->stop();
    }
    qreal newCurrentTime = (currentTimeline.currentKeyframe() - startFrame) * (1.0 / m_playbackSpeed);
    if (qRound(m_playbackAnimation->startValue().toDouble()) != qRound(startFrame)
        || qRound(m_playbackAnimation->endValue().toDouble()) != qRound(endFrame)) {
        newCurrentTime = 0;
    }
    m_playbackAnimation->setStartValue(qRound(startFrame));
    m_playbackAnimation->setEndValue(qRound(endFrame));
    m_playbackAnimation->setCurrentTime(newCurrentTime);
}

void TimelineWidget::toggleAnimationPlayback()
{
    QmlTimeline currentTimeline = graphicsScene()->currentTimeline();
    if (currentTimeline.isValid() && m_playbackSpeed > 0.0) {
        if (m_playbackAnimation->state() == QAbstractAnimation::Running) {
            m_playbackAnimation->pause();
        } else {
            updatePlaybackValues();
            m_playbackAnimation->start();
        }
    }
}

void TimelineWidget::setTimelineRecording(bool value)
{
    ModelNode node = timelineView()->modelNodeForId(m_toolbar->currentTimelineId());

    if (value) {
        timelineView()->activateTimelineRecording(node);
    } else {
        timelineView()->deactivateTimelineRecording();
        timelineView()->setCurrentTimeline(node);
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
    if (currentTimeline.isValid()) {
        setTimelineId(currentTimeline.modelNode().id());
        m_statusBar->setText(
            tr(TimelineConstants::statusBarPlayheadFrame).arg(getcurrentFrame(currentTimeline)));
    } else {
        setTimelineId({});
        m_statusBar->clear();
    }

    invalidateTimelineDuration(currentTimeline);

    m_graphicsScene->setWidth(m_graphicsView->viewport()->width());

    // setScaleFactor uses QSignalBlocker.
    m_toolbar->setScaleFactor(0);
    m_graphicsScene->setZoom(0);
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
        QmlTimeline currentTimeline = timelineView()->currentTimeline();
        if (currentTimeline.isValid() && currentTimeline == timeline) {
            m_toolbar->setStartFrame(timeline.startKeyframe());
            m_toolbar->setEndFrame(timeline.endKeyframe());
            graphicsScene()->setTimeline(timeline);

            qreal playHeadFrame = getcurrentFrame(timeline);
            if (playHeadFrame < timeline.startKeyframe())
                playHeadFrame = timeline.startKeyframe();
            else if (playHeadFrame > timeline.endKeyframe())
                playHeadFrame = timeline.endKeyframe();

            /* We have to set the current frame asynchronously,
             * because callbacks are not supposed to mutate the model. */
            QTimer::singleShot(0, [this, playHeadFrame] {
                graphicsScene()->setCurrentFrame(playHeadFrame);
            });
        }
    }
}

void TimelineWidget::invalidateTimelinePosition(const QmlTimeline &timeline)
{
    if (timelineView() && timelineView()->model()) {
        QmlTimeline currentTimeline = timelineView()->currentTimeline();
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
    const bool empty = m_timelineView->getTimelines().isEmpty();
    setTimelineActive(!empty);
    if (m_timelineView->isAttached() && !empty) {
        m_toolbar->setCurrentTimeline(m_timelineView->modelNodeForId(id));
        m_toolbar->setCurrentState(m_timelineView->currentState().name());
    } else {
        m_toolbar->setCurrentTimeline({});
        m_toolbar->setCurrentState({});
    }
    m_timelineView->setTimelineRecording(false);
}

void TimelineWidget::setTimelineActive(bool b)
{
    if (b) {
        m_toolbar->setVisible(true);
        m_graphicsView->setVisible(true);
        m_rulerView->setVisible(true);
        m_scrollbar->setVisible(true);
        m_addButton->setVisible(false);
        m_onboardingContainer->setVisible(false);
        m_graphicsView->update();
        m_rulerView->update();
    } else {
        m_toolbar->setVisible(false);
        m_graphicsView->setVisible(false);
        m_rulerView->setVisible(false);
        m_scrollbar->setVisible(false);
        m_statusBar->clear();
        m_addButton->setVisible(true);
        m_onboardingContainer->setVisible(true);
    }
}

void TimelineWidget::setFocus()
{
    m_graphicsView->setFocus();
}

void TimelineWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)

    /*
    m_timelineView->setEnabled(true);
     TODO See QDS-4191
    */

    if (m_timelineView->model())
        init();

    graphicsScene()->setWidth(m_graphicsView->viewport()->width());
    graphicsScene()->invalidateLayout();
    graphicsScene()->invalidate();
    graphicsScene()->onShow();

    QWidget::showEvent(event);
}

void TimelineWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    graphicsScene()->setWidth(m_graphicsView->viewport()->width());
}

void TimelineWidget::hideEvent(QHideEvent *event)
{
    /* m_timelineView->setEnabled(false); TODO See QDS-4191 */
    QWidget::hideEvent(event);
}

TimelineView *TimelineWidget::timelineView() const
{
    return m_timelineView;
}

} // namespace QmlDesigner
