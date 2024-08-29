// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "propertyeditorutils.h"

namespace QmlDesigner {

namespace PropertyEditorUtils {

#ifndef QDS_USE_PROJECTSTORAGE

static bool checkIfUnkownTypeProperty(const std::vector<PropertyName> &propertyNames,
                                      const PropertyName &name)
{
    int i = 0;

    PropertyName prefix = name + '.';

    for (auto propertyName : propertyNames) {
        if (propertyName.startsWith(prefix))
            i++;

        if (i > 10)
            return true;
    }

    return false;
}

#endif //QDS_USE_PROJECTSTORAGE

PropertyMetaInfos filteredProperties(const NodeMetaInfo &metaInfo)
{
    auto properties = metaInfo.properties();

#ifndef QDS_USE_PROJECTSTORAGE
    std::vector<TypeName> names = Utils::transform(properties, &PropertyMetaInfo::name);

    std::vector<PropertyName> itemProperties;

    for (const auto &property : properties) {
        if (!property.name().contains('.') && property.propertyType().prototypes().size() == 0
            && checkIfUnkownTypeProperty(names, property.name())) {
            itemProperties.push_back(property.name());
        }
    }

    if (properties.size() > 1000) {
        properties = Utils::filtered(properties, [&itemProperties](const PropertyMetaInfo &metaInfo) {
            if (metaInfo.name().contains('.')) {
                const auto splitted = metaInfo.name().split('.');
                if (std::find(itemProperties.begin(), itemProperties.end(), splitted.first())
                    != itemProperties.end())
                    return false;
            }

            return true;
        });
    }
#endif //QDS_USE_PROJECTSTORAGE

    return properties;
}

} // End namespace PropertyEditorUtils.

} // End namespace QmlDesigner.
