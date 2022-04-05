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
#include "curveeditortoolbar.h"
#include "detail/curveitem.h"
#include "detail/graphicsview.h"
#include "detail/treeview.h"

#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QSplitter>
#include <QVBoxLayout>

namespace QmlDesigner {

CurveEditor::CurveEditor(CurveEditorModel *model, QWidget *parent)
    : QWidget(parent)
    , m_infoText(nullptr)
    , m_toolbar(new CurveEditorToolBar(model, this))
    , m_tree(new TreeView(model, this))
    , m_view(new GraphicsView(model, this))
{
    const QString labelText = tr(
        "This file does not contain a timeline. <br><br>"
        "To create an animation, add a timeline by clicking the + button in the \"Timeline\" view."
    );
    m_infoText = new QLabel(labelText);

    auto *splitter = new QSplitter;
    splitter->addWidget(m_tree);
    splitter->addWidget(m_view);
    splitter->setStretchFactor(1, 2);

    QScrollArea* area = new QScrollArea;
    area->setWidget(splitter);
    area->setWidgetResizable(true);

    auto *box = new QVBoxLayout;
    box->addWidget(m_infoText);
    box->addWidget(m_toolbar);
    box->addWidget(area);
    setLayout(box);

    connect(m_toolbar, &CurveEditorToolBar::defaultClicked, [this]() {
        m_view->setDefaultInterpolation();
    });

    connect(m_toolbar, &CurveEditorToolBar::unifyClicked, [this]() {
        m_view->toggleUnified();
    });

    connect(m_toolbar, &CurveEditorToolBar::interpolationClicked, [this](Keyframe::Interpolation ipol) {
        m_view->setInterpolation(ipol);
    });

    connect(m_toolbar, &CurveEditorToolBar::startFrameChanged, [this, model](int frame) {
        model->setMinimumTime(frame);
        m_view->viewport()->update();
    });

    connect(m_toolbar, &CurveEditorToolBar::endFrameChanged, [this, model](int frame) {
        model->setMaximumTime(frame);
        m_view->viewport()->update();
    });

    connect(
        m_toolbar, &CurveEditorToolBar::currentFrameChanged,
        model, &CurveEditorModel::commitCurrentFrame);

    connect(
        m_view, &GraphicsView::currentFrameChanged,
        m_toolbar, &CurveEditorToolBar::setCurrentFrame);

    connect(m_tree, &TreeView::treeItemLocked, model, &CurveEditorModel::setLocked);
    connect(m_tree, &TreeView::treeItemPinned, model, &CurveEditorModel::setPinned);

    connect(
        m_tree->selectionModel(), &SelectionModel::curvesSelected,
        m_view, &GraphicsView::updateSelection);

    auto updateTimeline = [this, model](bool validTimeline) {
        if (validTimeline) {
            m_toolbar->updateBoundsSilent(model->minimumTime(), model->maximumTime());
            m_toolbar->show();
            m_tree->show();
            m_view->show();
            m_infoText->hide();
        } else {
            m_toolbar->hide();
            m_tree->hide();
            m_view->hide();
            m_infoText->show();
        }
    };
    connect(model, &CurveEditorModel::timelineChanged, this, updateTimeline);
}

bool CurveEditor::dragging() const
{
    return m_view->dragging();
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

void CurveEditor::showEvent(QShowEvent *event)
{
    emit viewEnabledChanged(true);
    QWidget::showEvent(event);
}

void CurveEditor::hideEvent(QHideEvent *event)
{
    emit viewEnabledChanged(false);
    QWidget::hideEvent(event);
}

} // End namespace QmlDesigner.
