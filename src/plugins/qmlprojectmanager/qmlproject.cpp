// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qmlproject.h"

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>
#include <qtsupport/qtsupportconstants.h>

#include <QTimer>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

#include "projectexplorer/devicesupport/idevice.h"
#include "qmlprojectconstants.h"
#include "qmlprojectmanagerconstants.h"
#include "qmlprojectmanagertr.h"
#include "utils/algorithm.h"
#include <qmljs/qmljsmodelmanagerinterface.h>

#include <texteditor/textdocument.h>

#include <utils/algorithm.h>
#include <utils/infobar.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QTextCodec>
#include <QTimer>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace QmlProjectManager {

QmlProject::QmlProject(const Utils::FilePath &fileName)
    : Project(QString::fromLatin1(Constants::QMLPROJECT_MIMETYPE), fileName)
{
    setId(QmlProjectManager::Constants::QML_PROJECT_ID);
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::QMLJS_LANGUAGE_ID));
    setDisplayName(fileName.completeBaseName());

    setNeedsBuildConfigurations(false);
    setBuildSystemCreator([](Target *t) { return new QmlBuildSystem(t); });

    if (Core::ICore::isQtDesignStudio()) {
        if (allowOnlySingleProject()) {
            EditorManager::closeAllDocuments();
            ProjectManager::closeAllProjects();
        }
    }

    connect(this, &QmlProject::anyParsingFinished, this, &QmlProject::parsingFinished);
}

void QmlProject::parsingFinished(const Target *target, bool success)
{
    // trigger only once
    disconnect(this, &QmlProject::anyParsingFinished, this, &QmlProject::parsingFinished);

    if (!target || !success || !activeTarget())
        return;

    const auto qmlBuildSystem = qobject_cast<QmlProjectManager::QmlBuildSystem *>(
        activeTarget()->buildSystem());
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

    if (activeTarget())
        return RestoreResult::Ok;

    // find a kit that matches prerequisites (prefer default one)
    const QList<Kit *> kits = Utils::filtered(KitManager::kits(), [this](const Kit *k) {
        return !containsType(projectIssues(k), Task::TaskType::Error)
               && DeviceTypeKitAspect::deviceTypeId(k)
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

    return RestoreResult::Ok;
}

bool QmlProject::setKitWithVersion(const int qtMajorVersion, const QList<Kit *> kits)
{
    const QList<Kit *> qtVersionkits = Utils::filtered(kits, [qtMajorVersion](const Kit *k) {
        if (!k->isAutoDetected())
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

Tasks QmlProject::projectIssues(const Kit *k) const
{
    Tasks result = Project::projectIssues(k);

    const QtSupport::QtVersion *version = QtSupport::QtKitAspect::qtVersion(k);
    if (!version)
        result.append(createProjectTask(Task::TaskType::Warning, Tr::tr("No Qt version set in kit.")));

    IDevice::ConstPtr dev = DeviceKitAspect::device(k);
    if (dev.isNull())
        result.append(createProjectTask(Task::TaskType::Error, Tr::tr("Kit has no device.")));

    if (version && version->qtVersion() < QVersionNumber(5, 0, 0))
        result.append(createProjectTask(Task::TaskType::Error, Tr::tr("Qt version is too old.")));

    if (dev.isNull() || !version)
        return result; // No need to check deeper than this

    if (dev->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
        if (version->type() == QtSupport::Constants::DESKTOPQT) {
            if (version->qmlRuntimeFilePath().isEmpty()) {
                result.append(
                    createProjectTask(Task::TaskType::Error,
                                      Tr::tr("Qt version has no QML utility.")));
            }
        } else {
            // Non-desktop Qt on a desktop device? We don't support that.
            result.append(createProjectTask(Task::TaskType::Error,
                                            Tr::tr("Non-desktop Qt is used with a desktop device.")));
        }
    } else {
        // If not a desktop device, don't check the Qt version for qml runtime binary.
        // The device is responsible for providing it and we assume qml runtime can be found
        // in $PATH if it's not explicitly given.
    }

    return result;
}

bool QmlProject::isQtDesignStudioStartedFromQtC()
{
    return qEnvironmentVariableIsSet(Constants::enviromentLaunchedQDS);
}

DeploymentKnowledge QmlProject::deploymentKnowledge() const
{
    return DeploymentKnowledge::Perfect;
}

bool QmlProject::isEditModePreferred() const
{
    return !Core::ICore::isQtDesignStudio();
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

} // namespace QmlProjectManager
