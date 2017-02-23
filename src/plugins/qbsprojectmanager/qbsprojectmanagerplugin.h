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
#include <utils/parameteraction.h>

#include <QObject>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Project;
class ProjectExplorerPlugin;
class Node;
class Target;
} // namespace ProjectExplorer

namespace QbsProjectManager {
namespace Internal {

class QbsProject;

class QbsProjectManagerPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QbsProjectManager.json")

public:
    bool initialize(const QStringList &arguments, QString *errorMessage);

    void extensionsInitialized();

private:
    void projectWasAdded(ProjectExplorer::Project *project);
    void currentProjectWasChanged(ProjectExplorer::Project *project);
    void projectWasRemoved();
    void nodeSelectionChanged(ProjectExplorer::Node *node, ProjectExplorer::Project *project);
    void buildStateChanged(ProjectExplorer::Project *project);
    void parsingStateChanged();
    void currentEditorChanged();

    void buildFileContextMenu();
    void buildFile();
    void buildProductContextMenu();
    void buildProduct();
    void buildSubprojectContextMenu();
    void buildSubproject();

    void reparseSelectedProject();
    void reparseCurrentProject();
    void reparseProject(QbsProject *project);

    void updateContextActions();
    void updateReparseQbsAction();
    void updateBuildActions();

    void buildFiles(QbsProject *project, const QStringList &files,
                    const QStringList &activeFileTags);
    void buildSingleFile(QbsProject *project, const QString &file);
    void buildProducts(QbsProject *project, const QStringList &products);

    QAction *m_reparseQbs = nullptr;
    QAction *m_reparseQbsCtx = nullptr;
    QAction *m_buildFileCtx = nullptr;
    QAction *m_buildProductCtx = nullptr;
    QAction *m_buildSubprojectCtx = nullptr;
    Utils::ParameterAction *m_buildFile = nullptr;
    Utils::ParameterAction *m_buildProduct = nullptr;
    Utils::ParameterAction *m_buildSubproject = nullptr;

    Internal::QbsProject *m_selectedProject = nullptr;
    ProjectExplorer::Node *m_selectedNode = nullptr;

    Internal::QbsProject *m_currentProject = nullptr;

    Internal::QbsProject *m_editorProject = nullptr;
    ProjectExplorer::Node *m_editorNode = nullptr;
};

} // namespace Internal
} // namespace QbsProjectManager
