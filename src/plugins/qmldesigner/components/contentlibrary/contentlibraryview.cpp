// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "contentlibraryview.h"

#include "contentlibraryiconprovider.h"
#include "contentlibraryitem.h"
#include "contentlibraryeffectsmodel.h"
#include "contentlibrarymaterial.h"
#include "contentlibrarymaterialsmodel.h"
#include "contentlibrarytexture.h"
#include "contentlibrarytexturesmodel.h"
#include "contentlibraryusermodel.h"
#include "contentlibrarywidget.h"

#include <asset.h>
#include <bindingproperty.h>
#include <bundlehelper.h>
#include <bundleimporter.h>
#include <designerpaths.h>
#include <documentmanager.h>
#include <enumeration.h>
#include <externaldependenciesinterface.h>
#include <modelutils.h>
#include <nodeinstanceview.h>
#include <nodelistproperty.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <qmlobjectnode.h>
#include <uniquename.h>
#include <utils3d.h>
#include <variantproperty.h>

#include <solutions/zip/zipreader.h>

#include <utils/algorithm.h>

#ifndef QMLDESIGNER_TEST
#include <projectexplorer/kit.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>
#endif

#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QPixmap>
#include <QVector3D>

namespace QmlDesigner {

ContentLibraryView::ContentLibraryView(AsynchronousImageCache &imageCache,
                                       ExternalDependenciesInterface &externalDependencies)
    : AbstractView(externalDependencies)
    , m_imageCache(imageCache)
    , m_createTexture(this)
{}

ContentLibraryView::~ContentLibraryView()
{}

bool ContentLibraryView::hasWidget() const
{
    return true;
}

WidgetInfo ContentLibraryView::widgetInfo()
{
    if (m_widget.isNull()) {
        m_widget = new ContentLibraryWidget();

        m_bundleHelper = std::make_unique<BundleHelper>(this, m_widget);

        connect(m_widget, &ContentLibraryWidget::bundleMaterialDragStarted, this,
                [&] (QmlDesigner::ContentLibraryMaterial *mat) {
            m_draggedBundleMaterial = mat;
        });
        connect(m_widget, &ContentLibraryWidget::bundleTextureDragStarted, this,
                [&] (QmlDesigner::ContentLibraryTexture *tex) {
            m_draggedBundleTexture = tex;
        });
        connect(m_widget, &ContentLibraryWidget::bundleItemDragStarted, this,
                [&] (QmlDesigner::ContentLibraryItem *item) {
            m_draggedBundleItem = item;
        });

        connect(m_widget, &ContentLibraryWidget::addTextureRequested, this,
                [&] (const QString &texPath, AddTextureMode mode) {
            executeInTransaction("ContentLibraryView::widgetInfo", [&]() {
                m_createTexture.execute(texPath, mode, m_sceneId);
            });
        });

        connect(m_widget, &ContentLibraryWidget::updateSceneEnvStateRequested, this, [&]() {
            ModelNode activeSceneEnv = m_createTexture.resolveSceneEnv(m_sceneId);
            const bool sceneEnvExists = activeSceneEnv.isValid();
            m_widget->texturesModel()->setHasSceneEnv(sceneEnvExists);
            m_widget->environmentsModel()->setHasSceneEnv(sceneEnvExists);
        });

        connect(m_widget, &ContentLibraryWidget::importBundle, this,
                &ContentLibraryView::importBundleToContentLib);

        connect(m_widget->materialsModel(),
                &ContentLibraryMaterialsModel::applyToSelectedTriggered,
                this,
                [&](ContentLibraryMaterial *bundleMat, bool add) {
            if (m_selectedModels.isEmpty())
                return;

            m_bundleMaterialTargets = m_selectedModels;
            m_bundleMaterialAddToSelected = add;

            ModelNode defaultMat = getBundleMaterialDefaultInstance(bundleMat->type());
            if (defaultMat.isValid())
                applyBundleMaterialToDropTarget(defaultMat);
            else
                m_widget->materialsModel()->addToProject(bundleMat);
        });

        connect(m_widget->userModel(),
                &ContentLibraryUserModel::applyToSelectedTriggered,
                this,
                [&](ContentLibraryItem *bundleMat, bool add) {
            if (m_selectedModels.isEmpty())
                return;

            m_bundleMaterialTargets = m_selectedModels;
            m_bundleMaterialAddToSelected = add;

            ModelNode defaultMat = getBundleMaterialDefaultInstance(bundleMat->type());
            if (defaultMat.isValid())
                applyBundleMaterialToDropTarget(defaultMat);
            else
                m_widget->userModel()->addToProject(bundleMat);
        });

        connectImporter();
    }

    return createWidgetInfo(m_widget.data(),
                            "ContentLibrary",
                            WidgetInfo::LeftPane,
                            tr("Content Library"));
}

void ContentLibraryView::connectImporter()
{
#ifdef QDS_USE_PROJECTSTORAGE
    connect(m_widget->importer(),
            &BundleImporter::importFinished,
            this,
            [&](const QmlDesigner::TypeName &typeName, const QString &bundleId) {
                QTC_ASSERT(typeName.size(), return);
                if (isMaterialBundle(bundleId)) {
                    applyBundleMaterialToDropTarget({}, typeName);
                } else if (isItemBundle(bundleId)) {
                    if (!m_bundleItemTarget)
                        m_bundleItemTarget = Utils3D::active3DSceneNode(this);

                    QTC_ASSERT(m_bundleItemTarget, return);

                    executeInTransaction("ContentLibraryView::widgetInfo", [&] {
                        QVector3D pos = m_bundleItemPos.value<QVector3D>();
                        ModelNode newNode = createModelNode(
                            typeName, -1, -1, {{"x", pos.x()}, {"y", pos.y()}, {"z", pos.z()}});
                        m_bundleItemTarget.defaultNodeListProperty().reparentHere(newNode);
                        clearSelectedModelNodes();
                        selectModelNode(newNode);
                    });

                    m_bundleItemTarget = {};
                    m_bundleItemPos = {};
                }
            });
#else
    connect(m_widget->importer(),
            &BundleImporter::importFinished,
            this,
            [&](const QmlDesigner::NodeMetaInfo &metaInfo, const QString &bundleId) {
                QTC_ASSERT(metaInfo.isValid(), return);
                if (isMaterialBundle(bundleId)) {
                    applyBundleMaterialToDropTarget({}, metaInfo);
                } else if (isItemBundle(bundleId)) {
                    if (!m_bundleItemTarget)
                        m_bundleItemTarget = Utils3D::active3DSceneNode(this);
                    if (!m_bundleItemTarget)
                        m_bundleItemTarget = rootModelNode();

                    QTC_ASSERT(m_bundleItemTarget, return);

                    executeInTransaction("ContentLibraryView::connectImporter", [&] {
                        QVector3D pos = m_bundleItemPos.value<QVector3D>();
                        ModelNode newNode = createModelNode(metaInfo.typeName(),
                                                               metaInfo.majorVersion(),
                                                               metaInfo.minorVersion(),
                                                               {{"x", pos.x()},
                                                                {"y", pos.y()},
                                                                {"z", pos.z()}});
                        m_bundleItemTarget.defaultNodeListProperty().reparentHere(newNode);
                        newNode.setIdWithoutRefactoring(model()->generateNewId(
                            newNode.simplifiedTypeName(), "node"));
                        clearSelectedModelNodes();
                        selectModelNode(newNode);
                    });

                    m_bundleItemTarget = {};
                    m_bundleItemPos = {};
                }
            });
#endif

    connect(m_widget->importer(), &BundleImporter::aboutToUnimport, this,
            [&] (const QmlDesigner::TypeName &type, const QString &bundleId) {
        if (isMaterialBundle(bundleId)) {
            // delete instances of the bundle material that is about to be unimported
            executeInTransaction("ContentLibraryView::connectImporter", [&] {
                ModelNode matLib = Utils3D::materialLibraryNode(this);
                if (!matLib.isValid())
                    return;

                Utils::reverseForeach(matLib.directSubModelNodes(), [&](const ModelNode &mat) {
                    if (mat.isValid() && mat.type() == type)
                        QmlObjectNode(mat).destroy();
                });
            });
        } else if (isItemBundle(bundleId)) {
            // delete instances of the bundle item that is about to be unimported
            executeInTransaction("ContentLibraryView::connectImporter", [&] {
                NodeMetaInfo metaInfo = model()->metaInfo(type);
                QList<ModelNode> nodes = allModelNodesOfType(metaInfo);
                for (ModelNode &node : nodes)
                    node.destroy();
            });
        }
    });
}

bool ContentLibraryView::isMaterialBundle(const QString &bundleId) const
{
    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();
    return bundleId == compUtils.materialsBundleId() || bundleId == compUtils.userMaterialsBundleId();
}

// item bundle includes effects and 3D components
bool ContentLibraryView::isItemBundle(const QString &bundleId) const
{
    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();
    return bundleId == compUtils.effectsBundleId() || bundleId == compUtils.userEffectsBundleId()
        || bundleId == compUtils.user3DBundleId();
}

void ContentLibraryView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    m_hasQuick3DImport = model->hasImport("QtQuick3D");

