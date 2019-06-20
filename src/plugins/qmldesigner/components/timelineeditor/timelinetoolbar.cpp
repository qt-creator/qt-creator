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

#include "timelinetoolbar.h"

#include "timelineconstants.h"
#include "timelinegraphicsscene.h"
#include "timelineicons.h"

#include "timelinewidget.h"

#include <designeractionmanager.h>
#include <nodelistproperty.h>
#include <theme.h>
#include <variantproperty.h>
#include <qmltimeline.h>
#include <qmltimelinekeyframegroup.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>

#include <utils/algorithm.h>

#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QResizeEvent>
#include <QSlider>
#include <QIntValidator>

namespace QmlDesigner {

bool isSpacer(QObject *object)
{
    return object->property("spacer_widget").toBool();
}

QWidget *createSpacer()
{
    QWidget *spacer = new QWidget();
    spacer->setProperty("spacer_widget", true);
    return spacer;
}

int controlWidth(QToolBar *bar, QObject *control)
{
    QWidget *widget = nullptr;

    if (auto *action = qobject_cast<QAction *>(control))
        widget = bar->widgetForAction(action);

    if (widget == nullptr)
        widget = qobject_cast<QWidget *>(control);

    if (widget)
        return widget->width();

    return 0;
}

QAction *createAction(const Core::Id &id,
                      const QIcon &icon,
                      const QString &name,
                      const QKeySequence &shortcut)
{
    QString text = QString("%1 (%2)").arg(name).arg(shortcut.toString());

    Core::Context context(TimelineConstants::C_QMLTIMELINE);

    auto *action = new QAction(icon, text);
    auto *command = Core::ActionManager::registerAction(action, id, context);
    command->setDefaultKeySequence(shortcut);

    return action;
}

TimelineToolBar::TimelineToolBar(QWidget *parent)
    : QToolBar(parent)
    , m_grp()
{
    setContentsMargins(0, 0, 0, 0);
    createLeftControls();
    createCenterControls();
    createRightControls();
}

void TimelineToolBar::reset()
{
    if (recording())
        m_recording->setChecked(false);
}

bool TimelineToolBar::recording() const
{
    if (m_recording)
        return m_recording->isChecked();

    return false;
}

int TimelineToolBar::scaleFactor() const
{
    if (m_scale)
        return m_scale->value();
    return 0;
}

QString TimelineToolBar::currentTimelineId() const
{
    return m_timelineLabel->text();
}

void TimelineToolBar::setCurrentState(const QString &name)
{
    if (name.isEmpty())
        m_stateLabel->setText(tr("Base State"));
    else
        m_stateLabel->setText(name);
}

void TimelineToolBar::setCurrentTimeline(const QmlTimeline &timeline)
{
    if (timeline.isValid()) {
        setStartFrame(timeline.startKeyframe());
        setEndFrame(timeline.endKeyframe());
        m_timelineLabel->setText(timeline.modelNode().id());
    } else {
        m_timelineLabel->setText("");
    }
}

void TimelineToolBar::setStartFrame(qreal frame)
{
    auto text = QString::number(frame, 'f', 0);
    m_firstFrame->setText(text);
    setupCurrentFrameValidator();
}

void TimelineToolBar::setCurrentFrame(qreal frame)
{
    auto text = QString::number(frame, 'f', 0);
    m_currentFrame->setText(text);
}

void TimelineToolBar::setEndFrame(qreal frame)
{
    auto text = QString::number(frame, 'f', 0);
    m_lastFrame->setText(text);
    setupCurrentFrameValidator();
}

void TimelineToolBar::setScaleFactor(int factor)
{
    const QSignalBlocker blocker(m_scale);
    m_scale->setValue(factor);
}

void TimelineToolBar::setActionEnabled(const QString &name, bool enabled)
{
    for (auto *action : actions())
        if (action->objectName() == name)
            action->setEnabled(enabled);
}

void TimelineToolBar::removeTimeline(const QmlTimeline &timeline)
{
    if (timeline.modelNode().id() == m_timelineLabel->text())
        setCurrentTimeline(QmlTimeline());
}

void TimelineToolBar::createLeftControls()
{
    auto addActionToGroup = [&](QAction *action) {
        addAction(action);
        m_grp << action;
    };

    auto addWidgetToGroup = [&](QWidget *widget) {
        addWidget(widget);
        m_grp << widget;
    };

    auto addSpacingToGroup = [&](int width) {
        auto *widget = new QWidget;
        widget->setFixedWidth(width);
        addWidget(widget);
        m_grp << widget;
    };

    addSpacingToGroup(5);

    auto *settingsAction = createAction(TimelineConstants::C_SETTINGS,
                                        TimelineIcons::ANIMATION.icon(),
                                        tr("Timeline Settings"),
                                        QKeySequence(Qt::Key_S));

    connect(settingsAction, &QAction::triggered, this, &TimelineToolBar::settingDialogClicked);

    addActionToGroup(settingsAction);

    addWidgetToGroup(createSpacer());

    m_timelineLabel = new QLabel(this);
    m_timelineLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    addWidgetToGroup(m_timelineLabel);
}

static QLineEdit *createToolBarLineEdit(QWidget *parent)
{
    auto lineEdit = new QLineEdit(parent);
    lineEdit->setStyleSheet("* { background-color: rgba(0, 0, 0, 0); }");
    lineEdit->setFixedWidth(48);
    lineEdit->setAlignment(Qt::AlignCenter);

    QPalette pal = parent->palette();
    pal.setColor(QPalette::Text, Theme::instance()->color(Utils::Theme::PanelTextColorLight));
    lineEdit->setPalette(pal);
    QValidator *validator = new QIntValidator(-100000, 100000, lineEdit);
    lineEdit->setValidator(validator);

    return lineEdit;
}

void TimelineToolBar::createCenterControls()
{
    addSpacing(5);

    auto *toStart = createAction(TimelineConstants::C_TO_START,
                                 TimelineIcons::TO_FIRST_FRAME.icon(),
                                 tr("To Start"),
                                 QKeySequence(Qt::Key_Home));

    connect(toStart, &QAction::triggered, this, &TimelineToolBar::toFirstFrameTriggered);
    addAction(toStart);

    addSpacing(2);

    auto *previous = createAction(TimelineConstants::C_PREVIOUS,
                                  TimelineIcons::BACK_ONE_FRAME.icon(),
                                  tr("Previous"),
                                  QKeySequence(Qt::Key_Comma));

    connect(previous, &QAction::triggered, this, &TimelineToolBar::previousFrameTriggered);
    addAction(previous);

    addSpacing(2);

    auto *play = createAction(TimelineConstants::C_PLAY,
                              TimelineIcons::START_PLAYBACK.icon(),
                              tr("Play"),
                              QKeySequence(Qt::Key_Space));

    connect(play, &QAction::triggered, this, &TimelineToolBar::playTriggered);
    addAction(play);

    addSpacing(2);

    auto *next = createAction(TimelineConstants::C_NEXT,
                              TimelineIcons::FORWARD_ONE_FRAME.icon(),
                              tr("Next"),
                              QKeySequence(Qt::Key_Period));

    connect(next, &QAction::triggered, this, &TimelineToolBar::nextFrameTriggered);
    addAction(next);

    addSpacing(2);

    auto *toEnd = createAction(TimelineConstants::C_TO_END,
                               TimelineIcons::TO_LAST_FRAME.icon(),
                               tr("To End"),
                               QKeySequence(Qt::Key_End));

    connect(toEnd, &QAction::triggered, this, &TimelineToolBar::toLastFrameTriggered);
    addAction(toEnd);

#if 0
    auto *loop = new QAction(TimelineIcons::LOOP_PLAYBACK.icon(), tr("Loop"), this);
    addAction(loop);
#endif

    addSpacing(5);

    addSeparator();

    m_currentFrame = createToolBarLineEdit(this);
    addWidget(m_currentFrame);

    auto emitCurrentChanged = [this]() { emit currentFrameChanged(m_currentFrame->text().toInt()); };
    connect(m_currentFrame, &QLineEdit::editingFinished, emitCurrentChanged);

    addSeparator();

    addSpacing(10);

    QIcon autoKeyIcon = TimelineUtils::mergeIcons(TimelineIcons::GLOBAL_RECORD_KEYFRAMES,
                                                  TimelineIcons::GLOBAL_RECORD_KEYFRAMES_OFF);

    m_recording = createAction(TimelineConstants::C_AUTO_KEYFRAME,
                               autoKeyIcon,
                               tr("Auto Key"),
                               QKeySequence(Qt::Key_K));

    m_recording->setCheckable(true);
    connect(m_recording, &QAction::toggled, [&](bool value) { emit recordToggled(value); });

    addAction(m_recording);

    addSpacing(10);

    addSeparator();

    addSpacing(10);

    auto *curvePicker = createAction(TimelineConstants::C_CURVE_PICKER,
                                     TimelineIcons::CURVE_EDITOR.icon(),
                                     tr("Curve Picker"),
                                     QKeySequence(Qt::Key_C));

    curvePicker->setObjectName("Curve Picker");
    connect(curvePicker, &QAction::triggered, this, &TimelineToolBar::openEasingCurveEditor);
    addAction(curvePicker);

    addSpacing(10);

#if 0
    addSeparator();

    addSpacing(10);

    auto *curveEditor = new QAction(TimelineIcons::CURVE_PICKER.icon(), tr("Curve Editor"));
    addAction(curveEditor);
#endif
}

void TimelineToolBar::createRightControls()
{
    auto *spacer = createSpacer();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    addWidget(spacer);

    addSeparator();

    m_firstFrame = createToolBarLineEdit(this);
    addWidget(m_firstFrame);

    auto emitStartChanged = [this]() { emit startFrameChanged(m_firstFrame->text().toInt()); };
    connect(m_firstFrame, &QLineEdit::editingFinished, emitStartChanged);

    addSeparator();

    addSpacing(10);

    auto *zoomOut = createAction(TimelineConstants::C_ZOOM_OUT,
                                 TimelineIcons::ZOOM_SMALL.icon(),
                                 tr("Zoom Out"),
                                 QKeySequence(QKeySequence::ZoomOut));

    connect(zoomOut, &QAction::triggered, [this]() {
        m_scale->setValue(m_scale->value() - m_scale->pageStep());
    });
    addAction(zoomOut);

    addSpacing(10);

    m_scale = new QSlider(this);
    m_scale->setOrientation(Qt::Horizontal);
    m_scale->setMaximumWidth(200);
    m_scale->setMinimumWidth(100);
    m_scale->setMinimum(0);
    m_scale->setMaximum(100);
    m_scale->setValue(0);

    connect(m_scale, &QSlider::valueChanged, this, &TimelineToolBar::scaleFactorChanged);
    addWidget(m_scale);

    addSpacing(10);

    auto *zoomIn = createAction(TimelineConstants::C_ZOOM_IN,
                                TimelineIcons::ZOOM_BIG.icon(),
                                tr("Zoom In"),
                                QKeySequence(QKeySequence::ZoomIn));

    connect(zoomIn, &QAction::triggered, [this]() {
        m_scale->setValue(m_scale->value() + m_scale->pageStep());
    });
    addAction(zoomIn);

    addSpacing(10);

    addSeparator();

    m_lastFrame = createToolBarLineEdit(this);
    addWidget(m_lastFrame);

    auto emitEndChanged = [this]() { emit endFrameChanged(m_lastFrame->text().toInt()); };
    connect(m_lastFrame, &QLineEdit::editingFinished, emitEndChanged);

    addSeparator();

    m_stateLabel = new QLabel(this);
    m_stateLabel->setFixedWidth(80);
    m_stateLabel->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    addWidget(m_stateLabel);
}

void TimelineToolBar::addSpacing(int width)
{
    auto *widget = new QWidget;
    widget->setFixedWidth(width);
    addWidget(widget);
}

void TimelineToolBar::setupCurrentFrameValidator()
{
    auto validator = static_cast<const QIntValidator*>(m_currentFrame->validator());
    const_cast<QIntValidator*>(validator)->setRange(m_firstFrame->text().toInt(), m_lastFrame->text().toInt());
}

void TimelineToolBar::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)

    int width = 0;
    QWidget *spacer = nullptr;
    for (auto *object : m_grp) {
        if (isSpacer(object))
            spacer = qobject_cast<QWidget *>(object);
        else
            width += controlWidth(this, object);
    }

    if (spacer) {
        int spacerWidth = TimelineConstants::sectionWidth - width - 12;
        spacer->setFixedWidth(spacerWidth > 0 ? spacerWidth : 0);
    }
}

} // namespace QmlDesigner
