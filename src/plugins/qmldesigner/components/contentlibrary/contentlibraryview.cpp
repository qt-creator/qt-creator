// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "contentlibraryview.h"

#include "contentlibrarybundleimporter.h"
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
#include <designerpaths.h>
#include <documentmanager.h>
#include <enumeration.h>
#include <externaldependenciesinterface.h>
#include <model/modelutils.h>
#include <nodelistproperty.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <qmlobjectnode.h>
#include <uniquename.h>
#include <utils3d.h>
#include <variantproperty.h>

#include <solutions/zip/zipreader.h>
#include <solutions/zip/zipwriter.h>

#include <utils/algorithm.h>

#ifndef QMLDESIGNER_TEST
#include <projectexplorer/kit.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>
#endif

#include <QBuffer>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QPixmap>
#include <QTemporaryDir>
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
                            0,
                            tr("Content Library"));
}

void ContentLibraryView::connectImporter()
{
#ifdef QDS_USE_PROJECTSTORAGE
    connect(m_widget->importer(),
            &ContentLibraryBundleImporter::importFinished,
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
            &ContentLibraryBundleImporter::importFinished,
            this,
            [&](const QmlDesigner::NodeMetaInfo &metaInfo, const QString &bundleId) {
                QTC_ASSERT(metaInfo.isValid(), return);
                if (isMaterialBundle(bundleId)) {
                    applyBundleMaterialToDropTarget({}, metaInfo);
                } else if (isItemBundle(bundleId)) {
                    if (!m_bundleItemTarget)
                        m_bundleItemTarget = Utils3D::active3DSceneNode(this);

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
                        clearSelectedModelNodes();
                        selectModelNode(newNode);
                    });

                    m_bundleItemTarget = {};
                    m_bundleItemPos = {};
                }
            });
