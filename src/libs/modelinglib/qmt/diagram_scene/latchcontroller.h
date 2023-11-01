// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/infrastructure/qmt_global.h"

#include "capabilities/latchable.h"

#include <QObject>

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
