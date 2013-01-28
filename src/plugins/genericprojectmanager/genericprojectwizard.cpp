/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "genericprojectwizard.h"
#include "filesselectionwizardpage.h"

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/customwizard/customwizard.h>

#include <utils/filewizardpage.h>

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QPainter>
#include <QPixmap>
#include <QStyle>

namespace GenericProjectManager {
namespace Internal {

//////////////////////////////////////////////////////////////////////////////
//
// GenericProjectWizardDialog
//
//////////////////////////////////////////////////////////////////////////////

GenericProjectWizardDialog::GenericProjectWizardDialog(QWidget *parent)
    : Utils::Wizard(parent)
{
    setWindowTitle(tr("Import Existing Project"));

    // first page
    m_firstPage = new Utils::FileWizardPage;
    m_firstPage->setTitle(tr("Project Name and Location"));
    m_firstPage->setFileNameLabel(tr("Project name:"));
    m_firstPage->setPathLabel(tr("Location:"));

    // second page
    m_secondPage = new FilesSelectionWizardPage(this);
    m_secondPage->setTitle(tr("File Selection"));

    const int firstPageId = addPage(m_firstPage);
    wizardProgress()->item(firstPageId)->setTitle(tr("Location"));

    const int secondPageId = addPage(m_secondPage);
    wizardProgress()->item(secondPageId)->setTitle(tr("Files"));
}

QString GenericProjectWizardDialog::path() const
{
    return m_firstPage->path();
}

QStringList GenericProjectWizardDialog::selectedPaths() const
{
    return m_secondPage->selectedPaths();
}

QStringList GenericProjectWizardDialog::selectedFiles() const
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
    : Core::BaseFileWizard(parameters())
{ }

Core::FeatureSet GenericProjectWizard::requiredFeatures() const
{
    return Core::FeatureSet();
}

Core::BaseFileWizardParameters GenericProjectWizard::parameters()
{
    Core::BaseFileWizardParameters parameters(ProjectWizard);
    // TODO do something about the ugliness of standard icons in sizes different than 16, 32, 64, 128
    {
        QPixmap icon(22, 22);
        icon.fill(Qt::transparent);
        QPainter p(&icon);
        p.drawPixmap(3, 3, 16, 16, qApp->style()->standardIcon(QStyle::SP_DirIcon).pixmap(16));
        parameters.setIcon(icon);
    }
    parameters.setDisplayName(tr("Import Existing Project"));
    parameters.setId(QLatin1String("Z.Makefile"));
    parameters.setDescription(tr("Imports existing projects that do not use qmake, CMake or Autotools. "
                                 "This allows you to use Qt Creator as a code editor."));
    parameters.setCategory(QLatin1String(ProjectExplorer::Constants::IMPORT_WIZARD_CATEGORY));
    parameters.setDisplayCategory(QLatin1String(ProjectExplorer::Constants::IMPORT_WIZARD_CATEGORY_DISPLAY));
    parameters.setFlags(Core::IWizard::PlatformIndependent);
    return parameters;
}

QWizard *GenericProjectWizard::createWizardDialog(QWidget *parent,
                                                  const Core::WizardDialogParameters &wizardDialogParameters) const
{
    GenericProjectWizardDialog *wizard = new GenericProjectWizardDialog(parent);
    setupWizard(wizard);

    wizard->setPath(wizardDialogParameters.defaultPath());

    foreach (QWizardPage *p, wizardDialogParameters.extensionPages())
        BaseFileWizard::applyExtensionPageShortTitle(wizard, wizard->addPage(p));

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
    const QStringList paths = wizard->selectedPaths();

    Core::MimeDatabase *mimeDatabase = Core::ICore::mimeDatabase();

    Core::MimeType headerTy = mimeDatabase->findByType(QLatin1String("text/x-chdr"));

    QStringList nameFilters;
    foreach (const Core::MimeGlobPattern &gp, headerTy.globPatterns())
        nameFilters.append(gp.regExp().pattern());

    QStringList includePaths;
    foreach (const QString &path, paths) {
        QFileInfo fileInfo(dir, path);
        QDir thisDir(fileInfo.absoluteFilePath());

        if (! thisDir.entryList(nameFilters, QDir::Files).isEmpty())
            includePaths.append(path);
    }

    Core::GeneratedFile generatedCreatorFile(creatorFileName);
    generatedCreatorFile.setContents(QLatin1String("[General]\n"));
    generatedCreatorFile.setAttributes(Core::GeneratedFile::OpenProjectAttribute);

    QStringList sources = wizard->selectedFiles();
    for (int i = 0; i < sources.length(); ++i)
        sources[i] = dir.relativeFilePath(sources[i]);

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

bool GenericProjectWizard::postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage)
{
    Q_UNUSED(w);
    return ProjectExplorer::CustomProjectWizard::postGenerateOpen(l, errorMessage);
}

} // namespace Internal
} // namespace GenericProjectManager
