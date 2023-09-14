// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "contentlibrarybundleimporter.h"

#include "documentmanager.h"
#include "import.h"
#include "model.h"
#include "qmldesignerconstants.h"
#include "qmldesignerplugin.h"
#include "rewritingexception.h"

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

using namespace Utils;

namespace QmlDesigner::Internal {

ContentLibraryBundleImporter::ContentLibraryBundleImporter(const QString &bundleDir,
                                                           const QString &bundleId,
                                                           const QStringList &sharedFiles,
                                                           QObject *parent)
    : QObject(parent)
    , m_bundleDir(FilePath::fromString(bundleDir))
    , m_bundleId(bundleId)
    , m_sharedFiles(sharedFiles)
{
    m_importTimer.setInterval(200);
    connect(&m_importTimer, &QTimer::timeout, this, &ContentLibraryBundleImporter::handleImportTimer);
    m_moduleName = QStringLiteral("%1.%2").arg(
                QLatin1String(Constants::COMPONENT_BUNDLES_FOLDER),
                m_bundleId).mid(1); // Chop leading slash
}

// Returns empty string on success or an error message on failure.
// Note that there is also an asynchronous portion to the import, which will only
// be done if this method returns success. Once the asynchronous portion of the
// import is completed, importFinished signal will be emitted.
QString ContentLibraryBundleImporter::importComponent(const QString &qmlFile,
                                                      const QStringList &files)
{
    FilePath bundleImportPath = resolveBundleImportPath();
    if (bundleImportPath.isEmpty())
        return "Failed to resolve bundle import folder";

    bool bundleImportPathExists = bundleImportPath.exists();

    if (!bundleImportPathExists && !bundleImportPath.createDir())
        return QStringLiteral("Failed to create bundle import folder: '%1'").arg(bundleImportPath.toString());

    for (const QString &file : std::as_const(m_sharedFiles)) {
        FilePath target = bundleImportPath.resolvePath(file);
        if (!target.exists()) {
            FilePath parentDir = target.parentDir();
            if (!parentDir.exists() && !parentDir.createDir())
                return QStringLiteral("Failed to create folder for: '%1'").arg(target.toString());
            FilePath source = m_bundleDir.resolvePath(file);
            if (!source.copyFile(target))
                return QStringLiteral("Failed to copy shared file: '%1'").arg(source.toString());
        }
    }

    FilePath qmldirPath = bundleImportPath.resolvePath(QStringLiteral("qmldir"));
    QString qmldirContent = QString::fromUtf8(qmldirPath.fileContents().value_or(QByteArray()));
    if (qmldirContent.isEmpty()) {
        qmldirContent.append("module ");
        qmldirContent.append(m_moduleName);
        qmldirContent.append('\n');
    }

    FilePath qmlSourceFile = bundleImportPath.resolvePath(FilePath::fromString(qmlFile));
    const bool qmlFileExists = qmlSourceFile.exists();
    const QString qmlType = qmlSourceFile.baseName();
    const QString fullTypeName = QStringLiteral("%1.%2.%3")
            .arg(QLatin1String(Constants::COMPONENT_BUNDLES_FOLDER).mid(1), m_bundleId, qmlType);
    if (m_pendingTypes.contains(fullTypeName) && !m_pendingTypes[fullTypeName])
        return QStringLiteral("Unable to import while unimporting the same type: '%1'").arg(fullTypeName);
    if (!qmldirContent.contains(qmlFile)) {
        qmldirContent.append(qmlType);
        qmldirContent.append(" 1.0 ");
        qmldirContent.append(qmlFile);
        qmldirContent.append('\n');
        qmldirPath.writeFileContents(qmldirContent.toUtf8());
    }

    QStringList allFiles;
    allFiles.append(files);
    allFiles.append(qmlFile);
    for (const QString &file : std::as_const(allFiles)) {
        FilePath target = bundleImportPath.resolvePath(file);
        FilePath parentDir = target.parentDir();
        if (!parentDir.exists() && !parentDir.createDir())
            return QStringLiteral("Failed to create folder for: '%1'").arg(target.toString());

        FilePath source = m_bundleDir.resolvePath(file);
        if (target.exists()) {
            if (source.lastModified() == target.lastModified())
                continue;
            target.removeFile(); // Remove existing file for update
        }
        if (!source.copyFile(target))
            return QStringLiteral("Failed to copy file: '%1'").arg(source.toString());
    }

    QVariantHash assetRefMap = loadAssetRefMap(bundleImportPath);
    bool writeAssetRefs = false;
    for (const QString &assetFile : files) {
        QStringList assets = assetRefMap[assetFile].toStringList();
        if (!assets.contains(qmlFile)) {
            assets.append(qmlFile);
            writeAssetRefs = true;
        }
        assetRefMap[assetFile] = assets;
    }
    if (writeAssetRefs)
        writeAssetRefMap(bundleImportPath, assetRefMap);

    m_fullReset = !qmlFileExists;
    auto doc = QmlDesignerPlugin::instance()->currentDesignDocument();
    Model *model = doc ? doc->currentModel() : nullptr;
    if (!model)
        return "Model not available, cannot add import statement or update code model";

    Import import = Import::createLibraryImport(m_moduleName, "1.0");
    if (!model->hasImport(import)) {
        if (model->possibleImports().contains(import)) {
            m_importAddPending = false;
            try {
                model->changeImports({import}, {});
            } catch (const RewritingException &) {
                // No point in trying to add import asynchronously either, so just fail out
                return QStringLiteral("Failed to add import statement for: '%1'").arg(m_moduleName);
            }
        } else {
            // If import is not yet possible, import statement needs to be added asynchronously to
            // avoid errors, as code model update takes a while.
            m_importAddPending = true;
        }
    }
    m_pendingTypes.insert(fullTypeName, true);
    m_importTimerCount = 0;
    m_importTimer.start();

    return {};
}

void ContentLibraryBundleImporter::handleImportTimer()
{
    auto handleFailure = [this]() {
        m_importTimer.stop();
        m_fullReset = false;
        m_importAddPending = false;
        m_importTimerCount = 0;

        // Emit dummy finished signals for all pending types
        const QStringList pendingTypes = m_pendingTypes.keys();
        for (const QString &pendingType : pendingTypes) {
            m_pendingTypes.remove(pendingType);
            if (m_pendingTypes[pendingType])
                emit importFinished({});
            else
                emit unimportFinished({});
        }
    };

    auto doc = QmlDesignerPlugin::instance()->currentDesignDocument();
    Model *model = doc ? doc->currentModel() : nullptr;
    if (!model || ++m_importTimerCount > 100) {
        handleFailure();
        return;
    }

    if (m_fullReset) {
        // Force code model reset to notice changes to existing module
        auto modelManager = QmlJS::ModelManagerInterface::instance();
        if (modelManager)
            modelManager->resetCodeModel();
        m_fullReset = false;
        return;
    }

    QmlDesignerPlugin::instance()->documentManager().resetPossibleImports();

    if (m_importAddPending) {
        try {
            Import import = Import::createLibraryImport(m_moduleName, "1.0");
            if (model->possibleImports().contains(import)) {
                model->changeImports({import}, {});
                m_importAddPending = false;
            }
        } catch (const RewritingException &) {
            // Import adding is unlikely to succeed later, either, so just bail out
            handleFailure();
        }
        return;
    }

    // Detect when the code model has the new material(s) fully available
    const QStringList pendingTypes = m_pendingTypes.keys();
    for (const QString &pendingType : pendingTypes) {
        NodeMetaInfo metaInfo = model->metaInfo(pendingType.toUtf8());
        const bool isImport = m_pendingTypes[pendingType];
        const bool typeComplete = metaInfo.isValid() && !metaInfo.prototypes().empty();
        if (isImport == typeComplete) {
            m_pendingTypes.remove(pendingType);
            if (isImport)
#ifdef QDS_USE_PROJECTSTORAGE
                emit importFinished(pendingType.toUtf8());
#else
                emit importFinished(metaInfo);
#endif
            else
                emit unimportFinished(metaInfo);
        }
    }

    if (m_pendingTypes.isEmpty()) {
        m_importTimer.stop();
        m_importTimerCount = 0;
    }
}

QVariantHash ContentLibraryBundleImporter::loadAssetRefMap(const Utils::FilePath &bundlePath)
{
    FilePath assetRefPath = bundlePath.resolvePath(QLatin1String(Constants::COMPONENT_BUNDLES_ASSET_REF_FILE));
    const Utils::expected_str<QByteArray> content = assetRefPath.fileContents();
    if (content) {
        QJsonParseError error;
        QJsonDocument bundleDataJsonDoc = QJsonDocument::fromJson(*content, &error);
        if (bundleDataJsonDoc.isNull()) {
            // Failure to read asset refs is not considred fatal, so just print error
            qWarning() << "Failed to parse bundle asset ref file:" << error.errorString();
        } else {
            return bundleDataJsonDoc.object().toVariantHash();
        }
    }
    return {};
}

void ContentLibraryBundleImporter::writeAssetRefMap(const Utils::FilePath &bundlePath,
                                                    const QVariantHash &assetRefMap)
{
    FilePath assetRefPath = bundlePath.resolvePath(QLatin1String(Constants::COMPONENT_BUNDLES_ASSET_REF_FILE));
    QJsonObject jsonObj = QJsonObject::fromVariantHash(assetRefMap);
    if (!assetRefPath.writeFileContents(QJsonDocument{jsonObj}.toJson())) {
        // Failure to write asset refs is not considred fatal, so just print error
        qWarning() << QStringLiteral("Failed to save bundle asset ref file: '%1'").arg(assetRefPath.toString()) ;
    }
}

QString ContentLibraryBundleImporter::unimportComponent(const QString &qmlFile)
{
    FilePath bundleImportPath = resolveBundleImportPath();
    if (bundleImportPath.isEmpty())
        return QStringLiteral("Failed to resolve bundle import folder for: '%1'").arg(qmlFile);

    if (!bundleImportPath.exists())
        return QStringLiteral("Unable to find bundle path: '%1'").arg(bundleImportPath.toString());

    FilePath qmlFilePath = bundleImportPath.resolvePath(qmlFile);
    if (!qmlFilePath.exists())
        return QStringLiteral("Unable to find specified file: '%1'").arg(qmlFilePath.toString());

    QStringList removedFiles;
    removedFiles.append(qmlFile);

    FilePath qmldirPath = bundleImportPath.resolvePath(QStringLiteral("qmldir"));
    const expected_str<QByteArray> qmldirContent = qmldirPath.fileContents();
    QByteArray newContent;

    QString qmlType = qmlFilePath.baseName();
    const QString fullTypeName = QStringLiteral("%1.%2.%3")
            .arg(QLatin1String(Constants::COMPONENT_BUNDLES_FOLDER).mid(1), m_bundleId, qmlType);
    if (m_pendingTypes.contains(fullTypeName) && m_pendingTypes[fullTypeName])
        return QStringLiteral("Unable to unimport while importing the same type: '%1'").arg(fullTypeName);

    if (qmldirContent) {
        int typeIndex = qmldirContent->indexOf(qmlType.toUtf8());
        if (typeIndex != -1) {
            int newLineIndex = qmldirContent->indexOf('\n', typeIndex);
            newContent = qmldirContent->left(typeIndex);
            if (newLineIndex != -1)
                newContent.append(qmldirContent->mid(newLineIndex + 1));
        }
        if (newContent != qmldirContent) {
            if (!qmldirPath.writeFileContents(newContent))
                return QStringLiteral("Failed to write qmldir file: '%1'").arg(qmldirPath.toString());
        }
    }

    m_pendingTypes.insert(fullTypeName, false);

    QVariantHash assetRefMap = loadAssetRefMap(bundleImportPath);
    bool writeAssetRefs = false;
    const auto keys = assetRefMap.keys();
    for (const QString &assetFile : keys) {
        QStringList assets = assetRefMap[assetFile].toStringList();
        if (assets.contains(qmlFile)) {
            assets.removeAll(qmlFile);
            writeAssetRefs = true;
        }
        if (!assets.isEmpty()) {
            assetRefMap[assetFile] = assets;
        } else {
            removedFiles.append(assetFile);
            assetRefMap.remove(assetFile);
            writeAssetRefs = true;
        }
    }

    for (const QString &removedFile : removedFiles) {
        FilePath removedFilePath = bundleImportPath.resolvePath(removedFile);
        if (removedFilePath.exists())
            removedFilePath.removeFile();
    }

    if (writeAssetRefs)
        writeAssetRefMap(bundleImportPath, assetRefMap);

    // If the bundle module contains no .qml files after unimport, remove the import statement
    if (bundleImportPath.dirEntries({{"*.qml"}, QDir::Files}).isEmpty()) {
        auto doc = QmlDesignerPlugin::instance()->currentDesignDocument();
        Model *model = doc ? doc->currentModel() : nullptr;
        if (model) {
            Import import = Import::createLibraryImport(m_moduleName, "1.0");
            if (model->imports().contains(import))
                model->changeImports({}, {import});
        }
    }

    m_fullReset = true;
    m_importTimerCount = 0;
    m_importTimer.start();

    return {};
}

FilePath ContentLibraryBundleImporter::resolveBundleImportPath()
{
    FilePath bundleImportPath = QmlDesignerPlugin::instance()->documentManager().currentProjectDirPath();
    if (bundleImportPath.isEmpty())
        return bundleImportPath;

    const QString projectBundlePath = QStringLiteral("%1%2/%3").arg(
                QLatin1String(Constants::DEFAULT_ASSET_IMPORT_FOLDER),
                QLatin1String(Constants::COMPONENT_BUNDLES_FOLDER),
                m_bundleId).mid(1); // Chop leading slash

    return bundleImportPath.resolvePath(projectBundlePath);
}

} // namespace QmlDesigner::Internal
