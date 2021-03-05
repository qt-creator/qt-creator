/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/
#include "itemlibraryassetimporter.h"
#include "assetimportupdatedialog.h"
#include "qmldesignerplugin.h"
#include "qmldesignerconstants.h"

#include "rewriterview.h"
#include "model.h"
#include "puppetcreator.h"
#include "rewritertransaction.h"
#include "rewritingexception.h"

#include <utils/algorithm.h>

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
        for (const auto &pd : parseData) {
            if (!startImportProcess(pd)) {
                addError(tr("Failed to start import 3D asset process"),
                         pd.sourceInfo.absoluteFilePath());
                m_parseData.remove(pd.importId);
            }
        }
    }

    if (!isCancelled()) {
        // Wait for puppet processes to finish
        if (m_qmlPuppetProcesses.empty()) {
            postImport();
        } else {
            m_qmlPuppetCount = static_cast<int>(m_qmlPuppetProcesses.size());
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

void ItemLibraryAssetImporter::importProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)

    ++m_qmlImportFinishedCount;

    m_qmlPuppetProcesses.erase(
        std::remove_if(m_qmlPuppetProcesses.begin(), m_qmlPuppetProcesses.end(),
                       [&](const auto &entry) {
        return !entry || entry->state() == QProcess::NotRunning;
    }));

    if (m_parseData.contains(-exitCode)) {
        const ParseData pd = m_parseData.take(-exitCode);
        addError(tr("Asset import process failed for: \"%1\"").arg(pd.sourceInfo.absoluteFilePath()));
    }

    if (m_qmlImportFinishedCount == m_qmlPuppetCount) {
        notifyProgress(100);
        QTimer::singleShot(0, this, &ItemLibraryAssetImporter::postImport);
    } else {
        notifyProgress(int(100. * (double(m_qmlImportFinishedCount) / double(m_qmlPuppetCount))));
    }
}

void ItemLibraryAssetImporter::iconProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)

    m_qmlPuppetProcesses.erase(
        std::remove_if(m_qmlPuppetProcesses.begin(), m_qmlPuppetProcesses.end(),
                       [&](const auto &entry) {
        return !entry || entry->state() == QProcess::NotRunning;
    }));

    if (m_qmlPuppetProcesses.empty()) {
        notifyProgress(100);
        QTimer::singleShot(0, this, &ItemLibraryAssetImporter::finalizeQuick3DImport);
    } else {
        notifyProgress(int(100. * (1. - (double(m_qmlPuppetProcesses.size()) / double(m_qmlPuppetCount)))));
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
    m_qmlPuppetProcesses.clear();
    m_qmlPuppetCount = 0;
    m_qmlImportFinishedCount = 0;
    m_parseData.clear();
    m_requiredImports.clear();
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
    double quota = 100.0 / filePaths.count();
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
        addWarning(tr("Skipped import of duplicate asset: \"%1\"").arg(pd.assetName));
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
            addWarning(tr("Skipped import of existing asset: \"%1\"").arg(pd.assetName));
            return false;
        } else if (result == OverwriteResult::Update) {
            // Add generated icons and existing source asset file, as those will always need
            // to be overwritten
            QSet<QString> alwaysOverwrite;
            QString iconPath = pd.targetDirPath + '/' + Constants::QUICK_3D_ASSET_ICON_DIR;
            // Note: Despite the name, QUICK_3D_ASSET_LIBRARY_ICON_SUFFIX is not a traditional file
            // suffix. It's guaranteed to be in the generated icon filename, though.
            QStringList filters {QStringLiteral("*%1*").arg(Constants::QUICK_3D_ASSET_LIBRARY_ICON_SUFFIX)};
            QDirIterator iconIt(iconPath, filters, QDir::Files);
            while (iconIt.hasNext()) {
                iconIt.next();
                alwaysOverwrite.insert(iconIt.fileInfo().absoluteFilePath());
            }
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
                overwriteFiles.append(Utils::toList(alwaysOverwrite));
                m_overwrittenImports.insert(pd.targetDirPath, overwriteFiles);
            } else {
                addWarning(tr("No files selected for overwrite, skipping import: \"%1\"").arg(pd.assetName));
                return false;
            }

        } else {
            m_overwrittenImports.insert(pd.targetDirPath, {});
        }
    }

    pd.outDir.mkpath(pd.assetName);

    if (!pd.outDir.cd(pd.assetName)) {
        addError(tr("Could not access temporary asset directory: \"%1\"")
                 .arg(pd.outDir.filePath(pd.assetName)));
        return false;
    }
    return true;
}

