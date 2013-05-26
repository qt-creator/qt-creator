/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkvoic
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
#include "menuhandler.h"

#include <QAction>
#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorer.h>

#include "vcprojectmanagerconstants.h"
#include "vcproject.h"
#include "vcprojectmodel/vcdocprojectnodes.h"
#include "vcprojectfile.h"
#include "vcprojectmodel/vcprojectdocument.h"
#include "vcprojectmodel/vcdocumentmodel.h"

namespace VcProjectManager {
namespace Internal {

MenuHandler* MenuHandler::m_instance = 0;

MenuHandler::MenuHandler()
{
    m_instance = this;
    initialize();
    initialize2005();
}

MenuHandler *MenuHandler::instance()
{
    return m_instance;
}

MenuHandler::~MenuHandler()
{
}

void MenuHandler::initialize()
{
    const Core::Context projectContext(Constants::VC_PROJECT_ID);

    Core::ActionContainer *projectContextMenu =
            Core::ActionManager::createMenu(ProjectExplorer::Constants::M_PROJECTCONTEXT);
    Core::ActionContainer *subProjectContextMenu =
            Core::ActionManager::createMenu(ProjectExplorer::Constants::M_SUBPROJECTCONTEXT);

    m_projectProperties = new QAction(tr("Show Properties..."), this);
    Core::Command *cmd = Core::ActionManager::registerAction(m_projectProperties, Constants::VC_PROJECT_SHOW_PROPERTIES_ACTION,
                                              projectContext);
    cmd->setAttribute(Core::Command::CA_Hide);
    projectContextMenu->addAction(cmd, ProjectExplorer::Constants::G_PROJECT_FILES);
    subProjectContextMenu->addAction(cmd, ProjectExplorer::Constants::G_PROJECT_FILES);

    connect(m_projectProperties, SIGNAL(triggered()), this, SLOT(onShowProjectSettings()));
}

void MenuHandler::initialize2005()
{
    const Core::Context project2005Context(Constants::VC_PROJECT_2005_ID);

    Core::ActionContainer *projectContextMenu =
            Core::ActionManager::createMenu(ProjectExplorer::Constants::M_PROJECTCONTEXT);
    Core::ActionContainer *subProjectContextMenu =
            Core::ActionManager::createMenu(ProjectExplorer::Constants::M_SUBPROJECTCONTEXT);

    m_projectProperties2005 = new QAction(tr("Show Properties..."), this);
    Core::Command *cmd = Core::ActionManager::registerAction(m_projectProperties2005, Constants::VC_PROJECT_SHOW_PROPERTIES_ACTION,
                                              project2005Context);
    cmd->setAttribute(Core::Command::CA_Hide);
    projectContextMenu->addAction(cmd, ProjectExplorer::Constants::G_PROJECT_FILES);
    subProjectContextMenu->addAction(cmd, ProjectExplorer::Constants::G_PROJECT_FILES);

    connect(m_projectProperties2005, SIGNAL(triggered()), this, SLOT(onShowProjectSettings()));
}

void MenuHandler::onShowProjectSettings()
{
    VcProject *project = dynamic_cast<VcProject *>(ProjectExplorer::ProjectExplorerPlugin::currentProject());

    if (project)
        project->showSettingsDialog();
}

} // namespace Internal
} // namespace VcProjectManager
