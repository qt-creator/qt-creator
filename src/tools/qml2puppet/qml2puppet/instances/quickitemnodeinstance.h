// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#include "objectnodeinstance.h"

#include <QQuickItem>

QT_BEGIN_NAMESPACE
class QQuickDesignerSupport;
QT_END_NAMESPACE

namespace QmlDesigner {
namespace Internal {

class QuickItemNodeInstance : public ObjectNodeInstance
{
public:
    using Pointer = QSharedPointer<QuickItemNodeInstance>;
    using WeakPointer = QWeakPointer<QuickItemNodeInstance>;

    ~QuickItemNodeInstance() override;

    static Pointer create(QObject *objectToBeWrapped);
    static void createEffectItem(bool createEffectItem);
    static void enableUnifiedRenderPath(bool createEffectItem);

    void initialize(const ObjectNodeInstance::Pointer &objectNodeInstance,
                    InstanceContainer::NodeFlags flags) override;

    QQuickItem *contentItem() const override;
    bool hasContent() const override;

    QRectF contentItemBoundingBox() const override;
    QRectF boundingRect() const override;
    QTransform contentTransform() const override;
    QTransform sceneTransform() const override;
    double opacity() const override;
    double rotation() const override;
    double scale() const override;
    QPointF transformOriginPoint() const override;
    double zValue() const override;
    QPointF position() const override;
    QSizeF size() const override;
    QTransform transform() const override;
    QTransform contentItemTransform() const override;
    int penWidth() const override;

    QImage renderImage() const override;
    QImage renderPreviewImage(const QSize &previewImageSize) const override;

    QSharedPointer<QQuickItemGrabResult> createGrabResult() const override;

    void updateAllDirtyNodesRecursive() override;


    QObject *parent() const override;
    QList<ServerNodeInstance> childItems() const override;

    void reparent(const ObjectNodeInstance::Pointer &oldParentInstance, const PropertyName &oldParentProperty, const ObjectNodeInstance::Pointer &newParentInstance, const PropertyName &newParentProperty) override;

    void setPropertyVariant(const PropertyName &name, const QVariant &value) override;
    void setPropertyBinding(const PropertyName &name, const QString &expression) override;
    QVariant property(const PropertyName &name) const override;
    void resetProperty(const PropertyName &name) override;

    bool isAnchoredByChildren() const override;
    bool hasAnchor(const PropertyName &name) const override;
    QPair<PropertyName, ServerNodeInstance> anchor(const PropertyName &name) const override;
    bool isAnchoredBySibling() const override;
    bool isResizable() const override;
    bool isMovable() const override;
    bool isQuickItem() const override;
    bool isRenderable() const override;

    QList<ServerNodeInstance> stateInstances() const override;

    void doComponentComplete() override;

    QList<QQuickItem*> allItemsRecursive() const override;
    QStringList allStates() const override;

    static void updateDirtyNode(QQuickItem *item);
    static bool unifiedRenderPath();

    void setHiddenInEditor(bool b) override;

protected:
    explicit QuickItemNodeInstance(QQuickItem*);
    QQuickItem *quickItem() const;
    void setMovable(bool movable);
    void setResizable(bool resizable);
    void setHasContent(bool hasContent);
    QQuickDesignerSupport *designerSupport() const;
    Qt5NodeInstanceServer *qt5NodeInstanceServer() const;
    void updateDirtyNodesRecursive(QQuickItem *parentItem) const;
    void updateAllDirtyNodesRecursive(QQuickItem *parentItem) const;
    void setAllNodesDirtyRecursive(QQuickItem *parentItem) const;
    QRectF boundingRectWithStepChilds(QQuickItem *parentItem) const;
    void resetHorizontal();
    void resetVertical();
    QList<ServerNodeInstance> childItemsForChild(QQuickItem *item) const;
    void refresh();
    static bool anyItemHasContent(QQuickItem *quickItem);
    static bool childItemsHaveContent(QQuickItem *quickItem);

    double x() const;
    double y() const;
    void markRepeaterParentDirty() const;

private: //variables
    QPointer<QQuickItem> m_contentItem;
    bool m_isResizable;
    bool m_isMovable;
    bool m_hasHeight;
    bool m_hasWidth;
    bool m_hasContent;
    double m_x;
    double m_y;
    double m_width;
    double m_height;
    bool m_hidden = false;
    static bool s_createEffectItem;
    static bool s_unifiedRenderPath;
};

} // namespace Internal
} // namespace QmlDesigner
