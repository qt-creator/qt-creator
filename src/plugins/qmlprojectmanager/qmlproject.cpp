// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qmlproject.h"

#include "qmlprojectconstants.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>

#include <utils/algorithm.h>
#include <utils/expected.h>
#include <utils/infobar.h>
#include <utils/mimeconstants.h>
#include <utils/predicates.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QTimer>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;
using namespace Qt::Literals::StringLiterals;

namespace {
Utils::Result<FilePath> mcuInstallationRoot()
{
    ExtensionSystem::IPlugin *mcuSupportPlugin = QmlProjectManager::findMcuSupportPlugin();
    if (mcuSupportPlugin == nullptr) {
        return make_unexpected("Failed to find MCU Support plugin"_L1);
    }

    Utils::Result<FilePath> root;
    QMetaObject::invokeMethod(mcuSupportPlugin,
                              "installationRoot",
                              Qt::DirectConnection,
                              Q_RETURN_ARG(Utils::Result<FilePath>, root));

    return root;
}
} // namespace

namespace QmlProjectManager {

ExtensionSystem::IPlugin *findMcuSupportPlugin()
{
    const QString pluginId = "mcusupport";
    const ExtensionSystem::PluginSpec *pluginSpec = Utils::findOrDefault(
        ExtensionSystem::PluginManager::plugins(),
        Utils::equal(&ExtensionSystem::PluginSpec::id, pluginId));

    if (pluginSpec == nullptr) {
        return nullptr;
    }

    return pluginSpec->plugin();
}

Utils::Result<FilePath> mcuFontsDir()
{
    Utils::Result<FilePath> mcuRoot = mcuInstallationRoot();
    if (!mcuRoot) {
        return mcuRoot;
    }

    return *mcuRoot / "src" / "3rdparty" / "fonts";
}

QmlProject::QmlProject(const Utils::FilePath &fileName)
    : Project(Utils::Constants::QMLPROJECT_MIMETYPE, fileName)
{
    setType(QmlProjectManager::Constants::QML_PROJECT_ID);
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::QMLJS_LANGUAGE_ID));
    setDisplayName(fileName.completeBaseName());

    setSupportsBuilding(false);
    setIsEditModePreferred(true);
    setBuildSystemCreator<QmlBuildSystem>();

    if (fileName.endsWith(Constants::fakeProjectName)) {
        auto uiFile = fileName.toUrlishString();
        uiFile.remove(Constants::fakeProjectName);
        auto parentDir = Utils::FilePath::fromString(uiFile).parentDir();

        setDisplayName(parentDir.completeBaseName());
    }

    connect(this, &QmlProject::anyParsingFinished, this, &QmlProject::parsingFinished);
}

void QmlProject::openStartupQmlFile()
{
    const auto qmlBuildSystem = qobject_cast<QmlProjectManager::QmlBuildSystem *>(activeBuildSystem());
    if (!qmlBuildSystem)
        return;

    disconnect(ProjectManager::instance(),
               &ProjectManager::projectAdded,
               this,
               &QmlProject::openStartupQmlFile);

    const Utils::FilePath fileToOpen = qmlBuildSystem->getStartupQmlFileWithFallback();
    QTimer::singleShot(0, qmlBuildSystem, [fileToOpen]() {
        if (!fileToOpen.isEmpty() && fileToOpen.exists() && !fileToOpen.isDir())
            Core::EditorManager::openEditor(fileToOpen, Utils::Id());
    });
}

void QmlProject::parsingFinished(bool success)
{
    // trigger only once
    disconnect(this, &QmlProject::anyParsingFinished, this, &QmlProject::parsingFinished);

    if (!success)
        return;

    if (activeBuildSystem()) {
        openStartupQmlFile();
        return;
    }
    connect(ProjectManager::instance(),
            &ProjectManager::projectAdded,
            this,
            &QmlProject::openStartupQmlFile);
}

Project::RestoreResult QmlProject::fromMap(const Store &map, QString *errorMessage)
{
    RestoreResult result = Project::fromMap(map, errorMessage);
    if (result != RestoreResult::Ok)
        return result;

    if (!activeTarget()) {
        // find a kit that matches prerequisites (prefer default one)
        const QList<Kit *> kits = Utils::filtered(KitManager::kits(), [this](const Kit *k) {
            return !containsType(projectIssues(k), Task::TaskType::Error)
                   && RunDeviceTypeKitAspect::deviceTypeId(k)
                          == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
        });

        if (!kits.isEmpty()) {
            if (kits.contains(KitManager::defaultKit()))
                addTargetForDefaultKit();
            else
                addTargetForKit(kits.first());
        }
    }

    // For projects created with Qt Creator < 17.
    for (Target * const t : targets()) {
        if (t->buildConfigurations().isEmpty())
            t->updateDefaultBuildConfigurations();
        QTC_CHECK(!t->buildConfigurations().isEmpty());
    }

    return RestoreResult::Ok;
}

DeploymentKnowledge QmlProject::deploymentKnowledge() const
{
    return DeploymentKnowledge::Perfect;
}

bool QmlProject::isMCUs()
{
    const QmlProjectManager::QmlBuildSystem *buildSystem
        = qobject_cast<QmlProjectManager::QmlBuildSystem *>(activeBuildSystemForActiveProject());
    return buildSystem && buildSystem->qtForMCUs();
}

} // namespace QmlProjectManager
