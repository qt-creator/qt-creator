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
    void initialize(const ObjectNodeInstance::Pointer &objectNodeInstance);

    bool isQuickItem() const;

    QRectF boundingRect() const;
    QPointF position() const;
    QSizeF size() const;
    QTransform transform() const;
    QTransform customTransform() const;
    QTransform sceneTransform() const;
    double opacity() const;

    QObject *parent() const;

    double rotation() const;
    double scale() const;
    QPointF transformOriginPoint() const;
    double zValue() const;

    bool equalQuickItem(QQuickItem *item) const;

    bool hasContent() const;

    QList<ServerNodeInstance> childItems() const;
    QList<ServerNodeInstance> childItemsForChild(QQuickItem *childItem) const;

    bool isMovable() const;
    void setMovable(bool movable);

    void setPropertyVariant(const QString &name, const QVariant &value);
    void setPropertyBinding(const QString &name, const QString &expression);

    QVariant property(const QString &name) const;
    void resetProperty(const QString &name);

    void reparent(const ObjectNodeInstance::Pointer &oldParentInstance, const QString &oldParentProperty, const ObjectNodeInstance::Pointer &newParentInstance, const QString &newParentProperty);

    int penWidth() const;

    bool hasAnchor(const QString &name) const;
    QPair<QString, ServerNodeInstance> anchor(const QString &name) const;
    bool isAnchoredBySibling() const;
    bool isAnchoredByChildren() const;
    void doComponentComplete();

    bool isResizable() const;
    void setResizable(bool resizeable);

    void setHasContent(bool hasContent);

    QList<ServerNodeInstance> stateInstances() const;

    QImage renderImage() const;

    DesignerSupport *designerSupport() const;
    Qt5NodeInstanceServer *qt5NodeInstanceServer() const;

    static void createEffectItem(bool createEffectItem);

protected:
    QuickItemNodeInstance(QQuickItem*);
    QQuickItem *quickItem() const;
    void resetHorizontal();
    void resetVertical();
    void refresh();
    QRectF boundingRectWithStepChilds(QQuickItem *parentItem) const;
    void updateDirtyNodeRecursive(QQuickItem *parentItem) const;
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

