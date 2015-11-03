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

#ifndef QMT_DIAGRAMGRAPHICSSCENE_H
#define QMT_DIAGRAMGRAPHICSSCENE_H

#include <QGraphicsScene>
#include "qmt/infrastructure/qmt_global.h"


namespace qmt {

class DiagramSceneModel;

class QMT_EXPORT DiagramGraphicsScene :
        public QGraphicsScene
{
    Q_OBJECT

public:
    DiagramGraphicsScene(DiagramSceneModel *diagram_scene_model, QObject *parent = 0);

    ~DiagramGraphicsScene();

signals:

    void sceneActivated();

protected:

    bool event(QEvent *event);

    bool eventFilter(QObject *watched, QEvent *event);

    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);

    void dragEnterEvent(QGraphicsSceneDragDropEvent *event);

    void dragMoveEvent(QGraphicsSceneDragDropEvent *event);

    void dragLeaveEvent(QGraphicsSceneDragDropEvent *event);

    void dropEvent(QGraphicsSceneDragDropEvent *event);

    void focusInEvent(QFocusEvent *event);

    void focusOutEvent(QFocusEvent *event);

    void helpEvent(QGraphicsSceneHelpEvent *event);

    void keyPressEvent(QKeyEvent *event);

    void keyReleaseEvent(QKeyEvent *event);

    void mousePressEvent(QGraphicsSceneMouseEvent *event);

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);

    void wheelEvent(QGraphicsSceneWheelEvent *event);

    void inputMethodEvent(QInputMethodEvent *event);

private:

    DiagramSceneModel *m_diagramSceneModel;

};

}

#endif // QMT_DIAGRAMGRAPHICSSCENE_H
