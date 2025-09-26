// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_global.h>

#include <projectstorage/projectstoragefwd.h>
#include <projectstorage/projectstoragetypes.h>
#include <projectstorageids.h>

#include <utils/smallstring.h>

#include <QString>

#include <memory>
#include <optional>
#include <vector>

namespace QmlDesigner {

class NodeMetaInfo;

class QMLDESIGNERCORE_EXPORT PropertyMetaInfo
{
public:
    PropertyMetaInfo() = default;

    PropertyMetaInfo([[maybe_unused]] PropertyDeclarationId id,
                     [[maybe_unused]] NotNullPointer<const ProjectStorageType> projectStorage)
        : m_projectStorage{projectStorage}
        , m_id{id}
    {}

    static PropertyMetaInfo create(NotNullPointer<const ProjectStorageType> projectStorage,
                                   PropertyDeclarationId id)
    {
        return {id, projectStorage};
    }

    static auto bind(NotNullPointer<const ProjectStorageType> projectStorage)
    {
        return std::bind_front(&PropertyMetaInfo::create, projectStorage);
    }

    explicit operator bool() const { return isValid(); }

    bool isValid() const
    {
        return bool(m_id);
    }

    PropertyDeclarationId id() const { return m_id; }

    PropertyName name() const;
    NodeMetaInfo propertyType() const;
    NodeMetaInfo type() const;
    bool isWritable() const;
    bool isReadOnly() const;
    bool isListProperty() const;
    bool isEnumType() const;
    bool isPrivate() const;
    bool isPointer() const;
    QVariant castedValue(const QVariant &value) const;

    friend bool operator==(const PropertyMetaInfo &first, const PropertyMetaInfo &second)
    {
        return first.m_id == second.m_id && first.m_projectStorage == second.m_projectStorage;
    }

    const ProjectStorageType &projectStorage() const { return *m_projectStorage; }

private:
    const Storage::Info::PropertyDeclaration &propertyData() const;

private:
    NotNullPointer<const ProjectStorageType> m_projectStorage;
    mutable std::optional<Storage::Info::PropertyDeclaration> m_propertyData;
    PropertyDeclarationId m_id;
};

using PropertyMetaInfos = std::vector<PropertyMetaInfo>;

struct QMLDESIGNERCORE_EXPORT CompoundPropertyMetaInfo
{
    CompoundPropertyMetaInfo(PropertyMetaInfo &&property)
        : property(std::move(property))
    {}

    CompoundPropertyMetaInfo(PropertyMetaInfo &&property, const PropertyMetaInfo &parent)
        : property(std::move(property))
        , parent(parent)
    {}

    PropertyName name() const
    {
        if (parent)
            return parent.name() + '.' + property.name();

        return property.name();
    }

    PropertyMetaInfo property;
    PropertyMetaInfo parent;
};

using CompoundPropertyMetaInfos = std::vector<CompoundPropertyMetaInfo>;

namespace MetaInfoUtils {

QMLDESIGNERCORE_EXPORT CompoundPropertyMetaInfos inflateValueProperties(PropertyMetaInfos properties);
QMLDESIGNERCORE_EXPORT CompoundPropertyMetaInfos inflateValueAndReferenceProperties(PropertyMetaInfos properties);
QMLDESIGNERCORE_EXPORT CompoundPropertyMetaInfos addInflatedValueAndReferenceProperties(PropertyMetaInfos properties);
} // namespace MetaInfoUtils

} // namespace QmlDesigner
