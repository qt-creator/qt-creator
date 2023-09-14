// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "curveeditortoolbar.h"
#include "curveeditorconstants.h"
#include "curveeditormodel.h"

#include "coreplugin/actionmanager/actionmanager.h"
#include "coreplugin/icontext.h"
#include "theme.h"

#include <utils/id.h>
#include <utils/stylehelper.h>

#include <QAction>
#include <QHBoxLayout>
#include <QLabel>
#include <QSignalBlocker>

namespace QmlDesigner {

ValidatableSpinBox::ValidatableSpinBox(std::function<bool(int)> validator, QWidget* parent)
    : QSpinBox(parent)
    , m_validator(validator)
{
    setButtonSymbols(NoButtons);
    setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    setFrame(false);
}

QValidator::State ValidatableSpinBox::validate(QString &text, int &pos) const
{
    auto result = QSpinBox::validate(text, pos);
    if (result==QValidator::Acceptable) {
        if (int val = text.toInt(); m_validator(val))
            return result;

        result = QValidator::Intermediate;
    }
    return result;
}

static QAction *createAction(const Utils::Id &id,
                      const QIcon &icon,
                      const QString &name,
                      const QKeySequence &shortcut)
{
    Core::Context context(CurveEditorConstants::C_QMLCURVEEDITOR);

    auto *action = new QAction(icon, name);
    auto *command = Core::ActionManager::registerAction(action, id, context);
    command->setDefaultKeySequence(shortcut);
    command->augmentActionWithShortcutToolTip(action);

    return action;
}

CurveEditorToolBar::CurveEditorToolBar(CurveEditorModel *model, QWidget* parent)
    : QToolBar(parent)
    , m_startSpin(nullptr)
    , m_endSpin(nullptr)
    , m_currentSpin(new QSpinBox)
    , m_stepAction(nullptr)
    , m_splineAction(nullptr)
    , m_unifyAction(nullptr)
{
    setFloatable(false);
    setFixedHeight(Theme::toolbarSize());
    setContentsMargins(0, 0, 0, 0);

    addSpace(5);

    QAction *tangentLinearAction = addAction(Theme::iconFromName(Theme::linear_medium), tr("Linear"));
    m_stepAction = addAction(Theme::iconFromName(Theme::step_medium), tr(m_stepLabel));
    m_splineAction = addAction(Theme::iconFromName(Theme::bezier_medium), tr(m_splineLabel));
    m_unifyAction = addAction(Theme::iconFromName(Theme::unify_medium), tr(m_unifyLabel));

    auto setLinearInterpolation = [this]() {
        emit interpolationClicked(Keyframe::Interpolation::Linear);
    };
    auto setStepInterpolation = [this]() {
        emit interpolationClicked(Keyframe::Interpolation::Step);
    };
    auto setSplineInterpolation = [this]() {
        emit interpolationClicked(Keyframe::Interpolation::Bezier);
    };
    auto toggleUnifyKeyframe = [this]() {
        emit unifyClicked();
    };

    connect(tangentLinearAction, &QAction::triggered, setLinearInterpolation);
    connect(m_stepAction, &QAction::triggered, setStepInterpolation);
    connect(m_splineAction, &QAction::triggered, setSplineInterpolation);
    connect(m_unifyAction, &QAction::triggered, toggleUnifyKeyframe);

    auto validateStart = [this](int val) -> bool {
        if (m_endSpin==nullptr)
            return false;
        return m_endSpin->value() > val;
    };
    m_startSpin = new ValidatableSpinBox(validateStart);
    m_startSpin->setRange(std::numeric_limits<int>::lowest(), std::numeric_limits<int>::max());
    m_startSpin->setValue(model->minimumTime());
    m_startSpin->setFixedWidth(70);

    connect(m_startSpin, &QSpinBox::valueChanged, this, &CurveEditorToolBar::startFrameChanged);
    connect(model, &CurveEditorModel::commitStartFrame,
            this, [this](int frame) { m_startSpin->setValue(frame); });

    auto validateEnd = [this](int val) -> bool {
        if (m_startSpin==nullptr)
            return false;
        return m_startSpin->value() < val;
    };
    m_endSpin = new ValidatableSpinBox(validateEnd);
    m_endSpin->setRange(std::numeric_limits<int>::lowest(), std::numeric_limits<int>::max());
    m_endSpin->setValue(model->maximumTime());
    m_endSpin->setFixedWidth(70);

    connect(m_endSpin, &QSpinBox::valueChanged, this, &CurveEditorToolBar::endFrameChanged);
    connect(model, &CurveEditorModel::commitEndFrame,
            this, [this](int frame) { m_endSpin->setValue(frame); });

    m_currentSpin->setMinimum(0);
    m_currentSpin->setMaximum(std::numeric_limits<int>::max());
    m_currentSpin->setFixedWidth(70);
    m_currentSpin->setButtonSymbols(QSpinBox::NoButtons);
    m_currentSpin->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_currentSpin->setFrame(false);

    connect(m_currentSpin, &QSpinBox::valueChanged, this, &CurveEditorToolBar::currentFrameChanged);

    addSpacer();

    auto *durationBox = new QHBoxLayout;
    durationBox->setContentsMargins(0, 0, 0, 0);
    durationBox->addWidget(new QLabel(tr("Start Frame")));
    durationBox->addWidget(m_startSpin);
    addSpace(durationBox);
    durationBox->addWidget(new QLabel(tr("End Frame")));
    durationBox->addWidget(m_endSpin);

    auto *durationWidget = new QWidget;
    durationWidget->setLayout(durationBox);
    addWidget(durationWidget);
    addSpace();

    auto *positionBox = new QHBoxLayout;
    positionBox->setContentsMargins(0, 0, 0, 0);
    positionBox->addWidget(new QLabel(tr("Current Frame")));
    positionBox->addWidget(m_currentSpin);

    auto *positionWidget = new QWidget;
    positionWidget->setLayout(positionBox);
    addWidget(positionWidget);
    addSpace();

    m_zoomSlider = new QSlider(Qt::Horizontal);
    m_zoomSlider->setRange(0, 100);
    Utils::StyleHelper::setPanelWidget(m_zoomSlider);
    Utils::StyleHelper::setPanelWidgetSingleRow(m_zoomSlider);
    m_zoomSlider->setFixedWidth(120);

    connect(m_zoomSlider, &QSlider::valueChanged, [this](int value) {
        emit zoomChanged(static_cast<double>(value)/100.0f);
    });

    auto *zoomOut = createAction(CurveEditorConstants::C_ZOOM_OUT,
                                 Theme::iconFromName(Theme::Icon::zoomOut_medium),
                                 tr("Zoom Out"),
                                 QKeySequence(QKeySequence::ZoomOut));

    connect(zoomOut, &QAction::triggered, [this]() {
        m_zoomSlider->setValue(m_zoomSlider->value() - m_zoomSlider->pageStep());
    });

    auto *zoomIn = createAction(CurveEditorConstants::C_ZOOM_IN,
                                Theme::iconFromName(Theme::Icon::zoomIn_medium),
                                tr("Zoom In"),
                                QKeySequence(QKeySequence::ZoomIn));

    connect(zoomIn, &QAction::triggered, [this]() {
        m_zoomSlider->setValue(m_zoomSlider->value() + m_zoomSlider->pageStep());
    });

    addAction(zoomOut);
    addWidget(m_zoomSlider);
    addAction(zoomIn);

    addSpace(5);
}

void CurveEditorToolBar::setIsMcuProject(bool isMcu)
{
    m_stepAction->setDisabled(isMcu);
    m_splineAction->setDisabled(isMcu);
    m_unifyAction->setDisabled(isMcu);

    static constexpr const char* notSupportedString = QT_TR_NOOP("Not supported for MCUs");

    if (isMcu) {
        m_stepAction->setText(tr(notSupportedString));
        m_splineAction->setText(tr(notSupportedString));
        m_unifyAction->setText(tr(notSupportedString));
    } else {
        m_stepAction->setText(tr(m_stepLabel));
        m_splineAction->setText(tr(m_splineLabel));
        m_unifyAction->setText(tr(m_unifyLabel));
    }
}

void CurveEditorToolBar::setZoom(double zoom)
{
    QSignalBlocker blocker(m_zoomSlider);
    m_zoomSlider->setValue( static_cast<int>(zoom*100));
}

void CurveEditorToolBar::setCurrentFrame(int current, bool notify)
{
    if (notify) {
        m_currentSpin->setValue(current);
    } else {
        QSignalBlocker blocker(m_currentSpin);
        m_currentSpin->setValue(current);
    }
}

void CurveEditorToolBar::updateBoundsSilent(int start, int end)
{
    QSignalBlocker startBlocker(m_startSpin);
    m_startSpin->setValue(start);

    QSignalBlocker endBlocker(m_endSpin);
    m_endSpin->setValue(end);
}

void CurveEditorToolBar::addSpacer()
{
    QWidget *spacerWidget = new QWidget(this);
    spacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    addWidget(spacerWidget);
}

void CurveEditorToolBar::addSpace(int spaceSize)
{
    QWidget *spacerWidget = new QWidget(this);
    spacerWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    spacerWidget->setFixedSize(spaceSize, 1);
    addWidget(spacerWidget);
}

void CurveEditorToolBar::addSpace(QLayout *layout, int spaceSize)
{
    QWidget *spacerWidget = new QWidget(layout->widget());
    spacerWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    spacerWidget->setFixedSize(spaceSize, 1);
    layout->addWidget(spacerWidget);
}

} // End namespace QmlDesigner.
