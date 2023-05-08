// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "bakelightsdatamodel.h"

#include "abstractview.h"
#include "bakelights.h"
#include "bindingproperty.h"
#include "enumeration.h"
#include "model.h"
#include "modelnode.h"
#include "nodelistproperty.h"
#include "nodemetainfo.h"
#include "variantproperty.h"

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
    return m_dataList.count();
}

QHash<int, QByteArray> BakeLightsDataModel::roleNames() const
{
    static const QHash<int, QByteArray> roles {
        {Qt::UserRole + 1, "nodeId"},
        {Qt::UserRole + 2, "isModel"},
        {Qt::UserRole + 3, "isEnabled"},
        {Qt::UserRole + 4, "inUse"},
        {Qt::UserRole + 5, "isTitle"},
        {Qt::UserRole + 6, "resolution"},
        {Qt::UserRole + 7, "bakeMode"}
    };
    return roles;
}

QVariant BakeLightsDataModel::data(const QModelIndex &index, int role) const
{
    QTC_ASSERT(index.isValid() && index.row() < m_dataList.count(), return {});
    QTC_ASSERT(roleNames().contains(role), return {});

    QByteArray roleName = roleNames().value(role);
    const BakeData &bakeData = m_dataList[index.row()];

    if (roleName == "nodeId") {
        const QString id = bakeData.id;
        const PropertyName aliasProp = bakeData.aliasProp;
        if (aliasProp.isEmpty())
            return id;
        else
            return QVariant{id + " - " + QString::fromUtf8(aliasProp)};
        return {};
    }

    if (roleName == "isModel")
        return bakeData.isModel;

    if (roleName == "isEnabled")
        return bakeData.enabled;

    if (roleName == "inUse")
        return bakeData.inUse;

    if (roleName == "isTitle")
        return bakeData.isTitle;

    if (roleName == "resolution")
        return bakeData.resolution;

    if (roleName == "bakeMode")
        return bakeData.bakeMode;

    return {};
}

bool BakeLightsDataModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    QTC_ASSERT(index.isValid() && index.row() < m_dataList.count(), return false);
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

void BakeLightsDataModel::reset()
{
    if (!m_view || !m_view->model())
        return;

    beginResetModel();
    m_dataList.clear();

    ModelNode view3dNode = BakeLights::resolveView3dNode(m_view);

    // Find all models and lights in active View3D
    QList<ModelNode> nodes = view3dNode.allSubModelNodes();

    if (view3dNode.hasBindingProperty("importScene"))
        nodes.append(view3dNode.bindingProperty("importScene").resolveToModelNode().allSubModelNodesAndThisNode());

    QList<BakeData> modelList;
    QList<BakeData> lightList;
    QList<BakeData> compModelList;
    QList<BakeData> compLightList;

    // Note: We are always loading base state values for baking. If users want to bake
    // differently for different states, they need to setup things manually for now.
    // Same goes if they want to use any unusual bindings in baking properties.
    for (const auto &node : std::as_const(nodes)) {
        BakeData data;
        data.id = node.id();
        if (data.id.isEmpty())
            continue; // Skip nodes without id

        if (node.metaInfo().isQtQuick3DModel()) {
            data.isModel = true;
            if (node.hasBindingProperty("bakedLightmap")) {
                ModelNode blm = node.bindingProperty("bakedLightmap").resolveToModelNode();
                if (blm.isValid()) {
                    if (blm.hasVariantProperty("enabled"))
                        data.enabled = blm.variantProperty("enabled").value().toBool();
                    else
                        data.enabled = true;
                }
            }
            if (node.hasVariantProperty("lightmapBaseResolution"))
                data.resolution = node.variantProperty("lightmapBaseResolution").value().toInt();
            if (node.hasVariantProperty("usedInBakedLighting"))
                data.inUse = node.variantProperty("usedInBakedLighting").value().toBool();
            modelList.append(data);
        } else if (node.metaInfo().isQtQuick3DLight()) {
            if (node.hasVariantProperty("bakeMode")) {
                data.bakeMode = node.variantProperty("bakeMode").value()
                                    .value<QmlDesigner::Enumeration>().toString();
            } else {
                data.bakeMode = "Light.BakeModeDisabled";
            }
            lightList.append(data);
        }

        if (node.isComponent()) {
            // Every component can expose multiple aliases
            // We ignore baking properties defined inside the component (no visibility there)
            const QList<AbstractProperty> props = node.properties();
            PropertyMetaInfos metaInfos = node.metaInfo().properties();
            for (const PropertyMetaInfo &mi : metaInfos) {
                if (mi.isValid() && !mi.isPrivate() && mi.isWritable()) {
                    BakeData propData;
                    propData.id = data.id;
                    propData.aliasProp = mi.name();
                    if (mi.propertyType().isQtQuick3DModel()) {
                        propData.isModel = true;
                        PropertyName dotName = mi.name() + '.';
                        for (const AbstractProperty &prop : props) {
                            if (prop.name().startsWith(dotName)) {
                                PropertyName subName = prop.name().mid(dotName.size());
                                if (subName == "bakedLightmap") {
                                    ModelNode blm = prop.toBindingProperty().resolveToModelNode();
                                    if (blm.isValid()) {
                                        if (blm.hasVariantProperty("enabled"))
                                            propData.enabled = blm.variantProperty("enabled").value().toBool();
                                        else
                                            propData.enabled = true;
                                    }
                                }
                                if (subName == "lightmapBaseResolution")
                                    propData.resolution = prop.toVariantProperty().value().toInt();
                                if (subName == "usedInBakedLighting")
                                    propData.inUse = prop.toVariantProperty().value().toBool();
                            }
                        }
                        compModelList.append(propData);
                    } else if (mi.propertyType().isQtQuick3DLight()) {
                        PropertyName dotName = mi.name() + '.';
                        for (const AbstractProperty &prop : props) {
                            if (prop.name().startsWith(dotName)) {
                                PropertyName subName = prop.name().mid(dotName.size());
                                if (subName == "bakeMode") {
                                    propData.bakeMode = prop.toVariantProperty().value()
                                                            .value<QmlDesigner::Enumeration>()
                                                            .toString();
                                }
                            }
                        }
                        if (propData.bakeMode.isEmpty())
                            propData.bakeMode = "Light.BakeModeDisabled";
                        compLightList.append(propData);
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

    endResetModel();
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

    auto setResolution = [setVariantProp](const ModelNode &node, const BakeData &data) {
        setVariantProp(node, "lightmapBaseResolution", data.aliasProp, data.resolution, 1024);
    };

    auto setInUse = [setVariantProp](const ModelNode &node, const BakeData &data) {
        setVariantProp(node, "usedInBakedLighting", data.aliasProp, data.inUse, false);
    };

    auto setBakeMode = [setVariantProp](const ModelNode &node, const BakeData &data) {
        setVariantProp(node, "bakeMode", data.aliasProp,
                       QVariant::fromValue(QmlDesigner::Enumeration(data.bakeMode)),
                       QVariant::fromValue(QmlDesigner::Enumeration("Light", "BakeModeDisabled")));
    };

    // Commits changes to scene model
    m_view->executeInTransaction(__FUNCTION__, [&]() {
        for (const BakeData &data : std::as_const(m_dataList)) {
            if (data.isTitle)
                continue;
            ModelNode node = m_view->modelNodeForId(data.id);
            if (data.isModel) {
                setBakedLightmap(node, data);
                setResolution(node, data);
                setInUse(node, data);
            } else {
                setBakeMode(node, data);
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
