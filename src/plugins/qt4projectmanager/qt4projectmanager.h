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

#ifndef QT4PROJECTMANAGER_H
#define QT4PROJECTMANAGER_H

#include "qt4projectmanager_global.h"

#include <projectexplorer/iprojectmanager.h>
#include <projectexplorer/projectnodes.h>

namespace Core { class IEditor; }
namespace ExtensionSystem { class PluginManager; }

namespace ProjectExplorer {
class Project;
class ProjectExplorerPlugin;
class Node;
class ToolChain;
}

namespace QtSupport {
class QtVersionManager;
class BaseQtVersion;
}

namespace Qt4ProjectManager {

namespace Internal {
class Qt4Builder;
class ProFileEditorWidget;
class Qt4ProjectManagerPlugin;
} // namespace Internal

class Qt4Project;

class QT4PROJECTMANAGER_EXPORT Qt4Manager : public ProjectExplorer::IProjectManager
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

    virtual QString mimeType() const;
    ProjectExplorer::Project *openProject(const QString &fileName, QString *errorString);

    // Context information used in the slot implementations
    ProjectExplorer::Node *contextNode() const;
    void setContextNode(ProjectExplorer::Node *node);
    ProjectExplorer::Project *contextProject() const;
    void setContextProject(ProjectExplorer::Project *project);
    ProjectExplorer::FileNode *contextFile() const;
    void setContextFile(ProjectExplorer::FileNode *file);

    // Return the id string of a file
    static QString fileTypeId(ProjectExplorer::FileType type);

    enum Action { BUILD, REBUILD, CLEAN };

public slots:
    void addLibrary();
    void addLibraryContextMenu();
    void runQMake();
    void runQMakeContextMenu();
    void buildSubDirContextMenu();
    void rebuildSubDirContextMenu();
    void cleanSubDirContextMenu();
    void buildFileContextMenu();

private slots:
    void editorAboutToClose(Core::IEditor *editor);
    void uiEditorContentsChanged();
    void editorChanged(Core::IEditor*);

private:
    QList<Qt4Project *> m_projects;
    void handleSubDirContextMenu(Action action, bool isFileBuild);
    void addLibrary(const QString &fileName, Internal::ProFileEditorWidget *editor = 0);
    void runQMake(ProjectExplorer::Project *p, ProjectExplorer::Node *node);

    Internal::Qt4ProjectManagerPlugin *m_plugin;
    ProjectExplorer::Node *m_contextNode;
    ProjectExplorer::Project *m_contextProject;
    ProjectExplorer::FileNode *m_contextFile;

    Core::IEditor *m_lastEditor;
    bool m_dirty;
};

} // namespace Qt4ProjectManager

#endif // QT4PROJECTMANAGER_H
