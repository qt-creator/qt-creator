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

#pragma once

#include <QObject>
#include "qmt/infrastructure/qmt_global.h"

#include "capabilities/latchable.h"

QT_BEGIN_NAMESPACE
class QGraphicsScene;
class QGraphicsSceneMouseEvent;
class QKeyEvent;
QT_END_NAMESPACE

namespace qmt {

class DiagramController;
class DiagramSceneModel;
class AlignLineItem;

class QMT_EXPORT LatchController : public QObject
{
    Q_OBJECT

public:
    explicit LatchController(QObject *parent = nullptr);
    ~LatchController() override;

    void setDiagramSceneModel(DiagramSceneModel *diagramSceneModel);

    void addToGraphicsScene(QGraphicsScene *graphicsScene);
    void removeFromGraphicsScene(QGraphicsScene *graphicsScene);

    void keyPressEventLatching(QKeyEvent *event);
    void keyReleaseEventLatching(QKeyEvent *event);
    void mousePressEventLatching(QGraphicsSceneMouseEvent *event);
    void mouseMoveEventLatching(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEventLatching(QGraphicsSceneMouseEvent *event);

private:
    void handleLatches();
    void hideLatches();
    void applyLatches();

private:
    DiagramSceneModel *m_diagramSceneModel = nullptr;
    AlignLineItem *m_horizontalAlignLine = nullptr;
    AlignLineItem *m_verticalAlignLine = nullptr;
    bool m_foundHorizontalLatch = false;
    ILatchable::Latch m_horizontalLatch;
    qreal m_horizontalDist = 0.0;
    bool m_foundVerticalLatch = false;
    ILatchable::Latch m_verticalLatch;
    qreal m_verticalDist = 0.0;
};

} // namespace qmt