void ItemLibraryAssetImporter::postParseQuick3DAsset(const ParseData &pd)
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
            outDir.mkdir(Constants::QUICK_3D_ASSET_ICON_DIR);
            if (qmldirFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                QString qmlInfo;
                qmlInfo.append("module ");
                qmlInfo.append(m_importPath.split('/').last());
                qmlInfo.append(".");
                qmlInfo.append(pd.assetName);
                qmlInfo.append('\n');
                m_requiredImports.append(Import::createLibraryImport(
                                             QStringLiteral("%1.%2").arg(pd.targetDir.dirName(),
                                                                         pd.assetName), version));
                while (qmlIt.hasNext()) {
                    qmlIt.next();
                    QFileInfo fi = QFileInfo(qmlIt.filePath());
                    qmlInfo.append(fi.baseName());
                    qmlInfo.append(' ');
                    qmlInfo.append(version);
                    qmlInfo.append(' ');
                    qmlInfo.append(outDir.relativeFilePath(qmlIt.filePath()));
                    qmlInfo.append('\n');

                    // Generate item library icon for qml file based on root component
                    QFile qmlFile(qmlIt.filePath());
                    if (qmlFile.open(QIODevice::ReadOnly)) {
                        QString iconFileName = outDir.path() + '/'
                                + Constants::QUICK_3D_ASSET_ICON_DIR + '/' + fi.baseName()
                                + Constants::QUICK_3D_ASSET_LIBRARY_ICON_SUFFIX;
                        QString iconFileName2x = iconFileName + "@2x";
                        QByteArray content = qmlFile.readAll();
                        int braceIdx = content.indexOf('{');
                        if (braceIdx != -1) {
                            int nlIdx = content.lastIndexOf('\n', braceIdx);
                            QByteArray rootItem = content.mid(nlIdx, braceIdx - nlIdx).trimmed();
                            if (rootItem == "Node") { // a 3D object
                                // create hints file with proper hints
                                QFile file(outDir.path() + '/' + fi.baseName() + ".hints");
                                file.open(QIODevice::WriteOnly | QIODevice::Text);
                                QTextStream out(&file);
                                out << "visibleInNavigator: true" << Qt::endl;
                                out << "canBeDroppedInFormEditor: false" << Qt::endl;
                                out << "canBeDroppedInView3D: true" << Qt::endl;
                                file.close();

                                // Add quick3D import unless it is already added
                                if (m_requiredImports.first().url() != "QtQuick3D") {
                                    QByteArray import3dStr{"import QtQuick3D"};
                                    int importIdx = content.indexOf(import3dStr);
                                    if (importIdx != -1 && importIdx < braceIdx) {
                                        importIdx += import3dStr.size();
                                        int nlIdx = content.indexOf('\n', importIdx);
                                        QByteArray versionStr = content.mid(importIdx, nlIdx - importIdx).trimmed();
                                        // There could be 'as abc' after version, so just take first part
                                        QList<QByteArray> parts = versionStr.split(' ');
                                        QString impVersion;
                                        if (parts.size() >= 1)
                                            impVersion = QString::fromUtf8(parts[0]);
                                        m_requiredImports.prepend(Import::createLibraryImport(
                                                                     "QtQuick3D", impVersion));
                                    }
                                }
                            }
                            if (startIconProcess(24, iconFileName, qmlIt.filePath())) {
                                // Since icon is generated by external process, the file won't be
                                // ready for asset gathering below, so assume its generation succeeds
                                // and add it now.
                                insertAsset(iconFileName);
                                insertAsset(iconFileName2x);
                            }
                        }
                    }
                }
                qmldirFile.write(qmlInfo.toUtf8());
                qmldirFile.commit();
            } else {
                addError(tr("Failed to create qmldir file for asset: \"%1\"").arg(pd.assetName));
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
        for (const auto &assetFiles : qAsConst(m_importFiles)) {
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

bool ItemLibraryAssetImporter::startImportProcess(const ParseData &pd)
{
    auto doc = QmlDesignerPlugin::instance()->currentDesignDocument();
    Model *model = doc ? doc->currentModel() : nullptr;

    if (model) {
        PuppetCreator puppetCreator(doc->currentTarget(), model);
        puppetCreator.createQml2PuppetExecutableIfMissing();
        QStringList puppetArgs;
        QJsonDocument optDoc(pd.options);

        puppetArgs << "--import3dAsset" << pd.sourceInfo.absoluteFilePath()
                   << pd.outDir.absolutePath() << QString::number(pd.importId)
                   << QString::fromUtf8(optDoc.toJson());

        QProcessUniquePointer process = puppetCreator.createPuppetProcess(
            "custom",
            {},
            std::function<void()>(),
            [&](int exitCode, QProcess::ExitStatus exitStatus) {
                importProcessFinished(exitCode, exitStatus);
            },
            puppetArgs);

        if (process->waitForStarted(5000)) {
            m_qmlPuppetProcesses.push_back(std::move(process));
            return true;
        } else {
            process.reset();
        }
    }
    return false;
}

bool ItemLibraryAssetImporter::startIconProcess(int size, const QString &iconFile,
                                                const QString &iconSource)
{
    auto doc = QmlDesignerPlugin::instance()->currentDesignDocument();
    Model *model = doc ? doc->currentModel() : nullptr;

    if (model) {
        PuppetCreator puppetCreator(doc->currentTarget(), model);
        puppetCreator.createQml2PuppetExecutableIfMissing();
        QStringList puppetArgs;
        puppetArgs << "--rendericon" << QString::number(size) << iconFile << iconSource;
        QProcessUniquePointer process = puppetCreator.createPuppetProcess(
            "custom",
            {},
            std::function<void()>(),
            [&](int exitCode, QProcess::ExitStatus exitStatus) {
                iconProcessFinished(exitCode, exitStatus);
            },
            puppetArgs);

        if (process->waitForStarted(5000)) {
            m_qmlPuppetProcesses.push_back(std::move(process));
            return true;
        } else {
            process.reset();
        }
    }
    return false;
}

void ItemLibraryAssetImporter::postImport()
{
    Q_ASSERT(m_qmlPuppetProcesses.empty());

    if (!isCancelled()) {
        for (const auto &pd : qAsConst(m_parseData))
            postParseQuick3DAsset(pd);
    }

    if (!isCancelled()) {
        // Wait for icon generation processes to finish
        if (m_qmlPuppetProcesses.empty()) {
            finalizeQuick3DImport();
        } else {
            m_qmlPuppetCount = static_cast<int>(m_qmlPuppetProcesses.size());
            const QString progressTitle = tr("Generating icons.");
            addInfo(progressTitle);
            notifyProgress(0, progressTitle);
        }
    }
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

            // First we have to wait a while to ensure qmljs detects new files and updates its
            // internal model. Then we make a non-change to the document to trigger qmljs snapshot
            // update. There is an inbuilt delay before rewriter change actually updates the data
            // model, so we need to wait for another moment to allow the change to take effect.
            // Otherwise subsequent subcomponent manager update won't detect new imports properly.
            QTimer *timer = new QTimer(parent());
            static int counter;
            counter = 0;
            timer->callOnTimeout([this, timer, progressTitle, model, doc]() {
                if (!isCancelled()) {
                    notifyProgress(++counter * 5, progressTitle);
                    if (counter < 10) {
                        // Do not proceed while application isn't active as the filesystem
                        // watcher qmljs uses won't trigger unless application is active
                        if (QApplication::applicationState() != Qt::ApplicationActive)
                            --counter;
                    } else if (counter == 10) {
                        model->rewriterView()->textModifier()->replace(0, 0, {});
                    } else if (counter == 19) {
                        try {
                            const QList<Import> currentImports = model->imports();
                            QList<Import> newImportsToAdd;
                            for (auto &imp : qAsConst(m_requiredImports)) {
                                if (!currentImports.contains(imp))
                                    newImportsToAdd.append(imp);
                            }
                            if (!newImportsToAdd.isEmpty()) {
                                RewriterTransaction transaction
                                        = model->rewriterView()->beginRewriterTransaction(
                                            QByteArrayLiteral("ItemLibraryAssetImporter::finalizeQuick3DImport"));

                                model->changeImports(newImportsToAdd, {});
                                transaction.commit();
                            }
                        } catch (const RewritingException &e) {
                            addError(tr("Failed to update imports: %1").arg(e.description()));
                        }
                        doc->updateSubcomponentManager();
                    } else if (counter >= 20) {
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
