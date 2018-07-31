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

#include <QObject>
#include <QHash>
#include <QSharedPointer>

#include "nodeinstanceglobal.h"

namespace QmlDesigner {

namespace Internal {

class ObjectNodeInstance;

using ObjectNodeInstancePointer = QSharedPointer<ObjectNodeInstance>;
using ObjectNodeInstanceWeakPointer = QWeakPointer<ObjectNodeInstance>;

class NodeInstanceSignalSpy : public QObject
{
public:
    explicit NodeInstanceSignalSpy();

    void setObjectNodeInstance(const ObjectNodeInstancePointer &nodeInstance);

    int qt_metacall(QMetaObject::Call, int, void **) override;

protected:
    void registerObject(QObject *spiedObject);
    void registerProperty(const QMetaProperty &metaProperty, QObject *spiedObject, const PropertyName &propertyPrefix = PropertyName());
    void registerChildObject(const QMetaProperty &metaProperty, QObject *spiedObject);

private:
    int methodeOffset;
    QMultiHash<int, PropertyName> m_indexPropertyHash;
    QObjectList m_registeredObjectList;
    ObjectNodeInstanceWeakPointer m_objectNodeInstance;
};

} // namespace Internal
} // namespace QmlDesigner