    updateBundlesQuick3DVersion();

    const bool hasLibrary = Utils3D::materialLibraryNode(this).isValid();
    m_widget->setHasMaterialLibrary(hasLibrary);
    m_widget->setHasQuick3DImport(m_hasQuick3DImport);
    m_widget->setIsQt6Project(externalDependencies().isQt6Project());

    m_sceneId = Utils3D::active3DSceneId(model);

    m_widget->setHasActive3DScene(m_sceneId != -1);
    m_widget->clearSearchFilter();

    // bundles loading has to happen here, otherwise project path is not ready which will
    // cause bundle items types to resolve incorrectly
    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();
    QString genFolderName = compUtils.generatedComponentsPath().fileName();
    bool forceReload = m_generatedFolderName != genFolderName;

    m_widget->materialsModel()->loadBundle();
    m_widget->effectsModel()->loadBundle(forceReload);
    m_widget->userModel()->loadBundles(forceReload);

    m_widget->updateImportedState(compUtils.materialsBundleId());
    m_widget->updateImportedState(compUtils.effectsBundleId());
    m_widget->updateImportedState(compUtils.userMaterialsBundleId());
    m_widget->updateImportedState(compUtils.user3DBundleId());

    m_generatedFolderName = genFolderName;
}

void ContentLibraryView::modelAboutToBeDetached(Model *model)
{
    m_widget->setHasMaterialLibrary(false);
    m_widget->setHasQuick3DImport(false);

    AbstractView::modelAboutToBeDetached(model);
}

