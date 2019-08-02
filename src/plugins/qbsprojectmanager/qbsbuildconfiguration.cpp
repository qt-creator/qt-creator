/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qbsbuildconfiguration.h"

#include "qbsbuildstep.h"
#include "qbscleanstep.h"
#include "qbsinstallstep.h"
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"
#include "qbsprojectmanagersettings.h"

#include <coreplugin/documentmanager.h>

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmacroexpander.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QCoreApplication>

using namespace ProjectExplorer;
using namespace Utils;

namespace QbsProjectManager {
namespace Internal {

static FilePath defaultBuildDirectory(const FilePath &projectFilePath, const Kit *k,
                                      const QString &bcName,
                                      BuildConfiguration::BuildType buildType)
{
    const QString projectName = projectFilePath.toFileInfo().completeBaseName();
    ProjectMacroExpander expander(projectFilePath, projectName, k, bcName, buildType);
    QString projectDir = Project::projectDirectory(projectFilePath).toString();
    QString buildPath = expander.expand(ProjectExplorerPlugin::buildDirectoryTemplate());
    return FilePath::fromString(FileUtils::resolvePath(projectDir, buildPath));
}

// ---------------------------------------------------------------------------
// QbsBuildConfiguration:
// ---------------------------------------------------------------------------

QbsBuildConfiguration::QbsBuildConfiguration(Target *target, Core::Id id)
    : BuildConfiguration(target, id)
{
    setConfigWidgetHasFrame(true);

    m_configurationName = addAspect<BaseStringAspect>();
    m_configurationName->setLabelText(tr("Configuration name:"));
    m_configurationName->setSettingsKey("Qbs.configName");
    m_configurationName->setDisplayStyle(BaseStringAspect::LineEditDisplay);
    connect(m_configurationName, &BaseStringAspect::changed,
            this, &BuildConfiguration::buildDirectoryChanged);

    connect(this, &BuildConfiguration::environmentChanged,
            this, &QbsBuildConfiguration::triggerReparseIfActive);
    connect(this, &BuildConfiguration::buildDirectoryChanged,
            this, &QbsBuildConfiguration::triggerReparseIfActive);
    connect(this, &QbsBuildConfiguration::qbsConfigurationChanged,
            this, &QbsBuildConfiguration::triggerReparseIfActive);
}

void QbsBuildConfiguration::initialize()
{
    BuildConfiguration::initialize();

    QVariantMap configData = extraInfo().value<QVariantMap>();
    configData.insert(QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY),
                      (initialBuildType() == BuildConfiguration::Debug)
                      ? QLatin1String(Constants::QBS_VARIANT_DEBUG)
                      : QLatin1String(Constants::QBS_VARIANT_RELEASE));

    Utils::FilePath buildDir = initialBuildDirectory();
    if (buildDir.isEmpty())
        buildDir = defaultBuildDirectory(target()->project()->projectFilePath(),
                                         target()->kit(), initialDisplayName(),
                                         initialBuildType());
    setBuildDirectory(buildDir);

    // Add the build configuration.
    QVariantMap bd = configData;
    QString configName = bd.take("configName").toString();
    if (configName.isEmpty()) {
        configName = "qtc_" + target()->kit()->fileSystemFriendlyName() + '_'
                + Utils::FileUtils::fileSystemFriendlyName(initialDisplayName());
    }

    m_configurationName->setValue(configName);

    BuildStepList *buildSteps = stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    auto bs = new QbsBuildStep(buildSteps);
    if (initialBuildType() == Release)
        bs->setQmlDebuggingEnabled(false);
    bs->setQbsConfiguration(bd);
    buildSteps->appendStep(bs);

    BuildStepList *cleanSteps = stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
    cleanSteps->appendStep(Constants::QBS_CLEANSTEP_ID);

