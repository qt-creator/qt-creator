// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bundlehelper.h"

#include "bundleimporter.h"
#include "utils3d.h"

#include <abstractview.h>
#include <asynchronousimagecache.h>
#include <modelutils.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <uniquename.h>
#include <variantproperty.h>

#include <coreplugin/icore.h>

#include <solutions/zip/zipreader.h>
#include <solutions/zip/zipwriter.h>

#include <utils/qtcassert.h>

#include <QBuffer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QTemporaryDir>
#include <QWidget>

namespace QmlDesigner {

BundleHelper::BundleHelper(AbstractView *view, QWidget *widget)
    : m_view(view)
    , m_widget(widget)
{
    createImporter();
}

BundleHelper::~BundleHelper()
{}

void BundleHelper::createImporter()
{
    m_importer = Utils::makeUniqueObjectPtr<BundleImporter>();

#ifdef QDS_USE_PROJECTSTORAGE
    QObject::connect(
        m_importer.get(),
        &BundleImporter::importFinished,
        m_widget,
        [&](const QmlDesigner::TypeName &typeName, const QString &bundleId) {
            QTC_ASSERT(typeName.size(), return);
            if (isMaterialBundle(bundleId)) {
                m_view->executeInTransaction("BundleHelper::createImporter", [&] {
                    Utils3D::createMaterial(m_view, typeName);
                });
            } else if (isItemBundle(bundleId)) {
                ModelNode target = Utils3D::active3DSceneNode(m_view);
                if (!target)
                    target = m_view->rootModelNode();
                QTC_ASSERT(target, return);

                m_view->executeInTransaction("BundleHelper::createImporter", [&] {
                    ModelNode newNode = m_view->createModelNode(typeName, -1, -1);
                    target.defaultNodeListProperty().reparentHere(newNode);
                    newNode.setIdWithoutRefactoring(m_view->model()->generateNewId(
                        newNode.simplifiedTypeName(), "node"));
                    m_view->clearSelectedModelNodes();
                    m_view->selectModelNode(newNode);
                });
            }
        });
#else
    QObject::connect(m_importer.get(), &BundleImporter::importFinished, m_widget,
        [&](const QmlDesigner::NodeMetaInfo &metaInfo, const QString &bundleId) {
            QTC_ASSERT(metaInfo.isValid(), return);
            if (isMaterialBundle(bundleId)) {
                m_view->executeInTransaction("BundleHelper::createImporter", [&] {
                    Utils3D::createMaterial(m_view, metaInfo);
                });
            } else if (isItemBundle(bundleId)) {

                ModelNode target = Utils3D::active3DSceneNode(m_view);
                if (!target)
                    target = m_view->rootModelNode();
                QTC_ASSERT(target, return);

                m_view->executeInTransaction("BundleHelper::createImporter", [&] {
                    ModelNode newNode = m_view->createModelNode(metaInfo.typeName(),
                                                                metaInfo.majorVersion(),
                                                                metaInfo.minorVersion());
                    target.defaultNodeListProperty().reparentHere(newNode);
                    newNode.setIdWithoutRefactoring(m_view->model()->generateNewId(
                        newNode.simplifiedTypeName(), "node"));
                    m_view->clearSelectedModelNodes();
                    m_view->selectModelNode(newNode);
                });
            }
        });
#endif
}

void BundleHelper::importBundleToProject()
{
    QString importPath = getImportPath();
    if (importPath.isEmpty())
        return;

    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();

    ZipReader zipReader(importPath);

    QByteArray bundleJsonContent = zipReader.fileData(Constants::BUNDLE_JSON_FILENAME);
    QTC_ASSERT(!bundleJsonContent.isEmpty(), return);

    const QJsonObject importedJsonObj = QJsonDocument::fromJson(bundleJsonContent).object();
    const QJsonArray importedItemsArr = importedJsonObj.value("items").toArray();
    QTC_ASSERT(!importedItemsArr.isEmpty(), return);

    QString bundleVersion = importedJsonObj.value("version").toString();
    bool bundleVersionOk = !bundleVersion.isEmpty() && bundleVersion == BUNDLE_VERSION;
    if (!bundleVersionOk) {
        QMessageBox::warning(m_widget, QObject::tr("Unsupported bundle file"),
                             QObject::tr("The chosen bundle was created with an incompatible version"
                                         " of Qt Design Studio"));
        return;
    }
    QString bundleId = importedJsonObj.value("id").toString();

    QTemporaryDir tempDir;
    QTC_ASSERT(tempDir.isValid(), return);
    auto bundlePath = Utils::FilePath::fromString(tempDir.path());

    const QStringList existingQmls = Utils::transform(compUtils.userBundlePath(bundleId)
                                                          .dirEntries(QDir::Files), [](const Utils::FilePath &path) {
                                                          return path.fileName();
                                                      });

    for (const QJsonValueConstRef &itemRef : importedItemsArr) {
        QJsonObject itemObj = itemRef.toObject();
        QString qml = itemObj.value("qml").toString();

        // confirm overwrite if an item with same name exists
        if (existingQmls.contains(qml)) {
            auto reply = QMessageBox::question(m_widget, QObject::tr("Component Exists"),
                                               QObject::tr("A component with the same name '%1' "
                                                           "already exists in the project, are you "
                                                           "sure you want to overwrite it?")
                                                   .arg(qml), QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::No)
                continue;

            // TODO: before overwriting remove old item's dependencies (not harmful but for cleanup)
        }

        // add entry to model
        QStringList files = itemObj.value("files").toVariant().toStringList();
        QString icon = itemObj.value("icon").toString();

        // copy files
        QStringList allFiles = files;
        allFiles << qml << icon;
        for (const QString &file : std::as_const(allFiles)) {
            Utils::FilePath filePath = bundlePath.pathAppended(file);
            filePath.parentDir().ensureWritableDir();
            QTC_ASSERT_EXPECTED(filePath.writeFileContents(zipReader.fileData(file)),);
        }

        QString typePrefix = compUtils.userBundleType(bundleId);
        TypeName type = QLatin1String("%1.%2").arg(typePrefix, qml.chopped(4)).toLatin1();

        QString err = m_importer->importComponent(bundlePath.toFSPathString(), type, qml, files);

        if (!err.isEmpty())
            qWarning() << __FUNCTION__ << err;
    }

    zipReader.close();
}

void BundleHelper::exportBundle(const ModelNode &node, const QPixmap &iconPixmap)
{
    if (node.isComponent())
        export3DComponent(node);
    else
        exportItem(node, iconPixmap);
}

void BundleHelper::export3DComponent(const ModelNode &node)
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
    jsonObj["version"] = BUNDLE_VERSION;

