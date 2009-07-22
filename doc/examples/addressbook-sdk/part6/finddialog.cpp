#include "finddialog.h"
#include "ui_finddialog.h"
#include <QMessageBox>

FindDialog::FindDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::FindDialog)
{
    m_ui->setupUi(this);

    connect(m_ui->findButton, SIGNAL(clicked()), this, SLOT(findClicked()));

    setWindowTitle(tr("Find a Contact"));
}

FindDialog::~FindDialog()
{
    delete m_ui;
}

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

QString FindDialog::findText()
{
    return m_ui->lineEdit->text();
}
