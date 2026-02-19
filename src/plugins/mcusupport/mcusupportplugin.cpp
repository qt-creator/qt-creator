// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcubuildstep.h"
#include "mcukitmanager.h"
#include "mculegacyconstants.h"
#include "mcuqmlprojectnode.h"
#include "mcusupportconstants.h"
#include "mcusupportdevice.h"
#include "mcusupportimportprovider.h"
#include "mcusupportoptions.h"
#include "mcusupportoptionspage.h"
#include "mcusupportrunconfiguration.h"
#include "mcusupporttr.h"

#if defined(WITH_TESTS) && defined(GOOGLE_TEST_IS_FOUND)
#include "test/unittest.h"
#endif

#include <cmakeprojectmanager/cmakekitaspect.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <extensionsystem/iplugin.h>

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/target.h>

#include <cmakeprojectmanager/cmakeprojectconstants.h>

#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljstools/qmljstoolsconstants.h>

#include <utils/expected.h>
#include <utils/filepath.h>
#include <utils/infobar.h>

#include <QAction>
#include <QDateTime>
#include <QDesktopServices>
#include <QTimer>

using namespace Core;
using namespace ProjectExplorer;

using namespace Qt::Literals::StringLiterals;

namespace McuSupport::Internal {

const char setupMcuSupportKits[] = "SetupMcuSupportKits";

void printMessage(const QString &message, bool important)
{
    const QString displayMessage = Tr::tr("Qt for MCUs: %1").arg(message);
    if (important)
        Core::MessageManager::writeFlashing(displayMessage);
    else
        Core::MessageManager::writeSilently(displayMessage);
}

void updateMCUProjectTree(ProjectExplorer::Project *p)
{
    if (!p || !p->rootProjectNode())
        return;
    ProjectExplorer::Kit *kit = p->activeKit();
    if (!kit || !kit->hasValue(Constants::KIT_MCUTARGET_KITVERSION_KEY))
        return;

    p->rootProjectNode()->forEachProjectNode([](const ProjectNode *node) {
        if (!node)
            return;

        const FilePath projectBuildFolder = FilePath::fromVariant(
            node->data(CMakeProjectManager::Constants::BUILD_FOLDER_ROLE));
        const QString targetName = node->displayName();
        if (targetName.isEmpty())
            return;

        const FilePath inputsJsonFile = projectBuildFolder / "CMakeFiles" / (targetName + ".dir")
                                        / "config/input.json";

        if (!inputsJsonFile.exists())
            return;

        auto qmlProjectNode = std::make_unique<McuQmlProjectNode>(FilePath(node->filePath()),
                                                                  inputsJsonFile);

        const_cast<ProjectNode *>(node)->replaceSubtree(nullptr, std::move(qmlProjectNode));
    });
};

class McuSupportPluginPrivate
{
public:
    McuSupportDeviceFactory deviceFactory;
    McuSupportRunConfigurationFactory runConfigurationFactory;
    FlashRunWorkerFactory flashRunWorkerFactory;
    SettingsHandler::Ptr m_settingsHandler{new SettingsHandler};
    McuSupportOptions m_options{m_settingsHandler};
    McuSupportOptionsPage optionsPage{m_options, m_settingsHandler};
    MCUBuildStepFactory mcuBuildStepFactory;
    McuSupportImportProvider mcuImportProvider;
}; // class McuSupportPluginPrivate

static McuSupportPluginPrivate *dd{nullptr};

static void askUserAboutMcuSupportKitsSetup()
{
    InfoBar *infoBar = ICore::popupInfoBar();
    if (!infoBar->canInfoBeAdded(setupMcuSupportKits)
        || dd->m_options.qulDirFromSettings().isEmpty()
        || !McuKitManager::existingKits(nullptr).isEmpty())
        return;

    Utils::InfoBarEntry info(setupMcuSupportKits,
                             Tr::tr("Create Kits for Qt for MCUs? "
                                    "To do it later, select Edit > Preferences > SDKs > MCU."),
                             Utils::InfoBarEntry::GlobalSuppression::Enabled);
    // clazy:excludeall=connect-3arg-lambda
    info.addCustomButton(
        Tr::tr("Create Kits for Qt for MCUs"),
        [] { QTimer::singleShot(0, []() { ICore::showSettings(Constants::SETTINGS_ID); }); },
        {},
        InfoBarEntry::ButtonAction::Hide);
    infoBar->addInfo(info);
}

static void askUserAboutRemovingUninstalledTargetsKits()
{
    const char removeUninstalledKits[] = "RemoveUninstalledKits";
    QList<Kit *> uninstalledTargetsKits;

    InfoBar *infoBar = ICore::popupInfoBar();
    if (!infoBar->canInfoBeAdded(removeUninstalledKits)
        || (uninstalledTargetsKits = McuKitManager::findUninstalledTargetsKits()).isEmpty())
        return;

    Utils::InfoBarEntry
        info(removeUninstalledKits,
             Tr::tr("Detected %n uninstalled MCU target(s). Remove corresponding kits?",
                    nullptr,
                    uninstalledTargetsKits.size()),
             Utils::InfoBarEntry::GlobalSuppression::Enabled);

    info.addCustomButton(Tr::tr("Keep"), [] {}, {}, InfoBarEntry::ButtonAction::Hide);

    info.addCustomButton(
        Tr::tr("Remove"),
        [uninstalledTargetsKits] {
            QTimer::singleShot(0, [uninstalledTargetsKits]() {
                McuKitManager::removeUninstalledTargetsKits(uninstalledTargetsKits);
            });
        },
        {},
        InfoBarEntry::ButtonAction::Hide);

    infoBar->addInfo(info);
}

class McuSupportPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "McuSupport.json")

