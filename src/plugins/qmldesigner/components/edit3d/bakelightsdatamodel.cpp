// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "bakelightsdatamodel.h"

#include "abstractview.h"
#include "bakelights.h"
#include "bindingproperty.h"
#include "enumeration.h"
#include "externaldependenciesinterface.h"
#include "model.h"
#include "modelnode.h"
#include "nodelistproperty.h"
#include "nodemetainfo.h"
#include "qmlobjectnode.h"
#include "variantproperty.h"

#include <utils/expected.h>
#include <utils/filepath.h>
#include <utils/qtcassert.h>

#include <algorithm>

namespace QmlDesigner {


BakeLightsDataModel::BakeLightsDataModel(AbstractView *view)
    : QAbstractListModel(view)
    , m_view(view)
{
}

BakeLightsDataModel::~BakeLightsDataModel()
{
}

int BakeLightsDataModel::rowCount(const QModelIndex &) const
{
    return m_dataList.size();
}

QHash<int, QByteArray> BakeLightsDataModel::roleNames() const
{
    static const QHash<int, QByteArray> roles {
        {Qt::UserRole + 1, "displayId"},
        {Qt::UserRole + 2, "nodeId"},
        {Qt::UserRole + 3, "isModel"},
        {Qt::UserRole + 4, "isEnabled"},
        {Qt::UserRole + 5, "inUse"},
        {Qt::UserRole + 6, "isTitle"},
        {Qt::UserRole + 7, "isUnexposed"},
        {Qt::UserRole + 8, "resolution"},
        {Qt::UserRole + 9, "bakeMode"}
    };
    return roles;
}

QVariant BakeLightsDataModel::data(const QModelIndex &index, int role) const
{
    QTC_ASSERT(index.isValid() && index.row() < m_dataList.size(), return {});
    QTC_ASSERT(roleNames().contains(role), return {});

    QByteArray roleName = roleNames().value(role);
    const BakeData &bakeData = m_dataList[index.row()];

    if (roleName == "displayId") {
        const QString id = bakeData.id;
        const PropertyName aliasProp = bakeData.aliasProp;
        if (aliasProp.isEmpty())
            return id;
        else
            return QVariant{id + " - " + QString::fromUtf8(aliasProp)};
        return {};
    }

    if (roleName == "nodeId")
        return bakeData.id;

    if (roleName == "isModel")
        return bakeData.isModel;

    if (roleName == "isEnabled")
        return bakeData.enabled;

    if (roleName == "inUse")
        return bakeData.inUse;

    if (roleName == "isTitle")
        return bakeData.isTitle;

    if (roleName == "isUnexposed")
        return bakeData.isUnexposed;

    if (roleName == "resolution")
        return bakeData.resolution;

    if (roleName == "bakeMode")
        return bakeData.bakeMode;

    return {};
}

bool BakeLightsDataModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    QTC_ASSERT(index.isValid() && index.row() < m_dataList.size(), return false);
    QTC_ASSERT(roleNames().contains(role), return false);

    QByteArray roleName = roleNames().value(role);
    BakeData &bakeData = m_dataList[index.row()];

    bool changed = false;

    if (roleName == "isEnabled") {
        changed = bakeData.enabled != value.toBool();
        bakeData.enabled = value.toBool();
    } else if (roleName == "inUse") {
        changed = bakeData.inUse != value.toBool();
        bakeData.inUse = value.toBool();
    } else if (roleName == "resolution") {
        changed = bakeData.resolution != value.toInt();
        bakeData.resolution = value.toInt();
    } else if (roleName == "bakeMode") {
        changed = bakeData.bakeMode != value.toString();
        bakeData.bakeMode = value.toString();
    }

    if (changed)
        emit dataChanged(index, index, {role});

    return changed;
}

