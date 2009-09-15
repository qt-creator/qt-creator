#include "revertdialog.h"

using namespace Mercurial::Internal;

RevertDialog::RevertDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::RevertDialog)
{
    m_ui->setupUi(this);
}

RevertDialog::~RevertDialog()
{
    delete m_ui;
}

void RevertDialog::changeEvent(QEvent *e)
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
