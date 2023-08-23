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
#include <projectexplorer/kit.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorertr.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <qtsupport/qtkitaspect.h>

#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QCryptographicHash>

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
                Project::projectDirectory(projectFilePath),
                projectFilePath, projectName, k, bcName, buildType, "qbs");
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

    setInitializer([this, target](const BuildInfo &info) {
        const Kit *kit = target->kit();
        QVariantMap configData = info.extraInfo.value<QVariantMap>();
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
        configData.insert(QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY), buildVariant);
        FilePath buildDir = info.buildDirectory;
        if (buildDir.isEmpty())
            buildDir = defaultBuildDirectory(target->project()->projectFilePath(),
                                             kit, info.displayName,
                                             buildType());
        setBuildDirectory(buildDir);

        // Add the build configuration.
        QVariantMap bd = configData;
        QString configName = bd.take("configName").toString();
        if (configName.isEmpty()) {
            configName = "qtc_" + kit->fileSystemFriendlyName() + '_'
                            + FileUtils::fileSystemFriendlyName(info.displayName);
        }

        const QString kitName = kit->displayName();
        const QByteArray hash = QCryptographicHash::hash((kitName + info.displayName).toUtf8(),
                                                         QCryptographicHash::Sha1);

        const QString uniqueConfigName = configName
                        + '_' + kit->fileSystemFriendlyName().left(8)
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

    m_buildSystem = new QbsBuildSystem(this);
}

QbsBuildConfiguration::~QbsBuildConfiguration()
{
    delete m_buildSystem;
}

BuildSystem *QbsBuildConfiguration::buildSystem() const
{
    return m_buildSystem;
}

void QbsBuildConfiguration::triggerReparseIfActive()
{
    if (isActive())
        m_buildSystem->delayParsing();
}

void QbsBuildConfiguration::fromMap(const Store &map)
{
    BuildConfiguration::fromMap(map);
    if (hasError())
        return;

    if (configurationName().isEmpty()) { // pre-4.4 backwards compatibility
        const QString profileName = QbsProfileManager::profileNameForKit(target()->kit());
        const QString buildVariant = qbsConfiguration()
                .value(QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY)).toString();
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

QVariantMap QbsBuildConfiguration::qbsConfiguration() const
{
    QVariantMap config;
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
    CommandLine commandLine;
    commandLine.addArg(QDir::toNativeSeparators(QbsSettings::qbsExecutableFilePath().toString()));
    commandLine.addArg(stepData.command);
    const QString buildDir = buildDirectory().toUserOutput();
    commandLine.addArgs({"-d", buildDir});
    commandLine.addArgs({"-f", project()->projectFilePath().toUserOutput()});
    if (QbsSettings::useCreatorSettingsDirForQbs()) {
        commandLine.addArgs({"--settings-dir",
                             QDir::toNativeSeparators(QbsSettings::qbsSettingsBaseDir())});
    }
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

    const QString profileName = QbsProfileManager::profileNameForKit(target()->kit());
    const QString buildVariant = qbsConfiguration()
            .value(QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY)).toString();
    commandLine.addArg("config:" + configurationName());
    commandLine.addArg(QString(Constants::QBS_CONFIG_VARIANT_KEY) + ':' + buildVariant);
    const FilePath installRoot = stepData.installRoot;
    if (!installRoot.isEmpty()) {
        commandLine.addArg(QString(Constants::QBS_INSTALL_ROOT_KEY) + ':' + installRoot.toUserOutput());
        if (stepData.isInstallStep)
            commandLine.addArgs({"--installRoot", installRoot.toUserOutput()});
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
    setSupportedProjectMimeTypeName(Constants::MIME_TYPE);
    setIssueReporter([](Kit *k, const FilePath &projectPath, const FilePath &buildDir) -> Tasks {
        const QtSupport::QtVersion * const version = QtSupport::QtKitAspect::qtVersion(k);
        return version ? version->reportIssues(projectPath, buildDir) : Tasks();
    });

    setBuildGenerator([this](const Kit *k, const FilePath &projectPath, bool forSetup) {
        QList<BuildInfo> result;

        if (forSetup) {
            BuildInfo info = createBuildInfo(BuildConfiguration::Debug);
            info.displayName = ProjectExplorer::Tr::tr("Debug");
            //: Non-ASCII characters in directory suffix may cause build issues.
            const QString dbg = QbsProjectManager::Tr::tr("Debug", "Shadow build directory suffix");
            info.buildDirectory = defaultBuildDirectory(projectPath, k, dbg, info.buildType);
            result << info;

            info = createBuildInfo(BuildConfiguration::Release);
            info.displayName = ProjectExplorer::Tr::tr("Release");
            //: Non-ASCII characters in directory suffix may cause build issues.
            const QString rel = QbsProjectManager::Tr::tr("Release", "Shadow build directory suffix");
            info.buildDirectory = defaultBuildDirectory(projectPath, k, rel, info.buildType);
            result << info;

            info = createBuildInfo(BuildConfiguration::Profile);
            info.displayName = ProjectExplorer::Tr::tr("Profile");
            //: Non-ASCII characters in directory suffix may cause build issues.
            const QString prof = QbsProjectManager::Tr::tr("Profile", "Shadow build directory suffix");
            info.buildDirectory = defaultBuildDirectory(projectPath, k, prof, info.buildType);
            result << info;
        } else {
            result << createBuildInfo(BuildConfiguration::Debug);
            result << createBuildInfo(BuildConfiguration::Release);
            result << createBuildInfo(BuildConfiguration::Profile);
        }

        return result;
    });
}

BuildInfo QbsBuildConfigurationFactory::createBuildInfo(BuildConfiguration::BuildType type) const
{
    BuildInfo info;
    info.buildType = type;
    info.typeName = type == BuildConfiguration::Profile
            ? ProjectExplorer::Tr::tr("Profiling") : type == BuildConfiguration::Release
            ? ProjectExplorer::Tr::tr("Release") : ProjectExplorer::Tr::tr("Debug");
    QVariantMap config;
    config.insert("configName", type == BuildConfiguration::Release
                  ? "Release" : type == BuildConfiguration::Profile
                  ? "Profile" : "Debug");
    info.extraInfo = config;
    return info;
}

} // namespace Internal
} // namespace QbsProjectManager
