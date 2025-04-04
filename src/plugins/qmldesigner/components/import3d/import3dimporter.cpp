// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "import3dimporter.h"

#include "assetimportupdatedialog.h"
#include "qmldesignerplugin.h"
#include "qmldesignerconstants.h"

#include "documentmanager.h"
#include "externaldependenciesinterface.h"
#include "model.h"
#include "puppetstarter.h"
#include "rewritertransaction.h"
#include "rewriterview.h"
#include "rewritingexception.h"
#include "viewmanager.h"

#include <modelutils.h>
#include <utils3d.h>

#ifndef QDS_USE_PROJECTSTORAGE
#include <qmljs/qmljsmodelmanagerinterface.h>
#endif

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/filepath.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QPushButton>
#include <QSaveFile>
#include <QTemporaryDir>

namespace
{
static Q_LOGGING_CATEGORY(importerLog, "qtc.itemlibrary.Import3dImporter", QtWarningMsg)
}

namespace QmlDesigner {

Import3dImporter::Import3dImporter(QObject *parent)
    : QObject (parent)
{
}

Import3dImporter::~Import3dImporter()
{
    cancelImport();
    delete m_tempDir;
};

void Import3dImporter::importQuick3D(const QStringList &inputFiles,
                                     const QString &importPath,
                                     const QVector<QJsonObject> &options,
                                     const QHash<QString, int> &extToImportOptionsMap,
                                     const QSet<QString> &preselectedFilesForOverwrite)
{
    reset();
    m_isImporting = true;

    if (!m_tempDir->isValid()) {
        addError(tr("Could not create a temporary directory for import."));
        notifyFinished();
        return;
    }

    m_importPath = importPath;

    parseFiles(inputFiles, options, extToImportOptionsMap, preselectedFilesForOverwrite);

    if (!isCancelled()) {
        const auto parseData = m_parseData;
        for (const auto &pd : parseData)
            m_puppetQueue.append(pd.importId);
        startNextImportProcess();
    }

    if (!isCancelled()) {
        // Wait for QML Puppet processes to finish
        if (m_puppetQueue.isEmpty() && !m_puppetProcess) {
            postImport();
        } else {
            const QString progressTitle = tr("Importing 3D assets.");
            addInfo(progressTitle);
            notifyProgress(0, progressTitle);
        }
    }
}

void Import3dImporter::reImportQuick3D(const QHash<QString, QJsonObject> &importOptions)
{
    m_isImporting = false;
    m_cancelled = false;

    int importId = 1;

    m_puppetProcess.reset();
    m_currentImportId = 0;
    m_puppetQueue.clear();
    m_importIdToAssetNameMap.clear();

    if (importOptions.isEmpty()) {
        addError(tr("Attempted to reimport no assets."));
        cancelImport();
        return;
    }

    const QStringList keys = importOptions.keys();
    for (const QString &key : keys) {
        if (!key.isEmpty() && !m_parseData.contains(key)) {
            addError(tr("Attempted to reimport non-existing asset: %1").arg(key));
            cancelImport();
            return;
        }

        ParseData &pd = m_parseData[key];
        // Change outDir just in case reimport generates different files
        QDir oldDir = pd.outDir;
        QString assetFolder = generateAssetFolderName(pd.assetName);
        pd.outDir.cdUp();
        pd.outDir.mkpath(assetFolder);

        if (!pd.outDir.cd(assetFolder)) {
            addError(tr("Could not access temporary asset directory: \"%1\".")
                         .arg(pd.outDir.filePath(assetFolder)));
            cancelImport();
            return;
        }

        if (oldDir.absolutePath().contains(tempDirNameBase()))
            oldDir.removeRecursively();

        for (ParseData &pd : m_parseData)
            pd.importId = -1;

        pd.options = importOptions[key];
        pd.importId = importId++;

        m_importFiles.remove(key);
        m_importIdToAssetNameMap[pd.importId] = key;

        m_puppetQueue.append(pd.importId);
    }
    startNextImportProcess();
}

bool Import3dImporter::isImporting() const
{
    return m_isImporting;
}

void Import3dImporter::cancelImport()
{
    m_cancelled = true;
    if (m_isImporting)
        notifyFinished();
}

void Import3dImporter::addError(const QString &errMsg, const QString &srcPath)
{
    qCDebug(importerLog) << "Error: "<< errMsg << srcPath;
    emit errorReported(errMsg, srcPath);
}

void Import3dImporter::addWarning(const QString &warningMsg, const QString &srcPath)
{
    qCDebug(importerLog) << "Warning: " << warningMsg << srcPath;
    emit warningReported(warningMsg, srcPath);
}

void Import3dImporter::addInfo(const QString &infoMsg, const QString &srcPath)
{
    qCDebug(importerLog) << "Info: " << infoMsg << srcPath;
    emit infoReported(infoMsg, srcPath);
}

void Import3dImporter::importProcessFinished([[maybe_unused]] int exitCode,
                                             QProcess::ExitStatus exitStatus)
{
    m_puppetProcess.reset();

    if (m_importIdToAssetNameMap.contains(m_currentImportId)) {
        const ParseData &pd = m_parseData[m_importIdToAssetNameMap[m_currentImportId]];
        QString errStr;
        if (exitStatus == QProcess::ExitStatus::CrashExit) {
            errStr = tr("Import process crashed.");
        } else {
            bool unknownFail = !pd.outDir.exists() || pd.outDir.isEmpty();
            if (!unknownFail) {
                QFile errorLog(pd.outDir.filePath("__error.log"));
                if (errorLog.exists()) {
                    if (errorLog.open(QIODevice::ReadOnly))
                        errStr = QString::fromUtf8(errorLog.readAll());
                    else
                        unknownFail = true;
                }
            }
            if (unknownFail)
                errStr = tr("Import failed for unknown reason.");
        }

        if (!errStr.isEmpty()) {
            addError(tr("Asset import process failed: \"%1\".")
                     .arg(pd.sourceInfo.absoluteFilePath()));
            addError(errStr);
            m_parseData.remove(m_importIdToAssetNameMap[m_currentImportId]);
            m_importIdToAssetNameMap.remove(m_currentImportId);
        }
    }

    int finishedCount = m_importIdToAssetNameMap.size() - m_puppetQueue.size();
    if (!m_puppetQueue.isEmpty())
        startNextImportProcess();

    if (m_puppetQueue.isEmpty() && !m_puppetProcess) {
        notifyProgress(100);
        QTimer::singleShot(0, this, &Import3dImporter::postImport);
    } else {
        notifyProgress(int(100. * (double(finishedCount) / double(m_importIdToAssetNameMap.size()))));
    }
}

void Import3dImporter::notifyFinished()
{
    m_isImporting = false;
    emit importFinished();
}

void Import3dImporter::reset()
{
    m_isImporting = false;
    m_cancelled = false;

    delete m_tempDir;
    m_tempDir = new QTemporaryDir(QDir::tempPath() + tempDirNameBase());
    m_importFiles.clear();
    m_puppetProcess.reset();
    m_parseData.clear();
    m_currentImportId = 0;
    m_puppetQueue.clear();
    m_importIdToAssetNameMap.clear();
}

void Import3dImporter::parseFiles(const QStringList &filePaths,
                                  const QVector<QJsonObject> &options,
                                  const QHash<QString, int> &extToImportOptionsMap,
                                  const QSet<QString> &preselectedFilesForOverwrite)
{
    if (isCancelled())
        return;
    const QString progressTitle = tr("Parsing files.");
    addInfo(progressTitle);
    notifyProgress(0, progressTitle);
    uint count = 0;
    double quota = 100.0 / filePaths.size();
    std::function<void(double)> progress = [this, quota, &count, &progressTitle](double value) {
        notifyProgress(qRound(quota * (count + value)), progressTitle);
    };
    for (const QString &file : filePaths) {
        int index = extToImportOptionsMap.value(QFileInfo(file).suffix());
        ParseData pd;
        pd.options = options[index];
        pd.optionsIndex = index;
        if (preParseQuick3DAsset(file, pd, preselectedFilesForOverwrite)) {
            pd.importId = ++m_importIdCounter;
            m_importIdToAssetNameMap[pd.importId] = pd.assetName;
            m_parseData.insert(pd.assetName, pd);
        }
        notifyProgress(qRound(++count * quota), progressTitle);
    }
}

bool Import3dImporter::preParseQuick3DAsset(const QString &file, ParseData &pd,
                                            const QSet<QString> &preselectedFilesForOverwrite)
{
    pd.targetDir = QDir(m_importPath);
    pd.outDir = QDir(m_tempDir->path());
    pd.sourceInfo = QFileInfo(file);
    pd.assetName = pd.sourceInfo.completeBaseName();

    if (!pd.assetName.isEmpty()) {
        // Fix name so it plays nice with imports
        for (QChar &currentChar : pd.assetName) {
            if (!currentChar.isLetter() && !currentChar.isDigit())
                currentChar = QLatin1Char('_');
        }
        const QChar firstChar = pd.assetName[0];
        if (firstChar.isDigit()) {
            // Match quick3d importer logic on starting digit
            pd.assetName.prepend("Node");
        } else if (firstChar.isLower()) {
            pd.assetName[0] = firstChar.toUpper();
        }
    }

    pd.targetDirPath = pd.targetDir.filePath(pd.assetName);

    if (m_parseData.contains(pd.assetName)) {
        addWarning(tr("Skipped import of duplicate asset: \"%1\".").arg(pd.assetName));
        return false;
    }

    pd.originalAssetName = pd.assetName;
    if (pd.targetDir.exists(pd.assetName)) {
        // If we have a file system with case insensitive filenames, assetName may be
        // different from the existing name. Modify assetName to ensure exact match to
        // the overwritten old asset capitalization
        const QStringList assetDirs = pd.targetDir.entryList({pd.assetName}, QDir::Dirs);
        if (assetDirs.size() == 1) {
            pd.assetName = assetDirs[0];
            pd.targetDirPath = pd.targetDir.filePath(pd.assetName);
        }
        OverwriteResult result = preselectedFilesForOverwrite.isEmpty()
                ? confirmAssetOverwrite(pd.assetName)
                : OverwriteResult::Update;
        if (result == OverwriteResult::Skip) {
            addWarning(tr("Skipped import of existing asset: \"%1\".").arg(pd.assetName));
            return false;
        } else if (result == OverwriteResult::Update) {
            // Add existing source asset file, as that will always need to be overwritten
            QSet<QString> alwaysOverwrite;
            alwaysOverwrite.insert(sourceSceneTargetFilePath(pd));
            alwaysOverwrite.insert(pd.targetDirPath + '/' + Constants::QUICK_3D_ASSET_IMPORT_DATA_NAME);

            Internal::AssetImportUpdateDialog dlg {pd.targetDirPath,
                                                   preselectedFilesForOverwrite,
                                                   alwaysOverwrite,
                                                   qobject_cast<QWidget *>(parent())};
            int exitVal = dlg.exec();

            QStringList overwriteFiles;
            if (exitVal == QDialog::Accepted)
                overwriteFiles = dlg.selectedFiles();
            if (!overwriteFiles.isEmpty()) {
                overwriteFiles.append(::Utils::toList(alwaysOverwrite));
                pd.overwrittenImports.insert(pd.targetDirPath, overwriteFiles);
            } else {
                addWarning(tr("No files selected for overwrite, skipping import: \"%1\".").arg(pd.assetName));
                return false;
            }

        } else {
            pd.overwrittenImports.insert(pd.targetDirPath, {});
        }
    }

    QString assetFolder = generateAssetFolderName(pd.assetName);
    pd.outDir.mkpath(assetFolder);

    if (!pd.outDir.cd(assetFolder)) {
        addError(tr("Could not access temporary asset directory: \"%1\".")
                 .arg(pd.outDir.filePath(assetFolder)));
        return false;
    }
    return true;
}

void Import3dImporter::postParseQuick3DAsset(ParseData &pd)
{
    QDir outDir = pd.outDir;
    if (pd.originalAssetName != pd.assetName) {
        // Fix the generated qml file name
        const QString assetQml = pd.originalAssetName + ".qml";
        if (outDir.exists(assetQml))
            outDir.rename(assetQml, pd.assetName + ".qml");
    }

    QHash<QString, QString> assetFiles;
    const int outDirPathSize = outDir.path().size();
    auto insertAsset = [&](const QString &filePath) {
        QString targetPath = filePath.mid(outDirPathSize);
        targetPath.prepend(pd.targetDirPath);
        assetFiles.insert(filePath, targetPath);
    };

    // Generate qmldir file if importer doesn't already make one
    QString qmldirFileName = outDir.absoluteFilePath(QStringLiteral("qmldir"));
    if (!QFileInfo::exists(qmldirFileName)) {
        QSaveFile qmldirFile(qmldirFileName);
        QString version = QStringLiteral("1.0");

        // Note: Currently Quick3D importers only generate externally usable qml files on the top
        // level of the import directory, so we don't search subdirectories. The qml files in
        // subdirs assume they are used within the context of the toplevel qml files.
        QDirIterator qmlIt(outDir.path(), {QStringLiteral("*.qml")}, QDir::Files);
        if (qmlIt.hasNext()) {
            if (qmldirFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                QString qmlInfo;
                qmlInfo.append("module ");
                qmlInfo.append(QmlDesignerPlugin::instance()->documentManager()
                                   .generatedComponentUtils().import3dTypePrefix());
                qmlInfo.append(".");
                qmlInfo.append(pd.assetName);
                qmlInfo.append('\n');
                while (qmlIt.hasNext()) {
                    qmlIt.next();
                    QFileInfo fi = QFileInfo(qmlIt.filePath());
                    if (pd.importedQmlName.isEmpty())
                        pd.importedQmlName = fi.baseName();
                    qmlInfo.append(fi.baseName());
                    qmlInfo.append(' ');
                    qmlInfo.append(version);
                    qmlInfo.append(' ');
                    qmlInfo.append(outDir.relativeFilePath(qmlIt.filePath()));
                    qmlInfo.append('\n');

                    QFile qmlFile(qmlIt.filePath());
                    if (qmlFile.open(QIODevice::ReadOnly)) {
                        QByteArray content = qmlFile.readAll();
                        int braceIdx = content.indexOf('{');
                        QString impVersionStr;
                        int impVersionMajor = -1;
                        if (braceIdx != -1) {
                            int nlIdx = content.lastIndexOf('\n', braceIdx);
                            QByteArray rootItem = content.mid(nlIdx, braceIdx - nlIdx).trimmed();
                            if (rootItem == "Node" || rootItem == "Model") { // a 3D object
                                // create hints file with proper hints
                                QFile file(outDir.path() + '/' + fi.baseName() + ".hints");
                                file.open(QIODevice::WriteOnly | QIODevice::Text);
                                QTextStream out(&file);
                                out << "visibleInNavigator: true" << Qt::endl;
                                out << "canBeDroppedInFormEditor: false" << Qt::endl;
                                out << "canBeDroppedInView3D: true" << Qt::endl;
                                file.close();

                                // Assume that all assets import the same QtQuick3D version,
                                // since they are being imported with same kit
                                if (impVersionMajor == -1) {
                                    QByteArray import3dStr{"import QtQuick3D"};
                                    int importIdx = content.indexOf(import3dStr);
                                    if (importIdx != -1 && importIdx < braceIdx) {
                                        importIdx += import3dStr.size();
                                        int nlIdx = content.indexOf('\n', importIdx);
                                        QByteArray versionStr = content.mid(importIdx, nlIdx - importIdx).trimmed();
                                        // There could be 'as abc' after version, so just take first part
                                        QList<QByteArray> parts = versionStr.split(' ');
                                        if (parts.size() >= 1) {
                                            impVersionStr = QString::fromUtf8(parts[0]);
                                            if (impVersionStr.isEmpty())
                                                impVersionMajor = 6;
                                            else
                                                impVersionMajor = impVersionStr.left(1).toInt();
                                        }
                                    }
                                }
                           }
                        }
                    }
                }
                qmldirFile.write(qmlInfo.toUtf8());
                qmldirFile.commit();
            } else {
                addError(tr("Failed to create qmldir file for asset: \"%1\".").arg(pd.assetName));
            }
        }
    }

    // Generate import metadata file
    const QString sourcePath = pd.sourceInfo.absoluteFilePath();
    QString importDataFileName = outDir.absoluteFilePath(Constants::QUICK_3D_ASSET_IMPORT_DATA_NAME);
    QSaveFile importDataFile(importDataFileName);
    if (importDataFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QJsonObject optObj;
        optObj.insert(Constants::QUICK_3D_ASSET_IMPORT_DATA_OPTIONS_KEY, pd.options);
        optObj.insert(Constants::QUICK_3D_ASSET_IMPORT_DATA_SOURCE_KEY, sourcePath);
        importDataFile.write(QJsonDocument{optObj}.toJson());
        importDataFile.commit();
    }

    // Gather all generated files
    QDirIterator dirIt(outDir.path(), QDir::Files, QDirIterator::Subdirectories);
    pd.assetSize = 0;
    while (dirIt.hasNext()) {
        dirIt.next();
        insertAsset(dirIt.filePath());
        QFileInfo fi = dirIt.fileInfo();
        if (fi.isFile())
            pd.assetSize += fi.size();
    }

    // Copy the original asset into a subdirectory
    assetFiles.insert(sourcePath, sourceSceneTargetFilePath(pd));
    m_importFiles.insert(pd.assetName, assetFiles);
}

void Import3dImporter::copyImportedFiles()
{
    QHash<QString, QStringList> allOverwrites;
    for (const ParseData &pd: std::as_const(m_parseData))
        allOverwrites.insert(pd.overwrittenImports);
    if (!allOverwrites.isEmpty()) {
        const QString progressTitle = tr("Removing old overwritten assets.");
        addInfo(progressTitle);
        notifyProgress(0, progressTitle);

        int counter = 0;
        auto it = allOverwrites.constBegin();
        while (it != allOverwrites.constEnd()) {
            Utils::FilePath dir = Utils::FilePath::fromUserInput(it.key());
            if (dir.exists()) {
                const auto &overwrittenFiles = it.value();
                if (overwrittenFiles.isEmpty()) {
                    // Overwrite entire import
                    dir.removeRecursively();
                } else {
                    // Overwrite just selected files
                    for (const auto &fileName : overwrittenFiles)
                        QFile::remove(fileName);
                }
            }
            notifyProgress((100 * ++counter) / allOverwrites.size(), progressTitle);
            ++it;
        }
    }

    if (!m_importFiles.isEmpty()) {
        const QString progressTitle = tr("Copying asset files.");
        addInfo(progressTitle);
        notifyProgress(0, progressTitle);

        int counter = 0;
        for (const auto &assetFiles : std::as_const(m_importFiles)) {
            // Only increase progress between entire assets instead of individual files, because
            // progress notify leads to processEvents call, which can lead to various filesystem
            // watchers triggering while library is still incomplete, leading to inconsistent model.
            // This also speeds up the copying as incomplete folder is not parsed unnecessarily
            // by filesystem watchers.
            QHash<QString, QString>::const_iterator it = assetFiles.begin();
            while (it != assetFiles.end()) {
                if (QFileInfo::exists(it.key()) && !QFileInfo::exists(it.value())) {
                    QDir targetDir = QFileInfo(it.value()).dir();
                    if (!targetDir.exists())
                        targetDir.mkpath(".");
                    QFile::copy(it.key(), it.value());
                }
                ++it;
            }
            notifyProgress((100 * ++counter) / m_importFiles.size(), progressTitle);
        }
        notifyProgress(100, progressTitle);
    }
}

void Import3dImporter::notifyProgress(int value, const QString &text)
{
    m_progressTitle = text;
    emit progressChanged(value, m_progressTitle);
    keepUiAlive();
}

void Import3dImporter::notifyProgress(int value)
{
    notifyProgress(value, m_progressTitle);
}

void Import3dImporter::keepUiAlive() const
{
    QApplication::processEvents();
}

QString Import3dImporter::generateAssetFolderName(const QString &assetName) const
{
    static int counter = 0;
    return assetName + "_QDS_" + QString::number(counter++);
}

Import3dImporter::OverwriteResult Import3dImporter::confirmAssetOverwrite(const QString &assetName)
{
    const QString title = tr("Overwrite Existing Asset?");
    const QString question = tr("Asset already exists. Overwrite existing or skip?\n\"%1\"").arg(assetName);

    QMessageBox msgBox {QMessageBox::Question, title, question, QMessageBox::NoButton,
                        qobject_cast<QWidget *>(parent())};
    QPushButton *updateButton = msgBox.addButton(tr("Overwrite Selected Files"), QMessageBox::NoRole);
    QPushButton *overwriteButton = msgBox.addButton(tr("Overwrite All Files"), QMessageBox::NoRole);
    QPushButton *skipButton = msgBox.addButton(tr("Skip"), QMessageBox::NoRole);
    msgBox.setDefaultButton(overwriteButton);
    msgBox.setEscapeButton(skipButton);

    msgBox.exec();

    if (msgBox.clickedButton() == updateButton)
        return OverwriteResult::Update;
    else if (msgBox.clickedButton() == overwriteButton)
        return OverwriteResult::Overwrite;
    return OverwriteResult::Skip;
}

void Import3dImporter::startNextImportProcess()
{
    if (m_puppetQueue.isEmpty())
        return;

    auto view = QmlDesignerPlugin::viewManager().view();
    auto doc = QmlDesignerPlugin::instance()->currentDesignDocument();
    Model *model = doc ? doc->currentModel() : nullptr;

    if (model && view) {
        bool done = false;
        while (!m_puppetQueue.isEmpty() && !done) {
            const ParseData pd = m_parseData.value(
                m_importIdToAssetNameMap.value(m_puppetQueue.takeLast()));
            QStringList puppetArgs;
            QJsonDocument optDoc(pd.options);

            puppetArgs << "--import3dAsset" << pd.sourceInfo.absoluteFilePath()
                       << pd.outDir.absolutePath() << QString::fromUtf8(optDoc.toJson());

            m_currentImportId = pd.importId;
            m_puppetProcess = PuppetStarter::createPuppetProcess(
                view->externalDependencies().puppetStartData(*model),
                "custom",
                {},
                [&] {},
                [&](int exitCode, QProcess::ExitStatus exitStatus) {
                    importProcessFinished(exitCode, exitStatus);
                },
                puppetArgs);

            if (m_puppetProcess->waitForStarted(10000)) {
                done = true;
            } else {
                addError(tr("Failed to start import 3D asset process."),
                         pd.sourceInfo.absoluteFilePath());
                const QString assetName = m_importIdToAssetNameMap.take(pd.importId);
                m_parseData.remove(assetName);
                m_puppetProcess.reset();
            }
        }
    }
}

void Import3dImporter::postImport()
{
    QTC_ASSERT(m_puppetQueue.isEmpty() && !m_puppetProcess, return);

    if (!isCancelled()) {
        for (auto &pd : m_parseData)
            postParseQuick3DAsset(pd);
    }

    if (!isCancelled()) {
        QList<PreviewData> dataList;
        for (const QString &assetName : std::as_const(m_importIdToAssetNameMap)) {
            const ParseData &pd = m_parseData[assetName];
            PreviewData data;
            data.name = pd.assetName;
            data.folderName = pd.outDir.dirName();
            data.qmlName = pd.importedQmlName;
            data.renderedOptions = pd.options;
            data.currentOptions = pd.options;
            data.optionsIndex = pd.optionsIndex;
            data.type = pd.sourceInfo.suffix().toLower();
            data.size = pd.assetSize;

            bool inserted = false;
            for (int i = 0; i < dataList.size(); ++i) {
                if (dataList[i].name.compare(data.name, Qt::CaseInsensitive) > 0) {
                    dataList.insert(i, data);
                    inserted = true;
                    break;
                }
            }
            if (!inserted)
                dataList.append(data);
        }

        emit importReadyForPreview(m_tempDir->path(), dataList);
    }
}

void Import3dImporter::finalizeQuick3DImport()
{
    if (!isCancelled()) {
        // Don't allow cancel anymore as existing asset overwrites are not trivially recoverable.
        // Also, on Windows at least you can't delete a subdirectory of a watched directory,
        // so complete rollback is no longer possible in any case.
        emit importNearlyFinished();

        copyImportedFiles();

        auto doc = QmlDesignerPlugin::instance()->currentDesignDocument();
        Model *model = doc ? doc->currentModel() : nullptr;
        if (model && !m_importFiles.isEmpty()) {
            const QString progressTitle = tr("Updating data model.");
            addInfo(progressTitle);
            notifyProgress(0, progressTitle);

            QTimer *timer = new QTimer(parent());
            static int counter;
            counter = 0;

#ifndef QDS_USE_PROJECTSTORAGE
            auto modelManager = QmlJS::ModelManagerInterface::instance();
            QFuture<void> result;
            if (modelManager) {
                QmlJS::PathsAndLanguages pathToScan;
                pathToScan.maybeInsert(::Utils::FilePath::fromString(m_importPath));
                result = ::Utils::asyncRun(&QmlJS::ModelManagerInterface::importScan,
                                           modelManager->workingCopy(),
                                           pathToScan,
                                           modelManager,
                                           true,
                                           true,
                                           true);
            }
            // First we have to wait a while to ensure qmljs detects new files and updates its
            // internal model. Then we force amend on rewriter to trigger qmljs snapshot update.
            timer->callOnTimeout([this, timer, progressTitle, model, result]() {
                if (!isCancelled()) {
                    notifyProgress(++counter * 2, progressTitle);
                    if (counter == 1) {
                        if (!Utils3D::addQuick3DImportAndView3D(model->rewriterView(), true))
                            addError(tr("Failed to insert QtQuick3D import to the qml document."));
                    } else if (counter < 50) {
                        if (result.isCanceled() || result.isFinished())
                            counter = 49; // skip to next step
                    } else if (counter >= 50) {
                        for (const ParseData &pd : std::as_const(m_parseData)) {
                            if (!pd.overwrittenImports.isEmpty()) {
                                model->rewriterView()->resetPuppet();
                                model->rewriterView()->emitCustomNotification("asset_import_update");
                                break;
                            }
                        }
                        timer->stop();
                        notifyFinished();
                        model->rewriterView()->emitCustomNotification("asset_import_finished");
                    }
#else
            counter = 0;
            timer->callOnTimeout([this, timer, progressTitle, model]() {
                if (!isCancelled()) {
                    notifyProgress(++counter * 50, progressTitle);
                    if (counter == 1) {
                        if (!Utils3D::addQuick3DImportAndView3D(model->rewriterView(), true))
                            addError(tr("Failed to insert QtQuick3D import to the qml document."));
                    } else if (counter > 1) {
                        for (const ParseData &pd : std::as_const(m_parseData)) {
                            if (!pd.overwrittenImports.isEmpty()) {
                                model->rewriterView()->resetPuppet();
                                model->rewriterView()->emitCustomNotification("asset_import_update");
                                break;
                            }
                        }
                        timer->stop();
                        notifyFinished();
                        model->rewriterView()->emitCustomNotification("asset_import_finished");
                    }
#endif
                } else {
                    timer->stop();
                }
            });
            timer->start(100);
        } else {
            notifyFinished();
        }
    }
}

void Import3dImporter::removeAssetFromImport(const QString &assetName)
{
    m_parseData.remove(assetName);
    m_importFiles.remove(assetName);
}

QString Import3dImporter::sourceSceneTargetFilePath(const ParseData &pd)
{
    return pd.targetDirPath + QStringLiteral("/source scene/") + pd.sourceInfo.fileName();
}

bool Import3dImporter::isCancelled() const
{
    keepUiAlive();
    return m_cancelled;
}

} // QmlDesigner
