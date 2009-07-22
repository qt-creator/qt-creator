#include "finddialog.h"
#include "ui_finddialog.h"
#include <QMessageBox>

//! [constructor]
FindDialog::FindDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::FindDialog)
{
    m_ui->setupUi(this);

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
        reject();
    } else {
        accept();
    }
}
//! [findClicked]

//! [findText]
QString FindDialog::findText()
{
    return m_ui->lineEdit->text();
}
//! [findText]
