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
    box->addWidget(createToolBar());
    box->addWidget(splitter);
    setLayout(box);

    connect(m_tree->selectionModel(), &SelectionModel::curvesSelected, m_view, &GraphicsView::reset);
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

QToolBar *CurveEditor::createToolBar()
{
    auto *bar = new QToolBar;
    bar->setFloatable(false);

    QAction *tangentLinearAction = bar->addAction(QIcon(":/curveeditor/images/tangetToolsLinearIcon.png"), "Linear");
    QAction *tangentStepAction = bar->addAction(QIcon(":/curveeditor/images/tangetToolsStepIcon.png"), "Step");
    QAction *tangentSplineAction = bar->addAction(QIcon(":/curveeditor/images/tangetToolsSplineIcon.png"), "Spline");
    QAction *tangentDefaultAction = bar->addAction("Set Default");

    auto setLinearInterpolation = [this]() {
        m_view->setInterpolation(Keyframe::Interpolation::Linear);
    };
    auto setStepInterpolation = [this]() {
        m_view->setInterpolation(Keyframe::Interpolation::Step);
    };
    auto setSplineInterpolation = [this]() {
        m_view->setInterpolation(Keyframe::Interpolation::Bezier);
    };

    connect(tangentLinearAction, &QAction::triggered, setLinearInterpolation);
    connect(tangentStepAction, &QAction::triggered, setStepInterpolation);
    connect(tangentSplineAction, &QAction::triggered, setSplineInterpolation);

    Q_UNUSED(tangentLinearAction);
    Q_UNUSED(tangentSplineAction);
    Q_UNUSED(tangentStepAction);
    Q_UNUSED(tangentDefaultAction);

    auto *valueBox = new QHBoxLayout;
    valueBox->addWidget(new QLabel(tr("Value")));
    valueBox->addWidget(new QDoubleSpinBox);
    auto *valueWidget = new QWidget;
    valueWidget->setLayout(valueBox);
    bar->addWidget(valueWidget);

    auto *durationBox = new QHBoxLayout;
    durationBox->addWidget(new QLabel(tr("Duration")));
    durationBox->addWidget(new QSpinBox);
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
