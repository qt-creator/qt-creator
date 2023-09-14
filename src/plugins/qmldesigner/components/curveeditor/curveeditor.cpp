// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "curveeditor.h"
#include "curveeditormodel.h"
#include "curveeditortoolbar.h"
#include "detail/curveitem.h"
#include "detail/graphicsview.h"
#include "detail/treeview.h"

#include <designermcumanager.h>
#include <utils/fileutils.h>

#include <QDoubleSpinBox>
#include <QLabel>
#include <QScrollArea>
#include <QSplitter>
#include <QVBoxLayout>

namespace QmlDesigner {

CurveEditor::CurveEditor(CurveEditorModel *model, QWidget *parent)
    : QWidget(parent)
    , m_infoText(nullptr)
    , m_statusLine(nullptr)
    , m_toolbar(new CurveEditorToolBar(model, this))
    , m_tree(new TreeView(model, this))
    , m_view(new GraphicsView(model, this))
{
    const QString labelText = tr(
        "This file does not contain a timeline. <br><br>"
        "To create an animation, add a timeline by clicking the + button in the \"Timeline\" view."
    );
    m_infoText = new QLabel(labelText);
    setContentsMargins(0, 0, 0, 0);

    m_toolbar->setStyleSheet(Theme::replaceCssColors(
        QString::fromUtf8(Utils::FileReader::fetchQrc(":/qmldesigner/stylesheet.css"))));

    auto *splitter = new QSplitter;
    splitter->addWidget(m_tree);
    splitter->addWidget(m_view);
    splitter->setStretchFactor(1, 2);

    QScrollArea* area = new QScrollArea;
    area->setWidget(splitter);
    area->setWidgetResizable(true);
    area->setContentsMargins(0, 0, 0, 0);

    m_statusLine = new QLabel();

    auto *box = new QVBoxLayout;
    box->setContentsMargins(0, 0, 0, 0);
    box->addWidget(m_infoText);
    box->addWidget(m_toolbar);
    box->addWidget(area);
    box->addWidget(m_statusLine);
    setLayout(box);

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

    connect(m_toolbar, &CurveEditorToolBar::currentFrameChanged, [this, model](int frame) {
        model->setCurrentFrame(frame);
        updateStatusLine();
        m_view->viewport()->update();
    });

    connect(m_toolbar, &CurveEditorToolBar::zoomChanged, [this](double zoom) {
        const bool wasBlocked = m_view->blockSignals(true);
        m_view->setZoomX(zoom);
        m_view->blockSignals(wasBlocked);
        m_view->viewport()->update();
    });

    connect(
        m_view, &GraphicsView::currentFrameChanged,
        m_toolbar, &CurveEditorToolBar::setCurrentFrame);

    connect(m_tree, &TreeView::treeItemLocked, model, &CurveEditorModel::setLocked);
    connect(m_tree, &TreeView::treeItemPinned, model, &CurveEditorModel::setPinned);

    connect(
        m_tree->selectionModel(), &SelectionModel::curvesSelected,
        m_view, &GraphicsView::updateSelection);

    connect(m_view, &GraphicsView::zoomChanged, [this](double x, double y) {
        Q_UNUSED(y);
        m_toolbar->setZoom(x);
    });

    auto updateTimeline = [this, model](bool validTimeline) {
        if (validTimeline) {
            updateStatusLine();
            updateMcuState();
            m_view->setCurrentFrame(m_view->model()->currentFrame(), false);
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

    connect(model, &CurveEditorModel::setStatusLineMsg, m_statusLine, &QLabel::setText);
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

void CurveEditor::updateStatusLine()
{
    int currentFrame = m_view->model()->currentFrame();
    QString currentText = QString("Playhead frame %1").arg(currentFrame);
    m_statusLine->setText(currentText);
}

void CurveEditor::updateMcuState()
{
    bool isMcu = DesignerMcuManager::instance().isMCUProject();
    m_toolbar->setIsMcuProject(isMcu);
    m_view->setIsMcu(isMcu);
}

} // End namespace QmlDesigner.
