/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef SESSION_H
#define SESSION_H

#include "projectexplorer_export.h"

#include <utils/persistentsettings.h>

#include <QHash>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QFutureInterface>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QTimer;
QT_END_NAMESPACE

namespace Core {
class IMode;
class IEditor;
}

namespace ProjectExplorer {

class Project;
class Node;
class SessionNode;

class PROJECTEXPLORER_EXPORT SessionManager : public QObject
{
    Q_OBJECT

public:
    explicit SessionManager(QObject *parent = 0);
    ~SessionManager();

    static SessionManager *instance();

    // higher level session management
    static QString activeSession();
    static QString lastSession();
    static QStringList sessions();

    static bool createSession(const QString &session);

    static bool confirmSessionDelete(const QString &session);
    static bool deleteSession(const QString &session);

    static bool cloneSession(const QString &original, const QString &clone);
    static bool renameSession(const QString &original, const QString &newName);

    static bool loadSession(const QString &session);

    static bool save();
    static void closeAllProjects();

    static void addProject(Project *project);
    static void addProjects(const QList<Project*> &projects);
    static void removeProject(Project *project);
    static void removeProjects(QList<Project *> remove);

    static void setStartupProject(Project *startupProject);

    static QList<Project *> dependencies(const Project *project);
    static bool hasDependency(const Project *project, const Project *depProject);
    static bool canAddDependency(const Project *project, const Project *depProject);
    static bool addDependency(Project *project, Project *depProject);
    static void removeDependency(Project *project, Project *depProject);

    static Utils::FileName sessionNameToFileName(const QString &session);
    static Project *startupProject();

    static QList<Project *> projects();
    static bool hasProjects();

    static bool isDefaultVirgin();
    static bool isDefaultSession(const QString &session);

    // Let other plugins store persistent values within the session file
    static void setValue(const QString &name, const QVariant &value);
    static QVariant value(const QString &name);

    // NBS rewrite projectOrder (dependency management)
    static QList<Project *> projectOrder(Project *project = 0);

    static SessionNode *sessionNode();

    static Project *projectForNode(Node *node);
    static QList<Node *> nodesForFile(const Utils::FileName &fileName);
    static Node *nodeForFile(const Utils::FileName &fileName);
    static Project *projectForFile(const Utils::FileName &fileName);

    static QStringList projectsForSessionName(const QString &session);

    static void reportProjectLoadingProgress();
    static bool loadingSession();

signals:
    void projectAdded(ProjectExplorer::Project *project);
    void singleProjectAdded(ProjectExplorer::Project *project);
    void aboutToRemoveProject(ProjectExplorer::Project *project);
    void projectDisplayNameChanged(ProjectExplorer::Project *project);
    void projectRemoved(ProjectExplorer::Project *project);

    void startupProjectChanged(ProjectExplorer::Project *project);

    void aboutToUnloadSession(QString sessionName);
    void aboutToLoadSession(QString sessionName);
    void sessionLoaded(QString sessionName);
    void aboutToSaveSession();
    void dependencyChanged(ProjectExplorer::Project *a, ProjectExplorer::Project *b);

private slots:
    static void saveActiveMode(Core::IMode *mode);
    void clearProjectFileCache();
    static void configureEditor(Core::IEditor *editor, const QString &fileName);
    static void markSessionFileDirty(bool makeDefaultVirginDirty = true);
    static void handleProjectDisplayNameChanged();
private:
    static void configureEditors(Project *project);
};

} // namespace ProjectExplorer

#endif // SESSION_H
