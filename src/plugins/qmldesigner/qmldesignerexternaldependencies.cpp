// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmldesignerexternaldependencies.h"

#include "qmldesignerplugin.h"

#include <edit3d/edit3dviewconfig.h>
#include <itemlibraryimport.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>
#include <puppetenvironmentbuilder.h>
#include <qmlpuppetpaths.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>
#include <qmlprojectmanager/buildsystem/qmlbuildsystem.h>

#include <coreplugin/icore.h>

#include <utils/hostosinfo.h>

namespace QmlDesigner {

double ExternalDependencies::formEditorDevicePixelRatio() const
{
    return QmlDesignerPlugin::formEditorDevicePixelRatio();
}

QUrl ExternalDependencies::projectUrl() const
{
    DesignDocument *document = QmlDesignerPlugin::instance()->viewManager().currentDesignDocument();
    if (document)
        return QUrl::fromLocalFile(document->projectFolder().toUrlishString());

    return {};
}

QString ExternalDependencies::projectName() const
{
    return QmlDesignerPlugin::instance()->documentManager().currentProjectName();
}

QString ExternalDependencies::currentProjectDirPath() const
{
    return QmlDesignerPlugin::instance()->documentManager().currentProjectDirPath().toUrlishString();
}

QUrl ExternalDependencies::currentResourcePath() const
{
    return QUrl::fromLocalFile(
        QmlDesigner::DocumentManager::currentResourcePath().toFileInfo().absoluteFilePath());
}

void ExternalDependencies::parseItemLibraryDescriptions() {}

const DesignerSettings &ExternalDependencies::designerSettings() const
{
    return QmlDesigner::designerSettings();
}

void ExternalDependencies::undoOnCurrentDesignDocument()
{
    QmlDesignerPlugin::instance()->currentDesignDocument()->undo();
}

bool ExternalDependencies::viewManagerUsesRewriterView(RewriterView *view) const
{
    return QmlDesignerPlugin::instance()->viewManager().usesRewriterView(view);
}

void ExternalDependencies::viewManagerDiableWidgets()
{
    QmlDesignerPlugin::instance()->viewManager().disableWidgets();
}

QString ExternalDependencies::itemLibraryImportUserComponentsTitle() const
{
    return ItemLibraryImport::userComponentsTitle();
}

bool ExternalDependencies::isQt6Import() const
{
    auto target = ProjectExplorer::ProjectManager::startupTarget();
    if (target) {
        QtSupport::QtVersion *currentQtVersion = QtSupport::QtKitAspect::qtVersion(target->kit());
        if (currentQtVersion && currentQtVersion->isValid()) {
            return currentQtVersion->qtVersion().majorVersion() == 6;
        }
    }

    return false;
}

bool ExternalDependencies::hasStartupTarget() const
{
    auto target = ProjectExplorer::ProjectManager::startupTarget();
    if (target) {
        QtSupport::QtVersion *currentQtVersion = QtSupport::QtKitAspect::qtVersion(target->kit());
        if (currentQtVersion && currentQtVersion->isValid()) {
            return true;
        }
    }

    return false;
}

static bool isForcingFreeType(ProjectExplorer::BuildSystem *buildSystem)
{
    if (Utils::HostOsInfo::isWindowsHost() && buildSystem) {
        const QVariant customData = buildSystem->additionalData("CustomForceFreeType");

        if (customData.isValid())
            return customData.toBool();
    }

    return false;
}

static QString createFreeTypeOption(ProjectExplorer::BuildSystem *buildSystem)
{
    if (isForcingFreeType(buildSystem))
        return "-platform windows:fontengine=freetype";

    return {};
}

PuppetStartData ExternalDependencies::puppetStartData(const Model &model) const
{
    PuppetStartData data;
    auto buildSystem = ProjectExplorer::activeBuildSystemForActiveProject();
    if (!buildSystem)
        return data;
    auto [workingDirectory, puppetPath] = QmlPuppetPaths::qmlPuppetPaths(buildSystem->kit());

    data.puppetPath = puppetPath;
    data.workingDirectoryPath = workingDirectory;
    data.environment = PuppetEnvironmentBuilder::createEnvironment(buildSystem, designerSettings(), model, qmlPuppetPath());
    data.debugPuppet = designerSettings().debugPuppet.stringValue();
    data.freeTypeOption = createFreeTypeOption(buildSystem);
    data.forwardOutput = designerSettings().forwardPuppetOutput.stringValue();

    return data;
}

bool ExternalDependencies::instantQmlTextUpdate() const
{
    return false;
}

Utils::FilePath ExternalDependencies::qmlPuppetPath() const
{
    auto target = ProjectExplorer::ProjectManager::startupTarget();
    auto [workingDirectory, puppetPath] = QmlPuppetPaths::qmlPuppetPaths(target->kit());
    return puppetPath;
}

static QString qmlPath(ProjectExplorer::Target *target)
{
    auto kit = target->kit();

    if (!kit)
        return {};

    auto qtVersion = QtSupport::QtKitAspect::qtVersion(kit);
    if (!qtVersion)
        return {};

    return qtVersion->qmlPath().toUrlishString();
}

static std::tuple<ProjectExplorer::Project *, ProjectExplorer::Target *, QmlProjectManager::QmlBuildSystem *>
activeProjectEntries()
{
    auto project = ProjectExplorer::ProjectManager::startupProject();

    if (!project)
        return {};

    auto target = project->activeTarget();

    if (!target)
        return {};

    const auto qmlBuildSystem = qobject_cast<QmlProjectManager::QmlBuildSystem *>(
        target->buildSystem());

    if (qmlBuildSystem)
        return std::make_tuple(project, target, qmlBuildSystem);

    return {};
}

QStringList ExternalDependencies::modulePaths() const
{
    auto [project, target, qmlBuildSystem] = activeProjectEntries();

    if (project && target && qmlBuildSystem) {
        QStringList modulePaths;

        if (auto path = qmlPath(target); !path.isEmpty())
            modulePaths.push_back(path);

        modulePaths.append(qmlBuildSystem->absoluteImportPaths());
        return modulePaths;
    }

    return {};
}

QStringList ExternalDependencies::projectModulePaths() const
{
    auto [project, target, qmlBuildSystem] = activeProjectEntries();

    if (project && target && qmlBuildSystem) {
        return qmlBuildSystem->absoluteImportPaths();
    }

    return {};
}

bool ExternalDependencies::isQt6Project() const
{
    auto [project, target, qmlBuildSystem] = activeProjectEntries();

    return qmlBuildSystem && qmlBuildSystem->qt6Project();
}

bool ExternalDependencies::isQtForMcusProject() const
{
    // QmlBuildSystem
    auto [project, target, qmlBuildSystem] = activeProjectEntries();
    if (qmlBuildSystem)
        return  qmlBuildSystem->qtForMCUs();

    // CMakeBuildSystem
    ProjectExplorer::Target *activeTarget = ProjectExplorer::ProjectManager::startupTarget();
    return activeTarget && activeTarget->kit() && activeTarget->kit()->hasValue("McuSupport.McuTargetKitVersion");
}

QString ExternalDependencies::qtQuickVersion() const
{
    auto [project, target, qmlBuildSystem] = activeProjectEntries();

    return qmlBuildSystem ? qmlBuildSystem->versionQtQuick() : QString{};
}

Utils::FilePath ExternalDependencies::resourcePath(const QString &relativePath) const
{
    return Core::ICore::resourcePath(relativePath);
}

QString ExternalDependencies::userResourcePath(QStringView relativePath) const
{
    return Core::ICore::userResourcePath(relativePath.toString()).path();
}

QWidget *ExternalDependencies::mainWindow() const
{
    return Core::ICore::mainWindow();
}

} // namespace QmlDesigner
