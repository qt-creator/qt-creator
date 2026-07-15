// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeautogenparser.h"
#include "cmakebuildconfiguration.h"
#include "cmakebuildstep.h"
#include "cmakebuildsystem.h"
#include "cmakeeditor.h"
#include "cmakeformatter.h"
#include "cmakeinstallstep.h"
#include "cmakelocatorfilter.h"
#include "cmakekitaspect.h"
#include "cmakeoutputparser.h"
#include "cmakeproject.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectimporter.h"
#include "cmakeprojectmanager.h"
#include "cmakeprojectmanagertr.h"
#include "cmakesettingspage.h"
#include "cmaketoolmanager.h"
#include "mcptools.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <extensionsystem/iplugin.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/toolchainkitaspect.h>

#include <texteditor/snippets/snippetprovider.h>

#include <utils/mimeconstants.h>
#include <utils/fsengine/fileiconprovider.h>

#include <QTimer>
#include <QMenu>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager::Internal {

class CMakeProjectPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "CMakeProjectManager.json")

    void initialize() final
    {
        IOptionsPage::registerCategory(
            Constants::Settings::CATEGORY, Tr::tr("CMake"), Constants::Icons::SETTINGS_CATEGORY);

        setupCMakeToolManager(this);

        setupCMakeSettingsPage();
        setupCMakeKitAspects();

        setupCMakeBuildConfiguration();
        setupCMakeBuildStep();
        setupCMakeInstallStep();

        setupCMakeEditor();

        setupCMakeLocatorFilters();
        setupCMakeFormatter();

        setupCMakeManager();

        registerMcpTools();

#ifdef WITH_TESTS
        addTestCreator(createCMakeConfigTest);
        addTestCreator(createCMakeOutputParserTest);
        addTestCreator(createCMakeAutogenParserTest);
        addTestCreator(createCMakeProjectImporterTest);
        addTestCreator(createAddDependenciesTest);
        addTestCreator(createTestPresetsInheritanceTest);
#endif

        FileIconProvider::registerIconOverlayForSuffix(Constants::Icons::FILE_OVERLAY, "cmake");
        FileIconProvider::registerIconOverlayForFilename(Constants::Icons::FILE_OVERLAY,
                                                         Constants::CMAKE_LISTS_TXT);

        TextEditor::SnippetProvider::registerGroup(Constants::CMAKE_SNIPPETS_GROUP_ID,
                                                   Tr::tr("CMake", "SnippetProvider"));
        const auto issuesGenerator = [](const Kit *k) {
            Tasks result;
            if (CMakeKitAspect::cmakeExecutable(k).isEmpty()) {
                result.append(
                    Project::createTask(Task::TaskType::Error, Tr::tr("No cmake tool set.")));
            }
            if (ToolchainKitAspect::toolChains(k).isEmpty()) {
                result.append(
                    Project::createTask(
                        Task::TaskType::Warning, Tr::tr("No compilers set in kit.")));
            }
            return result;
        };
        ProjectManager::registerProjectType<CMakeProject>(
            Utils::Constants::CMAKE_PROJECT_MIMETYPE, issuesGenerator);
    }

    void extensionsInitialized() final
    {
        // Delay the restoration to allow the devices to load first.
        connect(DeviceManager::instance(), &DeviceManager::devicesLoaded, this, [] {
            CMakeToolManager::restoreCMakeTools();
        });

        setupOnlineHelpManager();
    }
};

} // CMakeProjectManager::Internal

#include "cmakeprojectplugin.moc"
