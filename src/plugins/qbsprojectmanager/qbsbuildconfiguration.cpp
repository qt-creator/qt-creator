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

#include "qbsbuildconfigurationwidget.h"
#include "qbsbuildinfo.h"
#include "qbsbuildstep.h"
#include "qbscleanstep.h"
#include "qbsinstallstep.h"
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"
#include "qbsprojectmanagersettings.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <utils/qtcassert.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmacroexpander.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcprocess.h>

#include <QCoreApplication>
#include <QInputDialog>

using namespace ProjectExplorer;
using namespace Utils;

namespace QbsProjectManager {
namespace Internal {

static QString configNameKey() { return QStringLiteral("Qbs.configName"); }

static FileName defaultBuildDirectory(const QString &projectFilePath, const Kit *k,
                                      const QString &bcName,
                                      BuildConfiguration::BuildType buildType)
{
    const QString projectName = QFileInfo(projectFilePath).completeBaseName();
    ProjectMacroExpander expander(projectFilePath, projectName, k, bcName, buildType);
    QString projectDir = Project::projectDirectory(FileName::fromString(projectFilePath)).toString();
    QString buildPath = expander.expand(Core::DocumentManager::buildDirectory());
    return FileName::fromString(FileUtils::resolvePath(projectDir, buildPath));
}

// ---------------------------------------------------------------------------
// QbsBuildConfiguration:
// ---------------------------------------------------------------------------

QbsBuildConfiguration::QbsBuildConfiguration(Target *target, Core::Id id)
    : BuildConfiguration(target, id)
{
    connect(project(), &Project::parsingStarted, this, &BuildConfiguration::enabledChanged);
    connect(project(), &Project::parsingFinished, this, &BuildConfiguration::enabledChanged);
}

void QbsBuildConfiguration::initialize(const BuildInfo *info)
{
    BuildConfiguration::initialize(info);

    const auto * const bi = static_cast<const QbsBuildInfo *>(info);
    QVariantMap configData = bi->config;
    configData.insert(QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY),
                      (info->buildType == BuildConfiguration::Debug)
                      ? QLatin1String(Constants::QBS_VARIANT_DEBUG)
                      : QLatin1String(Constants::QBS_VARIANT_RELEASE));

    Utils::FileName buildDir = info->buildDirectory;
    if (buildDir.isEmpty())
        buildDir = defaultBuildDirectory(target()->project()->projectFilePath().toString(),
                                         target()->kit(), info->displayName, info->buildType);
    setBuildDirectory(buildDir);

    // Add the build configuration.
    QVariantMap bd = configData;
    QString configName = bd.take("configName").toString();
    if (configName.isEmpty()) {
        configName = "qtc_" + target()->kit()->fileSystemFriendlyName() + '_'
                + Utils::FileUtils::fileSystemFriendlyName(info->displayName);
    }
    setConfigurationName(configName);

    BuildStepList *buildSteps = stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    auto bs = new QbsBuildStep(buildSteps);
    if (info->buildType == Release)
        bs->setQmlDebuggingEnabled(false);
    bs->setQbsConfiguration(bd);
    buildSteps->appendStep(bs);

    BuildStepList *cleanSteps = stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
    cleanSteps->appendStep(new QbsCleanStep(cleanSteps));

    connect(bs, &QbsBuildStep::qbsConfigurationChanged, this, &QbsBuildConfiguration::qbsConfigurationChanged);
    emit qbsConfigurationChanged();
}

bool QbsBuildConfiguration::fromMap(const QVariantMap &map)
{
    if (!BuildConfiguration::fromMap(map))
        return false;

    m_configurationName = map.value(configNameKey()).toString();
    if (m_configurationName.isEmpty()) { // pre-4.4 backwards compatibility
        const QString profileName = QbsManager::profileForKit(target()->kit());
        const QString buildVariant = qbsConfiguration()
                .value(QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY)).toString();
        m_configurationName = profileName + QLatin1Char('-') + buildVariant;
    }
    BuildStepList *bsl = stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    // Fix up the existing build steps:
    for (int i = 0; i < bsl->count(); ++i) {
        auto bs = qobject_cast<QbsBuildStep *>(bsl->at(i));
        if (bs)
            connect(bs, &QbsBuildStep::qbsConfigurationChanged, this, &QbsBuildConfiguration::qbsConfigurationChanged);
    }

