/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "materialbrowserview.h"
#include "materialbrowserwidget.h"
#include "materialbrowsermodel.h"
#include "nodeabstractproperty.h"
#include "qmlobjectnode.h"
#include "variantproperty.h"
#include <coreplugin/icore.h>
#include <nodeinstanceview.h>
#include <qmldesignerconstants.h>

#include <QQuickItem>

namespace QmlDesigner {

MaterialBrowserView::MaterialBrowserView(QObject *parent)
    : AbstractView(parent)

{}

MaterialBrowserView::~MaterialBrowserView()
{}

bool MaterialBrowserView::hasWidget() const
{
    return true;
}

WidgetInfo MaterialBrowserView::widgetInfo()
{
    if (m_widget.isNull()) {
        m_widget = new MaterialBrowserWidget;
        connect(m_widget->materialBrowserModel().data(), SIGNAL(selectedIndexChanged(int)),
                this, SLOT(handleSelectedMaterialChanged(int)));
        connect(m_widget->materialBrowserModel().data(),
                SIGNAL(applyToSelectedTriggered(const QmlDesigner::ModelNode &, bool)),
                this, SLOT(handleApplyToSelectedTriggered(const QmlDesigner::ModelNode &, bool)));
        connect(m_widget->materialBrowserModel().data(),
                SIGNAL(renameMaterialTriggered(const QmlDesigner::ModelNode &, const QString &)),
                this, SLOT(handleRenameMaterial(const QmlDesigner::ModelNode &, const QString &)));
        connect(m_widget->materialBrowserModel().data(), SIGNAL(addNewMaterialTriggered()),
                this, SLOT(handleAddNewMaterial()));
    }

    return createWidgetInfo(m_widget.data(),
                            new WidgetInfo::ToolBarWidgetDefaultFactory<MaterialBrowserWidget>(m_widget.data()),
                            "MaterialBrowser",
                            WidgetInfo::LeftPane,
                            0,
                            tr("Material Browser"));
}

void MaterialBrowserView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    m_widget->clearSearchFilter();
    m_hasQuick3DImport = model->hasImport("QtQuick3D");
    QTimer::singleShot(0, this, &MaterialBrowserView::refreshModel);
}

void MaterialBrowserView::refreshModel()
{
    ModelNode matLib = modelNodeForId(Constants::MATERIAL_LIB_ID);
    QList <ModelNode> materials;

    if (m_hasQuick3DImport && matLib.isValid()) {
        const QList <ModelNode> matLibNodes = matLib.directSubModelNodes();
        for (const ModelNode &node : matLibNodes) {
            if (isMaterial(node))
                materials.append(node);
        }
    }

    m_widget->materialBrowserModel()->setMaterials(materials, m_hasQuick3DImport);

    for (const ModelNode &node : std::as_const(materials))
        model()->nodeInstanceView()->previewImageDataForGenericNode(node, {});
}

bool MaterialBrowserView::isMaterial(const ModelNode &node) const
{
    if (!node.isValid() || node.isComponent())
        return false;

    return node.isSubclassOf("QtQuick3D.Material");
}

void MaterialBrowserView::modelAboutToBeDetached(Model *model)
{
    AbstractView::modelAboutToBeDetached(model);
}

void MaterialBrowserView::selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                               const QList<ModelNode> &lastSelectedNodeList)
{
    ModelNode selectedModel;

    for (const ModelNode &node : selectedNodeList) {
        if (node.isSubclassOf("QtQuick3D.Model")) {
            selectedModel = node;
            break;
        }
    }

    m_widget->materialBrowserModel()->setHasModelSelection(selectedModel.isValid());

    if (!m_autoSelectModelMaterial)
        return;

    if (selectedNodeList.size() > 1 || !selectedModel.isValid())
        return;

    QmlObjectNode qmlObjNode(selectedModel);
    QString matExp = qmlObjNode.expression("materials");
    if (matExp.isEmpty())
        return;

    QString matId = matExp.remove('[').remove(']').split(',', Qt::SkipEmptyParts).at(0);
    ModelNode mat = modelNodeForId(matId);
    if (!mat.isValid())
        return;

    // if selected object is a model, select its material in the material browser and editor
    int idx = m_widget->materialBrowserModel()->materialIndex(mat);
    m_widget->materialBrowserModel()->selectMaterial(idx);
}

