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

#include "maemopackagecreationwidget.h"
#include "ui_maemopackagecreationwidget.h"

#include "maemopackagecontents.h"
#include "maemopackagecreationstep.h"
#include "maemotoolchain.h"

#include <utils/qtcassert.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4buildconfiguration.h>

#include <QtCore/QFileInfo>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

namespace Qt4ProjectManager {
namespace Internal {

MaemoPackageCreationWidget::MaemoPackageCreationWidget(MaemoPackageCreationStep *step)
    : ProjectExplorer::BuildStepConfigWidget(),
      m_step(step),
      m_ui(new Ui::MaemoPackageCreationWidget)
{
    m_ui->setupUi(this);
    m_ui->packageContentsView->setModel(step->packageContents());
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(step->packageContents(), SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            m_ui->packageContentsView, SLOT(resizeColumnsToContents()));
    connect(step->packageContents(), SIGNAL(rowsInserted(QModelIndex, int, int)),
            m_ui->packageContentsView, SLOT(resizeColumnsToContents()));
    connect(m_ui->packageContentsView->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this,
            SLOT(enableOrDisableRemoveButton()));
    m_ui->packageContentsView->resizeColumnsToContents();
    m_ui->packageContentsView->horizontalHeader()->setStretchLastSection(true);
    enableOrDisableRemoveButton();
}

void MaemoPackageCreationWidget::init()
{
}

QString MaemoPackageCreationWidget::summaryText() const
{
    return tr("<b>Create Package:</b> ") + m_step->packageFilePath();
}

QString MaemoPackageCreationWidget::displayName() const
{
    return m_step->displayName();
}

void MaemoPackageCreationWidget::addFile()
{
    const Qt4BuildConfiguration * const bc
        = static_cast<Qt4BuildConfiguration *>(m_step->buildConfiguration());
    QTC_ASSERT(bc, return);
    QString title = tr("Choose a local file");
    QString baseDir = bc->target()->project()->projectDirectory();
    const QString localFile = QFileDialog::getOpenFileName(this, title, baseDir);
    if (localFile.isEmpty())
        return;
    title = tr("Choose a remote file path");
    QTC_ASSERT(bc->toolChainType() == ProjectExplorer::ToolChain::GCC_MAEMO, return);
    baseDir = static_cast<MaemoToolChain *>(bc->toolChain())->sysrootRoot();
    QString remoteFile;
    const QString canonicalSysRoot = QFileInfo(baseDir).canonicalFilePath();
    do {
        QFileDialog d(this, title, baseDir);
        d.setFileMode(QFileDialog::AnyFile);
        d.selectFile(QFileInfo(localFile).fileName());
        if (!d.exec())
            return;
        remoteFile = d.selectedFiles().first();
        if (remoteFile.isEmpty())
            return;
        const QFileInfo remoteFileInfo(remoteFile);
        QString remoteDir = remoteFileInfo.dir().canonicalPath();
        if (!remoteDir.startsWith(canonicalSysRoot)) {
            QMessageBox::warning(this, tr("Invalid path"),
                tr("Please choose a location inside your sysroot directory."));
            remoteFile.clear();
        } else {
            remoteDir.remove(canonicalSysRoot);
            remoteFile = remoteDir + '/' + remoteFileInfo.fileName();
        }
    } while (remoteFile.isEmpty());

    const MaemoPackageContents::Deployable
        deployable(QFileInfo(localFile).absoluteFilePath(), remoteFile);
    if (!m_step->packageContents()->addDeployable(deployable)) {
        QMessageBox::information(this, tr("File already in package"),
                                 tr("You have already added this file."));
    }
}

void MaemoPackageCreationWidget::removeFile()
{
    const QModelIndexList selectedRows
        = m_ui->packageContentsView->selectionModel()->selectedRows();
    if (selectedRows.isEmpty())
        return;
    const int row = selectedRows.first().row();
    if (row != 0)
        m_step->packageContents()->removeDeployableAt(row);
}

void MaemoPackageCreationWidget::enableOrDisableRemoveButton()
{
    const QModelIndexList selectedRows
        = m_ui->packageContentsView->selectionModel()->selectedRows();
    m_ui->removeFileButton->setEnabled(!selectedRows.isEmpty()
                                       && selectedRows.first().row() != 0);
}

} // namespace Internal
} // namespace Qt4ProjectManager
