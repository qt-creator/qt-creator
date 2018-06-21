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
#include "project/nimbuildconfiguration.h"
#include "project/nimcompilerbuildstep.h"
#include "project/nimcompilercleanstep.h"
#include "project/nimproject.h"
#include "project/nimrunconfiguration.h"
#include "project/nimtoolchainfactory.h"
#include "settings/nimcodestylepreferencesfactory.h"
#include "settings/nimcodestylesettingspage.h"
#include "settings/nimsettings.h"

#include <coreplugin/fileiconprovider.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/toolchainmanager.h>
#include <texteditor/snippets/snippetprovider.h>

using namespace Utils;
using namespace ProjectExplorer;

namespace Nim {

class NimPluginPrivate
{
public:
    NimSettings settings;
    NimEditorFactory editorFactory;
    NimBuildConfigurationFactory buildConfigFactory;
    NimRunConfigurationFactory runConfigFactory;
    NimCompilerBuildStepFactory buildStepFactory;
    NimCompilerCleanStepFactory cleanStepFactory;
    NimCodeStyleSettingsPage codeStyleSettingsPage;
    NimCodeStylePreferencesFactory codeStylePreferencesPage;
    NimToolChainFactory toolChainFactory;
};

NimPlugin::~NimPlugin()
{
    delete d;
}

bool NimPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    d = new NimPluginPrivate;

    ToolChainManager::registerLanguage(Constants::C_NIMLANGUAGE_ID, Constants::C_NIMLANGUAGE_NAME);

    TextEditor::SnippetProvider::registerGroup(Constants::C_NIMSNIPPETSGROUP_ID,
                                               tr("Nim", "SnippetProvider"),
                                               &NimEditorFactory::decorateEditor);

    ProjectManager::registerProjectType<NimProject>(Constants::C_NIM_PROJECT_MIMETYPE);

    return true;
}

void NimPlugin::extensionsInitialized()
{
    // Add MIME overlay icons (these icons displayed at Project dock panel)
    const QIcon icon = Utils::Icon({{":/nim/images/settingscategory_nim.png",
                                     Utils::Theme::PanelTextColorDark}}, Utils::Icon::Tint).icon();
    if (!icon.isNull()) {
        Core::FileIconProvider::registerIconOverlayForMimeType(icon, Constants::C_NIM_MIMETYPE);
        Core::FileIconProvider::registerIconOverlayForMimeType(icon, Constants::C_NIM_SCRIPT_MIMETYPE);
    }
}

} // namespace Nim
