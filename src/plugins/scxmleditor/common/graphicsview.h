// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "layoutitem.h"

#include <QGraphicsView>
#include <QPointer>

QT_FORWARD_DECLARE_CLASS(QImage)

namespace ScxmlEditor {

namespace PluginInterface {
class ShapeProvider;
class GraphicsScene;
class ScxmlUiFactory;
class ScxmlDocument;
} // namespace PluginInterface

namespace Common {

class GraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit GraphicsView(QWidget *parent = nullptr);

    void setGraphicsScene(PluginInterface::GraphicsScene *scene);

    double minZoomValue() const;
    double maxZoomValue() const;
    void setDrawingEnabled(bool enabled);
    void setUiFactory(PluginInterface::ScxmlUiFactory *factory);
    void setDocument(PluginInterface::ScxmlDocument *document);
    void moveToPoint(const QPointF &p);
    void fitSceneToView();
    void zoomIn();
    void zoomOut();
    void zoomTo(int value);
    void zoomToItem(QGraphicsItem *item);
    void setPanning(bool pan);
    void magnifierClicked(double zoomLevel, const QPointF &p);
    QImage grabView();

signals:
    void viewChanged(const QPolygonF &mainView);
    void zoomPercentChanged(int value);
    void panningChanged(bool panning);
    void magnifierChanged(bool magnifier);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void centerToItem(QGraphicsItem *item);
    void sceneRectHasChanged(const QRectF &r);
    void updateView();
    void initLayoutItem();

    bool m_drawingEnabled = true;
    double m_minZoomValue = 0.1;
    double m_maxZoomValue = 1.5;
    PluginInterface::ShapeProvider *m_shapeProvider = nullptr;
    QPointer<PluginInterface::ScxmlDocument> m_document;
    QPointer<PluginInterface::LayoutItem> m_layoutItem;
};

} // namespace Common
} // namespace ScxmlEditor
