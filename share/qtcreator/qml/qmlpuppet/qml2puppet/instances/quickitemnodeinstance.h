/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QuickITEMNODEINSTANCE_H
#define QuickITEMNODEINSTANCE_H

#include <QtGlobal>

#include "objectnodeinstance.h"

#include <QQuickItem>
#include <designersupport.h>

namespace QmlDesigner {
namespace Internal {

class QuickItemNodeInstance : public ObjectNodeInstance
{
public:
    typedef QSharedPointer<QuickItemNodeInstance> Pointer;
    typedef QWeakPointer<QuickItemNodeInstance> WeakPointer;

    ~QuickItemNodeInstance();

    static Pointer create(QObject *objectToBeWrapped);
    void initialize(const ObjectNodeInstance::Pointer &objectNodeInstance) Q_DECL_OVERRIDE;

    bool isQuickItem() const Q_DECL_OVERRIDE;

    QRectF boundingRect() const Q_DECL_OVERRIDE;
    QPointF position() const Q_DECL_OVERRIDE;
    QSizeF size() const Q_DECL_OVERRIDE;
    QTransform transform() const Q_DECL_OVERRIDE;
    QTransform customTransform() const Q_DECL_OVERRIDE;
    QTransform sceneTransform() const Q_DECL_OVERRIDE;
    double opacity() const Q_DECL_OVERRIDE;

    QObject *parent() const Q_DECL_OVERRIDE;

    double rotation() const Q_DECL_OVERRIDE;
    double scale() const Q_DECL_OVERRIDE;
    QPointF transformOriginPoint() const Q_DECL_OVERRIDE;
    double zValue() const Q_DECL_OVERRIDE;

    bool equalQuickItem(QQuickItem *item) const;

    bool hasContent() const Q_DECL_OVERRIDE;

    QList<ServerNodeInstance> childItems() const Q_DECL_OVERRIDE;
    QList<ServerNodeInstance> childItemsForChild(QQuickItem *childItem) const;

    bool isMovable() const Q_DECL_OVERRIDE;
    void setMovable(bool movable);

    void setPropertyVariant(const PropertyName &name, const QVariant &value) Q_DECL_OVERRIDE;
    void setPropertyBinding(const PropertyName &name, const QString &expression) Q_DECL_OVERRIDE;

    QVariant property(const PropertyName &name) const Q_DECL_OVERRIDE;
    void resetProperty(const PropertyName &name) Q_DECL_OVERRIDE;

    void reparent(const ObjectNodeInstance::Pointer &oldParentInstance, const PropertyName &oldParentProperty, const ObjectNodeInstance::Pointer &newParentInstance, const PropertyName &newParentProperty) Q_DECL_OVERRIDE;

    int penWidth() const Q_DECL_OVERRIDE;

    bool hasAnchor(const PropertyName &name) const Q_DECL_OVERRIDE;
    QPair<PropertyName, ServerNodeInstance> anchor(const PropertyName &name) const Q_DECL_OVERRIDE;
    bool isAnchoredBySibling() const Q_DECL_OVERRIDE;
    bool isAnchoredByChildren() const Q_DECL_OVERRIDE;
    void doComponentComplete() Q_DECL_OVERRIDE;

    bool isResizable() const Q_DECL_OVERRIDE;
    void setResizable(bool resizeable);

    void setHasContent(bool hasContent);

    QList<ServerNodeInstance> stateInstances() const Q_DECL_OVERRIDE;

    QImage renderImage() const Q_DECL_OVERRIDE;
    QImage renderPreviewImage(const QSize &previewImageSize) const Q_DECL_OVERRIDE;

    DesignerSupport *designerSupport() const;
    Qt5NodeInstanceServer *qt5NodeInstanceServer() const;

    static void createEffectItem(bool createEffectItem);

    void updateDirtyNodeRecursive() Q_DECL_OVERRIDE;

protected:
    QuickItemNodeInstance(QQuickItem*);
    QQuickItem *quickItem() const;
    void resetHorizontal();
    void resetVertical();
    void refresh();
    QRectF boundingRectWithStepChilds(QQuickItem *parentItem) const;
    void updateDirtyNodeRecursive(QQuickItem *parentItem) const;
    void updateAllDirtyNodeRecursive(QQuickItem *parentItem) const;
    static bool anyItemHasContent(QQuickItem *graphicsItem);
    static bool childItemsHaveContent(QQuickItem *graphicsItem);

private: //variables
    bool m_hasHeight;
    bool m_hasWidth;
    bool m_isResizable;
    bool m_hasContent;
    bool m_isMovable;
    double m_x;
    double m_y;
    double m_width;
    double m_height;
    static bool s_createEffectItem;
};

} // namespace Internal
} // namespace QmlDesigner

#endif  // QuickITEMNODEINSTANCE_H

