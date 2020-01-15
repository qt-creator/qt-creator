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
#include "qmldesignerplugin.h"
#include "qmldesignerconstants.h"

#include "rewriterview.h"
#include "model.h"

#include <QtCore/qdir.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qsavefile.h>
#include <QtCore/qfile.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qtemporarydir.h>
#include <QtWidgets/qapplication.h>
#include <QtWidgets/qmessagebox.h>

#ifdef IMPORT_QUICK3D_ASSETS
#include <QtQuick3DAssetImport/private/qssgassetimportmanager_p.h>
#endif

namespace
{
static Q_LOGGING_CATEGORY(importerLog, "qtc.itemlibrary.assetImporter", QtWarningMsg)
}

namespace QmlDesigner {

ItemLibraryAssetImporter::ItemLibraryAssetImporter(QObject *parent) :
    QObject (parent)
{
#ifdef IMPORT_QUICK3D_ASSETS
    m_quick3DAssetImporter.reset(new QSSGAssetImportManager);
#endif
}

ItemLibraryAssetImporter::~ItemLibraryAssetImporter() {
    cancelImport();
    delete m_tempDir;
};

void ItemLibraryAssetImporter::importQuick3D(const QStringList &inputFiles,
                                             const QString &importPath,
                                             const QVector<QJsonObject> &options,
                                             const QHash<QString, int> &extToImportOptionsMap)
{
    if (m_isImporting)
        cancelImport();
    reset();
    m_isImporting = true;

#ifdef IMPORT_QUICK3D_ASSETS
    if (!m_tempDir->isValid()) {
        addError(tr("Could not create a temporary directory for import."));
        notifyFinished();
        return;
    }

    m_importPath = importPath;

    parseFiles(inputFiles, options, extToImportOptionsMap);

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

            // Trigger underlying qmljs snapshot update by making a non-change to the doc
            model->rewriterView()->textModifier()->replace(0, 0, {});

            // There is a inbuilt delay before rewriter change actually updates the data model,
            // so we need to wait for a moment to allow the change to take effect.
            // Otherwise subsequent subcomponent manager update won't detect new imports properly.
            QTimer *timer = new QTimer(parent());
            static int counter;
            counter = 0;
            timer->callOnTimeout([this, timer, progressTitle, doc]() {
                if (!isCancelled()) {
                    notifyProgress(++counter * 10, progressTitle);
                    if (counter >= 10) {
                        doc->updateSubcomponentManager();
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
#else
    Q_UNUSED(inputFiles)
    Q_UNUSED(importPath)
    Q_UNUSED(options)
    Q_UNUSED(extToImportOptionsMap)
    addError(tr("Importing 3D assets requires building against Qt Quick 3D module."));
    notifyFinished();
#endif
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

bool ItemLibraryAssetImporter::isQuick3DAsset(const QString &fileName) const
{
#ifdef IMPORT_QUICK3D_ASSETS
    static QStringList quick3DExt;
    if (quick3DExt.isEmpty()) {
        const auto exts = m_quick3DAssetImporter->getSupportedExtensions();
        for (const auto &ext : exts)
            quick3DExt << ext;
    }
    return quick3DExt.contains(QFileInfo(fileName).suffix().toLower());
#else
    Q_UNUSED(fileName)
    return false;
#endif
}

QVariantMap ItemLibraryAssetImporter::supportedOptions(const QString &modelFile) const
{
#ifdef IMPORT_QUICK3D_ASSETS
    return m_quick3DAssetImporter->getOptionsForFile(modelFile);
#else
    Q_UNUSED(modelFile)
    return {};
#endif
}

QHash<QString, QVariantMap> ItemLibraryAssetImporter::allOptions() const
{
#ifdef IMPORT_QUICK3D_ASSETS
    return m_quick3DAssetImporter->getAllOptions();
#else
    return {};
#endif
}

QHash<QString, QStringList> ItemLibraryAssetImporter::supportedExtensions() const
{
#ifdef IMPORT_QUICK3D_ASSETS
    return m_quick3DAssetImporter->getSupportedExtensions();
#else
    return {};
#endif
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

#ifdef IMPORT_QUICK3D_ASSETS
    delete m_tempDir;
    m_tempDir = new QTemporaryDir;
    m_importFiles.clear();
    m_overwrittenImports.clear();
#endif
}

void ItemLibraryAssetImporter::parseFiles(const QStringList &filePaths,
                                          const QVector<QJsonObject> &options,
                                          const QHash<QString, int> &extToImportOptionsMap)
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
        if (isCancelled())
            return;
        if (isQuick3DAsset(file)) {
            QVariantMap varOpts;
            int index = extToImportOptionsMap.value(QFileInfo(file).suffix());
            parseQuick3DAsset(file, options[index].toVariantMap());
        }
        notifyProgress(qRound(++count * quota), progressTitle);
    }
    notifyProgress(100, progressTitle);
}

void ItemLibraryAssetImporter::parseQuick3DAsset(const QString &file, const QVariantMap &options)
{
#ifdef IMPORT_QUICK3D_ASSETS
    addInfo(tr("Parsing 3D Model"), file);

    QString errorString;

    QDir targetDir(m_importPath);
    QDir outDir(m_tempDir->path());
    QFileInfo sourceInfo(file);
    QString assetName = sourceInfo.completeBaseName();

    if (!assetName.isEmpty()) {
        // Fix name so it plays nice with imports
        for (QChar &currentChar : assetName) {
            if (!currentChar.isLetter() && !currentChar.isDigit())
                currentChar = QLatin1Char('_');
        }
        QCharRef firstChar = assetName[0];
        if (firstChar.isDigit())
            firstChar = QLatin1Char('_');
        if (firstChar.isLower())
            firstChar = firstChar.toUpper();
    }

    QString targetDirPath = targetDir.filePath(assetName);

    if (targetDir.exists(assetName)) {
        if (!confirmAssetOverwrite(assetName)) {
            addWarning(tr("Skipped import of existing asset: \"%1\"").arg(assetName));
            return;
        }
        m_overwrittenImports << targetDirPath;
    }

    outDir.mkpath(assetName);

    if (!outDir.cd(assetName)) {
        addError(tr("Could not access temporary asset directory: \"%1\"")
                 .arg(outDir.filePath(assetName)));
        return;
    }

    addInfo(tr("Generating 3D assets for: \"%1\"").arg(targetDir.absoluteFilePath(assetName)));

    if (m_quick3DAssetImporter->importFile(
                sourceInfo.absoluteFilePath(), outDir, options, &errorString)
            != QSSGAssetImportManager::ImportState::Success) {
        addError(tr("Failed to import 3D asset with error: %1").arg(errorString),
                 sourceInfo.absoluteFilePath());
        return;
    }

    // Generate qmldir file if importer doesn't already make one
    QString qmldirFileName = outDir.absoluteFilePath(QStringLiteral("qmldir"));
    if (!QFileInfo(qmldirFileName).exists()) {
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
                qmlInfo.append(assetName);
                qmlInfo.append('\n');
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
                                QFile::copy(":/ItemLibrary/images/item-3D_model-icon.png", iconFileName);
                                QFile::copy(":/ItemLibrary/images/item-3D_model-icon@2x.png", iconFileName2x);
                                // create hints file with proper hints
                                QFile file(outDir.path() + '/' + fi.baseName() + ".hints");
                                file.open(QIODevice::WriteOnly | QIODevice::Text);
                                QTextStream out(&file);
                                out << "canBeDroppedInFormEditor: false" << endl;
                                out << "canBeDroppedInView3D: true" << endl;
                                file.close();
                            } else {
                                QFile::copy(":/ItemLibrary/images/item-default-icon.png", iconFileName);
                                QFile::copy(":/ItemLibrary/images/item-default-icon@2x.png", iconFileName2x);
                            }
                        }
                    }
                }
                qmldirFile.write(qmlInfo.toUtf8());
                qmldirFile.commit();
            } else {
                addError(tr("Failed to create qmldir file for asset: \"%1\"").arg(assetName));
            }
        }
    }

    // Gather all generated files
    const int outDirPathSize = outDir.path().size();
    QDirIterator dirIt(outDir.path(), QDir::Files, QDirIterator::Subdirectories);
    QHash<QString, QString> assetFiles;
    while (dirIt.hasNext()) {
        dirIt.next();
        const QString filePath = dirIt.filePath();
        QString targetPath = filePath.mid(outDirPathSize);
        targetPath.prepend(targetDirPath);
        assetFiles.insert(filePath, targetPath);
    }

    // Copy the original asset into a subdirectory
    assetFiles.insert(sourceInfo.absoluteFilePath(),
                      targetDirPath + QStringLiteral("/source scene/") + sourceInfo.fileName());
    m_importFiles.insert(assetFiles);

