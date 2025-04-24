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
        ScriptBinding = 3,
        SignalHandlerOldSyntax = 4,
        SignalHandlerNewSyntax = 5
    };

public:
    QmlRefactoring(const QmlJS::Document::Ptr &doc,
                   QmlDesigner::TextModifier &modifier,
                   Utils::span<const PropertyNameView> propertyOrder);

    bool reparseDocument();

    bool addImport(const Import &import);
    bool removeImport(const Import &import);

    bool addToArrayMemberList(int parentLocation, PropertyNameView propertyName, const QString &content);
    bool addToObjectMemberList(int parentLocation,
                               std::optional<int> nodeLocation,
                               const QString &content);
    bool addProperty(int parentLocation,
                     PropertyNameView name,
                     const QString &value,
                     PropertyType propertyType,
                     const TypeName &dynamicTypeName = TypeName());
    bool changeProperty(int parentLocation,
                        PropertyNameView name,
                        const QString &value,
                        PropertyType propertyType);
    bool changeObjectType(int nodeLocation, const QString &newType);

    bool moveObject(int objectLocation,
                    PropertyNameView targetPropertyName,
                    bool targetIsArray,
                    int targetParentObjectLocation);
    bool moveObjectBeforeObject(int movingObjectLocation, int beforeObjectLocation, bool inDefaultProperty);

    bool removeObject(int nodeLocation);
    bool removeProperty(int parentLocation, PropertyNameView name);

private:
    QmlJS::Document::Ptr qmlDocument;
    TextModifier *textModifier;
    Utils::span<const PropertyNameView> m_propertyOrder;
};

} // namespace QmlDesigner