    emit qbsConfigurationChanged();
}

void QbsBuildConfiguration::triggerReparseIfActive()
{
    if (isActive())
        qbsProject()->delayParsing();
}

bool QbsBuildConfiguration::fromMap(const QVariantMap &map)
{
    if (!BuildConfiguration::fromMap(map))
        return false;

    if (m_configurationName->value().isEmpty()) { // pre-4.4 backwards compatibility
        const QString profileName = QbsManager::profileForKit(target()->kit());
        const QString buildVariant = qbsConfiguration()
                .value(QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY)).toString();
        m_configurationName->setValue(profileName + '-' + buildVariant);
    }

    return true;
}

QbsBuildStep *QbsBuildConfiguration::qbsStep() const
{
    return stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD)->firstOfType<QbsBuildStep>();
}

QVariantMap QbsBuildConfiguration::qbsConfiguration() const
{
    QVariantMap config;
    QbsBuildStep *qbsBs = qbsStep();
    if (qbsBs)
        config = qbsBs->qbsConfiguration(QbsBuildStep::ExpandVariables);
    return config;
}

Internal::QbsProject *QbsBuildConfiguration::qbsProject() const
{
    return qobject_cast<Internal::QbsProject *>(project());
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

QString QbsBuildConfiguration::configurationName() const
{
    return m_configurationName->value();
}

class StepProxy
{
public:
    StepProxy(const BuildStep *buildStep)
        : m_qbsBuildStep(qobject_cast<const QbsBuildStep *>(buildStep))
        , m_qbsCleanStep(qobject_cast<const QbsCleanStep *>(buildStep))
        , m_qbsInstallStep(qobject_cast<const QbsInstallStep *>(buildStep))
    {
    }

    QString command() const {
        if (m_qbsBuildStep)
            return QLatin1String("build");
        if (m_qbsInstallStep)
            return QLatin1String("install");
        return QLatin1String("clean");
    }

    bool dryRun() const {
        if (m_qbsBuildStep)
            return false;
        if (m_qbsInstallStep)
            return m_qbsInstallStep->dryRun();
        return m_qbsCleanStep->dryRun();
    }

    bool keepGoing() const {
        if (m_qbsBuildStep)
            return m_qbsBuildStep->keepGoing();
        if (m_qbsInstallStep)
            return m_qbsInstallStep->keepGoing();
        return m_qbsCleanStep->keepGoing();
    }

    bool forceProbeExecution() const { return m_qbsBuildStep && m_qbsBuildStep->forceProbes(); }

    bool showCommandLines() const {
        return m_qbsBuildStep ? m_qbsBuildStep->showCommandLines() : false;
    }

    bool noInstall() const {
        return m_qbsBuildStep ? !m_qbsBuildStep->install() : false;
    }

    bool noBuild() const { return m_qbsInstallStep; }

    bool cleanInstallRoot() const {
        if (m_qbsBuildStep)
            return m_qbsBuildStep->cleanInstallRoot();
        if (m_qbsInstallStep)
            return m_qbsInstallStep->removeFirst();
        return false;
    }

    int jobCount() const {
        return m_qbsBuildStep ? m_qbsBuildStep->maxJobs() : 0;
    }

    Utils::FilePath installRoot() const {
        const QbsBuildStep *bs = nullptr;
        if (m_qbsBuildStep) {
            bs = m_qbsBuildStep;
        } else if (m_qbsInstallStep) {
            bs = static_cast<QbsBuildConfiguration *>(m_qbsInstallStep->deployConfiguration()
                    ->target()->activeBuildConfiguration())->qbsStep();
        }
        if (bs && bs->hasCustomInstallRoot())
            return bs->installRoot();
        return Utils::FilePath();
    }

private:
    const QbsBuildStep * const m_qbsBuildStep;
    const QbsCleanStep * const m_qbsCleanStep;
    const QbsInstallStep * const m_qbsInstallStep;
};

QString QbsBuildConfiguration::equivalentCommandLine(const BuildStep *buildStep) const
{
    CommandLine commandLine;
    const QString qbsInstallDir = QString::fromLocal8Bit(qgetenv("QBS_INSTALL_DIR"));
    const QString qbsFilePath = HostOsInfo::withExecutableSuffix(!qbsInstallDir.isEmpty()
            ? qbsInstallDir + QLatin1String("/bin/qbs")
            : QCoreApplication::applicationDirPath() + QLatin1String("/qbs"));
    commandLine.addArg(QDir::toNativeSeparators(qbsFilePath));
    const StepProxy stepProxy(buildStep);
    commandLine.addArg(stepProxy.command());
    const QString buildDir = buildDirectory().toUserOutput();
    commandLine.addArgs({"-d", buildDir});
    commandLine.addArgs({"-f", buildStep->project()->projectFilePath().toUserOutput()});
    if (QbsProjectManagerSettings::useCreatorSettingsDirForQbs()) {
        commandLine.addArgs({"--settings-dir",
                QDir::toNativeSeparators(QbsProjectManagerSettings::qbsSettingsBaseDir())});
    }
    if (stepProxy.dryRun())
        commandLine.addArg("--dry-run");
    if (stepProxy.keepGoing())
        commandLine.addArg("--keep-going");
    if (stepProxy.forceProbeExecution())
        commandLine.addArg("--force-probe-execution");
    if (stepProxy.showCommandLines())
        commandLine.addArgs({"--command-echo-mode", "command-line"});
    if (stepProxy.noInstall())
        commandLine.addArg("--no-install");
    if (stepProxy.noBuild())
        commandLine.addArg("--no-build");
    if (stepProxy.cleanInstallRoot())
        commandLine.addArg("--clean-install-root");
    const int jobCount = stepProxy.jobCount();
    if (jobCount > 0)
        commandLine.addArgs({"--jobs", QString::number(jobCount)});

    const QString profileName = QbsManager::profileForKit(buildStep->target()->kit());
    const QString buildVariant = qbsConfiguration()
            .value(QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY)).toString();
    commandLine.addArg("config:" + configurationName());
    commandLine.addArg(QString(Constants::QBS_CONFIG_VARIANT_KEY) + ':' + buildVariant);
    const FilePath installRoot = stepProxy.installRoot();
    if (!installRoot.isEmpty()) {
        commandLine.addArg(QString(Constants::QBS_INSTALL_ROOT_KEY) + ':' + installRoot.toUserOutput());
        if (qobject_cast<const QbsInstallStep *>(buildStep))
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
    setIssueReporter([](Kit *k, const QString &projectPath, const QString &buildDir) -> Tasks {
        const QtSupport::BaseQtVersion * const version = QtSupport::QtKitAspect::qtVersion(k);
        return version ? version->reportIssues(projectPath, buildDir)
                       : Tasks();
    });
}

QList<BuildInfo> QbsBuildConfigurationFactory::availableBuilds(const Kit *k, const FilePath &projectPath, bool forSetup) const
{
    QList<BuildInfo> result;

    if (forSetup) {

        BuildInfo info = createBuildInfo(k, BuildConfiguration::Debug);
        //: The name of the debug build configuration created by default for a qbs project.
        info.displayName = tr("Debug");
        //: Non-ASCII characters in directory suffix may cause build issues.
        info.buildDirectory
                = defaultBuildDirectory(projectPath, k, tr("Debug", "Shadow build directory suffix"),
                                        info.buildType);
        result << info;

        info = createBuildInfo(k, BuildConfiguration::Release);
        //: The name of the release build configuration created by default for a qbs project.
        info.displayName = tr("Release");
        //: Non-ASCII characters in directory suffix may cause build issues.
        info.buildDirectory
                = defaultBuildDirectory(projectPath, k, tr("Release", "Shadow build directory suffix"),
                                        info.buildType);
        result << info;

    } else {

        result << createBuildInfo(k, BuildConfiguration::Debug);

    }

    return result;
}

BuildInfo QbsBuildConfigurationFactory::createBuildInfo(const Kit *k,
                                                        BuildConfiguration::BuildType type) const
{
    BuildInfo info(this);
    info.kitId = k->id();
    info.buildType = type;
    info.typeName = tr("Build");
    QVariantMap config;
    config.insert("configName", type == BuildConfiguration::Debug ? "Debug" : "Release");
    info.extraInfo = config;
    return info;
}

} // namespace Internal
} // namespace QbsProjectManager
