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
#include "itemlibraryassetimportdialog.h"
#include "ui_itemlibraryassetimportdialog.h"

#include "qmldesignerplugin.h"
#include "qmldesignerconstants.h"
#include "model.h"

#include "utils/outputformatter.h"

#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qtimer.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qformlayout.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qscrollbar.h>

namespace QmlDesigner {

namespace {

static void addFormattedMessage(Utils::OutputFormatter *formatter, const QString &str,
                                const QString &srcPath, Utils::OutputFormat format) {
    if (!formatter)
        return;
    QString msg = str;
    if (!srcPath.isEmpty())
        msg += QStringLiteral(": \"%1\"").arg(srcPath);
    msg += QLatin1Char('\n');
    formatter->appendMessage(msg, format);
    formatter->plainTextEdit()->verticalScrollBar()->setValue(
                formatter->plainTextEdit()->verticalScrollBar()->maximum());
}

}

ItemLibraryAssetImportDialog::ItemLibraryAssetImportDialog(const QStringList &importFiles,
                                     const QString &defaulTargetDirectory, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ItemLibraryAssetImportDialog),
    m_importer(this)
{
    setModal(true);
    ui->setupUi(this);

    m_outputFormatter = new Utils::OutputFormatter;
    m_outputFormatter->setPlainTextEdit(ui->plainTextEdit);

    // Skip unsupported assets
    bool skipSome = false;
    for (const auto &file : importFiles) {
        if (m_importer.isQuick3DAsset(file))
            m_quick3DFiles << file;
        else
            skipSome = true;
    }

    if (skipSome)
        addWarning("Cannot import 3D and other assets simultaneously. Skipping non-3D assets.");

    // Import button will be used in near future when we add import options. Hide for now.
    ui->buttonBox->button(QDialogButtonBox::Ok)->hide();
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Import"));
    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked,
            this, &ItemLibraryAssetImportDialog::onImport);

    ui->buttonBox->button(QDialogButtonBox::Close)->setDefault(true);

    // Import is always done under known folder. The order of preference for folder is:
    // 1) An existing QUICK_3D_ASSETS_FOLDER under DEFAULT_ASSET_IMPORT_FOLDER project import path
    // 2) An existing QUICK_3D_ASSETS_FOLDER under any project import path
    // 3) New QUICK_3D_ASSETS_FOLDER under DEFAULT_ASSET_IMPORT_FOLDER project import path
    // 4) New QUICK_3D_ASSETS_FOLDER under any project import path
    // 5) New QUICK_3D_ASSETS_FOLDER under new DEFAULT_ASSET_IMPORT_FOLDER under project
    const QString defaultAssetFolder = QLatin1String(Constants::DEFAULT_ASSET_IMPORT_FOLDER);
    const QString quick3DFolder = QLatin1String(Constants::QUICK_3D_ASSETS_FOLDER);
    QString candidatePath = defaulTargetDirectory + defaultAssetFolder + quick3DFolder;
    int candidatePriority = 5;
    QStringList importPaths;

    auto doc = QmlDesignerPlugin::instance()->currentDesignDocument();
    if (doc) {
        Model *model = doc->currentModel();
        if (model)
            importPaths = model->importPaths();
    }

    for (auto importPath : qAsConst(importPaths)) {
        if (importPath.startsWith(defaulTargetDirectory)) {
            const bool isDefaultFolder = importPath.endsWith(defaultAssetFolder);
            const QString assetFolder = importPath + quick3DFolder;
            const bool exists = QFileInfo(assetFolder).exists();
            if (exists) {
                if (isDefaultFolder) {
                    // Priority one location, stop looking
                    candidatePath = assetFolder;
                    break;
                } else if (candidatePriority > 2) {
                    candidatePriority = 2;
                    candidatePath = assetFolder;
                }
            } else {
                if (candidatePriority > 3 && isDefaultFolder) {
                    candidatePriority = 3;
                    candidatePath = assetFolder;
                } else if (candidatePriority > 4) {
                    candidatePriority = 4;
                    candidatePath = assetFolder;
                }
            }
        }
    }
    m_quick3DImportPath = candidatePath;

    // Queue import immediately until we have some options
    QTimer::singleShot(0, this, &ItemLibraryAssetImportDialog::onImport);

    connect(ui->buttonBox->button(QDialogButtonBox::Close), &QPushButton::clicked,
            this, &ItemLibraryAssetImportDialog::onClose);

    connect(&m_importer, &ItemLibraryAssetImporter::errorReported,
            this, &ItemLibraryAssetImportDialog::addError);
    connect(&m_importer, &ItemLibraryAssetImporter::warningReported,
            this, &ItemLibraryAssetImportDialog::addWarning);
    connect(&m_importer, &ItemLibraryAssetImporter::infoReported,
            this, &ItemLibraryAssetImportDialog::addInfo);
    connect(&m_importer, &ItemLibraryAssetImporter::importFinished,
            this, &ItemLibraryAssetImportDialog::onImportFinished);
    connect(&m_importer, &ItemLibraryAssetImporter::progressChanged,
            this, &ItemLibraryAssetImportDialog::setImportProgress);
}

ItemLibraryAssetImportDialog::~ItemLibraryAssetImportDialog()
{
    delete ui;
}

void ItemLibraryAssetImportDialog::setImportUiState(bool importing)
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!importing);
    ui->buttonBox->button(QDialogButtonBox::Close)->setText(importing ? tr("Cancel") : tr("Close"));
}

void ItemLibraryAssetImportDialog::addError(const QString &error, const QString &srcPath)
{
    addFormattedMessage(m_outputFormatter, error, srcPath, Utils::StdErrFormat);
}

void ItemLibraryAssetImportDialog::addWarning(const QString &warning, const QString &srcPath)
{
    addFormattedMessage(m_outputFormatter, warning, srcPath, Utils::StdOutFormat);
}

void ItemLibraryAssetImportDialog::addInfo(const QString &info, const QString &srcPath)
{
    addFormattedMessage(m_outputFormatter, info, srcPath, Utils::NormalMessageFormat);
}

void ItemLibraryAssetImportDialog::onImport()
{
    setImportUiState(true);
    ui->progressBar->setValue(0);
    ui->plainTextEdit->clear();

    if (!m_quick3DFiles.isEmpty())
        m_importer.importQuick3D(m_quick3DFiles, m_quick3DImportPath);
}

void ItemLibraryAssetImportDialog::setImportProgress(int value, const QString &text)
{
    ui->progressLabel->setText(text);
    if (value < 0)
        ui->progressBar->setRange(0, 0);
    else
        ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(value);
}

void ItemLibraryAssetImportDialog::onImportFinished()
{
    setImportUiState(false);
    if (m_importer.isCancelled()) {
        QString interruptStr = tr("Import interrupted.");
        addError(interruptStr);
        setImportProgress(0, interruptStr);
    } else {
        QString doneStr = tr("Import done.");
        addInfo(doneStr);
        setImportProgress(100, doneStr);
    }
}

void ItemLibraryAssetImportDialog::onClose()
{
    if (m_importer.isImporting()) {
        addInfo(tr("Canceling import."));
        m_importer.cancelImport();
    } else {
        reject();
    }
    close();
    deleteLater();
}
}