    Utils::FilePath jsonFilePath = targetPath.pathAppended(Constants::BUNDLE_JSON_FILENAME);
    m_zipWriter->addFile(jsonFilePath.fileName(), QJsonDocument(jsonObj).toJson());

    // add icon
    m_iconSavePath = targetPath.pathAppended(iconPath);
    m_iconSavePath.parentDir().ensureWritableDir();
    getImageFromCache(compDir.pathAppended(compFileName).path(), [&](const QImage &image) {
        addIconAndCloseZip(image);
    });
}

void BundleHelper::exportItem(const ModelNode &node, const QPixmap &iconPixmap)
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
        name = node.displayName();

    QString qml = nodeNameToComponentFileName(name);
    QString iconBaseName = UniqueName::generateId(name);

    // generate and save Qml file
    auto [qmlString, depAssets] = modelNodeToQmlString(node);
    const QList<AssetPath> depAssetsList = depAssets.values();

    QStringList depAssetsRelativePaths;
    for (const AssetPath &assetPath : depAssetsList)
        depAssetsRelativePaths.append(assetPath.relativePath);

    auto qmlFilePath = targetPath.pathAppended(qml);
    auto result = qmlFilePath.writeFileContents(qmlString.toUtf8());
    QTC_ASSERT_EXPECTED(result, return);
    m_zipWriter->addFile(qmlFilePath.fileName(), qmlString.toUtf8());

    QString iconPath = QLatin1String("icons/%1.png").arg(iconBaseName);

    // add the item to the bundle json
    QJsonObject jsonObj;
    QJsonArray itemsArr;
    itemsArr.append(QJsonObject {
        {"name", name},
        {"qml", qml},
        {"icon", iconPath},
        {"files", QJsonArray::fromStringList(depAssetsRelativePaths)}
    });

    jsonObj["items"] = itemsArr;

    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();
    jsonObj["id"] = node.metaInfo().isQtQuick3DMaterial() ? compUtils.userMaterialsBundleId()
                                                          : compUtils.user3DBundleId();
    jsonObj["version"] = BUNDLE_VERSION;

    Utils::FilePath jsonFilePath = targetPath.pathAppended(Constants::BUNDLE_JSON_FILENAME);
    m_zipWriter->addFile(jsonFilePath.fileName(), QJsonDocument(jsonObj).toJson());

    // add item's dependency assets to the bundle zip
    for (const AssetPath &assetPath : depAssetsList)
        m_zipWriter->addFile(assetPath.relativePath, assetPath.absFilPath().fileContents().value_or(""));

    // add icon
    QPixmap iconPixmapToSave;

    if (node.metaInfo().isQtQuick3DCamera()) {
        iconPixmapToSave = Core::ICore::resourcePath("qmldesigner/contentLibraryImages/camera.png")
                               .toFSPathString();
    } else if (node.metaInfo().isQtQuick3DLight()) {
        iconPixmapToSave = Core::ICore::resourcePath("qmldesigner/contentLibraryImages/light.png")
                               .toFSPathString();
    } else {
        iconPixmapToSave = iconPixmap;
    }

    m_iconSavePath = targetPath.pathAppended(iconPath);
    if (iconPixmapToSave.isNull()) {
        getImageFromCache(qmlFilePath.toFSPathString(), [&](const QImage &image) {
            addIconAndCloseZip(image);
        });
    } else {
        addIconAndCloseZip(iconPixmapToSave);
    }
}

