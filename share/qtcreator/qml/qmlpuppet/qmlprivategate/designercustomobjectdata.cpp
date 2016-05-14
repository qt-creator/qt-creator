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

#include "designercustomobjectdata.h"

#include <qmlprivategate.h>

#include <QQmlContext>
#include <QQmlEngine>

#include <private/qqmlbinding_p.h>

namespace QmlDesigner {

namespace Internal {

namespace QmlPrivateGate {

QHash<QObject *, DesignerCustomObjectData*> m_objectToDataHash;

DesignerCustomObjectData::DesignerCustomObjectData(QObject *object)
    : m_object(object)
{
    if (object) {
        populateResetHashes();
        m_objectToDataHash.insert(object, this);
        QObject::connect(object, &QObject::destroyed, [=] {
            m_objectToDataHash.remove(object);
            delete this;
        });
    }
}

void DesignerCustomObjectData::registerData(QObject *object)
{
    new DesignerCustomObjectData(object);
}

DesignerCustomObjectData *DesignerCustomObjectData::get(QObject *object)
{
    return m_objectToDataHash.value(object);
}

QVariant DesignerCustomObjectData::getResetValue(QObject *object, const PropertyName &propertyName)
{
    DesignerCustomObjectData* data = get(object);

    if (data)
        return data->getResetValue(propertyName);

    return QVariant();
}

void DesignerCustomObjectData::doResetProperty(QObject *object, QQmlContext *context, const PropertyName &propertyName)
{
    DesignerCustomObjectData* data = get(object);

    if (data)
        data->doResetProperty(context, propertyName);
}

bool DesignerCustomObjectData::hasValidResetBinding(QObject *object, const PropertyName &propertyName)
{
    DesignerCustomObjectData* data = get(object);

    if (data)
        return data->hasValidResetBinding(propertyName);

    return false;
}

bool DesignerCustomObjectData::hasBindingForProperty(QObject *object, QQmlContext *context, const PropertyName &propertyName, bool *hasChanged)
{
    DesignerCustomObjectData* data = get(object);

    if (data)
        return data->hasBindingForProperty(context, propertyName, hasChanged);

    return false;
}

void DesignerCustomObjectData::setPropertyBinding(QObject *object, QQmlContext *context, const PropertyName &propertyName, const QString &expression)
{
    DesignerCustomObjectData* data = get(object);

    if (data)
        data->setPropertyBinding(context, propertyName, expression);
}

void DesignerCustomObjectData::keepBindingFromGettingDeleted(QObject *object, QQmlContext *context, const PropertyName &propertyName)
{
    DesignerCustomObjectData* data = get(object);

    if (data)
        data->keepBindingFromGettingDeleted(context, propertyName);
}

void DesignerCustomObjectData::populateResetHashes()
{
    PropertyNameList propertyNameList = QmlPrivateGate::propertyNameListForWritableProperties(object());

    foreach (const PropertyName &propertyName, propertyNameList) {
        QQmlProperty property(object(), QString::fromUtf8(propertyName), QQmlEngine::contextForObject(object()));

#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
        QQmlAbstractBinding::Ptr binding = QQmlAbstractBinding::Ptr(QQmlPropertyPrivate::binding(property));
#else
        QQmlAbstractBinding::Pointer binding = QQmlAbstractBinding::getPointer(QQmlPropertyPrivate::binding(property));
#endif

        if (binding) {
            m_resetBindingHash.insert(propertyName, binding);
        } else if (property.isWritable()) {
            m_resetValueHash.insert(propertyName, property.read());
        }
    }

    m_resetValueHash.insert("Layout.rowSpan", 1);
    m_resetValueHash.insert("Layout.columnSpan", 1);
    m_resetValueHash.insert("Layout.fillHeight", false);
    m_resetValueHash.insert("Layout.fillWidth", false);
}

QObject *DesignerCustomObjectData::object() const
{
    return m_object;
}

QVariant DesignerCustomObjectData::getResetValue(const PropertyName &propertyName) const
{
    return m_resetValueHash.value(propertyName);
}

void DesignerCustomObjectData::doResetProperty(QQmlContext *context, const PropertyName &propertyName)
{
    QQmlProperty property(object(), QString::fromUtf8(propertyName), context);

    if (!property.isValid())
        return;

    QVariant oldValue = property.read();
    if (oldValue.type() == QVariant::Url) {
        QUrl url = oldValue.toUrl();
        QString path = url.toLocalFile();
        /* ### TODO
        if (QFileInfo(path).exists() && nodeInstanceServer())
            nodeInstanceServer()->removeFilePropertyFromFileSystemWatcher(object(), propertyName, path);
            */
    }


    QQmlAbstractBinding *binding = QQmlPropertyPrivate::binding(property);
    if (binding && !(hasValidResetBinding(propertyName) && getResetBinding(propertyName) == binding)) {
        binding->setEnabled(false, 0);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
        //Refcounting is taking care
#else
        binding->destroy();
#endif
    }


    if (hasValidResetBinding(propertyName)) {
        QQmlAbstractBinding *binding = getResetBinding(propertyName);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
        QQmlBinding *qmlBinding = dynamic_cast<QQmlBinding*>(binding);
        if (qmlBinding)
            qmlBinding->setTarget(property);
        QQmlPropertyPrivate::setBinding(binding, QQmlPropertyPrivate::None, QQmlPropertyPrivate::DontRemoveBinding);
        if (qmlBinding)
            qmlBinding->update();
#else
        QQmlPropertyPrivate::setBinding(property, binding, QQmlPropertyPrivate::DontRemoveBinding);
        binding->update();
#endif

    } else if (property.isResettable()) {
        property.reset();
    } else if (property.propertyTypeCategory() == QQmlProperty::List) {
        QQmlListReference list = qvariant_cast<QQmlListReference>(property.read());

        if (!QmlPrivateGate::hasFullImplementedListInterface(list)) {
            qWarning() << "Property list interface not fully implemented for Class " << property.property().typeName() << " in property " << property.name() << "!";
            return;
        }

        list.clear();
    } else if (property.isWritable()) {
        if (property.read() == getResetValue(propertyName))
            return;

        property.write(getResetValue(propertyName));
    }
}

bool DesignerCustomObjectData::hasValidResetBinding(const PropertyName &propertyName) const
{
    return m_resetBindingHash.contains(propertyName) &&  m_resetBindingHash.value(propertyName).data();
}

QQmlAbstractBinding *DesignerCustomObjectData::getResetBinding(const PropertyName &propertyName) const
{
    return m_resetBindingHash.value(propertyName).data();
}

bool DesignerCustomObjectData::hasBindingForProperty(QQmlContext *context, const PropertyName &propertyName, bool *hasChanged) const
{
    if (QmlPrivateGate::isPropertyBlackListed(propertyName))
        return false;

    QQmlProperty property(object(), QString::fromUtf8(propertyName), context);

    bool hasBinding = QQmlPropertyPrivate::binding(property);

    if (hasChanged) {
        *hasChanged = hasBinding != m_hasBindingHash.value(propertyName, false);
        if (*hasChanged)
            m_hasBindingHash.insert(propertyName, hasBinding);
    }

    return QQmlPropertyPrivate::binding(property);
}

void DesignerCustomObjectData::setPropertyBinding(QQmlContext *context, const PropertyName &propertyName, const QString &expression)
{
    QQmlProperty property(object(), QString::fromUtf8(propertyName), context);

    if (!property.isValid())
        return;

    if (property.isProperty()) {
        QQmlBinding *binding = new QQmlBinding(expression, object(), context);
        binding->setTarget(property);
        binding->setNotifyOnValueChanged(true);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
        QQmlPropertyPrivate::setBinding(binding, QQmlPropertyPrivate::None, QQmlPropertyPrivate::DontRemoveBinding);
        //Refcounting is taking take care of deletion
#else
        QQmlAbstractBinding *oldBinding = QQmlPropertyPrivate::setBinding(property, binding);
        if (oldBinding && !hasValidResetBinding(propertyName))
            oldBinding->destroy();
#endif
        binding->update();
        if (binding->hasError()) {
            if (property.property().userType() == QVariant::String)
                property.write(QVariant(QString("#%1#").arg(expression)));
        }

    } else {
        qWarning() << Q_FUNC_INFO << ": Cannot set binding for property" << propertyName << ": property is unknown for type";
    }
}

void DesignerCustomObjectData::keepBindingFromGettingDeleted(QQmlContext *context, const PropertyName &propertyName)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    //Refcounting is taking care
    Q_UNUSED(context)
    Q_UNUSED(propertyName)
#else
    QQmlProperty property(object(), QString::fromUtf8(propertyName), context);
    QQmlPropertyPrivate::setBinding(property, 0, QQmlPropertyPrivate::BypassInterceptor
                                    | QQmlPropertyPrivate::DontRemoveBinding);
#endif
}


} // namespace QmlPrivateGate
} // namespace Internal
} // namespace QmlDesigner

