/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "haskellplugin.h"

#include "haskellbuildconfiguration.h"
#include "haskellconstants.h"
#include "haskelleditorfactory.h"
#include "haskellmanager.h"
#include "haskellproject.h"
#include "haskellrunconfiguration.h"
#include "optionspage.h"
#include "stackbuildstep.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/jsonwizard/jsonwizardfactory.h>
#include <texteditor/snippets/snippetprovider.h>

#include <QAction>

namespace Haskell {
namespace Internal {

class HaskellPluginPrivate
{
public:
    HaskellEditorFactory editorFactory;
    OptionsPage optionsPage;
    HaskellBuildConfigurationFactory buildConfigFactory;
    StackBuildStepFactory stackBuildStepFactory;
    HaskellRunConfigurationFactory runConfigFactory;
    ProjectExplorer::SimpleTargetRunnerFactory runWorkerFactory{{Constants::C_HASKELL_RUNCONFIG_ID}};
};

HaskellPlugin::~HaskellPlugin()
{
    delete d;
}

static void registerGhciAction()
{
    QAction *action = new QAction(HaskellManager::tr("Run GHCi"), HaskellManager::instance());
    Core::ActionManager::registerAction(action, Constants::A_RUN_GHCI);
    QObject::connect(action, &QAction::triggered, HaskellManager::instance(), [] {
        if (Core::IDocument *doc = Core::EditorManager::currentDocument())
            HaskellManager::openGhci(doc->filePath());
    });
}

bool HaskellPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    d = new HaskellPluginPrivate;

    ProjectExplorer::ProjectManager::registerProjectType<HaskellProject>(
        Constants::C_HASKELL_PROJECT_MIMETYPE);
    TextEditor::SnippetProvider::registerGroup(Constants::C_HASKELLSNIPPETSGROUP_ID,
                                               tr("Haskell", "SnippetProvider"));

    connect(Core::ICore::instance(), &Core::ICore::saveSettingsRequested, this, [] {
        HaskellManager::writeSettings(Core::ICore::settings());
    });

    registerGhciAction();

    HaskellManager::readSettings(Core::ICore::settings());

    ProjectExplorer::JsonWizardFactory::addWizardPath(":/haskell/share/wizards/");
    return true;
}

} // namespace Internal
} // namespace Haskell