#endif

    connect(m_widget->importer(), &ContentLibraryBundleImporter::aboutToUnimport, this,
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
    m_widget->materialsModel()->loadBundle();
    m_widget->effectsModel()->loadBundle();
    m_widget->userModel()->loadBundles();

    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();
    m_widget->updateImportedState(compUtils.materialsBundleId());
    m_widget->updateImportedState(compUtils.effectsBundleId());
    m_widget->updateImportedState(compUtils.userMaterialsBundleId());
    m_widget->updateImportedState(compUtils.user3DBundleId());
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
            if (m_bundleMaterialTargets.isEmpty()) // if no drop target, create a duplicate material
#ifdef QDS_USE_PROJECTSTORAGE
                createMaterial(m_draggedBundleMaterial->type());
#else
                createMaterial(defaultMat.metaInfo());
#endif
            else
                applyBundleMaterialToDropTarget(defaultMat);
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
            m_widget->userModel()->add3DInstance(m_draggedBundleItem);
        else
            m_widget->effectsModel()->addInstance(m_draggedBundleItem);
        m_bundleItemTarget = nodeList.first() ? nodeList.first() : Utils3D::active3DSceneNode(this);
    } else if (identifier == "add_material_to_content_lib") {
        QTC_ASSERT(nodeList.size() == 1 && data.size() == 1, return);

        addLibMaterial(nodeList.first(), data.first().value<QPixmap>());
    } else if (identifier == "add_assets_to_content_lib") {
        addLibAssets(data.first().toStringList());
    } else if (identifier == "add_3d_to_content_lib") {
        if (nodeList.first().isComponent())
            addLib3DComponent(nodeList.first());
        else
            addLib3DItem(nodeList.first());
    } else if (identifier == "export_item_as_bundle") {
        // TODO: support exporting 2D items
        if (nodeList.first().isComponent())
            exportLib3DComponent(nodeList.first());
        else
            exportLib3DItem(nodeList.first());
    } else if (identifier == "export_material_as_bundle") {
        exportLib3DItem(nodeList.first(), data.first().value<QPixmap>());
    } else if (identifier == "import_bundle") {
        importBundle();
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

#ifdef QDS_USE_PROJECTSTORAGE
void ContentLibraryView::applyBundleMaterialToDropTarget(const ModelNode &bundleMat,
                                                         const TypeName &typeName)
{
    if (!bundleMat.isValid() && !typeName.size())
        return;

    executeInTransaction("ContentLibraryView::applyBundleMaterialToDropTarget", [&] {
        ModelNode newMatNode = typeName.size() ? createMaterial(typeName) : bundleMat;

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
            if (target.isValid() && target.metaInfo().isQtQuick3DModel()) {
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
#else
void ContentLibraryView::applyBundleMaterialToDropTarget(const ModelNode &bundleMat,
                                                         const NodeMetaInfo &metaInfo)
{
    if (!bundleMat.isValid() && !metaInfo.isValid())
        return;

    executeInTransaction("ContentLibraryView::applyBundleMaterialToDropTarget", [&] {
        ModelNode newMatNode = metaInfo.isValid() ? createMaterial(metaInfo) : bundleMat;

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
            if (target.isValid() && target.metaInfo().isQtQuick3DModel()) {
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
#endif

// Add a project material to Content Library's user tab
void ContentLibraryView::addLibMaterial(const ModelNode &node, const QPixmap &iconPixmap)
{
    auto bundlePath = Utils::FilePath::fromString(Paths::bundlesPathSetting() + "/User/materials/");

    QString name = node.variantProperty("objectName").value().toString();
    auto [qml, icon] = m_widget->userModel()->getUniqueLibMaterialNames(node.id());

    QString iconPath = QLatin1String("icons/%1").arg(icon);
    QString fullIconPath = bundlePath.pathAppended(iconPath).toFSPathString();

    // save icon
    bool iconSaved = iconPixmap.save(fullIconPath);
    if (!iconSaved)
        qWarning() << __FUNCTION__ << "icon save failed";

    // generate and save material Qml file
    auto [qmlString, depAssets] = modelNodeToQmlString(node);
    const QStringList depAssetsList = depAssets.values();

    auto result = bundlePath.pathAppended(qml).writeFileContents(qmlString.toUtf8());
    QTC_ASSERT_EXPECTED(result,);

    // add the material to the bundle json
    QJsonObject &jsonRef = m_widget->userModel()->bundleJsonMaterialObjectRef();
    QJsonArray itemsArr = jsonRef.value("items").toArray();
    itemsArr.append(QJsonObject {
        {"name", name},
        {"qml", qml},
        {"icon", iconPath},
        {"files", QJsonArray::fromStringList(depAssetsList)}
    });

    jsonRef["items"] = itemsArr;

    result = bundlePath.pathAppended(Constants::BUNDLE_JSON_FILENAME)
                      .writeFileContents(QJsonDocument(jsonRef).toJson());
    QTC_ASSERT_EXPECTED(result,);

    // copy material assets to bundle folder
    for (const QString &assetPath : depAssetsList) {
        Utils::FilePath assetPathSource = DocumentManager::currentResourcePath().pathAppended(assetPath);
        Utils::FilePath assetPathTarget = bundlePath.pathAppended(assetPath);
        assetPathTarget.parentDir().ensureWritableDir();

        auto result = assetPathSource.copyFile(assetPathTarget);
        QTC_ASSERT_EXPECTED(result,);
    }

    m_widget->userModel()->addMaterial(name, qml, QUrl::fromLocalFile(fullIconPath), depAssetsList);
}

QPair<QString, QSet<QString>> ContentLibraryView::modelNodeToQmlString(const ModelNode &node, int depth)
{
    static QStringList depListIds;

    QString qml;
    QSet<QString> assets;

    if (depth == 0) {
        qml.append("import QtQuick\nimport QtQuick3D\n\n");
        depListIds.clear();
    }

    QString indent = QString(" ").repeated(depth * 4);

    qml += indent + node.simplifiedTypeName() + " {\n";

    indent = QString(" ").repeated((depth + 1) * 4);

    qml += indent + "id: " + (depth == 0 ? "root" : node.id()) + " \n\n";

    const QList<PropertyName> excludedProps = {"x", "y", "z", "eulerRotation.x", "eulerRotation.y",
                                               "eulerRotation.z", "scale.x", "scale.y", "scale.z",
                                               "pivot.x", "pivot.y", "pivot.z"};
    const QList<AbstractProperty> matProps = node.properties();
    for (const AbstractProperty &p : matProps) {
        if (excludedProps.contains(p.name()))
            continue;

        if (p.isVariantProperty()) {
            QVariant pValue = p.toVariantProperty().value();
            QString val;

            if (!pValue.typeName()) {
                // dynamic property with no value assigned
            } else if (strcmp(pValue.typeName(), "QString") == 0 || strcmp(pValue.typeName(), "QColor") == 0) {
                val = QLatin1String("\"%1\"").arg(pValue.toString());
            } else if (strcmp(pValue.typeName(), "QUrl") == 0) {
                QString pValueStr = pValue.toString();
                val = QLatin1String("\"%1\"").arg(pValueStr);
                if (!pValueStr.startsWith("#"))
                    assets.insert(pValue.toString());
            } else if (strcmp(pValue.typeName(), "QmlDesigner::Enumeration") == 0) {
                val = pValue.value<QmlDesigner::Enumeration>().toString();
            } else {
                val = pValue.toString();
            }
            if (p.isDynamic()) {
                QString valWithColon = val.isEmpty() ? QString() : (": " + val);
                qml += indent + "property "  + p.dynamicTypeName() + " " + p.name() + valWithColon + "\n";
            } else {
                qml += indent + p.name() + ": " + val + "\n";
            }
        } else if (p.isBindingProperty()) {
            ModelNode depNode = modelNodeForId(p.toBindingProperty().expression());
            QTC_ASSERT(depNode.isValid(), continue);

            if (p.isDynamic())
                qml += indent + "property "  + p.dynamicTypeName() + " " + p.name() + ": " + depNode.id() + "\n";
            else
                qml += indent + p.name() + ": " + depNode.id() + "\n";

            if (depNode && !depListIds.contains(depNode.id())) {
                depListIds.append(depNode.id());
                auto [depQml, depAssets] = modelNodeToQmlString(depNode, depth + 1);
                qml += "\n" + depQml + "\n";
                assets.unite(depAssets);
            }
        }
    }

    indent = QString(" ").repeated(depth * 4);

    qml += indent + "}\n";

    return {qml, assets};
}

void ContentLibraryView::addLibAssets(const QStringList &paths)
{
    auto bundlePath = Utils::FilePath::fromString(Paths::bundlesPathSetting() + "/User/textures");
    QStringList pathsInBundle;

    const QStringList existingTextures = Utils::transform(bundlePath.dirEntries(QDir::Files),
                                                    [](const Utils::FilePath &path) {
        return path.fileName();
    });

    for (const QString &path : paths) {
        auto assetFilePath = Utils::FilePath::fromString(path);
        if (existingTextures.contains(assetFilePath.fileName()))
            continue;

        Asset asset(path);

        // save icon
        QString iconSavePath = bundlePath.pathAppended("icons/" + assetFilePath.baseName() + ".png")
                                   .toFSPathString();
        QPixmap icon = asset.pixmap({120, 120});
        bool iconSaved = icon.save(iconSavePath);
        if (!iconSaved)
            qWarning() << __FUNCTION__ << "icon save failed";

        // save asset
        auto result = assetFilePath.copyFile(bundlePath.pathAppended(asset.fileName()));
        QTC_ASSERT_EXPECTED(result,);

        pathsInBundle.append(bundlePath.pathAppended(asset.fileName()).toFSPathString());
    }

    m_widget->userModel()->addTextures(pathsInBundle);
}

void ContentLibraryView::addLib3DComponent(const ModelNode &node)
{
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
        m_widget->userModel()->remove3DFromContentLibByName(compFileName);
    }

    // generate and save icon
    QString iconPath = QLatin1String("icons/%1").arg(UniqueName::generateId(compBaseName) + ".png");
    m_iconSavePath = bundlePath.pathAppended(iconPath);
    m_iconSavePath.parentDir().ensureWritableDir();
    getImageFromCache(compDir.pathAppended(compFileName).path(), [&](const QImage &image) {
        bool iconSaved = image.save(m_iconSavePath.toFSPathString());
        if (iconSaved)
            m_widget->userModel()->refreshSection(ContentLibraryUserModel::Items3DSectionIdx);
        else
            qWarning() << "ContentLibraryView::getImageFromCache(): icon save failed" << iconPath;
    });

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
    QJsonObject &jsonRef = m_widget->userModel()->bundleJson3DObjectRef();
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

    m_widget->userModel()->add3DItem(compBaseName, compFileName, m_iconSavePath.toUrl(), filesList);
}

void ContentLibraryView::exportLib3DComponent(const ModelNode &node)
{
    QString exportPath = getExportPath(node);
    if (exportPath.isEmpty())
        return;

    // targetPath is a temp path for collecting and zipping assets, actual export target is where
    // the user chose to export (i.e. exportPath)
    QTemporaryDir tempDir;
    QTC_ASSERT(tempDir.isValid(), return);
    auto targetPath = Utils::FilePath::fromString(tempDir.path());

    m_zipWriter = std::make_unique<ZipWriter>(exportPath);

    QString compBaseName = node.simplifiedTypeName();
    QString compFileName = compBaseName + ".qml";

    auto compDir = Utils::FilePath::fromString(ModelUtils::componentFilePath(node)).parentDir();

    QString iconPath = QLatin1String("icons/%1").arg(UniqueName::generateId(compBaseName) + ".png");

    const Utils::FilePaths sourceFiles = compDir.dirEntries({{}, QDir::Files, QDirIterator::Subdirectories});
    const QStringList ignoreList {"_importdata.json", "qmldir", compBaseName + ".hints"};
    QStringList filesList; // 3D component's assets (dependencies)

    for (const Utils::FilePath &sourcePath : sourceFiles) {
        Utils::FilePath relativePath = sourcePath.relativePathFrom(compDir);
        if (ignoreList.contains(sourcePath.fileName()) || relativePath.startsWith("source scene"))
            continue;

        m_zipWriter->addFile(relativePath.toFSPathString(), sourcePath.fileContents().value_or(""));

        if (sourcePath.fileName() != compFileName) // skip component file (only collect dependencies)
            filesList.append(relativePath.path());
    }

    // add the item to the bundle json
    QJsonObject jsonObj;
    QJsonArray itemsArr;
    itemsArr.append(QJsonObject {
        {"name", node.simplifiedTypeName()},
        {"qml", compFileName},
        {"icon", iconPath},
        {"files", QJsonArray::fromStringList(filesList)}
    });

    jsonObj["items"] = itemsArr;

    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();
    jsonObj["id"] = compUtils.user3DBundleId();

    Utils::FilePath jsonFilePath = targetPath.pathAppended(Constants::BUNDLE_JSON_FILENAME);
    m_zipWriter->addFile(jsonFilePath.fileName(), QJsonDocument(jsonObj).toJson());

    // add icon
    m_iconSavePath = targetPath.pathAppended(iconPath);
    m_iconSavePath.parentDir().ensureWritableDir();
    getImageFromCache(compDir.pathAppended(compFileName).path(), [&](const QImage &image) {
        QByteArray iconByteArray;
        QBuffer buffer(&iconByteArray);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "PNG");

        m_zipWriter->addFile("icons/" + m_iconSavePath.fileName(), iconByteArray);
        m_zipWriter->close();
    });
}

void ContentLibraryView::addLib3DItem(const ModelNode &node)
{
    auto bundlePath = Utils::FilePath::fromString(Paths::bundlesPathSetting() + "/User/3d/");

    QString name = node.variantProperty("objectName").value().toString();
    if (name.isEmpty())
        name = node.id();

    auto [qml, icon] = m_widget->userModel()->getUniqueLibItemNames(node.id(),
                                                m_widget->userModel()->bundleJson3DObjectRef());

    // generate and save Qml file
    auto [qmlString, depAssets] = modelNodeToQmlString(node);
    const QStringList depAssetsList = depAssets.values();

    auto result = bundlePath.pathAppended(qml).writeFileContents(qmlString.toUtf8());
    QTC_ASSERT_EXPECTED(result,);

    // generate and save icon
    QString qmlPath = bundlePath.pathAppended(qml).toFSPathString();
    QString iconPath = QLatin1String("icons/%1").arg(icon);
    m_iconSavePath = bundlePath.pathAppended(iconPath);
    m_iconSavePath.parentDir().ensureWritableDir();
    getImageFromCache(qmlPath, [&](const QImage &image) {
        bool iconSaved = image.save(m_iconSavePath.toFSPathString());
        if (iconSaved)
            m_widget->userModel()->refreshSection(ContentLibraryUserModel::Items3DSectionIdx);
        else
            qWarning() << "ContentLibraryView::getImageFromCache(): icon save failed" << iconPath;
    });

    // add the item to the bundle json
    QJsonObject &jsonRef = m_widget->userModel()->bundleJson3DObjectRef();
    QJsonArray itemsArr = jsonRef.value("items").toArray();
    itemsArr.append(QJsonObject {
        {"name", name},
        {"qml", qml},
        {"icon", iconPath},
        {"files", QJsonArray::fromStringList(depAssetsList)}
    });

    jsonRef["items"] = itemsArr;

    result = bundlePath.pathAppended(Constants::BUNDLE_JSON_FILENAME)
                      .writeFileContents(QJsonDocument(jsonRef).toJson());
    QTC_ASSERT_EXPECTED(result,);

    // copy item's assets to target folder
    for (const QString &assetPath : depAssetsList) {
        Utils::FilePath assetPathSource = DocumentManager::currentResourcePath().pathAppended(assetPath);
        Utils::FilePath assetPathTarget = bundlePath.pathAppended(assetPath);
        assetPathTarget.parentDir().ensureWritableDir();

        auto result = assetPathSource.copyFile(assetPathTarget);
        QTC_ASSERT_EXPECTED(result,);
    }

    m_widget->userModel()->add3DItem(name, qml, m_iconSavePath.toUrl(), depAssetsList);
}

QString ContentLibraryView::getExportPath(const ModelNode &node) const
{
    QString defaultName = node.hasId() ? node.id() : "component";
    QString defaultExportFileName = QLatin1String("%1.%2").arg(defaultName, Constants::BUNDLE_SUFFIX);
    Utils::FilePath projectFP = DocumentManager::currentProjectDirPath();
    if (projectFP.isEmpty()) {
        projectFP = QmlDesignerPlugin::instance()->documentManager()
                        .currentDesignDocument()->fileName().parentDir();
    }

    QString dialogTitle = node.metaInfo().isQtQuick3DMaterial() ? tr("Export Material")
                                                                : tr("Export Component");
    return QFileDialog::getSaveFileName(m_widget, dialogTitle,
                            projectFP.pathAppended(defaultExportFileName).toFSPathString(),
                            tr("Qt Design Studio Bundle Files (*.%1)").arg(Constants::BUNDLE_SUFFIX));
}

QString ContentLibraryView::getImportPath() const
{
    Utils::FilePath projectFP = DocumentManager::currentProjectDirPath();
    if (projectFP.isEmpty()) {
        projectFP = QmlDesignerPlugin::instance()->documentManager()
                        .currentDesignDocument()->fileName().parentDir();
    }

    return QFileDialog::getOpenFileName(m_widget, tr("Import Component"), projectFP.toFSPathString(),
                                        tr("Qt Design Studio Bundle Files (*.%1)").arg(Constants::BUNDLE_SUFFIX));
}

void ContentLibraryView::exportLib3DItem(const ModelNode &node, const QPixmap &iconPixmap)
{
    QString exportPath = getExportPath(node);
    if (exportPath.isEmpty())
        return;

    // targetPath is a temp path for collecting and zipping assets, actual export target is where
    // the user chose to export (i.e. exportPath)
    m_tempDir = std::make_unique<QTemporaryDir>();
    QTC_ASSERT(m_tempDir->isValid(), return);
    auto targetPath = Utils::FilePath::fromString(m_tempDir->path());

    m_zipWriter = std::make_unique<ZipWriter>(exportPath);

    QString name = node.variantProperty("objectName").value().toString();
    if (name.isEmpty())
        name = node.hasId() ? node.id() : "component";

    auto [qml, icon] = m_widget->userModel()->getUniqueLibItemNames(name);

    // generate and save Qml file
    auto [qmlString, depAssets] = modelNodeToQmlString(node);
    const QStringList depAssetsList = depAssets.values();

    auto qmlFilePath = targetPath.pathAppended(qml);
    auto result = qmlFilePath.writeFileContents(qmlString.toUtf8());
    QTC_ASSERT_EXPECTED(result, return);
    m_zipWriter->addFile(qmlFilePath.fileName(), qmlString.toUtf8());

    QString iconPath = QLatin1String("icons/%1").arg(icon);

    // add the item to the bundle json
    QJsonObject jsonObj;
    QJsonArray itemsArr;
    itemsArr.append(QJsonObject {
        {"name", name},
        {"qml", qml},
        {"icon", iconPath},
        {"files", QJsonArray::fromStringList(depAssetsList)}
    });

    jsonObj["items"] = itemsArr;

    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();
    jsonObj["id"] = node.metaInfo().isQtQuick3DMaterial() ? compUtils.userMaterialsBundleId()
                                                          : compUtils.user3DBundleId();

    Utils::FilePath jsonFilePath = targetPath.pathAppended(Constants::BUNDLE_JSON_FILENAME);
    m_zipWriter->addFile(jsonFilePath.fileName(), QJsonDocument(jsonObj).toJson());

    // add item's dependency assets to the bundle zip
    for (const QString &assetPath : depAssetsList) {
        Utils::FilePath assetPathSource = DocumentManager::currentResourcePath().pathAppended(assetPath);
        m_zipWriter->addFile(assetPath, assetPathSource.fileContents().value_or(""));
    }

    // add icon
    auto addIconAndCloseZip = [&] (const auto &image) { // auto: QImage or QPixmap
        QByteArray iconByteArray;
        QBuffer buffer(&iconByteArray);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "PNG");

        m_zipWriter->addFile("icons/" + m_iconSavePath.fileName(), iconByteArray);
        m_zipWriter->close();
        m_tempDir.reset();
    };

    m_iconSavePath = targetPath.pathAppended(iconPath);
    if (iconPixmap.isNull())
        getImageFromCache(qmlFilePath.toFSPathString(), addIconAndCloseZip);
    else
        addIconAndCloseZip(iconPixmap);
}

void ContentLibraryView::importBundle()
{
    QString importPath = getImportPath();
    if (importPath.isEmpty())
        return;

    ZipReader zipReader(importPath);

    QByteArray bundleJsonContent = zipReader.fileData(Constants::BUNDLE_JSON_FILENAME);
    QTC_ASSERT(!bundleJsonContent.isEmpty(), return);

    const QJsonObject importedJsonObj = QJsonDocument::fromJson(bundleJsonContent).object();
    const QJsonArray importedItemsArr = importedJsonObj.value("items").toArray();
    QTC_ASSERT(!importedItemsArr.isEmpty(), return);

    bool isMat = isMaterialBundle(importedJsonObj.value("id").toString());

    QString bundleFolderName = isMat ? QLatin1String("materials") : QLatin1String("3d");
    auto bundlePath = Utils::FilePath::fromString(QLatin1String("%1/User/%3/")
                                            .arg(Paths::bundlesPathSetting(), bundleFolderName));

    QJsonObject &jsonRef = isMat ? m_widget->userModel()->bundleJsonMaterialObjectRef()
                                 : m_widget->userModel()->bundleJson3DObjectRef();
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
            if (isMat)
                m_widget->userModel()->removeMaterialFromContentLibByName(qml);
            else
                m_widget->userModel()->remove3DFromContentLibByName(qml);
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

        if (isMat)
            m_widget->userModel()->addMaterial(name, qml, iconUrl, files);
        else
            m_widget->userModel()->add3DItem(name, qml, iconUrl, files);
    }

    zipReader.close();

    jsonRef["items"] = itemsArr;

    auto result = bundlePath.pathAppended(Constants::BUNDLE_JSON_FILENAME)
                      .writeFileContents(QJsonDocument(jsonRef).toJson());
    QTC_ASSERT_EXPECTED(result,);

    auto sectionIdx = isMat ? ContentLibraryUserModel::MaterialsSectionIdx
                            : ContentLibraryUserModel::Items3DSectionIdx;
    m_widget->userModel()->refreshSection(sectionIdx);
}

/**
 * @brief Generates an icon image from a qml component
 * @param qmlPath path to the qml component file to be rendered
 * @param iconPath output save path of the generated icon
 */
void ContentLibraryView::getImageFromCache(const QString &qmlPath,
                                           std::function<void(const QImage &image)> successCallback)
{
    m_imageCache.requestSmallImage(
        Utils::PathString{qmlPath},
        successCallback,
        [&](ImageCache::AbortReason abortReason) {
            if (abortReason == ImageCache::AbortReason::Abort) {
                qWarning() << QLatin1String("ContentLibraryView::getImageFromCache(): icon generation "
                                            "failed for path %1, reason: Abort").arg(qmlPath);
            } else if (abortReason == ImageCache::AbortReason::Failed) {
                qWarning() << QLatin1String("ContentLibraryView::getImageFromCache(): icon generation "
                                             "failed for path %1, reason: Failed").arg(qmlPath);
            } else if (abortReason == ImageCache::AbortReason::NoEntry) {
                qWarning() << QLatin1String("ContentLibraryView::getImageFromCache(): icon generation "
                                            "failed for path %1, reason: NoEntry").arg(qmlPath);
            }
        });
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

#ifdef QDS_USE_PROJECTSTORAGE
ModelNode ContentLibraryView::createMaterial(const TypeName &typeName)
{
    ModelNode matLib = Utils3D::materialLibraryNode(this);
    if (!matLib.isValid() || !typeName.size())
        return {};

    ModelNode newMatNode = createModelNode(typeName, -1, -1);
    matLib.defaultNodeListProperty().reparentHere(newMatNode);

    static QRegularExpression rgx("([A-Z])([a-z]*)");
    QString newName = QString::fromUtf8(typeName).replace(rgx, " \\1\\2").trimmed();
    if (newName.endsWith(" Material"))
        newName.chop(9); // remove trailing " Material"
    QString newId = model()->generateNewId(newName, "material");
    newMatNode.setIdWithRefactoring(newId);

    VariantProperty objNameProp = newMatNode.variantProperty("objectName");
    objNameProp.setValue(newName);

    emitCustomNotification("focus_material_section", {});

    return newMatNode;
}
#else
ModelNode ContentLibraryView::createMaterial(const NodeMetaInfo &metaInfo)
{
    ModelNode matLib = Utils3D::materialLibraryNode(this);
    if (!matLib.isValid() || !metaInfo.isValid())
        return {};

    ModelNode newMatNode = createModelNode(metaInfo.typeName(),
                                           metaInfo.majorVersion(),
                                           metaInfo.minorVersion());
    matLib.defaultNodeListProperty().reparentHere(newMatNode);

    static QRegularExpression rgx("([A-Z])([a-z]*)");
    QString newName = QString::fromLatin1(metaInfo.simplifiedTypeName()).replace(rgx, " \\1\\2").trimmed();
    if (newName.endsWith(" Material"))
        newName.chop(9); // remove trailing " Material"
    QString newId = model()->generateNewId(newName, "material");
    newMatNode.setIdWithRefactoring(newId);

    VariantProperty objNameProp = newMatNode.variantProperty("objectName");
    objNameProp.setValue(newName);

    emitCustomNotification("focus_material_section", {});

    return newMatNode;
}
#endif

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
