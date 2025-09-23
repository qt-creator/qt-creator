// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bundleimporter.h"

#include <documentmanager.h>
#include <import.h>
#include <model.h>
#include <nodemetainfo.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <rewritingexception.h>

#include <modelutils.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <utils/async.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

using namespace Utils;

namespace QmlDesigner {

static constexpr int normalImportDelay = 200;

BundleImporter::BundleImporter(QObject *parent)
    : QObject(parent)
{
    connect(&m_importTimer, &QTimer::timeout, this, &BundleImporter::handleImportTimer);
}

// Returns empty string on success or an error message on failure.
// Note that there is also an asynchronous portion to the import, which will only
// be done if this method returns success. Once the asynchronous portion of the
// import is completed, importFinished signal will be emitted.
QString BundleImporter::importComponent(const QString &bundleDir,
                                        const TypeName &type,
                                        const QString &qmlFile,
                                        const QStringList &files)
{
    QString module = QString::fromLatin1(type.left(type.lastIndexOf('.')));
    m_bundleId = module.mid(module.lastIndexOf('.') + 1);

    FilePath bundleDirPath = FilePath::fromString(bundleDir); // source dir
    FilePath bundleImportPath = resolveBundleImportPath(m_bundleId); // target dir

    if (bundleImportPath.isEmpty())
        return "Failed to resolve bundle import folder";

    if (!bundleImportPath.exists() && !bundleImportPath.createDir())
        return QStringLiteral("Failed to create bundle import folder: '%1'").arg(bundleImportPath.toUrlishString());

    bool doReset = false;

    FilePath qmldirPath = bundleImportPath.pathAppended("qmldir");
    QString qmldirContent = QString::fromUtf8(qmldirPath.fileContents().value_or(QByteArray()));
    if (qmldirContent.isEmpty()) {
        qmldirContent.append("module ");
        qmldirContent.append(module);
        qmldirContent.append('\n');
    }

    FilePath qmlSourceFile = bundleImportPath.pathAppended(qmlFile);
    const QString qmlType = qmlSourceFile.baseName();

    if (m_pendingImports.contains(type) && !m_pendingImports[type].isImport)
        return QStringLiteral("Unable to import while unimporting the same type: '%1'").arg(QLatin1String(type));

    bool typeAdded = false;
    auto addTypeToQmldir = [&qmldirContent, &typeAdded](const QString &qmlType,
                                                        const QString &qmlFile) {
        const QString pattern = QString{"^\\s*%1\\s+1\\.0\\s+%2\\s*$"}
                                    .arg(QRegularExpression::escape(qmlType),
                                         QRegularExpression::escape(qmlFile));
        const QRegularExpression regex{pattern, QRegularExpression::MultilineOption};

        if (!qmldirContent.contains(regex)) {
            qmldirContent.append(QString{"%1 1.0 %2\n"}.arg(qmlType, qmlFile));
            typeAdded = true;
        }
    };

    addTypeToQmldir(qmlType, qmlFile);
    for (const QString &file : files) {
        if (const QFileInfo fileInfo{file}; fileInfo.suffix() == "qml")
            addTypeToQmldir(fileInfo.baseName(), file);
    }

    if (typeAdded) {
        qmldirPath.writeFileContents(qmldirContent.toUtf8());
        doReset = true;
    }

    QStringList allFiles;
    allFiles.append(files);
    allFiles.append(qmlFile);
    for (const QString &file : std::as_const(allFiles)) {
        FilePath target = bundleImportPath.pathAppended(file);
        FilePath parentDir = target.parentDir();
        if (!parentDir.exists() && !parentDir.createDir())
            return QStringLiteral("Failed to create folder for: '%1'").arg(target.toUrlishString());

        FilePath source = bundleDirPath.pathAppended(file);
        if (target.exists()) {
            if (source.lastModified() == target.lastModified())
                continue;
            target.removeFile(); // Remove existing file for update
        }
        if (!source.copyFile(target))
            return QStringLiteral("Failed to copy file: '%1'").arg(source.toUrlishString());
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

    auto doc = QmlDesignerPlugin::instance()->currentDesignDocument();
    Model *model = doc ? doc->currentModel() : nullptr;
    if (!model)
        return "Model not available, cannot add import statement or update code model";

    ImportData data;
    data.isImport = true;
    data.typeAdded = typeAdded;

    m_pendingFullReset = doReset;
    data.simpleType = type.split('.').constLast();
    data.moduleName = module;
    data.module = model->module(module.toUtf8(), Storage::ModuleKind::QmlLibrary);

    m_pendingImports.insert(type, data);
    m_importTimerCount = 0;
    m_importTimer.start(normalImportDelay);

    return {};
}

void BundleImporter::handleImportTimer()
{
    auto handleFailure = [this] {
        m_importTimer.stop();
        m_importTimerCount = 0;
        m_pendingFullReset = false;

        // Emit dummy finished signals for all pending types
        const QList<TypeName> pendingTypes = m_pendingImports.keys();
        for (const TypeName &pendingType : pendingTypes) {
            ImportData data = m_pendingImports.take(pendingType);
            if (data.isImport)
                emit importFinished({}, m_bundleId, false);
            else
                emit unimportFinished(m_bundleId);
        }
        m_bundleId.clear();
    };

    auto doc = QmlDesignerPlugin::instance()->currentDesignDocument();
    Model *model = doc ? doc->currentModel() : nullptr;
    if (!model || ++m_importTimerCount > 100) {
        handleFailure();
        return;
    }

    const QList<TypeName> keys = m_pendingImports.keys();
    for (const TypeName &type : keys) {
        const ImportData data = m_pendingImports.value(type);
        // Verify that code model has the new type fully available (or removed for unimport)
        NodeMetaInfo metaInfo = model->metaInfo(data.module, data.simpleType);
        if (data.isImport == metaInfo.isValid()) {
            m_pendingImports.remove(type);
            if (data.isImport) {
                Import import = Import::createLibraryImport(data.moduleName);
                model->changeImports({import}, {});
                emit importFinished(data.simpleType, m_bundleId, data.typeAdded);
            } else {
                emit unimportFinished(m_bundleId);
            }
        }
    }

    if (keys.size() > 0)
        return; // Do the code model reset/cleanup on next timer tick

    if (m_pendingFullReset) {
        m_pendingFullReset = false;
        // Force code model reset to notice changes to existing module
        auto modelManager = QmlJS::ModelManagerInterface::instance();
        if (modelManager)
            modelManager->resetCodeModel();
    }

    if (m_pendingImports.isEmpty()) {
        m_bundleId.clear();
        m_importTimer.stop();
        m_importTimerCount = 0;
    }
}

QVariantHash BundleImporter::loadAssetRefMap(const FilePath &bundlePath)
{
    FilePath assetRefPath = bundlePath.resolvePath(QLatin1String(Constants::COMPONENT_BUNDLES_ASSET_REF_FILE));
    const Result<QByteArray> content = assetRefPath.fileContents();
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

void BundleImporter::writeAssetRefMap(const FilePath &bundlePath, const QVariantHash &assetRefMap)
{
    FilePath assetRefPath = bundlePath.resolvePath(QLatin1String(Constants::COMPONENT_BUNDLES_ASSET_REF_FILE));
    QJsonObject jsonObj = QJsonObject::fromVariantHash(assetRefMap);
    if (!assetRefPath.writeFileContents(QJsonDocument{jsonObj}.toJson())) {
        // Failure to write asset refs is not considred fatal, so just print error
        qWarning() << QStringLiteral("Failed to save bundle asset ref file: '%1'").arg(assetRefPath.toUrlishString()) ;
    }
}

QString BundleImporter::unimportComponent(const TypeName &type, const QString &qmlFile)
{
    QString module = QString::fromLatin1(type.left(type.lastIndexOf('.')));
    m_bundleId = module.mid(module.lastIndexOf('.') + 1);

    emit aboutToUnimport(type.split('.').constLast(), m_bundleId);

    FilePath bundleImportPath = resolveBundleImportPath(m_bundleId);
    if (bundleImportPath.isEmpty())
        return QStringLiteral("Failed to resolve bundle import folder for: '%1'").arg(qmlFile);

    if (!bundleImportPath.exists())
        return QStringLiteral("Unable to find bundle path: '%1'").arg(bundleImportPath.toUrlishString());

    FilePath qmlFilePath = bundleImportPath.resolvePath(qmlFile);
    if (!qmlFilePath.exists())
        return QStringLiteral("Unable to find specified file: '%1'").arg(qmlFilePath.toUrlishString());

    QStringList removedFiles;
    removedFiles.append(qmlFile);

    FilePath qmldirPath = bundleImportPath.resolvePath(QStringLiteral("qmldir"));
    const Result<QByteArray> qmldirContent = qmldirPath.fileContents();
    QByteArray newContent;

    QString qmlType = qmlFilePath.baseName();
    if (m_pendingImports.contains(type) && m_pendingImports[type].isImport) {
        return QStringLiteral("Unable to unimport while importing the same type: '%1'")
            .arg(QString::fromLatin1(type));
    }

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

    const QString qmldirPattern = "^\\s*%1\\s+.*\\.qml\\s*\\r?\\n";
    QString qmldirStr;
    QString newQmldirStr;
    if (qmldirContent) {
        // Remove the main qml type export from qmldir file
        qmldirStr = QString::fromUtf8(*qmldirContent);
        const QRegularExpression regex{qmldirPattern.arg(QRegularExpression::escape(qmlType)),
                                       QRegularExpression::MultilineOption};
        newQmldirStr = qmldirStr;
        newQmldirStr.remove(regex);
    }

    const QString qmlSuffix = "qml";
    for (const QString &removedFile : removedFiles) {
        FilePath removedFilePath = bundleImportPath.resolvePath(removedFile);
        if (removedFilePath.exists())
            removedFilePath.removeFile();
        if (removedFilePath.suffix() == qmlSuffix) {
            // If the removed dependency was a qml document, also remove the corresponding type
            // export from qmldir file
            QString removedType = removedFilePath.baseName();
            const QRegularExpression regex{qmldirPattern.arg(QRegularExpression::escape(removedType)),
                                           QRegularExpression::MultilineOption};
            newQmldirStr.remove(regex);
        }
    }

    if (newQmldirStr != qmldirStr) {
        if (!qmldirPath.writeFileContents(newQmldirStr.toUtf8()))
            return QStringLiteral("Failed to write qmldir file: '%1'").arg(qmldirPath.toUrlishString());
    }

    if (writeAssetRefs)
        writeAssetRefMap(bundleImportPath, assetRefMap);

    auto doc = QmlDesignerPlugin::instance()->currentDesignDocument();
    Model *model = doc ? doc->currentModel() : nullptr;
    if (!model)
        return "Model not available, cannot remove import statement or update code model";

    // If the bundle module contains no .qml files after unimport, remove the import statement
    if (bundleImportPath.dirEntries({{"*.qml"}, QDir::Files}).isEmpty()) {
        if (model) {
            Import import = Import::createLibraryImport(module);
            if (model->imports().contains(import))
                model->changeImports({}, {import});
        }
    }

    ImportData data;
    data.isImport = false;
    data.simpleType = type.split('.').constLast();
    data.moduleName = module;
    data.module = model->module(module.toUtf8(), Storage::ModuleKind::QmlLibrary);

    m_pendingImports.insert(type, data);

    m_importTimerCount = 0;
    m_importTimer.start(normalImportDelay);

    return {};
}

FilePath BundleImporter::resolveBundleImportPath(const QString &bundleId)
{
    FilePath bundleImportPath = QmlDesignerPlugin::instance()->documentManager()
                                    .generatedComponentUtils().componentBundlesBasePath();
    if (bundleImportPath.isEmpty())
        return {};

    return bundleImportPath.resolvePath(bundleId);
}

} // namespace QmlDesigner
