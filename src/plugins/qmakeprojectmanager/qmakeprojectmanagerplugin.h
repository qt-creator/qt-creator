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

#include <extensionsystem/iplugin.h>
#include <coreplugin/icontext.h>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Project;
class Target;
}
namespace Utils { class ParameterAction; }

namespace QmakeProjectManager {

class QmakeManager;
class QmakeProject;

namespace Internal {

class QmakeProjectManagerPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QmakeProjectManager.json")

public:
    bool initialize(const QStringList &arguments, QString *errorMessage);
    void extensionsInitialized();

#ifdef WITH_TESTS
private slots:
    void testQmakeOutputParsers_data();
    void testQmakeOutputParsers();
    void testMakefileParser_data();
    void testMakefileParser();
#endif

private:
    void projectChanged();
    void activeTargetChanged();
    void updateRunQMakeAction();
    void updateContextActions();
    void buildStateChanged(ProjectExplorer::Project *pro);
    void updateBuildFileAction();

    QmakeManager *m_qmakeProjectManager = nullptr;
    QmakeProject *m_previousStartupProject = nullptr;
    ProjectExplorer::Target *m_previousTarget = nullptr;

    QAction *m_runQMakeAction = nullptr;
    QAction *m_runQMakeActionContextMenu = nullptr;
    Utils::ParameterAction *m_buildSubProjectContextMenu = nullptr;
    QAction *m_subProjectRebuildSeparator = nullptr;
    QAction *m_rebuildSubProjectContextMenu = nullptr;
    QAction *m_cleanSubProjectContextMenu = nullptr;
    QAction *m_buildFileContextMenu = nullptr;
    Utils::ParameterAction *m_buildSubProjectAction = nullptr;
    Utils::ParameterAction *m_rebuildSubProjectAction = nullptr;
    Utils::ParameterAction *m_cleanSubProjectAction = nullptr;
    Utils::ParameterAction *m_buildFileAction = nullptr;
    QAction *m_addLibraryAction = nullptr;
    QAction *m_addLibraryActionContextMenu = nullptr;
    Core::Context m_projectContext;
};

} // namespace Internal
} // namespace QmakeProjectManager
