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

#include "nodeinstanceglobal.h"

#include <QHash>
#include <QObject>
#include <QVariant>

#include <private/qqmlbinding_p.h>

QT_BEGIN_NAMESPACE
class QQmlContext;
QT_END_NAMESPACE

namespace QmlDesigner {

namespace Internal {

namespace QmlPrivateGate {

class DesignerCustomObjectData
{
public:
    static void registerData(QObject *object);
    static DesignerCustomObjectData *get(QObject *object);
    static QVariant getResetValue(QObject *object, const PropertyName &propertyName);
    static void doResetProperty(QObject *object, QQmlContext *context, const PropertyName &propertyName);
    static bool hasValidResetBinding(QObject *object, const PropertyName &propertyName);
    static bool hasBindingForProperty(QObject *object, QQmlContext *context, const PropertyName &propertyName, bool *hasChanged);
    static void setPropertyBinding(QObject *object, QQmlContext *context, const PropertyName &propertyName, const QString &expression);
    static void keepBindingFromGettingDeleted(QObject *object, QQmlContext *context, const PropertyName &propertyName);

private:
    DesignerCustomObjectData(QObject *object);
    void populateResetHashes();
    QObject *object() const;
    QVariant getResetValue(const PropertyName &propertyName) const;
    void doResetProperty(QQmlContext *context, const PropertyName &propertyName);
    bool hasValidResetBinding(const PropertyName &propertyName) const;
    QQmlAbstractBinding *getResetBinding(const PropertyName &propertyName) const;
    bool hasBindingForProperty(QQmlContext *context, const PropertyName &propertyName, bool *hasChanged) const;
    void setPropertyBinding(QQmlContext *context, const PropertyName &propertyName, const QString &expression);
    void keepBindingFromGettingDeleted(QQmlContext *context, const PropertyName &propertyName);

    QObject *m_object;
    QHash<PropertyName, QVariant> m_resetValueHash;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    QHash<PropertyName, QQmlAbstractBinding::Ptr> m_resetBindingHash;
#else
    QHash<PropertyName, QWeakPointer<QQmlAbstractBinding> > m_resetBindingHash;
#endif
    mutable QHash<PropertyName, bool> m_hasBindingHash;
};

} // namespace QmlPrivateGate
} // namespace Internal
} // namespace QmlDesigner
