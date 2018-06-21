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

#include "qmakeprojectmanager_global.h"

#include <projectexplorer/projectnodes.h>

namespace Core { class IEditor; }
namespace TextEditor { class BaseTextEditor; }

namespace ProjectExplorer {
class Project;
class Node;
class ToolChain;
}

namespace QmakeProjectManager {

class QMAKEPROJECTMANAGER_EXPORT QmakeManager : public QObject
{
    Q_OBJECT

public:
    void notifyChanged(const Utils::FileName &name);

    // Context information used in the slot implementations
    static ProjectExplorer::Node *contextNode();
    static ProjectExplorer::Project *contextProject();
    static ProjectExplorer::FileNode *contextBuildableFileNode();

    enum Action { BUILD, REBUILD, CLEAN };

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
    void handleSubDirContextMenu(Action action, bool isFileBuild);
    void handleSubDirContextMenu(QmakeManager::Action action, bool isFileBuild,
                                 ProjectExplorer::Project *contextProject,
                                 ProjectExplorer::Node *contextProFileNode,
                                 ProjectExplorer::FileNode *buildableFile);
    void addLibraryImpl(const QString &fileName, TextEditor::BaseTextEditor *editor);
    void runQMakeImpl(ProjectExplorer::Project *p, ProjectExplorer::Node *node);
};

} // namespace QmakeProjectManager
