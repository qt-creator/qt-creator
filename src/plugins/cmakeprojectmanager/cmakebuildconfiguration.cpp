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
#include "cmakebuildinfo.h"
#include "cmakebuildstep.h"
#include "cmakekitinformation.h"
#include "cmakeproject.h"
#include "cmakeprojectconstants.h"
#include "cmakebuildsettingswidget.h"
#include "cmakeprojectmanager.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmacroexpander.h>
#include <projectexplorer/target.h>

#include <utils/algorithm.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QHash>
#include <QInputDialog>

using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager {
namespace Internal {

const char INITIAL_ARGUMENTS[] = "CMakeProjectManager.CMakeBuildConfiguration.InitialArgument"; // Obsolete since QtC 3.7
const char CONFIGURATION_KEY[] = "CMake.Configuration";

CMakeBuildConfiguration::CMakeBuildConfiguration(ProjectExplorer::Target *parent) :
    BuildConfiguration(parent, Core::Id(Constants::CMAKE_BC_ID))
{
    auto project = static_cast<CMakeProject *>(parent->project());
    setBuildDirectory(shadowBuildDirectory(project->projectFilePath(),
                                           parent->kit(),
                                           displayName(), BuildConfiguration::Unknown));

    m_buildDirManager = new BuildDirManager(this);
    connect(m_buildDirManager, &BuildDirManager::dataAvailable,
            this, &CMakeBuildConfiguration::dataAvailable);
    connect(m_buildDirManager, &BuildDirManager::errorOccured,
            this, &CMakeBuildConfiguration::setError);
    connect(m_buildDirManager, &BuildDirManager::configurationStarted,
            this, [this]() { m_completeConfigurationCache.clear(); emit parsingStarted(); });

    connect(this, &CMakeBuildConfiguration::environmentChanged,
            m_buildDirManager, &BuildDirManager::forceReparse);
    connect(this, &CMakeBuildConfiguration::buildDirectoryChanged,
            m_buildDirManager, &BuildDirManager::forceReparse);
    connect(target(), &Target::kitChanged, m_buildDirManager, &BuildDirManager::forceReparse);

    connect(this, &CMakeBuildConfiguration::parsingStarted, project, &CMakeProject::handleParsingStarted);
    connect(this, &CMakeBuildConfiguration::dataAvailable, project, &CMakeProject::parseCMakeOutput);
}

CMakeBuildConfiguration::~CMakeBuildConfiguration()
{
    m_buildDirManager->deleteLater(); // Do not block while waiting for cmake...
}

bool CMakeBuildConfiguration::isEnabled() const
{
    return m_error.isEmpty();
}

QString CMakeBuildConfiguration::disabledReason() const
{
    return error();
}

CMakeBuildConfiguration::CMakeBuildConfiguration(ProjectExplorer::Target *parent,
                                                 CMakeBuildConfiguration *source) :
    BuildConfiguration(parent, source),
    m_configuration(source->m_configuration)
{
    Q_ASSERT(parent);
    cloneSteps(source);
}

QVariantMap CMakeBuildConfiguration::toMap() const
{
    QVariantMap map(ProjectExplorer::BuildConfiguration::toMap());
    const QStringList config
            = Utils::transform(m_configuration, [](const CMakeConfigItem &i) { return i.toString(); });
    map.insert(QLatin1String(CONFIGURATION_KEY), config);
    return map;
}

bool CMakeBuildConfiguration::fromMap(const QVariantMap &map)
{
    if (!BuildConfiguration::fromMap(map))
        return false;

    const CMakeConfig conf
    = Utils::transform(map.value(QLatin1String(CONFIGURATION_KEY)).toStringList(),
                       [](const QString &v) { return CMakeConfigItem::fromString(v); });

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

    setCMakeConfiguration(legacyConf + conf);

    return true;
}

BuildDirManager *CMakeBuildConfiguration::buildDirManager() const
{
    return m_buildDirManager;
}

bool CMakeBuildConfiguration::isParsing() const
{
    return m_buildDirManager && m_buildDirManager->isParsing();
}

void CMakeBuildConfiguration::parse()
{
    m_buildDirManager->parse();
}

void CMakeBuildConfiguration::resetData()
{
    m_buildDirManager->resetData();
}

bool CMakeBuildConfiguration::persistCMakeState()
{
    return m_buildDirManager->persistCMakeState();
}

FileName CMakeBuildConfiguration::shadowBuildDirectory(const FileName &projectFilePath,
                                                       const Kit *k,
                                                       const QString &bcName,
                                                       BuildConfiguration::BuildType buildType)
{
    if (projectFilePath.isEmpty())
        return FileName();

    const QString projectName = projectFilePath.parentDir().fileName();
    ProjectMacroExpander expander(projectName, k, bcName, buildType);
    QDir projectDir = QDir(Project::projectDirectory(projectFilePath).toString());
    QString buildPath = expander.expand(Core::DocumentManager::buildDirectory());
    return FileName::fromUserInput(projectDir.absoluteFilePath(buildPath));
}

QList<ConfigModel::DataItem> CMakeBuildConfiguration::completeCMakeConfiguration() const
{
    if (m_buildDirManager->isParsing())
        return QList<ConfigModel::DataItem>();

    if (m_completeConfigurationCache.isEmpty())
        m_completeConfigurationCache = m_buildDirManager->configuration();

    return Utils::transform(m_completeConfigurationCache, [](const CMakeConfigItem &i) {
        ConfigModel::DataItem j;
        j.key = QString::fromUtf8(i.key);
        j.value = QString::fromUtf8(i.value);
        j.description = QString::fromUtf8(i.documentation);

        j.isAdvanced = i.isAdvanced;
        switch (i.type) {
        case CMakeConfigItem::FILEPATH:
            j.type = ConfigModel::DataItem::FILE;
            break;
        case CMakeConfigItem::PATH:
            j.type = ConfigModel::DataItem::DIRECTORY;
            break;
        case CMakeConfigItem::BOOL:
            j.type = ConfigModel::DataItem::BOOLEAN;
            break;
        case CMakeConfigItem::STRING:
            j.type = ConfigModel::DataItem::STRING;
            break;
        default:
            j.type = ConfigModel::DataItem::UNKNOWN;
            break;
        }

        return j;
    });
}

void CMakeBuildConfiguration::setCurrentCMakeConfiguration(const QList<ConfigModel::DataItem> &items)
{
    if (m_buildDirManager->isParsing())
        return;

    const CMakeConfig newConfig = Utils::transform(items, [](const ConfigModel::DataItem &i) {
        CMakeConfigItem ni;
        ni.key = i.key.toUtf8();
        ni.value = i.value.toUtf8();
        ni.documentation = i.description.toUtf8();
        ni.isAdvanced = i.isAdvanced;
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

    // There is a buildDirManager, so there must also be an active BC:
    const CMakeConfig config = cmakeConfiguration() + newConfig;
    setCMakeConfiguration(config);

    m_buildDirManager->forceReparse();
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

void CMakeBuildConfiguration::setCMakeConfiguration(const CMakeConfig &config)
{
    m_configuration = removeDuplicates(config);
}

CMakeConfig CMakeBuildConfiguration::cmakeConfiguration() const
{
    return m_configuration;
}

void CMakeBuildConfiguration::setError(const QString &message)
{
    if (m_error == message)
        return;
    m_error = message;
    emit enabledChanged();
    emit errorOccured(m_error);
}

QString CMakeBuildConfiguration::error() const
{
    return m_error;
}

ProjectExplorer::NamedWidget *CMakeBuildConfiguration::createConfigWidget()
{
    return new CMakeBuildSettingsWidget(this);
}

/*!
  \class CMakeBuildConfigurationFactory
*/

CMakeBuildConfigurationFactory::CMakeBuildConfigurationFactory(QObject *parent) :
    ProjectExplorer::IBuildConfigurationFactory(parent)
{ }

int CMakeBuildConfigurationFactory::priority(const ProjectExplorer::Target *parent) const
{
    return canHandle(parent) ? 0 : -1;
}

QList<ProjectExplorer::BuildInfo *> CMakeBuildConfigurationFactory::availableBuilds(const ProjectExplorer::Target *parent) const
{
    QList<ProjectExplorer::BuildInfo *> result;

    for (int type = BuildTypeNone; type != BuildTypeLast; ++type) {
        CMakeBuildInfo *info = createBuildInfo(parent->kit(),
                                               parent->project()->projectDirectory().toString(),
                                               BuildType(type));
        result << info;
    }
    return result;
}

int CMakeBuildConfigurationFactory::priority(const ProjectExplorer::Kit *k, const QString &projectPath) const
{
    Utils::MimeDatabase mdb;
    if (k && mdb.mimeTypeForFile(projectPath).matchesName(QLatin1String(Constants::CMAKEPROJECTMIMETYPE)))
        return 0;
    return -1;
}

QList<ProjectExplorer::BuildInfo *> CMakeBuildConfigurationFactory::availableSetups(const ProjectExplorer::Kit *k,
                                                                                    const QString &projectPath) const
{
    QList<ProjectExplorer::BuildInfo *> result;
    const FileName projectPathName = FileName::fromString(projectPath);
    for (int type = BuildTypeNone; type != BuildTypeLast; ++type) {
        CMakeBuildInfo *info = createBuildInfo(k,
                                               ProjectExplorer::Project::projectDirectory(projectPathName).toString(),
                                               BuildType(type));
        if (type == BuildTypeNone) {
            //: The name of the build configuration created by default for a cmake project.
            info->displayName = tr("Default");
        } else {
            info->displayName = info->typeName;
        }
        info->buildDirectory
                = CMakeBuildConfiguration::shadowBuildDirectory(projectPathName, k,
                                                                info->displayName, info->buildType);
        result << info;
    }
    return result;
}

ProjectExplorer::BuildConfiguration *CMakeBuildConfigurationFactory::create(ProjectExplorer::Target *parent,
                                                                            const ProjectExplorer::BuildInfo *info) const
{
    QTC_ASSERT(info->factory() == this, return 0);
    QTC_ASSERT(info->kitId == parent->kit()->id(), return 0);
    QTC_ASSERT(!info->displayName.isEmpty(), return 0);

    CMakeBuildInfo copy(*static_cast<const CMakeBuildInfo *>(info));
    CMakeProject *project = static_cast<CMakeProject *>(parent->project());

    if (copy.buildDirectory.isEmpty()) {
        copy.buildDirectory
                = CMakeBuildConfiguration::shadowBuildDirectory(project->projectFilePath(),
                                                                parent->kit(),
                                                                copy.displayName, info->buildType);
    }

    auto bc = new CMakeBuildConfiguration(parent);
    bc->setDisplayName(copy.displayName);
    bc->setDefaultDisplayName(copy.displayName);

    ProjectExplorer::BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    ProjectExplorer::BuildStepList *cleanSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);

    auto buildStep = new CMakeBuildStep(buildSteps);
    buildSteps->insertStep(0, buildStep);

    auto cleanStep = new CMakeBuildStep(cleanSteps);
    cleanSteps->insertStep(0, cleanStep);
    cleanStep->setBuildTarget(CMakeBuildStep::cleanTarget(), true);

    bc->setBuildDirectory(copy.buildDirectory);
    bc->setCMakeConfiguration(copy.configuration);

    // Default to all
    if (project->hasBuildTarget(QLatin1String("all")))
        buildStep->setBuildTarget(QLatin1String("all"), true);

    return bc;
}

bool CMakeBuildConfigurationFactory::canClone(const ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) const
{
    if (!canHandle(parent))
        return false;
    return source->id() == Constants::CMAKE_BC_ID;
}

CMakeBuildConfiguration *CMakeBuildConfigurationFactory::clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    auto old = static_cast<CMakeBuildConfiguration *>(source);
    return new CMakeBuildConfiguration(parent, old);
}

bool CMakeBuildConfigurationFactory::canRestore(const ProjectExplorer::Target *parent, const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;
    return ProjectExplorer::idFromMap(map) == Constants::CMAKE_BC_ID;
}

CMakeBuildConfiguration *CMakeBuildConfigurationFactory::restore(ProjectExplorer::Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    auto bc = new CMakeBuildConfiguration(parent);
    if (bc->fromMap(map))
        return bc;
    delete bc;
    return 0;
}

bool CMakeBuildConfigurationFactory::canHandle(const ProjectExplorer::Target *t) const
{
    QTC_ASSERT(t, return false);
    if (!t->project()->supportsKit(t->kit()))
        return false;
    return qobject_cast<CMakeProject *>(t->project());
}

CMakeBuildInfo *CMakeBuildConfigurationFactory::createBuildInfo(const ProjectExplorer::Kit *k,
                                                                const QString &sourceDir,
                                                                BuildType buildType) const
{
    auto info = new CMakeBuildInfo(this);
    info->kitId = k->id();
    info->sourceDirectory = sourceDir;
    info->configuration = CMakeConfigurationKitInformation::configuration(k);

    CMakeConfigItem buildTypeItem;
    switch (buildType) {
    case BuildTypeNone:
        info->typeName = tr("Build");
        break;
    case BuildTypeDebug:
        buildTypeItem = { CMakeConfigItem("CMAKE_BUILD_TYPE", "Debug") };
        info->typeName = tr("Debug");
        info->buildType = BuildConfiguration::Debug;
        break;
    case BuildTypeRelease:
        buildTypeItem = { CMakeConfigItem("CMAKE_BUILD_TYPE", "Release") };
        info->typeName = tr("Release");
        info->buildType = BuildConfiguration::Release;
        break;
    case BuildTypeMinSizeRel:
        buildTypeItem = { CMakeConfigItem("CMAKE_BUILD_TYPE", "MinSizeRel") };
        info->typeName = tr("Minimum Size Release");
        info->buildType = BuildConfiguration::Release;
        break;
    case BuildTypeRelWithDebInfo:
        buildTypeItem = { CMakeConfigItem("CMAKE_BUILD_TYPE", "RelWithDebInfo") };
        info->typeName = tr("Release with Debug Information");
        info->buildType = BuildConfiguration::Profile;
        break;
    default:
        QTC_CHECK(false);
        break;
    }

    if (!buildTypeItem.isNull())
        info->configuration.append(buildTypeItem);

    return info;
}

ProjectExplorer::BuildConfiguration::BuildType CMakeBuildConfiguration::buildType() const
{
    QString cmakeBuildType;
    QFile cmakeCache(buildDirectory().toString() + QLatin1String("/CMakeCache.txt"));
    if (cmakeCache.open(QIODevice::ReadOnly)) {
        while (!cmakeCache.atEnd()) {
            QByteArray line = cmakeCache.readLine();
            if (line.startsWith("CMAKE_BUILD_TYPE")) {
                if (int pos = line.indexOf('='))
                    cmakeBuildType = QString::fromLocal8Bit(line.mid(pos + 1).trimmed());
                break;
            }
        }
        cmakeCache.close();
    }

    // Cover all common CMake build types
    if (cmakeBuildType.compare(QLatin1String("Release"), Qt::CaseInsensitive) == 0
        || cmakeBuildType.compare(QLatin1String("MinSizeRel"), Qt::CaseInsensitive) == 0) {
        return Release;
    } else if (cmakeBuildType.compare(QLatin1String("Debug"), Qt::CaseInsensitive) == 0
               || cmakeBuildType.compare(QLatin1String("DebugFull"), Qt::CaseInsensitive) == 0) {
        return Debug;
    } else if (cmakeBuildType.compare(QLatin1String("RelWithDebInfo"), Qt::CaseInsensitive) == 0) {
        return Profile;
    }

    return Unknown;
}

} // namespace Internal
} // namespace CMakeProjectManager
