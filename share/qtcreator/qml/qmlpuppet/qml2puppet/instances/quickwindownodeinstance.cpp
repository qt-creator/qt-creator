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

#include "quickwindownodeinstance.h"


#include <QQuickItem>

#include <private/qquickitem_p.h>

namespace QmlDesigner {
namespace Internal {

QuickWindowNodeInstance::QuickWindowNodeInstance(QQuickWindow *item)
   : GraphicalNodeInstance(item)
{
}

QuickWindowNodeInstance::~QuickWindowNodeInstance()
{

}

QObject *QuickWindowNodeInstance::parent() const
{
    return 0;
}

QImage QuickWindowNodeInstance::renderImage() const
{
    /*
     Since the content item transucient
     we just fill an image with the window color
     */

    QRectF renderBoundingRect = boundingRect();

    QImage renderImage(renderBoundingRect.size().toSize(), QImage::Format_ARGB32_Premultiplied);

    QPalette palette;

    renderImage.fill(palette.color(QPalette::Window).rgba());

    return renderImage;
}

QuickWindowNodeInstance::Pointer QuickWindowNodeInstance::create(QObject *object)
{
    QQuickWindow *quickWindow = qobject_cast<QQuickWindow*>(object);

    Q_ASSERT(quickWindow);

    Pointer instance(new QuickWindowNodeInstance(quickWindow));

    instance->setHasContent(anyItemHasContent(quickWindow->contentItem()));
    quickWindow->contentItem()->setFlag(QQuickItem::ItemHasContents, true);

    static_cast<QQmlParserStatus*>(quickWindow->contentItem())->classBegin();

    instance->populateResetHashes();

    QQuickItemPrivate *privateItem = static_cast<QQuickItemPrivate*>(QObjectPrivate::get(quickWindow->contentItem()));

    if (privateItem->window) {
        if (!privateItem->parentItem)
            QQuickWindowPrivate::get(privateItem->window)->parentlessItems.remove(quickWindow->contentItem());
        privateItem->derefWindow();
        privateItem->window = 0;
    }

    return instance;
}



bool QuickWindowNodeInstance::isQuickWindow() const
{
    return true;
}


void QuickWindowNodeInstance::reparent(const ObjectNodeInstance::Pointer &oldParentInstance, const PropertyName &oldParentProperty, const ObjectNodeInstance::Pointer &newParentInstance, const PropertyName &newParentProperty)
{
    ObjectNodeInstance::reparent(oldParentInstance, oldParentProperty, newParentInstance, newParentProperty);

    DesignerSupport::updateDirtyNode(quickItem());
}

bool QuickWindowNodeInstance::isAnchoredBySibling() const
{
    return false;
}


QQuickItem *QuickWindowNodeInstance::quickItem() const
{
    if (object() == 0)
        return 0;

    return static_cast<QQuickWindow*>(object())->contentItem();
}

} // namespace Internal
} // namespace QmlDesigner

