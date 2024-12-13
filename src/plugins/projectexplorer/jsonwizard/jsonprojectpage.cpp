// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jsonprojectpage.h"

#include "jsonwizard.h"
#include "../buildsystem.h"
#include "../project.h"
#include "../projectexplorerconstants.h"
#include "../projectexplorertr.h"
#include "../projectnodes.h"
#include "../projectmanager.h"
#include "../projecttree.h"
#include "../target.h"

#include <coreplugin/documentmanager.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QVariant>

using namespace Utils;

namespace ProjectExplorer {

namespace {
constexpr char UserPreferredPath[] = "UserPreferredPath";
constexpr char UserPreferredNode[] = "UserPreferredNode";
} // anon namespace

JsonProjectPage::JsonProjectPage(QWidget *parent) : ProjectIntroPage(parent)
{ }

void JsonProjectPage::initializePage()
{
    auto wiz = qobject_cast<JsonWizard *>(wizard());
    QTC_ASSERT(wiz, return);
    setFilePath(FilePath::fromString(wiz->stringValue(QLatin1String("InitialPath"))));
    if (wiz->value(QLatin1String(Constants::PROJECT_ENABLESUBPROJECT)).toBool()) {
        initUiForSubProject();
        connect(ProjectTree::instance(), &ProjectTree::treeChanged,
                this, &JsonProjectPage::initUiForSubProject);
    }
    setProjectName(uniqueProjectName(filePath().toString()));
}

bool JsonProjectPage::validatePage()
{
    if (isComplete() && useAsDefaultPath()) {
        // Store the path as default path for new projects if desired.
        Core::DocumentManager::setProjectsDirectory(filePath());
        Core::DocumentManager::setUseProjectsDirectory(true);
    }

    Wizard *wiz = qobject_cast<JsonWizard *>(wizard());
    QTC_ASSERT(wiz, return ProjectIntroPage::validatePage());
    if (!forceSubProject()) {
        wiz->setProperty(Constants::PROJECT_ISSUBPROJECT, false);
        wiz->setProperty("BuildSystem", QVariant());
        wiz->setProperty(Constants::PROJECT_POINTER, QVariant());
        wiz->setProperty(Constants::PREFERRED_PROJECT_NODE, QVariant());
        wiz->setProperty(Constants::PREFERRED_PROJECT_NODE_PATH, QVariant());
        wiz->setSkipForSubprojects(false);
    } else {
        wiz->setProperty(Constants::PROJECT_ISSUBPROJECT, true);
        const FilePath preferred = FilePath::fromVariant(property(UserPreferredPath));

        ProjectIntroPage::ProjectInfo info = currentProjectInfo();
        Project *project = ProjectManager::projectWithProjectFilePath(info.projectFile);
        wiz->setProperty("BuildSystem", info.buildSystem);
        wiz->setProperty(Constants::PROJECT_POINTER, QVariant::fromValue(static_cast<void *>(project)));
        // only if destination path and parent project hasn't changed keep original preferred
        if (!preferred.isEmpty() && preferred == filePath()) {
            wiz->setProperty(Constants::PREFERRED_PROJECT_NODE, property(UserPreferredNode));
            wiz->setProperty(Constants::PREFERRED_PROJECT_NODE_PATH, preferred.toVariant());
        } else {
            wiz->setProperty(Constants::PREFERRED_PROJECT_NODE,
                             project ? QVariant::fromValue(static_cast<void *>(project->rootProjectNode()))
                                     : QVariant());
            wiz->setProperty(Constants::PREFERRED_PROJECT_NODE_PATH, info.projectFile.toVariant());
        }
        wiz->setSkipForSubprojects(true);
    }

    const FilePath target = filePath().pathAppended(projectName());

    wiz->setProperty("ProjectDirectory", target.toString());
    wiz->setProperty("TargetPath", target.toString());

    return Utils::ProjectIntroPage::validatePage();
}

QString JsonProjectPage::uniqueProjectName(const QString &path)
{
    const QDir pathDir(path);
    //: File path suggestion for a new project. If you choose
    //: to translate it, make sure it is a valid path name without blanks
    //: and using only ascii chars.
    const QString prefix = Tr::tr("untitled");
    for (unsigned i = 0; ; ++i) {
        QString name = prefix;
        if (i)
            name += QString::number(i);
        if (!pathDir.exists(name))
            return name;
    }
}

// populate "Add to project" combo, gather build system info
void JsonProjectPage::initUiForSubProject()
{
    auto wiz = qobject_cast<JsonWizard *>(wizard());
    QTC_ASSERT(wiz, return);
    Node * contextNode = nullptr;
    if (void *storedNode = property(UserPreferredNode).value<void *>()) {
        // fixup stored contextNode / node path
        Node *node = static_cast<Node *>(storedNode);
        if (auto p = ProjectManager::projectWithProjectFilePath(node->filePath()))
            contextNode = p->rootProjectNode();
    } else {
        contextNode = static_cast<Node *>(wiz->value(Constants::PREFERRED_PROJECT_NODE).value<void *>());
        if (!contextNode) {
            const QVariant prefProjPath = wiz->value(Constants::PREFERRED_PROJECT_NODE_PATH);
            if (prefProjPath.isValid()) {
                if (auto project = ProjectManager::projectWithProjectFilePath(FilePath::fromVariant(prefProjPath)))
                    contextNode = project->rootProjectNode();
            }
        }
    }
    if (contextNode) {
        const FilePath preferredPath = contextNode->filePath().parentDir();
        setProperty(UserPreferredPath, preferredPath.toVariant());
        setProperty(UserPreferredNode, QVariant::fromValue(static_cast<void *>(contextNode)));
        setFilePath(preferredPath);
    }

    const QList<Project *> currentProjects = ProjectManager::projects();
    QList<ProjectInfo> projectInfos;
    projectInfos.append(
        {Tr::tr("None", "Add to project: None"),
         Core::DocumentManager::projectsDirectory(),
         {},
         {},
         {}});
    int index = -1;
    int counter = 1; // we've added None already
    for (const Project *proj : currentProjects) {
        ProjectNode *rootNode = proj->rootProjectNode();
        if (!rootNode)
            continue;

        const QList<Target *> targets = proj->targets();
        const BuildSystem *bs = targets.isEmpty() ? nullptr : targets.first()->buildSystem();
        if (!bs)
            continue;
        if (bs->isParsing()) {
            connect(bs, &BuildSystem::parsingFinished, this, &JsonProjectPage::initUiForSubProject,
                    Qt::UniqueConnection);
        }
        if (!rootNode->supportsAction(AddSubProject, rootNode))
            continue;

        ProjectInfo info;
        info.projectFile = proj->projectFilePath();
        info.projectId = proj->id();
        info.projectDirectory = proj->rootProjectDirectory();
        info.display = rootNode->displayName() + " - " + proj->projectFilePath().toUserOutput();
        info.buildSystem = (bs ? bs->name() : "");
        if (contextNode && contextNode->getProject() == proj)
            index = counter;
        projectInfos.append(info);
        ++counter;
    }

    setProjectInfos(projectInfos);
    if (index == -1) // if we fail to get a valid preferred parent project, avoid illegal access
        index = 0;
    wiz->setValue(QLatin1String("BuildSystem"), projectInfos.at(index).buildSystem);
    wiz->setValue(QLatin1String("NodeProjectId"), projectInfos.at(index).projectId.toString());
    setProjectIndex(index);
    setForceSubProject(true);
}

} // namespace ProjectExplorer
