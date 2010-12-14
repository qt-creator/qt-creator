#include "s60certificatedetailsdialog.h"
#include "ui_s60certificatedetailsdialog.h"

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

struct S60CertificateDetailsDialogPrivate
{
    S60CertificateDetailsDialogPrivate(){};
    Ui::S60CertificateDetailsDialog m_ui;
};

S60CertificateDetailsDialog::S60CertificateDetailsDialog(QWidget *parent) :
    QDialog(parent),
    m_d(new S60CertificateDetailsDialogPrivate)
{
    m_d->m_ui.setupUi(this);
    connect(m_d->m_ui.buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(close()));
}

S60CertificateDetailsDialog::~S60CertificateDetailsDialog()
{
    delete m_d;
}

void S60CertificateDetailsDialog::setText(const QString &text)
{
    m_d->m_ui.textBrowser->setText(text);
}
