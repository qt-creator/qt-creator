/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef NODEINSTANCEMETAOBJECT_H
#define NODEINSTANCEMETAOBJECT_H

#include <QQmlContext>
#include <private/qqmlopenmetaobject_p.h>

namespace QmlDesigner {
namespace Internal {

class ObjectNodeInstance;
typedef QSharedPointer<ObjectNodeInstance> ObjectNodeInstancePointer;
typedef QWeakPointer<ObjectNodeInstance> ObjectNodeInstanceWeakPointer;

class NodeInstanceMetaObject : public QQmlOpenMetaObject
{
public:
    NodeInstanceMetaObject(const ObjectNodeInstancePointer &nodeInstance, QQmlEngine *engine);
    NodeInstanceMetaObject(const ObjectNodeInstancePointer &nodeInstance, QObject *object, const QString &prefix, QQmlEngine *engine);
    void createNewProperty(const QString &name);

protected:
    int metaCall(QMetaObject::Call _c, int _id, void **_a);
    void notifyPropertyChange(int id);

private:
    ObjectNodeInstanceWeakPointer m_nodeInstance;
    QString m_prefix;
    QWeakPointer<QQmlContext> m_context;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // NODEINSTANCEMETAOBJECT_H
