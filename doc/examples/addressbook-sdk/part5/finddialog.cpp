#include "finddialog.h"
#include "ui_finddialog.h"
#include <QMessageBox>

//! [constructor]
FindDialog::FindDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::FindDialog)
{
    m_ui->setupUi(this);
    lineEdit = new QLineEdit;
    lineEdit = m_ui->lineEdit;

    findButton = new QPushButton;
    findButton = m_ui->findButton;

    findText = "";

    connect(findButton, SIGNAL(clicked()), this, SLOT(findClicked()));

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
    QString text = lineEdit->text();

    if (text.isEmpty()) {
        QMessageBox::information(this, tr("Empty Field"),
            tr("Please enter a name."));
        return;
    } else {
        findText = text;
        lineEdit->clear();
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
