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

#ifndef QMAKEPROJECTMANAGER_H
#define QMAKEPROJECTMANAGER_H

#include "qmakeprojectmanager_global.h"

#include <projectexplorer/iprojectmanager.h>
#include <projectexplorer/projectnodes.h>

namespace Core { class IEditor; }
namespace TextEditor { class BaseTextEditor; }

namespace ProjectExplorer {
class Project;
class Node;
class ToolChain;
}

namespace QmakeProjectManager {

class QmakeProject;

class QMAKEPROJECTMANAGER_EXPORT QmakeManager : public ProjectExplorer::IProjectManager
{
    Q_OBJECT

public:
    ~QmakeManager();

    void registerProject(QmakeProject *project);
    void unregisterProject(QmakeProject *project);
    void notifyChanged(const Utils::FileName &name);

    virtual QString mimeType() const;
    ProjectExplorer::Project *openProject(const QString &fileName, QString *errorString);

    // Context information used in the slot implementations
    ProjectExplorer::Node *contextNode() const;
    void setContextNode(ProjectExplorer::Node *node);
    ProjectExplorer::Project *contextProject() const;
    void setContextProject(ProjectExplorer::Project *project);
    ProjectExplorer::FileNode *contextFile() const;
    void setContextFile(ProjectExplorer::FileNode *file);

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
    void buildFile();

private:
    QList<QmakeProject *> m_projects;
    void handleSubDirContextMenu(Action action, bool isFileBuild);
    void handleSubDirContextMenu(QmakeManager::Action action, bool isFileBuild,
                                 ProjectExplorer::Project *contextProject,
                                 ProjectExplorer::Node *contextNode,
                                 ProjectExplorer::FileNode *contextFile);
    void addLibrary(const QString &fileName, TextEditor::BaseTextEditor *editor = 0);
    void runQMake(ProjectExplorer::Project *p, ProjectExplorer::Node *node);

    ProjectExplorer::Node *m_contextNode = nullptr;
    ProjectExplorer::Project *m_contextProject = nullptr;
    ProjectExplorer::FileNode *m_contextFile = nullptr;
};

} // namespace QmakeProjectManager

#endif // QMAKEPROJECTMANAGER_H
