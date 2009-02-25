/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef SESSION_H
#define SESSION_H

#include "projectexplorer_export.h"
#include "projectnodes.h"

#include <coreplugin/ifile.h>

#include <QtCore/QObject>
#include <QtCore/QAbstractItemModel>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace Core {
class ICore;
class IMode;
class IEditor;
}

namespace ProjectExplorer {

class Project;
class Node;
class SessionNode;
class SessionManager;

namespace Internal {

class SessionFile;

// Must be in header as otherwise moc has issues
// with ProjectExplorer::SessionNode on msvc2005
class SessionNodeImpl : public ProjectExplorer::SessionNode
{
    Q_OBJECT
public:
    SessionNodeImpl(SessionManager *manager);

    void addProjectNode(ProjectNode *projectNode);
    void removeProjectNode(ProjectNode *projectNode);

    void setFileName(const QString &fileName);
};

} // namespace Internal

// TODO the interface of this class is not really great

// The implementation suffers that all the functions from the
// public interface just wrap around functions which do the actual work

// This could be improved.
class PROJECTEXPLORER_EXPORT SessionManager : public QObject
{
    Q_OBJECT

public:
    explicit SessionManager(QObject *parent = 0);
    ~SessionManager();

    // higher level session management
    QString activeSession() const;
    QString lastSession() const;
    QStringList sessions() const;

    // creates a new default session and switches to it
    void createAndLoadNewDefaultSession();

    // Just creates a new session (Does not actually create the file)
    bool createSession(const QString &session);

    // delete session name from session list
    // delete file from disk
    bool deleteSession(const QString &session);

    bool cloneSession(const QString &original, const QString &clone);

    // loads a session, takes a session name (not filename)
    bool loadSession(const QString &session);

    bool save();
    bool clear();

    void addProject(Project *project);
    void addProjects(const QList<Project*> &projects);
    void removeProject(Project *project);
    void removeProjects(QList<Project *> remove);

    void setStartupProject(Project *startupProject);

    QList<Project *> dependencies(const Project *project) const;
    bool hasDependency(const Project *project, const Project *depProject) const;
    bool canAddDependency(const Project *project, const Project *depProject) const;
    bool addDependency(const Project *project, const Project *depProject);
    void removeDependency(const Project *project, const Project *depProject);

    Core::IFile *file() const;
    Project *startupProject() const;

    const QList<Project *> &projects() const;

    bool isDefaultVirgin() const;
    bool isDefaultSession(const QString &session) const;

    // Let other plugins store persistent values within the session file
    void setValue(const QString &name, const QVariant &value);
    QVariant value(const QString &name);

    // NBS rewrite projectOrder (dependency management)
    QList<Project *> projectOrder(Project *project = 0) const;
    QAbstractItemModel *model(const QString &modelId) const;

    SessionNode *sessionNode() const;

    Project *projectForNode(ProjectExplorer::Node *node) const;
    Node *nodeForFile(const QString &fileName) const;
    Project *projectForFile(const QString &fileName) const;


    void reportProjectLoadingProgress();

signals:
    void projectAdded(ProjectExplorer::Project *project);
    void singleProjectAdded(ProjectExplorer::Project *project);
    void aboutToRemoveProject(ProjectExplorer::Project *project);

    void projectRemoved(ProjectExplorer::Project *project);

    void startupProjectChanged(ProjectExplorer::Project *project);

    void sessionUnloaded();
    void sessionLoaded();
    void aboutToSaveSession();

private slots:
    void saveActiveMode(Core::IMode *mode);
    void clearProjectFileCache();
    void setEditorCodec(Core::IEditor *editor, const QString &fileName);
    void updateWindowTitle();

private:
    bool loadImpl(const QString &fileName);
    bool createImpl(const QString &fileName);
    QString sessionNameToFileName(const QString &session);
    bool projectContainsFile(Project *p, const QString &fileName) const;

    bool recursiveDependencyCheck(const QString &newDep, const QString &checkDep) const;
    QStringList dependencies(const QString &proName) const;
    QStringList dependenciesOrder() const;
    Project *defaultStartupProject() const;

    QList<Project *> requestCloseOfAllFiles(bool *cancelled);

    void updateName(const QString &session);

    Core::ICore *m_core;

    Internal::SessionFile *m_file;
    Internal::SessionNodeImpl *m_sessionNode;
    QString m_displayName;
    QString m_sessionName;

    mutable
    QHash<Project *, QStringList> m_projectFileCache;
};

} // namespace ProjectExplorer

#endif // SESSION_H
