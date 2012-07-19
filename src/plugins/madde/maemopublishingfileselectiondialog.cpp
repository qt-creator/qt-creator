/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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

#include "maemopublishingfileselectiondialog.h"
#include "ui_maemopublishingfileselectiondialog.h"

#include "maemopublishedprojectmodel.h"

namespace Madde {
namespace Internal {

MaemoPublishingFileSelectionDialog::MaemoPublishingFileSelectionDialog(const QString &projectPath,
    QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MaemoPublishingFileSelectionDialog)
{
    ui->setupUi(this);
    m_projectModel = new MaemoPublishedProjectModel(this);
    const QModelIndex rootIndex = m_projectModel->setRootPath(projectPath);
    m_projectModel->initFilesToExclude();
    ui->projectView->setModel(m_projectModel);
    ui->projectView->setRootIndex(rootIndex);
    ui->projectView->header()->setResizeMode(0, QHeaderView::ResizeToContents);
}

MaemoPublishingFileSelectionDialog::~MaemoPublishingFileSelectionDialog()
{
    delete ui;
}

QStringList MaemoPublishingFileSelectionDialog::filesToExclude() const
{
    return m_projectModel->filesToExclude();
}

} // namespace Internal
} // namespace Madde
