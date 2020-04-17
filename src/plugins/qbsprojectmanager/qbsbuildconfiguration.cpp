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
#include "qbssettings.h"

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
#include <QCryptographicHash>

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
    FilePath projectDir = Project::projectDirectory(projectFilePath);
    QString buildPath = expander.expand(ProjectExplorerPlugin::buildDirectoryTemplate());
    return projectDir.resolvePath(buildPath);
}

// ---------------------------------------------------------------------------
// QbsBuildConfiguration:
// ---------------------------------------------------------------------------

QbsBuildConfiguration::QbsBuildConfiguration(Target *target, Core::Id id)
    : BuildConfiguration(target, id)
{
    setConfigWidgetHasFrame(true);

    appendInitialBuildStep(Constants::QBS_BUILDSTEP_ID);
    appendInitialCleanStep(Constants::QBS_CLEANSTEP_ID);

    setInitializer([this, target](const BuildInfo &info) {
        const Kit *kit = target->kit();
        QVariantMap configData = info.extraInfo.value<QVariantMap>();
        configData.insert(QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY),
                          (info.buildType == BuildConfiguration::Debug)
                          ? QLatin1String(Constants::QBS_VARIANT_DEBUG)
                          : QLatin1String(Constants::QBS_VARIANT_RELEASE));

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

        m_configurationName->setValue(uniqueConfigName);

        auto bs = buildSteps()->firstOfType<QbsBuildStep>();
        QTC_ASSERT(bs, return);
        bs->setQbsConfiguration(bd);

        emit qbsConfigurationChanged();
    });

    m_configurationName = addAspect<BaseStringAspect>();
    m_configurationName->setLabelText(tr("Configuration name:"));
    m_configurationName->setSettingsKey("Qbs.configName");
    m_configurationName->setDisplayStyle(BaseStringAspect::LineEditDisplay);
    connect(m_configurationName, &BaseStringAspect::changed,
            this, &BuildConfiguration::buildDirectoryChanged);

    const auto separateDebugInfoAspect = addAspect<SeparateDebugInfoAspect>();
    connect(separateDebugInfoAspect, &SeparateDebugInfoAspect::changed,
            this, &QbsBuildConfiguration::qbsConfigurationChanged);

    const auto qmlDebuggingAspect = addAspect<QtSupport::QmlDebuggingAspect>();
    qmlDebuggingAspect->setKit(target->kit());
    connect(qmlDebuggingAspect, &QtSupport::QmlDebuggingAspect::changed,
            this, &QbsBuildConfiguration::qbsConfigurationChanged);

    const auto qtQuickCompilerAspect = addAspect<QtSupport::QtQuickCompilerAspect>();
    qtQuickCompilerAspect->setKit(target->kit());
    connect(qtQuickCompilerAspect, &QtSupport::QtQuickCompilerAspect::changed,
            this, &QbsBuildConfiguration::qbsConfigurationChanged);

    connect(this, &BuildConfiguration::environmentChanged,
            this, &QbsBuildConfiguration::triggerReparseIfActive);
    connect(this, &BuildConfiguration::buildDirectoryChanged,
            this, &QbsBuildConfiguration::triggerReparseIfActive);
    connect(this, &QbsBuildConfiguration::qbsConfigurationChanged,
            this, &QbsBuildConfiguration::triggerReparseIfActive);

    macroExpander()->registerVariable("CurrentBuild:QbsBuildRoot", tr("The qbs project build root"),
        [this] { return buildDirectory().pathAppended(configurationName()).toUserOutput(); });

    m_buildSystem = new QbsBuildSystem(this);
}

QbsBuildConfiguration::~QbsBuildConfiguration()
{
    for (BuildStep * const bs : buildSteps()->steps()) {
        if (const auto qbs = qobject_cast<QbsBuildStep *>(bs))
            qbs->dropSession();
    }
    for (BuildStep * const cs : cleanSteps()->steps()) {
        if (const auto qcs = qobject_cast<QbsCleanStep *>(cs))
            qcs->dropSession();
    }
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

bool QbsBuildConfiguration::fromMap(const QVariantMap &map)
{
    if (!BuildConfiguration::fromMap(map))
        return false;

    if (m_configurationName->value().isEmpty()) { // pre-4.4 backwards compatibility
        const QString profileName = QbsProfileManager::profileNameForKit(target()->kit());
        const QString buildVariant = qbsConfiguration()
                .value(QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY)).toString();
        m_configurationName->setValue(profileName + '-' + buildVariant);
    }

    return true;
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

QString QbsBuildConfiguration::equivalentCommandLine(const QbsBuildStepData &stepData) const
{
    CommandLine commandLine;
    const QString qbsInstallDir = QString::fromLocal8Bit(qgetenv("QBS_INSTALL_DIR"));
    const QString qbsFilePath = HostOsInfo::withExecutableSuffix(!qbsInstallDir.isEmpty()
            ? qbsInstallDir + QLatin1String("/bin/qbs")
            : QCoreApplication::applicationDirPath() + QLatin1String("/qbs"));
    commandLine.addArg(QDir::toNativeSeparators(qbsFilePath));
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

TriState QbsBuildConfiguration::qmlDebuggingSetting() const
{
    return aspect<QtSupport::QmlDebuggingAspect>()->setting();
}

TriState QbsBuildConfiguration::qtQuickCompilerSetting() const
{
    return aspect<QtSupport::QtQuickCompilerAspect>()->setting();
}

TriState QbsBuildConfiguration::separateDebugInfoSetting() const
{
    return aspect<SeparateDebugInfoAspect>()->setting();
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

    setBuildGenerator([this](const Kit *k, const FilePath &projectPath, bool forSetup) {
        QList<BuildInfo> result;

        if (forSetup) {
            BuildInfo info = createBuildInfo(BuildConfiguration::Debug);
            //: The name of the debug build configuration created by default for a qbs project.
            info.displayName = BuildConfiguration::tr("Debug");
            //: Non-ASCII characters in directory suffix may cause build issues.
            const QString dbg = QbsBuildConfiguration::tr("Debug", "Shadow build directory suffix");
            info.buildDirectory = defaultBuildDirectory(projectPath, k, dbg, info.buildType);
            result << info;

            info = createBuildInfo(BuildConfiguration::Release);
            //: The name of the release build configuration created by default for a qbs project.
            info.displayName = BuildConfiguration::tr("Release");
            //: Non-ASCII characters in directory suffix may cause build issues.
            const QString rel = QbsBuildConfiguration::tr("Release", "Shadow build directory suffix");
            info.buildDirectory = defaultBuildDirectory(projectPath, k, rel, info.buildType);
            result << info;
        } else {
            result << createBuildInfo(BuildConfiguration::Debug);
            result << createBuildInfo(BuildConfiguration::Release);
        }

        return result;
    });
}

BuildInfo QbsBuildConfigurationFactory::createBuildInfo(BuildConfiguration::BuildType type) const
{
    BuildInfo info;
    info.buildType = type;
    info.typeName = type == BuildConfiguration::Debug
            ? BuildConfiguration::tr("Debug") : BuildConfiguration::tr("Release");
    QVariantMap config;
    config.insert("configName", type == BuildConfiguration::Debug ? "Debug" : "Release");
    info.extraInfo = config;
    return info;
}

} // namespace Internal
} // namespace QbsProjectManager
