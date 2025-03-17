// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "edit3dmaterialsaction.h"

#include <bindingproperty.h>
#include <designmodewidget.h>
#include <model.h>
#include <modelnode.h>
#include <modelutils.h>
#include <qmldesignerplugin.h>
#include <qmleditormenu.h>
#include <utils3d.h>
#include <variantproperty.h>

#include <utils/qtcassert.h>

using namespace Qt::StringLiterals;

namespace QmlDesigner {

static QString getMaterialName(const ModelNode &material, bool forceIncludeId = false)
{
    QString materialName = material.variantProperty("objectName").value().toString();
    if (materialName.isEmpty() || forceIncludeId)
        materialName.append(QString("[%1]").arg(material.id()));
    return materialName;
}

struct MaterialNameLessThan
{
    bool operator()(const ModelNode &a, const ModelNode &b)
    {
        const QString aName = getMaterialName(a, true);
        const QString bName = getMaterialName(b, true);
        return aName.compare(bName, Qt::CaseSensitive) < 0;
    };
};

static QList<ModelNode> getMaterials(const ModelNode &node)
{
    BindingProperty matsProp = node.bindingProperty("materials");
    if (!matsProp.exists())
        return {};

    Model *model = node.model();
    QList<ModelNode> materials;
    if (model->hasId(matsProp.expression()))
        materials.append(model->modelNodeForId(matsProp.expression()));
    else
        materials = matsProp.resolveToModelNodeList();

    return materials;
}

static QList<ModelNode> getSortedMaterials(const ModelNode &node)
{
    QList<ModelNode> materials = getMaterials(node);
    std::sort(materials.begin(), materials.end(), MaterialNameLessThan{});
    return materials;
}

static void removeMaterialFromNode(const ModelNode &node, const QString &materialId, int nthMaterial)
{
    BindingProperty matsProp = node.bindingProperty("materials");
    if (!matsProp.exists())
        return;

    const QString materialsExpression = matsProp.expression();
    Model *model = node.model();

    if (matsProp.isList()) {
        matsProp.removeModelNodeFromArray(model->modelNodeForId(materialId));
        QStringList nodeMaterials = ModelUtils::expressionToList(materialsExpression);

        int indexToBeRemoved = -1;
        do
            indexToBeRemoved = nodeMaterials.indexOf(materialId, indexToBeRemoved + 1);
        while (nthMaterial-- && indexToBeRemoved != -1);

        if (indexToBeRemoved != -1)
            nodeMaterials.removeAt(indexToBeRemoved);

        if (nodeMaterials.isEmpty())
            matsProp.parentModelNode().removeProperty(matsProp.name());
        else if (nodeMaterials.size() == 1)
            matsProp.setExpression(nodeMaterials.first());
        else
            matsProp.setExpression('[' + nodeMaterials.join(',') + ']');
    } else if (materialsExpression == materialId) {
        matsProp.parentModelNode().removeProperty(matsProp.name());
    }
}

static QList<ModelNode> commonMaterialsOfNodes(const QList<ModelNode> &selectedNodes)
{
    if (selectedNodes.isEmpty())
        return {};

    if (selectedNodes.size() == 1)
        return getMaterials(selectedNodes.first());

    QList<ModelNode> commonMaterials = getSortedMaterials(selectedNodes.first());
    for (const ModelNode &node : Utils::span(selectedNodes).subspan(1)) {
        const QList<ModelNode> materials = getSortedMaterials(node);
        QList<ModelNode> materialIntersection;
        std::set_intersection(commonMaterials.begin(),
                              commonMaterials.end(),
                              materials.begin(),
                              materials.end(),
                              std::back_inserter(materialIntersection),
                              MaterialNameLessThan{});
        std::swap(commonMaterials, materialIntersection);
        if (commonMaterials.isEmpty())
            return {};
    }
    return commonMaterials;
}

Edit3DMaterialsAction::Edit3DMaterialsAction(const QIcon &icon, QObject *parent)
    : QAction(icon, tr("Materials"), parent)
{
    this->setMenu(new QmlEditorMenu("Materials"));
    connect(this, &QObject::destroyed, this->menu(), &QObject::deleteLater);
}

void Edit3DMaterialsAction::updateMenu(const QList<ModelNode> &selecedNodes)
{
    QMenu *menu = this->menu();
    QTC_ASSERT(menu, return);

    m_selectedNodes = selecedNodes;

    menu->clear();
    const QList<ModelNode> materials = commonMaterialsOfNodes(m_selectedNodes);
    QHash<ModelNode, int> nthMaterialMap; // <material, n times repeated>

    for (const ModelNode &material : materials) {
        int nthMaterialWithTheSameId = nthMaterialMap.value(material, -1) + 1;
        nthMaterialMap.insert(material, nthMaterialWithTheSameId);
        QAction *materialAction = createMaterialAction(material, menu, nthMaterialWithTheSameId);
        if (materialAction)
            menu->addAction(materialAction);
    }

    setVisible(!menu->actions().isEmpty());
    setEnabled(isVisible());
}

void Edit3DMaterialsAction::removeMaterial(const QString &materialId, int nthMaterial)
{
    if (m_selectedNodes.isEmpty())
        return;

    AbstractView *nodesView = m_selectedNodes.first().view();
    nodesView->executeInTransaction(__FUNCTION__, [&] {
        for (ModelNode &node : m_selectedNodes)
            removeMaterialFromNode(node, materialId, nthMaterial);
    });
}

QAction *Edit3DMaterialsAction::createMaterialAction(const ModelNode &material,
                                                     QMenu *parentMenu,
                                                     int nthMaterial)
{
    const QString materialId = material.id();
    if (materialId.isEmpty())
        return nullptr;

    QString materialName = getMaterialName(material);

    QAction *action = new QAction(materialName, parentMenu);
    QMenu *menu = new QmlEditorMenu(materialName, parentMenu);
    connect(action, &QObject::destroyed, menu, &QObject::deleteLater);

    QAction *removeMaterialAction = new QAction(tr("Remove"), menu);
    connect(removeMaterialAction,
            &QAction::triggered,
            menu,
            std::bind(&Edit3DMaterialsAction::removeMaterial, this, materialId, nthMaterial));

    QAction *editMaterialAction = new QAction(tr("Edit"), menu);
    connect(editMaterialAction, &QAction::triggered, menu, [material] {
        Utils3D::openNodeInPropertyEditor(material);
    });

    menu->addAction(editMaterialAction);
    menu->addAction(removeMaterialAction);
    action->setMenu(menu);

    return action;
}

}; // namespace QmlDesigner
