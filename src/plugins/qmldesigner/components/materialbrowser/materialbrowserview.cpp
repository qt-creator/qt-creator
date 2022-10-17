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

#include "bindingproperty.h"
#include "bundlematerial.h"
#include "bundleimporter.h"
#include "materialbrowserwidget.h"
#include "materialbrowsermodel.h"
#include "materialbrowserbundlemodel.h"
#include "nodeabstractproperty.h"
#include "nodemetainfo.h"
#include "qmlobjectnode.h"
#include "variantproperty.h"

#include <coreplugin/icore.h>
#include <designmodecontext.h>
#include <nodeinstanceview.h>
#include <nodelistproperty.h>
#include <qmldesignerconstants.h>
#include <utils/algorithm.h>

#include <QQuickItem>
#include <QRegularExpression>
#include <QTimer>

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
        m_widget = new MaterialBrowserWidget(this);

        auto matEditorContext = new Internal::MaterialBrowserContext(m_widget.data());
        Core::ICore::addContextObject(matEditorContext);

        MaterialBrowserModel *matBrowserModel = m_widget->materialBrowserModel().data();

        // custom notifications below are sent to the MaterialEditor

        connect(matBrowserModel, &MaterialBrowserModel::selectedIndexChanged, this, [&] (int idx) {
            ModelNode matNode = m_widget->materialBrowserModel()->materialAt(idx);
            emitCustomNotification("selected_material_changed", {matNode}, {});
        });

        connect(matBrowserModel, &MaterialBrowserModel::applyToSelectedTriggered, this,
                [&] (const ModelNode &material, bool add) {
            emitCustomNotification("apply_to_selected_triggered", {material}, {add});
        });

        connect(matBrowserModel, &MaterialBrowserModel::renameMaterialTriggered, this,
                [&] (const ModelNode &material, const QString &newName) {
            emitCustomNotification("rename_material", {material}, {newName});
        });

        connect(matBrowserModel, &MaterialBrowserModel::addNewMaterialTriggered, this, [&] {
            emitCustomNotification("add_new_material");
        });

        connect(matBrowserModel, &MaterialBrowserModel::duplicateMaterialTriggered, this,
                [&] (const ModelNode &material) {
            emitCustomNotification("duplicate_material", {material});
        });

        connect(matBrowserModel, &MaterialBrowserModel::pasteMaterialPropertiesTriggered, this,
                [&] (const ModelNode &material,
                     const QList<QmlDesigner::MaterialBrowserModel::PropertyCopyData> &propDatas,
                     bool all) {
            QmlObjectNode mat(material);
            executeInTransaction(__FUNCTION__, [&] {
                if (all) { // all material properties copied
                    // remove current properties
                    PropertyNameList propNames;
                    if (mat.isInBaseState()) {
                        const QList<AbstractProperty> baseProps = material.properties();
                        for (const auto &baseProp : baseProps) {
                            if (!baseProp.isDynamic())
                                propNames.append(baseProp.name());
                        }
                    } else {
                        QmlPropertyChanges changes = mat.propertyChangeForCurrentState();
                        if (changes.isValid()) {
                            const QList<AbstractProperty> changedProps = changes.targetProperties();
                            for (const auto &changedProp : changedProps) {
                                if (!changedProp.isDynamic())
                                    propNames.append(changedProp.name());
                            }
                        }
                    }
                    for (const PropertyName &propName : qAsConst(propNames)) {
                        if (propName != "objectName")
                            mat.removeProperty(propName);
                    }
                }

                // apply pasted properties
                for (const QmlDesigner::MaterialBrowserModel::PropertyCopyData &propData : propDatas) {
                    if (propData.isValid) {
                        const bool isDynamic = !propData.dynamicTypeName.isEmpty();
                        const bool isBaseState = currentState().isBaseState();
                        const bool hasProperty = mat.hasProperty(propData.name);
                        if (propData.isBinding) {
                            if (isDynamic && (!hasProperty || isBaseState)) {
                                mat.modelNode().bindingProperty(propData.name)
                                        .setDynamicTypeNameAndExpression(
                                            propData.dynamicTypeName, propData.value.toString());
                                continue;
                            }
                            mat.setBindingProperty(propData.name, propData.value.toString());
                        } else {
                            const bool isRecording = mat.timelineIsActive()
                                    && mat.currentTimeline().isRecording();
                            if (isDynamic && (!hasProperty || (isBaseState && !isRecording))) {
                                mat.modelNode().variantProperty(propData.name)
                                        .setDynamicTypeNameAndValue(
                                            propData.dynamicTypeName, propData.value);
                                continue;
                            }
                            mat.setVariantProperty(propData.name, propData.value);
                        }
                    } else {
                        mat.removeProperty(propData.name);
                    }
                }
            });
        });

        connect(m_widget, &MaterialBrowserWidget::bundleMaterialDragStarted, this,
                [&] (QmlDesigner::BundleMaterial *bundleMat) {
            m_draggedBundleMaterial = bundleMat;
        });

        MaterialBrowserBundleModel *matBrowserBundleModel = m_widget->materialBrowserBundleModel().data();

        connect(matBrowserBundleModel, &MaterialBrowserBundleModel::applyToSelectedTriggered, this,
                [&] (BundleMaterial *bundleMat, bool add) {
            if (m_selectedModels.isEmpty())
                return;

            m_bundleMaterialTargets = m_selectedModels;
            m_bundleMaterialAddToSelected = add;

            ModelNode defaultMat = getBundleMaterialDefaultInstance(bundleMat->type());
            if (defaultMat.isValid())
                applyBundleMaterialToDropTarget(defaultMat);
            else
                m_widget->materialBrowserBundleModel()->addToProject(bundleMat);
        });

        connect(matBrowserBundleModel, &MaterialBrowserBundleModel::bundleMaterialImported, this,
                [&] (const QmlDesigner::NodeMetaInfo &metaInfo) {
            applyBundleMaterialToDropTarget({}, metaInfo);
            updateBundleMaterialsImportedState();
        });

        connect(matBrowserBundleModel, &MaterialBrowserBundleModel::bundleMaterialAboutToUnimport, this,
                [&] (const QmlDesigner::TypeName &type) {
            // delete instances of the bundle material that is about to be unimported
            executeInTransaction("MaterialBrowserView::widgetInfo", [&] {
                Utils::reverseForeach(m_widget->materialBrowserModel()->materials(), [&](const ModelNode &mat) {
                    if (mat.isValid() && mat.type() == type)
                        QmlObjectNode(mat).destroy();
                });
            });
        });

        connect(matBrowserBundleModel, &MaterialBrowserBundleModel::bundleMaterialUnimported, this,
                &MaterialBrowserView::updateBundleMaterialsImportedState);
    }

    return createWidgetInfo(m_widget.data(),
                            "MaterialBrowser",
                            WidgetInfo::LeftPane,
                            0,
                            tr("Material Browser"));
}

