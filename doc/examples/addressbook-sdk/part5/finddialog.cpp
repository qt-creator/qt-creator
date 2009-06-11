#include "finddialog.h"
#include "ui_finddialog.h"

FindDialog::FindDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::FindDialog)
{
    m_ui->setupUi(this);
}

FindDialog::~FindDialog()
{
    delete m_ui;
}

void FindDialog::findClicked()
{
}

QString FindDialog::getFindText()
{
}
