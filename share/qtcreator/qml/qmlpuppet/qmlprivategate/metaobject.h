/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QQmlContext>
#include <QScopedPointer>
#include <private/qqmlopenmetaobject_p.h>
#include <private/qqmlvmemetaobject_p.h>

#include "nodeinstanceglobal.h"

namespace QmlDesigner {

namespace Internal {

namespace QmlPrivateGate {

void createNewDynamicProperty(QObject *object, QQmlEngine *engine, const QString &name);
void registerNodeInstanceMetaObject(QObject *object, QQmlEngine *engine);

struct MetaPropertyData;

class MetaObject : public QQmlVMEMetaObject
{
public:
    ~MetaObject() override;

    static void registerNotifyPropertyChangeCallBack(void (*callback)(QObject*, const PropertyName &propertyName));

protected:
    MetaObject(QObject *object, QQmlEngine *engine);
    static MetaObject* getNodeInstanceMetaObject(QObject *object, QQmlEngine *engine);

    void createNewDynamicProperty(const QString &name);
    int openMetaCall(QMetaObject::Call _c, int _id, void **_a);
    using QQmlVMEMetaObject::metaCall;
    int metaCall(QMetaObject::Call _c, int _id, void **_a) override;
    void notifyPropertyChange(int id);
    void setValue(int id, const QVariant &value);
    QVariant propertyWriteValue(int, const QVariant &);

    QObject *myObject() const { return QQmlVMEMetaObject::object; }
    QAbstractDynamicMetaObject *parent() const { return const_cast<QAbstractDynamicMetaObject *>(dynamicMetaObjectParent()); }

    const QAbstractDynamicMetaObject *dynamicMetaObjectParent() const;

    const QMetaObject *metaObjectParent() const;

    int propertyOffset() const;

    int count() const;
    QByteArray name(int) const;

    void copyTypeMetaObject();

private:
    void init(QObject *, QQmlEngine *engine);

    QPointer<QQmlContext> m_context;
    QQmlOpenMetaObjectType *m_type;
    QScopedPointer<MetaPropertyData> m_data;
    //QAbstractDynamicMetaObject *m_parent;
    QQmlPropertyCache *m_cache;


    friend void createNewDynamicProperty(QObject *object, QQmlEngine *engine, const QString &name);
    friend void registerNodeInstanceMetaObject(QObject *object, QQmlEngine *engine);
};

} // namespace QmlPrivateGate
} // namespace Internal
} // namespace QmlDesigner
