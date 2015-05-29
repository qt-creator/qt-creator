//
// Copyright (C) 2013 Mateusz Łoskot <mateusz@loskot.net>
// Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
//
// This file is part of Qt Creator Boost.Build plugin project.
//
// This is free software; you can redistribute and/or modify it under
// the terms of the  GNU Lesser General Public License, Version 2.1
// as published by the Free Software Foundation.
// See accompanying file LICENSE.txt or copy at
// http://www.gnu.org/licenses/lgpl-2.1-standalone.html.
//
#include "b2openprojectwizard.h"
#include "b2project.h"
#include "b2projectmanagerconstants.h"
#include "b2utility.h"
#include "filesselectionwizardpage.h"

#include <coreplugin/iwizardfactory.h>
#include <coreplugin/icore.h>
#include <projectexplorer/customwizard/customwizard.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/mimetypes/mimedatabase.h>

#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace BoostBuildProjectManager {
namespace Internal {

//////////////////////////////////////////////////////////////////////////////////////////

OpenProjectWizard::OpenProjectWizard(Project const* const project)
    : m_project(project)
    , m_projectOpened(false)
{
    // Project instance has been created, but it's only partially initialised and
    // rest of the initialization takes place after this wizard completes.
    Q_ASSERT(m_project);

    setDisplayName(tr("Open %1 Project").arg(BBPM_C(BOOSTBUILD)));
    setId(BBPM_C(PROJECT_WIZARD_ID));
    setWizardKind(ProjectWizard); // affects dir vs file path and sub-projects handling

    // TODO: do we need categories or flags?
}

bool OpenProjectWizard::run(QString const& platform, QVariantMap const& extraValues)
{
    QVariantMap extraValuesCopy(extraValues);

    // Project name should be passed by caller, but,
    // since we have Project instance handy, double-check.
    if (!extraValuesCopy.contains(BBPM_C(P_KEY_PROJECTNAME)))
        extraValuesCopy.insert(BBPM_C(P_KEY_PROJECTNAME), m_project->displayName());

    m_projectOpened = false;
    m_outputValues.clear();

    runWizard(m_project->projectFilePath().toString(), 0, platform, extraValuesCopy);

    return m_projectOpened;
}

Core::BaseFileWizard *OpenProjectWizard::create(QWidget* parent, Core::WizardDialogParameters const& parameters) const
{
    OpenProjectWizardDialog* wizard(new OpenProjectWizardDialog(parent
        , parameters.defaultPath(), parameters.extraValues()
        , const_cast<OpenProjectWizard*>(this)->m_outputValues));

    foreach (QWizardPage* p, parameters.extensionPages())
        wizard->addPage(p);

    return wizard;
}

Core::GeneratedFiles
OpenProjectWizard::generateFiles(QWizard const* wizard, QString* errorMessage) const
{
    Q_UNUSED(errorMessage)
    Q_ASSERT(m_project);

    OpenProjectWizardDialog const* openWizard
        = qobject_cast<OpenProjectWizardDialog const*>(wizard);

    QDir const projectDir(openWizard->path());

    // Set up MIME filters for C/C++ headers
    QStringList headerFilters;
    QStringList const headerMimeTypes = QStringList()
        << QLatin1String("text/x-chdr") << QLatin1String("text/x-c++hdr");

    Utils::MimeDatabase mdb;
    foreach (QString const& headerMime, headerMimeTypes) {
        Utils::MimeType mime = mdb.mimeTypeForName(headerMime);
        foreach (QString const& gp, mime.globPatterns())
            headerFilters.append(gp);
    }

    // Generate list of include paths.
    // If any C/C++ headers in any directory from paths, add it to include paths,
    // used for C/C++ parsing only.
    QStringList includePaths;
    QStringList const paths = openWizard->selectedPaths();
    foreach (QString const& path, paths) {
        QFileInfo const fileInfo(path);
        QDir const thisDir(fileInfo.absoluteFilePath());
        if (!thisDir.entryList(headerFilters, QDir::Files).isEmpty()) {
            QString const relative = projectDir.relativeFilePath(thisDir.path());
            includePaths.append(relative.isEmpty() ? QLatin1String(".") : relative);
        }
    }

    // Generate list of sources
    QStringList sources = openWizard->selectedFiles();
    Utility::makeRelativePaths(projectDir.absolutePath(), sources);

    Core::GeneratedFile generatedFilesFile(m_project->filesFilePath());
    generatedFilesFile.setContents(sources.join(QLatin1String("\n")));

    Core::GeneratedFile generatedIncludesFile(m_project->includesFilePath());
    generatedIncludesFile.setContents(includePaths.join(QLatin1String("\n")));

    Core::GeneratedFiles files;
    files.append(generatedFilesFile);
    files.append(generatedIncludesFile);
    return files;
}

bool
OpenProjectWizard::postGenerateFiles(QWizard const* wizard
    , Core::GeneratedFiles const& files, QString* errorMessage)
{
    Q_UNUSED(wizard);

    m_projectOpened
        = ProjectExplorer::CustomProjectWizard::postGenerateOpen(files, errorMessage);

    return m_projectOpened;
}

//////////////////////////////////////////////////////////////////////////////////////////
OpenProjectWizardDialog::OpenProjectWizardDialog(QWidget* parent
    , QString const& projectFile
    , QVariantMap const& extraValues, QVariantMap& outputValues)
    : Core::BaseFileWizard(parent)
    , m_outputValues(outputValues)
    , m_extraValues(extraValues)
    , m_projectFile(projectFile)
{
    setWindowTitle(tr("Open %1 Project").arg(BBPM_C(BOOSTBUILD)));

    m_pathsPage = new PathsSelectionWizardPage(this);
    m_pathsPage->setTitle(tr("Project Name and Paths"));
    int const pathsPageId = addPage(m_pathsPage);
    wizardProgress()->item(pathsPageId)->setTitle(tr("Location"));

    m_filesPage = new FilesSelectionWizardPage(this);
    m_filesPage->setTitle(tr("File Selection"));
    int const filesPageId = addPage(m_filesPage);
    wizardProgress()->item(filesPageId)->setTitle(tr("Files"));
}


QString OpenProjectWizardDialog::path() const
{
    QFileInfo const projectFileInfo(projectFile());
    QTC_ASSERT(projectFileInfo.isFile(), return QString());

    return projectFileInfo.absoluteDir().absolutePath();
}

QString OpenProjectWizardDialog::projectFile() const
{
    return m_projectFile;
}

QString OpenProjectWizardDialog::projectName() const
{
    return m_pathsPage->projectName();
}

QString OpenProjectWizardDialog::defaultProjectName() const
{
    return m_extraValues.value(BBPM_C(P_KEY_PROJECTNAME)).toString();
}

QStringList OpenProjectWizardDialog::selectedFiles() const
{
    return m_filesPage->selectedFiles();
}

QStringList OpenProjectWizardDialog::selectedPaths() const
{
    return m_filesPage->selectedPaths();
}

void OpenProjectWizardDialog::setProjectName(QString const& name)
{
    m_outputValues.insert(QLatin1String(Constants::P_KEY_PROJECTNAME), name);
}

//////////////////////////////////////////////////////////////////////////////////////////
PathsSelectionWizardPage::PathsSelectionWizardPage(OpenProjectWizardDialog* wizard)
    : QWizardPage(wizard)
    , m_wizard(wizard)
{
    QFormLayout *fl = new QFormLayout();
    setLayout(fl);

    QLabel* pathLabel = new QLabel(this);
    pathLabel->setText(tr("Opening the following Jamfile as a project:"));
    fl->addRow(pathLabel);

    QLineEdit* pathLine = new QLineEdit(this);
    pathLine->setReadOnly(true);
    pathLine->setDisabled(true);
    pathLine->setText(m_wizard->projectFile());
    fl->addRow(pathLine);

    QString projectName(Utility::parseJamfileProjectName(m_wizard->projectFile()));
    if (projectName.isEmpty())
        projectName = m_wizard->defaultProjectName();

    m_nameLineEdit = new QLineEdit(this);
    connect(m_nameLineEdit, &QLineEdit::textChanged
          , m_wizard, &OpenProjectWizardDialog::setProjectName);
    m_nameLineEdit->setText(projectName);
    fl->addRow(tr("Project name:"), m_nameLineEdit);

    QLabel* defaultsLabel = new QLabel(this);
    defaultsLabel->setText(tr("Default Boost.Build runtime locations:"));
    fl->addRow(defaultsLabel);

    QLineEdit* workingLine = new QLineEdit(this);
    workingLine->setReadOnly(true);
    workingLine->setDisabled(true);
    workingLine->setText(Project::defaultWorkingDirectory(m_wizard->projectFile()));
    fl->addRow(tr("Working directory:"), workingLine);

    QLineEdit* buildLine = new QLineEdit(this);
    buildLine->setReadOnly(true);
    buildLine->setDisabled(true);
    buildLine->setText(Project::defaultBuildDirectory(m_wizard->projectFile()));
    fl->addRow(tr("Build directory:"), buildLine);

    // TODO: indicate if we can find Boost.Build executable?

    QString const info(tr(
        "This allows you to use Qt Creator as an IDE to edit and navigate C++ code, "
        "run %1 command, work through compilation issues, "
        "configure executable targets to run and debug.")
        .arg(QLatin1String(Constants::BOOSTBUILD)));
    QLabel* infoLabel = new QLabel(this);
    infoLabel->setWordWrap(true);
    infoLabel->setText(info);
    fl->addRow(infoLabel);
}

QString PathsSelectionWizardPage::projectName() const
{
    return m_nameLineEdit->text();
}

} // namespace Internal
} // namespace BoostBuildProjectManager
