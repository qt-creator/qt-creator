// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bundlehelper.h"

#include "bundleimporter.h"
#include "componentcoretracing.h"
#include "utils3d.h"

#include <abstractview.h>
#include <asynchronousimagecache.h>
#include <modelutils.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <qmldesignertr.h>
#include <uniquename.h>
#include <variantproperty.h>

#include <coreplugin/icore.h>

#include <utils/qtcassert.h>
#include <utils/fileutils.h>

#include <QBuffer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QTemporaryDir>
#include <QWidget>

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
#  include <QtCore/private/qzipreader_p.h>
#  include <QtCore/private/qzipwriter_p.h>
#else
#  include <QtGui/private/qzipreader_p.h>
#  include <QtGui/private/qzipwriter_p.h>
#endif
namespace QmlDesigner {

using NanotraceHR::keyValue;
using QmlDesigner::ComponentCoreTracing::category;

Utils::FilePath AssetPath::absFilPath() const
{
    NanotraceHR::Tracer tracer{"asset path abs file path", category()};

    return basePath.pathAppended(relativePath);
}

QByteArray AssetPath::fileContent() const
{
    NanotraceHR::Tracer tracer{"asset path file content", category()};

    return absFilPath().fileContents().value_or("");
}

BundleHelper::BundleHelper(AbstractView *view, QWidget *widget)
    : m_view(view)
    , m_widget(widget)
{
    NanotraceHR::Tracer tracer{"bundle helper constructor", category()};

    createImporter();
}

BundleHelper::~BundleHelper()
{
    NanotraceHR::Tracer tracer{"bundle helper destructor", category()};
}

void BundleHelper::createImporter()
{
    NanotraceHR::Tracer tracer{"bundle helper create importer", category()};

    m_importer = Utils::makeUniqueObjectPtr<BundleImporter>();

    QObject::connect(
        m_importer.get(),
        &BundleImporter::importFinished,
        m_view,
        [&](const QmlDesigner::TypeName &typeName, const QString &bundleId, bool typeAdded) {
            QTC_ASSERT(typeName.size(), return);
            if (isMaterialBundle(bundleId)) {
                m_view->executeInTransaction("BundleHelper::createImporter", [&] {
                    Utils3D::createMaterial(m_view, typeName);
                    if (typeAdded)
                        m_view->resetPuppet();
                });
            } else if (isItemBundle(bundleId)) {
                ModelNode target = Utils3D::active3DSceneNode(m_view);
                if (!target)
                    target = m_view->rootModelNode();
                QTC_ASSERT(target, return);

                m_view->executeInTransaction("BundleHelper::createImporter", [&] {
                    ModelNode newNode = m_view->createModelNode(typeName);
                    target.defaultNodeListProperty().reparentHere(newNode);
                    newNode.setIdWithoutRefactoring(
                        m_view->model()->generateNewId(newNode.simplifiedDocumentTypeRepresentation(),
                                                       "node"));
                    m_view->clearSelectedModelNodes();
                    m_view->selectModelNode(newNode);
                    if (typeAdded)
                        m_view->resetPuppet();
                });
            }
        });
}

void BundleHelper::importBundleToProject()
{
    NanotraceHR::Tracer tracer{"bundle helper import bundle to project", category()};

    QString importPath = getImportPath();
    if (importPath.isEmpty())
        return;

    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();

    QZipReader zipReader(importPath);

    QByteArray bundleJsonContent = zipReader.fileData(Constants::BUNDLE_JSON_FILENAME);
    QTC_ASSERT(!bundleJsonContent.isEmpty(), return);

    const QJsonObject importedJsonObj = QJsonDocument::fromJson(bundleJsonContent).object();
    const QJsonArray importedItemsArr = importedJsonObj.value("items").toArray();
    QTC_ASSERT(!importedItemsArr.isEmpty(), return);

    QString bundleVersion = importedJsonObj.value("version").toString();
    bool bundleVersionOk = !bundleVersion.isEmpty() && bundleVersion == BUNDLE_VERSION;
    if (!bundleVersionOk) {
        QMessageBox::warning(m_widget,
                             Tr::tr("Unsupported Bundle File"),
                             Tr::tr("The chosen bundle was created with an incompatible version"
                                    " of Qt Design Studio."));
        return;
    }

    QString bundleId = importedJsonObj.value("id").toString();

    bool hasQuick3DImport = m_view->model()->hasImport("QtQuick3D");

    if (!hasQuick3DImport) {
        Import import = Import::createLibraryImport("QtQuick3D");
        m_view->model()->changeImports({import}, {});
    }

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
            auto reply = QMessageBox::question(m_widget,
                                               Tr::tr("Component Exists"),
                                               Tr::tr("A component with the same name '%1' "
                                                      "already exists in the project, are you "
                                                      "sure you want to overwrite it?")
                                                   .arg(qml),
                                               QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::No)
                continue;

            // TODO: before overwriting remove old item's dependencies (not harmful but for cleanup)
        }