QPair<QString, QSet<AssetPath>> BundleHelper::modelNodeToQmlString(const ModelNode &node, int depth)
{
    static QStringList depListIds;

    QString qml;
    QSet<AssetPath> assets;

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
                if (!pValueStr.startsWith("#")) {
                    assets.insert({DocumentManager::currentResourcePath().toFSPathString(),
                                   pValue.toString()});
                }
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
            ModelNode depNode = m_view->modelNodeForId(p.toBindingProperty().expression());
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

    // add child nodes
    const ModelNodes nodeChildren = node.directSubModelNodes();
    for (const ModelNode &childNode : nodeChildren) {
        if (childNode && !depListIds.contains(childNode.id())) {
            depListIds.append(childNode.id());
            auto [depQml, depAssets] = modelNodeToQmlString(childNode, depth + 1);
            qml += "\n" + depQml + "\n";
            assets.unite(depAssets);
        }
    }

    indent = QString(" ").repeated(depth * 4);

    qml += indent + "}\n";

    if (node.isComponent()) {
        auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();
        bool isBundle = node.type().startsWith(compUtils.componentBundlesTypePrefix().toLatin1());

        if (depth > 0) {
            // add component file to the dependency assets
            Utils::FilePath compFilePath = componentPath(node.metaInfo());
            assets.insert({compFilePath.parentDir().path(), compFilePath.fileName()});
        }

        if (isBundle)
            assets.unite(getBundleComponentDependencies(node));
    }

    return {qml, assets};
}

