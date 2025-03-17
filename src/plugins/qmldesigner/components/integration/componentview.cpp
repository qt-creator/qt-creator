// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "componentview.h"
#include "componentaction.h"

#include <designeractionmanager.h>
#include <import3ddialog.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <qmldesignerplugin.h>
#include <utils3d.h>

#include <coreplugin/icore.h>

#include <QDebug>
#include <QStandardItemModel>

// silence gcc warnings about unused parameters

namespace QmlDesigner {

ComponentView::ComponentView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
    , m_standardItemModel(new QStandardItemModel(this))
    , m_componentAction(new ComponentAction(this))
{
    connect(&m_ensureMatLibTimer, &QTimer::timeout, this, &ComponentView::ensureMatLibTriggered);
    m_ensureMatLibTimer.setInterval(500);
}

void ComponentView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    removeFromListRecursive(removedNode);
}

QStandardItemModel *ComponentView::standardItemModel() const
{
    return m_standardItemModel;
}

ModelNode ComponentView::modelNode(int index) const
{
    if (m_standardItemModel->hasIndex(index, 0)) {
        QStandardItem *item = m_standardItemModel->item(index, 0);
        return modelNodeForInternalId(qint32(item->data(ModelNodeRole).toInt()));
    }

    return ModelNode();
}

void ComponentView::setComponentNode(const ModelNode &node)
{
    m_componentAction->setCurrentIndex(indexForNode(node));
}

void ComponentView::setComponentToMaster()
{
    m_componentAction->setCurrentIndex(indexOfMaster());
}

void ComponentView::removeNodeFromList(const ModelNode &node)
{
    for (int row = 0; row < m_standardItemModel->rowCount(); row++) {
        if (m_standardItemModel->item(row)->data(ModelNodeRole).toInt() == node.internalId())
            m_standardItemModel->removeRow(row);
    }
}

void ComponentView::addNodeToList(const ModelNode &node)
{
    if (hasEntryForNode(node))
        return;

    QString description = descriptionForNode(node);
    auto item = new QStandardItem(description);
    item->setData(QVariant::fromValue(node.internalId()), ModelNodeRole);
    item->setEditable(false);
    m_standardItemModel->appendRow(item);
}

int ComponentView::indexForNode(const ModelNode &node) const
{
    for (int row = 0; row < m_standardItemModel->rowCount(); row++) {
        if (m_standardItemModel->item(row)->data(ModelNodeRole).toInt() == node.internalId())
            return row;
    }
    return -1;
}

int ComponentView::indexOfMaster() const
{
    for (int row = 0; row < m_standardItemModel->rowCount(); row++) {
        if (m_standardItemModel->item(row)->data(ModelNodeRole).toInt() == 0)
            return row;
    }

    return -1;
}

bool ComponentView::hasMasterEntry() const
{
    return indexOfMaster() >= 0;
}

bool ComponentView::hasEntryForNode(const ModelNode &node) const
{
    return indexForNode(node) >= 0;
}

void ComponentView::ensureMasterDocument()
{
    if (!hasMasterEntry()) {
        QStandardItem *item = new QStandardItem(QLatin1String("master"));
        item->setData(QVariant::fromValue(0), ModelNodeRole);
        item->setEditable(false);
        m_standardItemModel->appendRow(item);
    }
}

void ComponentView::maybeRemoveMasterDocument()
{
    int idx = indexOfMaster();
    if (idx >= 0 && m_standardItemModel->rowCount() == 1)
        m_standardItemModel->removeRow(idx);
}

QString ComponentView::descriptionForNode(const ModelNode &node) const
{
    QString description;

    if (!node.id().isEmpty()) {
        description = node.id();
    } else if (node.hasParentProperty()) {
        ModelNode parentNode = node.parentProperty().parentModelNode();

        if (parentNode.id().isEmpty())
            description = parentNode.simplifiedTypeName() + ' ';
        else
            description = parentNode.id() + ' ';

        description += QString::fromUtf8(node.parentProperty().name());
    }

    return description;
}

void ComponentView::updateDescription(const ModelNode &node)
{
    int nodeIndex = indexForNode(node);

    if (nodeIndex > -1)
        m_standardItemModel->item(nodeIndex)->setText(descriptionForNode(node));
}

bool ComponentView::isSubComponentNode(const ModelNode &node) const
{
    return node.nodeSourceType() == ModelNode::NodeWithComponentSource
            || (node.hasParentProperty()
                && !node.parentProperty().isDefaultProperty()
                && node.metaInfo().isValid()
                && node.metaInfo().isGraphicalItem());
}

