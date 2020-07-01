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

#include <QIcon>
#include <QList>
#include <QString>
#include <QVariant>

#include "qmldesignercorelib_global.h"

QT_BEGIN_NAMESPACE
class QDeclarativeContext;
QT_END_NAMESPACE

namespace QmlDesigner {

class MetaInfo;
class Model;
class AbstractProperty;

class NodeMetaInfo
{
public:
    NodeMetaInfo() {}
    NodeMetaInfo(Model *, const TypeName &, int, int) {}

    bool isValid() const { return {}; }
    bool isFileComponent() const { return {}; }
    bool hasProperty(const PropertyName &) const { return {}; }
    PropertyNameList propertyNames() const { return {}; }
    PropertyNameList signalNames() const { return {}; }
    PropertyNameList directPropertyNames() const { return {}; }
    PropertyName defaultPropertyName() const { return "data"; }
    bool hasDefaultProperty() const { return {}; }
    TypeName propertyTypeName(const PropertyName &) const { return {}; }
    bool propertyIsWritable(const PropertyName &) const { return {}; }
    bool propertyIsListProperty(const PropertyName &) const { return {}; }
    bool propertyIsEnumType(const PropertyName &) const { return {}; }
    bool propertyIsPrivate(const PropertyName &) const { return {}; }
    QString propertyEnumScope(const PropertyName &) const { return {}; }
    QStringList propertyKeysForEnum(const PropertyName &) const { return {}; }
    QVariant propertyCastedValue(const PropertyName &, const QVariant &) const { return {}; }

    QList<NodeMetaInfo> classHierarchy() const { return {}; }
    QList<NodeMetaInfo> superClasses() const { return {}; }
    NodeMetaInfo directSuperClass() const { return {}; }

    bool defaultPropertyIsComponent() const { return {}; }

    TypeName typeName() const { return {}; }
    TypeName simplifiedTypeName() const { return {}; }
    int majorVersion() const { return {}; }
    int minorVersion() const { return {}; }

    QString componentSource() const { return {}; }
    QString componentFileName() const { return {}; }

    bool hasCustomParser() const { return {}; }

    bool availableInVersion(int, int) const { return {}; }
    bool isSubclassOf(const TypeName &, int = -1, int = -1) const { return {}; }

    bool isGraphicalItem() const { return {}; }
    bool isLayoutable() const { return {}; }
    bool isView() const { return {}; }
    bool isTabView() const { return {}; }

    QString importDirectoryPath() const { return {}; }

    static void clearCache() {}
};

} // namespace QmlDesigner
