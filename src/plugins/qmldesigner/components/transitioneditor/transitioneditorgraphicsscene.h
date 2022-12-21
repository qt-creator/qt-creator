// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once


#include <timelineeditor/timelinegraphicsscene.h>

#include <qmltimeline.h>

#include <QElapsedTimer>
#include <QGraphicsScene>

#include <memory>

QT_FORWARD_DECLARE_CLASS(QGraphicsLinearLayout)
QT_FORWARD_DECLARE_CLASS(QComboBox)

namespace QmlDesigner {

class TransitionEditorView;
class TransitionEditorWidget;
class TransitionEditorToolBar;
class TransitionEditorGraphicsLayout;

class TimelineRulerSectionItem;
class TimelineFrameHandle;
class TimelineAbstractTool;
class TimelineMoveTool;
class TimelineKeyframeItem;
class TimelinePlaceholder;
class TimelineToolBar;
class TransitionEditorPropertyItem;

class TransitionEditorGraphicsScene : public AbstractScrollGraphicsScene
{
    Q_OBJECT
public:
    explicit TransitionEditorGraphicsScene(TransitionEditorWidget *parent);

    ~TransitionEditorGraphicsScene() override;

    void onShow();

    void setTransition(const ModelNode &transition);
    void clearTransition();

    void setWidth(int width);

    void invalidateLayout();
    void setDuration(int duration);

    TimelineRulerSectionItem *layoutRuler() const;
    TransitionEditorView *transitionEditorView() const;
    TransitionEditorWidget *transitionEditorWidget() const;
    TransitionEditorToolBar *toolBar() const;

    int zoom() const override;
    qreal rulerScaling() const override;
    int rulerWidth() const override;
    qreal rulerDuration() const override;
    qreal endFrame() const override;
    qreal startFrame() const override;

    qreal mapToScene(qreal x) const;
    qreal mapFromScene(qreal x) const;

    void setZoom(int scaling);
    void setZoom(int scaling, double pivot);

    void invalidateSectionForTarget(const ModelNode &modelNode);
    void invalidateHeightForTarget(const ModelNode &modelNode);

    void invalidateScene();
    void invalidateCurrentValues();
    void invalidateRecordButtonsStatus();

    QGraphicsView *graphicsView() const;
    QGraphicsView *rulerView() const;

    QRectF selectionBounds() const;

    void selectKeyframes(const SelectionMode &mode, const QList<TimelineKeyframeItem *> &items);
    void clearSelection() override;

    void activateLayout();

    AbstractView *abstractView() const override;
    ModelNode transitionModelNode() const;

    TransitionEditorPropertyItem *selectedPropertyItem() const;
    void setSelectedPropertyItem(TransitionEditorPropertyItem *item);

    void invalidateScrollbar() override;

protected:
    bool event(QEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

    void keyPressEvent(QKeyEvent *keyEvent) override;
    void keyReleaseEvent(QKeyEvent *keyEvent) override;

    void focusOutEvent(QFocusEvent *focusEvent) override;
    void focusInEvent(QFocusEvent *focusEvent) override;

private:
    void invalidateSections();
    QList<QGraphicsItem *> itemsAt(const QPointF &pos);

private:
    TransitionEditorWidget *m_parent = nullptr;
    TransitionEditorGraphicsLayout *m_layout = nullptr;
    ModelNode m_transition;

    int m_scrollOffset = 0;
    TimelineToolDelegate m_tools;
    TransitionEditorPropertyItem *m_selectedProperty = nullptr;
    QElapsedTimer m_usageTimer;
};

} // namespace QmlDesigner
