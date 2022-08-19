// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/project.h>

namespace Utils { class ParameterAction; }

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace CMakeProjectManager {
namespace Internal {

class CMakeManager : public QObject
{
    Q_OBJECT

public:
    CMakeManager();

private:
    void updateCmakeActions(ProjectExplorer::Node *node);
    void clearCMakeCache(ProjectExplorer::BuildSystem *buildSystem);
    void runCMake(ProjectExplorer::BuildSystem *buildSystem);
    void rescanProject(ProjectExplorer::BuildSystem *buildSystem);
    void buildFileContextMenu();
    void buildFile(ProjectExplorer::Node *node = nullptr);
    void updateBuildFileAction();
    void enableBuildFileMenus(ProjectExplorer::Node *node);

    QAction *m_runCMakeAction;
    QAction *m_clearCMakeCacheAction;
    QAction *m_runCMakeActionContextMenu;
    QAction *m_rescanProjectAction;
    QAction *m_buildFileContextMenu;
    Utils::ParameterAction *m_buildFileAction;
};

} // namespace Internal
} // namespace CMakeProjectManager
