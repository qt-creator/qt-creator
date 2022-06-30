/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator Squish plugin.
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

#include "opensquishsuitesdialog.h"
#include "squishutils.h"
#include "ui_opensquishsuitesdialog.h"

#include <QDir>
#include <QListWidgetItem>
#include <QPushButton>

namespace Squish {
namespace Internal {

static QString previousPath;

OpenSquishSuitesDialog::OpenSquishSuitesDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::OpenSquishSuitesDialog)
{
    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Open)->setEnabled(false);

    connect(ui->directoryLineEdit,
            &Utils::PathChooser::pathChanged,
            this,
            &OpenSquishSuitesDialog::onDirectoryChanged);
    connect(ui->selectAllPushButton,
            &QPushButton::clicked,
            this,
            &OpenSquishSuitesDialog::selectAll);
    connect(ui->deselectAllPushButton,
            &QPushButton::clicked,
            this,
            &OpenSquishSuitesDialog::deselectAll);
    connect(this, &OpenSquishSuitesDialog::accepted, this, &OpenSquishSuitesDialog::setChosenSuites);

    ui->directoryLineEdit->setPath(previousPath);
}

OpenSquishSuitesDialog::~OpenSquishSuitesDialog()
{
    delete ui;
}

void OpenSquishSuitesDialog::onDirectoryChanged()
{
    ui->suitesListWidget->clear();
    ui->buttonBox->button(QDialogButtonBox::Open)->setEnabled(false);
    QDir baseDir(ui->directoryLineEdit->path());
    if (!baseDir.exists()) {
        return;
    }

    const QFileInfoList subDirs = baseDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &subDir : subDirs) {
        if (!subDir.baseName().startsWith("suite_"))
            continue;
        if (SquishUtils::validTestCases(subDir.absoluteFilePath()).size()) {
            QListWidgetItem *item = new QListWidgetItem(subDir.baseName(), ui->suitesListWidget);
            item->setCheckState(Qt::Checked);
            connect(ui->suitesListWidget,
                    &QListWidget::itemChanged,
                    this,
                    &OpenSquishSuitesDialog::onListItemChanged);
        }
    }
    ui->buttonBox->button(QDialogButtonBox::Open)->setEnabled(ui->suitesListWidget->count());
}

void OpenSquishSuitesDialog::onListItemChanged(QListWidgetItem *)
{
    const int count = ui->suitesListWidget->count();
    for (int row = 0; row < count; ++row) {
        if (ui->suitesListWidget->item(row)->checkState() == Qt::Checked) {
            ui->buttonBox->button(QDialogButtonBox::Open)->setEnabled(true);
            return;
        }
    }
    ui->buttonBox->button(QDialogButtonBox::Open)->setEnabled(false);
}

void OpenSquishSuitesDialog::selectAll()
{
    const int count = ui->suitesListWidget->count();
    for (int row = 0; row < count; ++row)
        ui->suitesListWidget->item(row)->setCheckState(Qt::Checked);
}

void OpenSquishSuitesDialog::deselectAll()
{
    const int count = ui->suitesListWidget->count();
    for (int row = 0; row < count; ++row)
        ui->suitesListWidget->item(row)->setCheckState(Qt::Unchecked);
}

void OpenSquishSuitesDialog::setChosenSuites()
{
    const int count = ui->suitesListWidget->count();
    previousPath = ui->directoryLineEdit->path();
    const QDir baseDir(previousPath);
    for (int row = 0; row < count; ++row) {
        QListWidgetItem *item = ui->suitesListWidget->item(row);
        if (item->checkState() == Qt::Checked)
            m_chosenSuites.append(QFileInfo(baseDir, item->text()).absoluteFilePath());
    }
}

} // namespace Internal
} // namespace Squish
