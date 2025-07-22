// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qmlproject.h"

#include "qmlprojectconstants.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

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
#include <utils/mimeconstants.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QTimer>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace QmlProjectManager {

QmlProject::QmlProject(const Utils::FilePath &fileName)
    : Project(Utils::Constants::QMLPROJECT_MIMETYPE, fileName)
{
    setType(QmlProjectManager::Constants::QML_PROJECT_ID);
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::QMLJS_LANGUAGE_ID));
    setDisplayName(fileName.completeBaseName());

    setSupportsBuilding(false);
    setIsEditModePreferred(!Core::ICore::isQtDesignStudio());
    setBuildSystemCreator<QmlBuildSystem>("qml");

    if (Core::ICore::isQtDesignStudio()) {
        if (allowOnlySingleProject() && !fileName.endsWith(Constants::fakeProjectName)) {
            EditorManager::closeAllDocuments();
            ProjectManager::closeAllProjects();
        }
    }

    if (fileName.endsWith(Constants::fakeProjectName)) {
        auto uiFile = fileName.toUrlishString();
        uiFile.remove(Constants::fakeProjectName);
        auto parentDir = Utils::FilePath::fromString(uiFile).parentDir();

        setDisplayName(parentDir.completeBaseName());
    }

    connect(this, &QmlProject::anyParsingFinished, this, &QmlProject::parsingFinished);
}

void QmlProject::parsingFinished(bool success)
{
    // trigger only once
    disconnect(this, &QmlProject::anyParsingFinished, this, &QmlProject::parsingFinished);

    if (!success || !activeBuildSystem())
        return;

    const auto qmlBuildSystem = qobject_cast<QmlProjectManager::QmlBuildSystem *>(
        activeBuildSystem());
    if (!qmlBuildSystem)
        return;

    const auto openFile = [&](const Utils::FilePath file) {
        //why is this timer needed here?
        QTimer::singleShot(1000, this, [file] {
            Core::EditorManager::openEditor(file, Utils::Id());
        });
    };

    const Utils::FilePath fileToOpen = qmlBuildSystem->getStartupQmlFileWithFallback();
    if (!fileToOpen.isEmpty() && fileToOpen.exists() && !fileToOpen.isDir())
        openFile(fileToOpen);
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

        // FIXME: are there any other way?
        // What if it's not a Design Studio project? What should we do then?
        if (Core::ICore::isQtDesignStudio()) {
            int preferedVersion = preferedQtTarget(activeTarget());

            setKitWithVersion(preferedVersion, kits);
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

bool QmlProject::setKitWithVersion(const int qtMajorVersion, const QList<Kit *> kits)
{
    const QList<Kit *> qtVersionkits = Utils::filtered(kits, [qtMajorVersion](const Kit *k) {
        if (!k->detectionSource().isAutoDetected())
            return false;

        if (k->isReplacementKit())
            return false;

        QtSupport::QtVersion *version = QtSupport::QtKitAspect::qtVersion(k);
        return (version && version->qtVersion().majorVersion() == qtMajorVersion);
    });


    Target *target = nullptr;

    if (!qtVersionkits.isEmpty()) {
        if (qtVersionkits.contains(KitManager::defaultKit()))
            target = addTargetForDefaultKit();
        else
            target = addTargetForKit(qtVersionkits.first());
    }

    if (target)
        target->project()->setActiveTarget(target, SetActive::NoCascade);

    return true;
}

Utils::FilePaths QmlProject::collectUiQmlFilesForFolder(const Utils::FilePath &folder) const
{
    const Utils::FilePaths uiFiles = files([&](const Node *node) {
        return node->filePath().completeSuffix() == "ui.qml"
               && node->filePath().parentDir() == folder;
    });
    return uiFiles;
}

Utils::FilePaths QmlProject::collectQmlFiles() const
{
    const Utils::FilePaths qmlFiles = files([&](const Node *node) {
        return node->filePath().completeSuffix() == "qml";
    });
    return qmlFiles;
}

bool QmlProject::isQtDesignStudioStartedFromQtC()
{
    return qEnvironmentVariableIsSet(Constants::enviromentLaunchedQDS);
}

DeploymentKnowledge QmlProject::deploymentKnowledge() const
{
    return DeploymentKnowledge::Perfect;
}

int QmlProject::preferedQtTarget(Target *target)
{
    if (!target)
        return -1;

    auto buildSystem = qobject_cast<QmlBuildSystem *>(target->buildSystem());
    return (buildSystem && buildSystem->qt6Project()) ? 6 : 5;
}

bool QmlProject::allowOnlySingleProject()
{
    QtcSettings *settings = Core::ICore::settings();
    const Key key = "QML/Designer/AllowMultipleProjects";
    return !settings->value(key, false).toBool();
}

bool QmlProject::isMCUs()
{
    const QmlProjectManager::QmlBuildSystem *buildSystem
        = qobject_cast<QmlProjectManager::QmlBuildSystem *>(activeBuildSystemForActiveProject());
    return buildSystem && buildSystem->qtForMCUs();
}

QmlProject::Version QmlProject::qtQuickVersion()
{
    const QmlProjectManager::QmlBuildSystem *buildSystem
        = qobject_cast<QmlProjectManager::QmlBuildSystem *>(activeBuildSystemForActiveProject());

    if (buildSystem) {
        const QStringList versions = buildSystem->versionQtQuick().split('.');
        if (versions.size() >= 2) {
            const QString majorVersion = versions[0];
            const QString minorVersion = versions[1];
            Version version {
                majorVersion.isEmpty() ? -1 : majorVersion.toInt(),
                minorVersion.isEmpty() ? -1 : minorVersion.toInt()
            };
            return version;
        }
    }
    return {};
}

} // namespace QmlProjectManager
