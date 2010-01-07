/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "genericprojectwizard.h"

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/filewizardpage.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QtDebug>
#include <QtCore/QCoreApplication>

using namespace GenericProjectManager::Internal;
using namespace Utils;

//////////////////////////////////////////////////////////////////////////////
// GenericProjectWizardDialog
//////////////////////////////////////////////////////////////////////////////

GenericProjectWizardDialog::GenericProjectWizardDialog(QWidget *parent)
    : QWizard(parent)
{
    setWindowTitle(tr("Import of Makefile-based Project"));

    // first page
    m_firstPage = new FileWizardPage;
    m_firstPage->setTitle(tr("Generic Project"));
    m_firstPage->setFileNameLabel(tr("Project name:"));
    m_firstPage->setPathLabel(tr("Location:"));

    addPage(m_firstPage);
}

GenericProjectWizardDialog::~GenericProjectWizardDialog()
{ }

QString GenericProjectWizardDialog::path() const
{
    return m_firstPage->path();
}

void GenericProjectWizardDialog::setPath(const QString &path)
{
    m_firstPage->setPath(path);
}

QString GenericProjectWizardDialog::projectName() const
{
    return m_firstPage->fileName();
}

GenericProjectWizard::GenericProjectWizard()
    : Core::BaseFileWizard(parameters())
{ }

GenericProjectWizard::~GenericProjectWizard()
{ }

Core::BaseFileWizardParameters GenericProjectWizard::parameters()
{
    static Core::BaseFileWizardParameters parameters(ProjectWizard);
    parameters.setIcon(QIcon(":/wizards/images/console.png"));
    parameters.setDisplayName(tr("Import of Makefile-based Project"));
    parameters.setId(QLatin1String("Z.Makefile"));
    parameters.setDescription(tr("Creates a generic project, supporting any build system."));
    parameters.setCategory(QLatin1String(ProjectExplorer::Constants::PROJECT_WIZARD_CATEGORY));
    parameters.setDisplayCategory(QCoreApplication::translate("ProjectExplorer", ProjectExplorer::Constants::PROJECT_WIZARD_TR_CATEGORY));
    return parameters;
}

QWizard *GenericProjectWizard::createWizardDialog(QWidget *parent,
                                                  const QString &defaultPath,
                                                  const WizardPageList &extensionPages) const
{
    GenericProjectWizardDialog *wizard = new GenericProjectWizardDialog(parent);
    setupWizard(wizard);

    wizard->setPath(defaultPath);

    foreach (QWizardPage *p, extensionPages)
        wizard->addPage(p);

    return wizard;
}

void GenericProjectWizard::getFileList(const QDir &dir, const QString &projectRoot,
                                       const QStringList &suffixes,
                                       QStringList *files, QStringList *paths) const
{
    const QFileInfoList fileInfoList = dir.entryInfoList(QDir::Files |
                                                         QDir::Dirs |
                                                         QDir::NoDotAndDotDot |
                                                         QDir::NoSymLinks);

    foreach (const QFileInfo &fileInfo, fileInfoList) {
        QString filePath = fileInfo.absoluteFilePath();
        filePath = filePath.mid(projectRoot.length() + 1);

        if (fileInfo.isDir() && isValidDir(fileInfo)) {
            getFileList(QDir(fileInfo.absoluteFilePath()), projectRoot,
                        suffixes, files, paths);

            if (! paths->contains(filePath))
                paths->append(filePath);
        }

        else if (suffixes.contains(fileInfo.suffix()))
            files->append(filePath);
    }
}

bool GenericProjectWizard::isValidDir(const QFileInfo &fileInfo) const
{
    const QString fileName = fileInfo.fileName();
    const QString suffix = fileInfo.suffix();

    if (fileName.startsWith(QLatin1Char('.')))
        return false;

    else if (fileName == QLatin1String("CVS"))
        return false;    

    // ### user include/exclude

    return true;
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

    Core::ICore *core = Core::ICore::instance();
    Core::MimeDatabase *mimeDatabase = core->mimeDatabase();

    const QStringList suffixes = mimeDatabase->suffixes();

    QStringList sources, paths;
    getFileList(dir, projectPath, suffixes, &sources, &paths);

    Core::MimeType headerTy = mimeDatabase->findByType(QLatin1String("text/x-chdr"));

    QStringList nameFilters;
    foreach (const QRegExp &rx, headerTy.globPatterns())
        nameFilters.append(rx.pattern());

    QStringList includePaths;
    foreach (const QString &path, paths) {
        QFileInfo fileInfo(dir, path);
        QDir thisDir(fileInfo.absoluteFilePath());

        if (! thisDir.entryList(nameFilters, QDir::Files).isEmpty())
            includePaths.append(path);
    }

    Core::GeneratedFile generatedCreatorFile(creatorFileName);
    generatedCreatorFile.setContents(QLatin1String("[General]\n"));

    Core::GeneratedFile generatedFilesFile(filesFileName);
    generatedFilesFile.setContents(sources.join(QLatin1String("\n")));

    Core::GeneratedFile generatedIncludesFile(includesFileName);
    generatedIncludesFile.setContents(includePaths.join(QLatin1String("\n")));

    Core::GeneratedFile generatedConfigFile(configFileName);
    generatedConfigFile.setContents(QLatin1String("// ADD PREDEFINED MACROS HERE!\n"));

    Core::GeneratedFiles files;
    files.append(generatedFilesFile);
    files.append(generatedIncludesFile);
    files.append(generatedConfigFile);
    files.append(generatedCreatorFile);

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

