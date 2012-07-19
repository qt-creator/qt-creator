/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/
#include "profilesupdatedialog.h"
#include "ui_profilesupdatedialog.h"

#include "deployablefilesperprofile.h"

#include <qt4projectmanager/qt4nodes.h>

#include <QDir>
#include <QTableWidgetItem>

namespace RemoteLinux {
namespace Internal {

ProFilesUpdateDialog::ProFilesUpdateDialog(const QList<DeployableFilesPerProFile *> &models,
    QWidget *parent)
    : QDialog(parent),
    m_models(models),
    ui(new Ui::ProFilesUpdateDialog)
{
    ui->setupUi(this);
    ui->tableWidget->setRowCount(models.count());
    ui->tableWidget->setHorizontalHeaderItem(0,
        new QTableWidgetItem(tr("Updateable Project Files")));
    for (int row = 0; row < models.count(); ++row) {
        QTableWidgetItem *const item
            = new QTableWidgetItem(QDir::toNativeSeparators(models.at(row)->proFilePath()));
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setCheckState(Qt::Unchecked);
        ui->tableWidget->setItem(row, 0, item);
    }
    ui->tableWidget->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    ui->tableWidget->resizeRowsToContents();
    connect(ui->checkAllButton, SIGNAL(clicked()), this, SLOT(checkAll()));
    connect(ui->uncheckAllButton, SIGNAL(clicked()), this, SLOT(uncheckAll()));
}

ProFilesUpdateDialog::~ProFilesUpdateDialog()
{
    delete ui;
}

void ProFilesUpdateDialog::checkAll()
{
    setCheckStateForAll(Qt::Checked);
}

void ProFilesUpdateDialog::uncheckAll()
{
    setCheckStateForAll(Qt::Unchecked);
}

void ProFilesUpdateDialog::setCheckStateForAll(Qt::CheckState checkState)
{
    for (int row = 0; row < ui->tableWidget->rowCount(); ++row)  {
        ui->tableWidget->item(row, 0)->setCheckState(checkState);
    }
}

QList<ProFilesUpdateDialog::UpdateSetting> ProFilesUpdateDialog::getUpdateSettings() const
{
    QList<UpdateSetting> settings;
    for (int row = 0; row < m_models.count(); ++row) {
        const bool doUpdate = result() != Rejected
            && ui->tableWidget->item(row, 0)->checkState() == Qt::Checked;
        settings << UpdateSetting(m_models.at(row), doUpdate);
    }
    return settings;
}

} // namespace RemoteLinux
} // namespace Internal
