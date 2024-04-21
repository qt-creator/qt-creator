// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QGraphicsView>
#include "qmt/infrastructure/qmt_global.h"

#include <QPointer>
#include <QTimer>

namespace qmt {

class DiagramSceneModel;
class MPackage;

class QMT_EXPORT DiagramView : public QGraphicsView
{
public:
    explicit DiagramView(QWidget *parent);
    ~DiagramView() override;

    DiagramSceneModel *diagramSceneModel() const;
    void setDiagramSceneModel(DiagramSceneModel *diagramSceneModel);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    void onSceneRectChanged(const QRectF &sceneRect);
    void onPanTimeout();

    QPointer<DiagramSceneModel> m_diagramSceneModel;
    QPointF m_lastMouse;
    QTimer m_panTimer;
};

} // namespace qmt