void ComponentView::ensureMatLibTriggered()
{
    if (!model() || !model()->rewriterView()
        || model()->rewriterView()->hasIncompleteTypeInformation()
        || !model()->rewriterView()->errors().isEmpty()) {
        return;
    }

    m_ensureMatLibTimer.stop();
    ModelNode matLib = Utils3D::materialLibraryNode(this);
    if (matLib.isValid())
        return;

    DesignDocument *doc = QmlDesignerPlugin::instance()->currentDesignDocument();
    if (doc && !doc->inFileComponentModelActive())
        Utils3D::ensureMaterialLibraryNode(this);

    matLib = Utils3D::materialLibraryNode(this);
    if (!matLib.isValid())
        return;

    bool texSelected = Utils3D::selectedTexture(this).isValid();
    if (!texSelected) {
        const QList<ModelNode> matLibNodes = matLib.directSubModelNodes();
        for (const ModelNode &node : matLibNodes) {
            if (node.metaInfo().isQtQuick3DTexture()) {
                Utils3D::selectTexture(node);
                break;
            }
        }
    }
}

void ComponentView::modelAttached(Model *model)
{
    if (AbstractView::model() == model)
        return;

    QSignalBlocker blocker(m_componentAction);
    m_standardItemModel->clear();

    AbstractView::modelAttached(model);

    searchForComponentAndAddToList(rootModelNode());

    if (model->hasImport("QtQuick3D")) {
        // Creating the material library node on model attach causes errors as long as the type
        // information is not complete yet, so we keep checking until type info is complete.
        m_ensureMatLibTimer.start();
    }
}

void ComponentView::modelAboutToBeDetached(Model *model)
{
    QSignalBlocker blocker(m_componentAction);
    m_standardItemModel->clear();
    m_ensureMatLibTimer.stop();
    AbstractView::modelAboutToBeDetached(model);
}

ComponentAction *ComponentView::action()
{
    return m_componentAction;
}

void ComponentView::nodeCreated(const ModelNode &createdNode)
{
    searchForComponentAndAddToList(createdNode);
}

void ComponentView::searchForComponentAndAddToList(const ModelNode &node)
{
    const auto nodeList = node.allSubModelNodesAndThisNode();
    bool hasMaster = false;
    for (const ModelNode &childNode : nodeList) {
        if (isSubComponentNode(childNode)) {
            if (!hasMaster) {
                hasMaster = true;
                ensureMasterDocument();
            }
            addNodeToList(childNode);
        }
    }
}

void ComponentView::removeFromListRecursive(const ModelNode &node)
{
    const auto nodeList = node.allSubModelNodesAndThisNode();
    for (const ModelNode &childNode : std::as_const(nodeList))
        removeNodeFromList(childNode);
    maybeRemoveMasterDocument();
}

void ComponentView::nodeReparented(const ModelNode &node, const NodeAbstractProperty &/*newPropertyParent*/, const NodeAbstractProperty &/*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    searchForComponentAndAddToList(node);

    updateDescription(node);
}

void ComponentView::nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId)
{
    updateDescription(node);

    if (oldId.isEmpty())
        return;

    // Material/texture id handling is done here as ComponentView is guaranteed to always be
    // attached, unlike the views actually related to material/texture handling.

    auto maybeSetAuxData = [this, &oldId, &newId](AuxiliaryDataKeyView key) {
        auto rootNode = rootModelNode();
        if (auto property = rootNode.auxiliaryData(key)) {
            if (oldId == property->toString()) {
                QTimer::singleShot(0, this, [rootNode, newId, key]() {
                    rootNode.setAuxiliaryData(key, newId);
                });
            }
        }
    };

    auto metaInfo = node.metaInfo();
    if (metaInfo.isQtQuick3DTexture())
        maybeSetAuxData(Utils3D::matLibSelectedTextureProperty);
}

void ComponentView::nodeSourceChanged(const ModelNode &node, const QString &/*newNodeSource*/)
{
    if (isSubComponentNode(node)) {
        if (!hasEntryForNode(node)) {
            ensureMasterDocument();
            addNodeToList(node);
        }
    } else {
        removeNodeFromList(node);
        maybeRemoveMasterDocument();
    }
}

