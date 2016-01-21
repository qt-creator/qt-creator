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

#include "metaobject.h"

#include <QSharedPointer>
#include <QMetaProperty>
#include <qnumeric.h>
#include <QDebug>

#include <private/qqmlengine_p.h>
#include <private/qqmlpropertycache_p.h>

namespace QmlDesigner {

namespace Internal {

namespace QmlPrivateGate {

static QHash<QDynamicMetaObjectData *, bool> nodeInstanceMetaObjectList;
static void (*notifyPropertyChangeCallBack)(QObject*, const PropertyName &propertyName) = 0;

struct MetaPropertyData {
    inline QPair<QVariant, bool> &getDataRef(int idx) {
        while (m_data.count() <= idx)
            m_data << QPair<QVariant, bool>(QVariant(), false);
        return m_data[idx];
    }

    inline QVariant &getData(int idx) {
        QPair<QVariant, bool> &prop = getDataRef(idx);
        if (!prop.second) {
            prop.first = QVariant();
            prop.second = true;
        }
        return prop.first;
    }

    inline bool hasData(int idx) const {
        if (idx >= m_data.count())
            return false;
        return m_data[idx].second;
    }

    inline int count() { return m_data.count(); }

    QList<QPair<QVariant, bool> > m_data;
};

static bool constructedMetaData(const QQmlVMEMetaData* data)
{
    return data->propertyCount == 0
#if QT_VERSION < QT_VERSION_CHECK(5, 6, 0)
            && data->varPropertyCount == 0
#endif
            && data->aliasCount == 0
            && data->signalCount == 0
            && data->methodCount == 0;
}

static QQmlVMEMetaData* fakeMetaData()
{
    QQmlVMEMetaData* data = new QQmlVMEMetaData;
#if QT_VERSION < QT_VERSION_CHECK(5, 6, 0)
    data->varPropertyCount = 0;
#endif
    data->propertyCount = 0;
    data->aliasCount = 0;
    data->signalCount = 0;
    data->methodCount = 0;

    return data;
}

static const QQmlVMEMetaData* vMEMetaDataForObject(QObject *object)
{
    QQmlVMEMetaObject *metaObject = QQmlVMEMetaObject::get(object);
    if (metaObject)
        return metaObject->metaData;

    return fakeMetaData();
}

static QQmlPropertyCache *cacheForObject(QObject *object, QQmlEngine *engine)
{
    QQmlVMEMetaObject *metaObject = QQmlVMEMetaObject::get(object);
    if (metaObject)
        return metaObject->cache;

    return QQmlEnginePrivate::get(engine)->cache(object);
}

MetaObject* MetaObject::getNodeInstanceMetaObject(QObject *object, QQmlEngine *engine)
{
    //Avoid setting up multiple MetaObjects on the same QObject
    QObjectPrivate *op = QObjectPrivate::get(object);
    QDynamicMetaObjectData *parent = op->metaObject;
    if (nodeInstanceMetaObjectList.contains(parent))
        return static_cast<MetaObject *>(parent);

    // we just create one and the ownership goes automatically to the object in nodeinstance see init method
    return new MetaObject(object, engine);
}

void MetaObject::init(QObject *object, QQmlEngine *engine)
{
    //Creating QQmlOpenMetaObjectType
    m_type = new QQmlOpenMetaObjectType(metaObjectParent(), engine);
    m_type->addref();
    //Assigning type to this
    copyTypeMetaObject();

    //Assign this to object
    QObjectPrivate *op = QObjectPrivate::get(object);
    op->metaObject = this;

    //create cache
    cache = m_cache = QQmlEnginePrivate::get(engine)->cache(this);
    cache->addref();

    //If our parent is not a VMEMetaObject we just se the flag to false again
    if (constructedMetaData(metaData))
        QQmlData::get(object)->hasVMEMetaObject = false;

    nodeInstanceMetaObjectList.insert(this, true);
    hasAssignedMetaObjectData = true;
}

MetaObject::MetaObject(QObject *object, QQmlEngine *engine)
    : QQmlVMEMetaObject(object, cacheForObject(object, engine), vMEMetaDataForObject(object)),
      m_context(engine->contextForObject(object)),
      m_data(new MetaPropertyData),
      m_cache(0)
{
    init(object, engine);

    QQmlData *ddata = QQmlData::get(object, false);

    //Assign cache to object
    if (ddata && ddata->propertyCache) {
        cache->setParent(ddata->propertyCache);
        cache->invalidate(engine, this);
        ddata->propertyCache = m_cache;
    }

}

MetaObject::~MetaObject()
{
    if (cache->count() > 1) // qml is crashing because the property cache is not removed from the engine
        cache->release();
    else
    m_type->release();

    nodeInstanceMetaObjectList.remove(this);
}

void MetaObject::createNewDynamicProperty(const QString &name)
{
    int id = m_type->createProperty(name.toUtf8());
    copyTypeMetaObject();
    setValue(id, QVariant());
    Q_ASSERT(id >= 0);
    Q_UNUSED(id);

    //Updating cache
    QQmlPropertyCache *oldParent = m_cache->parent();
    QQmlEnginePrivate::get(m_context->engine())->cache(this)->invalidate(m_context->engine(), this);
    m_cache->setParent(oldParent);

    QQmlProperty property(myObject(), name, m_context);
    Q_ASSERT(property.isValid());
}

void MetaObject::setValue(int id, const QVariant &value)
{
    QPair<QVariant, bool> &prop = m_data->getDataRef(id);
    prop.first = propertyWriteValue(id, value);
    prop.second = true;
    QMetaObject::activate(myObject(), id + m_type->signalOffset(), 0);
}

QVariant MetaObject::propertyWriteValue(int, const QVariant &value)
{
    return value;
}

const QAbstractDynamicMetaObject *MetaObject::dynamicMetaObjectParent() const
{
    if (QQmlVMEMetaObject::parent.isT1())
        return QQmlVMEMetaObject::parent.asT1()->toDynamicMetaObject(QQmlVMEMetaObject::object);
    else
        return 0;
}

const QMetaObject *MetaObject::metaObjectParent() const
{
    if (QQmlVMEMetaObject::parent.isT1())
        return QQmlVMEMetaObject::parent.asT1()->toDynamicMetaObject(QQmlVMEMetaObject::object);

    return QQmlVMEMetaObject::parent.asT2();
}

int MetaObject::propertyOffset() const
{
    return cache->propertyOffset();
}

int MetaObject::openMetaCall(QMetaObject::Call call, int id, void **a)
{
    if ((call == QMetaObject::ReadProperty || call == QMetaObject::WriteProperty)
            && id >= m_type->propertyOffset()) {
        int propId = id - m_type->propertyOffset();
        if (call == QMetaObject::ReadProperty) {
            //propertyRead(propId);
            *reinterpret_cast<QVariant *>(a[0]) = m_data->getData(propId);
        } else if (call == QMetaObject::WriteProperty) {
            if (propId <= m_data->count() || m_data->m_data[propId].first != *reinterpret_cast<QVariant *>(a[0]))  {
                //propertyWrite(propId);
                QPair<QVariant, bool> &prop = m_data->getDataRef(propId);
                prop.first = propertyWriteValue(propId, *reinterpret_cast<QVariant *>(a[0]));
                prop.second = true;
                //propertyWritten(propId);
                activate(myObject(), m_type->signalOffset() + propId, 0);
            }
        }
        return -1;
    } else {
        QAbstractDynamicMetaObject *directParent = parent();
        if (directParent)
            return directParent->metaCall(call, id, a);
        else
            return myObject()->qt_metacall(call, id, a);
    }
}

int MetaObject::metaCall(QMetaObject::Call call, int id, void **a)
{
    int metaCallReturnValue = -1;

    const QMetaProperty propertyById = QQmlVMEMetaObject::property(id);

    if (call == QMetaObject::WriteProperty
            && propertyById.userType() == QMetaType::QVariant
            && reinterpret_cast<QVariant *>(a[0])->type() == QVariant::Double
            && qIsNaN(reinterpret_cast<QVariant *>(a[0])->toDouble())) {
        return -1;
    }

    if (call == QMetaObject::WriteProperty
            && propertyById.userType() == QMetaType::Double
            && qIsNaN(*reinterpret_cast<double*>(a[0]))) {
        return -1;
    }

    if (call == QMetaObject::WriteProperty
            && propertyById.userType() == QMetaType::Float
            && qIsNaN(*reinterpret_cast<float*>(a[0]))) {
        return -1;
    }

    QVariant oldValue;

    if (call == QMetaObject::WriteProperty && !propertyById.hasNotifySignal())
    {
        oldValue = propertyById.read(myObject());
    }

    QAbstractDynamicMetaObject *directParent = parent();
    if (directParent && id < directParent->propertyOffset()) {
        metaCallReturnValue = directParent->metaCall(call, id, a);
    } else {
        openMetaCall(call, id, a);
    }

    /*
    if ((call == QMetaObject::WriteProperty || call == QMetaObject::ReadProperty) && metaCallReturnValue < 0) {
        if (objectNodeInstance
                && objectNodeInstance->nodeInstanceServer()
                && objectNodeInstance->nodeInstanceServer()->dummyContextObject()
                && !(objectNodeInstance && !objectNodeInstance->isRootNodeInstance()
                     && property(id).name() == QLatin1String("parent"))) {

            QObject *contextDummyObject = objectNodeInstance->nodeInstanceServer()->dummyContextObject();
            int properyIndex = contextDummyObject->metaObject()->indexOfProperty(propertyById.name());
            if (properyIndex >= 0)
                metaCallReturnValue = contextDummyObject->qt_metacall(call, properyIndex, a);
        }
    }
    */

    if (call == QMetaObject::WriteProperty
            && !propertyById.hasNotifySignal()
            && oldValue != propertyById.read(myObject()))
        notifyPropertyChange(id);

    return metaCallReturnValue;
}

void MetaObject::notifyPropertyChange(int id)
{
    const QMetaProperty propertyById = property(id);

    if (id < propertyOffset()) {
        if (notifyPropertyChangeCallBack)
            notifyPropertyChangeCallBack(myObject(), propertyById.name());
    } else {
        if (notifyPropertyChangeCallBack)
            notifyPropertyChangeCallBack(myObject(), name(id - propertyOffset()));
    }
}

int MetaObject::count() const
{
    return m_type->propertyCount();
}

QByteArray MetaObject::name(int idx) const
{
    return m_type->propertyName(idx);
}

void MetaObject::copyTypeMetaObject()
{
    *static_cast<QMetaObject *>(this) = *m_type->metaObject();
}

void MetaObject::registerNotifyPropertyChangeCallBack(void (*callback)(QObject *, const PropertyName &))
{
    notifyPropertyChangeCallBack = callback;
}

} // namespace QmlPrivateGate

} // namespace Internal

} // namespace QmlDesigner