        QStringList files = itemObj.value("files").toVariant().toStringList();
        QString icon = itemObj.value("icon").toString();

        // copy files
        QStringList allFiles = files;
        allFiles << qml << icon;
        for (const QString &file : std::as_const(allFiles)) {
            Utils::FilePath filePath = bundlePath.pathAppended(file);
            filePath.parentDir().ensureWritableDir();
            QTC_ASSERT_RESULT(filePath.writeFileContents(zipReader.fileData(file)),);
        }

        QString typePrefix = compUtils.userBundleType(bundleId);
        TypeName type = QLatin1String("%1.%2").arg(typePrefix, qml.section('.', 0, 0)).toLatin1();

        QString err = m_importer->importComponent(bundlePath.toFSPathString(), type, qml, files);

        if (!err.isEmpty())
            qWarning() << __FUNCTION__ << err;
    }

    zipReader.close();
}

void BundleHelper::exportBundle(const QList<ModelNode> &nodes, const QPixmap &iconPixmap)
{
    NanotraceHR::Tracer tracer{"bundle helper export bundle", category()};

    QTC_ASSERT(!nodes.isEmpty(), return);

    QString exportPath = getExportPath(nodes.at(0));
    if (exportPath.isEmpty())
        return;

    m_zipWriter = std::make_unique<QZipWriter>(exportPath);

    m_tempDir = std::make_unique<QTemporaryDir>();
    QTC_ASSERT(m_tempDir->isValid(), return);

    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();
    QJsonObject jsonObj;
    jsonObj["version"] = BUNDLE_VERSION;
    QJsonArray itemsArr;

    // remove nested nodes (they will be exported anyway as dependency of the parent)
    QList<ModelNode> nodesToExport;
    for (const ModelNode &node : nodes) {
        bool isChild = std::ranges::any_of(nodes, [&](const ModelNode &possibleParent) {
            return &node != &possibleParent && possibleParent.isAncestorOf(node);
        });

        if (!isChild)
            nodesToExport.append(node);
    }
    jsonObj["id"] = !nodesToExport.isEmpty() && nodesToExport[0].metaInfo().isQtQuick3DMaterial()
                        ? compUtils.userMaterialsBundleId()
                        : compUtils.user3DBundleId();

    m_remainingFiles = nodesToExport.size() + 1;

    for (const ModelNode &node : std::as_const(nodesToExport)) {
        if (isProjectComponent(node))
            itemsArr.append(exportComponent(node));
        else
            itemsArr.append(exportNode(node, iconPixmap));
    }

    jsonObj["items"] = itemsArr;
    m_zipWriter->addFile(Constants::BUNDLE_JSON_FILENAME, QJsonDocument(jsonObj).toJson());
    maybeCloseZip();
}