void ContentLibraryView::importsChanged(const Imports &addedImports, const Imports &removedImports)
{
    Q_UNUSED(addedImports)
    Q_UNUSED(removedImports)

    updateBundlesQuick3DVersion();

    bool hasQuick3DImport = model()->hasImport("QtQuick3D");

    if (hasQuick3DImport == m_hasQuick3DImport)
        return;

    m_hasQuick3DImport = hasQuick3DImport;
    m_widget->setHasQuick3DImport(m_hasQuick3DImport);
}

void ContentLibraryView::active3DSceneChanged(qint32 sceneId)
{
    m_sceneId = sceneId;
    m_widget->setHasActive3DScene(m_sceneId != -1);
}

void ContentLibraryView::selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                              const QList<ModelNode> &lastSelectedNodeList)
{
    Q_UNUSED(lastSelectedNodeList)

    m_selectedModels = Utils::filtered(selectedNodeList, [](const ModelNode &node) {
        return node.metaInfo().isQtQuick3DModel();
    });

    m_widget->setHasModelSelection(!m_selectedModels.isEmpty());
}

void ContentLibraryView::customNotification(const AbstractView *view,
                                            const QString &identifier,
                                            const QList<ModelNode> &nodeList,
                                            const QList<QVariant> &data)
{
    if (view == this)
        return;

    if (identifier == "drop_bundle_material") {
        ModelNode matLib = Utils3D::materialLibraryNode(this);
        if (!matLib.isValid())
            return;

        m_bundleMaterialTargets = nodeList;

        ModelNode defaultMat = getBundleMaterialDefaultInstance(m_draggedBundleMaterial->type());
        if (defaultMat.isValid()) {
            if (m_bundleMaterialTargets.isEmpty()) { // if no drop target, create a duplicate material
                executeInTransaction(__FUNCTION__, [&] {
#ifdef QDS_USE_PROJECTSTORAGE
                    Utils3D::createMaterial(this, m_draggedBundleMaterial->type());
#else
                    Utils3D::createMaterial(this, defaultMat.metaInfo());
#endif
                });
            } else {
                applyBundleMaterialToDropTarget(defaultMat);
            }
        } else {
            m_widget->materialsModel()->addToProject(m_draggedBundleMaterial);
        }

        m_draggedBundleMaterial = nullptr;
    } else if (identifier == "drop_bundle_texture") {
        ModelNode matLib = Utils3D::materialLibraryNode(this);
        if (!matLib.isValid())
            return;

        m_widget->addTexture(m_draggedBundleTexture);

        m_draggedBundleTexture = nullptr;
    } else if (identifier == "drop_bundle_item") {
        QTC_ASSERT(nodeList.size() == 1, return);

        auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();
        bool is3D = m_draggedBundleItem->type().startsWith(compUtils.user3DBundleType().toLatin1());

        m_bundleItemPos = data.size() == 1 ? data.first() : QVariant();
        if (is3D)
            m_widget->userModel()->addToProject(m_draggedBundleItem);
        else
            m_widget->effectsModel()->addInstance(m_draggedBundleItem);
        m_bundleItemTarget = nodeList.first() ? nodeList.first() : Utils3D::active3DSceneNode(this);
    } else if (identifier == "add_material_to_content_lib") {
        QTC_ASSERT(nodeList.size() == 1 && data.size() == 1, return);

        addLibItem(nodeList.first(), data.first().value<QPixmap>());
        m_widget->showTab(ContentLibraryWidget::TabIndex::UserAssetsTab);
    } else if (identifier == "add_assets_to_content_lib") {
        addLibAssets(data.first().toStringList());
        m_widget->showTab(ContentLibraryWidget::TabIndex::UserAssetsTab);
    } else if (identifier == "add_3d_to_content_lib") {
        if (nodeList.first().isComponent())
            addLib3DComponent(nodeList.first());
        else
            addLibItem(nodeList.first());
        m_widget->showTab(ContentLibraryWidget::TabIndex::UserAssetsTab);
    }
}

