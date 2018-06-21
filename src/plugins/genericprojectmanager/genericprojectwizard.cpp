/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "genericprojectwizard.h"
#include "genericprojectconstants.h"
#include "filesselectionwizardpage.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/customwizard/customwizard.h>

#include <app/app_version.h>
#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/filewizardpage.h>
#include <utils/mimetypes/mimedatabase.h>

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QPainter>
#include <QPixmap>
#include <QStyle>

namespace GenericProjectManager {
namespace Internal {

static const char *const ConfigFileTemplate =
        "// Add predefined macros for your project here. For example:\n"
        "// #define THE_ANSWER 42\n"
        ;

//////////////////////////////////////////////////////////////////////////////
//
// GenericProjectWizardDialog
//
//////////////////////////////////////////////////////////////////////////////

GenericProjectWizardDialog::GenericProjectWizardDialog(const Core::BaseFileWizardFactory *factory,
                                                       QWidget *parent) :
    Core::BaseFileWizard(factory, QVariantMap(), parent)
{
    setWindowTitle(tr("Import Existing Project"));

    // first page
    m_firstPage = new Utils::FileWizardPage;
    m_firstPage->setTitle(tr("Project Name and Location"));
    m_firstPage->setFileNameLabel(tr("Project name:"));
    m_firstPage->setPathLabel(tr("Location:"));
    addPage(m_firstPage);

    // second page
    m_secondPage = new FilesSelectionWizardPage(this);
    m_secondPage->setTitle(tr("File Selection"));
    addPage(m_secondPage);
}

QString GenericProjectWizardDialog::path() const
{
    return m_firstPage->path();
}

Utils::FileNameList GenericProjectWizardDialog::selectedPaths() const
{
    return m_secondPage->selectedPaths();
}

Utils::FileNameList GenericProjectWizardDialog::selectedFiles() const
{
    return m_secondPage->selectedFiles();
}

void GenericProjectWizardDialog::setPath(const QString &path)
{
    m_firstPage->setPath(path);
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
    setIcon(QIcon(QLatin1String(":/genericprojectmanager/images/genericprojectmanager.png")));
    setDisplayName(tr("Import Existing Project"));
    setId("Z.Makefile");
    setDescription(tr("Imports existing projects that do not use qmake, CMake or Autotools. "
                      "This allows you to use %1 as a code editor.")
                   .arg(Core::Constants::IDE_DISPLAY_NAME));
    setCategory(QLatin1String(ProjectExplorer::Constants::IMPORT_WIZARD_CATEGORY));
    setDisplayCategory(QLatin1String(ProjectExplorer::Constants::IMPORT_WIZARD_CATEGORY_DISPLAY));
    setFlags(Core::IWizardFactory::PlatformIndependent);
}

Core::BaseFileWizard *GenericProjectWizard::create(QWidget *parent,
                                                   const Core::WizardDialogParameters &parameters) const
{
    GenericProjectWizardDialog *wizard = new GenericProjectWizardDialog(this, parent);

    wizard->setPath(parameters.defaultPath());

    foreach (QWizardPage *p, wizard->extensionPages())
        wizard->addPage(p);

    return wizard;
}

Core::GeneratedFiles GenericProjectWizard::generateFiles(const QWizard *w,
                                                         QString *errorMessage) const
{
    Q_UNUSED(errorMessage)

    const GenericProjectWizardDialog *wizard = qobject_cast<const GenericProjectWizardDialog *>(w);
    const QString projectPath = wizard->path();
    const QDir dir(projectPath);
    const QString projectName = wizard->projectName();
    const QString creatorFileName = QFileInfo(dir, projectName + QLatin1String(".creator")).absoluteFilePath();
    const QString filesFileName = QFileInfo(dir, projectName + QLatin1String(".files")).absoluteFilePath();
    const QString includesFileName = QFileInfo(dir, projectName + QLatin1String(".includes")).absoluteFilePath();
    const QString configFileName = QFileInfo(dir, projectName + QLatin1String(".config")).absoluteFilePath();
    const QStringList paths = Utils::transform(wizard->selectedPaths(), &Utils::FileName::toString);

    Utils::MimeType headerTy = Utils::mimeTypeForName(QLatin1String("text/x-chdr"));

    QStringList nameFilters = headerTy.globPatterns();

    QStringList includePaths;
    foreach (const QString &path, paths) {
        QFileInfo fileInfo(path);
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

    QStringList sources = Utils::transform(wizard->selectedFiles(), &Utils::FileName::toString);
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

    Core::GeneratedFiles files;
    files.append(generatedFilesFile);
    files.append(generatedIncludesFile);
    files.append(generatedConfigFile);
    files.append(generatedCreatorFile);

    return files;
}

bool GenericProjectWizard::postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l,
                                             QString *errorMessage) const
{
    Q_UNUSED(w);
    return ProjectExplorer::CustomProjectWizard::postGenerateOpen(l, errorMessage);
}

} // namespace Internal
} // namespace GenericProjectManager
