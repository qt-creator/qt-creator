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

#include "modelutilities.h"
#include <metainfo.h>
#include <model.h>
#include "model/propertyparser.h"
#include <QtCore/QDebug>
#include <QtDeclarative/QmlMetaType>
#include <QtCore/QMetaProperty>
#include <widgetqueryview.h>

namespace QmlDesigner {
using namespace Internal;
namespace ModelUtilities {



bool mustBeProxied(const ModelNode &child, const ModelNode &parent)
{
    return child.metaInfo().isSubclassOf("QWidget") &&
    (parent.metaInfo().isSubclassOf("QGraphicsItem") ||
     parent.metaInfo().isSubclassOf("QGraphicsScene"));
    //if child widget is a QWidget and the target is either
    //QGraphicsItem or QGraphicsScene return true
}

//ModelNode reparentAndProxymize(const ModelNode &child, const ModelNode &parent)
//{
//    ModelNode oldProxy;
//    ModelNode oldParent(parent);
//    if (child.parentNode().type() == "QGraphicsProxyWidget")
//        oldProxy = child.parentNode();
//    if (mustBeProxied(child, parent)) {
//        if (oldProxy.isValid()) {
//            oldProxy.setParentNode(parent);
//            oldProxy = ModelNode();
//        } else {
//            ModelNode proxy = oldParent.addChildNode("QGraphicsProxyWidget");
//            const QString id = child.id() + "_proxy";
//            proxy.setPropertyValue("objectName", id);
//            proxy.setId(id);
//            return proxy;
//        }
//    } else {
//        return parent;
//    }
//    if (oldProxy.isValid())
//        oldProxy.remove();
//
//    return ModelNode();
//}

/* \brief Returns the QGraphicsScene for a QGraphicsView,
          if node is not derived from QGraphicsView it returns node
*/

QVariant parseProperty(const QString &className, const QString &propertyName, const QString &value)
{
    const QMetaObject *metaObject = QmlMetaType::qmlType(className.toAscii().constData(), 4, 6)->metaObject();
    if (!metaObject) {
        qWarning() << "Type " << className << "is unknown to the Qml type system";
        return QVariant();
    }
    const int propertyIndex = metaObject->indexOfProperty(propertyName.toAscii().constData());
    if (propertyIndex < 0) {
        return QVariant();
    }

    const QString typeName = QString(metaObject->property(propertyIndex).typeName());
    Q_ASSERT_X(!typeName.isEmpty(), Q_FUNC_INFO,
           QString("Type name for class %1 and property %2 is empty").arg(className, propertyName).toAscii().constData());
//    NodeMetaInfo metaInfo = MetaInfo::global().create(className);
//    PropertyMetaInfo property = metaInfo.property(propertyName);
//    QString type = property.type();

    return PropertyParser::read(typeName, value);
}

//void setAbsolutePosition(ModelNode node, QPointF position)
//{
//    QPointF newPos = position;
//    ModelNode parentNode;
//    parentNode = node.parentNode();
//    if (parentNode.isValid()) {
////        NodeInstance nodeInstance = instanceForNode(parentNode);
////        newPos = nodeInstance.mapFromGlobal(position);
//        Q_ASSERT(false);
//    }
//    node.setPropertyValue("x", newPos.x());
//    node.setPropertyValue("y", newPos.y());
//}




}  //namespace ModelUtilities
}  //namespace QmlDesigner

