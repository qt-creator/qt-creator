// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qbsbuildconfiguration.h"

#include "qbsbuildstep.h"
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"
#include "qbsprojectmanagertr.h"
#include "qbssettings.h"

#include <coreplugin/documentmanager.h>

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/environmentkitaspect.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorertr.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <qtsupport/qtkitaspect.h>

#include <utils/mimeconstants.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QCryptographicHash>

#include <tuple>

using namespace ProjectExplorer;
using namespace Utils;

namespace QbsProjectManager {
namespace Internal {

static FilePath defaultBuildDirectory(const FilePath &projectFilePath, const Kit *k,
                                      const QString &bcName,
                                      BuildConfiguration::BuildType buildType)
{
    const QString projectName = projectFilePath.completeBaseName();
    return BuildConfiguration::buildDirectoryFromTemplate(
        projectFilePath.absolutePath(), projectFilePath, projectName, k, bcName, buildType, "qbs");
}

// ---------------------------------------------------------------------------
// QbsBuildConfiguration:
// ---------------------------------------------------------------------------

QbsBuildConfiguration::QbsBuildConfiguration(Target *target, Utils::Id id)
    : BuildConfiguration(target, id)
{
    setConfigWidgetHasFrame(true);

    appendInitialBuildStep(Constants::QBS_BUILDSTEP_ID);
    appendInitialCleanStep(Constants::QBS_CLEANSTEP_ID);

    setInitializer([this](const BuildInfo &info) {
        Store configData = storeFromVariant(info.extraInfo);
        const QString buildVariant = [](BuildConfiguration::BuildType buildType) -> QString {
            switch (buildType) {
            case BuildConfiguration::Release: return Constants::QBS_VARIANT_RELEASE;
            case BuildConfiguration::Profile: return Constants::QBS_VARIANT_PROFILING;
            case BuildConfiguration::Debug:
            case BuildConfiguration::Unknown:
                break;
            }
            return Constants::QBS_VARIANT_DEBUG;
        }(info.buildType);
        configData.insert(Constants::QBS_CONFIG_VARIANT_KEY, buildVariant);
        FilePath buildDir = info.buildDirectory;
        if (buildDir.isEmpty())
            buildDir = defaultBuildDirectory(project()->projectFilePath(),
                                             kit(), info.displayName,
                                             buildType());
        setBuildDirectory(buildDir);

        // Add the build configuration.
        Store bd = configData;
        QString configName = bd.take("configName").toString();
        if (configName.isEmpty()) {
            configName = "qtc_" + kit()->fileSystemFriendlyName() + '_'
                            + FileUtils::fileSystemFriendlyName(info.displayName);
        }

        const QString kitName = kit()->displayName();
        const QByteArray hash = QCryptographicHash::hash((kitName + info.displayName).toUtf8(),
                                                         QCryptographicHash::Sha1);

        const QString uniqueConfigName = configName
                        + '_' + kit()->fileSystemFriendlyName().left(8)
                        + '_' + hash.toHex().left(16);

        configurationName.setValue(uniqueConfigName);

        auto bs = buildSteps()->firstOfType<QbsBuildStep>();
        QTC_ASSERT(bs, return);
        bs->setQbsConfiguration(bd);

        emit qbsConfigurationChanged();
    });

    configurationName.setSettingsKey("Qbs.configName");
    configurationName.setLabelText(QbsProjectManager::Tr::tr("Configuration name:"));
    configurationName.setDisplayStyle(StringAspect::LineEditDisplay);
    connect(&configurationName, &StringAspect::changed,
            this, &BuildConfiguration::buildDirectoryChanged);

    connect(&separateDebugInfoSetting, &BaseAspect::changed,
            this, &QbsBuildConfiguration::qbsConfigurationChanged);

    qmlDebuggingSetting.setBuildConfiguration(this);
    connect(&qmlDebuggingSetting, &BaseAspect::changed,
            this, &QbsBuildConfiguration::qbsConfigurationChanged);

    qtQuickCompilerSetting.setBuildConfiguration(this);
    connect(&qtQuickCompilerSetting, &BaseAspect::changed,
            this, &QbsBuildConfiguration::qbsConfigurationChanged);

    connect(this, &BuildConfiguration::environmentChanged,
            this, &QbsBuildConfiguration::triggerReparseIfActive);
    connect(this, &BuildConfiguration::buildDirectoryChanged,
            this, &QbsBuildConfiguration::triggerReparseIfActive);
    connect(this, &QbsBuildConfiguration::qbsConfigurationChanged,
            this, &QbsBuildConfiguration::triggerReparseIfActive);

    macroExpander()->registerVariable("CurrentBuild:QbsBuildRoot",
                                      QbsProjectManager::Tr::tr("The qbs project build root"),
        [this] { return buildDirectory().pathAppended(configurationName()).toUserOutput(); });
}

void QbsBuildConfiguration::triggerReparseIfActive()
{
    if (isActive())
        qobject_cast<QbsBuildSystem *>(buildSystem())->delayParsing();
}

void QbsBuildConfiguration::fromMap(const Store &map)
{
    BuildConfiguration::fromMap(map);
    if (hasError())
        return;

    if (configurationName().isEmpty()) { // pre-4.4 backwards compatibility
        const QString profileName = QbsProfileManager::profileNameForKit(kit());
        const QString buildVariant = qbsConfiguration()
                .value(Constants::QBS_CONFIG_VARIANT_KEY).toString();
        configurationName.setValue(profileName + '-' + buildVariant);
    }
}

void QbsBuildConfiguration::restrictNextBuild(const RunConfiguration *rc)
{
    if (!rc) {
        setProducts({});
        return;
    }
    const auto productNode = dynamic_cast<QbsProductNode *>(rc->productNode());
    QTC_ASSERT(productNode, return);
    setProducts({productNode->fullDisplayName()});
}

QbsBuildStep *QbsBuildConfiguration::qbsStep() const
{
    return buildSteps()->firstOfType<QbsBuildStep>();
}

Store QbsBuildConfiguration::qbsConfiguration() const
{
    Store config;
    QbsBuildStep *qbsBs = qbsStep();
    if (qbsBs)
        config = qbsBs->qbsConfiguration(QbsBuildStep::ExpandVariables);
    return config;
}

BuildConfiguration::BuildType QbsBuildConfiguration::buildType() const
{
    QString variant;
    if (qbsStep())
        variant = qbsStep()->buildVariant();

    if (variant == QLatin1String(Constants::QBS_VARIANT_DEBUG))
        return Debug;
    if (variant == QLatin1String(Constants::QBS_VARIANT_RELEASE))
        return Release;
    if (variant == QLatin1String(Constants::QBS_VARIANT_PROFILING))
        return Profile;
    return Unknown;
}

void QbsBuildConfiguration::setChangedFiles(const QStringList &files)
{
    m_changedFiles = files;
}

QStringList QbsBuildConfiguration::changedFiles() const
{
    return m_changedFiles;
}

void QbsBuildConfiguration::setActiveFileTags(const QStringList &fileTags)
{
    m_activeFileTags = fileTags;
}

QStringList QbsBuildConfiguration::activeFileTags() const
{
    return m_activeFileTags;
}

void QbsBuildConfiguration::setProducts(const QStringList &products)
{
    m_products = products;
}

QStringList QbsBuildConfiguration::products() const
{
    return m_products;
}

QString QbsBuildConfiguration::equivalentCommandLine(const QbsBuildStepData &stepData) const
{
    const IDeviceConstPtr dev = BuildDeviceKitAspect::device(kit());
    if (!dev)
        return Tr::tr("<No build device>");
    CommandLine commandLine;
    commandLine.addArg(QbsSettings::qbsExecutableFilePath(dev).nativePath());
    commandLine.addArg(stepData.command);
    const QString buildDir = buildDirectory().nativePath();
    commandLine.addArgs({"-d", buildDir});
    commandLine.addArgs({"-f", project()->projectFilePath().nativePath()});
    if (QbsSettings::useCreatorSettingsDirForQbs(dev))
        commandLine.addArgs({"--settings-dir", QbsSettings::qbsSettingsBaseDir(dev).nativePath()});
    if (stepData.dryRun)
        commandLine.addArg("--dry-run");
    if (stepData.keepGoing)
        commandLine.addArg("--keep-going");
    if (stepData.forceProbeExecution)
        commandLine.addArg("--force-probe-execution");
    if (stepData.showCommandLines)
        commandLine.addArgs({"--command-echo-mode", "command-line"});
    if (stepData.noInstall)
        commandLine.addArg("--no-install");
    if (stepData.noBuild)
        commandLine.addArg("--no-build");
    if (stepData.cleanInstallRoot)
        commandLine.addArg("--clean-install-root");
    const int jobCount = stepData.jobCount;
    if (jobCount > 0)
        commandLine.addArgs({"--jobs", QString::number(jobCount)});

    const QString profileName = QbsProfileManager::profileNameForKit(kit());
    const QString buildVariant = qbsConfiguration()
            .value(Constants::QBS_CONFIG_VARIANT_KEY).toString();
    commandLine.addArg("config:" + configurationName());
    commandLine.addArg(QString(Constants::QBS_CONFIG_VARIANT_KEY) + ':' + buildVariant);
    const FilePath installRoot = stepData.installRoot;
    if (!installRoot.isEmpty()) {
        commandLine.addArg(QString(Constants::QBS_INSTALL_ROOT_KEY) + ':' + installRoot.nativePath());
        if (stepData.isInstallStep)
            commandLine.addArgs({"--installRoot", installRoot.nativePath()});
    }
    commandLine.addArg("profile:" + profileName);

    return commandLine.arguments();
}

// ---------------------------------------------------------------------------
// QbsBuildConfigurationFactory:
// ---------------------------------------------------------------------------

QbsBuildConfigurationFactory::QbsBuildConfigurationFactory()
{
    registerBuildConfiguration<QbsBuildConfiguration>(Constants::QBS_BC_ID);
    setSupportedProjectType(Constants::PROJECT_ID);
    setSupportedProjectMimeTypeName(Utils::Constants::QBS_MIMETYPE);
    setIssueReporter([](Kit *k, const FilePath &projectPath, const FilePath &buildDir) -> Tasks {
        const QtSupport::QtVersion * const version = QtSupport::QtKitAspect::qtVersion(k);
        return version ? version->reportIssues(projectPath, buildDir) : Tasks();
    });

    setBuildGenerator([](const Kit *k, const FilePath &projectPath, bool forSetup) {
        QList<BuildInfo> result;
        for (const auto &[type, name, configName] :
             {std::make_tuple(BuildConfiguration::Debug, ProjectExplorer::Tr::tr("Debug"), "Debug"),
              std::make_tuple(
                  BuildConfiguration::Release, ProjectExplorer::Tr::tr("Release"), "Release"),
              std::make_tuple(
                  BuildConfiguration::Profile, ProjectExplorer::Tr::tr("Profile"), "Profile")}) {
            BuildInfo info;
            info.buildType = type;
            info.typeName = name;
            if (forSetup) {
                info.displayName = name;
                info.buildDirectory = defaultBuildDirectory(projectPath, k, name, type);
            }
            info.enabledByDefault = type == BuildConfiguration::Debug;
            QVariantMap config;
            config.insert("configName", configName);
            info.extraInfo = config;
            result << info;
        }
        return result;
    });
}

} // namespace Internal
} // namespace QbsProjectManager