QJsonObject BundleHelper::exportComponent(const ModelNode &node)
{
    NanotraceHR::Tracer tracer{"bundle helper export component", category()};

    Utils::FilePath compFilePath = Utils::FilePath::fromString(ModelUtils::componentFilePath(node));
    Utils::FilePath compDir = compFilePath.parentDir();
    QString compBaseName = compFilePath.completeBaseName();
    QString compFileName = compFilePath.fileName();

    QString iconPath = QLatin1String("icons/%1").arg(UniqueName::generateId(compBaseName) + ".png");

    const QSet<AssetPath> compDependencies = getComponentDependencies(compFilePath, compDir);

    QStringList filesList;
    for (const AssetPath &asset : compDependencies) {
        Utils::FilePath assetAbsPath = asset.absFilPath();
        QByteArray assetContent = asset.fileContent();

        // remove imports of sub components
        for (const QString &import : std::as_const(asset.importsToRemove)) {
            int removeIdx = assetContent.indexOf(QByteArray("import " + import.toLatin1()));
            int removeLen = assetContent.indexOf('\n', removeIdx) - removeIdx;
            assetContent.remove(removeIdx, removeLen);
        }

        m_zipWriter->addFile(asset.relativePath, assetContent);

        if (assetAbsPath.fileName() != compFileName) // skip component file (only collect dependencies)
            filesList.append(asset.relativePath);
    }

    // add icon
    QString filePath = compFilePath.path();
    getImageFromCache(filePath, [this, iconPath](const QImage &image) {
        addIconToZip(iconPath, image);
    });

    return {{"name", node.simplifiedDocumentTypeRepresentation()},
            {"qml", compFileName},
            {"icon", iconPath},
            {"files", QJsonArray::fromStringList(filesList)}};
}

QJsonObject BundleHelper::exportNode(const ModelNode &node, const QPixmap &iconPixmap)
{
    NanotraceHR::Tracer tracer{"bundle helper export node", category()};

    // tempPath is a temp path for collecting and zipping assets, actual export target is where
    // the user chose to export (i.e. exportPath)
    auto tempPath = Utils::FilePath::fromString(m_tempDir->path());

    QString name = node.variantProperty("objectName").value().toString();
    if (name.isEmpty())
        name = node.displayName();

    QString qml = nodeNameToComponentFileName(name);
    QString iconBaseName = UniqueName::generateId(name);
    QString iconPath = QLatin1String("icons/%1.png").arg(iconBaseName);

    // generate and save Qml file
    auto [qmlString, depAssets] = modelNodeToQmlString(node);
    const QList<AssetPath> depAssetsList = depAssets.values();

    QStringList depAssetsRelativePaths;
    for (const AssetPath &assetPath : depAssetsList)
        depAssetsRelativePaths.append(assetPath.relativePath);

    auto qmlFilePath = tempPath.pathAppended(qml);
    auto result = qmlFilePath.writeFileContents(qmlString.toUtf8());
    QTC_ASSERT_RESULT(result, return {});
    m_zipWriter->addFile(qmlFilePath.fileName(), qmlString.toUtf8());

    // add item's dependency assets to the bundle zip and target path (for icon generation)
    for (const AssetPath &assetPath : depAssetsList) {
        QByteArray assetContent = assetPath.fileContent();
        m_zipWriter->addFile(assetPath.relativePath, assetContent);

        Utils::FilePath assetTargetPath = tempPath.pathAppended(assetPath.relativePath);
        assetTargetPath.parentDir().ensureWritableDir();
        assetTargetPath.writeFileContents(assetContent);
    }

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

    if (iconPixmapToSave.isNull()) {
        getImageFromCache(qmlFilePath.toFSPathString(), [this, iconPath](const QImage &image) {
            addIconToZip(iconPath, image);
        });
    } else {
        addIconToZip(iconPath, iconPixmapToSave);
    }

    return {
        {"name", name},
        {"qml", qml},
        {"icon", iconPath},
        {"files", QJsonArray::fromStringList(depAssetsRelativePaths)}
    };
}

void BundleHelper::maybeCloseZip()
{
    NanotraceHR::Tracer tracer{"bundle helper maybe close zip", category()};

    if (--m_remainingFiles <= 0)
        m_zipWriter->close();
}

static QString nodeModuleName(const ModelNode &node, Model *model)
{
    using Storage::Info::ExportedTypeName;
    using Storage::Module;

    ExportedTypeName exportedName = node.exportedTypeName();
    if (exportedName.moduleId) {
        Module moduleStorageModule = model->projectStorageDependencies().modulesStorage.module(
            exportedName.moduleId);
        return moduleStorageModule.name.toQString();
    }

    return QString::fromUtf8(node.documentTypeRepresentation())
        .left(node.documentTypeRepresentation().lastIndexOf('.'));
}

