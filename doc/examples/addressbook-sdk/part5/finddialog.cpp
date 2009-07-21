#include "finddialog.h"
#include "ui_finddialog.h"
#include <QMessageBox>

//! [constructor]
FindDialog::FindDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::FindDialog)
{
    m_ui->setupUi(this);
    findText = "";

    connect(m_ui->findButton, SIGNAL(clicked()), this, SLOT(findClicked()));

    setWindowTitle(tr("Find a Contact"));
}
//! [constructor]

FindDialog::~FindDialog()
{
    delete m_ui;
}

//! [findClicked]
void FindDialog::findClicked()
{
    QString text = m_ui->lineEdit->text();

    if (text.isEmpty()) {
        QMessageBox::information(this, tr("Empty Field"),
            tr("Please enter a name."));
        return;
    } else {
        findText = text;
        m_ui->lineEdit->clear();
        hide();
    }
}
//! [findClicked]

//! [getFindText]
QString FindDialog::getFindText()
{
    return findText;
}
//! [getFindText]
