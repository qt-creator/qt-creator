// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "compatibleproperties.h"

#include <bindingproperty.h>
#include <qmlobjectnode.h>
#include <qmltimelinekeyframegroup.h>
#include <variantproperty.h>

namespace QmlDesigner {

/*!
 * \internal
 * \brief Extracts and removes compatible properties from \param properties.
 * compatible properties will be stored.
 */
void CompatibleProperties::createCompatibilityMap(QList<PropertyName> &properties)
{
    m_compatibilityMap.clear();
    if (m_oldInfo == m_newInfo || !m_oldInfo.isQtQuick3DMaterial() || !m_newInfo.isQtQuick3DMaterial())
        return;

    enum class MaterialType { Unknown, Principled, Default, SpecularGlossy };
    auto getMaterialType = [](const NodeMetaInfo &info) {
        if (info.isQtQuick3DPrincipledMaterial())
            return MaterialType::Principled;
        else if (info.isQtQuick3DDefaultMaterial())
            return MaterialType::Default;
        else if (info.isQtQuick3DSpecularGlossyMaterial())
            return MaterialType::SpecularGlossy;
        return MaterialType::Unknown;
    };

    static const QHash<MaterialType, PropertyName> baseColors = {
        {MaterialType::Principled, "baseColor"},
        {MaterialType::Default, "diffuseColor"},
        {MaterialType::SpecularGlossy, "albedoColor"},
    };

    static const QHash<MaterialType, PropertyName> baseMaps = {
        {MaterialType::Principled, "baseColorMap"},
        {MaterialType::Default, "diffuseMap"},
        {MaterialType::SpecularGlossy, "albedoMap"},
    };

    MaterialType sourceMaterialType = getMaterialType(m_oldInfo);
    MaterialType destMaterialType = getMaterialType(m_newInfo);

    if (sourceMaterialType == MaterialType::Unknown || destMaterialType == MaterialType::Unknown)
        return;

    if (properties.contains(baseColors.value(sourceMaterialType))) {
        m_compatibilityMap.insert(baseColors[sourceMaterialType], baseColors[destMaterialType]);
        properties.removeOne(baseColors[sourceMaterialType]);
    }

    if (properties.contains(baseMaps[sourceMaterialType])) {
        m_compatibilityMap.insert(baseMaps[sourceMaterialType], baseMaps[destMaterialType]);
        properties.removeOne(baseMaps[sourceMaterialType]);
    }
}

void CompatibleProperties::copyMappedProperties(const ModelNode &node)
{
    if (m_compatibilityMap.isEmpty())
        return;
    // Copy mapped properties to new name. Note that this will only copy the base
    // property value and adjust the keyframe groups. Any other bindings related
    // to the property will be ignored.
    const QList<ModelNode> timeLines = QmlObjectNode(node).allTimelines();
    for (const auto &pair : m_compatibilityMap.asKeyValueRange()) {
        const PropertyName &key = pair.first;
        CopyData &copyData = pair.second;
        for (const ModelNode &timeLineNode : timeLines) {
            QmlTimeline timeLine(timeLineNode);
            if (timeLine.hasKeyframeGroup(node, key)) {
                QmlTimelineKeyframeGroup group = timeLine.keyframeGroup(node, key);
                group.setPropertyName(copyData.name);
            }
        }
        // Property value itself cannot be copied until type has been changed, so store it
        AbstractProperty prop = node.property(key);
        if (prop.isValid()) {
            if (prop.isBindingProperty()) {
                copyData.isBinding = true;
                copyData.value = prop.toBindingProperty().expression();
            } else {
                copyData.value = prop.toVariantProperty().value();
            }
        }
        node.removeProperty(key);
    }
}

void CompatibleProperties::applyCompatibleProperties(const ModelNode &node)
{
    for (const auto &pair : m_compatibilityMap.asKeyValueRange()) {
        const CopyData &copyData = pair.second;
        if (copyData.isBinding) {
            BindingProperty property = node.bindingProperty(copyData.name);
            property.setExpression(copyData.value.toString());
        } else {
            VariantProperty property = node.variantProperty(copyData.name);
            property.setValue(copyData.value);
        }
    }
}

} // namespace QmlDesigner