// Return value of false indicates unexpected property assignments were detected, which means manual
// mode for baking setup should be enforced.
bool BakeLightsDataModel::reset()
{
    if (!m_view || !m_view->model())
        return true;

    beginResetModel();
    m_dataList.clear();

    m_view3dNode = BakeLights::resolveView3dNode(m_view);

    // Find all models and lights in active View3D
    QList<ModelNode> nodes = m_view3dNode.allSubModelNodes();

    if (m_view3dNode.hasBindingProperty("importScene"))
        nodes.append(m_view3dNode.bindingProperty("importScene").resolveToModelNode().allSubModelNodesAndThisNode());

    QList<BakeData> modelList;
    QList<BakeData> lightList;
    QList<BakeData> compModelList;
    QList<BakeData> compLightList;
    QList<BakeData> unexposedList;

    bool forceManualMode = false;

    // Note: We are always loading base state values for baking. If users want to bake
    // differently for different states, they need to setup things manually for now.
    // Same goes if they want to use any unusual bindings in baking properties.
    for (const auto &node : std::as_const(nodes)) {
        if (QmlObjectNode(node).hasError())
            continue;

        BakeData data;
        data.id = node.id();
        if (data.id.isEmpty())
            continue; // Skip nodes without id

        if (node.metaInfo().isQtQuick3DModel()) {
            data.isModel = true;
            if (node.hasBindingProperty("bakedLightmap")) {
                ModelNode blm = node.bindingProperty("bakedLightmap").resolveToModelNode();
                if (blm.isValid()) {
                    if (blm.hasVariantProperty("enabled")) {
                        data.enabled = blm.variantProperty("enabled").value().toBool();
                    } else {
                        data.enabled = true;
                        if (blm.hasProperty("enabled"))
                            forceManualMode = true;
                    }
                }
            } else if (node.hasProperty("bakedLightmap")) {
                forceManualMode = true;
            }
            if (node.hasVariantProperty("lightmapBaseResolution"))
                data.resolution = node.variantProperty("lightmapBaseResolution").value().toInt();
            else if (node.hasProperty("lightmapBaseResolution"))
                forceManualMode = true;

            if (node.hasVariantProperty("usedInBakedLighting"))
                data.inUse = node.variantProperty("usedInBakedLighting").value().toBool();
            else if (node.hasProperty("usedInBakedLighting"))
                forceManualMode = true;

            modelList.append(data);
        } else if (node.metaInfo().isQtQuick3DLight()) {
            if (node.hasVariantProperty("bakeMode")) {
                // Enum properties that have binding can still resolve as variant property,
                // so check if the value is actually valid enum
                QString bakeModeStr = node.variantProperty("bakeMode").value()
                                          .value<QmlDesigner::Enumeration>().toString();
                if (bakeModeStr.startsWith("Light.BakeMode"))
                    data.bakeMode = bakeModeStr;
                else
                    forceManualMode = true;
            } else {
                data.bakeMode = "Light.BakeModeDisabled";
                if (node.hasProperty("bakeMode"))
                    forceManualMode = true;
            }
            lightList.append(data);
        }

        if (node.isComponent()) {
            // Every component can expose multiple aliases
            // We ignore baking properties defined inside the component (no visibility there)
            bool hasExposedProps = false;
            const QList<AbstractProperty> props = node.properties();
            PropertyMetaInfos metaInfos = node.metaInfo().properties();
            for (const PropertyMetaInfo &mi : metaInfos) {
                if (mi.isValid() && !mi.isPrivate() && mi.isWritable()) {
                    BakeData propData;
                    propData.id = data.id;
                    propData.aliasProp = mi.name();
                    if (mi.propertyType().isQtQuick3DModel()) {
                        hasExposedProps = true;
                        propData.isModel = true;
                        PropertyName dotName = mi.name() + '.';
                        for (const AbstractProperty &prop : props) {
                            if (prop.name().startsWith(dotName)) {
                                PropertyName subName = prop.name().mid(dotName.size());
                                if (subName == "bakedLightmap") {
                                    ModelNode blm = prop.toBindingProperty().resolveToModelNode();
                                    if (blm.isValid()) {
                                        if (blm.hasVariantProperty("enabled")) {
                                            propData.enabled = blm.variantProperty("enabled").value().toBool();
                                        } else {
                                            propData.enabled = true;
                                            if (blm.hasProperty("enabled"))
                                                forceManualMode = true;
                                        }
                                    } else {
                                        forceManualMode = true;
                                    }
                                }
                                if (subName == "lightmapBaseResolution") {
                                    if (prop.isVariantProperty())
                                        propData.resolution = prop.toVariantProperty().value().toInt();
                                    else
                                        forceManualMode = true;
                                }
                                if (subName == "usedInBakedLighting") {
                                    if (prop.isVariantProperty())
                                        propData.inUse = prop.toVariantProperty().value().toBool();
                                    else
                                        forceManualMode = true;
                                }
                            }
                        }
                        compModelList.append(propData);
                    } else if (mi.propertyType().isQtQuick3DLight()) {
                        hasExposedProps = true;
                        PropertyName dotName = mi.name() + '.';
                        for (const AbstractProperty &prop : props) {
                            if (prop.name().startsWith(dotName)) {
                                PropertyName subName = prop.name().mid(dotName.size());
                                if (subName == "bakeMode") {
                                    if (prop.isVariantProperty()) {
                                        QString bakeModeStr = prop.toVariantProperty().value()
                                                                  .value<QmlDesigner::Enumeration>()
                                                                  .toString();
                                        if (bakeModeStr.startsWith("Light.BakeMode"))
                                            propData.bakeMode = bakeModeStr;
                                        else
                                            forceManualMode = true;
                                    } else {
                                        forceManualMode = true;
                                    }
                                }
                            }
                        }
                        if (propData.bakeMode.isEmpty())
                            propData.bakeMode = "Light.BakeModeDisabled";
                        compLightList.append(propData);
                    }
                }
            }

            if (!hasExposedProps && node.metaInfo().isFileComponent()
                && node.metaInfo().isQtQuick3DNode()) {
                const QString compFile = node.metaInfo().componentFileName();
                const QString projPath = m_view->externalDependencies().currentProjectDirPath();
                if (compFile.startsWith(projPath)) {
                    // Quick and dirty scan of the component source to check if it potentially has
                    // models or lights.
                    QByteArray src = Utils::FilePath::fromString(compFile).fileContents().value();
                    src = src.mid(src.indexOf('{')); // Skip root element
                    if (src.contains("Model {") || src.contains("Light {")) {
                        data.isUnexposed = true;
                        unexposedList.append(data);
                    }
                }
            }
        }
    }

    auto sortList = [](QList<BakeData> &list) {
        std::sort(list.begin(), list.end(), [](const BakeData &a, const BakeData &b) {
            return a.id.compare(b.id) < 0;
        });
    };

    sortList(modelList);
    sortList(lightList);
    sortList(compModelList);
    sortList(compLightList);
    sortList(unexposedList);

    BakeData titleData;
    titleData.isTitle = true;
    titleData.id = tr("Lights");
    m_dataList.append(titleData);
    m_dataList.append(lightList);
    m_dataList.append(compLightList);
    titleData.id = tr("Models");
    m_dataList.append(titleData);
    m_dataList.append(modelList);
    m_dataList.append(compModelList);

    if (!unexposedList.isEmpty()) {
        titleData.id = tr("Components with unexposed models and/or lights");
        m_dataList.append(titleData);
        m_dataList.append(unexposedList);
    }

    endResetModel();

    return !forceManualMode;
}

