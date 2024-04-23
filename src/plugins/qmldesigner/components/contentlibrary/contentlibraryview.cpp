// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "contentlibraryview.h"

#include "asset.h"
#include "bindingproperty.h"
#include "contentlibrarybundleimporter.h"
#include "contentlibraryeffect.h"
#include "contentlibraryeffectsmodel.h"
#include "contentlibrarymaterial.h"
#include "contentlibrarymaterialsmodel.h"
#include "contentlibrarytexture.h"
#include "contentlibrarytexturesmodel.h"
#include "contentlibraryusermodel.h"
#include "contentlibrarywidget.h"
#include "documentmanager.h"
#include "externaldependenciesinterface.h"
#include "nodelistproperty.h"
#include "qmldesignerconstants.h"
#include "qmlobjectnode.h"
#include "variantproperty.h"
#include "utils3d.h"

#include <designerpaths.h>

#include <coreplugin/messagebox.h>
#include <enumeration.h>
#include <utils/algorithm.h>

#ifndef QMLDESIGNER_TEST
#include <projectexplorer/kit.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>
#endif

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPixmap>
#include <QVector3D>

namespace QmlDesigner {

ContentLibraryView::ContentLibraryView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView(externalDependencies)
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
        connect(m_widget, &ContentLibraryWidget::bundleEffectDragStarted, this,
                [&] (QmlDesigner::ContentLibraryEffect *eff) {
            m_draggedBundleEffect = eff;
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

        ContentLibraryMaterialsModel *materialsModel = m_widget->materialsModel().data();

        connect(materialsModel,
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

#ifdef QDS_USE_PROJECTSTORAGE
        connect(materialsModel,
                &ContentLibraryMaterialsModel::bundleMaterialImported,
                this,
                [&](const QmlDesigner::TypeName &typeName) {
                    applyBundleMaterialToDropTarget({}, typeName);
                    updateBundleMaterialsImportedState();
                });
#else
        connect(materialsModel,
                &ContentLibraryMaterialsModel::bundleMaterialImported,
                this,
                [&](const QmlDesigner::NodeMetaInfo &metaInfo) {
                    applyBundleMaterialToDropTarget({}, metaInfo);
                    updateBundleMaterialsImportedState();
                });
#endif

        connect(materialsModel, &ContentLibraryMaterialsModel::bundleMaterialAboutToUnimport, this,
                [&] (const QmlDesigner::TypeName &type) {
            // delete instances of the bundle material that is about to be unimported
            executeInTransaction("ContentLibraryView::widgetInfo", [&] {
                ModelNode matLib = Utils3D::materialLibraryNode(this);
                if (!matLib.isValid())
                    return;

                Utils::reverseForeach(matLib.directSubModelNodes(), [&](const ModelNode &mat) {
                    if (mat.isValid() && mat.type() == type)
                        QmlObjectNode(mat).destroy();
                });
            });
        });

        connect(materialsModel, &ContentLibraryMaterialsModel::bundleMaterialUnimported, this,
                &ContentLibraryView::updateBundleMaterialsImportedState);

        ContentLibraryEffectsModel *effectsModel = m_widget->effectsModel().data();

#ifdef QDS_USE_PROJECTSTORAGE
        connect(effectsModel,
                &ContentLibraryEffectsModel::bundleItemImported,
                this,
                [&](const QmlDesigner::TypeName &typeName) {
                    QTC_ASSERT(typeName.size(), return);

                    if (!m_bundleEffectTarget)
                        m_bundleEffectTarget = Utils3D::active3DSceneNode(this);

                    QTC_ASSERT(m_bundleEffectTarget, return);

                    executeInTransaction("ContentLibraryView::widgetInfo", [&] {
                        QVector3D pos = m_bundleEffectPos.value<QVector3D>();
                        ModelNode newEffNode = createModelNode(
                            typeName, -1, -1, {{"x", pos.x()}, {"y", pos.y()}, {"z", pos.z()}});
                        m_bundleEffectTarget.defaultNodeListProperty().reparentHere(newEffNode);
                        clearSelectedModelNodes();
                        selectModelNode(newEffNode);
                    });

                    updateBundleEffectsImportedState();
                    m_bundleEffectTarget = {};
                    m_bundleEffectPos = {};
                });
#else
        connect(effectsModel,
                &ContentLibraryEffectsModel::bundleItemImported,
                this,
                [&](const QmlDesigner::NodeMetaInfo &metaInfo) {
                    QTC_ASSERT(metaInfo.isValid(), return);

                    if (!m_bundleEffectTarget)
                        m_bundleEffectTarget = Utils3D::active3DSceneNode(this);

                    QTC_ASSERT(m_bundleEffectTarget, return);

                    executeInTransaction("ContentLibraryView::widgetInfo", [&] {
                        QVector3D pos = m_bundleEffectPos.value<QVector3D>();
                        ModelNode newEffNode = createModelNode(metaInfo.typeName(),
                                                               metaInfo.majorVersion(),
                                                               metaInfo.minorVersion(),
                                                               {{"x", pos.x()},
                                                                {"y", pos.y()},
                                                                {"z", pos.z()}});
                        m_bundleEffectTarget.defaultNodeListProperty().reparentHere(newEffNode);
                        clearSelectedModelNodes();
                        selectModelNode(newEffNode);
                    });

                    updateBundleEffectsImportedState();
                    m_bundleEffectTarget = {};
                    m_bundleEffectPos = {};
                });
#endif
        connect(effectsModel, &ContentLibraryEffectsModel::bundleItemAboutToUnimport, this,
                [&] (const QmlDesigner::TypeName &type) {
                    // delete instances of the bundle effect that is about to be unimported
                    executeInTransaction("ContentLibraryView::widgetInfo", [&] {
                        NodeMetaInfo metaInfo = model()->metaInfo(type);
                        QList<ModelNode> effects = allModelNodesOfType(metaInfo);
                        for (ModelNode &eff : effects)
                            eff.destroy();
                    });
                });

        connect(effectsModel, &ContentLibraryEffectsModel::bundleItemUnimported, this,
                &ContentLibraryView::updateBundleEffectsImportedState);

        connectUserBundle();
    }

    return createWidgetInfo(m_widget.data(),
                            "ContentLibrary",
                            WidgetInfo::LeftPane,
                            0,
                            tr("Content Library"));
}

void ContentLibraryView::connectUserBundle()
{
    ContentLibraryUserModel *userModel = m_widget->userModel().data();

    connect(userModel,
            &ContentLibraryUserModel::applyToSelectedTriggered,
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
                    m_widget->userModel()->addToProject(bundleMat);
            });

#ifdef QDS_USE_PROJECTSTORAGE
    connect(userModel,
            &ContentLibraryUserModel::bundleMaterialImported,
            this,
            [&](const QmlDesigner::TypeName &typeName) {
                applyBundleMaterialToDropTarget({}, typeName);
                updateBundleUserMaterialsImportedState();
            });
#else
    connect(userModel,
            &ContentLibraryUserModel::bundleMaterialImported,
            this,
            [&](const QmlDesigner::NodeMetaInfo &metaInfo) {
                applyBundleMaterialToDropTarget({}, metaInfo);
                updateBundleUserMaterialsImportedState();
            });
#endif

    connect(userModel, &ContentLibraryUserModel::bundleMaterialAboutToUnimport, this,
            [&] (const QmlDesigner::TypeName &type) {
                // delete instances of the bundle material that is about to be unimported
                executeInTransaction("ContentLibraryView::connectUserModel", [&] {
                    ModelNode matLib = Utils3D::materialLibraryNode(this);
                    if (!matLib.isValid())
                        return;

                    Utils::reverseForeach(matLib.directSubModelNodes(), [&](const ModelNode &mat) {
                        if (mat.isValid() && mat.type() == type)
                            QmlObjectNode(mat).destroy();
                    });
                });
            });

    connect(userModel, &ContentLibraryUserModel::bundleMaterialUnimported, this,
            &ContentLibraryView::updateBundleUserMaterialsImportedState);
}

void ContentLibraryView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    m_hasQuick3DImport = model->hasImport("QtQuick3D");

