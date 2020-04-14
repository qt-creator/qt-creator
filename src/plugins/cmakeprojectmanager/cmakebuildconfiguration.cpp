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

#include <android/androidconstants.h>

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

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtbuildaspects.h>
#include <qtsupport/qtkitinformation.h>

#include <utils/algorithm.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QLoggingCategory>

using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager {
namespace Internal {

static Q_LOGGING_CATEGORY(cmakeBuildConfigurationLog, "qtc.cmake.bc", QtWarningMsg);

const char CONFIGURATION_KEY[] = "CMake.Configuration";

CMakeBuildConfiguration::CMakeBuildConfiguration(Target *target, Core::Id id)
    : BuildConfiguration(target, id)
{
    setBuildDirectory(shadowBuildDirectory(project()->projectFilePath(),
                                           target->kit(),
                                           displayName(),
                                           BuildConfiguration::Unknown));

    appendInitialBuildStep(Constants::CMAKE_BUILD_STEP_ID);
    appendInitialCleanStep(Constants::CMAKE_BUILD_STEP_ID);

    setInitializer([this, target](const BuildInfo &info) {

        CMakeConfig config;
        config.append({"CMAKE_BUILD_TYPE", info.typeName.toUtf8()});

        Kit *k = target->kit();
        const QString sysRoot = SysRootKitAspect::sysRoot(k).toString();
        if (!sysRoot.isEmpty()) {
            config.append(CMakeConfigItem("CMAKE_SYSROOT", sysRoot.toUtf8()));
            if (ToolChain *tc = ToolChainKitAspect::cxxToolChain(k)) {
                const QByteArray targetTriple = tc->originalTargetTriple().toUtf8();
                config.append(CMakeConfigItem("CMAKE_C_COMPILER_TARGET", targetTriple));
                config.append(CMakeConfigItem("CMAKE_CXX_COMPILER_TARGET ", targetTriple));
            }
        }

        if (DeviceTypeKitAspect::deviceTypeId(k) == Android::Constants::ANDROID_DEVICE_TYPE) {
            buildSteps()->appendStep(Android::Constants::ANDROID_BUILD_APK_ID);
            const auto &bs = buildSteps()->steps().constLast();
            m_initialConfiguration.prepend(CMakeProjectManager::CMakeConfigItem{"ANDROID_NATIVE_API_LEVEL",
                                                                                CMakeProjectManager::CMakeConfigItem::Type::STRING,
                                                                                "Android native API level",
                                                                                bs->data(Android::Constants::AndroidNdkPlatform).toString().toUtf8()});
            auto ndkLocation = bs->data(Android::Constants::NdkLocation).value<FilePath>();
            m_initialConfiguration.prepend(CMakeProjectManager::CMakeConfigItem{"ANDROID_NDK",
                                                                                CMakeProjectManager::CMakeConfigItem::Type::PATH,
                                                                                "Android NDK PATH",
                                                                                ndkLocation.toString().toUtf8()});

            m_initialConfiguration.prepend(CMakeProjectManager::CMakeConfigItem{"CMAKE_TOOLCHAIN_FILE",
                                                                                CMakeProjectManager::CMakeConfigItem::Type::PATH,
                                                                                "Android CMake toolchain file",
                                                                                ndkLocation.pathAppended("build/cmake/android.toolchain.cmake").toString().toUtf8()});

            auto androidAbis = bs->data(Android::Constants::AndroidABIs).toStringList();
            QString preferredAbi;
            if (androidAbis.contains("armeabi-v7a")) {
                preferredAbi = "armeabi-v7a";
            } else if (androidAbis.isEmpty() || androidAbis.contains("arm64-v8a")) {
                preferredAbi = "arm64-v8a";
            } else {
                preferredAbi = androidAbis.first();
            }
            // FIXME: configmodel doesn't care about our androidAbis list...
            m_initialConfiguration.prepend(CMakeProjectManager::CMakeConfigItem{"ANDROID_ABI",
                                                                                CMakeProjectManager::CMakeConfigItem::Type::STRING,
                                                                                "Android ABI",
                                                                                preferredAbi.toLatin1(),
                                                                                androidAbis});

            QtSupport::BaseQtVersion *qt = QtSupport::QtKitAspect::qtVersion(k);
            if (qt->qtVersion() >= QtSupport::QtVersionNumber{5, 14, 0}) {
                auto sdkLocation = bs->data(Android::Constants::SdkLocation).value<FilePath>();
                m_initialConfiguration.prepend(CMakeProjectManager::CMakeConfigItem{"ANDROID_SDK",
                                                                                    CMakeProjectManager::CMakeConfigItem::Type::PATH,
                                                                                    "Android SDK PATH",
                                                                                    sdkLocation.toString().toUtf8()});

            }

            m_initialConfiguration.prepend(CMakeProjectManager::CMakeConfigItem{"ANDROID_STL",
                                                                                CMakeProjectManager::CMakeConfigItem::Type::STRING,
                                                                                "Android STL",
                                                                                "c++_shared"});

            m_initialConfiguration.prepend(CMakeProjectManager::CMakeConfigItem{"CMAKE_FIND_ROOT_PATH", "%{Qt:QT_INSTALL_PREFIX}"});
        }

        if (info.buildDirectory.isEmpty()) {
            setBuildDirectory(shadowBuildDirectory(target->project()->projectFilePath(),
                                                   k,
                                                   info.displayName,
                                                   info.buildType));
        }

        setConfigurationForCMake(config);

        // Only do this after everything has been set up!
        m_buildSystem = new CMakeBuildSystem(this);
    });

    const auto qmlDebuggingAspect = addAspect<QtSupport::QmlDebuggingAspect>();
    qmlDebuggingAspect->setKit(target->kit());
    connect(qmlDebuggingAspect, &QtSupport::QmlDebuggingAspect::changed,
            this, &CMakeBuildConfiguration::configurationForCMakeChanged);

    // m_buildSystem is still nullptr here since it the build directory to be available
    // before it can get created.
    //
    // This means this needs to be done in the lambda for the setInitializer(...) call
    // defined above as well as in fromMap!
}

CMakeBuildConfiguration::~CMakeBuildConfiguration()
{
    delete m_buildSystem;
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
    QTC_CHECK(!m_buildSystem);

    if (!BuildConfiguration::fromMap(map))
        return false;

    const CMakeConfig conf
            = Utils::filtered(Utils::transform(map.value(QLatin1String(CONFIGURATION_KEY)).toStringList(),
                                               [](const QString &v) { return CMakeConfigItem::fromString(v); }),
                              [](const CMakeConfigItem &c) { return !c.isNull(); });

    setConfigurationForCMake(conf);

    m_buildSystem = new CMakeBuildSystem(this);

    return true;
}

FilePath CMakeBuildConfiguration::shadowBuildDirectory(const FilePath &projectFilePath,
                                                       const Kit *k,
                                                       const QString &bcName,
                                                       BuildConfiguration::BuildType buildType)
{
    if (projectFilePath.isEmpty())
        return FilePath();

    const QString projectName = projectFilePath.parentDir().fileName();
    ProjectMacroExpander expander(projectFilePath, projectName, k, bcName, buildType);
    QDir projectDir = QDir(Project::projectDirectory(projectFilePath).toString());
    QString buildPath = expander.expand(ProjectExplorerPlugin::buildDirectoryTemplate());
    buildPath.replace(" ", "-");
    return FilePath::fromUserInput(projectDir.absoluteFilePath(buildPath));
}

void CMakeBuildConfiguration::buildTarget(const QString &buildTarget)
{
    auto cmBs = qobject_cast<CMakeBuildStep *>(Utils::findOrDefault(
                                                   buildSteps()->steps(),
                                                   [](const ProjectExplorer::BuildStep *bs) {
        return bs->id() == Constants::CMAKE_BUILD_STEP_ID;
    }));

    QString originalBuildTarget;
    if (cmBs) {
        originalBuildTarget = cmBs->buildTarget();
        cmBs->setBuildTarget(buildTarget);
    }

    BuildManager::buildList(buildSteps());

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

    if (Utils::indexOf(newConfig, [](const CMakeConfigItem &item){
            return item.key.startsWith("ANDROID_BUILD_ABI_");
        }) != -1) {
        // We always need to clean when we change the ANDROID_BUILD_ABI_ variables
        BuildManager::buildLists({cleanSteps()});
    }
}

void CMakeBuildConfiguration::clearError(ForceEnabledChanged fec)
{
    if (!m_error.isEmpty()) {
        m_error.clear();
        fec = ForceEnabledChanged::True;
    }
    if (fec == ForceEnabledChanged::True) {
        qCDebug(cmakeBuildConfigurationLog) << "Emitting enabledChanged signal";
        emit enabledChanged();
    }
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
    auto configs = removeDuplicates(config);
    if (m_configurationForCMake.isEmpty())
        m_configurationForCMake = removeDuplicates(m_initialConfiguration +
            CMakeConfigurationKitAspect::configuration(target()->kit()) + configs);
    else
        m_configurationForCMake = configs;

    const Kit *k = target()->kit();
    CMakeConfig kitConfig = CMakeConfigurationKitAspect::configuration(k);
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
    return removeDuplicates(CMakeConfigurationKitAspect::configuration(target()->kit()) + m_configurationForCMake);
}

void CMakeBuildConfiguration::setError(const QString &message)
{
    qCDebug(cmakeBuildConfigurationLog) << "Setting error to" << message;
    QTC_ASSERT(!message.isEmpty(), return );

    const QString oldMessage = m_error;
    if (m_error != message)
        m_error = message;
    if (oldMessage.isEmpty() != !message.isEmpty()) {
        qCDebug(cmakeBuildConfigurationLog) << "Emitting enabledChanged signal";
        emit enabledChanged();
    }
    emit errorOccurred(m_error);
}

void CMakeBuildConfiguration::setWarning(const QString &message)
{
    if (m_warning == message)
        return;
    m_warning = message;
    emit warningOccurred(m_warning);
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
    registerBuildConfiguration<CMakeBuildConfiguration>(
        "CMakeProjectManager.CMakeBuildConfiguration");

    setSupportedProjectType(CMakeProjectManager::Constants::CMAKEPROJECT_ID);
    setSupportedProjectMimeTypeName(Constants::CMAKEPROJECTMIMETYPE);

    setBuildGenerator([](const Kit *k, const FilePath &projectPath, bool forSetup) {
        QList<BuildInfo> result;

        FilePath path = forSetup ? Project::projectDirectory(projectPath) : projectPath;

        for (int type = BuildTypeDebug; type != BuildTypeLast; ++type) {
            BuildInfo info = createBuildInfo(BuildType(type));
            if (forSetup) {
                info.buildDirectory = CMakeBuildConfiguration::shadowBuildDirectory(projectPath,
                                k,
                                info.typeName,
                                info.buildType);
            }
            result << info;
        }
        return result;
    });
}

CMakeBuildConfigurationFactory::BuildType CMakeBuildConfigurationFactory::buildTypeFromByteArray(
    const QByteArray &in)
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

BuildConfiguration::BuildType CMakeBuildConfigurationFactory::cmakeBuildTypeToBuildType(
    const CMakeBuildConfigurationFactory::BuildType &in)
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

BuildInfo CMakeBuildConfigurationFactory::createBuildInfo(BuildType buildType)
{
    BuildInfo info;

    switch (buildType) {
    case BuildTypeNone:
        info.typeName = "Build";
        info.displayName = BuildConfiguration::tr("Build");
        info.buildType = BuildConfiguration::Unknown;
        break;
    case BuildTypeDebug:
        info.typeName = "Debug";
        info.displayName = BuildConfiguration::tr("Debug");
        info.buildType = BuildConfiguration::Debug;
        break;
    case BuildTypeRelease:
        info.typeName = "Release";
        info.displayName = BuildConfiguration::tr("Release");
        info.buildType = BuildConfiguration::Release;
        break;
    case BuildTypeMinSizeRel:
        info.typeName = "MinSizeRel";
        info.displayName = CMakeBuildConfiguration::tr("Minimum Size Release");
        info.buildType = BuildConfiguration::Release;
        break;
    case BuildTypeRelWithDebInfo:
        info.typeName = "RelWithDebInfo";
        info.displayName = CMakeBuildConfiguration::tr("Release with Debug Information");
        info.buildType = BuildConfiguration::Profile;
        break;
    default:
        QTC_CHECK(false);
        break;
    }

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

BuildSystem *CMakeBuildConfiguration::buildSystem() const
{
    return m_buildSystem;
}

} // namespace Internal
} // namespace CMakeProjectManager
