/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
#include "assetexportdialog.h"
#include "ui_assetexportdialog.h"

#include "utils/fileutils.h"

#include <QPushButton>
#include <QDialogButtonBox>
#include <QMessageBox>

#include <algorithm>

namespace QmlDesigner {

AssetExportDialog::AssetExportDialog(const Utils::FilePath &exportPath,
                                     AssetExporter &assetExporter, QWidget *parent) :
    QDialog(parent),
    m_assetExporter(assetExporter),
    m_ui(new Ui::AssetExportDialog),
    m_model(this)
{
    m_ui->setupUi(this);
    m_ui->buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(false);

    m_ui->filesView->setModel(&m_model);

    m_exportBtn = m_ui->buttonBox->addButton(tr("Export"), QDialogButtonBox::AcceptRole);
    m_exportBtn->setEnabled(false);
    connect(m_exportBtn, &QPushButton::clicked, this, &AssetExportDialog::onExport);
    m_ui->exportPathEdit->setText(exportPath.toString());

    connect(&m_assetExporter, &AssetExporter::qmlFileResult, this, [this] (const Utils::FilePath &path) {
        m_qmlFiles.append(path);
        QStringList files = m_model.stringList();
        files.append(path.toString());
        m_model.setStringList(files);
    });

    connect(&m_assetExporter, &AssetExporter::stateChanged,
            this, &AssetExportDialog::onExportStateChanged);
}

AssetExportDialog::~AssetExportDialog()
{
    m_assetExporter.cancel();
}

void AssetExportDialog::onExport()
{
    m_assetExporter.exportQml(m_qmlFiles,
                              Utils::FilePath::fromString(m_ui->exportPathEdit->text()));
}

void AssetExportDialog::onExportStateChanged(AssetExporter::ParsingState newState)
{
    switch (newState) {
    case AssetExporter::ParsingState::PreProcessing:
        m_model.setStringList({});
        break;
    case AssetExporter::ParsingState::ExportingDone:
        QMessageBox::information(this, tr("QML Export"), tr("Done"));
        break;
    default:
        break;
    }

    m_exportBtn->setEnabled(newState == AssetExporter::ParsingState::PreProcessingFinished ||
                            newState == AssetExporter::ParsingState::ExportingDone);
    m_ui->buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(m_assetExporter.isBusy());
}
}