    updateBundlesQuick3DVersion();
    updateBundleMaterialsImportedState();

    const bool hasLibrary = Utils3D::materialLibraryNode(this).isValid();
    m_widget->setHasMaterialLibrary(hasLibrary);
    m_widget->setHasQuick3DImport(m_hasQuick3DImport);
    m_widget->setIsQt6Project(externalDependencies().isQt6Project());

    m_sceneId = Utils3D::active3DSceneId(model);

    m_widget->setHasActive3DScene(m_sceneId != -1);
    m_widget->clearSearchFilter();

    m_widget->effectsModel()->loadBundle();
    updateBundleEffectsImportedState();
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

    m_widget->materialsModel()->setHasModelSelection(!m_selectedModels.isEmpty());
    m_widget->userModel()->setHasModelSelection(!m_selectedModels.isEmpty());
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
    } else if (identifier == "drop_bundle_effect") {
        QTC_ASSERT(nodeList.size() == 1, return);

        m_bundleEffectPos = data.size() == 1 ? data.first() : QVariant();
        m_widget->effectsModel()->addInstance(m_draggedBundleEffect);
        m_bundleEffectTarget = nodeList.first() ? nodeList.first() : Utils3D::active3DSceneNode(this);
    } else if (identifier == "add_material_to_content_lib") {
        QTC_ASSERT(nodeList.size() == 1 && data.size() == 1, return);

        addLibMaterial(nodeList.first(), data.first().value<QPixmap>());
    } else if (identifier == "add_assets_to_content_lib") {
        addLibAssets(data.first().toStringList());
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
void ContentLibraryView::addLibMaterial(const ModelNode &mat, const QPixmap &icon)
{
    auto bundlePath = Utils::FilePath::fromString(Paths::bundlesPathSetting() + "/User/materials/");

    auto [name, qml] = m_widget->userModel()->getUniqueLibMaterialNameAndQml(
                                    mat.variantProperty("objectName").value().toString());

    bundlePath.pathAppended("icons").createDir();
    bundlePath.pathAppended("images").createDir();
    bundlePath.pathAppended("shaders").createDir();

    QString iconPath = QLatin1String("icons/%1.png").arg(mat.id());
    QString fullIconPath = bundlePath.pathAppended(iconPath).toString();

    // save icon
    bool iconSaved = icon.save(fullIconPath);
    if (!iconSaved)
        qWarning() << __FUNCTION__ << "icon save failed";

    // generate and save material Qml file
    const QStringList depAssets = writeLibMaterialQml(mat, qml);

    // add the material to the bundle json
    QJsonObject &jsonRef = m_widget->userModel()->bundleJsonObjectRef();
    QJsonObject matsObj = jsonRef.value("materials").toObject();
    QJsonObject matObj;
    matObj.insert("qml", qml);
    matObj.insert("icon", iconPath);
    QJsonArray filesArr;
    for (const QString &assetPath : depAssets)
        filesArr.append(assetPath);
    matObj.insert("files", filesArr);

    matsObj.insert(name, matObj);
    jsonRef.insert("materials", matsObj);
    auto result = bundlePath.pathAppended("user_materials_bundle.json")
                      .writeFileContents(QJsonDocument(jsonRef).toJson());
    if (!result)
        qWarning() << __FUNCTION__ << result.error();

    // copy material assets to bundle folder
    for (const QString &assetPath : depAssets) {
        Asset asset(assetPath);
        QString subDir;
        if (asset.isImage())
            subDir = "images";
        else if (asset.isShader())
            subDir = "shaders";

        Utils::FilePath assetPathSource = DocumentManager::currentResourcePath().pathAppended(assetPath);
        Utils::FilePath assetPathTarget = bundlePath.pathAppended(QString("%1/%2")
                                                            .arg(subDir, "/" + asset.fileName()));

        auto result = assetPathSource.copyFile(assetPathTarget);
        if (!result)
            qWarning() << __FUNCTION__ << result.error();
    }

    m_widget->userModel()->addMaterial(name, qml, QUrl::fromLocalFile(fullIconPath), depAssets);
}

QStringList ContentLibraryView::writeLibMaterialQml(const ModelNode &mat, const QString &qml)
{
    QStringList depListIds;
    auto [qmlString, assets] = modelNodeToQmlString(mat, depListIds);

    qmlString.prepend("import QtQuick\nimport QtQuick3D\n\n");

    auto qmlPath = Utils::FilePath::fromString(Paths::bundlesPathSetting() + "/User/materials/" + qml);
    auto result = qmlPath.writeFileContents(qmlString.toUtf8());
    if (!result)
        qWarning() << __FUNCTION__ << result.error();

    return assets.values();
}

QPair<QString, QSet<QString>> ContentLibraryView::modelNodeToQmlString(const ModelNode &node,
                                                                       QStringList &depListIds,
                                                                       int depth)
{
    QString qml;
    QSet<QString> assets;

    QString indent = QString(" ").repeated(depth * 4);

    qml += indent + node.simplifiedTypeName() + " {\n";

    indent = QString(" ").repeated((depth + 1) * 4);

    qml += indent + "id: " + (depth == 0 ? "root" : node.id()) + " \n\n";

    const QList<AbstractProperty> matProps = node.properties();
    for (const AbstractProperty &p : matProps) {
        if (p.isVariantProperty()) {
            QVariant pValue = p.toVariantProperty().value();
            QString val;
            if (strcmp(pValue.typeName(), "QString") == 0 || strcmp(pValue.typeName(), "QColor") == 0) {
                val = QLatin1String("\"%1\"").arg(pValue.toString());
            } else if (strcmp(pValue.typeName(), "QUrl") == 0) {
                val = QLatin1String("\"%1\"").arg(pValue.toString());
                assets.insert(pValue.toString());
            } else if (strcmp(pValue.typeName(), "QmlDesigner::Enumeration") == 0) {
                val = pValue.value<QmlDesigner::Enumeration>().toString();
            } else {
                val = pValue.toString();
            }

            qml += indent + p.name() + ": " + val + "\n";
        } else if (p.isBindingProperty()) {
            qml += indent + p.name() + ": " + p.toBindingProperty().expression() + "\n";

            ModelNode depNode = modelNodeForId(p.toBindingProperty().expression());

            if (depNode && !depListIds.contains(depNode.id())) {
                depListIds.append(depNode.id());
                auto [depQml, depAssets] = modelNodeToQmlString(depNode, depListIds, depth + 1);
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

    for (const QString &path : paths) {
        Asset asset(path);
        auto assetPath = Utils::FilePath::fromString(path);

        // save icon
        QString iconSavePath = bundlePath.pathAppended("icons/" + assetPath.baseName() + ".png").toString();
        QPixmap icon = asset.pixmap({120, 120});
        bool iconSaved = icon.save(iconSavePath);
        if (!iconSaved)
            qWarning() << __FUNCTION__ << "icon save failed";

        // save asset
        auto result = assetPath.copyFile(bundlePath.pathAppended(asset.fileName()));
        if (!result)
            qWarning() << __FUNCTION__ << result.error();

        pathsInBundle.append(bundlePath.pathAppended(asset.fileName()).toString());
    }

    m_widget->userModel()->addTextures(pathsInBundle);
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
    QString newId = model()->generateIdFromName(newName, "material");
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
    QString newId = model()->generateIdFromName(newName, "material");
    newMatNode.setIdWithRefactoring(newId);

    VariantProperty objNameProp = newMatNode.variantProperty("objectName");
    objNameProp.setValue(newName);

    emitCustomNotification("focus_material_section", {});

    return newMatNode;
}
#endif

void ContentLibraryView::updateBundleMaterialsImportedState()
{
    using namespace Utils;

    if (!m_widget->materialsModel()->bundleImporter())
        return;

    QStringList importedBundleMats;

    FilePath materialBundlePath = m_widget->materialsModel()->bundleImporter()->resolveBundleImportPath();

    if (materialBundlePath.exists()) {
        importedBundleMats = transform(materialBundlePath.dirEntries({{"*.qml"}, QDir::Files}),
                                       [](const FilePath &f) { return f.fileName().chopped(4); });
    }

    m_widget->materialsModel()->updateImportedState(importedBundleMats);
}

void ContentLibraryView::updateBundleUserMaterialsImportedState()
{
    using namespace Utils;

    if (!m_widget->userModel()->bundleImporter())
        return;

    QStringList importedBundleMats;

    FilePath bundlePath = m_widget->userModel()->bundleImporter()->resolveBundleImportPath();

    if (bundlePath.exists()) {
        importedBundleMats = transform(bundlePath.dirEntries({{"*.qml"}, QDir::Files}),
                                       [](const FilePath &f) { return f.fileName().chopped(4); });
    }

    m_widget->userModel()->updateImportedState(importedBundleMats);
}

void ContentLibraryView::updateBundleEffectsImportedState()
{
    using namespace Utils;

    if (!m_widget->effectsModel()->bundleImporter())
        return;

    QStringList importedBundleEffs;

    FilePath bundlePath = m_widget->effectsModel()->bundleImporter()->resolveBundleImportPath();

    if (bundlePath.exists()) {
        importedBundleEffs = transform(bundlePath.dirEntries({{"*.qml"}, QDir::Files}),
                                       [](const FilePath &f) { return f.fileName().chopped(4); });
    }

    m_widget->effectsModel()->updateImportedState(importedBundleEffs);
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
}

} // namespace QmlDesigner