    return true;
}

QVariantMap QbsBuildConfiguration::toMap() const
{
    QVariantMap map = BuildConfiguration::toMap();
    map.insert(configNameKey(), m_configurationName);
    return map;
}

NamedWidget *QbsBuildConfiguration::createConfigWidget()
{
    return new QbsBuildConfigurationWidget(this);
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

Internal::QbsProject *QbsBuildConfiguration::project() const
{
    return qobject_cast<Internal::QbsProject *>(BuildConfiguration::project());
}

IOutputParser *QbsBuildConfiguration::createOutputParser() const
{
    ToolChain *tc = ToolChainKitInformation::toolChain(target()->kit(), ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    return tc ? tc->outputParser() : nullptr;
}

bool QbsBuildConfiguration::isEnabled() const
{
    return !project()->isParsing() && project()->hasParseResult();
}

QString QbsBuildConfiguration::disabledReason() const
{
    if (project()->isParsing())
        return tr("Parsing the Qbs project.");
    if (!project()->hasParseResult())
        return tr("Parsing of Qbs project has failed.");
    return QString();
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

void QbsBuildConfiguration::emitBuildTypeChanged()
{
    emit buildTypeChanged();
}

void QbsBuildConfiguration::setConfigurationName(const QString &configName)
{
    if (m_configurationName == configName)
        return;
    m_configurationName = configName;
    emit buildDirectoryChanged();
}

QString QbsBuildConfiguration::configurationName() const
{
    return m_configurationName;
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

    Utils::FileName installRoot() const {
        const QbsBuildStep *bs = nullptr;
        if (m_qbsBuildStep) {
            bs = m_qbsBuildStep;
        } else if (m_qbsInstallStep) {
            bs = static_cast<QbsBuildConfiguration *>(m_qbsInstallStep->deployConfiguration()
                    ->target()->activeBuildConfiguration())->qbsStep();
        }
        if (bs && bs->hasCustomInstallRoot())
            return bs->installRoot();
        return Utils::FileName();
    }

private:
    const QbsBuildStep * const m_qbsBuildStep;
    const QbsCleanStep * const m_qbsCleanStep;
    const QbsInstallStep * const m_qbsInstallStep;
};

QString QbsBuildConfiguration::equivalentCommandLine(const BuildStep *buildStep) const
{
    QString commandLine;
    const QString qbsInstallDir = QString::fromLocal8Bit(qgetenv("QBS_INSTALL_DIR"));
    const QString qbsFilePath = Utils::HostOsInfo::withExecutableSuffix(!qbsInstallDir.isEmpty()
            ? qbsInstallDir + QLatin1String("/bin/qbs")
            : QCoreApplication::applicationDirPath() + QLatin1String("/qbs"));
    Utils::QtcProcess::addArg(&commandLine, QDir::toNativeSeparators(qbsFilePath));
    const StepProxy stepProxy(buildStep);
    Utils::QtcProcess::addArg(&commandLine, stepProxy.command());
    const QString buildDir = buildDirectory().toUserOutput();
    Utils::QtcProcess::addArgs(&commandLine, QStringList({"-d", buildDir}));
    Utils::QtcProcess::addArgs(&commandLine, QStringList("-f")
                               << buildStep->project()->projectFilePath().toUserOutput());
    if (QbsProjectManagerSettings::useCreatorSettingsDirForQbs()) {
        Utils::QtcProcess::addArgs(&commandLine, QStringList({"--settings-dir",
                QDir::toNativeSeparators(QbsProjectManagerSettings::qbsSettingsBaseDir())}));
    }
    if (stepProxy.dryRun())
        Utils::QtcProcess::addArg(&commandLine, QLatin1String("--dry-run"));
    if (stepProxy.keepGoing())
        Utils::QtcProcess::addArg(&commandLine, QLatin1String("--keep-going"));
    if (stepProxy.forceProbeExecution())
        Utils::QtcProcess::addArg(&commandLine, QLatin1String("--force-probe-execution"));
    if (stepProxy.showCommandLines())
        Utils::QtcProcess::addArgs(&commandLine, QStringList({"--command-echo-mode",
                                                              "command-line"}));
    if (stepProxy.noInstall())
        Utils::QtcProcess::addArg(&commandLine, QLatin1String("--no-install"));
    if (stepProxy.noBuild())
        Utils::QtcProcess::addArg(&commandLine, QLatin1String("--no-build"));
    if (stepProxy.cleanInstallRoot())
        Utils::QtcProcess::addArg(&commandLine, QLatin1String("--clean-install-root"));
    const int jobCount = stepProxy.jobCount();
    if (jobCount > 0) {
        Utils::QtcProcess::addArgs(&commandLine, QStringList({"--jobs",
                                                              QString::number(jobCount)}));
    }
    const QString profileName = QbsManager::profileForKit(buildStep->target()->kit());
    const QString buildVariant = qbsConfiguration()
            .value(QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY)).toString();
    Utils::QtcProcess::addArg(&commandLine, QLatin1String("config:") + configurationName());
    Utils::QtcProcess::addArg(&commandLine, QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY)
                                  + QLatin1Char(':') + buildVariant);
    const Utils::FileName installRoot = stepProxy.installRoot();
    if (!installRoot.isEmpty()) {
        Utils::QtcProcess::addArg(&commandLine, QLatin1String(Constants::QBS_INSTALL_ROOT_KEY)
                                  + QLatin1Char(':') + installRoot.toUserOutput());
        if (qobject_cast<const QbsInstallStep *>(buildStep)) {
            Utils::QtcProcess::addArgs(&commandLine, QStringList({ QLatin1String("--installRoot"),
                                                                   installRoot.toUserOutput() } ));
        }
    }
    Utils::QtcProcess::addArg(&commandLine, QLatin1String("profile:") + profileName);

    return commandLine;
}

// ---------------------------------------------------------------------------
// QbsBuildConfigurationFactory:
// ---------------------------------------------------------------------------

QbsBuildConfigurationFactory::QbsBuildConfigurationFactory()
{
    registerBuildConfiguration<QbsBuildConfiguration>(Constants::QBS_BC_ID);
    setSupportedProjectType(Constants::PROJECT_ID);
    setSupportedProjectMimeTypeName(Constants::MIME_TYPE);
}

BuildInfo *QbsBuildConfigurationFactory::createBuildInfo(const Kit *k,
                                                         BuildConfiguration::BuildType type) const
{
    auto info = new QbsBuildInfo(this);
    info->typeName = tr("Build");
    info->kitId = k->id();
    info->buildType = type;
    return info;
}

QList<BuildInfo *> QbsBuildConfigurationFactory::availableBuilds(const Target *parent) const
{
    return {createBuildInfo(parent->kit(), BuildConfiguration::Debug)};
}

QList<BuildInfo *> QbsBuildConfigurationFactory::availableSetups(const Kit *k, const QString &projectPath) const
{
    QList<BuildInfo *> result;

    BuildInfo *info = createBuildInfo(k, BuildConfiguration::Debug);
    //: The name of the debug build configuration created by default for a qbs project.
    info->displayName = tr("Debug");
    //: Non-ASCII characters in directory suffix may cause build issues.
    info->buildDirectory
            = defaultBuildDirectory(projectPath, k, tr("Debug", "Shadow build directory suffix"),
                                    info->buildType);
    result << info;

    info = createBuildInfo(k, BuildConfiguration::Release);
    //: The name of the release build configuration created by default for a qbs project.
    info->displayName = tr("Release");
    //: Non-ASCII characters in directory suffix may cause build issues.
    info->buildDirectory
            = defaultBuildDirectory(projectPath, k, tr("Release", "Shadow build directory suffix"),
                                    info->buildType);
    result << info;

    return result;
}

} // namespace Internal
} // namespace QbsProjectManager
