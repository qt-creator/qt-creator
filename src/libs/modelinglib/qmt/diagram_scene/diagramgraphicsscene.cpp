/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "diagramgraphicsscene.h"

#include "diagramscenemodel.h"

namespace qmt {

DiagramGraphicsScene::DiagramGraphicsScene(DiagramSceneModel *diagramSceneModel, QObject *parent)
    : QGraphicsScene(parent),
      m_diagramSceneModel(diagramSceneModel)
{
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
