// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "itemlibraryassetimporter.h"
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

#include <model/modelutils.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <utils/algorithm.h>
#include <utils/async.h>
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
static Q_LOGGING_CATEGORY(importerLog, "qtc.itemlibrary.assetImporter", QtWarningMsg)
}

namespace QmlDesigner {

ItemLibraryAssetImporter::ItemLibraryAssetImporter(QObject *parent) :
    QObject (parent)
{
}

ItemLibraryAssetImporter::~ItemLibraryAssetImporter() {
    cancelImport();
    delete m_tempDir;
};

void ItemLibraryAssetImporter::importQuick3D(const QStringList &inputFiles,
                                             const QString &importPath,
                                             const QVector<QJsonObject> &options,
                                             const QHash<QString, int> &extToImportOptionsMap,
                                             const QSet<QString> &preselectedFilesForOverwrite)
{
    if (m_isImporting)
        cancelImport();
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
        // Wait for puppet processes to finish
        if (m_puppetQueue.isEmpty() && !m_puppetProcess) {
            postImport();
        } else {
            const QString progressTitle = tr("Importing 3D assets.");
            addInfo(progressTitle);
            notifyProgress(0, progressTitle);
        }
    }
}

bool ItemLibraryAssetImporter::isImporting() const
{
    return m_isImporting;
}

void ItemLibraryAssetImporter::cancelImport()
{
    m_cancelled = true;
    if (m_isImporting)
        notifyFinished();
}

void ItemLibraryAssetImporter::addError(const QString &errMsg, const QString &srcPath) const
{
    qCDebug(importerLog) << "Error: "<< errMsg << srcPath;
    emit errorReported(errMsg, srcPath);
}

void ItemLibraryAssetImporter::addWarning(const QString &warningMsg, const QString &srcPath) const
{
    qCDebug(importerLog) << "Warning: " << warningMsg << srcPath;
    emit warningReported(warningMsg, srcPath);
}

void ItemLibraryAssetImporter::addInfo(const QString &infoMsg, const QString &srcPath) const
{
    qCDebug(importerLog) << "Info: " << infoMsg << srcPath;
    emit infoReported(infoMsg, srcPath);
}

void ItemLibraryAssetImporter::importProcessFinished([[maybe_unused]] int exitCode,
                                                     QProcess::ExitStatus exitStatus)
{
    m_puppetProcess.reset();

    if (m_parseData.contains(m_currentImportId)) {
        const ParseData &pd = m_parseData[m_currentImportId];
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
            m_parseData.remove(m_currentImportId);
        }
    }

    int finishedCount = m_parseData.size() - m_puppetQueue.size();
    if (!m_puppetQueue.isEmpty())
        startNextImportProcess();

    if (m_puppetQueue.isEmpty() && !m_puppetProcess) {
        notifyProgress(100);
        QTimer::singleShot(0, this, &ItemLibraryAssetImporter::postImport);
    } else {
        notifyProgress(int(100. * (double(finishedCount) / double(m_parseData.size()))));
    }
}

void ItemLibraryAssetImporter::notifyFinished()
{
    m_isImporting = false;
    emit importFinished();
}

void ItemLibraryAssetImporter::reset()
{
    m_isImporting = false;
    m_cancelled = false;

    delete m_tempDir;
    m_tempDir = new QTemporaryDir;
    m_importFiles.clear();
    m_overwrittenImports.clear();
    m_puppetProcess.reset();
    m_parseData.clear();
    m_requiredImports.clear();
    m_currentImportId = 0;
    m_puppetQueue.clear();
}

void ItemLibraryAssetImporter::parseFiles(const QStringList &filePaths,
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
        if (preParseQuick3DAsset(file, pd, preselectedFilesForOverwrite)) {
            pd.importId = ++m_importIdCounter;
            m_parseData.insert(pd.importId, pd);
        }
        notifyProgress(qRound(++count * quota), progressTitle);
    }
}

bool ItemLibraryAssetImporter::preParseQuick3DAsset(const QString &file, ParseData &pd,
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
        if (firstChar.isDigit())
            pd.assetName[0] = QLatin1Char('_');
        if (firstChar.isLower())
            pd.assetName[0] = firstChar.toUpper();
    }

    pd.targetDirPath = pd.targetDir.filePath(pd.assetName);

    if (pd.outDir.exists(pd.assetName)) {
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
                m_overwrittenImports.insert(pd.targetDirPath, overwriteFiles);
            } else {
                addWarning(tr("No files selected for overwrite, skipping import: \"%1\".").arg(pd.assetName));
                return false;
            }

        } else {
            m_overwrittenImports.insert(pd.targetDirPath, {});
        }
    }

    pd.outDir.mkpath(pd.assetName);

    if (!pd.outDir.cd(pd.assetName)) {
        addError(tr("Could not access temporary asset directory: \"%1\".")
                 .arg(pd.outDir.filePath(pd.assetName)));
        return false;
    }
    return true;
}