void MaterialBrowserView::applyBundleMaterialToDropTarget(const ModelNode &bundleMat,
                                                          const NodeMetaInfo &metaInfo)
{
    if (!bundleMat.isValid() && !metaInfo.isValid())
        return;

    ModelNode matLib = materialLibraryNode();
    if (!matLib.isValid())
        return;

    executeInTransaction("MaterialBrowserView::applyBundleMaterialToDropTarget", [&] {
        ModelNode newMatNode;
        if (metaInfo.isValid()) {
            newMatNode = createModelNode(metaInfo.typeName(), metaInfo.majorVersion(),
                                                              metaInfo.minorVersion());
            matLib.defaultNodeListProperty().reparentHere(newMatNode);

            static QRegularExpression rgx("([A-Z])([a-z]*)");
            QString newName = QString::fromLatin1(metaInfo.simplifiedTypeName()).replace(rgx, " \\1\\2").trimmed();
            if (newName.endsWith(" Material"))
                newName.chop(9); // remove trailing " Material"
            QString newId = model()->generateIdFromName(newName, "material");
            newMatNode.setIdWithRefactoring(newId);

            VariantProperty objNameProp = newMatNode.variantProperty("objectName");
            objNameProp.setValue(newName);
        } else {
            newMatNode = bundleMat;
        }

        // TODO: unify this logic as it exist elsewhere also
        auto expToList = [](const QString &exp) {
            QString copy = exp;
            copy = copy.remove("[").remove("]");

            QStringList tmp = copy.split(',', Qt::SkipEmptyParts);
            for (QString &str : tmp)
                str = str.trimmed();

            return tmp;
        };

        auto listToExp = [](QStringList &stringList) {
            if (stringList.size() > 1)
                return QString("[" + stringList.join(",") + "]");

            if (stringList.size() == 1)
                return stringList.first();

            return QString();
        };

        for (const ModelNode &target : std::as_const(m_bundleMaterialTargets)) {
            if (target.isValid() && target.isSubclassOf("QtQuick3D.Model")) {
                QmlObjectNode qmlObjNode(target);
                if (m_bundleMaterialAddToSelected) {
                    QStringList matList = expToList(qmlObjNode.expression("materials"));
                    matList.append(newMatNode.id());
                    QString updatedExp = listToExp(matList);
                    qmlObjNode.setBindingProperty("materials", updatedExp);
                } else {
                    qmlObjNode.setBindingProperty("materials", newMatNode.id());
                }
            }

            m_bundleMaterialTargets = {};
            m_bundleMaterialAddToSelected = false;
        }
    });
}