static Storage::Info::ExportedTypeNames getTypeNamesForNode(const ModelNode &node)
{
    using Storage::Info::ExportedTypeName;
    using Storage::Info::ExportedTypeNames;

    const ModelNodes &allNodes = node.allSubModelNodesAndThisNode();
    ExportedTypeNames typeNames;
    typeNames.reserve(Utils::usize(allNodes));

    for (const ModelNode &node : allNodes) {
        ExportedTypeName exportedName = node.exportedTypeName();
        if (exportedName.moduleId)
            typeNames.push_back(exportedName);
    }

    std::sort(typeNames.begin(), typeNames.end());
    typeNames.erase(std::unique(typeNames.begin(), typeNames.end()), typeNames.end());

    return typeNames;
}

static QString getRelativePath(const std::string_view path, const Model *model)
{
    std::filesystem::path filePath{path};
    std::filesystem::path modelPath(std::u16string_view(model->fileUrl().toLocalFile()));
    return QString{filePath.lexically_relative(modelPath).generic_u16string()};
}

static Imports getRequiredImports(const ModelNode &node,
                                  const ModulesStorage &modulesStorage,
                                  Model *model)
{
    using Storage::Info::ExportedTypeName;
    using Storage::Info::ExportedTypeNames;
    using Storage::Module;
    using Storage::ModuleKind;

    auto createImport = [model](const Module &module) -> Import {
        switch (module.kind) {
        case ModuleKind::QmlLibrary:
            return Import::createLibraryImport(module.name.toQString());
        case ModuleKind::PathLibrary:
            return Import::createFileImport(getRelativePath(module.name, model));
        default:
            return {};
        };
    };

    ExportedTypeNames typeNames = getTypeNamesForNode(node);
    std::ranges::sort(typeNames, {}, &ExportedTypeName::moduleId);
    auto removedRanges = std::ranges::unique(typeNames, {}, &ExportedTypeName::moduleId);
    typeNames.erase(removedRanges.begin(), removedRanges.end());

    const Module qtQuickModule = modulesStorage.module(
        modulesStorage.moduleId("QtQuick", ModuleKind::QmlLibrary));

    QVarLengthArray<Module, 32> modules;
    for (const ExportedTypeName &typeName : typeNames) {
        const Module &module = modulesStorage.module(typeName.moduleId);
        if (module != qtQuickModule && (module.kind == ModuleKind::QmlLibrary
                                        || module.kind == ModuleKind::PathLibrary)) {
            modules.push_back(module);
        }
    }

    std::ranges::sort(modules, {}, &Module::name);

    // Every component needs QtQuick import, and we should also ensure it is the first one as
    // sometimes order matters
    modules.insert(modules.cbegin(), qtQuickModule);

    Imports imports;
    imports.reserve(typeNames.size());
    for (const Module &module : modules) {
        if (const Import &import = createImport(module); !import.isEmpty())
            imports.append(import);
    }

    return imports;
}

