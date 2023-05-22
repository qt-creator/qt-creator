// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    PropertyMetaInfo([[maybe_unused]] PropertyDeclarationId id,
                     [[maybe_unused]] NotNullPointer<const ProjectStorageType> projectStorage)
#ifdef QDS_USE_PROJECTSTORAGE
        : m_projectStorage{projectStorage}
        , m_id{id}
#endif
    {}
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
#ifdef QDS_USE_PROJECTSTORAGE
        return first.m_id == second.m_id;
#else
        return first.m_nodeMetaInfoPrivateData == second.m_nodeMetaInfoPrivateData
               && first.name() == second.name();
#endif
    }

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
    QSharedPointer<class NodeMetaInfoPrivate> m_nodeMetaInfoPrivateData;
    PropertyName m_propertyName;
#endif
};

using PropertyMetaInfos = std::vector<PropertyMetaInfo>;

} // namespace QmlDesigner
