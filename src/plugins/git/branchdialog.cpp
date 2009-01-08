#include "branchdialog.h"
#include "branchmodel.h"
#include "gitclient.h"
#include "ui_branchdialog.h"

#include <QtGui/QItemSelectionModel>
#include <QtGui/QPushButton>
#include <QtGui/QMessageBox>

// Single selection helper
static inline int selectedRow(const QAbstractItemView *listView)
{
    const QModelIndexList indexList = listView->selectionModel()->selectedIndexes();
    if (indexList.size() == 1)
        return indexList.front().row();
    return -1;
}

namespace Git {
    namespace Internal {

BranchDialog::BranchDialog(QWidget *parent) :
    QDialog(parent),
    m_client(0),
    m_ui(new Ui::BranchDialog),
    m_checkoutButton(0),
    m_deleteButton(0),
    m_localModel(0),
    m_remoteModel(0)
{
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    m_ui->setupUi(this);
    m_checkoutButton = m_ui->buttonBox->addButton(tr("Checkout"), QDialogButtonBox::AcceptRole);
    connect(m_checkoutButton, SIGNAL(clicked()), this, SLOT(slotCheckoutSelectedBranch()));

    m_deleteButton = m_ui->buttonBox->addButton(tr("Delete"), QDialogButtonBox::ActionRole);
    connect(m_deleteButton, SIGNAL(clicked()), this, SLOT(slotDeleteSelectedBranch()));

    connect(m_ui->localBranchListView, SIGNAL(doubleClicked(QModelIndex)), this,
            SLOT(slotLocalBranchActivated()));
}

BranchDialog::~BranchDialog()
{
    delete m_ui;
}

bool BranchDialog::init(GitClient *client, const QString &workingDirectory, QString *errorMessage)
{
    // Find repository and populate models.
    m_client = client;
    m_repoDirectory = GitClient::findRepositoryForDirectory(workingDirectory);
    if (m_repoDirectory.isEmpty()) {
        *errorMessage = tr("Unable to find the repository directory for '%1'.").arg(workingDirectory);
        return false;
    }
    m_ui->repositoryFieldLabel->setText(m_repoDirectory);

    m_localModel = new LocalBranchModel(client, this);
    connect(m_localModel, SIGNAL(newBranchCreated(QString)), this, SLOT(slotNewLocalBranchCreated(QString)));
    m_remoteModel = new RemoteBranchModel(client, this);
    if (!m_localModel->refresh(workingDirectory, errorMessage)
        || !m_remoteModel->refresh(workingDirectory, errorMessage))
        return false;

    m_ui->localBranchListView->setModel(m_localModel);
    m_ui->remoteBranchListView->setModel(m_remoteModel);
    // Selection model comes into existence only now
    connect(m_ui->localBranchListView->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(slotEnableButtons()));
    connect(m_ui->remoteBranchListView->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(slotEnableButtons()));
    slotEnableButtons();
    return true;
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
    const int selectedLocalRow = selectedLocalBranchIndex();
    const int currentLocalBranch = m_localModel->currentBranch();

    const bool hasSelection = selectedLocalRow != -1 && !m_localModel->isNewBranchRow(selectedLocalRow);
    const bool currentIsNotSelected = hasSelection && selectedLocalRow != currentLocalBranch;

    m_checkoutButton->setEnabled(currentIsNotSelected);
    m_deleteButton->setEnabled(currentIsNotSelected);
}

void BranchDialog::slotNewLocalBranchCreated(const QString &b)
{
    // Select the newly created branch
    const int row = m_localModel->findBranchByName(b);
    if (row != -1) {
        const QModelIndex index = m_localModel->index(row);
        m_ui->localBranchListView->selectionModel()->select(index, QItemSelectionModel::Select);
    }
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
        if (!m_client->synchronousBranchCmd(m_repoDirectory, args, &output, &errorMessage))
            break;
        if (!m_localModel->refresh(m_repoDirectory, &errorMessage))
            break;
        ok = true;
    } while (false);
    slotEnableButtons();
    if (!ok)
        QMessageBox::warning(this, tr("Failed to delete branch"), errorMessage);
}

void BranchDialog::slotLocalBranchActivated()
{
    if (m_checkoutButton->isEnabled())
        m_checkoutButton->animateClick();
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
    switch (m_client->ensureStash(m_repoDirectory, &errorMessage)) {
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
    accept();
    m_client->checkoutBranch(m_repoDirectory, name);
}

void BranchDialog::changeEvent(QEvent *e)
{
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
