/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QuickITEMNODEINSTANCE_H
#define QuickITEMNODEINSTANCE_H

#include <QtGlobal>

#include "objectnodeinstance.h"

#include <QQuickItem>
#include <designersupportdelegate.h>

namespace QmlDesigner {
namespace Internal {

class QuickItemNodeInstance : public ObjectNodeInstance
{
public:
    typedef QSharedPointer<QuickItemNodeInstance> Pointer;
    typedef QWeakPointer<QuickItemNodeInstance> WeakPointer;

    ~QuickItemNodeInstance();

    static Pointer create(QObject *objectToBeWrapped);
    static void createEffectItem(bool createEffectItem);

    void initialize(const ObjectNodeInstance::Pointer &objectNodeInstance) override;

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

    QList<ServerNodeInstance> stateInstances() const override;

    void doComponentComplete() override;

    QList<QQuickItem*> allItemsRecursive() const override;

protected:
    explicit QuickItemNodeInstance(QQuickItem*);
    QQuickItem *quickItem() const;
    void setMovable(bool movable);
    void setResizable(bool resizable);
    void setHasContent(bool hasContent);
    DesignerSupport *designerSupport() const;
    Qt5NodeInstanceServer *qt5NodeInstanceServer() const;
    void updateDirtyNodesRecursive(QQuickItem *parentItem) const;
    void updateAllDirtyNodesRecursive(QQuickItem *parentItem) const;
    QRectF boundingRectWithStepChilds(QQuickItem *parentItem) const;
    void resetHorizontal();
    void resetVertical();
    QList<ServerNodeInstance> childItemsForChild(QQuickItem *item) const;
    void refresh();
    static bool anyItemHasContent(QQuickItem *quickItem);
    static bool childItemsHaveContent(QQuickItem *quickItem);

    double x() const;
    double y() const;

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
    static bool s_createEffectItem;
};

} // namespace Internal
} // namespace QmlDesigner

#endif  // QuickITEMNODEINSTANCE_H

