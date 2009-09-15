#include "srcdestdialog.h"
#include "ui_srcdestdialog.h"


using namespace Mercurial::Internal;

SrcDestDialog::SrcDestDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::SrcDestDialog)
{
    m_ui->setupUi(this);
    m_ui->localPathChooser->setExpectedKind(Core::Utils::PathChooser::Directory);
}

SrcDestDialog::~SrcDestDialog()
{
    delete m_ui;
}

void SrcDestDialog::setPathChooserKind(Core::Utils::PathChooser::Kind kind)
{
    m_ui->localPathChooser->setExpectedKind(kind);
}

QString SrcDestDialog::getRepositoryString()
{
    if (m_ui->defaultButton->isChecked())
        return QString();
    else if (m_ui->localButton->isChecked())
        return m_ui->localPathChooser->path();
    else
        return m_ui->urlLineEdit->text();
}

void SrcDestDialog::changeEvent(QEvent *e)
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
