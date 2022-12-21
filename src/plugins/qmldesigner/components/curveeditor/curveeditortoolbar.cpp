// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "curveeditortoolbar.h"
#include "curveeditormodel.h"

#include <QAction>
#include <QHBoxLayout>
#include <QLabel>
#include <QSignalBlocker>

namespace QmlDesigner {

ValidatableSpinBox::ValidatableSpinBox(std::function<bool(int)> validator, QWidget* parent)
    : QSpinBox(parent)
    , m_validator(validator)
{ }

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


CurveEditorToolBar::CurveEditorToolBar(CurveEditorModel *model, QWidget* parent)
    : QToolBar(parent)
    , m_startSpin(nullptr)
    , m_endSpin(nullptr)
    , m_currentSpin(new QSpinBox)

{
    setFloatable(false);

    QAction *tangentLinearAction = addAction(
        QIcon(":/curveeditor/images/tangetToolsLinearIcon.png"), "Linear");
    QAction *tangentStepAction = addAction(
        QIcon(":/curveeditor/images/tangetToolsStepIcon.png"), "Step");
    QAction *tangentSplineAction = addAction(
        QIcon(":/curveeditor/images/tangetToolsSplineIcon.png"), "Spline");

    QAction *tangentUnifyAction = addAction(tr("Unify"));

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
    connect(tangentStepAction, &QAction::triggered, setStepInterpolation);
    connect(tangentSplineAction, &QAction::triggered, setSplineInterpolation);
    connect(tangentUnifyAction, &QAction::triggered, toggleUnifyKeyframe);

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

    connect(m_currentSpin, &QSpinBox::valueChanged, this, &CurveEditorToolBar::currentFrameChanged);

    auto *durationBox = new QHBoxLayout;
    durationBox->addWidget(new QLabel(tr("Start Frame")));
    durationBox->addWidget(m_startSpin);
    durationBox->addWidget(new QLabel(tr("End Frame")));
    durationBox->addWidget(m_endSpin);

    auto *durationWidget = new QWidget;
    durationWidget->setLayout(durationBox);
    addWidget(durationWidget);

    auto *positionBox = new QHBoxLayout;
    positionBox->addWidget(new QLabel(tr("Current Frame")));
    positionBox->addWidget(m_currentSpin);

    auto *positionWidget = new QWidget;
    positionWidget->setLayout(positionBox);
    addWidget(positionWidget);

    m_zoomSlider = new QSlider(Qt::Horizontal);
    m_zoomSlider->setRange(0, 100);
    connect(m_zoomSlider, &QSlider::valueChanged, [this](int value) {
        emit zoomChanged(static_cast<double>(value)/100.0f);
    });
    addWidget(m_zoomSlider);
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

} // End namespace QmlDesigner.
