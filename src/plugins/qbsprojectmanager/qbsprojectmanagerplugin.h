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

#ifndef QBSPROJECTMANAGERPLUGIN_H
#define QBSPROJECTMANAGERPLUGIN_H

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
    QbsProjectManagerPlugin();

    bool initialize(const QStringList &arguments, QString *errorMessage);

    void extensionsInitialized();

private slots:
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

private:
    void updateContextActions();
    void updateReparseQbsAction();
    void updateBuildActions();

    void buildFiles(QbsProject *project, const QStringList &files,
                    const QStringList &activeFileTags);
    void buildSingleFile(QbsProject *project, const QString &file);
    void buildProducts(QbsProject *project, const QStringList &products);

    QAction *m_reparseQbs;
    QAction *m_reparseQbsCtx;
    QAction *m_buildFileCtx;
    QAction *m_buildProductCtx;
    QAction *m_buildSubprojectCtx;
    Utils::ParameterAction *m_buildFile;
    Utils::ParameterAction *m_buildProduct;
    Utils::ParameterAction *m_buildSubproject;

    Internal::QbsProject *m_selectedProject;
    ProjectExplorer::Node *m_selectedNode;

    Internal::QbsProject *m_currentProject;

    Internal::QbsProject *m_editorProject;
    ProjectExplorer::Node *m_editorNode;
};

} // namespace Internal
} // namespace QbsProjectManager

#endif // QBSPROJECTMANAGERPLUGIN_H
