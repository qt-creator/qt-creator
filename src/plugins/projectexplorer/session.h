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

#pragma once

#include "projectexplorer_export.h"

#include <coreplugin/id.h>

#include <utils/persistentsettings.h>

#include <QString>
#include <QStringList>
#include <QDateTime>

namespace Core { class IEditor; }

namespace ProjectExplorer {

class Project;
class Target;
class BuildConfiguration;
class DeployConfiguration;
class Node;

enum class SetActive { Cascade, NoCascade };

class PROJECTEXPLORER_EXPORT SessionManager : public QObject
{
    Q_OBJECT

public:
    explicit SessionManager(QObject *parent = nullptr);
    ~SessionManager() override;

    static SessionManager *instance();

    // higher level session management
    static QString activeSession();
    static QString lastSession();
    static QStringList sessions();
    static QDateTime sessionDateTime(const QString &session);

    static bool createSession(const QString &session);

    static bool confirmSessionDelete(const QString &session);
    static bool deleteSession(const QString &session);

    static bool cloneSession(const QString &original, const QString &clone);
    static bool renameSession(const QString &original, const QString &newName);

    static bool loadSession(const QString &session);

    static bool save();
    static void closeAllProjects();

    static void addProject(Project *project);
    static void removeProject(Project *project);
    static void removeProjects(const QList<Project *> &remove);

    static void setStartupProject(Project *startupProject);

    static QList<Project *> dependencies(const Project *project);
    static bool hasDependency(const Project *project, const Project *depProject);
    static bool canAddDependency(const Project *project, const Project *depProject);
    static bool addDependency(Project *project, Project *depProject);
    static void removeDependency(Project *project, Project *depProject);

    static bool isProjectConfigurationCascading();
    static void setProjectConfigurationCascading(bool b);

    static void setActiveTarget(Project *p, Target *t, SetActive cascade);
    static void setActiveBuildConfiguration(Target *t, BuildConfiguration *bc, SetActive cascade);
    static void setActiveDeployConfiguration(Target *t, DeployConfiguration *dc, SetActive cascade);

    static Utils::FileName sessionNameToFileName(const QString &session);
    static Project *startupProject();

    static const QList<Project *> projects();
    static bool hasProjects();
    static bool hasProject(Project *p);

    static bool isDefaultVirgin();
    static bool isDefaultSession(const QString &session);

    // Let other plugins store persistent values within the session file
    static void setValue(const QString &name, const QVariant &value);
    static QVariant value(const QString &name);

    // NBS rewrite projectOrder (dependency management)
    static QList<Project *> projectOrder(const Project *project = 0);

    static Project *projectForNode(Node *node);
    static Node *nodeForFile(const Utils::FileName &fileName);
    static Project *projectForFile(const Utils::FileName &fileName);
    static bool projectContainsFile(Project *p, const Utils::FileName &fileName);

    static QStringList projectsForSessionName(const QString &session);

    static void reportProjectLoadingProgress();
    static bool loadingSession();

signals:
    void projectAdded(ProjectExplorer::Project *project); void aboutToRemoveProject(ProjectExplorer::Project *project);
    void projectDisplayNameChanged(ProjectExplorer::Project *project);
    void projectRemoved(ProjectExplorer::Project *project);

    void startupProjectChanged(ProjectExplorer::Project *project);

    void aboutToUnloadSession(QString sessionName);
    void aboutToLoadSession(QString sessionName);
    void sessionLoaded(QString sessionName);
    void aboutToSaveSession();
    void dependencyChanged(ProjectExplorer::Project *a, ProjectExplorer::Project *b);

signals: // for tests only
    void projectFinishedParsing(ProjectExplorer::Project *project);

private:
    static void saveActiveMode(Core::Id mode);
    void clearProjectFileCache();
    static void configureEditor(Core::IEditor *editor, const QString &fileName);
    static void markSessionFileDirty(bool makeDefaultVirginDirty = true);
    static void configureEditors(Project *project);
};

} // namespace ProjectExplorer
