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
#ifndef QDS_USE_PROJECTSTORAGE
#include <qmljs/qmljsmodelmanagerinterface.h>
#endif
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

#ifndef QDS_USE_PROJECTSTORAGE
    bool doScan = false;
    bool doReset = false;
#endif
    FilePath qmldirPath = bundleImportPath.pathAppended("qmldir");
    QString qmldirContent = QString::fromUtf8(qmldirPath.fileContents().value_or(QByteArray()));
    if (qmldirContent.isEmpty()) {
        qmldirContent.append("module ");
        qmldirContent.append(module);
        qmldirContent.append('\n');
#ifndef QDS_USE_PROJECTSTORAGE
        doScan = true;
#endif
    }

    FilePath qmlSourceFile = bundleImportPath.pathAppended(qmlFile);
    const QString qmlType = qmlSourceFile.baseName();

    if (m_pendingImports.contains(type) && !m_pendingImports[type].isImport)
        return QStringLiteral("Unable to import while unimporting the same type: '%1'").arg(QLatin1String(type));

    if (!qmldirContent.contains(qmlFile)) {
        qmldirContent.append(qmlType);
        qmldirContent.append(" 1.0 ");
        qmldirContent.append(qmlFile);
        qmldirContent.append('\n');
        qmldirPath.writeFileContents(qmldirContent.toUtf8());
#ifndef QDS_USE_PROJECTSTORAGE
        doReset = true;
#endif
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
    data.type = type;
    Import import = Import::createLibraryImport(module, "1.0");
#ifdef QDS_USE_PROJECTSTORAGE
    model->changeImports({import}, {});
#else
    if (doScan)
        data.pathToScan = bundleImportPath;
    else
        data.fullReset = doReset;

    if (!model->hasImport(import)) {
        if (model->possibleImports().contains(import)) {
            try {
                model->changeImports({import}, {});
            } catch (const RewritingException &) {
                // No point in trying to add import asynchronously either, so just fail out
                return QStringLiteral("Failed to add import statement for: '%1'").arg(module);
            }
        } else {
            // If import is not yet possible, import statement needs to be added asynchronously to
            // avoid errors, as code model update takes a while.
            data.importToAdd = module;
        }
    }
#endif
    m_pendingImports.insert(type, data);
    m_importTimerCount = 0;
    m_importTimer.start(normalImportDelay);

    return {};
}

