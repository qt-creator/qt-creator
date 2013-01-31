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

#ifndef NODEINSTANCEMETAOBJECT_H
#define NODEINSTANCEMETAOBJECT_H

#include <QQmlContext>
#include <QScopedPointer>
#include <private/qqmlopenmetaobject_p.h>
#include <private/qqmlvmemetaobject_p.h>

namespace QmlDesigner {
namespace Internal {

class ObjectNodeInstance;
typedef QSharedPointer<ObjectNodeInstance> ObjectNodeInstancePointer;
typedef QWeakPointer<ObjectNodeInstance> ObjectNodeInstanceWeakPointer;

struct MetaPropertyData;

class NodeInstanceMetaObject : public QQmlVMEMetaObject
{
public:
    static NodeInstanceMetaObject *createNodeInstanceMetaObject(const ObjectNodeInstancePointer &nodeInstance, QQmlEngine *engine);
    static NodeInstanceMetaObject *createNodeInstanceMetaObject(const ObjectNodeInstancePointer &nodeInstance, QObject *object, const QString &prefix, QQmlEngine *engine);
    ~NodeInstanceMetaObject();
    void createNewProperty(const QString &name);

protected:
    NodeInstanceMetaObject(const ObjectNodeInstancePointer &nodeInstance, QQmlEngine *engine);
    NodeInstanceMetaObject(const ObjectNodeInstancePointer &nodeInstance, QObject *object, const QString &prefix, QQmlEngine *engine);

    int openMetaCall(QMetaObject::Call _c, int _id, void **_a);
    int metaCall(QMetaObject::Call _c, int _id, void **_a);
    void notifyPropertyChange(int id);
    void setValue(int id, const QVariant &value);
    int createProperty(const char *, const char *);
    QVariant propertyWriteValue(int, const QVariant &);

    QObject *myObject() const { return QQmlVMEMetaObject::object; }
    QAbstractDynamicMetaObject *parent() const { return const_cast<QAbstractDynamicMetaObject *>(dynamicMetaObjectParent()); }

    const QAbstractDynamicMetaObject *dynamicMetaObjectParent() const
    {
        if (QQmlVMEMetaObject::parent.isT1())
            return QQmlVMEMetaObject::parent.asT1()->toDynamicMetaObject(QQmlVMEMetaObject::object);
        else
            return 0;
    }

    const QMetaObject *metaObjectParent() const
    {
        if (QQmlVMEMetaObject::parent.isT1())
            return QQmlVMEMetaObject::parent.asT1()->toDynamicMetaObject(QQmlVMEMetaObject::object);

        return QQmlVMEMetaObject::parent.asT2();
    }

    int propertyOffset() const { return cache->propertyOffset(); }

    int count() const;
    QByteArray name(int) const;

    void copyTypeMetaObject();

private:
    void init(QObject *, QQmlEngine *engine);

    ObjectNodeInstanceWeakPointer m_nodeInstance;
    QString m_prefix;
    QPointer<QQmlContext>  m_context;
    QQmlOpenMetaObjectType *m_type;
    QScopedPointer<MetaPropertyData> m_data;
    //QAbstractDynamicMetaObject *m_parent;
    QQmlPropertyCache *m_cache;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // NODEINSTANCEMETAOBJECT_H
