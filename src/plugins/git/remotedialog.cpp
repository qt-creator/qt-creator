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

#include "remotedialog.h"

#include "gitclient.h"
#include "gitplugin.h"
#include "remotemodel.h"
#include "ui_remotedialog.h"
#include "ui_remoteadditiondialog.h"

#include <utils/fancylineedit.h>
#include <utils/headerviewstretcher.h>
#include <vcsbase/vcsoutputwindow.h>

#include <QMessageBox>
#include <QRegularExpression>

namespace Git {
namespace Internal {

// --------------------------------------------------------------------------
// RemoteAdditionDialog:
// --------------------------------------------------------------------------

class RemoteAdditionDialog : public QDialog
{
public:
    RemoteAdditionDialog(const QStringList &remoteNames) :
        m_invalidRemoteNameChars(GitPlugin::invalidBranchAndRemoteNamePattern()),
        m_remoteNames(remoteNames)
    {
        m_ui.setupUi(this);
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
        m_ui.nameEdit->setValidationFunction([this](Utils::FancyLineEdit *edit, QString *errorMessage) {
            if (!edit)
                return false;

            QString input = edit->text();
            edit->setText(input.replace(m_invalidRemoteNameChars, "_"));

            // "Intermediate" patterns, may change to Acceptable when user edits further:

            if (input.endsWith(".lock")) //..may not end with ".lock"
                return false;

            if (input.endsWith('.')) // no dot at the end (but allowed in the middle)
                return false;

            if (input.endsWith('/')) // no slash at the end (but allowed in the middle)
                return false;

            if (m_remoteNames.contains(input)) {
                if (errorMessage)
                    *errorMessage = tr("A remote with the name \"%1\" already exists.").arg(input);
                return false;
            }

            // is a valid remote name
            return !input.isEmpty();
        });
        connect(m_ui.nameEdit, &QLineEdit::textChanged, [this]() {
            m_ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(m_ui.nameEdit->isValid());
        });

        m_ui.urlEdit->setValidationFunction([](Utils::FancyLineEdit *edit, QString *errorMessage) {
            if (!edit || edit->text().isEmpty())
                return false;

            const GitRemote r(edit->text());
            if (!r.isValid && errorMessage)
                *errorMessage = tr("The URL may not be valid.");

            return r.isValid;
        });

        m_ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }

    QString remoteName() const
    {
        return m_ui.nameEdit->text();
    }

    QString remoteUrl() const
    {
        return m_ui.urlEdit->text();
    }

private:
    Ui::RemoteAdditionDialog m_ui;
    const QRegularExpression m_invalidRemoteNameChars;
    QStringList m_remoteNames;
};


// --------------------------------------------------------------------------
// RemoteDialog:
// --------------------------------------------------------------------------


RemoteDialog::RemoteDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::RemoteDialog),
    m_remoteModel(new RemoteModel(this))
{
    setModal(false);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setAttribute(Qt::WA_DeleteOnClose, true); // Do not update unnecessarily

    m_ui->setupUi(this);

    m_ui->remoteView->setModel(m_remoteModel);
    new Utils::HeaderViewStretcher(m_ui->remoteView->header(), 1);

    connect(m_ui->addButton, &QPushButton::clicked, this, &RemoteDialog::addRemote);
    connect(m_ui->fetchButton, &QPushButton::clicked, this, &RemoteDialog::fetchFromRemote);
    connect(m_ui->pushButton, &QPushButton::clicked, this, &RemoteDialog::pushToRemote);
    connect(m_ui->removeButton, &QPushButton::clicked, this, &RemoteDialog::removeRemote);
    connect(m_ui->refreshButton, &QPushButton::clicked, this, &RemoteDialog::refreshRemotes);

    connect(m_ui->remoteView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &RemoteDialog::updateButtonState);

    updateButtonState();
}

RemoteDialog::~RemoteDialog()
{
    delete m_ui;
}

void RemoteDialog::refresh(const QString &repository, bool force)
{
    if (m_remoteModel->workingDirectory() == repository && !force)
        return;
    // Refresh
    m_ui->repositoryLabel->setText(GitPlugin::msgRepositoryLabel(repository));
    if (repository.isEmpty()) {
        m_remoteModel->clear();
    } else {
        QString errorMessage;
        if (!m_remoteModel->refresh(repository, &errorMessage))
            VcsBase::VcsOutputWindow::appendError(errorMessage);
    }
}

void RemoteDialog::refreshRemotes()
{
    refresh(m_remoteModel->workingDirectory(), true);
}

void RemoteDialog::addRemote()
{
    RemoteAdditionDialog addDialog(m_remoteModel->allRemoteNames());
    if (addDialog.exec() != QDialog::Accepted)
        return;

    m_remoteModel->addRemote(addDialog.remoteName(), addDialog.remoteUrl());
}

void RemoteDialog::removeRemote()
{
    const QModelIndexList indexList = m_ui->remoteView->selectionModel()->selectedIndexes();
    if (indexList.isEmpty())
        return;

    int row = indexList.at(0).row();
    const QString remoteName = m_remoteModel->remoteName(row);
    if (QMessageBox::question(this, tr("Delete Remote"),
                              tr("Would you like to delete the remote \"%1\"?").arg(remoteName),
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::Yes) == QMessageBox::Yes) {
        m_remoteModel->removeRemote(row);
    }
}

void RemoteDialog::pushToRemote()
{
    const QModelIndexList indexList = m_ui->remoteView->selectionModel()->selectedIndexes();
    if (indexList.isEmpty())
        return;

    const int row = indexList.at(0).row();
    const QString remoteName = m_remoteModel->remoteName(row);
    GitPlugin::client()->push(m_remoteModel->workingDirectory(), {remoteName});
}

void RemoteDialog::fetchFromRemote()
{
    const QModelIndexList indexList = m_ui->remoteView->selectionModel()->selectedIndexes();
    if (indexList.isEmpty())
        return;

    int row = indexList.at(0).row();
    const QString remoteName = m_remoteModel->remoteName(row);
    GitPlugin::client()->fetch(m_remoteModel->workingDirectory(), remoteName);
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

} // namespace Internal
} // namespace Git