#else
    Q_UNUSED(file)
    Q_UNUSED(options)
#endif
}

void ItemLibraryAssetImporter::copyImportedFiles()
{
#ifdef IMPORT_QUICK3D_ASSETS
    if (!m_overwrittenImports.isEmpty()) {
        const QString progressTitle = tr("Removing old overwritten assets.");
        addInfo(progressTitle);
        notifyProgress(0, progressTitle);

        int counter = 0;
        for (const QString &dirPath : m_overwrittenImports) {
            QDir dir(dirPath);
            if (dir.exists())
                dir.removeRecursively();
            notifyProgress((100 * ++counter) / m_overwrittenImports.size(), progressTitle);
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
                QDir targetDir = QFileInfo(it.value()).dir();
                if (!targetDir.exists())
                    targetDir.mkpath(QStringLiteral("."));
                QFile::copy(it.key(), it.value());
                ++it;
            }
            notifyProgress((100 * ++counter) / m_importFiles.size(), progressTitle);
        }
        notifyProgress(100, progressTitle);
    }
#endif
}

void ItemLibraryAssetImporter::notifyProgress(int value, const QString &text) const
{
    emit progressChanged(value, text);
    keepUiAlive();
}

void ItemLibraryAssetImporter::keepUiAlive() const
{
    QApplication::processEvents();
}

bool ItemLibraryAssetImporter::confirmAssetOverwrite(const QString &assetName)
{
    const QString title = tr("Overwrite Existing Asset?");
    const QString question = tr("Asset already exists. Overwrite?\n\"%1\"").arg(assetName);
    return QMessageBox::question(qobject_cast<QWidget *>(parent()),
                                 title, question,
                                 QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
}

bool ItemLibraryAssetImporter::isCancelled() const
{
    keepUiAlive();
    return m_cancelled;
}

} // QmlDesigner
