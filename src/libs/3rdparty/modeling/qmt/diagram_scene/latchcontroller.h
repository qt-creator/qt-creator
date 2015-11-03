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

#ifndef QMT_LATCHCONTROLLER_H
#define QMT_LATCHCONTROLLER_H

#include <QObject>
#include "qmt/infrastructure/qmt_global.h"

#include "capabilities/latchable.h"

QT_BEGIN_NAMESPACE
class QGraphicsScene;
class QGraphicsSceneMouseEvent;
QT_END_NAMESPACE


namespace qmt {

class DiagramController;
class DiagramSceneModel;
class AlignLineItem;

class QMT_EXPORT LatchController :
        public QObject
{
    Q_OBJECT

public:

    LatchController(QObject *parent = 0);

    ~LatchController();

public:

    void setDiagramSceneModel(DiagramSceneModel *diagram_scene_model);

public:

    void addToGraphicsScene(QGraphicsScene *graphics_scene);

    void removeFromGraphicsScene(QGraphicsScene *graphics_scene);

    void mousePressEventLatching(QGraphicsSceneMouseEvent *event);

    void mouseMoveEventLatching(QGraphicsSceneMouseEvent *event);

    void mouseReleaseEventLatching(QGraphicsSceneMouseEvent *event);

private:

    DiagramSceneModel *m_diagramSceneModel;

    AlignLineItem *m_horizontalAlignLine;

    AlignLineItem *m_verticalAlignLine;

    bool m_foundHorizontalLatch;

    ILatchable::Latch m_horizontalLatch;

    qreal m_horizontalDist;

    bool m_foundVerticalLatch;

    ILatchable::Latch m_verticalLatch;

    qreal m_verticalDist;
};

}

#endif // QMT_LATCHCONTROLLER_H
