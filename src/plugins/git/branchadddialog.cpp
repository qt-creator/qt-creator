#include "branchadddialog.h"
#include "ui_branchadddialog.h"

namespace Git {
namespace Internal {

BranchAddDialog::BranchAddDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::BranchAddDialog)
{
    m_ui->setupUi(this);
}

BranchAddDialog::~BranchAddDialog()
{
    delete m_ui;
}

void BranchAddDialog::setBranchName(const QString &n)
{
    m_ui->branchNameEdit->setText(n);
    m_ui->branchNameEdit->selectAll();
}

QString BranchAddDialog::branchName() const
{
    return m_ui->branchNameEdit->text();
}

void BranchAddDialog::setTrackedBranchName(const QString &name, bool remote)
{
    m_ui->trackingCheckBox->setVisible(true);
    if (!name.isEmpty())
        m_ui->trackingCheckBox->setText(remote ? tr("Track remote branch \'%1\'").arg(name) :
                                                 tr("Track local branch \'%1\'").arg(name));
    else
        m_ui->trackingCheckBox->setVisible(false);
    m_ui->trackingCheckBox->setChecked(remote);
}

bool BranchAddDialog::track()
{
    return m_ui->trackingCheckBox->isVisible() && m_ui->trackingCheckBox->isChecked();
}

} // namespace Internal
} // namespace Git
