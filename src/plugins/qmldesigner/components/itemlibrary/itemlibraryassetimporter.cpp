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
#include <QtCore/qsavefile.h>
#include <QtCore/qloggingcategory.h>
#include <QtWidgets/qapplication.h>
#include <QtWidgets/qmessagebox.h>

#ifdef IMPORT_QUICK3D_ASSETS
#include <QtQuick3DAssetImport/private/qssgassetimportmanager_p.h>
#endif

namespace
{
Q_LOGGING_CATEGORY(importerLog, "qtc.itemlibrary.assetImporter", QtWarningMsg)
}

namespace QmlDesigner {

ItemLibraryAssetImporter::ItemLibraryAssetImporter(QObject *parent) :
    QObject (parent)
{
}

ItemLibraryAssetImporter::~ItemLibraryAssetImporter() {
    cancelImport();
};

void ItemLibraryAssetImporter::importQuick3D(const QStringList &inputFiles,
                                             const QString &importPath)
{
    if (m_isImporting)
        cancelImport();
    reset();
    m_isImporting = true;

#ifdef IMPORT_QUICK3D_ASSETS
    if (!QDir().mkpath(importPath)) {
        addError(tr("Cannot create import directory."), importPath);
        notifyFinished();
        return;
    }

    m_importPath = importPath;

    parseFiles(inputFiles);

    if (!isCancelled()) {
        auto doc = QmlDesignerPlugin::instance()->currentDesignDocument();
        Model *model = doc ? doc->currentModel() : nullptr;
        if (model && !m_quick3DAddedImports.isEmpty()) {
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
    static QStringList quick3DExt;
    if (quick3DExt.isEmpty()) {
        quick3DExt << QString(Constants::FbxExtension)
                   << QString(Constants::ColladaExtension)
                   << QString(Constants::ObjExtension)
                   << QString(Constants::BlenderExtension)
                   << QString(Constants::GltfExtension);
    }
    return quick3DExt.contains(QFileInfo(fileName).suffix());
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
    m_quick3DAddedImports.clear();
#endif
}

void ItemLibraryAssetImporter::parseFiles(const QStringList &filePaths)
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
        if (isQuick3DAsset(file))
            parseQuick3DAsset(file);
        notifyProgress(qRound(++count * quota), progressTitle);
    }
    notifyProgress(100, progressTitle);
}

void ItemLibraryAssetImporter::parseQuick3DAsset(const QString &file)
{
#ifdef IMPORT_QUICK3D_ASSETS
    addInfo(tr("Parsing 3D Model"), file);
    if (!m_quick3DAssetImporter)
        m_quick3DAssetImporter.reset(new QSSGAssetImportManager);

    QString errorString;

    QDir rootDir(m_importPath);
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

    QDir outDir = rootDir;
    if (outDir.exists(assetName)) {
        if (confirmAssetOverwrite(assetName)) {
            if (outDir.cd(assetName)) {
                outDir.removeRecursively();
                outDir.cdUp();
                outDir.mkpath(assetName);
            } // If cd fails here, it will fail below, too, so no error handling here
        } else {
            addWarning(tr("Skipped import of existing asset: \"%1\"").arg(assetName));
            return;
        }
    } else {
        outDir.mkpath(assetName);
    }
    if (!outDir.cd(assetName)) {
        addError(tr("Could not access asset directory: \"%1\"").arg(outDir.filePath(assetName)));
        return;
    }

    addInfo(tr("Generating 3D assets into: \"%1\"").arg(outDir.absolutePath()));

    if (m_quick3DAssetImporter->importFile(
                sourceInfo.absoluteFilePath(), outDir,
                &errorString) != QSSGAssetImportManager::ImportState::Success) {
        addError(tr("Failed to import 3D asset with error: %1").arg(errorString),
                 sourceInfo.absoluteFilePath());
        return;
    }

    // Generate qmldir file
    outDir.setNameFilters({QStringLiteral("*.qml")});
    const QFileInfoList qmlFiles = outDir.entryInfoList(QDir::Files);

    if (!qmlFiles.isEmpty()) {
        QString qmldirFileName = outDir.absoluteFilePath(QStringLiteral("qmldir"));
        QSaveFile qmldirFile(qmldirFileName);
        QString version = QStringLiteral("1.0");
        if (qmldirFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            for (const auto &fi : qmlFiles) {
                QString qmlInfo;
                qmlInfo.append(fi.baseName());
                qmlInfo.append(QLatin1Char(' '));
                qmlInfo.append(version);
                qmlInfo.append(QLatin1Char(' '));
                qmlInfo.append(fi.fileName());
                qmldirFile.write(qmlInfo.toUtf8());
            }
            QString assetFolder = QLatin1String(Constants::QUICK_3D_ASSETS_FOLDER);
            assetFolder = assetFolder.mid(assetFolder.lastIndexOf(QLatin1Char('/')) + 1);
            m_quick3DAddedImports.insert(
                        qmldirFileName,
                        Import::createLibraryImport(
                            QStringLiteral("%1.%2").arg(assetFolder).arg(assetName), version));
            qmldirFile.commit();
        } else {
            addError(tr("Failed to create qmldir file for asset: \"%1\"").arg(assetName));
        }
    }

    // Copy the original asset into a subdirectory
    QString origAssetDirName = QStringLiteral("source model");
    QDir origAssetDir = outDir;

    origAssetDir.mkpath(origAssetDirName);
    origAssetDir.cd(origAssetDirName);
    QFile::copy(sourceInfo.absoluteFilePath(), origAssetDir.filePath(sourceInfo.fileName()));
#else
    Q_UNUSED(file)
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