QPair<QString, QSet<AssetPath>> BundleHelper::modelNodeToQmlString(const ModelNode &node, int depth)
{
    NanotraceHR::Tracer tracer{"bundle helper model node to qml string", category()};

    static QStringList depListIds;

    QString qml;
    QSet<AssetPath> assets;

    if (node.metaInfo().isQtQmlConnections())
        return {qml, assets};

    if (depth == 0) {
        // add imports
        Model *model = m_view->model();
        ModulesStorage &modulesStorage = model->projectStorageDependencies().modulesStorage;

        Imports imports = getRequiredImports(node, modulesStorage, model);
        auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();
        const QString materialsBundleType = compUtils.materialsBundleType();

        // For the exported components we should move Bundles.Materials dependencies to Bundles.UserMaterials
        // In order to make them portable. So we need to change the imports here as well.
        for (Import &import : imports) {
            if (!import.isLibraryImport())
                continue;

            if (QString importUrl = import.url(); importUrl == materialsBundleType) {
                importUrl.replace(materialsBundleType, compUtils.userMaterialsBundleType());
                import = Import::createLibraryImport(importUrl,
                                                     import.version(),
                                                     import.alias(),
                                                     import.importPaths());
            }
        }

        QString importsStr = Utils::transform(imports, &Import::toImportString).join(QChar::LineFeed);
        if (!importsStr.isEmpty())
            importsStr.append(QString(2, QChar::LineFeed));

        qml.append(importsStr);
        depListIds.clear();
    }

    QString indent = QString(" ").repeated(depth * 4);

    qml += indent + node.simplifiedDocumentTypeRepresentation() + " {\n";

    indent = QString(" ").repeated((depth + 1) * 4);

    if (!node.id().isEmpty()) {
        depListIds.append(node.id());
        qml += indent + "id: " + node.id() + " \n\n";
    }

    const QList<PropertyName> excludedProps = {"x", "y", "z", "eulerRotation.x", "eulerRotation.y",
                                               "eulerRotation.z", "scale.x", "scale.y", "scale.z",
                                               "pivot.x", "pivot.y", "pivot.z"};
    const QList<AbstractProperty> nodeProps = node.properties();
    for (const AbstractProperty &p : nodeProps) {
        if (depth == 0 && excludedProps.contains(p.name()))
            continue;

        if (p.isVariantProperty()) {
            QVariant pValue = p.toVariantProperty().value();
            QString val;

            if (!pValue.typeName()) {
                // dynamic property with no value assigned
            } else if (strcmp(pValue.typeName(), "QString") == 0 || strcmp(pValue.typeName(), "QColor") == 0) {
                val = QLatin1String("\"%1\"").arg(pValue.toString());
            } else if (std::string_view{pValue.typeName()} == "QUrl") {
                QString pValueStr = pValue.toString();
                val = QLatin1String("\"%1\"").arg(pValueStr);
                if (!pValueStr.startsWith("#"))
                    assets.insert({DocumentManager::currentFilePath().parentDir(), pValue.toString()});
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
            const QString pExp = p.toBindingProperty().expression();
            const QStringList depNodesIds = ModelUtils::expressionToList(pExp);
            QTC_ASSERT(!depNodesIds.isEmpty(), continue);

            if (p.isDynamic())
                qml += indent + "property "  + p.dynamicTypeName() + " " + p.name() + ": " + pExp + "\n";
            else
                qml += indent + p.name() + ": " + pExp + "\n";

            for (const QString &id : depNodesIds) {
                ModelNode depNode = m_view->modelNodeForId(id);
                QTC_ASSERT(depNode.isValid(), continue);

                if (depNode && !depListIds.contains(depNode.id())) {
                    auto [depQml, depAssets] = modelNodeToQmlString(depNode, depth + 1);
                    qml += "\n" + depQml + "\n";
                    assets.unite(depAssets);
                }
            }
        }
    }

    // add child nodes
    const ModelNodes nodeChildren = node.directSubModelNodes();
    for (const ModelNode &childNode : nodeChildren) {
        if (childNode && !depListIds.contains(childNode.id())) {
            auto [depQml, depAssets] = modelNodeToQmlString(childNode, depth + 1);
            qml += "\n" + depQml + "\n";
            assets.unite(depAssets);
        }
    }

    indent = QString(" ").repeated(depth * 4);

    qml += indent + "}\n";

    if (isProjectComponent(node)) {
        auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();
        bool isBundle = nodeModuleName(node, node.model())
                            .startsWith(compUtils.componentBundlesTypePrefix() + ".");

        if (depth > 0) {
            // add component file to the dependency assets
            Utils::FilePath compFilePath = componentPath(node);
            assets.insert({compFilePath.parentDir(), compFilePath.fileName()});
        }

        if (isBundle) {
            Utils::FilePath compFilePath = componentPath(node);
            Utils::FilePath compDir = compFilePath.parentDir();
            assets.unite(getComponentDependencies(compFilePath, compDir));
        }
    }

    return {qml, assets};
}

QSet<AssetPath> BundleHelper::getBundleComponentDependencies(const ModelNode &node) const
{
    NanotraceHR::Tracer tracer{"bundle helper get bundle component dependencies", category()};

    const QString compFileName = node.simplifiedDocumentTypeRepresentation() + ".qml";

    Utils::FilePath compPath = componentPath(node).parentDir();

    QTC_ASSERT(compPath.exists(), return {});

    QSet<AssetPath> depList;

    Utils::FilePath assetRefPath = compPath.pathAppended(Constants::COMPONENT_BUNDLES_ASSET_REF_FILE);

    Utils::Result<QByteArray> assetRefContents = assetRefPath.fileContents();
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
            depList.insert({compPath, asset});
    }

    return depList;
}

