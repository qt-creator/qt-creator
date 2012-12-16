#include "branchcheckoutdialog.h"
#include "ui_branchcheckoutdialog.h"

namespace Git {
namespace Internal {

BranchCheckoutDialog::BranchCheckoutDialog(QWidget *parent,
                                           const QString& currentBranch,
                                           const QString& nextBranch) :
    QDialog(parent),
    m_ui(new Ui::BranchCheckoutDialog),
    m_foundStashForNextBranch(false),
    m_hasLocalChanges(true)
{
    m_ui->setupUi(this);

    setWindowTitle(tr("Checkout branch \"%1\"").arg(nextBranch));
    m_ui->moveChangesRadioButton->setText(tr("Move Local Changes to \"%1\"").arg(nextBranch));
    m_ui->popStashCheckBox->setText(tr("Pop Stash of \"%1\"").arg(nextBranch));

    if (!currentBranch.isEmpty()) {
        m_ui->makeStashRadioButton->setText(tr("Create Branch Stash for \"%1\"").arg(currentBranch));
    } else {
        m_ui->makeStashRadioButton->setText(tr("Create Branch Stash for Current Branch"));
        foundNoLocalChanges();
    }

    connect(m_ui->moveChangesRadioButton, SIGNAL(toggled(bool)), this, SLOT(updatePopStashCheckBox(bool)));
}

BranchCheckoutDialog::~BranchCheckoutDialog()
{
    delete m_ui;
}

void BranchCheckoutDialog::foundNoLocalChanges()
{
    m_ui->discardChangesRadioButton->setChecked(true);
    m_ui->localChangesGroupBox->setEnabled(false);
    m_hasLocalChanges = false;
}

void BranchCheckoutDialog::foundStashForNextBranch()
{
    m_ui->popStashCheckBox->setChecked(true);
    m_ui->popStashCheckBox->setEnabled(true);
    m_foundStashForNextBranch = true;
}

bool BranchCheckoutDialog::makeStashOfCurrentBranch()
{
    return m_ui->makeStashRadioButton->isChecked();
}

bool BranchCheckoutDialog::moveLocalChangesToNextBranch()
{
    return m_ui->moveChangesRadioButton->isChecked();
}

bool BranchCheckoutDialog::discardLocalChanges()
{
    return m_ui->discardChangesRadioButton->isChecked() && m_ui->localChangesGroupBox->isEnabled();
}

bool BranchCheckoutDialog::popStashOfNextBranch()
{
    return m_ui->popStashCheckBox->isChecked();
}

bool BranchCheckoutDialog::hasStashForNextBranch()
{
    return m_foundStashForNextBranch;
}

bool BranchCheckoutDialog::hasLocalChanges()
{
    return m_hasLocalChanges;
}

void BranchCheckoutDialog::updatePopStashCheckBox(bool moveChangesChecked)
{
    m_ui->popStashCheckBox->setChecked(!moveChangesChecked && m_foundStashForNextBranch);
    m_ui->popStashCheckBox->setEnabled(!moveChangesChecked && m_foundStashForNextBranch);
}

} // namespace Internal
} // namespace Git

