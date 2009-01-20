/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

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