Utils::FilePath BundleHelper::componentPath(const ModelNode &node) const
{
    NanotraceHR::Tracer tracer{"bundle helper component path", category()};

    return Utils::FilePath::fromString(ModelUtils::componentFilePath(node));
}

QString BundleHelper::nodeNameToComponentFileName(const QString &name) const
{
    NanotraceHR::Tracer tracer{"bundle helper node name to component file name", category()};

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
    NanotraceHR::Tracer tracer{"bundle helper get image from cache", category()};

    QmlDesignerPlugin::imageCache().requestSmallImage(
        Utils::PathString{qmlPath}, successCallback, [qmlPath](ImageCache::AbortReason abortReason) {
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

void BundleHelper::addIconToZip(const QString &iconPath, const auto &image)
{
    NanotraceHR::Tracer tracer{"bundle helper add icon to zip", category()};

    QByteArray iconByteArray;
    QBuffer buffer(&iconByteArray);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");

    m_zipWriter->addFile(iconPath, iconByteArray);
    maybeCloseZip();
};

QString BundleHelper::getImportPath() const
{
    NanotraceHR::Tracer tracer{"bundle helper get import path", category()};

    Utils::FilePath projectFP = DocumentManager::currentProjectDirPath();
    if (projectFP.isEmpty()) {
        projectFP = QmlDesignerPlugin::instance()->documentManager()
                        .currentDesignDocument()->fileName().parentDir();
    }

    return QFileDialog::getOpenFileName(m_widget,
                                        Tr::tr("Import Component"),
                                        projectFP.toFSPathString(),
                                        Tr::tr("Qt Design Studio Bundle Files")
                                            + QString(" (*.%1)").arg(Constants::BUNDLE_SUFFIX));
}

QString BundleHelper::getExportPath(const ModelNode &node) const
{
    NanotraceHR::Tracer tracer{"bundle helper get export path", category()};

    QString defaultExportFileName = QLatin1String("%1.%2").arg(node.displayName(),
                                                               Constants::BUNDLE_SUFFIX);
    Utils::FilePath projectFP = DocumentManager::currentProjectDirPath();
    if (projectFP.isEmpty()) {
        projectFP = QmlDesignerPlugin::instance()->documentManager()
                        .currentDesignDocument()->fileName().parentDir();
    }

    QString dialogTitle = node.metaInfo().isQtQuick3DMaterial() ? Tr::tr("Export Material")
                                                                : Tr::tr("Export Component");
    return QFileDialog::getSaveFileName(
        m_widget,
        dialogTitle,
        projectFP.pathAppended(defaultExportFileName).toFSPathString(),
        Tr::tr("Qt Design Studio Bundle Files (*.%1)").arg(Constants::BUNDLE_SUFFIX));
}

bool BundleHelper::isMaterialBundle(const QString &bundleId) const
{
    NanotraceHR::Tracer tracer{"bundle helper is material bundle", category()};

    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();
    return bundleId == compUtils.materialsBundleId() || bundleId == compUtils.userMaterialsBundleId();
}

// item bundle includes effects and 3D components
bool BundleHelper::isItemBundle(const QString &bundleId) const
{
    NanotraceHR::Tracer tracer{"bundle helper is item bundle", category()};

    auto compUtils = QmlDesignerPlugin::instance()->documentManager().generatedComponentUtils();
    return bundleId == compUtils.effectsBundleId() || bundleId == compUtils.userEffectsBundleId()
           || bundleId == compUtils.user3DBundleId();
}

QSet<AssetPath> BundleHelper::getComponentDependencies(const Utils::FilePath &filePath,
                                                       const Utils::FilePath &mainCompDir) const
{
    NanotraceHR::Tracer tracer{"bundle helper get component dependencies", category()};

    QSet<AssetPath> depList;
    AssetPath compAssetPath = {mainCompDir, filePath.relativePathFromDir(mainCompDir).toFSPathString()};

    ModelPointer model = m_view->model()->createModel({"Item"});

    const Utils::Result<QByteArray> res = filePath.fileContents();
    QTC_ASSERT(res, return {});

    QPlainTextEdit textEdit;
    textEdit.setPlainText(QString::fromUtf8(*res));
    NotIndentingTextEditModifier modifier(textEdit.document());
    modifier.setParent(model.get());
    RewriterView rewriterView(m_view->externalDependencies(),
                              model->projectStorageDependencies().modulesStorage,
                              RewriterView::Validate);
    rewriterView.setCheckSemanticErrors(false);
    rewriterView.setTextModifier(&modifier);
    model->attachView(&rewriterView);
    rewriterView.restoreAuxiliaryData();
    ModelNode rootNode = rewriterView.rootModelNode();
    QTC_ASSERT(rootNode.isValid(), return {});

    std::function<void(const ModelNode &node)> parseNode;
    parseNode = [&](const ModelNode &node) {
        if (isProjectComponent(node)) {
            Utils::FilePath compFilePath = Utils::FilePath::fromString(ModelUtils::componentFilePath(node));
            if (!compFilePath.isEmpty()) {
                Utils::FilePath compDir = compFilePath.isChildOf(mainCompDir)
                                              ? mainCompDir : compFilePath.parentDir();
                depList.unite(getComponentDependencies(compFilePath, compDir));

                // for sub components, mark their imports to be removed from their parent component
                // as they will be moved to the same folder as the parent
                QString import = nodeModuleName(node, node.model());
                if (model->hasImport(import))
                    compAssetPath.importsToRemove.append(import);

                return;
            }
        }

        const QList<AbstractProperty> nodeProps = node.properties();
        for (const AbstractProperty &p : nodeProps) {
            if (p.isVariantProperty()) {
                QVariant pValue = p.toVariantProperty().value();
                if (std::string_view{pValue.typeName()} == "QUrl") {
                    QString pValueStr = pValue.toString();
                    if (!pValueStr.isEmpty() && !pValueStr.startsWith("#")) {
                        Utils::FilePath pValuePath = Utils::FilePath::fromString(pValueStr);
                        Utils::FilePath assetPathBase;
                        QString assetPathRelative;

                        if (!pValuePath.toUrl().isLocalFile() || pValuePath.startsWith("www.")) {
                            qWarning() << "BundleHelper::getComponentDependencies(): Web urls are not"
                                          " supported. Skipping " << pValuePath;
                            continue;
                        } else if (pValuePath.isAbsolutePath()) {
                            assetPathRelative = pValuePath.fileName();
                            assetPathBase = pValuePath.parentDir();
                        } else {
                            Utils::FilePath assetPath = filePath.parentDir().resolvePath(pValueStr);
                            assetPathRelative = assetPath.relativePathFromDir(mainCompDir).toFSPathString();
                            assetPathBase = mainCompDir;
                        }

                        QTC_ASSERT(!assetPathRelative.isEmpty(), continue);
                        depList.insert({assetPathBase, assetPathRelative});
                    }
                }
            } else if (p.isBindingProperty()) {
                // check if the property value is in this format: Qt.resolvedUrl("path")
                static const QRegularExpression regex(R"(Qt\.resolvedUrl\(\"([^\"]+)\"\))");
                QRegularExpressionMatch match = regex.match(p.toBindingProperty().expression());

                if (match.hasMatch()) {
                    Utils::FilePath assetPath = filePath.parentDir().resolvePath(match.captured(1));
                    QString assetPathRelative = assetPath.relativePathFromDir(mainCompDir).toFSPathString();

                    QTC_ASSERT(assetPath.exists(), continue);
                    depList.insert({mainCompDir, assetPathRelative});
                }
            }
        }

        // parse child nodes
        const QList<ModelNode> childNodes = node.directSubModelNodes();
        for (const ModelNode &childNode : childNodes)
            parseNode(childNode);
    };

    parseNode(rootNode);

    depList.insert(compAssetPath);

    return depList;
}

bool BundleHelper::isProjectComponent(const ModelNode &node) const
{
    NanotraceHR::Tracer tracer{"bundle helper is project component", category()};

    if (node.isComponent()) {
        const QString projPath = m_view->externalDependencies().currentProjectDirPath();
        const QString compFilePath = ModelUtils::componentFilePath(node);
        return compFilePath.startsWith(projPath);
    }
    return false;
};

} // namespace QmlDesigner
