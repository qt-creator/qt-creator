#ifndef QDECLARATIVEDESIGNVIEW_H
#define QDECLARATIVEDESIGNVIEW_H

#include "qmlviewerconstants.h"
#include <qdeclarativeview.h>

QT_FORWARD_DECLARE_CLASS(QDeclarativeItem);
QT_FORWARD_DECLARE_CLASS(QMouseEvent);
QT_FORWARD_DECLARE_CLASS(QToolBar);

namespace QmlViewer {

class AbstractFormEditorTool;
class SelectionTool;
class ZoomTool;
class ColorPickerTool;
class LayerItem;
class BoundingRectHighlighter;
class SubcomponentEditorTool;
class QmlToolbar;

class QDeclarativeDesignView : public QDeclarativeView
{
    Q_OBJECT
public:

    explicit QDeclarativeDesignView(QWidget *parent = 0);
    ~QDeclarativeDesignView();

    void setSelectedItems(QList<QGraphicsItem *> items);
    QList<QGraphicsItem *> selectedItems() const;
    AbstractFormEditorTool *currentTool() const;

    LayerItem *manipulatorLayer() const;
    void changeTool(Constants::DesignTool tool,
                    Constants::ToolFlags flags = Constants::NoToolFlags);

    void clearHighlightBoundingRect();
    void highlightBoundingRect(QGraphicsItem *item);

    bool mouseInsideContextItem() const;
    QList<QGraphicsItem*> selectableItems(const QPoint &pos) const;
    QList<QGraphicsItem*> selectableItems(const QPointF &scenePos) const;
    QList<QGraphicsItem*> selectableItems(const QRectF &sceneRect, Qt::ItemSelectionMode selectionMode) const;
    QGraphicsItem *currentRootItem() const;

    QToolBar *toolbar() const;

public Q_SLOTS:
    void setDesignModeBehavior(bool value);
    bool designModeBehavior() const;
    void changeToSingleSelectTool();
    void changeToMarqueeSelectTool();
    void changeToZoomTool();
    void changeToColorPickerTool();

    void changeAnimationSpeed(qreal slowdownFactor);
    void continueExecution(qreal slowdownFactor = 1.0f);
    void pauseExecution();

Q_SIGNALS:
    void designModeBehaviorChanged(bool inDesignMode);
    void reloadRequested();
    void marqueeSelectToolActivated();
    void selectToolActivated();
    void zoomToolActivated();
    void colorPickerActivated();
    void selectedColorChanged(const QColor &color);

    void executionStarted(qreal slowdownFactor);
    void executionPaused();

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *keyEvent);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);

private Q_SLOTS:
    void reloadView();
    void onStatusChanged(QDeclarativeView::Status status);
    void onCurrentObjectsChanged(QList<QObject*> objects);
    void applyChangesFromClient();

private:
    void createToolbar();
    void changeToSelectTool();
    QList<QGraphicsItem*> filteredItemList(QList<QGraphicsItem*> &itemlist) const;

private:
    QPointF m_cursorPos;
    QList<QGraphicsItem *> m_currentSelection;

    Constants::DesignTool m_currentToolMode;
    AbstractFormEditorTool *m_currentTool;

    SelectionTool *m_selectionTool;
    ZoomTool *m_zoomTool;
    ColorPickerTool *m_colorPickerTool;
    SubcomponentEditorTool *m_subcomponentEditorTool;
    LayerItem *m_manipulatorLayer;

    BoundingRectHighlighter *m_boundingRectHighlighter;

    bool m_designModeBehavior;

    bool m_executionPaused;
    qreal m_slowdownFactor;

    QmlToolbar *m_toolbar;
};

} //namespace QmlViewer

#endif // QDECLARATIVEDESIGNVIEW_H
