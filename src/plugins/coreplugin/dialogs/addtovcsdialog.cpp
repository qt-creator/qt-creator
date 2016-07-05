/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "addtovcsdialog.h"
#include "ui_addtovcsdialog.h"

#include <QDir>
#include <QListWidgetItem>

namespace Core {
namespace Internal {

AddToVcsDialog::AddToVcsDialog(QWidget *parent, const QString &title,
                               const QStringList &files, const QString &vcsDisplayName) :
    QDialog(parent),
    ui(new Ui::AddToVcsDialog)
{
    ui->setupUi(this);
    QString addTo = files.size() == 1
            ? tr("Add the file to version control (%1)").arg(vcsDisplayName)
            : tr("Add the files to version control (%1)").arg(vcsDisplayName);

    ui->addFilesLabel->setText(addTo);
    setWindowTitle(title);

    foreach (const QString &file, files) {
        QListWidgetItem *item = new QListWidgetItem(QDir::toNativeSeparators(file));
        ui->filesListWidget->addItem(item);
    }
}

AddToVcsDialog::~AddToVcsDialog()
{
    delete ui;
}

} // namespace Internal
} // namespace Core