void ItemLibraryAssetImporter::postParseQuick3DAsset(ParseData &pd)
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
                qmlInfo.append(m_importPath.split('/').last());
                qmlInfo.append(".");
                qmlInfo.append(pd.assetName);
                qmlInfo.append('\n');
                m_requiredImports.append(
                    QStringLiteral("%1.%2").arg(pd.targetDir.dirName(), pd.assetName));
                while (qmlIt.hasNext()) {
                    qmlIt.next();
                    QFileInfo fi = QFileInfo(qmlIt.filePath());
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

                                // Add quick3D import unless it is already added
                                if (impVersionMajor > 0 && m_requiredImports.first() != "QtQuick3D")
                                    m_requiredImports.prepend("QtQuick3D");
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
    while (dirIt.hasNext()) {
        dirIt.next();
        insertAsset(dirIt.filePath());
    }

    // Copy the original asset into a subdirectory
    assetFiles.insert(sourcePath, sourceSceneTargetFilePath(pd));
    m_importFiles.insert(assetFiles);
}

void ItemLibraryAssetImporter::copyImportedFiles()
{
    if (!m_overwrittenImports.isEmpty()) {
        const QString progressTitle = tr("Removing old overwritten assets.");
        addInfo(progressTitle);
        notifyProgress(0, progressTitle);

        int counter = 0;
        auto it = m_overwrittenImports.constBegin();
        while (it != m_overwrittenImports.constEnd()) {
            QDir dir(it.key());
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
            notifyProgress((100 * ++counter) / m_overwrittenImports.size(), progressTitle);
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

void ItemLibraryAssetImporter::notifyProgress(int value, const QString &text)
{
    m_progressTitle = text;
    emit progressChanged(value, m_progressTitle);
    keepUiAlive();
}

void ItemLibraryAssetImporter::notifyProgress(int value)
{
    notifyProgress(value, m_progressTitle);
}

void ItemLibraryAssetImporter::keepUiAlive() const
{
    QApplication::processEvents();
}

ItemLibraryAssetImporter::OverwriteResult ItemLibraryAssetImporter::confirmAssetOverwrite(const QString &assetName)
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

void ItemLibraryAssetImporter::startNextImportProcess()
{
    if (m_puppetQueue.isEmpty())
        return;

    auto view = QmlDesignerPlugin::viewManager().view();
    auto doc = QmlDesignerPlugin::instance()->currentDesignDocument();
    Model *model = doc ? doc->currentModel() : nullptr;

    if (model && view) {
        bool done = false;
        while (!m_puppetQueue.isEmpty() && !done) {
            const ParseData pd = m_parseData.value(m_puppetQueue.takeLast());
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
                m_parseData.remove(pd.importId);
                m_puppetProcess.reset();
            }
        }
    }
}

void ItemLibraryAssetImporter::postImport()
{
    QTC_ASSERT(m_puppetQueue.isEmpty() && !m_puppetProcess, return);

    if (!isCancelled()) {
        for (auto &pd : m_parseData)
            postParseQuick3DAsset(pd);
    }

    if (!isCancelled())
        finalizeQuick3DImport();
}

void ItemLibraryAssetImporter::finalizeQuick3DImport()
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
            QTimer *timer = new QTimer(parent());
            static int counter;
            counter = 0;

            timer->callOnTimeout([this, timer, progressTitle, model, result]() {
                if (!isCancelled()) {
                    notifyProgress(++counter * 2, progressTitle);
                    if (counter < 49) {
                        if (result.isCanceled() || result.isFinished())
                            counter = 48; // skip to next step
                    } else if (counter == 49) {
                        QmlDesignerPlugin::instance()->documentManager().resetPossibleImports();
                        model->rewriterView()->forceAmend();
                        try {
                            RewriterTransaction transaction = model->rewriterView()->beginRewriterTransaction(
                                QByteArrayLiteral("ItemLibraryAssetImporter::finalizeQuick3DImport"));
                            bool success = ModelUtils::addImportsWithCheck(m_requiredImports, model);
                            if (!success)
                                addError(tr("Failed to insert import statement into qml document."));
                            transaction.commit();
                        } catch (const RewritingException &e) {
                            addError(tr("Failed to update imports: %1").arg(e.description()));
                        }
                    } else if (counter >= 50) {
                        if (!m_overwrittenImports.isEmpty())
                            model->rewriterView()->emitCustomNotification("asset_import_update");
                        timer->stop();
                        notifyFinished();
                    }
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

QString ItemLibraryAssetImporter::sourceSceneTargetFilePath(const ParseData &pd)
{
    return pd.targetDirPath + QStringLiteral("/source scene/") + pd.sourceInfo.fileName();
}

bool ItemLibraryAssetImporter::isCancelled() const
{
    keepUiAlive();
    return m_cancelled;
}

} // QmlDesigner