void ContentLibraryView::nodeReparented(const ModelNode &node,
                                        [[maybe_unused]] const NodeAbstractProperty &newPropertyParent,
                                        [[maybe_unused]] const NodeAbstractProperty &oldPropertyParent,
                                        [[maybe_unused]] PropertyChangeFlags propertyChange)
{
    if (node.id() == Constants::MATERIAL_LIB_ID)
        m_widget->setHasMaterialLibrary(true);
}

void ContentLibraryView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    if (removedNode.id() == Constants::MATERIAL_LIB_ID)
        m_widget->setHasMaterialLibrary(false);
}

void ContentLibraryView::auxiliaryDataChanged(const ModelNode &,
                                              AuxiliaryDataKeyView type,
                                              const QVariant &data)
{
    if (type == Utils3D::active3dSceneProperty)
        active3DSceneChanged(data.toInt());
}

void ContentLibraryView::modelNodePreviewPixmapChanged(const ModelNode &,
                                                       const QPixmap &pixmap,
                                                       const QByteArray &requestId)
{
    if (requestId == ADD_ITEM_REQ_ID)
        saveIconToBundle(pixmap);
}

#ifdef QDS_USE_PROJECTSTORAGE
void ContentLibraryView::applyBundleMaterialToDropTarget(const ModelNode &bundleMat,
                                                         const TypeName &typeName)
{
    if (!bundleMat.isValid() && !typeName.size())
        return;

    executeInTransaction("ContentLibraryView::applyBundleMaterialToDropTarget", [&] {
        ModelNode newMatNode = typeName.size() ? Utils3D::createMaterial(this, typeName) : bundleMat;
        for (const ModelNode &target : std::as_const(m_bundleMaterialTargets)) {
            if (target.isValid() && target.metaInfo().isQtQuick3DModel()) {
                QmlObjectNode qmlObjNode(target);
                if (m_bundleMaterialAddToSelected) {
                    QStringList matList = ModelUtils::expressionToList(
                        qmlObjNode.expression("materials"));
                    matList.append(newMatNode.id());
                    QString updatedExp = ModelUtils::listToExpression(matList);
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
#else
void ContentLibraryView::applyBundleMaterialToDropTarget(const ModelNode &bundleMat,
                                                         const NodeMetaInfo &metaInfo)
{
    if (!bundleMat.isValid() && !metaInfo.isValid())
        return;

    executeInTransaction("ContentLibraryView::applyBundleMaterialToDropTarget", [&] {
        ModelNode newMatNode = metaInfo.isValid() ? Utils3D::createMaterial(this, metaInfo) : bundleMat;

        for (const ModelNode &target : std::as_const(m_bundleMaterialTargets)) {
            if (target.isValid() && target.metaInfo().isQtQuick3DModel()) {
                QmlObjectNode qmlObjNode(target);
                if (m_bundleMaterialAddToSelected) {
                    QStringList matList = ModelUtils::expressionToList(
                        qmlObjNode.expression("materials"));
                    matList.append(newMatNode.id());
                    QString updatedExp = ModelUtils::listToExpression(matList);
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
#endif

void ContentLibraryView::addLibAssets(const QStringList &paths)
{
    auto bundlePath = Utils::FilePath::fromString(Paths::bundlesPathSetting() + "/User/textures");
    Utils::FilePaths sourcePathsToAdd;
    Utils::FilePaths targetPathsToAdd;
    QStringList fileNamesToRemove;

    const QStringList existingAssetsFileNames = Utils::transform(bundlePath.dirEntries(QDir::Files),
                                                                 &Utils::FilePath::fileName);

    for (const QString &path : paths) {
        auto assetFilePath = Utils::FilePath::fromString(path);
        QString assetFileName = assetFilePath.fileName();

        // confirm overwrite if an item with same name exists
        if (existingAssetsFileNames.contains(assetFileName)) {
            QMessageBox::StandardButton reply = QMessageBox::question(m_widget, tr("Texture Exists"),
                  tr("A texture with the same name '%1' already exists in the Content Library, are you sure you want to overwrite it?")
                      .arg(assetFileName), QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::No)
                continue;

            fileNamesToRemove.append(assetFileName);
        }

        sourcePathsToAdd.append(assetFilePath);
    }

    // remove the to-be-overwritten resources from target bundle path
    m_widget->userModel()->removeTextures(fileNamesToRemove);

    // copy resources to target bundle path
    for (const Utils::FilePath &sourcePath : sourcePathsToAdd) {
        Utils::FilePath targetPath = bundlePath.pathAppended(sourcePath.fileName());
        Asset asset{sourcePath.toFSPathString()};

        // save icon
        QString iconSavePath = bundlePath.pathAppended("icons/" + sourcePath.baseName() + ".png")
                                   .toFSPathString();
        QPixmap icon = asset.pixmap({120, 120});
        bool iconSaved = icon.save(iconSavePath);
        if (!iconSaved)
            qWarning() << __FUNCTION__ << "icon save failed";

        // save asset
        auto result = sourcePath.copyFile(targetPath);
        QTC_ASSERT_EXPECTED(result,);

        targetPathsToAdd.append(targetPath);
    }

    m_widget->userModel()->addTextures(targetPathsToAdd);
}

void ContentLibraryView::addLib3DComponent(const ModelNode &node)
{
    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();

    m_bundleId = compUtils.user3DBundleId();

    QString compBaseName = node.simplifiedTypeName();
    QString compFileName = compBaseName + ".qml";

    auto compDir = Utils::FilePath::fromString(ModelUtils::componentFilePath(node)).parentDir();
    auto bundlePath = Utils::FilePath::fromString(Paths::bundlesPathSetting() + "/User/3d/");

    // confirm overwrite if an item with same name exists
    if (bundlePath.pathAppended(compFileName).exists()) {
        // Show a QML confirmation dialog before proceeding
        QMessageBox::StandardButton reply = QMessageBox::question(m_widget, tr("3D Item Exists"),
            tr("A 3D item with the same name '%1' already exists in the Content Library, are you sure you want to overwrite it?")
                    .arg(compFileName), QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No)
            return;

        // before overwriting remove old item (to avoid partial items and dangling assets)
        m_widget->userModel()->removeItemByName(compFileName, m_bundleId);
    }

    QString iconPath = QLatin1String("icons/%1").arg(UniqueName::generateId(compBaseName) + ".png");
    m_iconSavePath = bundlePath.pathAppended(iconPath);
    m_iconSavePath.parentDir().ensureWritableDir();

    const Utils::FilePaths sourceFiles = compDir.dirEntries({{}, QDir::Files, QDirIterator::Subdirectories});
    const QStringList ignoreList {"_importdata.json", "qmldir", compBaseName + ".hints"};
    QStringList filesList; // 3D component's assets (dependencies)

    for (const Utils::FilePath &sourcePath : sourceFiles) {
        Utils::FilePath relativePath = sourcePath.relativePathFrom(compDir);
        if (ignoreList.contains(sourcePath.fileName()) || relativePath.startsWith("source scene"))
            continue;

        Utils::FilePath targetPath = bundlePath.pathAppended(relativePath.path());
        targetPath.parentDir().ensureWritableDir();

        // copy item from project to user bundle
        auto result = sourcePath.copyFile(targetPath);
        QTC_ASSERT_EXPECTED(result,);

        if (sourcePath.fileName() != compFileName) // skip component file (only collect dependencies)
            filesList.append(relativePath.path());
    }

    // add the item to the bundle json
    QJsonObject &jsonRef = m_widget->userModel()->bundleObjectRef(m_bundleId);
    QJsonArray itemsArr = jsonRef.value("items").toArray();
    itemsArr.append(QJsonObject {
        {"name", node.simplifiedTypeName()},
        {"qml", compFileName},
        {"icon", iconPath},
        {"files", QJsonArray::fromStringList(filesList)}
    });

    jsonRef["items"] = itemsArr;

    auto result = bundlePath.pathAppended(Constants::BUNDLE_JSON_FILENAME)
                      .writeFileContents(QJsonDocument(jsonRef).toJson());
    QTC_ASSERT_EXPECTED(result,);

    m_widget->userModel()->addItem(m_bundleId, compBaseName, compFileName, m_iconSavePath.toUrl(),
                                   filesList);

    // generate and save icon
    m_bundleHelper->getImageFromCache(compDir.pathAppended(compFileName).path(), [&](const QImage &image) {
        saveIconToBundle(image);
    });
}

void ContentLibraryView::addLibItem(const ModelNode &node, const QPixmap &iconPixmap)
{
    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();

    m_bundleId.clear();
    Utils::FilePath bundlePath;

    if (node.metaInfo().isQtQuick3DMaterial()) {
        bundlePath = Utils::FilePath::fromString(Paths::bundlesPathSetting() + "/User/materials/");
        m_bundleId = compUtils.userMaterialsBundleId();
    } else if (node.metaInfo().isQtQuick3DEffect()) {
        bundlePath = Utils::FilePath::fromString(Paths::bundlesPathSetting() + "/User/effects/");
        m_bundleId = compUtils.userEffectsBundleId();
    } else if (node.metaInfo().isQtQuick3DNode()) {
        bundlePath = Utils::FilePath::fromString(Paths::bundlesPathSetting() + "/User/3d/");
        m_bundleId = compUtils.user3DBundleId();
    } else {
        qWarning() << __FUNCTION__ << "Unsuppported node type";
        return;
    }

    QString name = node.variantProperty("objectName").value().toString();
    if (name.isEmpty())
        name = node.displayName();

    QJsonObject &jsonRef = m_widget->userModel()->bundleObjectRef(m_bundleId);
    QJsonArray itemsArr = jsonRef.value("items").toArray();

    QString qml = m_bundleHelper->nodeNameToComponentFileName(name);

    // confirm overwrite if an item with same name exists
    if (m_widget->userModel()->jsonPropertyExists("qml", qml, m_bundleId)) {
        QMessageBox::StandardButton reply = QMessageBox::question(m_widget, tr("Component Exists"),
                                              tr("A component with the same name '%1' already "
                                                 "exists in the Content Library, are you sure "
                                                 "you want to overwrite it?")
                                                  .arg(qml), QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No)
            return;

        // before overwriting remove old item (to avoid partial items and dangling assets)
        m_widget->userModel()->removeItemByName(qml, m_bundleId);
    }

    // generate and save Qml file
    auto [qmlString, depAssets] = m_bundleHelper->modelNodeToQmlString(node);
    const QList<AssetPath> depAssetsList = depAssets.values();

    QStringList depAssetsRelativePaths;
    for (const AssetPath &assetPath : depAssetsList)
        depAssetsRelativePaths.append(assetPath.relativePath);

    auto result = bundlePath.pathAppended(qml).writeFileContents(qmlString.toUtf8());
    QTC_ASSERT_EXPECTED(result,);

    // get icon path
    QString iconPathTemplate = QLatin1String("icons/%1.png");
    QString iconBaseName = UniqueName::generateId(name, [&] (const QString &currName) {
        return m_widget->userModel()->jsonPropertyExists("icon", iconPathTemplate.arg(currName),
                                                         m_bundleId);
    });

    QString iconPath = iconPathTemplate.arg(iconBaseName);

    // add the item to the bundle json
    itemsArr = jsonRef.value("items").toArray();
    itemsArr.append(QJsonObject {
        {"name", name},
        {"qml", qml},
        {"icon", iconPath},
        {"files", QJsonArray::fromStringList(depAssetsRelativePaths)}
    });

    jsonRef["items"] = itemsArr;

    result = bundlePath.pathAppended(Constants::BUNDLE_JSON_FILENAME)
                      .writeFileContents(QJsonDocument(jsonRef).toJson());
    QTC_ASSERT_EXPECTED(result,);

    // copy item's assets to target folder
    for (const AssetPath &assetPath : depAssetsList) {
        Utils::FilePath assetPathSource = assetPath.absFilPath();
        Utils::FilePath assetPathTarget = bundlePath.pathAppended(assetPath.relativePath);
        assetPathTarget.parentDir().ensureWritableDir();

        auto result = assetPathSource.copyFile(assetPathTarget);
        QTC_ASSERT_EXPECTED(result,);
    }

    m_iconSavePath = bundlePath.pathAppended(iconPath);
    m_iconSavePath.parentDir().ensureWritableDir();

    m_widget->userModel()->addItem(m_bundleId, name, qml, m_iconSavePath.toUrl(), depAssetsRelativePaths);

    // generate and save icon
    QPixmap iconPixmapToSave;
    if (node.metaInfo().isQtQuick3DCamera())
        iconPixmapToSave = m_widget->iconProvider()->requestPixmap("camera.png", nullptr, {});
    else if (node.metaInfo().isQtQuick3DLight())
        iconPixmapToSave = m_widget->iconProvider()->requestPixmap("light.png", nullptr, {});
    else
        iconPixmapToSave = iconPixmap;

    if (iconPixmapToSave.isNull()) {
        static_cast<const NodeInstanceView *>(model()->nodeInstanceView())
            ->previewImageDataForGenericNode(node, {}, {}, ADD_ITEM_REQ_ID);
    } else {
        saveIconToBundle(iconPixmapToSave);
    }
}

void ContentLibraryView::saveIconToBundle(const auto &image) { // auto: QImage or QPixmap
    bool iconSaved = image.save(m_iconSavePath.toFSPathString());
    if (iconSaved)
        m_widget->userModel()->refreshSection(m_bundleId);
    else
        qWarning() << __FUNCTION__ << ": icon save failed";

    m_iconSavePath.clear();
};

void ContentLibraryView::importBundleToContentLib()
{
    QString importPath = m_bundleHelper->getImportPath();
    if (importPath.isEmpty())
        return;

    ZipReader zipReader(importPath);

    QByteArray bundleJsonContent = zipReader.fileData(Constants::BUNDLE_JSON_FILENAME);
    QTC_ASSERT(!bundleJsonContent.isEmpty(), return);

    const QJsonObject importedJsonObj = QJsonDocument::fromJson(bundleJsonContent).object();
    const QJsonArray importedItemsArr = importedJsonObj.value("items").toArray();
    QTC_ASSERT(!importedItemsArr.isEmpty(), return);

    QString bundleVersion = importedJsonObj.value("version").toString();
    bool bundleVersionOk = !bundleVersion.isEmpty() && bundleVersion == BUNDLE_VERSION;
    if (!bundleVersionOk) {
        QMessageBox::warning(m_widget, tr("Unsupported bundle file"),
                             tr("The chosen bundle was created with an incompatible version of Qt Design Studio"));
        return;
    }
    QString bundleId = importedJsonObj.value("id").toString();
    bool isMat = isMaterialBundle(bundleId);

    QString bundleFolderName = isMat ? QLatin1String("materials") : QLatin1String("3d");

    auto bundlePath = Utils::FilePath::fromString(QLatin1String("%1/User/%3/")
                                            .arg(Paths::bundlesPathSetting(), bundleFolderName));

    QJsonObject &jsonRef = m_widget->userModel()->bundleObjectRef(bundleId);
    QJsonArray itemsArr = jsonRef.value("items").toArray();

    QStringList existingQmls;
    for (const QJsonValueConstRef &itemRef : std::as_const(itemsArr))
        existingQmls.append(itemRef.toObject().value("qml").toString());

    for (const QJsonValueConstRef &itemRef : importedItemsArr) {
        QJsonObject itemObj = itemRef.toObject();
        QString qml = itemObj.value("qml").toString();

        // confirm overwrite if an item with same name exists
        if (existingQmls.contains(qml)) {
            QMessageBox::StandardButton reply = QMessageBox::question(m_widget, tr("Component Exists"),
                                                tr("A component with the same name '%1' already "
                                                   "exists in the Content Library, are you sure "
                                                   "you want to overwrite it?")
                                            .arg(qml), QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::No)
                continue;

            // before overwriting remove old item (to avoid partial items and dangling assets)
            m_widget->userModel()->removeItemByName(qml, bundleId);
        }

        // add entry to json
        itemsArr.append(itemRef);

        // add entry to model
        QString name = itemObj.value("name").toString();
        QStringList files = itemObj.value("files").toVariant().toStringList();
        QString icon = itemObj.value("icon").toString();
        QUrl iconUrl = bundlePath.pathAppended(icon).toUrl();

        // copy files
        files << qml << icon; // all files
        for (const QString &file : std::as_const(files)) {
            Utils::FilePath filePath = bundlePath.pathAppended(file);
            filePath.parentDir().ensureWritableDir();
            QTC_ASSERT_EXPECTED(filePath.writeFileContents(zipReader.fileData(file)),);
        }

        m_widget->userModel()->addItem(bundleId, name, qml, iconUrl, files);
    }

    m_widget->userModel()->refreshSection(bundleId);

    zipReader.close();

    jsonRef["items"] = itemsArr;

    auto result = bundlePath.pathAppended(Constants::BUNDLE_JSON_FILENAME)
                      .writeFileContents(QJsonDocument(jsonRef).toJson());
    QTC_ASSERT_EXPECTED(result,);
}

ModelNode ContentLibraryView::getBundleMaterialDefaultInstance(const TypeName &type)
{
    ModelNode matLib = Utils3D::materialLibraryNode(this);
    if (!matLib.isValid())
        return {};

    const QList<ModelNode> matLibNodes = matLib.directSubModelNodes();
    for (const ModelNode &mat : matLibNodes) {
        if (mat.isValid() && mat.type() == type) {
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

void ContentLibraryView::updateBundlesQuick3DVersion()
{
    bool hasImport = false;
    int major = -1;
    int minor = -1;
    const QString url{"QtQuick3D"};
    const auto imports = model()->imports();
    for (const auto &import : imports) {
        if (import.url() == url) {
            hasImport = true;
            const int importMajor = import.majorVersion();
            if (major < importMajor) {
                minor = -1;
                major = importMajor;
            }
            if (major == importMajor)
                minor = qMax(minor, import.minorVersion());
        }
    }
#ifndef QMLDESIGNER_TEST
    if (hasImport && major == -1) {
        // Import without specifying version, so we take the kit version
        auto target = ProjectExplorer::ProjectManager::startupTarget();
        if (target) {
            QtSupport::QtVersion *qtVersion = QtSupport::QtKitAspect::qtVersion(target->kit());
            if (qtVersion) {
                major = qtVersion->qtVersion().majorVersion();
                minor = qtVersion->qtVersion().minorVersion();
            }
        }
    }
#endif
    m_widget->materialsModel()->setQuick3DImportVersion(major, minor);
    m_widget->effectsModel()->setQuick3DImportVersion(major, minor);
    m_widget->userModel()->setQuick3DImportVersion(major, minor);
}

} // namespace QmlDesigner
