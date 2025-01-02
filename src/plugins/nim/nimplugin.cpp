// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimconstants.h"
#include "nimtr.h"
#include "editor/nimeditorfactory.h"
#include "project/nimblebuildconfiguration.h"
#include "project/nimblebuildstep.h"
#include "project/nimbleproject.h"
#include "project/nimblerunconfiguration.h"
#include "project/nimbletaskstep.h"
#include "project/nimbuildconfiguration.h"
#include "project/nimcompilerbuildstep.h"
#include "project/nimcompilercleanstep.h"
#include "project/nimoutputtaskparser.h"
#include "project/nimproject.h"
#include "project/nimrunconfiguration.h"
#include "project/nimtoolchain.h"
#include "settings/nimcodestylepreferencesfactory.h"
#include "settings/nimcodestylesettingspage.h"

#include <extensionsystem/iplugin.h>

#include <debugger/debuggerruncontrol.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchainmanager.h>

#include <texteditor/snippets/snippetprovider.h>

#include <utils/fsengine/fileiconprovider.h>
#include <utils/icon.h>
#include <utils/theme/theme.h>

using namespace Debugger;
using namespace Utils;
using namespace ProjectExplorer;

namespace Nim {

class NimPluginPrivate
{
public:
    NimEditorFactory editorFactory;
    NimBuildConfigurationFactory buildConfigFactory;
    NimbleBuildConfigurationFactory nimbleBuildConfigFactory;
    NimRunConfigurationFactory nimRunConfigFactory;
    NimbleRunConfigurationFactory nimbleRunConfigFactory;
    NimbleTestConfigurationFactory nimbleTestConfigFactory;
    ProcessRunnerFactory nimRunWorkerFactory{{nimRunConfigFactory.runConfigurationId()}};
    ProcessRunnerFactory nimbleRunWorkerFactory{{nimbleRunConfigFactory.runConfigurationId()}};
    SimpleDebugRunnerFactory nimDebugWorkerFactory{{nimRunConfigFactory.runConfigurationId()}};
    SimpleDebugRunnerFactory nimbleDebugWorkerFactory{{nimbleRunConfigFactory.runConfigurationId()}};
    ProcessRunnerFactory nimbleTestWorkerFactory{{nimbleTestConfigFactory.runConfigurationId()}};
    NimbleBuildStepFactory nimbleBuildStepFactory;
    NimbleTaskStepFactory nimbleTaskStepFactory;
    NimCompilerBuildStepFactory buildStepFactory;
    NimCompilerCleanStepFactory cleanStepFactory;
    NimCodeStyleSettingsPage codeStyleSettingsPage;
    NimCodeStylePreferencesFactory codeStylePreferencesPage;
    NimToolchainFactory toolChainFactory;
};

class NimPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Nim.json")

    ~NimPlugin() final
    {
        delete d;
    }

    void initialize() final
    {
        d = new NimPluginPrivate;

        Core::IOptionsPage::registerCategory(
            Constants::C_NIMTOOLSSETTINGSPAGE_CATEGORY,
            Tr::tr("Nim"),
            ":/nim/images/settingscategory_nim.png");

        // ???
        Core::IOptionsPage::registerCategory(
            Constants::C_NIMCODESTYLESETTINGSPAGE_CATEGORY,
            Tr::tr("Nim"),
            ":/nim/images/settingscategory_nim.png");

        setupNimProject();
        setupNimbleProject();

#ifdef WITH_TESTS
        addTest<NimParserTest>();
#endif

        ToolchainManager::registerLanguage(Constants::C_NIMLANGUAGE_ID, Constants::C_NIMLANGUAGE_NAME);

        TextEditor::SnippetProvider::registerGroup(Constants::C_NIMSNIPPETSGROUP_ID,
                                                   Tr::tr("Nim", "SnippetProvider"),
                                                   &NimEditorFactory::decorateEditor);
    }

    void extensionsInitialized() final
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
    }

private:
    NimPluginPrivate *d = nullptr;
};

} // namespace Nim

#include "nimplugin.moc"
