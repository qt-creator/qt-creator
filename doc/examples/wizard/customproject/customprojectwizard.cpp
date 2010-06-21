#include "customprojectwizard.h"

#include <QMessageBox>
#include <QApplication>
#include <QIcon>

CustomProjectWizard::CustomProjectWizard()
{
}

CustomProjectWizard::~CustomProjectWizard()
{
}

Core::IWizard::Kind CustomProjectWizard::kind() const
{
    return IWizard::ProjectWizard;
}

QIcon CustomProjectWizard::icon() const
{
    return qApp->windowIcon();
}

QString CustomProjectWizard::description() const
{
    return "A custom project";
}

QString CustomProjectWizard::name() const
{
    return "CustomProject";
}

QString CustomProjectWizard::category() const
{
    return "FooCompanyInc";
}

QString CustomProjectWizard::trCategory() const
{
    return tr("FooCompanyInc");
}

QStringList CustomProjectWizard::runWizard(const QString &path, QWidget *parent)
{
    Q_UNUSED(path);
    Q_UNUSED(parent);
    QMessageBox::information(parent, "Custom Wizard Dialog", "Hi there!");
    return QStringList();
}