void ComponentView::customNotification(const AbstractView *,
                                       const QString &identifier,
                                       const QList<ModelNode> &nodeList,
                                       const QList<QVariant> &data)
{
    if (identifier == "UpdateImported3DAsset") {
        Utils::FilePath import3dQml;
        if (!data.isEmpty())
            import3dQml = Utils::FilePath::fromString(data[0].toString());

        ModelNode node;
        if (!nodeList.isEmpty())
            node = nodeList[0];

        Import3dDialog::updateImport(this, import3dQml, node,
                                     m_importableExtensions3DMap,
                                     m_importOptions3DMap);
    }
}

void ComponentView::updateImport3DSupport(const QVariantMap &supportMap)
{
    QVariantMap extMap = qvariant_cast<QVariantMap>(supportMap.value("extensions"));
    if (m_importableExtensions3DMap != extMap) {
        DesignerActionManager *actionManager =
            &QmlDesignerPlugin::instance()->viewManager().designerActionManager();

        if (!m_importableExtensions3DMap.isEmpty())
            actionManager->unregisterAddResourceHandlers(ComponentCoreConstants::add3DAssetsDisplayString);

        m_importableExtensions3DMap = extMap;

        AddResourceOperation import3DModelOperation = [this](const QStringList &fileNames,
                                                             const QString &,
                                                             bool showDialog) -> AddFilesResult {
            Q_UNUSED(showDialog)

            auto importDlg = new Import3dDialog(fileNames,
                                                m_importableExtensions3DMap,
                                                m_importOptions3DMap, {}, {},
                                                this, Core::ICore::dialogParent());
            int result = importDlg->exec();

            return result == QDialog::Accepted ? AddFilesResult::succeeded() : AddFilesResult::cancelled();
        };

        auto add3DHandler = [&](const QString &group, const QString &ext) {
            const QString filter = QStringLiteral("*.%1").arg(ext);
            actionManager->registerAddResourceHandler(
                AddResourceHandler(group, filter,
                                   import3DModelOperation, 10));
        };

        const QHash<QString, QString> groupNames {
            {"3D Scene",                  ComponentCoreConstants::add3DAssetsDisplayString},
            {"Qt 3D Studio Presentation", ComponentCoreConstants::addQt3DSPresentationsDisplayString}
        };

        const auto groups = extMap.keys();
        for (const auto &group : groups) {
            const QStringList exts = extMap[group].toStringList();
            const QString grp = groupNames.contains(group) ? groupNames.value(group) : group;
            for (const auto &ext : exts)
                add3DHandler(grp, ext);
        }
    }

    m_importOptions3DMap = qvariant_cast<QVariantMap>(supportMap.value("options"));
}

void ComponentView::importsChanged(const Imports &addedImports, const Imports &removedImports)
{
#ifndef QDS_USE_PROJECTSTORAGE
    DesignDocument *document = QmlDesignerPlugin::instance()->currentDesignDocument();
    for (const auto &import : addedImports)
        document->addSubcomponentManagerImport(import);
#endif

    // TODO: generalize the logic below to allow adding/removing any Qml component when its import is added/removed
    bool simulinkImportAdded = std::any_of(addedImports.cbegin(), addedImports.cend(), [](const Import &import) {
        return import.url() == "SimulinkConnector";
    });
    if (simulinkImportAdded) {
        // add SLConnector component when SimulinkConnector import is added
        ModelNode node = createModelNode("SLConnector", 1, 0);
        node.bindingProperty("root").setExpression(rootModelNode().validId());
        rootModelNode().defaultNodeListProperty().reparentHere(node);
    } else {
        bool simulinkImportRemoved = std::any_of(removedImports.cbegin(), removedImports.cend(), [](const Import &import) {
            return import.url() == "SimulinkConnector";
        });

        if (simulinkImportRemoved) {
            // remove SLConnector component when SimulinkConnector import is removed
            const QList<ModelNode> slConnectors = Utils::filtered(rootModelNode().directSubModelNodes(),
                                                                  [](const ModelNode &node) {
                                                                      return node.type() == "SLConnector" || node.type() == "SimulinkConnector.SLConnector";
                                                                  });

            for (ModelNode node : slConnectors)
                node.destroy();

            resetPuppet();
        }
    }

    if (model()->hasImport("QtQuick3D"))
        m_ensureMatLibTimer.start();
}

void ComponentView::possibleImportsChanged([[maybe_unused]] const Imports &possibleImports)
{
#ifndef QDS_USE_PROJECTSTORAGE
    DesignDocument *document = QmlDesignerPlugin::instance()->currentDesignDocument();
    for (const auto &import : possibleImports)
        document->addSubcomponentManagerImport(import);
#endif
}

} // namespace QmlDesigner
