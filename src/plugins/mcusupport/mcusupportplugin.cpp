// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcubuildstep.h"
#include "mcukitmanager.h"
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

#include <utils/filepath.h>
#include <utils/infobar.h>

#include <QAction>
#include <QDateTime>
#include <QDesktopServices>
#include <QTimer>

using namespace Core;
using namespace ProjectExplorer;

namespace McuSupport::Internal {

const char setupMcuSupportKits[] = "SetupMcuSupportKits";
const char qdsMcuDocInfoEntry[] = "McuDocInfoEntry";

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

static bool isQtMCUsProject(ProjectExplorer::Project *p)
{
    if (!Core::ICore::isQtDesignStudio())
        // should be unreachable
        printMessage("Testing if the QDS project is an MCU project outside the QDS", true);

    if (!p || !p->rootProjectNode())
        return false;

    BuildSystem *bs = p->activeBuildSystem();
    if (!bs)
        return false;
    return bs->additionalData("CustomQtForMCUs").toBool();
}

static void askUserAboutMcuSupportKitsSetup()
{
    if (!ICore::infoBar()->canInfoBeAdded(setupMcuSupportKits)
        || dd->m_options.qulDirFromSettings().isEmpty()
        || !McuKitManager::existingKits(nullptr).isEmpty())
        return;

    Utils::InfoBarEntry info(setupMcuSupportKits,
                             Tr::tr("Create Kits for Qt for MCUs? "
                                    "To do it later, select Edit > Preferences > SDKs > MCU."),
                             Utils::InfoBarEntry::GlobalSuppression::Enabled);
    // clazy:excludeall=connect-3arg-lambda
    info.addCustomButton(Tr::tr("Create Kits for Qt for MCUs"), [] {
        ICore::infoBar()->removeInfo(setupMcuSupportKits);
        QTimer::singleShot(0, []() { ICore::showOptionsDialog(Constants::SETTINGS_ID); });
    });
    ICore::infoBar()->addInfo(info);
}

static void askUserAboutRemovingUninstalledTargetsKits()
{
    const char removeUninstalledKits[] = "RemoveUninstalledKits";
    QList<Kit *> uninstalledTargetsKits;

    if (!ICore::infoBar()->canInfoBeAdded(removeUninstalledKits)
        || (uninstalledTargetsKits = McuKitManager::findUninstalledTargetsKits()).isEmpty())
        return;

    Utils::InfoBarEntry
        info(removeUninstalledKits,
             Tr::tr("Detected %n uninstalled MCU target(s). Remove corresponding kits?",
                    nullptr,
                    uninstalledTargetsKits.size()),
             Utils::InfoBarEntry::GlobalSuppression::Enabled);

    info.addCustomButton(Tr::tr("Keep"), [removeUninstalledKits] {
        ICore::infoBar()->removeInfo(removeUninstalledKits);
    });

    info.addCustomButton(Tr::tr("Remove"), [removeUninstalledKits, uninstalledTargetsKits] {
        ICore::infoBar()->removeInfo(removeUninstalledKits);
        QTimer::singleShot(0, [uninstalledTargetsKits]() {
            McuKitManager::removeUninstalledTargetsKits(uninstalledTargetsKits);
        });
    });

    ICore::infoBar()->addInfo(info);
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

    if (!Core::ICore::isQtDesignStudio()) {
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
    } else {
        // Only in design studio
        connect(ProjectManager::instance(),
                &ProjectManager::projectFinishedParsing,
                [&](ProjectExplorer::Project *p) {
                    if (!isQtMCUsProject(p) || !ICore::infoBar()->canInfoBeAdded(qdsMcuDocInfoEntry))
                        return;
                    Utils::InfoBarEntry docInfo(
                        qdsMcuDocInfoEntry,
                        Tr::tr("Read about using Qt Design Studio for Qt for MCUs."),
                        Utils::InfoBarEntry::GlobalSuppression::Enabled);
                    docInfo.addCustomButton(Tr::tr("Go to the Documentation"), [] {
                        ICore::infoBar()->suppressInfo(qdsMcuDocInfoEntry);
                        QDesktopServices::openUrl(
                            QUrl("https://doc.qt.io/qtdesignstudio/studio-on-mcus.html"));
                    });
                    ICore::infoBar()->addInfo(docInfo);
                });
    }

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

} // namespace McuSupport::Internal

#include "mcusupportplugin.moc"
