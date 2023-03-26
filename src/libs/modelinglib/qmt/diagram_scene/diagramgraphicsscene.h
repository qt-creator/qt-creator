// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QGraphicsScene>
#include "qmt/infrastructure/qmt_global.h"

namespace qmt {

class DiagramSceneModel;

class QMT_EXPORT DiagramGraphicsScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit DiagramGraphicsScene(DiagramSceneModel *diagramSceneModel, QObject *parent = nullptr);
    ~DiagramGraphicsScene() override;

signals:
    void sceneActivated();

protected:
    bool event(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
    void dragEnterEvent(QGraphicsSceneDragDropEvent *event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent *event) override;
    void dragLeaveEvent(QGraphicsSceneDragDropEvent *event) override;
    void dropEvent(QGraphicsSceneDragDropEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void helpEvent(QGraphicsSceneHelpEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void wheelEvent(QGraphicsSceneWheelEvent *event) override;
    void inputMethodEvent(QInputMethodEvent *event) override;

private:
    DiagramSceneModel *m_diagramSceneModel = nullptr;
};

} // namespace qmt