public:
    ~McuSupportPlugin() final
    {
        delete dd;
        dd = nullptr;
    }

    void initialize() final;
    void extensionsInitialized() final;

    Q_INVOKABLE static void updateDeployStep(ProjectExplorer::BuildConfiguration *bc, bool enabled);
    Q_INVOKABLE static Utils::Result<Utils::FilePath> installationRoot();
};

void McuSupportPlugin::initialize()
{
    setObjectName("McuSupportPlugin");
    dd = new McuSupportPluginPrivate;

    connect(ProjectManager::instance(),
            &ProjectManager::projectFinishedParsing,
            updateMCUProjectTree);

    // Temporary fix for CodeModel/Checker race condition
    // Remove after https://bugreports.qt.io/browse/QTCREATORBUG-29269 is closed

    connect(
        QmlJS::ModelManagerInterface::instance(),
        &QmlJS::ModelManagerInterface::documentUpdated,
        [lasttime = QTime::currentTime()](QmlJS::Document::Ptr doc) mutable {
            // Prevent inifinite recall loop
            auto currenttime = QTime::currentTime();
            if (lasttime.msecsTo(currenttime) < 1000) {
                lasttime = currenttime;
                return;
            }
            lasttime = currenttime;

            if (!doc)
                return;
            //Reset code model only for QtMCUs documents
            const Project *project = ProjectManager::projectForFile(doc->path());
            if (!project)
                return;
            const QList<Target *> targets = project->targets();
            bool isMcuDocument
                = std::any_of(std::begin(targets), std::end(targets), [](const Target *target) {
                      if (!target || !target->kit()
                          || !target->kit()->hasValue(Constants::KIT_MCUTARGET_KITVERSION_KEY))
                          return false;
                      return true;
                  });
            if (!isMcuDocument)
                return;

            Core::ActionManager::command(QmlJSTools::Constants::RESET_CODEMODEL)
                ->action()
                ->trigger();
        });

    dd->m_options.registerQchFiles();
    dd->m_options.registerExamples();

#if defined(WITH_TESTS) && defined(GOOGLE_TEST_IS_FOUND)
    addTest<Test::McuSupportTest>();
#endif
}

void McuSupportPlugin::extensionsInitialized()
{
    DeviceManager::addDevice(McuSupportDevice::create());

    connect(KitManager::instance(), &KitManager::kitsLoaded, this, [] {
        McuKitManager::removeOutdatedKits();
        McuKitManager::createAutomaticKits(dd->m_settingsHandler);
        McuKitManager::fixExistingKits(dd->m_settingsHandler);
        askUserAboutMcuSupportKitsSetup();
        askUserAboutRemovingUninstalledTargetsKits();
    });
}

void McuSupportPlugin::updateDeployStep(ProjectExplorer::BuildConfiguration *bc, bool enabled)
{
    MCUBuildStepFactory::updateDeployStep(bc, enabled);
}

Utils::Result<Utils::FilePath> McuSupportPlugin::installationRoot()
{
    const ProjectExplorer::Kit *kit = MCUBuildStepFactory::findMostRecentQulKit();
    if (kit == nullptr) {
        return make_unexpected("No QUL kits installed"_L1);
    }

    const auto config
        = CMakeProjectManager::CMakeConfigurationKitAspect::configuration(kit).toList();
    const auto key = QString{Internal::Legacy::Constants::QUL_CMAKE_VAR}.toUtf8();
    for (const CMakeProjectManager::CMakeConfigItem &configItem : config) {
        if (configItem.key == key) {
            return Utils::FilePath::fromUserInput(QString::fromUtf8(configItem.value));
        }
    }
    return make_unexpected("No QUL installation root ('%1') key in kit '%2' configuration"_L1
                               .arg(Internal::Legacy::Constants::QUL_CMAKE_VAR)
                               .arg(kit->displayName()));
}

} // namespace McuSupport::Internal

#include "mcusupportplugin.moc"
