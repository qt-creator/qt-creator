// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "diagramgraphicsscene.h"

#include "diagramscenemodel.h"

namespace qmt {

DiagramGraphicsScene::DiagramGraphicsScene(DiagramSceneModel *diagramSceneModel, QObject *parent)
    : QGraphicsScene(parent),
      m_diagramSceneModel(diagramSceneModel)
{
    setBackgroundBrush(Qt::white);
}

DiagramGraphicsScene::~DiagramGraphicsScene()
{
}

bool DiagramGraphicsScene::event(QEvent *event)
{
    return QGraphicsScene::event(event);
}

bool DiagramGraphicsScene::eventFilter(QObject *watched, QEvent *event)
{
    return QGraphicsScene::eventFilter(watched, event);
}

void DiagramGraphicsScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    QGraphicsScene::contextMenuEvent(event);
}

void DiagramGraphicsScene::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    QGraphicsScene::dragEnterEvent(event);
}

void DiagramGraphicsScene::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
    QGraphicsScene::dragMoveEvent(event);
}

void DiagramGraphicsScene::dragLeaveEvent(QGraphicsSceneDragDropEvent *event)
{
    QGraphicsScene::dragLeaveEvent(event);
}

void DiagramGraphicsScene::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    QGraphicsScene::dropEvent(event);
}

void DiagramGraphicsScene::focusInEvent(QFocusEvent *event)
{
    QGraphicsScene::focusInEvent(event);
}

void DiagramGraphicsScene::focusOutEvent(QFocusEvent *event)
{
    QGraphicsScene::focusOutEvent(event);
}

void DiagramGraphicsScene::helpEvent(QGraphicsSceneHelpEvent *event)
{
    QGraphicsScene::helpEvent(event);
}

void DiagramGraphicsScene::keyPressEvent(QKeyEvent *event)
{
    QGraphicsScene::keyPressEvent(event);
    m_diagramSceneModel->keyPressEvent(event);
}

void DiagramGraphicsScene::keyReleaseEvent(QKeyEvent *event)
{
    QGraphicsScene::keyReleaseEvent(event);
    m_diagramSceneModel->keyReleaseEvent(event);
}

void DiagramGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    m_diagramSceneModel->sceneActivated();
    QGraphicsScene::mousePressEvent(event);
    m_diagramSceneModel->mousePressEvent(event);
}

void DiagramGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsScene::mouseMoveEvent(event);
    m_diagramSceneModel->mouseMoveEvent(event);
}

void DiagramGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsScene::mouseReleaseEvent(event);
    m_diagramSceneModel->mouseReleaseEvent(event);
}

void DiagramGraphicsScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsScene::mouseDoubleClickEvent(event);
}

void DiagramGraphicsScene::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    QGraphicsScene::wheelEvent(event);
}

void DiagramGraphicsScene::inputMethodEvent(QInputMethodEvent *event)
{
    QGraphicsScene::inputMethodEvent(event);
}

} // namespace qmt
