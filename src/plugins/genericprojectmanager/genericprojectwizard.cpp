#include "genericprojectwizard.h"
#include <projectexplorer/projectexplorer.h>

#include <utils/pathchooser.h>

#include <QtGui/QWizard>
#include <QtGui/QFormLayout>
#include <QtCore/QDir>
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
    parameters.setIcon(QIcon(":/wizards/images/console.png"));
    parameters.setName(tr("Existing Project"));
    parameters.setDescription(tr("Import Existing Project"));
    parameters.setCategory(QLatin1String("Projects"));
    parameters.setTrCategory(tr("Projects"));
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
    firstPage->setTitle(tr("Project"));

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
    const QString projectName = QFileInfo(pathChooser->path()).baseName() + QLatin1String(".creator");
    const QDir dir(pathChooser->path());

    // ### FIXME: use the mimetype database.
    // ### FIXME: import nested folders.
    const QStringList sources = dir.entryList(QStringList() << "Makefile" << "*.c" << "*.cpp" << "*.h", QDir::Files);

    QString projectContents;
    QTextStream stream(&projectContents);
    stream << "files=" << sources.join(",");
    stream << endl;

    Core::GeneratedFile file(QFileInfo(dir, projectName).absoluteFilePath()); // ### fixme
    file.setContents(projectContents);

    Core::GeneratedFiles files;
    files.append(file);

    return files;
}

bool GenericProjectWizard::postGenerateFiles(const Core::GeneratedFiles &l, QString *errorMessage)
{
    // Post-Generate: Open the project
    const QString proFileName = l.back().path();
    if (!ProjectExplorer::ProjectExplorerPlugin::instance()->openProject(proFileName)) {
        *errorMessage = tr("The project %1 could not be opened.").arg(proFileName);
        return false;
    }
    return true;
}

