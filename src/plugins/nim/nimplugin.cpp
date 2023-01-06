// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimplugin.h"

#include "nimconstants.h"
#include "nimtr.h"
#include "editor/nimeditorfactory.h"
#include "project/nimblerunconfiguration.h"
#include "project/nimblebuildconfiguration.h"
#include "project/nimbuildconfiguration.h"
#include "project/nimcompilerbuildstep.h"
#include "project/nimcompilercleanstep.h"
#include "project/nimproject.h"
#include "project/nimbleproject.h"
#include "project/nimrunconfiguration.h"
#include "project/nimtoolchainfactory.h"
#include "project/nimblebuildstep.h"
#include "project/nimbletaskstep.h"
#include "settings/nimcodestylepreferencesfactory.h"
#include "settings/nimcodestylesettingspage.h"
#include "settings/nimsettings.h"
#include "suggest/nimsuggestcache.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchainmanager.h>

#include <texteditor/snippets/snippetprovider.h>

#include <utils/fsengine/fileiconprovider.h>

using namespace Utils;
using namespace ProjectExplorer;

namespace Nim {

class NimPluginPrivate
{
public:
    NimPluginPrivate()
    {
        Suggest::NimSuggestCache::instance().setExecutablePath(settings.nimSuggestPath.value());
        QObject::connect(&settings.nimSuggestPath, &StringAspect::valueChanged,
                         &Suggest::NimSuggestCache::instance(),
                         &Suggest::NimSuggestCache::setExecutablePath);
    }

    NimSettings settings;
    NimEditorFactory editorFactory;
    NimBuildConfigurationFactory buildConfigFactory;
    NimbleBuildConfigurationFactory nimbleBuildConfigFactory;
    NimRunConfigurationFactory nimRunConfigFactory;
    NimbleRunConfigurationFactory nimbleRunConfigFactory;
    NimbleTestConfigurationFactory nimbleTestConfigFactory;
    SimpleTargetRunnerFactory nimRunWorkerFactory{{nimRunConfigFactory.runConfigurationId()}};
    SimpleTargetRunnerFactory nimbleRunWorkerFactory{{nimbleRunConfigFactory.runConfigurationId()}};
    SimpleTargetRunnerFactory nimbleTestWorkerFactory{{nimbleTestConfigFactory.runConfigurationId()}};
    NimbleBuildStepFactory nimbleBuildStepFactory;
    NimbleTaskStepFactory nimbleTaskStepFactory;
    NimCompilerBuildStepFactory buildStepFactory;
    NimCompilerCleanStepFactory cleanStepFactory;
    NimCodeStyleSettingsPage codeStyleSettingsPage;
    NimToolsSettingsPage toolsSettingsPage{&settings};
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
                                               Tr::tr("Nim", "SnippetProvider"),
                                               &NimEditorFactory::decorateEditor);

    ProjectManager::registerProjectType<NimProject>(Constants::C_NIM_PROJECT_MIMETYPE);
    ProjectManager::registerProjectType<NimbleProject>(Constants::C_NIMBLE_MIMETYPE);

    return true;
}

void NimPlugin::extensionsInitialized()
{
    // Add MIME overlay icons (these icons displayed at Project dock panel)
    const QIcon icon = Icon({{":/nim/images/settingscategory_nim.png",
            Theme::PanelTextColorDark
        }}, Icon::Tint).icon();
    if (!icon.isNull()) {
        FileIconProvider::registerIconOverlayForMimeType(icon, Constants::C_NIM_MIMETYPE);
        FileIconProvider::registerIconOverlayForMimeType(icon, Constants::C_NIM_SCRIPT_MIMETYPE);
        FileIconProvider::registerIconOverlayForMimeType(icon, Constants::C_NIMBLE_MIMETYPE);
    }
    TaskHub::addCategory(Constants::C_NIMPARSE_ID, "Nim");
}

} // namespace Nim
