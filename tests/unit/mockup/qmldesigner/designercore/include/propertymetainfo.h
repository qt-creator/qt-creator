// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_global.h>

#include <QSharedPointer>
#include <QString>

#include <vector>

namespace QmlDesigner {

class PropertyMetaInfo
{
public:
    PropertyMetaInfo() = default;
    PropertyMetaInfo(QSharedPointer<class NodeMetaInfoPrivate>, const PropertyName &) {}
    ~PropertyMetaInfo() {}

    const TypeName &propertyTypeName() const
    {
        static TypeName foo;
        return foo;
    }
    class NodeMetaInfo propertyNodeMetaInfo() const;

    bool isWritable() const { return {}; }
    bool isListProperty() const { return {}; }
    bool isEnumType() const { return {}; }
    bool isPrivate() const { return {}; }
    bool isPointer() const { return {}; }
    QVariant castedValue(const QVariant &) const { return {}; }
    PropertyName name() const & { return {}; }

    template<typename... TypeName>
    bool hasPropertyTypeName(const TypeName &...typeName) const
    {
        auto propertyTypeName_ = propertyTypeName();
        return ((propertyTypeName_ == typeName) && ...);
    }

    bool propertyTypeNameIsUrl() const { return hasPropertyTypeName("QUrl", "url"); }
};

using PropertyMetaInfos = std::vector<PropertyMetaInfo>;

} // namespace QmlDesigner
