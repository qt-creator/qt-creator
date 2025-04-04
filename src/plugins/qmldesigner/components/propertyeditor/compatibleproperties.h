// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <modelfwd.h>
#include <nodemetainfo.h>

#include <QHash>

namespace QmlDesigner {

// Currently this compatibility is only checked for the materials
// If there are more compatible types in the future, we can make a proxy model for Compatibility
class CompatibleProperties
{
public:
    CompatibleProperties(const NodeMetaInfo &oldInfo, NodeMetaInfo &newInfo)
        : m_oldInfo(oldInfo)
        , m_newInfo(newInfo)
    {}

    void createCompatibilityMap(QList<PropertyName> &properties);
    void copyMappedProperties(const ModelNode &node);
    void applyCompatibleProperties(const ModelNode &node);

private:
    void *operator new(size_t) = delete;

    struct CopyData
    {
        CopyData() = default;

        CopyData(PropertyName n)
            : name(n)
        {}

        PropertyName name;
        QVariant value;
        bool isBinding = false;
    };

    NodeMetaInfo m_oldInfo;
    NodeMetaInfo m_newInfo;

    QHash<PropertyName, CopyData> m_compatibilityMap;
};

} // namespace QmlDesigner
