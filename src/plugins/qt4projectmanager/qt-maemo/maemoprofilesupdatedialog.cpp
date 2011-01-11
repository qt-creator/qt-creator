/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#include "maemoprofilesupdatedialog.h"
#include "ui_maemoprofilesupdatedialog.h"

#include "maemodeployablelistmodel.h"

#include <qt4projectmanager/qt4nodes.h>

#include <QtGui/QTableWidgetItem>

namespace Qt4ProjectManager {
namespace Internal {

MaemoProFilesUpdateDialog::MaemoProFilesUpdateDialog(const QList<MaemoDeployableListModel *> &models,
    QWidget *parent)
    : QDialog(parent),
    m_models(models),
    ui(new Ui::MaemoProFilesUpdateDialog)
{
    ui->setupUi(this);
    ui->tableWidget->setRowCount(models.count());
    ui->tableWidget->setHorizontalHeaderItem(0,
        new QTableWidgetItem(tr("Updateable Project Files")));
    for (int row = 0; row < models.count(); ++row) {
        QTableWidgetItem *const item
            = new QTableWidgetItem(models.at(row)->proFilePath());
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setCheckState(Qt::Unchecked);
        ui->tableWidget->setItem(row, 0, item);
    }
    ui->tableWidget->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    ui->tableWidget->resizeRowsToContents();
    connect(ui->checkAllButton, SIGNAL(clicked()), this, SLOT(checkAll()));
    connect(ui->uncheckAllButton, SIGNAL(clicked()), this, SLOT(uncheckAll()));
}

MaemoProFilesUpdateDialog::~MaemoProFilesUpdateDialog()
{
    delete ui;
}

void MaemoProFilesUpdateDialog::checkAll()
{
    setCheckStateForAll(Qt::Checked);
}

void MaemoProFilesUpdateDialog::uncheckAll()
{
    setCheckStateForAll(Qt::Unchecked);
}

void MaemoProFilesUpdateDialog::setCheckStateForAll(Qt::CheckState checkState)
{
    for (int row = 0; row < ui->tableWidget->rowCount(); ++row)  {
        ui->tableWidget->item(row, 0)->setCheckState(checkState);
    }
}

QList<MaemoProFilesUpdateDialog::UpdateSetting>
MaemoProFilesUpdateDialog::getUpdateSettings() const
{
    QList<UpdateSetting> settings;
    for (int row = 0; row < m_models.count(); ++row) {
        const bool doUpdate = result() != Rejected
            && ui->tableWidget->item(row, 0)->checkState() == Qt::Checked;
        settings << UpdateSetting(m_models.at(row), doUpdate);
    }
    return settings;
}

} // namespace Qt4ProjectManager
} // namespace Internal