void BundleImporter::handleImportTimer()
{
#ifdef QDS_USE_PROJECTSTORAGE
    auto handleFailure = [this] {
        m_importTimer.stop();
        m_importTimerCount = 0;

        // Emit dummy finished signals for all pending types
        const QList<TypeName> pendingTypes = m_pendingImports.keys();
        for (const TypeName &pendingType : pendingTypes) {
            ImportData data = m_pendingImports.take(pendingType);
            if (data.isImport)
                emit importFinished({}, m_bundleId);
            else
                emit unimportFinished({}, m_bundleId);
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
        ImportData &data = m_pendingImports[type];
        // Verify that code model has the new type fully available (or removed for unimport)
        NodeMetaInfo metaInfo = model->metaInfo(type);
        const bool typeComplete = metaInfo.isValid() && !metaInfo.prototypes().empty();
        if (data.isImport == typeComplete) {
            m_pendingImports.remove(type);
            if (data.isImport)
                emit importFinished(type, m_bundleId);
            else
                emit unimportFinished(metaInfo, m_bundleId);
        }
    }

    if (m_pendingImports.isEmpty()) {
        m_bundleId.clear();
        m_importTimer.stop();
        m_importTimerCount = 0;
    }
#else
    auto handleFailure = [this] {
        m_importTimer.stop();
        m_importTimerCount = 0;

        disconnect(m_libInfoConnection);

        // Emit dummy finished signals for all pending types
        const QList<TypeName> pendingTypes = m_pendingImports.keys();
        for (const TypeName &pendingType : pendingTypes) {
            ImportData data = m_pendingImports.take(pendingType);
            if (data.isImport)
                emit importFinished({}, m_bundleId);
            else
                emit unimportFinished({}, m_bundleId);
        }
        m_bundleId.clear();
    };

    auto doc = QmlDesignerPlugin::instance()->currentDesignDocument();
    Model *model = doc ? doc->currentModel() : nullptr;
    if (!model || ++m_importTimerCount > 100) {
        handleFailure();
        return;
    }

    auto modelManager = QmlJS::ModelManagerInterface::instance();
    if (modelManager) {
        const QList<TypeName> keys = m_pendingImports.keys();
        int scanDone = 0;
        bool refreshImports = false;
        for (const TypeName &type : keys) {
            ImportData &data = m_pendingImports[type];
            if (data.state == ImportData::Starting) {
                if (data.pathToScan.isEmpty()) {
                    data.state = ImportData::FullReset;
                } else {
                    // A new bundle module was added, so we can scan it without doing full code
                    // model reset, which is faster
                    QmlJS::PathsAndLanguages pathToScan;
                    pathToScan.maybeInsert(data.pathToScan);
                    data.future = Utils::asyncRun(&QmlJS::ModelManagerInterface::importScan,
                                                  modelManager->workingCopy(),
                                                  pathToScan, modelManager,
                                                  true, true, true);
                    data.state = ImportData::WaitingForImportScan;
                }
            } else if (data.state == ImportData::WaitingForImportScan) {
                // Import scan is asynchronous, so we need to wait for it to be finished
                if ((data.future.isCanceled() || data.future.isFinished()))
                    data.state = ImportData::RefreshImports;
            } else if (data.state >= ImportData::RefreshImports) {
                // Do not mode to next stage until all pending scans are done
                ++scanDone;
                if (data.state == ImportData::RefreshImports)
                    refreshImports = true;
            }
        }
        if (scanDone != m_pendingImports.size()) {
            return;
        } else {
            if (refreshImports) {
                // The new import is now available in code model, so reset the possible imports
                QmlDesignerPlugin::instance()->documentManager().resetPossibleImports();
                model->rewriterView()->forceAmend();

                for (const TypeName &type : keys) {
                    ImportData &data = m_pendingImports[type];
                    if (data.state == ImportData::RefreshImports) {
                        if (!data.importToAdd.isEmpty()) {
                            try {
                                RewriterTransaction transaction = model->rewriterView()->beginRewriterTransaction(
                                    QByteArrayLiteral(__FUNCTION__));
                                bool success = ModelUtils::addImportWithCheck(data.importToAdd, model);
                                if (!success)
                                    handleFailure();
                                transaction.commit();
                            } catch (const RewritingException &) {
                                handleFailure();
                            }
                        }
                    }
                    data.state = ImportData::FullReset;
                }
                return;
            }

            bool fullReset = false;
            for (const TypeName &type : keys) {
                ImportData &data = m_pendingImports[type];
                if (data.state == ImportData::FullReset) {
                    if (data.fullReset)
                        fullReset = true;
                    data.state = ImportData::Finalize;
                }
            }
            if (fullReset) {
                // Force code model reset to notice changes to existing module
                auto modelManager = QmlJS::ModelManagerInterface::instance();
                if (modelManager) {
                    modelManager->resetCodeModel();
                    disconnect(m_libInfoConnection);
                    m_libInfoConnection = connect(modelManager, &QmlJS::ModelManagerInterface::libraryInfoUpdated,
                        this, [this]() {
                            // This signal comes for each library in code model, so we need to compress
                            // it until no more notifications come
                            m_importTimer.start(1000);
                        }, Qt::QueuedConnection);
                    // Stop the import timer for a bit to allow full reset to complete
                    m_importTimer.stop();
                }
                return;
            }

            for (const TypeName &type : keys) {
                ImportData &data = m_pendingImports[type];
                if (data.state == ImportData::Finalize) {
                     // Reset delay just in case full reset was done earlier
                    m_importTimer.start(normalImportDelay);
                    disconnect(m_libInfoConnection);
                    model->rewriterView()->forceAmend();
                    // Verify that code model has the new material fully available (or removed for unimport)
                    NodeMetaInfo metaInfo = model->metaInfo(type);
                    const bool typeComplete = metaInfo.isValid() && !metaInfo.prototypes().empty();
                    if (data.isImport == typeComplete) {
                        m_pendingImports.remove(type);
                        if (data.isImport)
                            emit importFinished(metaInfo, m_bundleId);
                        else
                            emit unimportFinished(metaInfo, m_bundleId);
                    }
                }
            }
        }
    }

    if (m_pendingImports.isEmpty()) {
        m_bundleId.clear();
        m_importTimer.stop();
        m_importTimerCount = 0;
        disconnect(m_libInfoConnection);
    }
#endif
}

QVariantHash BundleImporter::loadAssetRefMap(const FilePath &bundlePath)
{
    FilePath assetRefPath = bundlePath.resolvePath(QLatin1String(Constants::COMPONENT_BUNDLES_ASSET_REF_FILE));
    const expected_str<QByteArray> content = assetRefPath.fileContents();
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

    emit aboutToUnimport(type, m_bundleId);

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
    const expected_str<QByteArray> qmldirContent = qmldirPath.fileContents();
    QByteArray newContent;

    QString qmlType = qmlFilePath.baseName();
    if (m_pendingImports.contains(type) && m_pendingImports[type].isImport) {
        return QStringLiteral("Unable to unimport while importing the same type: '%1'")
            .arg(QString::fromLatin1(type));
    }

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
                return QStringLiteral("Failed to write qmldir file: '%1'").arg(qmldirPath.toUrlishString());
        }
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
            Import import = Import::createLibraryImport(module, "1.0");
            if (model->imports().contains(import))
                model->changeImports({}, {import});
        }
    }

    ImportData data;
    data.isImport = false;
    data.type = type;
#ifndef QDS_USE_PROJECTSTORAGE
    data.fullReset = true;
#endif
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