void MaterialBrowserView::modelNodePreviewPixmapChanged(const ModelNode &node, const QPixmap &pixmap)
{
    if (isMaterial(node))
        m_widget->updateMaterialPreview(node, pixmap);
}

void MaterialBrowserView::variantPropertiesChanged(const QList<VariantProperty> &propertyList,
                                                   PropertyChangeFlags propertyChange)
{
    for (const VariantProperty &property : propertyList) {
        ModelNode node(property.parentModelNode());

        if (isMaterial(node) && property.name() == "objectName")
            m_widget->materialBrowserModel()->updateMaterialName(node);
    }
}

void MaterialBrowserView::nodeReparented(const ModelNode &node,
                                         const NodeAbstractProperty &newPropertyParent,
                                         const NodeAbstractProperty &oldPropertyParent,
                                         PropertyChangeFlags propertyChange)
{
    if (!isMaterial(node))
        return;

    ModelNode newParentNode = newPropertyParent.parentModelNode();
    ModelNode oldParentNode = oldPropertyParent.parentModelNode();
    bool matAdded = newParentNode.isValid() && newParentNode.id() == Constants::MATERIAL_LIB_ID;
    bool matRemoved = oldParentNode.isValid() && oldParentNode.id() == Constants::MATERIAL_LIB_ID;

    if (matAdded || matRemoved) {
        refreshModel();

        int idx = m_widget->materialBrowserModel()->materialIndex(node);
        m_widget->materialBrowserModel()->selectMaterial(idx);
    }
}

void MaterialBrowserView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    // removing the material editor node
    if (removedNode.isValid() && removedNode.id() == Constants::MATERIAL_LIB_ID) {
        m_widget->materialBrowserModel()->setMaterials({}, m_hasQuick3DImport);
        return;
    }

    // not a material under the material editor
    if (!isMaterial(removedNode)
        || removedNode.parentProperty().parentModelNode().id() != Constants::MATERIAL_LIB_ID) {
        return;
    }

    m_widget->materialBrowserModel()->removeMaterial(removedNode);
}

void MaterialBrowserView::nodeRemoved(const ModelNode &removedNode,
                                      const NodeAbstractProperty &parentProperty,
                                      PropertyChangeFlags propertyChange)
{
    if (parentProperty.parentModelNode().id() != Constants::MATERIAL_LIB_ID)
        return;

    m_widget->materialBrowserModel()->updateSelectedMaterial();
}

void MaterialBrowserView::importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports)
{
    bool hasQuick3DImport = model()->hasImport("QtQuick3D");

    if (hasQuick3DImport == m_hasQuick3DImport)
        return;

    m_hasQuick3DImport = hasQuick3DImport;
    refreshModel();

}

void MaterialBrowserView::customNotification(const AbstractView *view, const QString &identifier,
                                             const QList<ModelNode> &nodeList, const QList<QVariant> &data)
{
    if (view == this)
        return;

    if (identifier == "selected_material_changed") {
        int idx = m_widget->materialBrowserModel()->materialIndex(nodeList.first());
        if (idx != -1)
            m_widget->materialBrowserModel()->selectMaterial(idx);
    }
}

void MaterialBrowserView::handleSelectedMaterialChanged(int idx)
{
    ModelNode matNode = m_widget->materialBrowserModel()->materialAt(idx);
    // to MaterialEditor...
    emitCustomNotification("selected_material_changed", {matNode}, {});
}

void MaterialBrowserView::handleApplyToSelectedTriggered(const ModelNode &material, bool add)
{
    // to MaterialEditor...
    emitCustomNotification("apply_to_selected_triggered", {material}, {add});
}

void MaterialBrowserView::handleRenameMaterial(const ModelNode &material, const QString &newName)
{
    // to MaterialEditor...
    emitCustomNotification("rename_material", {material}, {newName});
}

void MaterialBrowserView::handleAddNewMaterial()
{
    // to MaterialEditor...
    emitCustomNotification("add_new_material");
}

} // namespace QmlDesigner
