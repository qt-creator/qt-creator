// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_global.h>

#include <projectstorage/projectstoragefwd.h>
#include <projectstorage/projectstoragetypes.h>
#include <projectstorageids.h>

#include <QSharedPointer>
#include <QString>

#include <optional>
#include <vector>

namespace QmlDesigner {

class NodeMetaInfo;

class QMLDESIGNERCORE_EXPORT PropertyMetaInfo
{
public:
    PropertyMetaInfo() = default;
    PropertyMetaInfo(QSharedPointer<class NodeMetaInfoPrivate> nodeMetaInfoPrivateData,
                     const PropertyName &propertyName);
    PropertyMetaInfo(PropertyDeclarationId id,
                     NotNullPointer<const ProjectStorage<Sqlite::Database>> projectStorage)
        : m_id{id}
        , m_projectStorage{projectStorage}
    {}
    ~PropertyMetaInfo();

    explicit operator bool() const { return isValid(); }

    bool isValid() const
    {
        if (useProjectStorage()) {
            return bool(m_id);
        } else {
            return bool(m_nodeMetaInfoPrivateData);
        }
    }
    PropertyName name() const;
    NodeMetaInfo propertyType() const;
    bool isWritable() const;
    bool isListProperty() const;
    bool isEnumType() const;
    bool isPrivate() const;
    bool isPointer() const;
    QVariant castedValue(const QVariant &value) const;

    friend bool operator==(const PropertyMetaInfo &first, const PropertyMetaInfo &second)
    {
        return first.m_nodeMetaInfoPrivateData == second.m_nodeMetaInfoPrivateData
               && first.name() == second.name();
    }

private:
    const Storage::Info::PropertyDeclaration &propertyData() const;
    TypeName propertyTypeName() const;

private:
    QSharedPointer<class NodeMetaInfoPrivate> m_nodeMetaInfoPrivateData;
    PropertyName m_propertyName;
    PropertyDeclarationId m_id;
    NotNullPointer<const ProjectStorage<Sqlite::Database>> m_projectStorage;
    mutable std::optional<Storage::Info::PropertyDeclaration> m_propertyData;
};

using PropertyMetaInfos = std::vector<PropertyMetaInfo>;

} // namespace QmlDesigner
