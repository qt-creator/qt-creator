/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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


#include <timelineeditor/timelinegraphicsscene.h>

#include <qmltimeline.h>

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

signals:
    void selectionChanged();

    void scroll(const TimelineUtils::Side &side);

public:
    explicit TransitionEditorGraphicsScene(TransitionEditorWidget *parent);

    ~TransitionEditorGraphicsScene() override;

    void onShow();

    void setTransition(const ModelNode &transition);
    void clearTransition();

    void setWidth(int width);

    void invalidateLayout();
    void setDuration(int duration);

    TransitionEditorView *transitionEditorView() const;
    TransitionEditorWidget *transitionEditorWidget() const;
    TransitionEditorToolBar *toolBar() const;

    qreal rulerScaling() const override;
    int rulerWidth() const override;
    qreal rulerDuration() const override;
    qreal endFrame() const override;
    qreal startFrame() const override;

    qreal mapToScene(qreal x) const;
    qreal mapFromScene(qreal x) const;

    void setRulerScaling(int scaling);

    void invalidateSectionForTarget(const ModelNode &modelNode);

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

signals:
    void statusBarMessageChanged(const QString &message);

protected:
    bool event(QEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

    void keyPressEvent(QKeyEvent *keyEvent) override;
    void keyReleaseEvent(QKeyEvent *keyEvent) override;

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
};

} // namespace QmlDesigner
