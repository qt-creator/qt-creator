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
#include "b2buildconfiguration.h"
#include "b2buildstep.h"
#include "b2openprojectwizard.h"
#include "b2project.h"
#include "b2projectfile.h"
#include "b2projectmanager.h"
#include "b2projectmanagerconstants.h"
#include "b2projectnode.h"
#include "b2utility.h"

#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/generatedfile.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cppprojects.h>
#include <cpptools/cpptoolsconstants.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <qtsupport/customexecutablerunconfiguration.h>
#include <utils/fileutils.h>
#include <utils/QtConcurrentTools>

#include <QDir>
#include <QFileInfo>
#include <QMessageBox>

namespace BoostBuildProjectManager {
namespace Internal {

Project::Project(ProjectManager* manager, QString const& fileName)
    : m_manager(manager)
    , m_filePath(fileName)
    , m_projectFile(new ProjectFile(this, m_filePath)) // enables projectDirectory()
    , m_projectNode(new ProjectNode(this, m_projectFile))
{
    Q_ASSERT(m_manager);
    Q_ASSERT(!m_filePath.isEmpty());

    setProjectContext(Core::Context(Constants::PROJECT_CONTEXT));
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::LANG_CXX));
#if defined(IDE_VERSION_MAJOR) && (IDE_VERSION_MAJOR == 3 && IDE_VERSION_MINOR > 0)
    setId(Constants::PROJECT_ID);
#endif

    QFileInfo const projectFileInfo(m_filePath);
    QDir const projectDir(projectFileInfo.dir());
    m_projectName = defaultProjectName(m_filePath);
    m_filesFilePath = QFileInfo(projectDir
        , m_filePath + QLatin1String(Constants::EXT_JAMFILE_FILES)).absoluteFilePath();
    m_includesFilePath = QFileInfo(projectDir
        , m_filePath + QLatin1String(Constants::EXT_JAMFILE_INCLUDES)).absoluteFilePath();

    m_projectNode->setDisplayName(displayName());

    m_manager->registerProject(this);

    // TODO: Add file watchers
    //projectFileWatcher_->addPath(projectFilePath);
    //connect(projectFileWatcher_, SIGNAL(fileChanged(QString)), this, SLOT(refresh()));

    BBPM_QDEBUG("created project: " << displayName() << " in " << projectDirectory());
}

Project::~Project()
{
    m_manager->unregisterProject(this);
    delete m_projectNode;
}

QString Project::displayName() const
{
    return m_projectName;
}

#if defined(IDE_VERSION_MAJOR) && (IDE_VERSION_MAJOR == 3 && IDE_VERSION_MINOR == 0)
Core::Id Project::id() const
{
    return Core::Id(Constants::PROJECT_ID);
}
#endif

Core::IDocument* Project::document() const
{
    return m_projectFile;
}

ProjectExplorer::IProjectManager* Project::projectManager() const
{
    return m_manager;
}

ProjectExplorer::ProjectNode* Project::rootProjectNode() const
{
    return m_projectNode;
}

QStringList Project::files(FilesMode fileMode) const
{
    // TODO: handle ExcludeGeneratedFiles, but what files exactly?
    //       *.qtcreator.files, *.qtcreator.includes and *.user?
    Q_UNUSED(fileMode);
    return m_files;
}

QStringList Project::files() const
{
    return files(FilesMode::AllFiles);
}

QString Project::filesFilePath() const
{
    Q_ASSERT(!m_filesFilePath.isEmpty());
    return m_filesFilePath;
}

QString Project::includesFilePath() const
{
    Q_ASSERT(!m_includesFilePath.isEmpty());
    return m_includesFilePath;
}

bool Project::needsConfiguration() const
{
    // TODO: Does Boost.Build project require any configuration on loading?
    // - Kit selection
    // - build/stage directory
    // - targets listing
    // CMakeProjectManager seems to request configuration in fromMap()

    return false;
}

/*static*/
QString Project::defaultProjectName(QString const& fileName)
{
    QFileInfo const fileInfo(fileName);
    return fileInfo.absoluteDir().dirName();
}

/*static*/
QString Project::defaultBuildDirectory(QString const& top)
{
    Utils::FileName fn(Utils::FileName::fromString(defaultWorkingDirectory(top)));
    fn.appendPath(BBPM_C(BUILD_DIR_NAME));
    return fn.toString();
}

/*static*/
QString Project::defaultWorkingDirectory(QString const& top)
{
    // Accepts both, project file or project directory, as top.
    return ProjectExplorer::Project::projectDirectory(Utils::FileName::fromString(top)).toString();
}

void Project::setProjectName(QString const& name)
{
    if (m_projectName != name) {
        m_projectName = name;
        m_projectNode->setDisplayName(m_projectName);
        // TODO: signal?
    }
}

