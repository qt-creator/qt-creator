/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <qmlitemnode.h>
#include "snappinglinecreator.h"

#include <QPointer>
#include <QGraphicsWidget>

QT_BEGIN_NAMESPACE
class QTimeLine;
class QPainterPath;
QT_END_NAMESPACE

namespace QmlDesigner {

class FormEditorScene;
class FormEditorView;
class AbstractFormEditorTool;

namespace Internal {
    class GraphicItemResizer;
    class MoveController;
}

class QMLDESIGNERCORE_EXPORT FormEditorItem : public QGraphicsItem
{
    friend class QmlDesigner::FormEditorScene;
public:
    ~FormEditorItem() override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    bool isContainer() const;
    QmlItemNode qmlItemNode() const;


    enum { Type = UserType + 0xfffa };

    int type() const override;

    static FormEditorItem* fromQGraphicsItem(QGraphicsItem *graphicsItem);

    void updateSnappingLines(const QList<FormEditorItem*> &exceptionList,
                             FormEditorItem *transformationSpaceItem);

    SnapLineMap topSnappingLines() const;
    SnapLineMap bottomSnappingLines() const;
    SnapLineMap leftSnappingLines() const;
    SnapLineMap rightSnappingLines() const;
    SnapLineMap horizontalCenterSnappingLines() const;
    SnapLineMap verticalCenterSnappingLines() const;

    SnapLineMap topSnappingOffsets() const;
    SnapLineMap bottomSnappingOffsets() const;
    SnapLineMap leftSnappingOffsets() const;
    SnapLineMap rightSnappingOffsets() const;

    QList<FormEditorItem*> childFormEditorItems() const;
    QList<FormEditorItem*> offspringFormEditorItems() const;

    FormEditorScene *scene() const;
    FormEditorItem *parentItem() const;

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    bool contains(const QPointF &point) const override;

    virtual void updateGeometry();
    void updateVisibilty();

    void showAttention();

    FormEditorView *formEditorView() const;

    void setHighlightBoundingRect(bool highlight);
    void blurContent(bool blurContent);

    void setContentVisible(bool visible);
    bool isContentVisible() const;

    bool isFormEditorVisible() const;
    void setFormEditorVisible(bool isVisible);

    QPointF center() const;
    qreal selectionWeigth(const QPointF &point, int iteration);

    virtual void synchronizeOtherProperty(const QByteArray &propertyName);
    virtual void setDataModelPosition(const QPointF &position);
    virtual void setDataModelPositionInBaseState(const QPointF &position);
    virtual QPointF instancePosition() const;
    virtual QTransform instanceSceneTransform() const;
    virtual QTransform instanceSceneContentItemTransform() const;

    virtual bool flowHitTest(const QPointF &point) const;

protected:
    AbstractFormEditorTool* tool() const;
    void paintBoundingRect(QPainter *painter) const;
    void paintPlaceHolderForInvisbleItem(QPainter *painter) const;
    void paintComponentContentVisualisation(QPainter *painter, const QRectF &clippinRectangle) const;
    QList<FormEditorItem*> offspringFormEditorItemsRecursive(const FormEditorItem *formEditorItem) const;
    FormEditorItem(const QmlItemNode &qmlItemNode, FormEditorScene* scene);
    QTransform viewportTransform() const;

    QRectF m_boundingRect;
    QRectF m_paintedBoundingRect;
    QRectF m_selectionBoundingRect;

private: // functions
    void setup();

private: // variables
    SnappingLineCreator m_snappingLineCreator;
    QmlItemNode m_qmlItemNode;
    QPointer<QTimeLine> m_attentionTimeLine;
    QTransform m_inverseAttentionTransform;

    double m_borderWidth;
    bool m_highlightBoundingRect;
    bool m_blurContent;
    bool m_isContentVisible;
    bool m_isFormEditorVisible;
};

class FormEditorFlowItem : public FormEditorItem
{
    friend class QmlDesigner::FormEditorScene;
public:
    void synchronizeOtherProperty(const QByteArray &propertyName) override;
    void setDataModelPosition(const QPointF &position) override;
    void setDataModelPositionInBaseState(const QPointF &position) override;
    void updateGeometry() override;
    QPointF instancePosition() const override;

protected:
    FormEditorFlowItem(const QmlItemNode &qmlItemNode, FormEditorScene *scene)
        : FormEditorItem(qmlItemNode, scene)
    {}
};

class FormEditorFlowActionItem : public FormEditorItem
{
    friend class QmlDesigner::FormEditorScene;
public:
    void setDataModelPosition(const QPointF &position) override;
    void setDataModelPositionInBaseState(const QPointF &position) override;
    void updateGeometry() override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    QTransform instanceSceneTransform() const override;
    QTransform instanceSceneContentItemTransform() const override;

protected:
    FormEditorFlowActionItem(const QmlItemNode &qmlItemNode, FormEditorScene *scene)
        : FormEditorItem(qmlItemNode, scene)
    {}
};

class FormEditorTransitionItem : public FormEditorItem
{
    friend class QmlDesigner::FormEditorScene;
public:
    void synchronizeOtherProperty(const QByteArray &propertyName) override;
    void setDataModelPosition(const QPointF &position) override;
    void setDataModelPositionInBaseState(const QPointF &position) override;
    void updateGeometry() override;
    QPointF instancePosition() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    bool flowHitTest(const QPointF &point) const override;

protected:
    FormEditorTransitionItem(const QmlItemNode &qmlItemNode, FormEditorScene *scene)
        : FormEditorItem(qmlItemNode, scene)
    {}
private:
    mutable bool m_hitTest = false;
};

class FormEditorFlowDecisionItem : FormEditorFlowItem
{
    friend class QmlDesigner::FormEditorScene;

public:
    void updateGeometry() override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    bool flowHitTest(const QPointF &point) const override;

protected:
    enum IconType {
        DecisionIcon,
        WildcardIcon
    };

    FormEditorFlowDecisionItem(const QmlItemNode &qmlItemNode,
                               FormEditorScene *scene,
                               IconType iconType = DecisionIcon)
        : FormEditorFlowItem(qmlItemNode, scene), m_iconType(iconType)
    {}
    IconType m_iconType;
};

class FormEditorFlowWildcardItem : FormEditorFlowDecisionItem
{
    friend class QmlDesigner::FormEditorScene;

public:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

protected:
    FormEditorFlowWildcardItem(const QmlItemNode &qmlItemNode, FormEditorScene *scene)
        : FormEditorFlowDecisionItem(qmlItemNode, scene, WildcardIcon)
    {
    }
};

inline int FormEditorItem::type() const
{
    return UserType + 0xfffa;
}

} // namespace QmlDesigner