QSet<AssetPath> BundleHelper::getBundleComponentDependencies(const ModelNode &node) const
{
    const QString compFileName = node.simplifiedTypeName() + ".qml";

    Utils::FilePath compPath = componentPath(node.metaInfo()).parentDir();

    QTC_ASSERT(compPath.exists(), return {});

    QSet<AssetPath> depList;

    Utils::FilePath assetRefPath = compPath.pathAppended(Constants::COMPONENT_BUNDLES_ASSET_REF_FILE);

    Utils::expected_str<QByteArray> assetRefContents = assetRefPath.fileContents();
    if (!assetRefContents.has_value()) {
        qWarning() << __FUNCTION__ << assetRefContents.error();
        return {};
    }

    QJsonDocument jsonDoc = QJsonDocument::fromJson(assetRefContents.value());
    if (jsonDoc.isNull()) {
        qWarning() << __FUNCTION__ << "Invalid json file" << assetRefPath;
        return {};
    }

    const QJsonObject rootObj = jsonDoc.object();
    const QStringList bundleAssets = rootObj.keys();

    for (const QString &asset : bundleAssets) {
        if (rootObj.value(asset).toArray().contains(compFileName))
            depList.insert({compPath.toFSPathString(), asset});
    }

    return depList;
}

Utils::FilePath BundleHelper::componentPath([[maybe_unused]] const NodeMetaInfo &metaInfo) const
{
#ifdef QDS_USE_PROJECTSTORAGE
    // TODO
    return {};
#else
    return Utils::FilePath::fromString(metaInfo.componentFileName());
#endif
}

QString BundleHelper::nodeNameToComponentFileName(const QString &name) const
{
    QString fileName = UniqueName::generateId(name, "Component");
    fileName[0] = fileName.at(0).toUpper();
    fileName.prepend("My");

    return fileName + ".qml";
}

/**
 * @brief Generates an icon image from a qml component
 * @param qmlPath path to the qml component file to be rendered
 * @param iconPath output save path of the generated icon
 */
void BundleHelper::getImageFromCache(const QString &qmlPath,
                                     std::function<void(const QImage &image)> successCallback)
{
    QmlDesignerPlugin::imageCache().requestSmallImage(
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

void BundleHelper::addIconAndCloseZip(const auto &image) { // auto: QImage or QPixmap
    QByteArray iconByteArray;
    QBuffer buffer(&iconByteArray);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");

    m_zipWriter->addFile("icons/" + m_iconSavePath.fileName(), iconByteArray);
    m_zipWriter->close();
};

QString BundleHelper::getImportPath() const
{
    Utils::FilePath projectFP = DocumentManager::currentProjectDirPath();
    if (projectFP.isEmpty()) {
        projectFP = QmlDesignerPlugin::instance()->documentManager()
                        .currentDesignDocument()->fileName().parentDir();
    }

    return QFileDialog::getOpenFileName(m_widget, QObject::tr("Import Component"),
                                        projectFP.toFSPathString(),
                                        QObject::tr("Qt Design Studio Bundle Files (*.%1)")
                                            .arg(Constants::BUNDLE_SUFFIX));
}

QString BundleHelper::getExportPath(const ModelNode &node) const
{
    QString defaultExportFileName = QLatin1String("%1.%2").arg(node.displayName(),
                                                               Constants::BUNDLE_SUFFIX);
    Utils::FilePath projectFP = DocumentManager::currentProjectDirPath();
    if (projectFP.isEmpty()) {
        projectFP = QmlDesignerPlugin::instance()->documentManager()
                        .currentDesignDocument()->fileName().parentDir();
    }

    QString dialogTitle = node.metaInfo().isQtQuick3DMaterial() ? QObject::tr("Export Material")
                                                                : QObject::tr("Export Component");
    return QFileDialog::getSaveFileName(m_widget, dialogTitle,
                                        projectFP.pathAppended(defaultExportFileName).toFSPathString(),
                                        QObject::tr("Qt Design Studio Bundle Files (*.%1)")
                                            .arg(Constants::BUNDLE_SUFFIX));
}

bool BundleHelper::isMaterialBundle(const QString &bundleId) const
{
    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();
    return bundleId == compUtils.materialsBundleId() || bundleId == compUtils.userMaterialsBundleId();
}

// item bundle includes effects and 3D components
bool BundleHelper::isItemBundle(const QString &bundleId) const
{
    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();
    return bundleId == compUtils.effectsBundleId() || bundleId == compUtils.userEffectsBundleId()
           || bundleId == compUtils.user3DBundleId();
}

} // namespace QmlDesigner
