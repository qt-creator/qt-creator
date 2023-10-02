// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    void runCMakeWithProfiling(ProjectExplorer::BuildSystem *buildSystem);
    void rescanProject(ProjectExplorer::BuildSystem *buildSystem);
    void buildFileContextMenu();
    void buildFile(ProjectExplorer::Node *node = nullptr);
    void updateBuildFileAction();
    void enableBuildFileMenus(ProjectExplorer::Node *node);
    void reloadCMakePresets();

    QAction *m_runCMakeAction;
    QAction *m_clearCMakeCacheAction;
    QAction *m_runCMakeActionContextMenu;
    QAction *m_rescanProjectAction;
    QAction *m_buildFileContextMenu;
    QAction *m_reloadCMakePresetsAction;
    Utils::ParameterAction *m_buildFileAction;
    QAction *m_cmakeProfilerAction;
    QAction *m_cmakeDebuggerAction;
    QAction *m_cmakeDebuggerSeparator;
    bool m_canDebugCMake = false;
};

} // namespace Internal
} // namespace CMakeProjectManager
