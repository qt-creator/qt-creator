// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <import.h>
#include <textmodifier.h>
#include <QString>

namespace QmlDesigner {

class QmlRefactoring
{
    QmlRefactoring(const QmlRefactoring &);
    QmlRefactoring &operator=(const QmlRefactoring &);

public:
    enum PropertyType {
        Invalid = -1,
        ArrayBinding = 1,
        ObjectBinding = 2,
        ScriptBinding = 3
    };

public:
    QmlRefactoring(const QmlJS::Document::Ptr &doc, QmlDesigner::TextModifier &modifier, const PropertyNameList &propertyOrder);

    bool reparseDocument();

    bool addImport(const Import &import);
    bool removeImport(const Import &import);

    bool addToArrayMemberList(int parentLocation, const PropertyName &propertyName, const QString &content);
    bool addToObjectMemberList(int parentLocation, const QString &content);
    bool addProperty(int parentLocation,
                     const PropertyName &name,
                     const QString &value,
                     PropertyType propertyType,
                     const TypeName &dynamicTypeName = TypeName());
    bool changeProperty(int parentLocation, const PropertyName &name, const QString &value, PropertyType propertyType);
    bool changeObjectType(int nodeLocation, const QString &newType);

    bool moveObject(int objectLocation, const PropertyName &targetPropertyName, bool targetIsArray, int targetParentObjectLocation);
    bool moveObjectBeforeObject(int movingObjectLocation, int beforeObjectLocation, bool inDefaultProperty);

    bool removeObject(int nodeLocation);
    bool removeProperty(int parentLocation, const PropertyName &name);

private:
    QmlJS::Document::Ptr qmlDocument;
    TextModifier *textModifier;
    PropertyNameList m_propertyOrder;
};

} // namespace QmlDesigner
