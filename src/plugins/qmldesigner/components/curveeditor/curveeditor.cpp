/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "curveeditor.h"
#include "curveeditormodel.h"
#include "detail/curveitem.h"
#include "detail/graphicsview.h"
#include "detail/treeview.h"

#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSplitter>
#include <QVBoxLayout>

namespace DesignTools {

CurveEditor::CurveEditor(CurveEditorModel *model, QWidget *parent)
    : QWidget(parent)
    , m_tree(new TreeView(model, this))
    , m_view(new GraphicsView(model))
{
    auto *splitter = new QSplitter;
    splitter->addWidget(m_tree);
    splitter->addWidget(m_view);
    splitter->setStretchFactor(1, 2);

    auto *box = new QVBoxLayout;
    box->addWidget(createToolBar(model));
    box->addWidget(splitter);
    setLayout(box);

    connect(m_tree, &TreeView::treeItemLocked, model, &CurveEditorModel::curveChanged);
    connect(m_tree, &TreeView::treeItemPinned, model, &CurveEditorModel::curveChanged);

    connect(m_tree, &TreeView::treeItemLocked, m_view, &GraphicsView::setLocked);
    connect(m_tree->selectionModel(),
            &SelectionModel::curvesSelected,
            m_view,
            &GraphicsView::updateSelection);
}

void CurveEditor::zoomX(double zoom)
{
    m_view->setZoomX(zoom);
}

void CurveEditor::zoomY(double zoom)
{
    m_view->setZoomY(zoom);
}

void CurveEditor::clearCanvas()
{
    m_view->reset({});
}

QToolBar *CurveEditor::createToolBar(CurveEditorModel *model)
{
    auto *bar = new QToolBar;
    bar->setFloatable(false);

    QAction *tangentLinearAction = bar->addAction(QIcon(":/curveeditor/images/tangetToolsLinearIcon.png"), "Linear");
    QAction *tangentStepAction = bar->addAction(QIcon(":/curveeditor/images/tangetToolsStepIcon.png"), "Step");
    QAction *tangentSplineAction = bar->addAction(QIcon(":/curveeditor/images/tangetToolsSplineIcon.png"), "Spline");
    QAction *tangentDefaultAction = bar->addAction("Set Default");
    QAction *tangentUnifyAction = bar->addAction("Unify");

    auto setLinearInterpolation = [this]() {
        m_view->setInterpolation(Keyframe::Interpolation::Linear);
    };
    auto setStepInterpolation = [this]() {
        m_view->setInterpolation(Keyframe::Interpolation::Step);
    };
    auto setSplineInterpolation = [this]() {
        m_view->setInterpolation(Keyframe::Interpolation::Bezier);
    };

    auto toggleUnifyKeyframe = [this]() { m_view->toggleUnified(); };

    connect(tangentLinearAction, &QAction::triggered, setLinearInterpolation);
    connect(tangentStepAction, &QAction::triggered, setStepInterpolation);
    connect(tangentSplineAction, &QAction::triggered, setSplineInterpolation);
    connect(tangentUnifyAction, &QAction::triggered, toggleUnifyKeyframe);

    Q_UNUSED(tangentLinearAction);
    Q_UNUSED(tangentSplineAction);
    Q_UNUSED(tangentStepAction);
    Q_UNUSED(tangentDefaultAction);

    auto *durationBox = new QHBoxLayout;
    auto *startSpin = new QSpinBox;
    auto *endSpin = new QSpinBox;

    startSpin->setRange(std::numeric_limits<int>::lowest(), std::numeric_limits<int>::max());
    startSpin->setValue(model->minimumTime());

    auto updateStartFrame = [this, model](int frame) {
        model->setMinimumTime(frame, false);
        m_view->viewport()->update();
    };
    connect(startSpin, QOverload<int>::of(&QSpinBox::valueChanged), updateStartFrame);

    endSpin->setRange(std::numeric_limits<int>::lowest(), std::numeric_limits<int>::max());
    endSpin->setValue(model->maximumTime());

    auto updateEndFrame = [this, model](int frame) {
        model->setMaximumTime(frame, false);
        m_view->viewport()->update();
    };
    connect(endSpin, QOverload<int>::of(&QSpinBox::valueChanged), updateEndFrame);

    auto setStartSlot = [startSpin](int frame) { startSpin->setValue(frame); };
    connect(model, &CurveEditorModel::updateStartFrame, setStartSlot);

    auto setEndSlot = [endSpin](int frame) { endSpin->setValue(frame); };
    connect(model, &CurveEditorModel::updateEndFrame, setEndSlot);

    durationBox->addWidget(new QLabel(tr("Start Frame")));
    durationBox->addWidget(startSpin);
    durationBox->addWidget(new QLabel(tr("End Frame")));
    durationBox->addWidget(endSpin);

    auto *durationWidget = new QWidget;
    durationWidget->setLayout(durationBox);
    bar->addWidget(durationWidget);

    auto *cfspin = new QSpinBox;
    cfspin->setMinimum(0);
    cfspin->setMaximum(std::numeric_limits<int>::max());

    auto intSignal = static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged);
    connect(cfspin, intSignal, [this](int val) { m_view->setCurrentFrame(val, false); });
    connect(m_view, &GraphicsView::notifyFrameChanged, [cfspin](int val) {
        QSignalBlocker blocker(cfspin);
        cfspin->setValue(val);
    });

    auto *positionBox = new QHBoxLayout;
    positionBox->addWidget(new QLabel(tr("Current Frame")));
    positionBox->addWidget(cfspin);
    auto *positionWidget = new QWidget;
    positionWidget->setLayout(positionBox);
    bar->addWidget(positionWidget);

    return bar;
}

} // End namespace DesignTools.
