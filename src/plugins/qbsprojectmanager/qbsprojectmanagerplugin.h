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

#ifndef QBSPROJECTPLUGIN_H
#define QBSPROJECTPLUGIN_H

#include <extensionsystem/iplugin.h>

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
class QbsManager;

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
    void updateContextActions(ProjectExplorer::Node *node, ProjectExplorer::Project *project);
    void updateReparseQbsAction();
    void activeTargetChanged();
    void buildStateChanged(ProjectExplorer::Project *project);
    void parsingStateChanged();
    void buildFileContextMenu();

    void reparseCurrentProject();

private:
    QbsManager *m_manager;
    ProjectExplorer::ProjectExplorerPlugin *m_projectExplorer;

    QAction *m_reparseQbs;
    QAction *m_reparseQbsCtx;
    QAction *m_buildFileContextMenu;

    Internal::QbsProject *m_currentProject;
    ProjectExplorer::Target *m_currentTarget;
    ProjectExplorer::Node *m_currentNode;
};

} // namespace Internal
} // namespace QbsProject

#endif // QBSPROJECTPLUGIN_H