void MaterialBrowserView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    m_widget->clearSearchFilter();
    m_widget->materialBrowserModel()->setHasMaterialRoot(rootModelNode().isSubclassOf("QtQuick3D.Material"));
    m_hasQuick3DImport = model->hasImport("QtQuick3D");

    updateBundleMaterialsImportedState();

    // Project load is already very busy and may even trigger puppet reset, so let's wait a moment
    // before refreshing the model
    QTimer::singleShot(1000, this, [this]() {
        refreshModel(true);
        loadPropertyGroups(); // Needs the delay because it uses metaInfo
    });
}

void MaterialBrowserView::refreshModel(bool updateImages)
{
    if (!model() || !model()->nodeInstanceView())
        return;

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

    if (updateImages) {
        for (const ModelNode &node : std::as_const(materials))
            model()->nodeInstanceView()->previewImageDataForGenericNode(node, {});
    }
}

bool MaterialBrowserView::isMaterial(const ModelNode &node) const
{
    if (!node.isValid())
        return false;

    return node.isSubclassOf("QtQuick3D.Material");
}

void MaterialBrowserView::modelAboutToBeDetached(Model *model)
{
    m_widget->materialBrowserModel()->setMaterials({}, m_hasQuick3DImport);

    AbstractView::modelAboutToBeDetached(model);
}

