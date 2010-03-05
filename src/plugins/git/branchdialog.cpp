/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "branchdialog.h"
#include "branchmodel.h"
#include "gitclient.h"
#include "gitplugin.h"
#include "ui_branchdialog.h"
#include "stashdialog.h" // Label helpers

#include <vcsbase/vcsbaseoutputwindow.h>

#include <QtGui/QItemSelectionModel>
#include <QtGui/QPushButton>
#include <QtGui/QMessageBox>

#include <QtCore/QDebug>

enum { debug = 0 };

// Single selection helper
static inline int selectedRow(const QAbstractItemView *listView)
{
    const QModelIndexList indexList = listView->selectionModel()->selectedIndexes();
    if (indexList.size() == 1)
        return indexList.front().row();
    return -1;
}

// Helper to select a row. No sooner said then done
static inline void selectListRow(QAbstractItemView *iv, int row)
{
    const QModelIndex index = iv->model()->index(row, 0);
    iv->selectionModel()->select(index, QItemSelectionModel::Select);
}

namespace Git {
    namespace Internal {

static inline GitClient *gitClient()
{
    return GitPlugin::instance()->gitClient();
}

BranchDialog::BranchDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::BranchDialog),
    m_checkoutButton(0),
    m_diffButton(0),
    m_refreshButton(0),
    m_deleteButton(0),
    m_localModel(new LocalBranchModel(gitClient(), this)),
    m_remoteModel(new RemoteBranchModel(gitClient(), this))
{
    setModal(false);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setAttribute(Qt::WA_DeleteOnClose, true); // Do not update unnecessarily

    m_ui->setupUi(this);

    m_checkoutButton = m_ui->buttonBox->addButton(tr("Checkout"), QDialogButtonBox::ActionRole);
    connect(m_checkoutButton, SIGNAL(clicked()), this, SLOT(slotCheckoutSelectedBranch()));

    m_diffButton = m_ui->buttonBox->addButton(tr("Diff"), QDialogButtonBox::ActionRole);
    connect(m_diffButton, SIGNAL(clicked()), this, SLOT(slotDiffSelected()));

    m_refreshButton = m_ui->buttonBox->addButton(tr("Refresh"), QDialogButtonBox::ActionRole);
    connect(m_refreshButton, SIGNAL(clicked()), this, SLOT(slotRefresh()));

    m_deleteButton = m_ui->buttonBox->addButton(tr("Delete..."), QDialogButtonBox::ActionRole);
    connect(m_deleteButton, SIGNAL(clicked()), this, SLOT(slotDeleteSelectedBranch()));

    connect(m_ui->localBranchListView, SIGNAL(doubleClicked(QModelIndex)), this,
            SLOT(slotLocalBranchActivated()));
    connect(m_ui->remoteBranchListView, SIGNAL(doubleClicked(QModelIndex)), this,
            SLOT(slotRemoteBranchActivated(QModelIndex)));

    connect(m_localModel, SIGNAL(newBranchEntered(QString)), this, SLOT(slotCreateLocalBranch(QString)));
    m_ui->localBranchListView->setModel(m_localModel);
    m_ui->remoteBranchListView->setModel(m_remoteModel);

    connect(m_ui->localBranchListView->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(slotEnableButtons()));
    connect(m_ui->remoteBranchListView->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(slotEnableButtons()));

    slotEnableButtons();
}

BranchDialog::~BranchDialog()
{
    delete m_ui;
}

void BranchDialog::refresh(const QString &repository, bool force)
{
    if (m_repository == repository && !force)
            return;
        // Refresh
    m_repository = repository;
    m_ui->repositoryLabel->setText(StashDialog::msgRepositoryLabel(m_repository));
    if (m_repository.isEmpty()) {
        m_localModel->clear();
        m_remoteModel->clear();
    } else {
        QString errorMessage;
        const bool success = m_localModel->refresh(m_repository, &errorMessage)
                             && m_remoteModel->refresh(m_repository, &errorMessage);
        if (!success)
            VCSBase::VCSBaseOutputWindow::instance()->appendError(errorMessage);
    }
    slotEnableButtons();
}

int BranchDialog::selectedLocalBranchIndex() const
{
    return selectedRow(m_ui->localBranchListView);
}

int BranchDialog::selectedRemoteBranchIndex() const
{
    return selectedRow(m_ui->remoteBranchListView);
}

void BranchDialog::slotEnableButtons()
{
    // We can switch to or delete branches that are not current.
    const bool hasRepository = !m_repository.isEmpty();
    const int selectedLocalRow = selectedLocalBranchIndex();
    const int currentLocalBranch = m_localModel->currentBranch();

    const bool hasSelection = selectedLocalRow != -1 && !m_localModel->isNewBranchRow(selectedLocalRow);
    const bool currentIsNotSelected = hasSelection && selectedLocalRow != currentLocalBranch;

    m_checkoutButton->setEnabled(currentIsNotSelected);
    m_diffButton->setEnabled(currentIsNotSelected);
    m_deleteButton->setEnabled(currentIsNotSelected);
    m_refreshButton->setEnabled(hasRepository);
    // Also disable <New Branch> entry of list view
    m_ui->localBranchListView->setEnabled(hasRepository);
    m_ui->remoteBranchListView->setEnabled(hasRepository);
}

void BranchDialog::slotRefresh()
{
    refresh(m_repository, true);
}

void BranchDialog::selectLocalBranch(const QString &b)
{
    // Select the newly created branch
    const int row = m_localModel->findBranchByName(b);
    if (row != -1)
        selectListRow(m_ui->localBranchListView, row);
}

bool BranchDialog::ask(const QString &title, const QString &what, bool defaultButton)
{
    return QMessageBox::question(this, title, what, QMessageBox::Yes|QMessageBox::No,
                                 defaultButton ? QMessageBox::Yes : QMessageBox::No) == QMessageBox::Yes;
}

/* Prompt to delete a local branch and do so. */
void BranchDialog::slotDeleteSelectedBranch()
{
    const int idx = selectedLocalBranchIndex();
    if (idx == -1)
        return;
    const QString name = m_localModel->branchName(idx);
    if (!ask(tr("Delete Branch"), tr("Would you like to delete the branch '%1'?").arg(name), true))
        return;
    QString errorMessage;
    bool ok = false;
    do {
        QString output;
        QStringList args(QLatin1String("-D"));
        args << name;
        if (!gitClient()->synchronousBranchCmd(m_repository, args, &output, &errorMessage))
            break;
        if (!m_localModel->refresh(m_repository, &errorMessage))
            break;
        ok = true;
    } while (false);
    slotEnableButtons();
    if (!ok)
        QMessageBox::warning(this, tr("Failed to delete branch"), errorMessage);
}

void BranchDialog::slotCreateLocalBranch(const QString &branchName)
{
    // Create
    QString output;
    QString errorMessage;
    bool ok = false;
    do {
        if (!gitClient()->synchronousBranchCmd(m_repository, QStringList(branchName), &output, &errorMessage))
            break;
        if (!m_localModel->refresh(m_repository, &errorMessage))
            break;
        ok = true;
    } while (false);
    if (!ok) {
        QMessageBox::warning(this, tr("Failed to create branch"), errorMessage);
        return;
    }
    selectLocalBranch(branchName);
}

void BranchDialog::slotLocalBranchActivated()
{
    if (m_checkoutButton->isEnabled())
        m_checkoutButton->animateClick();
}

void BranchDialog::slotDiffSelected()
{
    const int idx = selectedLocalBranchIndex();
    if (idx == -1)
        return;
    gitClient()->diffBranch(m_repository, QStringList(), m_localModel->branchName(idx));
}

/* Ask to stash away changes and then close dialog and do an asynchronous
 * checkout. */
void BranchDialog::slotCheckoutSelectedBranch()
{
    const int idx = selectedLocalBranchIndex();
    if (idx == -1)
        return;
    const QString name = m_localModel->branchName(idx);
    QString errorMessage;
    switch (gitClient()->ensureStash(m_repository, &errorMessage)) {
        case GitClient::StashUnchanged:
        case GitClient::Stashed:
        case GitClient::NotStashed:
        break;
        case GitClient::StashCanceled:
        return;
        case GitClient::StashFailed:
        QMessageBox::warning(this, tr("Failed to stash"), errorMessage);
        return;
    }
    if (gitClient()->synchronousCheckoutBranch(m_repository, name, &errorMessage)) {
        refresh(m_repository, true);
    } else {
        QMessageBox::warning(this, tr("Checkout failed"), errorMessage);
    }
}

void BranchDialog::slotRemoteBranchActivated(const QModelIndex &i)
{
    // Double click on a remote branch (origin/foo): Switch to matching
    // local (foo) one or offer to create a tracking branch.
    const QString remoteName = m_remoteModel->branchName(i.row());
    // build the name of the corresponding local branch
    // and look for it in the local model.
    const int slashPos = remoteName.indexOf(QLatin1Char('/'));
    if (slashPos == -1)
        return;
    const QString localBranch = remoteName.mid(slashPos + 1);
    if (localBranch == QLatin1String("HEAD") || localBranch == QLatin1String("master"))
        return;
   const int localIndex = m_localModel->findBranchByName(localBranch);
   if (debug)
        qDebug() << Q_FUNC_INFO << remoteName << localBranch << localIndex;
   // There is a matching a local one!
   if (localIndex != -1) {
       // Is it the current one? Just close.
       if (m_localModel->currentBranch() == localIndex) {
           accept();
           return;
       }
       // Nope, select and trigger checkout
       selectListRow(m_ui->localBranchListView, localIndex);
       slotLocalBranchActivated();
       return;
    }
    // Does not exist yet. Ask to create.
    const QString msg = tr("Would you like to create a local branch '%1' tracking the remote branch '%2'?").arg(localBranch, remoteName);
    if (!ask(tr("Create branch"), msg, true))
        return;
    QStringList args(QLatin1String("--track"));
    args << localBranch << remoteName;
    QString errorMessage;
    bool ok = false;
    do {
        QString output;
        if (!gitClient()->synchronousBranchCmd(m_repository, args, &output, &errorMessage))
            break;
        if (!m_localModel->refresh(m_repository, &errorMessage))
            break;
        ok = true;
    } while (false);
    if (!ok) {
        QMessageBox::warning(this, tr("Failed to create a tracking branch"), errorMessage);
        return;
    }
    // Select it
    selectLocalBranch(localBranch);
}

void BranchDialog::changeEvent(QEvent *e)
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
