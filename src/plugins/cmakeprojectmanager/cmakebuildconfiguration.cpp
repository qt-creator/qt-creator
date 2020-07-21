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

#include "cmakebuildsettingswidget.h"
#include "cmakebuildstep.h"
#include "cmakebuildsystem.h"
#include "cmakekitinformation.h"
#include "cmakeprojectconstants.h"

#include <android/androidconstants.h>
#include <projectexplorer/buildaspects.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmacroexpander.h>
#include <projectexplorer/target.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtbuildaspects.h>
#include <qtsupport/qtkitinformation.h>

#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QDir>
#include <QLoggingCategory>
#include <QMessageBox>

using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager {
namespace Internal {

static Q_LOGGING_CATEGORY(cmakeBuildConfigurationLog, "qtc.cmake.bc", QtWarningMsg);

const char CONFIGURATION_KEY[] = "CMake.Configuration";

// -----------------------------------------------------------------------------
// Helper:
// -----------------------------------------------------------------------------

static QStringList defaultInitialCMakeArguments(const Kit *k, const QString buildType)
{
    // Generator:
    QStringList initialArgs = CMakeGeneratorKitAspect::generatorArguments(k);

    // CMAKE_BUILD_TYPE:
    if (!buildType.isEmpty())
        initialArgs.append(QString::fromLatin1("-DCMAKE_BUILD_TYPE:String=%1").arg(buildType));

    // Cross-compilation settings:
    const QString sysRoot = SysRootKitAspect::sysRoot(k).toString();
    if (!sysRoot.isEmpty()) {
        initialArgs.append(QString::fromLatin1("-DCMAKE_SYSROOT:PATH=%1").arg(sysRoot));
        if (ToolChain *tc = ToolChainKitAspect::cxxToolChain(k)) {
            const QString targetTriple = tc->originalTargetTriple();
            initialArgs.append(
                QString::fromLatin1("-DCMAKE_C_COMPILER_TARGET:STRING=%1").arg(targetTriple));
            initialArgs.append(
                QString::fromLatin1("-DCMAKE_CXX_COMPILER_TARGET:STRING=%1").arg(targetTriple));
        }
    }

    initialArgs += CMakeConfigurationKitAspect::toArgumentsList(k);

    return initialArgs;
}

// -----------------------------------------------------------------------------
// CMakeBuildConfiguration:
// -----------------------------------------------------------------------------

CMakeBuildConfiguration::CMakeBuildConfiguration(Target *target, Utils::Id id)
    : BuildConfiguration(target, id)
{
    m_buildSystem = new CMakeBuildSystem(this);

    buildDirectoryAspect()->setFileDialogOnly(true);
    const auto buildDirAspect = aspect<BuildDirectoryAspect>();
    buildDirAspect->setFileDialogOnly(true);
    buildDirAspect->setValueAcceptor(
        [](const QString &oldDir, const QString &newDir) -> Utils::optional<QString> {
            if (oldDir.isEmpty())
                return newDir;

            if (QDir(oldDir).exists("CMakeCache.txt")) {
                if (QMessageBox::information(nullptr,
                                             tr("Changing Build Directory"),
                                             tr("Change the build directory and start with a "
                                                "basic CMake configuration?"),
                                             QMessageBox::Ok,
                                             QMessageBox::Cancel)
                    == QMessageBox::Ok) {
                    return newDir;
                }
                return Utils::nullopt;
            }
            return newDir;
        });

    addAspect<InitialCMakeArgumentsAspect>();

    appendInitialBuildStep(Constants::CMAKE_BUILD_STEP_ID);
    appendInitialCleanStep(Constants::CMAKE_BUILD_STEP_ID);

    setInitializer([this, target](const BuildInfo &info) {
        const Kit *k = target->kit();

        QStringList initialArgs = defaultInitialCMakeArguments(k, info.typeName);

        // Android magic:
        if (DeviceTypeKitAspect::deviceTypeId(k) == Android::Constants::ANDROID_DEVICE_TYPE) {
            buildSteps()->appendStep(Android::Constants::ANDROID_BUILD_APK_ID);
            const auto &bs = buildSteps()->steps().constLast();
            initialArgs.append(
                QString::fromLatin1("-DANDROID_NATIVE_API_LEVEL:STRING=%1")
                    .arg(bs->data(Android::Constants::AndroidNdkPlatform).toString()));
            auto ndkLocation = bs->data(Android::Constants::NdkLocation).value<FilePath>();
            initialArgs.append(
                QString::fromLatin1("-DANDROID_NDK:PATH=%1").arg(ndkLocation.toString()));

            initialArgs.append(
                QString::fromLatin1("-DCMAKE_TOOLCHAIN_FILE:PATH=%1")
                    .arg(
                        ndkLocation.pathAppended("build/cmake/android.toolchain.cmake").toString()));

            auto androidAbis = bs->data(Android::Constants::AndroidABIs).toStringList();
            QString preferredAbi;
            if (androidAbis.contains("armeabi-v7a")) {
                preferredAbi = "armeabi-v7a";
            } else if (androidAbis.isEmpty() || androidAbis.contains("arm64-v8a")) {
                preferredAbi = "arm64-v8a";
            } else {
                preferredAbi = androidAbis.first();
            }
            initialArgs.append(QString::fromLatin1("-DANDROID_ABI:STRING=%1").arg(preferredAbi));

            QtSupport::BaseQtVersion *qt = QtSupport::QtKitAspect::qtVersion(k);
            if (qt && qt->qtVersion() >= QtSupport::QtVersionNumber{5, 14, 0}) {
                auto sdkLocation = bs->data(Android::Constants::SdkLocation).value<FilePath>();
                initialArgs.append(
                    QString::fromLatin1("-DANDROID_SDK:PATH=%1").arg(sdkLocation.toString()));
            }

            initialArgs.append(QString::fromLatin1("-DANDROID_STL:STRING=cxx_shared"));

            initialArgs.append(
                QString::fromLatin1("-DCMAKE_FIND_ROOT_PATH:PATH=%{Qt:QT_INSTALL_PREFIX}"));
        }

        if (info.buildDirectory.isEmpty()) {
            setBuildDirectory(shadowBuildDirectory(target->project()->projectFilePath(),
                                                   k,
                                                   info.displayName,
                                                   info.buildType));
        }

        setInitialCMakeArguments(initialArgs);
    });

    const auto qmlDebuggingAspect = addAspect<QtSupport::QmlDebuggingAspect>();
    qmlDebuggingAspect->setKit(target->kit());
}

CMakeBuildConfiguration::~CMakeBuildConfiguration()
{
    delete m_buildSystem;
}

QVariantMap CMakeBuildConfiguration::toMap() const
{
    QVariantMap map(ProjectExplorer::BuildConfiguration::toMap());
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

    // TODO: Upgrade from Qt Creator < 4.13: Remove when no longer supported!
    const QString buildTypeName = [this]() {
        switch (buildType()) {
        case Debug:
            return QString("Debug");
        case Profile:
            return QString("RelWithDebInfo");
        case Release:
            return QString("Release");
        case Unknown:
        default:
            return QString("");
        }
    }();
    if (initialCMakeArguments().isEmpty()) {
        QStringList initialArgs = defaultInitialCMakeArguments(target()->kit(),
                                                               buildTypeName)
                                  + Utils::transform(conf, [this](const CMakeConfigItem &i) {
                                        return i.toArgument(macroExpander());
                                    });

        setInitialCMakeArguments(initialArgs);
    }

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

    QStringList originalBuildTargets;
    if (cmBs) {
        originalBuildTargets = cmBs->buildTargets();
        cmBs->setBuildTargets({buildTarget});
    }

    BuildManager::buildList(buildSteps());

    if (cmBs)
        cmBs->setBuildTargets(originalBuildTargets);
}

CMakeConfig CMakeBuildConfiguration::configurationFromCMake() const
{
    return m_configurationFromCMake;
}

QStringList CMakeBuildConfiguration::extraCMakeArguments() const
{
    return m_extraCMakeArguments;
}

QStringList CMakeBuildConfiguration::initialCMakeArguments() const
{
    return aspect<InitialCMakeArgumentsAspect>()->value().split('\n', Qt::SkipEmptyParts);
}

void CMakeBuildConfiguration::setExtraCMakeArguments(const QStringList &args)
{
    if (m_extraCMakeArguments == args)
        return;

    qCDebug(cmakeBuildConfigurationLog)
        << "Extra Args changed from" << m_extraCMakeArguments << "to" << args << "...";
    m_extraCMakeArguments = args;
}

void CMakeBuildConfiguration::setConfigurationFromCMake(const CMakeConfig &config)
{
    m_configurationFromCMake = config;
}

// FIXME: Run clean steps when a setting starting with "ANDROID_BUILD_ABI_" is changed.
// FIXME: Warn when kit settings are overridden by a project.

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

void CMakeBuildConfiguration::setInitialCMakeArguments(const QStringList &args)
{
    aspect<InitialCMakeArgumentsAspect>()->setValue(args.join('\n'));
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

    setSupportedProjectType(CMakeProjectManager::Constants::CMAKE_PROJECT_ID);
    setSupportedProjectMimeTypeName(Constants::CMAKE_PROJECT_MIMETYPE);

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
    QByteArray cmakeBuildTypeName = CMakeConfigItem::valueOf("CMAKE_BUILD_TYPE", m_configurationFromCMake);

    // Cover all common CMake build types
    const CMakeBuildConfigurationFactory::BuildType cmakeBuildType
        = CMakeBuildConfigurationFactory::buildTypeFromByteArray(cmakeBuildTypeName);
    return CMakeBuildConfigurationFactory::cmakeBuildTypeToBuildType(cmakeBuildType);
}

BuildSystem *CMakeBuildConfiguration::buildSystem() const
{
    return m_buildSystem;
}

void CMakeBuildConfiguration::runCMakeWithExtraArguments()
{
    m_buildSystem->runCMakeWithExtraArguments();
}

// ----------------------------------------------------------------------
// - InitialCMakeParametersAspect:
// ----------------------------------------------------------------------

InitialCMakeArgumentsAspect::InitialCMakeArgumentsAspect()
{
    setSettingsKey("CMake.Initial.Parameters");
    setLabelText(tr("Initial CMake parameters:"));
    setDisplayStyle(TextEditDisplay);
}

} // namespace Internal
} // namespace CMakeProjectManager
