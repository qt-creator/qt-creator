#include "genericprojectwizard.h"
#include <utils/pathchooser.h>

#include <QtGui/QWizard>
#include <QtGui/QFormLayout>
#include <QtCore/QtDebug>

using namespace GenericProjectManager::Internal;
using namespace Core::Utils;

GenericProjectWizard::GenericProjectWizard()
    : Core::BaseFileWizard(parameters())
{
}

GenericProjectWizard::~GenericProjectWizard()
{
}

Core::BaseFileWizardParameters GenericProjectWizard::parameters()
{
    static Core::BaseFileWizardParameters parameters(ProjectWizard);
    parameters.setName(tr("Existing Project"));
    parameters.setDescription(tr("Import Existing Project"));
    parameters.setCategory(QLatin1String("Import"));
    parameters.setTrCategory(tr("Import"));
    return parameters;
}

QWizard *GenericProjectWizard::createWizardDialog(QWidget *parent,
                                                  const QString &defaultPath,
                                                  const WizardPageList &extensionPages) const
{
    QWizard *wizard = new QWizard(parent);
    wizard->setWindowTitle(tr("Import Existing Project"));
    setupWizard(wizard);

    QWizardPage *firstPage = new QWizardPage;
    firstPage->setTitle(tr(""));

    QFormLayout *layout = new QFormLayout(firstPage);

    PathChooser *pathChooser = new PathChooser;
    pathChooser->setObjectName("pathChooser");
    layout->addRow(tr("Source Directory:"), pathChooser);    

    wizard->addPage(firstPage);

    foreach (QWizardPage *p, extensionPages)
        wizard->addPage(p);

    return wizard;
}

Core::GeneratedFiles GenericProjectWizard::generateFiles(const QWizard *w,
                                                         QString *errorMessage) const
{
    PathChooser *pathChooser = w->findChild<PathChooser *>("pathChooser");

    return Core::GeneratedFiles();
}