QVariantMap Project::toMap() const
{
    QVariantMap map(ProjectExplorer::Project::toMap());
    map.insert(QLatin1String(Constants::P_KEY_PROJECTNAME), m_projectName);
    return map;
}

// This function is called at the very beginning to restore the settings
// from .user file, if there is any with previous settings stored.
bool Project::fromMap(QVariantMap const& map)
{
    BBPM_QDEBUG(displayName());
    QTC_ASSERT(m_projectNode, return false);

    if (!ProjectExplorer::Project::fromMap(map))
        return false;

    QVariantMap extraValues(map);
    if (!extraValues.contains(BBPM_C(P_KEY_PROJECTNAME)))
        extraValues.insert(BBPM_C(P_KEY_PROJECTNAME), m_projectName);
    setProjectName(map.value(BBPM_C(P_KEY_PROJECTNAME)).toString());

    // Check required auxiliary files, run wizard of necessary
    if (!QFileInfo(filesFilePath()).exists() || !QFileInfo(includesFilePath()).exists()) {
        ProjectExplorer::Kit* defaultKit = ProjectExplorer::KitManager::defaultKit();
        Q_ASSERT(defaultKit);

        OpenProjectWizard wizard(this);
        if (!wizard.run(defaultKit->displayName(), extraValues))
            return false;

        QVariantMap outputValues = wizard.outputValues();
        setProjectName(outputValues.value(BBPM_C(P_KEY_PROJECTNAME)).toString());
    }

    // Set up active ProjectConfiguration (aka Target).
    // NOTE: Call setActiveBuildConfiguration when creating new build configurations.
    if (!activeTarget()) {
        // Create project configuration from scratch

        // TODO: Map the Kit to Boost.Build toolset option value
        ProjectExplorer::Kit* defaultKit = ProjectExplorer::KitManager::defaultKit();
        Q_ASSERT(defaultKit);

        // Creates as many {Build|Run|Deploy}Configurations for as corresponding
        // factories report as available.
        // For example, BuildConfigurationFactory::availableBuilds => Debug and Release
        ProjectExplorer::Target* target = createTarget(defaultKit);
        QTC_ASSERT(target, return false);

        addTarget(target);
    } else {
        // Configure project from settings sorced from .user file
        // TODO: Do we need to handle anything from .user here? Do we need this case?
        BBPM_QDEBUG(displayName() << "has user file");
    }

    // Sanity check (taken from GenericProjectManager):
    // We need both a BuildConfiguration and a RunConfiguration!
    QList<ProjectExplorer::Target*> targetList = targets();
    foreach (ProjectExplorer::Target* t, targetList) {
        if (!t->activeBuildConfiguration()) {
            removeTarget(t);
            continue;
        }
        if (!t->activeRunConfiguration())
            t->addRunConfiguration(new QtSupport::CustomExecutableRunConfiguration(t));
    }

    QTC_ASSERT(hasActiveBuildSettings(), return false);
    QTC_ASSERT(activeTarget() != 0, return false);

    // Trigger loading project tree and parsing sources
    refresh();

    return true;
}

void Project::refresh()
{
    QTC_ASSERT(QFileInfo(filesFilePath()).exists(), return);
    QTC_ASSERT(QFileInfo(includesFilePath()).exists(), return);

    QSet<QString> oldFileList;
    oldFileList = m_files.toSet();

    // Parse project:
    // The manager does not parse Jamfile files.
    // Only generates and parses list of source files in Jamfile.${JAMFILE_FILES_EXT}
    QString const projectPath(projectDirectory().toString());
    m_filesRaw = Utility::readLines(filesFilePath());
    m_files = Utility::makeAbsolutePaths(projectPath, m_filesRaw);

    QStringList includePaths =
        Utility::makeAbsolutePaths(projectPath,
            Utility::readLines(includesFilePath()));

    emit fileListChanged();

    m_projectNode->refresh(oldFileList);

    // TODO: Does it make sense to move this to separate asynchronous task?
    // TODO: extract updateCppCodeModel
    CppTools::CppModelManager *modelmanager =
        CppTools::CppModelManager::instance();
    if (modelmanager) {
        CppTools::ProjectInfo pinfo(this);

        CppTools::ProjectPartBuilder builder(pinfo);
        //builder.setDefines(); // TODO: waiting for Jamfile parser
        builder.setIncludePaths(QStringList() << projectDirectory().toString() << includePaths);

        const QList<Core::Id> languages = builder.createProjectPartsForFiles(m_files);
        foreach (Core::Id language, languages)
            setProjectLanguage(language, true);

        m_cppModelFuture.cancel();
        pinfo.finish();
        m_cppModelFuture = modelmanager->updateProjectInfo(pinfo);
    }
}

} // namespace Internal
} // namespace BoostBuildProjectManager
