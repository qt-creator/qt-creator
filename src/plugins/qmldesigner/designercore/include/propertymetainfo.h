// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_global.h>

#include <projectstorage/projectstoragefwd.h>
#include <projectstorage/projectstoragetypes.h>
#include <projectstorageids.h>

#include <QString>

#include <memory>
#include <optional>
#include <vector>

namespace QmlDesigner {

class NodeMetaInfo;
class NodeMetaInfoPrivate;

class QMLDESIGNERCORE_EXPORT PropertyMetaInfo
{
public:
    PropertyMetaInfo();
    PropertyMetaInfo(std::shared_ptr<NodeMetaInfoPrivate> nodeMetaInfoPrivateData,
                     const PropertyName &propertyName);
    PropertyMetaInfo([[maybe_unused]] PropertyDeclarationId id,
                     [[maybe_unused]] NotNullPointer<const ProjectStorageType> projectStorage)
#ifdef QDS_USE_PROJECTSTORAGE
        : m_projectStorage{projectStorage}
        , m_id{id}
#endif
    {}
    PropertyMetaInfo(const PropertyMetaInfo &);
    PropertyMetaInfo &operator=(const PropertyMetaInfo &);
    PropertyMetaInfo(PropertyMetaInfo &&);
    PropertyMetaInfo &operator=(PropertyMetaInfo &&);
    ~PropertyMetaInfo();

    explicit operator bool() const { return isValid(); }

    bool isValid() const
    {
#ifdef QDS_USE_PROJECTSTORAGE
        return bool(m_id);
#else
        return bool(m_nodeMetaInfoPrivateData);
#endif
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
#ifdef QDS_USE_PROJECTSTORAGE
        return first.m_id == second.m_id;
#else
        return first.m_nodeMetaInfoPrivateData == second.m_nodeMetaInfoPrivateData
               && first.name() == second.name();
#endif
    }

    const ProjectStorageType &projectStorage() const { return *m_projectStorage; }

private:
    const Storage::Info::PropertyDeclaration &propertyData() const;
    TypeName propertyTypeName() const;
    const NodeMetaInfoPrivate *nodeMetaInfoPrivateData() const;
    const PropertyName &propertyName() const;

private:
    NotNullPointer<const ProjectStorageType> m_projectStorage;
    mutable std::optional<Storage::Info::PropertyDeclaration> m_propertyData;
    PropertyDeclarationId m_id;
#ifndef QDS_USE_PROJECTSTORAGE
    std::shared_ptr<NodeMetaInfoPrivate> m_nodeMetaInfoPrivateData;
    PropertyName m_propertyName;
#endif
};

using PropertyMetaInfos = std::vector<PropertyMetaInfo>;

struct CompoundPropertyMetaInfo
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

CompoundPropertyMetaInfos inflateValueProperties(PropertyMetaInfos properties);
CompoundPropertyMetaInfos inflateValueAndReadOnlyProperties(PropertyMetaInfos properties);
} // namespace MetaInfoUtils

} // namespace QmlDesigner
