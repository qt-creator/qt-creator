// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmldesignerexternaldependencies.h"

#include "qmldesignerplugin.h"

#include <app/app_version.h>
#include <edit3d/edit3dviewconfig.h>
#include <itemlibraryimport.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <puppetenvironmentbuilder.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>

#include <coreplugin/icore.h>

#include <utils/hostosinfo.h>

namespace QmlDesigner {

double ExternalDependencies::formEditorDevicePixelRatio() const
{
    return QmlDesignerPlugin::formEditorDevicePixelRatio();
}

QString ExternalDependencies::defaultPuppetFallbackDirectory() const
{
    return Core::ICore::libexecPath().toString();
}

QString ExternalDependencies::qmlPuppetFallbackDirectory() const
{
    QString puppetFallbackDirectory = m_designerSettings
                                          .value(DesignerSettingsKey::PUPPET_DEFAULT_DIRECTORY)
                                          .toString();
    if (puppetFallbackDirectory.isEmpty() || !QFileInfo::exists(puppetFallbackDirectory))
        return defaultPuppetFallbackDirectory();
    return puppetFallbackDirectory;
}

QString ExternalDependencies::defaultPuppetToplevelBuildDirectory() const
{
    return Core::ICore::userResourcePath("qmlpuppet/").toString();
}

QUrl ExternalDependencies::projectUrl() const
{
    DesignDocument *document = QmlDesignerPlugin::instance()->viewManager().currentDesignDocument();
    if (document)
        return QUrl::fromLocalFile(document->projectFolder().toString());

    return {};
}

QString ExternalDependencies::currentProjectDirPath() const
{
    return QmlDesignerPlugin::instance()->documentManager().currentProjectDirPath().toString();
}

QList<QColor> ExternalDependencies::designerSettingsEdit3DViewBackgroundColor() const
{
    return Edit3DViewConfig::loadColor(DesignerSettingsKey::EDIT3DVIEW_BACKGROUND_COLOR);
}

QColor ExternalDependencies::designerSettingsEdit3DViewGridColor() const
{
    QList<QColor> gridColorList = Edit3DViewConfig::loadColor(DesignerSettingsKey::EDIT3DVIEW_GRID_COLOR);
    if (!gridColorList.isEmpty())
        return gridColorList.front();

    return {};
}

QUrl ExternalDependencies::currentResourcePath() const
{
    return QUrl::fromLocalFile(
        QmlDesigner::DocumentManager::currentResourcePath().toFileInfo().absoluteFilePath());
}

void ExternalDependencies::parseItemLibraryDescriptions() {}

const DesignerSettings &ExternalDependencies::designerSettings() const
{
    return m_designerSettings;
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
    auto target = ProjectExplorer::SessionManager::startupTarget();
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
    auto target = ProjectExplorer::SessionManager::startupTarget();
    if (target) {
        QtSupport::QtVersion *currentQtVersion = QtSupport::QtKitAspect::qtVersion(target->kit());
        if (currentQtVersion && currentQtVersion->isValid()) {
            return true;
        }
    }

    return false;
}

namespace {

Utils::FilePath qmlPuppetExecutablePath(const Utils::FilePath &workingDirectory)
{
    return workingDirectory.pathAppended(QString{"qml2puppet-"} + Core::Constants::IDE_VERSION_LONG)
        .withExecutableSuffix();
}

Utils::FilePath qmlPuppetFallbackDirectory(const DesignerSettings &settings)
{
    auto puppetFallbackDirectory = Utils::FilePath::fromString(
        settings.value(DesignerSettingsKey::PUPPET_DEFAULT_DIRECTORY).toString());
    if (puppetFallbackDirectory.isEmpty() || !puppetFallbackDirectory.exists())
        return Core::ICore::libexecPath();
    return puppetFallbackDirectory;
}

std::pair<Utils::FilePath, Utils::FilePath> qmlPuppetFallbackPaths(const DesignerSettings &settings)
{
    auto workingDirectory = qmlPuppetFallbackDirectory(settings);

    return {workingDirectory, qmlPuppetExecutablePath(workingDirectory)};
}

std::pair<Utils::FilePath, Utils::FilePath> pathsForKitPuppet(ProjectExplorer::Target *target)
{
    if (!target || !target->kit())
        return {};

    QtSupport::QtVersion *currentQtVersion = QtSupport::QtKitAspect::qtVersion(target->kit());

    if (currentQtVersion) {
        auto path = currentQtVersion->binPath();
        return {path, qmlPuppetExecutablePath(path)};
    }

    return {};
}

std::pair<Utils::FilePath, Utils::FilePath> qmlPuppetPaths(ProjectExplorer::Target *target,
                                                           const DesignerSettings &settings)
{
    auto [workingDirectoryPath, puppetPath] = pathsForKitPuppet(target);

    if (workingDirectoryPath.isEmpty() || !puppetPath.exists())
        return qmlPuppetFallbackPaths(settings);

    return {workingDirectoryPath, puppetPath};
}

bool isForcingFreeType(ProjectExplorer::Target *target)
{
    if (Utils::HostOsInfo::isWindowsHost() && target) {
        const QVariant customData = target->additionalData("CustomForceFreeType");

        if (customData.isValid())
            return customData.toBool();
    }

    return false;
}

QString createFreeTypeOption(ProjectExplorer::Target *target)
{
    if (isForcingFreeType(target))
        return "-platform windows:fontengine=freetype";

    return {};
}

} // namespace

PuppetStartData ExternalDependencies::puppetStartData(const Model &model) const
{
    PuppetStartData data;
    auto target = ProjectExplorer::SessionManager::startupTarget();
    auto [workingDirectory, puppetPath] = qmlPuppetPaths(target, m_designerSettings);

    data.puppetPath = puppetPath.toString();
    data.workingDirectoryPath = workingDirectory.toString();
    data.environment = PuppetEnvironmentBuilder::createEnvironment(target, m_designerSettings, model);
    data.debugPuppet = m_designerSettings.value(DesignerSettingsKey::DEBUG_PUPPET).toString();
    data.freeTypeOption = createFreeTypeOption(target);
    data.forwardOutput = m_designerSettings.value(DesignerSettingsKey::FORWARD_PUPPET_OUTPUT).toString();

    return data;
}

bool ExternalDependencies::instantQmlTextUpdate() const
{
    return false;
}

} // namespace QmlDesigner
