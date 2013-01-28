/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "remotedialog.h"

#include "gitclient.h"
#include "gitplugin.h"
#include "remotemodel.h"
#include "stashdialog.h" // for messages
#include "ui_remotedialog.h"
#include "ui_remoteadditiondialog.h"

#include <vcsbase/vcsbaseoutputwindow.h>

#include <QItemSelectionModel>
#include <QPushButton>
#include <QMessageBox>

namespace Git {
namespace Internal {

// --------------------------------------------------------------------------
// RemoteAdditionDialog:
// --------------------------------------------------------------------------

RemoteAdditionDialog::RemoteAdditionDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::RemoteAdditionDialog)
{
    m_ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

RemoteAdditionDialog::~RemoteAdditionDialog()
{
    delete m_ui;
}

QString RemoteAdditionDialog::remoteName() const
{
    return m_ui->nameEdit->text();
}

QString RemoteAdditionDialog::remoteUrl() const
{
    return m_ui->urlEdit->text();
}

void RemoteAdditionDialog::clear()
{
    m_ui->nameEdit->setText(QString());
    m_ui->urlEdit->setText(QString());
}

// --------------------------------------------------------------------------
// RemoteDialog:
// --------------------------------------------------------------------------


RemoteDialog::RemoteDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::RemoteDialog),
    m_remoteModel(new RemoteModel(GitPlugin::instance()->gitClient(), this)),
    m_addDialog(0)
{
    setModal(false);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setAttribute(Qt::WA_DeleteOnClose, true); // Do not update unnecessarily

    m_ui->setupUi(this);

    m_ui->remoteView->setModel(m_remoteModel);
    m_ui->remoteView->horizontalHeader()->setStretchLastSection(true);
    m_ui->remoteView->horizontalHeader()->setResizeMode(0, QHeaderView::ResizeToContents);
    QFontMetrics fm(font());
    m_ui->remoteView->verticalHeader()->setDefaultSectionSize(qMax(static_cast<int>(fm.height() * 1.2), fm.height() + 4));

    connect(m_ui->addButton, SIGNAL(clicked()), this, SLOT(addRemote()));
    connect(m_ui->fetchButton, SIGNAL(clicked()), this, SLOT(fetchFromRemote()));
    connect(m_ui->pushButton, SIGNAL(clicked()), this, SLOT(pushToRemote()));
    connect(m_ui->removeButton, SIGNAL(clicked()), this, SLOT(removeRemote()));
    connect(m_ui->refreshButton, SIGNAL(clicked()), this, SLOT(refreshRemotes()));

    connect(m_ui->remoteView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(updateButtonState()));

    updateButtonState();
}

RemoteDialog::~RemoteDialog()
{
    delete m_ui;
}

void RemoteDialog::refresh(const QString &repository, bool force)
{
    if (m_repository == repository && !force)
        return;
    // Refresh
    m_repository = repository;
    m_ui->repositoryLabel->setText(StashDialog::msgRepositoryLabel(m_repository));
    if (m_repository.isEmpty()) {
        m_remoteModel->clear();
    } else {
        QString errorMessage;
        if (!m_remoteModel->refresh(m_repository, &errorMessage))
            VcsBase::VcsBaseOutputWindow::instance()->appendError(errorMessage);
    }
}

void RemoteDialog::refreshRemotes()
{
    refresh(m_remoteModel->workingDirectory(), true);
}

void RemoteDialog::addRemote()
{
    if (!m_addDialog)
        m_addDialog = new RemoteAdditionDialog;
    m_addDialog->clear();

    if (m_addDialog->exec() != QDialog::Accepted)
        return;

    m_remoteModel->addRemote(m_addDialog->remoteName(), m_addDialog->remoteUrl());
}

void RemoteDialog::removeRemote()
{
    const QModelIndexList indexList = m_ui->remoteView->selectionModel()->selectedIndexes();
    if (indexList.count() == 0)
        return;

    int row = indexList.at(0).row();
    const QString remoteName = m_remoteModel->remoteName(row);
    if (QMessageBox::question(this, tr("Delete Remote"),
                              tr("Would you like to delete the remote \"%1\"?").arg(remoteName),
                              QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes) {
        m_remoteModel->removeRemote(row);
    }
}

void RemoteDialog::pushToRemote()
{
    const QModelIndexList indexList = m_ui->remoteView->selectionModel()->selectedIndexes();
    if (indexList.count() == 0)
        return;

    const int row = indexList.at(0).row();
    const QString remoteName = m_remoteModel->remoteName(row);
    m_remoteModel->client()->synchronousPush(m_remoteModel->workingDirectory(), remoteName);
}

void RemoteDialog::fetchFromRemote()
{
    const QModelIndexList indexList = m_ui->remoteView->selectionModel()->selectedIndexes();
    if (indexList.count() == 0)
        return;

    int row = indexList.at(0).row();
    const QString remoteName = m_remoteModel->remoteName(row);
    m_remoteModel->client()->synchronousFetch(m_remoteModel->workingDirectory(), remoteName);
}

void RemoteDialog::updateButtonState()
{
    const QModelIndexList indexList = m_ui->remoteView->selectionModel()->selectedIndexes();

    const bool haveSelection = (indexList.count() > 0);
    m_ui->addButton->setEnabled(true);
    m_ui->fetchButton->setEnabled(haveSelection);
    m_ui->pushButton->setEnabled(haveSelection);
    m_ui->removeButton->setEnabled(haveSelection);
}

void RemoteDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

} // namespace Internal
} // namespace Git
