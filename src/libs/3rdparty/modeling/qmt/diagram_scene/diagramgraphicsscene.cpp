/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "diagramgraphicsscene.h"

#include "diagramscenemodel.h"

#include <QDebug>


namespace qmt {

DiagramGraphicsScene::DiagramGraphicsScene(DiagramSceneModel *diagram_scene_model, QObject *parent)
    : QGraphicsScene(parent),
      m_diagramSceneModel(diagram_scene_model)
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
}

void DiagramGraphicsScene::keyReleaseEvent(QKeyEvent *event)
{
    QGraphicsScene::keyReleaseEvent(event);
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

}
