#include "s60certificatedetailsdialog.h"
#include "ui_s60certificatedetailsdialog.h"

S60CertificateDetailsDialog::S60CertificateDetailsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::S60CertificateDetailsDialog)
{
    ui->setupUi(this);
    connect(ui->buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(close()));
}

S60CertificateDetailsDialog::~S60CertificateDetailsDialog()
{
    delete ui;
}

void S60CertificateDetailsDialog::setText(const QString &text)
{
    ui->textBrowser->setText(text);
}
