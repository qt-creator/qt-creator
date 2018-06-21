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

#include "genericprojectplugin.h"

#include "genericbuildconfiguration.h"
#include "genericprojectwizard.h"
#include "genericprojectconstants.h"
#include "genericprojectfileseditor.h"
#include "genericmakestep.h"
#include "genericproject.h"

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/selectablefilesmodel.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>

#include <QAction>

using namespace Core;
using namespace ProjectExplorer;

namespace GenericProjectManager {
namespace Internal {

class GenericProjectPluginPrivate : public QObject
{
public:
    GenericProjectPluginPrivate();

    ProjectFilesFactory projectFilesFactory;
    GenericMakeAllStepFactory makeAllStepFactory;
    GenericMakeCleanStepFactory makeCleanStepFactory;
    GenericBuildConfigurationFactory buildConfigFactory;

    QAction editFilesAction{GenericProjectPluginPrivate::tr("Edit Files..."), nullptr};
};

static GenericProjectPluginPrivate *dd = nullptr;

GenericProjectPlugin::~GenericProjectPlugin()
{
    delete dd;
}

bool GenericProjectPlugin::initialize(const QStringList &, QString *)
{
    dd = new GenericProjectPluginPrivate;
    return true;
}

GenericProjectPluginPrivate::GenericProjectPluginPrivate()
{
    ProjectManager::registerProjectType<GenericProject>(Constants::GENERICMIMETYPE);

    IWizardFactory::registerFactoryCreator([] { return QList<IWizardFactory *>{new GenericProjectWizard}; });

    ActionContainer *mproject =
            ActionManager::actionContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT);

    Command *command = ActionManager::registerAction(&editFilesAction,
        "GenericProjectManager.EditFiles", Context(Constants::GENERICPROJECT_ID));
    command->setAttribute(Command::CA_Hide);
    mproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_FILES);

    connect(&editFilesAction, &QAction::triggered, this, [] {
        auto genericProject = qobject_cast<GenericProject *>(ProjectTree::currentProject());
        if (!genericProject)
            return;
        SelectableFilesDialogEditFiles sfd(genericProject->projectDirectory(),
                                           genericProject->files(Project::AllFiles),
                                           ICore::mainWindow());
        if (sfd.exec() == QDialog::Accepted)
            genericProject->setFiles(Utils::transform(sfd.selectedFiles(), &Utils::FileName::toString));
    });
}

} // namespace Internal
} // namespace GenericProjectManager