void BakeLightsDataModel::apply()
{
    if (!m_view || !m_view->model())
        return;

    auto setBakedLightmap = [this](const ModelNode &node, const BakeData &data) {
        ModelNode blmNode;
        PropertyName propName{"bakedLightmap"};
        if (!data.aliasProp.isEmpty())
            propName.prepend(data.aliasProp + '.');
        if (node.hasBindingProperty(propName))
            blmNode = node.bindingProperty(propName).resolveToModelNode();
        if (!blmNode.isValid() && data.enabled) {
            NodeMetaInfo metaInfo = m_view->model()->qtQuick3DBakedLightmapMetaInfo();
            blmNode = m_view->createModelNode("QtQuick3D.BakedLightmap",
                                          metaInfo.majorVersion(),
                                          metaInfo.minorVersion());
            QString idPart;
            if (data.aliasProp.isEmpty())
                idPart = data.id;
            else
                idPart = QStringLiteral("%1_%2").arg(data.id, QString::fromUtf8(data.aliasProp));
            QString newId = m_view->model()->generateNewId(QStringLiteral("blm_%1").arg(idPart));
            blmNode.setIdWithoutRefactoring(newId);
            node.defaultNodeListProperty().reparentHere(blmNode);
            node.bindingProperty(propName).setExpression(newId);
        }
        if (blmNode.isValid()) {
            VariantProperty enabledProp = blmNode.variantProperty("enabled");
            VariantProperty prefixProp = blmNode.variantProperty("loadPrefix");
            VariantProperty keyProp = blmNode.variantProperty("key");
            enabledProp.setValue(data.enabled);
            prefixProp.setValue(commonPrefix());
            keyProp.setValue(blmNode.id());
        }
    };

    auto setVariantProp = [](const ModelNode &node,
                             const PropertyName &propName,
                             const PropertyName &aliasProp,
                             const QVariant &value,
                             const QVariant &defaultValue) {
        PropertyName resolvedName = propName;
        if (!aliasProp.isEmpty())
            resolvedName.prepend(aliasProp + '.');
        if (node.hasVariantProperty(resolvedName) || value != defaultValue)
            node.variantProperty(resolvedName).setValue(value);
    };

    // Commits changes to scene model
    m_view->executeInTransaction(__FUNCTION__, [&]() {
        for (const BakeData &data : std::as_const(m_dataList)) {
            if (data.isTitle || data.isUnexposed)
                continue;
            ModelNode node = m_view->modelNodeForId(data.id);
            if (data.isModel) {
                setBakedLightmap(node, data);
                setVariantProp(node, "lightmapBaseResolution", data.aliasProp, data.resolution, 1024);
                setVariantProp(node, "usedInBakedLighting", data.aliasProp, data.inUse, false);
            } else {
                setVariantProp(node, "bakeMode", data.aliasProp,
                               QVariant::fromValue(QmlDesigner::Enumeration(data.bakeMode)),
                               QVariant::fromValue(QmlDesigner::Enumeration("Light", "BakeModeDisabled")));
            }
        }
    });
}

QString BakeLightsDataModel::commonPrefix()
{
    static QString prefix = "lightmaps";
    return prefix;
}

QDebug operator<<(QDebug debug, const BakeLightsDataModel::BakeData &data)
{
    QDebugStateSaver saver(debug);
    debug.space() << "("
                  << "id:" << data.id
                  << "aliasProp:" << data.aliasProp
                  << "isModel:" << data.isModel
                  << "enabled:" << data.enabled
                  << "inUse:" << data.inUse
                  << "resolution:" << data.resolution
                  << "bakeMode:" << data.bakeMode
                  << "isTitle:" << data.isTitle
                  << ")";
    return debug;
}

} // namespace QmlDesigner
