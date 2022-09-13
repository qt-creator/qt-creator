/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "bundleimporter.h"

#include "import.h"
#include "model.h"
#include "qmldesignerconstants.h"
#include "qmldesignerplugin.h"
#include "rewritingexception.h"

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStringList>

using namespace Utils;

namespace QmlDesigner::Internal {

BundleImporter::BundleImporter(const QString &bundleDir,
                               const QString &bundleId,
                               const QStringList &sharedFiles,
                               QObject *parent)
    : QObject(parent)
    , m_bundleDir(FilePath::fromString(bundleDir))
    , m_bundleId(bundleId)
    , m_sharedFiles(sharedFiles)
{
    m_importTimer.setInterval(200);
    connect(&m_importTimer, &QTimer::timeout, this, &BundleImporter::handleImportTimer);
    m_moduleName = QStringLiteral("%1.%2").arg(
                QLatin1String(Constants::COMPONENT_BUNDLES_FOLDER),
                m_bundleId).mid(1); // Chop leading slash
}

// Returns empty string on success or an error message on failure.
// Note that there is also an asynchronous portion to the import, which will only
// be done if this method returns success. Once the asynchronous portion of the
// import is completed, importFinished signal will be emitted.
QString BundleImporter::importComponent(const QString &qmlFile,
                                        const QStringList &files)
{
    FilePath bundleImportPath = QmlDesignerPlugin::instance()->documentManager().currentProjectDirPath();
    if (bundleImportPath.isEmpty())
        return "Failed to resolve current project path";

    const QString projectBundlePath = QStringLiteral("%1%2/%3").arg(
                QLatin1String(Constants::DEFAULT_ASSET_IMPORT_FOLDER),
                QLatin1String(Constants::COMPONENT_BUNDLES_FOLDER),
                m_bundleId).mid(1); // Chop leading slash
    bundleImportPath = bundleImportPath.resolvePath(projectBundlePath);

    if (!bundleImportPath.exists()) {
        if (!bundleImportPath.createDir())
            return QStringLiteral("Failed to create bundle import folder: '%1'").arg(bundleImportPath.toString());
    }

    for (const QString &file : qAsConst(m_sharedFiles)) {
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
    QFile qmldirFile(qmldirPath.toString());

    QString qmldirContent;
    if (qmldirPath.exists()) {
        if (!qmldirFile.open(QIODeviceBase::ReadOnly))
            return QStringLiteral("Failed to open qmldir file for reading: '%1'").arg(qmldirPath.toString());
        qmldirContent = QString::fromUtf8(qmldirFile.readAll());
        qmldirFile.close();
    } else {
        qmldirContent.append("module ");
        qmldirContent.append(m_moduleName);
        qmldirContent.append('\n');
    }

    FilePath qmlSourceFile = FilePath::fromString(qmlFile);
    const bool qmlFileExists = qmlSourceFile.exists();
    const QString qmlType = qmlSourceFile.baseName();
    m_pendingTypes.append(QStringLiteral("%1.%2")
                          .arg(QLatin1String(Constants::COMPONENT_BUNDLES_FOLDER).mid(1), qmlType));
    if (!qmldirContent.contains(qmlFile)) {
        QSaveFile qmldirSaveFile(qmldirPath.toString());
        if (!qmldirSaveFile.open(QIODeviceBase::WriteOnly | QIODeviceBase::Truncate))
            return QStringLiteral("Failed to open qmldir file for writing: '%1'").arg(qmldirPath.toString());

        qmldirContent.append(qmlType);
        qmldirContent.append(" 1.0 ");
        qmldirContent.append(qmlFile);
        qmldirContent.append('\n');

        qmldirSaveFile.write(qmldirContent.toUtf8());
        qmldirSaveFile.commit();
    }

    QStringList allFiles;
    allFiles.append(files);
    allFiles.append(qmlFile);
    for (const QString &file : qAsConst(allFiles)) {
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
            // avoid errors, as code model update takes a while. Full reset is not necessary
            // in this case, as new import directory appearing will trigger scanning of it.
            m_importAddPending = true;
            m_fullReset = false;
        }
    }
    m_importTimerCount = 0;
    m_importTimer.start();

    return {};
}

void BundleImporter::handleImportTimer()
{
    auto handleFailure = [this]() {
        m_importTimer.stop();
        m_fullReset = false;
        m_importAddPending = false;
        m_importTimerCount = 0;
        m_pendingTypes.clear();
        emit importFinished({});
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
    const QStringList pendingTypes = m_pendingTypes;
    for (const QString &pendingType : pendingTypes) {
        NodeMetaInfo metaInfo = model->metaInfo(pendingType.toUtf8());
        if (metaInfo.isValid() && !metaInfo.superClasses().empty()) {
            m_pendingTypes.removeAll(pendingType);
            emit importFinished(metaInfo);
        }
    }

    if (m_pendingTypes.isEmpty()) {
        m_importTimer.stop();
        m_importTimerCount = 0;
    }
}

} // namespace QmlDesigner::Internal
