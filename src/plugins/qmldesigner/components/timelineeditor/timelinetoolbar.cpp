// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelinetoolbar.h"

#include "timelineconstants.h"
#include "timelinegraphicsscene.h"
#include "timelineicons.h"

#include "timelineview.h"
#include "timelinewidget.h"

#include <designeractionmanager.h>
#include <nodelistproperty.h>
#include <theme.h>
#include <variantproperty.h>
#include <qmlstate.h>
#include <qmltimeline.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/stylehelper.h>

#include <QApplication>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QResizeEvent>
#include <QSlider>

#include <cmath>

namespace QmlDesigner {

class LineEditDoubleValidator : public QDoubleValidator
{
public:
    LineEditDoubleValidator(double bottom, double top, int decimals, QLineEdit *parent)
        : QDoubleValidator(bottom, top, decimals, parent),
          m_value(1.0) { parent->setText(locale().toString(1.0, 'f', 1)); }
    ~LineEditDoubleValidator() {};

    void fixup(QString &input) const override { input = locale().toString(m_value, 'f', 1); };
    void setValue(double value) { m_value = value; };
private:
    double m_value;
};

static bool isSpacer(QObject *object)
{
    return object->property("spacer_widget").toBool();
}

static QWidget *createSpacer()
{
    QWidget *spacer = new QWidget();
    spacer->setProperty("spacer_widget", true);
    return spacer;
}

static int controlWidth(QToolBar *bar, QObject *control)
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

static QAction *createAction(const Utils::Id &id,
                      const QIcon &icon,
                      const QString &name,
                      const QKeySequence &shortcut)
{
    Core::Context context(TimelineConstants::C_QMLTIMELINE);

    auto *action = new QAction(icon, name);
    auto *command = Core::ActionManager::registerAction(action, id, context);
    command->setDefaultKeySequence(shortcut);
    command->augmentActionWithShortcutToolTip(action);

    return action;
}

TimelineToolBar::TimelineToolBar(QWidget *parent)
    : QToolBar(parent)
    , m_grp()
{
    setContentsMargins(0, 0, 0, 0);
    setFixedHeight(Theme::toolbarSize());
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

void TimelineToolBar::setBlockReflection(bool block)
{
    m_blockReflection = block;
}

void TimelineToolBar::setCurrentTimeline(const QmlTimeline &timeline)
{
    if (m_blockReflection)
        return;

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

void TimelineToolBar::setPlayState(bool state)
{
    m_playing->setChecked(state);
}

void TimelineToolBar::setIsMcu(bool isMcu)
{
    m_curvePicker->setDisabled(isMcu);
    if (isMcu)
        m_curvePicker->setText(tr("Not Supported for MCUs"));
    else
        m_curvePicker->setText(tr(m_curveEditorName));
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
                                        Theme::iconFromName(Theme::Icon::settings_medium),
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
                                 Theme::iconFromName(Theme::Icon::toStartFrame_medium),
                                 tr("To Start"),
                                 QKeySequence(Qt::Key_Home));

    connect(toStart, &QAction::triggered, this, &TimelineToolBar::toFirstFrameTriggered);
    addAction(toStart);

    addSpacing(2);

    auto *previous = createAction(TimelineConstants::C_PREVIOUS,
                                  Theme::iconFromName(Theme::Icon::toPrevFrame_medium),
                                  tr("Previous"),
                                  QKeySequence(Qt::Key_Comma));

    connect(previous, &QAction::triggered, this, &TimelineToolBar::previousFrameTriggered);
    addAction(previous);

    addSpacing(2);
    QIcon playbackIcon = TimelineUtils::mergeIcons(
        Theme::iconFromName(Theme::Icon::pause,
                            Theme::getColor(Theme::Color::DStextSelectedTextColor)),
        Theme::iconFromName(Theme::Icon::playOutline_medium,
                            Theme::getColor(Theme::Color::IconsRunColor)));

    m_playing = createAction(TimelineConstants::C_PLAY,
                             playbackIcon,
                             tr("Play"),
                             QKeySequence(Qt::Key_Space));
    m_playing->setCheckable(true);
    connect(m_playing, &QAction::triggered, this, &TimelineToolBar::playTriggered);
    addAction(m_playing);

    addSpacing(2);

    auto *next = createAction(TimelineConstants::C_NEXT,
                              Theme::iconFromName(Theme::Icon::toNextFrame_medium),
                              tr("Next"),
                              QKeySequence(Qt::Key_Period));

    connect(next, &QAction::triggered, this, &TimelineToolBar::nextFrameTriggered);
    addAction(next);

    addSpacing(2);

    auto *toEnd = createAction(TimelineConstants::C_TO_END,
                               Theme::iconFromName(Theme::Icon::toEndFrame_medium),
                               tr("To End"),
                               QKeySequence(Qt::Key_End));

    connect(toEnd, &QAction::triggered, this, &TimelineToolBar::toLastFrameTriggered);
    addAction(toEnd);

    addSpacing(10);

    addSeparator();

    addSpacing(10);

    auto *loopAnimation = createAction(
        TimelineConstants::C_LOOP_PLAYBACK,
        Theme::iconFromName(Theme::Icon::loopPlayback_medium),
        tr("Loop Playback"),
        QKeySequence((Qt::ControlModifier | Qt::ShiftModifier)
                     + Qt::Key_Space)); // TODO: Toggles looping. Select shortcut for this QDS-4941

    loopAnimation->setCheckable(true);
    connect(loopAnimation, &QAction::toggled, [&](bool value) { emit loopPlaybackToggled(value);} );

    addAction(loopAnimation);

    addSpacing(5);

    m_animationPlaybackSpeed = createToolBarLineEdit(this);
    LineEditDoubleValidator *validator = new LineEditDoubleValidator(0.1, 100.0, 1, m_animationPlaybackSpeed);
    m_animationPlaybackSpeed->setValidator(validator);
    m_animationPlaybackSpeed->setToolTip(tr("Playback Speed"));
    addWidget(m_animationPlaybackSpeed);

    auto emitPlaybackSpeedChanged = [this, validator]() {
        bool ok = false;
        if (double res = validator->locale().toDouble(m_animationPlaybackSpeed->text(), &ok); ok==true) {
            validator->setValue(res);
            m_animationPlaybackSpeed->setText(locale().toString(res, 'f', 1));
            emit playbackSpeedChanged(res);
        }
     };
    connect(m_animationPlaybackSpeed, &QLineEdit::editingFinished, emitPlaybackSpeedChanged);

    addSeparator();

    m_currentFrame = createToolBarLineEdit(this);
    addWidget(m_currentFrame);

    auto emitCurrentChanged = [this]() { emit currentFrameChanged(m_currentFrame->text().toInt()); };
    connect(m_currentFrame, &QLineEdit::editingFinished, emitCurrentChanged);

    addSeparator();

    addSpacing(10);

    QIcon autoKeyIcon = TimelineUtils::mergeIcons(
        Theme::iconFromName(Theme::Icon::recordFill_medium,
                            Theme::getColor(Theme::Color::IconsStopColor)),
        Theme::iconFromName(Theme::Icon::recordOutline_medium));

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

    m_curvePicker = createAction(TimelineConstants::C_CURVE_PICKER,
                                     Theme::iconFromName(Theme::Icon::curveDesigner_medium),
                                     tr(m_curveEditorName),
                                     QKeySequence(Qt::Key_C));

    m_curvePicker->setObjectName("Easing Curve Editor");
    connect(m_curvePicker, &QAction::triggered, this, &TimelineToolBar::openEasingCurveEditor);
    addAction(m_curvePicker);

    addSpacing(10);

#if 0
    addSeparator();

    addSpacing(10);

    auto *curveEditor = new QAction(TimelineIcons::CURVE_PICKER.icon(), tr(m_curveEditorName));
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
                                 Theme::iconFromName(Theme::Icon::zoomOut_medium),
                                 tr("Zoom Out"),
                                 QKeySequence(QKeySequence::ZoomOut));

    connect(zoomOut, &QAction::triggered, [this]() {
        m_scale->setValue(m_scale->value() - m_scale->pageStep());
    });
    addAction(zoomOut);

    addSpacing(10);

    m_scale = new QSlider(this);
    m_scale->setOrientation(Qt::Horizontal);
    Utils::StyleHelper::setPanelWidget(m_scale);
    Utils::StyleHelper::setPanelWidgetSingleRow(m_scale);
    m_scale->setMaximumWidth(200);
    m_scale->setMinimumWidth(100);
    m_scale->setMinimum(0);
    m_scale->setMaximum(100);
    m_scale->setValue(0);

    connect(m_scale, &QSlider::valueChanged, this, &TimelineToolBar::scaleFactorChanged);
    addWidget(m_scale);

    addSpacing(10);

    auto *zoomIn = createAction(TimelineConstants::C_ZOOM_IN,
                                Theme::iconFromName(Theme::Icon::zoomIn_medium),
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
    auto validator = static_cast<const QIntValidator *>(m_currentFrame->validator());
    const_cast<QIntValidator *>(validator)->setRange(m_firstFrame->text().toInt(),
                                                     m_lastFrame->text().toInt());
}

void TimelineToolBar::resizeEvent([[maybe_unused]] QResizeEvent *event)
{
    int width = 0;
    QWidget *spacer = nullptr;
    for (auto *object : std::as_const(m_grp)) {
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
