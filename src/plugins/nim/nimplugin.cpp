/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

#include "nimplugin.h"

#include "nimconstants.h"
#include "editor/nimeditorfactory.h"
#include "editor/nimhighlighter.h"
#include "project/nimbuildconfigurationfactory.h"
#include "project/nimcompilerbuildstepfactory.h"
#include "project/nimcompilercleanstepfactory.h"
#include "project/nimproject.h"
#include "project/nimrunconfiguration.h"
#include "project/nimrunconfigurationfactory.h"
#include "project/nimtoolchainfactory.h"
#include "settings/nimcodestylepreferencesfactory.h"
#include "settings/nimcodestylesettingspage.h"
#include "settings/nimsettings.h"

#include <coreplugin/fileiconprovider.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/runconfiguration.h>
#include <texteditor/snippets/snippetprovider.h>

#include <QtPlugin>

using namespace Utils;
using namespace ProjectExplorer;

namespace Nim {

static NimPlugin *m_instance = 0;

NimPlugin::NimPlugin()
{
    m_instance = this;
}

NimPlugin::~NimPlugin()
{
    m_instance = 0;
}

bool NimPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    ProjectExplorer::ToolChainManager::registerLanguage(Constants::C_NIMLANGUAGE_ID, Constants::C_NIMLANGUAGE_NAME);

    RunControl::registerWorker<NimRunConfiguration, SimpleTargetRunner>
            (ProjectExplorer::Constants::NORMAL_RUN_MODE);

    addAutoReleasedObject(new NimSettings);
    addAutoReleasedObject(new NimEditorFactory);
    addAutoReleasedObject(new NimBuildConfigurationFactory);
    addAutoReleasedObject(new NimRunConfigurationFactory);
    addAutoReleasedObject(new NimCompilerBuildStepFactory);
    addAutoReleasedObject(new NimCompilerCleanStepFactory);
    addAutoReleasedObject(new NimCodeStyleSettingsPage);
    addAutoReleasedObject(new NimCodeStylePreferencesFactory);
    addAutoReleasedObject(new NimToolChainFactory);
    TextEditor::SnippetProvider::registerGroup(Constants::C_NIMSNIPPETSGROUP_ID,
                                               tr("Nim", "SnippetProvider"),
                                               &NimEditorFactory::decorateEditor);

    ProjectExplorer::ProjectManager::registerProjectType<NimProject>(Constants::C_NIM_PROJECT_MIMETYPE);

    return true;
}

void NimPlugin::extensionsInitialized()
{
    // Add MIME overlay icons (these icons displayed at Project dock panel)
    const QIcon icon((QLatin1String(Constants::C_NIM_ICON_PATH)));
    if (!icon.isNull()) {
        Core::FileIconProvider::registerIconOverlayForMimeType(icon, Constants::C_NIM_MIMETYPE);
        Core::FileIconProvider::registerIconOverlayForMimeType(icon, Constants::C_NIM_SCRIPT_MIMETYPE);
    }
}

} // namespace Nim
