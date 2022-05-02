/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
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

    QAction *tangentDefaultAction = addAction(tr("Set Default"));
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
    auto setDefaultKeyframe = [this]() {
        emit defaultClicked();
    };
    auto toggleUnifyKeyframe = [this]() {
        emit unifyClicked();
    };

    connect(tangentLinearAction, &QAction::triggered, setLinearInterpolation);
    connect(tangentStepAction, &QAction::triggered, setStepInterpolation);
    connect(tangentSplineAction, &QAction::triggered, setSplineInterpolation);
    connect(tangentDefaultAction, &QAction::triggered, setDefaultKeyframe);
    connect(tangentUnifyAction, &QAction::triggered, toggleUnifyKeyframe);

    auto validateStart = [this](int val) -> bool {
        if (m_endSpin==nullptr)
            return false;
        return m_endSpin->value() > val;
    };
    m_startSpin = new ValidatableSpinBox(validateStart);
    m_startSpin->setRange(std::numeric_limits<int>::lowest(), std::numeric_limits<int>::max());
    m_startSpin->setValue(model->minimumTime());

    connect(
        m_startSpin, QOverload<int>::of(&QSpinBox::valueChanged),
        this, &CurveEditorToolBar::startFrameChanged);

    connect(
        model, &CurveEditorModel::commitStartFrame,
        [this](int frame) { m_startSpin->setValue(frame); });

    auto validateEnd = [this](int val) -> bool {
        if (m_startSpin==nullptr)
            return false;
        return m_startSpin->value() < val;
    };
    m_endSpin = new ValidatableSpinBox(validateEnd);
    m_endSpin->setRange(std::numeric_limits<int>::lowest(), std::numeric_limits<int>::max());
    m_endSpin->setValue(model->maximumTime());

    connect(
        m_endSpin, QOverload<int>::of(&QSpinBox::valueChanged),
        this, &CurveEditorToolBar::endFrameChanged);

    connect(
        model, &CurveEditorModel::commitEndFrame,
        [this](int frame) { m_endSpin->setValue(frame); });

    m_currentSpin->setMinimum(0);
    m_currentSpin->setMaximum(std::numeric_limits<int>::max());

    connect(
        m_currentSpin, QOverload<int>::of(&QSpinBox::valueChanged),
        this, &CurveEditorToolBar::currentFrameChanged);

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
