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

#ifndef QT4PROJECTMANAGER_H
#define QT4PROJECTMANAGER_H

#include <projectexplorer/iprojectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <coreplugin/dialogs/iwizard.h>

#include <QtCore/QModelIndex>

namespace ExtensionSystem {
class PluginManager;
}

namespace ProjectExplorer {
class Project;
class ProjectExplorerPlugin;
}

namespace Qt4ProjectManager {

namespace Internal {
class Qt4Builder;
class ProFileEditor;
class Qt4ProjectManagerPlugin;
class QtVersionManager;
}

class Qt4Project;

class Qt4Manager : public ProjectExplorer::IProjectManager
{
    Q_OBJECT

public:
    Qt4Manager(Internal::Qt4ProjectManagerPlugin *plugin);
    ~Qt4Manager();

    void init();

    void registerProject(Qt4Project *project);
    void unregisterProject(Qt4Project *project);
    void notifyChanged(const QString &name);

    ProjectExplorer::ProjectExplorerPlugin *projectExplorer() const;

    // ProjectExplorer::IProjectManager
    int projectContext() const;
    int projectLanguage() const;

    virtual QString mimeType() const;
    ProjectExplorer::Project* openProject(const QString &fileName);

    // Context information used in the slot implementations
    ProjectExplorer::Node *contextNode() const;
    void setContextNode(ProjectExplorer::Node *node);
    void setContextProject(ProjectExplorer::Project *project);
    ProjectExplorer::Project *contextProject() const;

    Internal::QtVersionManager *versionManager() const;

    // Return the id string of a file
    static QString fileTypeId(ProjectExplorer::FileType type);

public slots:
    void runQMake();
    void runQMakeContextMenu();

private:
    QList<Qt4Project *> m_projects;
    void runQMake(ProjectExplorer::Project *p);

    const QString m_mimeType;
    Internal::Qt4ProjectManagerPlugin *m_plugin;
    ProjectExplorer::ProjectExplorerPlugin *m_projectExplorer;

    ProjectExplorer::Node *m_contextNode;
    ProjectExplorer::Project *m_contextProject;

    int m_languageID;

};

} // namespace Qt4ProjectManager

#endif // QT4PROJECTMANAGER_H
