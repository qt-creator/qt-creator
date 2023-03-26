// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "snappinglinecreator.h"

#include <qmldesignercomponents_global.h>
#include <qmlitemnode.h>

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
class Connection;

namespace Internal {
    class GraphicItemResizer;
    class MoveController;
}

class QMLDESIGNERCOMPONENTS_EXPORT FormEditorItem : public QGraphicsItem
{
    friend FormEditorScene;

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

    QPointF center() const;
    qreal selectionWeigth(const QPointF &point, int iteration);

    virtual void synchronizeOtherProperty(const QByteArray &propertyName);
    virtual void setDataModelPosition(const QPointF &position);
    virtual void setDataModelPositionInBaseState(const QPointF &position);
    virtual QPointF instancePosition() const;
    virtual QTransform instanceSceneTransform() const;
    virtual QTransform instanceSceneContentItemTransform() const;

    virtual bool flowHitTest(const QPointF &point) const;

    void setFrameColor(const QColor &color);

protected:
    AbstractFormEditorTool* tool() const;
    void paintBoundingRect(QPainter *painter) const;
    void paintPlaceHolderForInvisbleItem(QPainter *painter) const;
    void paintComponentContentVisualisation(QPainter *painter, const QRectF &clippinRectangle) const;
    QList<FormEditorItem*> offspringFormEditorItemsRecursive(const FormEditorItem *formEditorItem) const;
    FormEditorItem(const QmlItemNode &qmlItemNode, FormEditorScene* scene);
    QTransform viewportTransform() const;

    qreal getItemScaleFactor() const;
    qreal getLineScaleFactor() const;
    qreal getTextScaleFactor() const;

    QRectF m_boundingRect;
    QRectF m_paintedBoundingRect;
    QRectF m_selectionBoundingRect;
    QColor m_frameColor;

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
};

class FormEditorFlowItem : public FormEditorItem
{
    friend FormEditorScene;

public:
    void synchronizeOtherProperty(const QByteArray &propertyName) override;
    void setDataModelPosition(const QPointF &position) override;
    void setDataModelPositionInBaseState(const QPointF &position) override;
    void updateGeometry() override;
    QPointF instancePosition() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

protected:
    FormEditorFlowItem(const QmlItemNode &qmlItemNode, FormEditorScene *scene)
        : FormEditorItem(qmlItemNode, scene)
    {}
private:
    QPointF m_oldPos;
};

class FormEditor3dPreview : public FormEditorItem
{
    friend FormEditorScene;

public:
    void updateGeometry() override;
    QPointF instancePosition() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

protected:
    FormEditor3dPreview(const QmlItemNode &qmlItemNode, FormEditorScene *scene)
        : FormEditorItem(qmlItemNode, scene)
    {
        setHighlightBoundingRect(true);
    }
};

class FormEditorFlowActionItem : public FormEditorItem
{
    friend FormEditorScene;

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
private:
    QPointF m_oldPos;
};

class FormEditorTransitionItem : public FormEditorItem
{
    friend FormEditorScene;

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
    void paintConnection(QPainter *painter, const Connection &connection);
    void drawLabels(QPainter *painter, const Connection &connection);

    void drawGeneralLabel(QPainter *painter, const Connection &connection);
    void drawSingleLabel(QPainter *painter, const Connection &connection);
    void drawSelectionLabel(QPainter *painter, const Connection &connection);

private:
    mutable bool m_hitTest = false;
};

class FormEditorFlowDecisionItem : FormEditorFlowItem
{
    friend FormEditorScene;

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
    friend FormEditorScene;

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