void MaterialBrowserView::selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                               const QList<ModelNode> &lastSelectedNodeList)
{
    Q_UNUSED(lastSelectedNodeList)

    m_selectedModels = Utils::filtered(selectedNodeList, [](const ModelNode &node) {
        return node.isSubclassOf("QtQuick3D.Model");
    });

    m_widget->materialBrowserModel()->setHasModelSelection(!m_selectedModels.isEmpty());

    // the logic below selects the material of the first selected model if auto selection is on
    if (!m_autoSelectModelMaterial)
        return;

    if (selectedNodeList.size() > 1 || m_selectedModels.isEmpty())
        return;

    QmlObjectNode qmlObjNode(m_selectedModels.at(0));
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
    Q_UNUSED(propertyChange)

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
    Q_UNUSED(propertyChange)

    if (!isMaterial(node))
        return;

    ModelNode newParentNode = newPropertyParent.parentModelNode();
    ModelNode oldParentNode = oldPropertyParent.parentModelNode();
    bool matAdded = newParentNode.isValid() && newParentNode.id() == Constants::MATERIAL_LIB_ID;
    bool matRemoved = oldParentNode.isValid() && oldParentNode.id() == Constants::MATERIAL_LIB_ID;

    if (matAdded || matRemoved) {
        if (matAdded && !m_puppetResetPending) {
            // Workaround to fix various material issues all likely caused by QTBUG-103316
            resetPuppet();
            m_puppetResetPending = true;
        }
        refreshModel(!matAdded);
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
    Q_UNUSED(removedNode)
    Q_UNUSED(propertyChange)

    if (parentProperty.parentModelNode().id() != Constants::MATERIAL_LIB_ID)
        return;

    m_widget->materialBrowserModel()->updateSelectedMaterial();
}

void QmlDesigner::MaterialBrowserView::loadPropertyGroups()
{
    if (!m_hasQuick3DImport || m_propertyGroupsLoaded)
        return;

    QString matPropsPath = model()->metaInfo("QtQuick3D.Material").importDirectoryPath()
                               + "/designer/propertyGroups.json";
    m_propertyGroupsLoaded = m_widget->materialBrowserModel()->loadPropertyGroups(matPropsPath);
}

void MaterialBrowserView::updateBundleMaterialsImportedState()
{
    using namespace Utils;

    if (!m_widget->materialBrowserBundleModel()->bundleImporter())
        return;

    QStringList importedBundleMats;

    FilePath materialBundlePath = m_widget->materialBrowserBundleModel()->bundleImporter()->resolveBundleImportPath();

    if (materialBundlePath.exists()) {
        importedBundleMats = transform(materialBundlePath.dirEntries({{"*.qml"}, QDir::Files}),
                                       [](const FilePath &f) { return f.fileName().chopped(4); });
    }

    m_widget->materialBrowserBundleModel()->updateImportedState(importedBundleMats);
}

ModelNode MaterialBrowserView::getBundleMaterialDefaultInstance(const TypeName &type)
{
    const QList<ModelNode> materials = m_widget->materialBrowserModel()->materials();
    for (const ModelNode &mat : materials) {
        if (mat.type() == type) {
            bool isDefault = true;
            const QList<AbstractProperty> props = mat.properties();
            for (const AbstractProperty &prop : props) {
                if (prop.name() != "objectName") {
                    isDefault = false;
                    break;
                }
            }

            if (isDefault)
                return mat;
        }
    }

    return {};
}

void MaterialBrowserView::importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports)
{
    Q_UNUSED(addedImports)
    Q_UNUSED(removedImports)

    bool hasQuick3DImport = model()->hasImport("QtQuick3D");

    if (hasQuick3DImport == m_hasQuick3DImport)
        return;

    m_hasQuick3DImport = hasQuick3DImport;

    loadPropertyGroups();

    // Import change will trigger puppet reset, so we don't want to update previews immediately
    refreshModel(false);
}

void MaterialBrowserView::customNotification(const AbstractView *view, const QString &identifier,
                                             const QList<ModelNode> &nodeList, const QList<QVariant> &data)
{
    Q_UNUSED(data)

    if (view == this)
        return;

    if (identifier == "selected_material_changed") {
        int idx = m_widget->materialBrowserModel()->materialIndex(nodeList.first());
        if (idx != -1)
            m_widget->materialBrowserModel()->selectMaterial(idx);
    } else if (identifier == "refresh_material_browser") {
        QTimer::singleShot(0, this, [this]() {
            refreshModel(true);
        });
    } else if (identifier == "delete_selected_material") {
        m_widget->materialBrowserModel()->deleteSelectedMaterial();
    } else if (identifier == "drop_bundle_material") {
        m_bundleMaterialTargets = nodeList;

        ModelNode defaultMat = getBundleMaterialDefaultInstance(m_draggedBundleMaterial->type());
        if (defaultMat.isValid())
            applyBundleMaterialToDropTarget(defaultMat);
        else
            m_widget->materialBrowserBundleModel()->addToProject(m_draggedBundleMaterial);

        m_draggedBundleMaterial = nullptr;
    }
}

void MaterialBrowserView::instancesCompleted(const QVector<ModelNode> &completedNodeList)
{
    for (const ModelNode &node : completedNodeList) {
        // We use root node completion as indication of puppet reset
        if (node.isRootNode()) {
            m_puppetResetPending  = false;
            QTimer::singleShot(1000, this, [this]() {
                if (!model() || !model()->nodeInstanceView())
                    return;
                const QList<ModelNode> materials = m_widget->materialBrowserModel()->materials();
                for (const ModelNode &node : materials)
                    model()->nodeInstanceView()->previewImageDataForGenericNode(node, {});
            });
            break;
        }
    }
}

} // namespace QmlDesigner
