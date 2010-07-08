/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
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
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "maemodeployablelistwidget.h"
#include "ui_maemodeployablelistwidget.h"

#include "maemodeployablelistmodel.h"

#include <utils/qtcassert.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

namespace Qt4ProjectManager {
namespace Internal {

MaemoDeployableListWidget::MaemoDeployableListWidget(QWidget *parent,
    MaemoDeployableListModel *model)
    : QWidget(parent), m_ui(new Ui::MaemoDeployableListWidget), m_model(model)
{
    m_ui->setupUi(this);
    m_ui->addFileButton->hide();
    m_ui->removeFileButton->hide();
    m_ui->deployablesView->setWordWrap(false);
    m_ui->deployablesView->setModel(m_model);
    connect(m_model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
        m_ui->deployablesView, SLOT(resizeColumnsToContents()));
    connect(m_model, SIGNAL(rowsInserted(QModelIndex, int, int)),
        m_ui->deployablesView, SLOT(resizeColumnsToContents()));
    connect(m_ui->deployablesView->selectionModel(),
        SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this,
        SLOT(enableOrDisableRemoveButton()));
    m_ui->deployablesView->resizeColumnsToContents();
    m_ui->deployablesView->horizontalHeader()->setStretchLastSection(true);
    enableOrDisableRemoveButton();
}

MaemoDeployableListWidget::~MaemoDeployableListWidget()
{
    delete m_ui;
}

void MaemoDeployableListWidget::addFile()
{
    const QString title = tr("Choose a local file");
    const QString localFile = QFileDialog::getOpenFileName(this, title, m_model->projectDir()); // TODO: Support directories.
    if (localFile.isEmpty())
        return;
    const MaemoDeployable
        deployable(QDir::toNativeSeparators(QFileInfo(localFile).absoluteFilePath()),
        "/");
    QString errorString;
    if (!m_model->addDeployable(deployable, &errorString)) {
        QMessageBox::information(this, tr("Error adding file"), errorString);
    } else {
        const QModelIndex newIndex
            = m_model->index(m_model->rowCount() - 1, 1);
        m_ui->deployablesView->selectionModel()->clear();
        m_ui->deployablesView->scrollTo(newIndex);
        m_ui->deployablesView->edit(newIndex);
    }
}

void MaemoDeployableListWidget::removeFile()
{
    const QModelIndexList selectedRows
        = m_ui->deployablesView->selectionModel()->selectedRows();
    if (selectedRows.isEmpty())
        return;
    const int row = selectedRows.first().row();
    if (row != 0) {
        QString errorString;
        if (!m_model->removeDeployableAt(row, &errorString)) {
            QMessageBox::information(this, tr("Error removing file"),
                errorString);
        }
    }
}

void MaemoDeployableListWidget::enableOrDisableRemoveButton()
{
    const QModelIndexList selectedRows
        = m_ui->deployablesView->selectionModel()->selectedRows();
    m_ui->removeFileButton->setEnabled(!selectedRows.isEmpty()
                                       && selectedRows.first().row() != 0);
}

} // namespace Internal
} // namespace Qt4ProjectManager
