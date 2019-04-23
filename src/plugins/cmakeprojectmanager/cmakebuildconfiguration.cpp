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

#include "cmakebuildconfiguration.h"

#include "builddirmanager.h"
#include "cmakebuildstep.h"
#include "cmakeconfigitem.h"
#include "cmakekitinformation.h"
#include "cmakeprojectconstants.h"
#include "cmakebuildsettingswidget.h"
#include "cmakeprojectmanager.h"
#include "cmakeprojectnodes.h"

#include <coreplugin/icore.h>

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmacroexpander.h>
#include <projectexplorer/target.h>

#include <utils/algorithm.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager {

class CMakeExtraBuildInfo
{
public:
    QString sourceDirectory;
    CMakeConfig configuration;
};

} // namespace CMakeProjectManager

Q_DECLARE_METATYPE(CMakeProjectManager::CMakeExtraBuildInfo)


namespace CMakeProjectManager {
namespace Internal {

const char INITIAL_ARGUMENTS[] = "CMakeProjectManager.CMakeBuildConfiguration.InitialArgument"; // Obsolete since QtC 3.7
const char CONFIGURATION_KEY[] = "CMake.Configuration";

CMakeBuildConfiguration::CMakeBuildConfiguration(Target *parent, Core::Id id)
    : BuildConfiguration(parent, id)
{
    auto project = target()->project();
    setBuildDirectory(shadowBuildDirectory(project->projectFilePath(),
                                           target()->kit(),
                                           displayName(), BuildConfiguration::Unknown));
    connect(project, &Project::parsingFinished, this, &BuildConfiguration::enabledChanged);
}

void CMakeBuildConfiguration::initialize(const BuildInfo &info)
{
    BuildConfiguration::initialize(info);

    BuildStepList *buildSteps = stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    buildSteps->appendStep(new CMakeBuildStep(buildSteps));

    BuildStepList *cleanSteps = stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
    cleanSteps->appendStep(new CMakeBuildStep(cleanSteps));

    if (info.buildDirectory.isEmpty()) {
        auto project = target()->project();
        setBuildDirectory(CMakeBuildConfiguration::shadowBuildDirectory(project->projectFilePath(),
                                                                        target()->kit(),
                                                                        info.displayName, info.buildType));
    }
    auto extraInfo = info.extraInfo.value<CMakeExtraBuildInfo>();
    setConfigurationForCMake(extraInfo.configuration);
}

bool CMakeBuildConfiguration::isEnabled() const
{
    return m_error.isEmpty() && !isParsing();
}

QString CMakeBuildConfiguration::disabledReason() const
{
    return error();
}

QVariantMap CMakeBuildConfiguration::toMap() const
{
    QVariantMap map(ProjectExplorer::BuildConfiguration::toMap());
    const QStringList config
            = Utils::transform(m_configurationForCMake, [](const CMakeConfigItem &i) { return i.toString(); });
    map.insert(QLatin1String(CONFIGURATION_KEY), config);
    return map;
}

bool CMakeBuildConfiguration::fromMap(const QVariantMap &map)
{
    if (!BuildConfiguration::fromMap(map))
        return false;

    const CMakeConfig conf
            = Utils::filtered(Utils::transform(map.value(QLatin1String(CONFIGURATION_KEY)).toStringList(),
                                               [](const QString &v) { return CMakeConfigItem::fromString(v); }),
                              [](const CMakeConfigItem &c) { return !c.isNull(); });

    // Legacy (pre QtC 3.7):
    const QStringList args = QtcProcess::splitArgs(map.value(QLatin1String(INITIAL_ARGUMENTS)).toString());
    CMakeConfig legacyConf;
    bool nextIsConfig = false;
    foreach (const QString &a, args) {
        if (a == QLatin1String("-D")) {
            nextIsConfig = true;
            continue;
        }
        if (!a.startsWith(QLatin1String("-D")))
            continue;
        legacyConf << CMakeConfigItem::fromString(nextIsConfig ? a : a.mid(2));
        nextIsConfig = false;
    }
    // End Legacy

    setConfigurationForCMake(legacyConf + conf);

    return true;
}

bool CMakeBuildConfiguration::isParsing() const
{
    return project()->isParsing() && isActive();
}

BuildTargetInfoList CMakeBuildConfiguration::appTargets() const
{
    BuildTargetInfoList appTargetList;

    for (const CMakeBuildTarget &ct : m_buildTargets) {
        if (ct.targetType == UtilityType)
            continue;

        if (ct.targetType == ExecutableType) {
            BuildTargetInfo bti;
            bti.displayName = ct.title;
            bti.targetFilePath = ct.executable;
            bti.projectFilePath = ct.sourceDirectory;
            bti.projectFilePath.appendString('/');
            bti.workingDirectory = ct.workingDirectory;
            bti.buildKey = ct.title + QChar('\n') + bti.projectFilePath.toString();
            appTargetList.list.append(bti);
        }
    }

    return appTargetList;
}

DeploymentData CMakeBuildConfiguration::deploymentData() const
{
    DeploymentData result;

    QDir sourceDir = target()->project()->projectDirectory().toString();
    QDir buildDir = buildDirectory().toString();

    QString deploymentPrefix;
    QString deploymentFilePath = sourceDir.filePath("QtCreatorDeployment.txt");

    bool hasDeploymentFile = QFileInfo::exists(deploymentFilePath);
    if (!hasDeploymentFile) {
        deploymentFilePath = buildDir.filePath("QtCreatorDeployment.txt");
        hasDeploymentFile = QFileInfo::exists(deploymentFilePath);
    }
    if (hasDeploymentFile) {
        deploymentPrefix = result.addFilesFromDeploymentFile(deploymentFilePath,
                                                             sourceDir.absolutePath());
    }

    for (const CMakeBuildTarget &ct : m_buildTargets) {
        if (ct.targetType == ExecutableType || ct.targetType == DynamicLibraryType) {
            if (!ct.executable.isEmpty()
                    && !result.deployableForLocalFile(ct.executable.toString()).isValid()) {
                result.addFile(ct.executable.toString(),
                               deploymentPrefix + buildDir.relativeFilePath(ct.executable.toFileInfo().dir().path()),
                               DeployableFile::TypeExecutable);
            }
        }
    }

    return result;
}

QStringList CMakeBuildConfiguration::buildTargetTitles() const
{
    return transform(m_buildTargets, &CMakeBuildTarget::title);
}

FileName CMakeBuildConfiguration::shadowBuildDirectory(const FileName &projectFilePath,
                                                       const Kit *k,
                                                       const QString &bcName,
                                                       BuildConfiguration::BuildType buildType)
{
    if (projectFilePath.isEmpty())
        return FileName();

    const QString projectName = projectFilePath.parentDir().fileName();
    ProjectMacroExpander expander(projectFilePath.toString(), projectName, k, bcName, buildType);
    QDir projectDir = QDir(Project::projectDirectory(projectFilePath).toString());
    QString buildPath = expander.expand(ProjectExplorerPlugin::buildDirectoryTemplate());
    buildPath.replace(" ", "-");
    return FileName::fromUserInput(projectDir.absoluteFilePath(buildPath));
}

void CMakeBuildConfiguration::buildTarget(const QString &buildTarget)
{
    const Core::Id buildStep = ProjectExplorer::Constants::BUILDSTEPS_BUILD;
    auto cmBs = qobject_cast<CMakeBuildStep *>(Utils::findOrDefault(
                                                   stepList(buildStep)->steps(),
                                                   [](const ProjectExplorer::BuildStep *bs) {
        return bs->id() == Constants::CMAKE_BUILD_STEP_ID;
    }));

    QString originalBuildTarget;
    if (cmBs) {
        originalBuildTarget = cmBs->buildTarget();
        cmBs->setBuildTarget(buildTarget);
    }

    BuildManager::buildList(stepList(buildStep));

    if (cmBs)
        cmBs->setBuildTarget(originalBuildTarget);
}

CMakeConfig CMakeBuildConfiguration::configurationFromCMake() const
{
    return m_configurationFromCMake;
}

void CMakeBuildConfiguration::setConfigurationFromCMake(const CMakeConfig &config)
{
    m_configurationFromCMake = config;
}

void CMakeBuildConfiguration::setBuildTargets(const QList<CMakeBuildTarget> &targets)
{
    m_buildTargets = targets;
}

void CMakeBuildConfiguration::setConfigurationForCMake(const QList<ConfigModel::DataItem> &items)
{
    const CMakeConfig newConfig = Utils::transform(items, [](const ConfigModel::DataItem &i) {
        CMakeConfigItem ni;
        ni.key = i.key.toUtf8();
        ni.value = i.value.toUtf8();
        ni.documentation = i.description.toUtf8();
        ni.isAdvanced = i.isAdvanced;
        ni.isUnset = i.isUnset;
        ni.inCMakeCache = i.inCMakeCache;
        ni.values = i.values;
        switch (i.type) {
        case CMakeProjectManager::ConfigModel::DataItem::BOOLEAN:
            ni.type = CMakeConfigItem::BOOL;
            break;
        case CMakeProjectManager::ConfigModel::DataItem::FILE:
            ni.type = CMakeConfigItem::FILEPATH;
            break;
        case CMakeProjectManager::ConfigModel::DataItem::DIRECTORY:
            ni.type = CMakeConfigItem::PATH;
            break;
        case CMakeProjectManager::ConfigModel::DataItem::STRING:
            ni.type = CMakeConfigItem::STRING;
            break;
        case CMakeProjectManager::ConfigModel::DataItem::UNKNOWN:
        default:
            ni.type = CMakeConfigItem::INTERNAL;
            break;
        }
        return ni;
    });

    const CMakeConfig config = configurationForCMake() + newConfig;
    setConfigurationForCMake(config);
}

void CMakeBuildConfiguration::clearError(ForceEnabledChanged fec)
{
    if (!m_error.isEmpty()) {
        m_error.clear();
        fec = ForceEnabledChanged::True;
    }
    if (fec == ForceEnabledChanged::True)
        emit enabledChanged();
}

void CMakeBuildConfiguration::emitBuildTypeChanged()
{
    emit buildTypeChanged();
}

static CMakeConfig removeDuplicates(const CMakeConfig &config)
{
    CMakeConfig result;
    // Remove duplicates (last value wins):
    QSet<QByteArray> knownKeys;
    for (int i = config.count() - 1; i >= 0; --i) {
        const CMakeConfigItem &item = config.at(i);
        if (knownKeys.contains(item.key))
            continue;
        result.append(item);
        knownKeys.insert(item.key);
    }
    Utils::sort(result, CMakeConfigItem::sortOperator());
    return result;
}

void CMakeBuildConfiguration::setConfigurationForCMake(const CMakeConfig &config)
{
    m_configurationForCMake = removeDuplicates(config);

    const Kit *k = target()->kit();
    CMakeConfig kitConfig = CMakeConfigurationKitInformation::configuration(k);
    bool hasKitOverride = false;
    foreach (const CMakeConfigItem &i, m_configurationForCMake) {
        const QString b = CMakeConfigItem::expandedValueOf(k, i.key, kitConfig);
        if (!b.isNull() && (i.expandedValue(k) != b || i.isUnset)) {
            hasKitOverride = true;
            break;
        }
    }

    if (hasKitOverride)
        setWarning(tr("CMake configuration set by the kit was overridden in the project."));
    else
        setWarning(QString());

    emit configurationForCMakeChanged();
}

CMakeConfig CMakeBuildConfiguration::configurationForCMake() const
{
    return removeDuplicates(CMakeConfigurationKitInformation::configuration(target()->kit()) + m_configurationForCMake);
}

void CMakeBuildConfiguration::setError(const QString &message)
{
    const QString oldMessage = m_error;
    if (m_error != message)
        m_error = message;
    if (oldMessage.isEmpty() && !message.isEmpty())
        emit enabledChanged();
    emit errorOccured(m_error);
}

void CMakeBuildConfiguration::setWarning(const QString &message)
{
    if (m_warning == message)
        return;
    m_warning = message;
    emit warningOccured(m_warning);
}

QString CMakeBuildConfiguration::error() const
{
    return m_error;
}

QString CMakeBuildConfiguration::warning() const
{
    return m_warning;
}

ProjectExplorer::NamedWidget *CMakeBuildConfiguration::createConfigWidget()
{
    return new CMakeBuildSettingsWidget(this);
}

/*!
  \class CMakeBuildConfigurationFactory
*/

CMakeBuildConfigurationFactory::CMakeBuildConfigurationFactory()
{
    registerBuildConfiguration<CMakeBuildConfiguration>("CMakeProjectManager.CMakeBuildConfiguration");

    setSupportedProjectType(CMakeProjectManager::Constants::CMAKEPROJECT_ID);
    setSupportedProjectMimeTypeName(Constants::CMAKEPROJECTMIMETYPE);
}

CMakeBuildConfigurationFactory::BuildType CMakeBuildConfigurationFactory::buildTypeFromByteArray(const QByteArray &in)
{
    const QByteArray bt = in.toLower();
    if (bt == "debug")
        return BuildTypeDebug;
    if (bt == "release")
        return BuildTypeRelease;
    if (bt == "relwithdebinfo")
        return BuildTypeRelWithDebInfo;
    if (bt == "minsizerel")
        return BuildTypeMinSizeRel;
    return BuildTypeNone;
}

BuildConfiguration::BuildType CMakeBuildConfigurationFactory::cmakeBuildTypeToBuildType(const CMakeBuildConfigurationFactory::BuildType &in)
{
    // Cover all common CMake build types
    if (in == BuildTypeRelease || in == BuildTypeMinSizeRel)
        return BuildConfiguration::Release;
    else if (in == BuildTypeDebug)
        return BuildConfiguration::Debug;
    else if (in == BuildTypeRelWithDebInfo)
        return BuildConfiguration::Profile;
    else
        return BuildConfiguration::Unknown;
}

QList<BuildInfo> CMakeBuildConfigurationFactory::availableBuilds(const Target *parent) const
{
    QList<BuildInfo> result;

    for (int type = BuildTypeNone; type != BuildTypeLast; ++type) {
        result << createBuildInfo(parent->kit(),
                                  parent->project()->projectDirectory().toString(),
                                  BuildType(type));
    }
    return result;
}

QList<BuildInfo> CMakeBuildConfigurationFactory::availableSetups(const Kit *k, const QString &projectPath) const
{
    QList<BuildInfo> result;
    const FileName projectPathName = FileName::fromString(projectPath);
    for (int type = BuildTypeNone; type != BuildTypeLast; ++type) {
        BuildInfo info = createBuildInfo(k,
                                         ProjectExplorer::Project::projectDirectory(projectPathName).toString(),
                                         BuildType(type));
        if (type == BuildTypeNone) {
            //: The name of the build configuration created by default for a cmake project.
            info.displayName = tr("Default");
        } else {
            info.displayName = info.typeName;
        }
        info.buildDirectory
                = CMakeBuildConfiguration::shadowBuildDirectory(projectPathName, k,
                                                                info.displayName, info.buildType);
        result << info;
    }
    return result;
}

BuildInfo CMakeBuildConfigurationFactory::createBuildInfo(const Kit *k,
                                                          const QString &sourceDir,
                                                          BuildType buildType) const
{
    BuildInfo info(this);
    info.kitId = k->id();

    CMakeExtraBuildInfo extra;
    extra.sourceDirectory = sourceDir;

    CMakeConfigItem buildTypeItem;
    switch (buildType) {
    case BuildTypeNone:
        info.typeName = tr("Build");
        break;
    case BuildTypeDebug:
        buildTypeItem = {CMakeConfigItem("CMAKE_BUILD_TYPE", "Debug")};
        info.typeName = tr("Debug");
        info.buildType = BuildConfiguration::Debug;
        break;
    case BuildTypeRelease:
        buildTypeItem = {CMakeConfigItem("CMAKE_BUILD_TYPE", "Release")};
        info.typeName = tr("Release");
        info.buildType = BuildConfiguration::Release;
        break;
    case BuildTypeMinSizeRel:
        buildTypeItem = {CMakeConfigItem("CMAKE_BUILD_TYPE", "MinSizeRel")};
        info.typeName = tr("Minimum Size Release");
        info.buildType = BuildConfiguration::Release;
        break;
    case BuildTypeRelWithDebInfo:
        buildTypeItem = {CMakeConfigItem("CMAKE_BUILD_TYPE", "RelWithDebInfo")};
        info.typeName = tr("Release with Debug Information");
        info.buildType = BuildConfiguration::Profile;
        break;
    default:
        QTC_CHECK(false);
        break;
    }

    if (!buildTypeItem.isNull())
        extra.configuration.append(buildTypeItem);

    const QString sysRoot = SysRootKitInformation::sysRoot(k).toString();
    if (!sysRoot.isEmpty()) {
        extra.configuration.append(CMakeConfigItem("CMAKE_SYSROOT", sysRoot.toUtf8()));
        ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(
            k, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
        if (tc) {
            const QByteArray targetTriple = tc->originalTargetTriple().toUtf8();
            extra.configuration.append(CMakeConfigItem("CMAKE_C_COMPILER_TARGET", targetTriple));
            extra.configuration.append(CMakeConfigItem("CMAKE_CXX_COMPILER_TARGET ", targetTriple));
        }
    }
    info.extraInfo = QVariant::fromValue(extra);

    return info;
}

ProjectExplorer::BuildConfiguration::BuildType CMakeBuildConfiguration::buildType() const
{
    QByteArray cmakeBuildTypeName;
    QFile cmakeCache(buildDirectory().toString() + QLatin1String("/CMakeCache.txt"));
    if (cmakeCache.open(QIODevice::ReadOnly)) {
        while (!cmakeCache.atEnd()) {
            QByteArray line = cmakeCache.readLine();
            if (line.startsWith("CMAKE_BUILD_TYPE")) {
                if (int pos = line.indexOf('='))
                    cmakeBuildTypeName = line.mid(pos + 1).trimmed();
                break;
            }
        }
        cmakeCache.close();
    }

    // Cover all common CMake build types
    const CMakeBuildConfigurationFactory::BuildType cmakeBuildType
            = CMakeBuildConfigurationFactory::buildTypeFromByteArray(cmakeBuildTypeName);
    return CMakeBuildConfigurationFactory::cmakeBuildTypeToBuildType(cmakeBuildType);
}

} // namespace Internal
} // namespace CMakeProjectManager
