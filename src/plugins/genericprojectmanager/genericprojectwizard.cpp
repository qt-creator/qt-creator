// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "genericprojectwizard.h"

#include "filesselectionwizardpage.h"
#include "genericprojectconstants.h"
#include "genericprojectmanagertr.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/customwizard/customwizard.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/filewizardpage.h>
#include <utils/mimeutils.h>

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QPainter>
#include <QPixmap>
#include <QStyle>

using namespace Utils;

namespace GenericProjectManager {
namespace Internal {

const char ConfigFileTemplate[] =
        "// Add predefined macros for your project here. For example:\n"
        "// #define THE_ANSWER 42\n";

//////////////////////////////////////////////////////////////////////////////
//
// GenericProjectWizardDialog
//
//////////////////////////////////////////////////////////////////////////////

GenericProjectWizardDialog::GenericProjectWizardDialog(const Core::BaseFileWizardFactory *factory,
                                                       QWidget *parent) :
    Core::BaseFileWizard(factory, QVariantMap(), parent)
{
    setWindowTitle(Tr::tr("Import Existing Project"));

    // first page
    m_firstPage = new Utils::FileWizardPage;
    m_firstPage->setTitle(Tr::tr("Project Name and Location"));
    m_firstPage->setFileNameLabel(Tr::tr("Project name:"));
    m_firstPage->setPathLabel(Tr::tr("Location:"));
    addPage(m_firstPage);

    // second page
    m_secondPage = new FilesSelectionWizardPage(this);
    m_secondPage->setTitle(Tr::tr("File Selection"));
    addPage(m_secondPage);
}

FilePath GenericProjectWizardDialog::filePath() const
{
    return m_firstPage->filePath();
}

FilePaths GenericProjectWizardDialog::selectedPaths() const
{
    return m_secondPage->selectedPaths();
}

FilePaths GenericProjectWizardDialog::selectedFiles() const
{
    return m_secondPage->selectedFiles();
}

void GenericProjectWizardDialog::setFilePath(const FilePath &path)
{
    m_firstPage->setFilePath(path);
}

QString GenericProjectWizardDialog::projectName() const
{
    return m_firstPage->fileName();
}

//////////////////////////////////////////////////////////////////////////////
//
// GenericProjectWizard
//
//////////////////////////////////////////////////////////////////////////////

GenericProjectWizard::GenericProjectWizard()
{
    setSupportedProjectTypes({Constants::GENERICPROJECT_ID});
    setIcon(ProjectExplorer::Icons::WIZARD_IMPORT_AS_PROJECT.icon());
    setDisplayName(Tr::tr("Import Existing Project"));
    setId("Z.Makefile");
    setDescription(
        Tr::tr("Imports existing projects that do not use qmake, CMake, Qbs, Meson, or Autotools. "
               "This allows you to use %1 as a code editor.")
            .arg(QGuiApplication::applicationDisplayName()));
    setCategory(QLatin1String(ProjectExplorer::Constants::IMPORT_WIZARD_CATEGORY));
    setDisplayCategory(QLatin1String(ProjectExplorer::Constants::IMPORT_WIZARD_CATEGORY_DISPLAY));
    setFlags(Core::IWizardFactory::PlatformIndependent);
}

Core::BaseFileWizard *GenericProjectWizard::create(QWidget *parent,
                                                   const Core::WizardDialogParameters &parameters) const
{
    auto wizard = new GenericProjectWizardDialog(this, parent);
    wizard->setFilePath(parameters.defaultPath());
    const QList<QWizardPage *> pages = wizard->extensionPages();
    for (QWizardPage *p : pages)
        wizard->addPage(p);

    return wizard;
}

Core::GeneratedFiles GenericProjectWizard::generateFiles(const QWizard *w,
                                                         QString *errorMessage) const
{
    Q_UNUSED(errorMessage)

    auto wizard = qobject_cast<const GenericProjectWizardDialog *>(w);
    const FilePath projectPath = wizard->filePath();
    const QString projectName = wizard->projectName();
    const FilePath creatorFileName = projectPath.pathAppended(projectName + ".creator");
    const FilePath filesFileName = projectPath.pathAppended(projectName + ".files");
    const FilePath includesFileName = projectPath.pathAppended(projectName + ".includes");
    const FilePath configFileName = projectPath.pathAppended(projectName + ".config");
    const FilePath cxxflagsFileName = projectPath.pathAppended(projectName + ".cxxflags");
    const FilePath cflagsFileName = projectPath.pathAppended(projectName + ".cflags");
    const QStringList paths = Utils::transform(wizard->selectedPaths(), &Utils::FilePath::toString);

    Utils::MimeType headerTy = Utils::mimeTypeForName(QLatin1String("text/x-chdr"));

    QStringList nameFilters = headerTy.globPatterns();

    QStringList includePaths;
    const QDir dir(projectPath.toString());
    for (const QString &path : paths) {
        QFileInfo fileInfo(path);
        if (fileInfo.fileName() != "include")
            continue;
        QDir thisDir(fileInfo.absoluteFilePath());

        if (! thisDir.entryList(nameFilters, QDir::Files).isEmpty()) {
            QString relative = dir.relativeFilePath(path);
            if (relative.isEmpty())
                relative = QLatin1Char('.');
            includePaths.append(relative);
        }
    }
    includePaths.append(QString()); // ensure newline at EOF

    Core::GeneratedFile generatedCreatorFile(creatorFileName);
    generatedCreatorFile.setContents(QLatin1String("[General]\n"));
    generatedCreatorFile.setAttributes(Core::GeneratedFile::OpenProjectAttribute);

    QStringList sources = Utils::transform(wizard->selectedFiles(), &Utils::FilePath::toString);
    for (int i = 0; i < sources.length(); ++i)
        sources[i] = dir.relativeFilePath(sources[i]);
    Utils::sort(sources);
    sources.append(QString()); // ensure newline at EOF

    Core::GeneratedFile generatedFilesFile(filesFileName);
    generatedFilesFile.setContents(sources.join(QLatin1Char('\n')));

    Core::GeneratedFile generatedIncludesFile(includesFileName);
    generatedIncludesFile.setContents(includePaths.join(QLatin1Char('\n')));

    Core::GeneratedFile generatedConfigFile(configFileName);
    generatedConfigFile.setContents(QLatin1String(ConfigFileTemplate));

    Core::GeneratedFile generatedCxxFlagsFile(cxxflagsFileName);
    generatedCxxFlagsFile.setContents(
        QLatin1String(Constants::GENERICPROJECT_CXXFLAGS_FILE_TEMPLATE));

    Core::GeneratedFile generatedCFlagsFile(cflagsFileName);
    generatedCFlagsFile.setContents(QLatin1String(Constants::GENERICPROJECT_CFLAGS_FILE_TEMPLATE));

    Core::GeneratedFiles files;
    files.append(generatedFilesFile);
    files.append(generatedIncludesFile);
    files.append(generatedConfigFile);
    files.append(generatedCreatorFile);
    files.append(generatedCxxFlagsFile);
    files.append(generatedCFlagsFile);

    return files;
}

bool GenericProjectWizard::postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l,
                                             QString *errorMessage) const
{
    Q_UNUSED(w)
    return ProjectExplorer::CustomProjectWizard::postGenerateOpen(l, errorMessage);
}

} // namespace Internal
} // namespace GenericProjectManager
