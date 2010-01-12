/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qmlviewnodeinstance.h"


#include <QmlMetaType>
#include <QmlView>
#include <QmlGraphicsItem>

#include <invalidnodeinstanceexception.h>

namespace QmlDesigner {
namespace Internal {

QmlViewNodeInstance::QmlViewNodeInstance(QmlView *view)
  : GraphicsViewNodeInstance(view)
{
}


QmlViewNodeInstance::Pointer QmlViewNodeInstance::create(const NodeMetaInfo &nodeMetaInfo, QmlContext *context, QObject *objectToBeWrapped)
{
    QObject *object = 0;
    if (objectToBeWrapped)
        object = objectToBeWrapped;
    else
        createObject(nodeMetaInfo, context);

    QmlView* view = qobject_cast<QmlView*>(object);
    if (view == 0)
        throw InvalidNodeInstanceException(__LINE__, __FUNCTION__, __FILE__);

    Pointer instance(new QmlViewNodeInstance(view));

    if (objectToBeWrapped)
        instance->setDeleteHeldInstance(false); // the object isn't owned

    instance->populateResetValueHash();

    return instance;
}

QmlView* QmlViewNodeInstance::view() const
{
    QmlView* view = qobject_cast<QmlView*>(widget());
    Q_ASSERT(view);
    return view;
}

bool QmlViewNodeInstance::isQmlView() const
{
    return true;
}

void QmlViewNodeInstance::addItem(QmlGraphicsItem *item)
{
    item->setParent(view()->root());
}

}
}
