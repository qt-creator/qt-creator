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

#include "qmakebuildconfiguration.h"

#include "qmakebuildinfo.h"
#include "qmakekitinformation.h"
#include "qmakeproject.h"
#include "qmakeprojectconfigwidget.h"
#include "qmakeprojectmanagerconstants.h"
#include "qmakenodes.h"
#include "qmakestep.h"
#include "qmakemakestep.h"
#include "makefileparse.h"
#include "qmakebuildconfiguration.h"

#include <android/androidconstants.h>

#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmacroexpander.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>

#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>

#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QInputDialog>
#include <QLoggingCategory>

#include <limits>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;
using namespace QmakeProjectManager::Internal;

namespace QmakeProjectManager {

// --------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------

QString QmakeBuildConfiguration::shadowBuildDirectory(const QString &proFilePath, const Kit *k,
                                                      const QString &suffix,
                                                      BuildConfiguration::BuildType buildType)
{
    if (proFilePath.isEmpty())
        return QString();

    const QString projectName = QFileInfo(proFilePath).completeBaseName();
    ProjectMacroExpander expander(proFilePath, projectName, k, suffix, buildType);
    QString projectDir = Project::projectDirectory(FileName::fromString(proFilePath)).toString();
    QString buildPath = expander.expand(ProjectExplorerPlugin::buildDirectoryTemplate());
    return FileUtils::resolvePath(projectDir, buildPath);
}

static FileName defaultBuildDirectory(const QString &projectPath,
                                      const Kit *k,
                                      const QString &suffix,
                                      BuildConfiguration::BuildType type)
{
    return FileName::fromString(QmakeBuildConfiguration::shadowBuildDirectory(projectPath, k,
                                                                              suffix, type));
}

const char USE_SHADOW_BUILD_KEY[] = "Qt4ProjectManager.Qt4BuildConfiguration.UseShadowBuild";
const char BUILD_CONFIGURATION_KEY[] = "Qt4ProjectManager.Qt4BuildConfiguration.BuildConfiguration";

enum { debug = 0 };

QmakeBuildConfiguration::QmakeBuildConfiguration(Target *target, Core::Id id)
    : BuildConfiguration(target, id)
{
    connect(this, &BuildConfiguration::buildDirectoryChanged,
            this, &QmakeBuildConfiguration::emitProFileEvaluateNeeded);
    connect(this, &BuildConfiguration::environmentChanged,
            this, &QmakeBuildConfiguration::emitProFileEvaluateNeeded);
    connect(target, &Target::kitChanged,
            this, &QmakeBuildConfiguration::kitChanged);
    MacroExpander *expander = macroExpander();
    expander->registerVariable("Qmake:Makefile", "Qmake makefile", [this]() -> QString {
        const QString file = makefile();
        if (!file.isEmpty())
            return file;
        return QLatin1String("Makefile");
    });
    connect(ToolChainManager::instance(), &ToolChainManager::toolChainUpdated,
            this, &QmakeBuildConfiguration::toolChainUpdated);
    connect(QtVersionManager::instance(), &QtVersionManager::qtVersionsChanged,
            this, &QmakeBuildConfiguration::qtVersionsChanged);
}

void QmakeBuildConfiguration::initialize(const BuildInfo &info)
{
    BuildConfiguration::initialize(info);

    BuildStepList *buildSteps = stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    auto qmakeStep = new QMakeStep(buildSteps);
    buildSteps->appendStep(qmakeStep);
    buildSteps->appendStep(new QmakeMakeStep(buildSteps));

    BuildStepList *cleanSteps = stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
    cleanSteps->appendStep(new QmakeMakeStep(cleanSteps));

    const QmakeExtraBuildInfo qmakeExtra = info.extraInfo.value<QmakeExtraBuildInfo>();
    BaseQtVersion *version = QtKitInformation::qtVersion(target()->kit());

    BaseQtVersion::QmakeBuildConfigs config = version->defaultBuildConfig();
    if (info.buildType == BuildConfiguration::Debug)
        config |= BaseQtVersion::DebugBuild;
    else
        config &= ~BaseQtVersion::DebugBuild;

    QString additionalArguments = qmakeExtra.additionalArguments;
    if (!additionalArguments.isEmpty())
        qmakeStep->setUserArguments(additionalArguments);
    qmakeStep->setLinkQmlDebuggingLibrary(qmakeExtra.config.linkQmlDebuggingQQ2);
    qmakeStep->setSeparateDebugInfo(qmakeExtra.config.separateDebugInfo);
    qmakeStep->setUseQtQuickCompiler(qmakeExtra.config.useQtQuickCompiler);

    setQMakeBuildConfiguration(config);

    FileName directory = info.buildDirectory;
    if (directory.isEmpty()) {
        directory = defaultBuildDirectory(target()->project()->projectFilePath().toString(),
                                          target()->kit(), info.displayName, buildType());
    }

    setBuildDirectory(directory);

    if (DeviceTypeKitInformation::deviceTypeId(target()->kit())
            == Android::Constants::ANDROID_DEVICE_TYPE) {
        buildSteps->appendStep(Android::Constants::ANDROID_PACKAGE_INSTALLATION_STEP_ID);
        buildSteps->appendStep(Android::Constants::ANDROID_BUILD_APK_ID);
    }

    updateCacheAndEmitEnvironmentChanged();
}

QmakeBuildConfiguration::~QmakeBuildConfiguration() = default;

QVariantMap QmakeBuildConfiguration::toMap() const
{
    QVariantMap map(BuildConfiguration::toMap());
    map.insert(QLatin1String(USE_SHADOW_BUILD_KEY), m_shadowBuild);
    map.insert(QLatin1String(BUILD_CONFIGURATION_KEY), int(m_qmakeBuildConfiguration));
    return map;
}

bool QmakeBuildConfiguration::fromMap(const QVariantMap &map)
{
    if (!BuildConfiguration::fromMap(map))
        return false;

    m_shadowBuild = map.value(QLatin1String(USE_SHADOW_BUILD_KEY), true).toBool();
    m_qmakeBuildConfiguration = BaseQtVersion::QmakeBuildConfigs(map.value(QLatin1String(BUILD_CONFIGURATION_KEY)).toInt());

    m_lastKitState = LastKitState(target()->kit());
    return true;
}

void QmakeBuildConfiguration::kitChanged()
{
    LastKitState newState = LastKitState(target()->kit());
    if (newState != m_lastKitState) {
        // This only checks if the ids have changed!
        // For that reason the QmakeBuildConfiguration is also connected
        // to the toolchain and qtversion managers
        emitProFileEvaluateNeeded();
        m_lastKitState = newState;
    }
}

void QmakeBuildConfiguration::toolChainUpdated(ToolChain *tc)
{
    if (ToolChainKitInformation::toolChain(target()->kit(), ProjectExplorer::Constants::CXX_LANGUAGE_ID) == tc)
        emitProFileEvaluateNeeded();
}

void QmakeBuildConfiguration::qtVersionsChanged(const QList<int> &,const QList<int> &, const QList<int> &changed)
{
    if (changed.contains(QtKitInformation::qtVersionId(target()->kit())))
        emitProFileEvaluateNeeded();
}

NamedWidget *QmakeBuildConfiguration::createConfigWidget()
{
    return new QmakeProjectConfigWidget(this);
}

/// If only a sub tree should be build this function returns which sub node
/// should be build
/// \see QMakeBuildConfiguration::setSubNodeBuild
QmakeProFileNode *QmakeBuildConfiguration::subNodeBuild() const
{
    return m_subNodeBuild;
}

/// A sub node build on builds a sub node of the project
/// That is triggered by a right click in the project explorer tree
/// The sub node to be build is set via this function immediately before
/// calling BuildManager::buildProject( BuildConfiguration * )
/// and reset immediately afterwards
/// That is m_subNodesBuild is set only temporarly
void QmakeBuildConfiguration::setSubNodeBuild(QmakeProFileNode *node)
{
    m_subNodeBuild = node;
}

FileNode *QmakeBuildConfiguration::fileNodeBuild() const
{
    return m_fileNodeBuild;
}

void QmakeBuildConfiguration::setFileNodeBuild(FileNode *node)
{
    m_fileNodeBuild = node;
}

/// returns whether this is a shadow build configuration or not
/// note, even if shadowBuild() returns true, it might be using the
/// source directory as the shadow build directory, thus it
/// still is a in-source build
bool QmakeBuildConfiguration::isShadowBuild() const
{
    return buildDirectory() != target()->project()->projectDirectory();
}

QString QmakeBuildConfiguration::makefile() const
{
    auto rootNode = dynamic_cast<QmakeProFileNode *>(target()->project()->rootProjectNode());
    return rootNode ? rootNode->makefile() : QString();
}

BaseQtVersion::QmakeBuildConfigs QmakeBuildConfiguration::qmakeBuildConfiguration() const
{
    return m_qmakeBuildConfiguration;
}

void QmakeBuildConfiguration::setQMakeBuildConfiguration(BaseQtVersion::QmakeBuildConfigs config)
{
    if (m_qmakeBuildConfiguration == config)
        return;
    m_qmakeBuildConfiguration = config;

    emit qmakeBuildConfigurationChanged();
    emitProFileEvaluateNeeded();
    emit buildTypeChanged();
}

void QmakeBuildConfiguration::emitProFileEvaluateNeeded()
{
    Target *t = target();
    Project *p = t->project();
    if (t->activeBuildConfiguration() == this && p->activeTarget() == t)
        static_cast<QmakeProject *>(p)->scheduleAsyncUpdate();
}

void QmakeBuildConfiguration::emitQMakeBuildConfigurationChanged()
{
    emit qmakeBuildConfigurationChanged();
}

QStringList QmakeBuildConfiguration::configCommandLineArguments() const
{
    QStringList result;
    BaseQtVersion *version = QtKitInformation::qtVersion(target()->kit());
    BaseQtVersion::QmakeBuildConfigs defaultBuildConfiguration =
            version ? version->defaultBuildConfig() : BaseQtVersion::QmakeBuildConfigs(BaseQtVersion::DebugBuild | BaseQtVersion::BuildAll);
    BaseQtVersion::QmakeBuildConfigs userBuildConfiguration = m_qmakeBuildConfiguration;
    if ((defaultBuildConfiguration & BaseQtVersion::BuildAll) && !(userBuildConfiguration & BaseQtVersion::BuildAll))
        result << QLatin1String("CONFIG-=debug_and_release");

    if (!(defaultBuildConfiguration & BaseQtVersion::BuildAll) && (userBuildConfiguration & BaseQtVersion::BuildAll))
        result << QLatin1String("CONFIG+=debug_and_release");
    if ((defaultBuildConfiguration & BaseQtVersion::DebugBuild) && !(userBuildConfiguration & BaseQtVersion::DebugBuild))
        result << QLatin1String("CONFIG+=release");
    if (!(defaultBuildConfiguration & BaseQtVersion::DebugBuild) && (userBuildConfiguration & BaseQtVersion::DebugBuild))
        result << QLatin1String("CONFIG+=debug");
    return result;
}

QMakeStep *QmakeBuildConfiguration::qmakeStep() const
{
    QMakeStep *qs = nullptr;
    BuildStepList *bsl = stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD));
    Q_ASSERT(bsl);
    for (int i = 0; i < bsl->count(); ++i)
        if ((qs = qobject_cast<QMakeStep *>(bsl->at(i))) != nullptr)
            return qs;
    return nullptr;
}

QmakeMakeStep *QmakeBuildConfiguration::makeStep() const
{
    QmakeMakeStep *ms = nullptr;
    BuildStepList *bsl = stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD));
    Q_ASSERT(bsl);
    for (int i = 0; i < bsl->count(); ++i)
        if ((ms = qobject_cast<QmakeMakeStep *>(bsl->at(i))) != nullptr)
            return ms;
    return nullptr;
}

// Returns true if both are equal.
QmakeBuildConfiguration::MakefileState QmakeBuildConfiguration::compareToImportFrom(const QString &makefile, QString *errorString)
{
    const QLoggingCategory &logs = MakeFileParse::logging();
    qCDebug(logs) << "QMakeBuildConfiguration::compareToImport";

    QMakeStep *qs = qmakeStep();
    MakeFileParse parse(makefile);

    if (parse.makeFileState() == MakeFileParse::MakefileMissing) {
        qCDebug(logs) << "**Makefile missing";
        return MakefileMissing;
    }
    if (parse.makeFileState() == MakeFileParse::CouldNotParse) {
        qCDebug(logs) << "**Makefile incompatible";
        if (errorString)
            *errorString = tr("Could not parse Makefile.");
        return MakefileIncompatible;
    }

    if (!qs) {
        qCDebug(logs) << "**No qmake step";
        return MakefileMissing;
    }

    BaseQtVersion *version = QtKitInformation::qtVersion(target()->kit());
    if (!version) {
        qCDebug(logs) << "**No qt version in kit";
        return MakefileForWrongProject;
    }

    const Utils::FileName projectPath =
            m_subNodeBuild ? m_subNodeBuild->filePath() : qs->project()->projectFilePath();
    if (parse.srcProFile() != projectPath.toString()) {
        qCDebug(logs) << "**Different profile used to generate the Makefile:"
                      << parse.srcProFile() << " expected profile:" << projectPath;
        if (errorString)
            *errorString = tr("The Makefile is for a different project.");
        return MakefileIncompatible;
    }

    if (version->qmakeCommand() != parse.qmakePath()) {
        qCDebug(logs) << "**Different Qt versions, buildconfiguration:" << version->qmakeCommand().toString()
                      << " Makefile:"<< parse.qmakePath().toString();
        return MakefileForWrongProject;
    }

    // same qtversion
    BaseQtVersion::QmakeBuildConfigs buildConfig = parse.effectiveBuildConfig(version->defaultBuildConfig());
    if (qmakeBuildConfiguration() != buildConfig) {
        qCDebug(logs) << "**Different qmake buildconfigurations buildconfiguration:"
                      << qmakeBuildConfiguration() << " Makefile:" << buildConfig;
        if (errorString)
            *errorString = tr("The build type has changed.");
        return MakefileIncompatible;
    }

    // The qmake Build Configuration are the same,
    // now compare arguments lists
    // we have to compare without the spec/platform cmd argument
    // and compare that on its own
    QString workingDirectory = QFileInfo(makefile).absolutePath();
    QStringList actualArgs;
    QString userArgs = macroExpander()->expandProcessArgs(qs->userArguments());
    // This copies the settings from userArgs to actualArgs (minus some we
    // are not interested in), splitting them up into individual strings:
    extractSpecFromArguments(&userArgs, workingDirectory, version, &actualArgs);
    FileName actualSpec = qs->mkspec();

    QString qmakeArgs = parse.unparsedArguments();
    QStringList parsedArgs;
    FileName parsedSpec = extractSpecFromArguments(&qmakeArgs, workingDirectory, version, &parsedArgs);

    qCDebug(logs) << "  Actual args:" << actualArgs;
    qCDebug(logs) << "  Parsed args:" << parsedArgs;
    qCDebug(logs) << "  Actual spec:" << actualSpec.toString();
    qCDebug(logs) << "  Parsed spec:" << parsedSpec.toString();
    qCDebug(logs) << "  Actual config:" << qs->deducedArguments();
    qCDebug(logs) << "  Parsed config:" << parse.config();

    // Comparing the sorted list is obviously wrong
    // Though haven written a more complete version
    // that managed had around 200 lines and yet faild
    // to be actually foolproof at all, I think it's
    // not feasible without actually taking the qmake
    // command line parsing code

    // Things, sorting gets wrong:
    // parameters to positional parameters matter
    //  e.g. -o -spec is different from -spec -o
    //       -o 1 -spec 2 is diffrent from -spec 1 -o 2
    // variable assignment order matters
    // variable assignment vs -after
    // -norecursive vs. recursive
    actualArgs.sort();
    parsedArgs.sort();
    if (actualArgs != parsedArgs) {
        qCDebug(logs) << "**Mismatched args";
        if (errorString)
            *errorString = tr("The qmake arguments have changed.");
        return MakefileIncompatible;
    }

    if (parse.config() != qs->deducedArguments()) {
        qCDebug(logs) << "**Mismatched config";
        if (errorString)
            *errorString = tr("The qmake arguments have changed.");
        return MakefileIncompatible;
    }

    // Specs match exactly
    if (actualSpec == parsedSpec) {
        qCDebug(logs) << "**Matched specs (1)";
        return MakefileMatches;
    }
    // Actual spec is the default one
//                    qDebug() << "AS vs VS" << actualSpec << version->mkspec();
    if ((actualSpec == version->mkspec() || actualSpec == FileName::fromLatin1("default"))
            && (parsedSpec == version->mkspec() || parsedSpec == FileName::fromLatin1("default") || parsedSpec.isEmpty())) {
        qCDebug(logs) << "**Matched specs (2)";
        return MakefileMatches;
    }

    qCDebug(logs) << "**Incompatible specs";
    if (errorString)
        *errorString = tr("The mkspec has changed.");
    return MakefileIncompatible;
}

FileName QmakeBuildConfiguration::extractSpecFromArguments(QString *args,
                                                         const QString &directory, const BaseQtVersion *version,
                                                         QStringList *outArgs)
{
    FileName parsedSpec;

    bool ignoreNext = false;
    bool nextIsSpec = false;
    for (QtcProcess::ArgIterator ait(args); ait.next(); ) {
        if (ignoreNext) {
            ignoreNext = false;
            ait.deleteArg();
        } else if (nextIsSpec) {
            nextIsSpec = false;
            parsedSpec = FileName::fromUserInput(ait.value());
            ait.deleteArg();
        } else if (ait.value() == QLatin1String("-spec") || ait.value() == QLatin1String("-platform")) {
            nextIsSpec = true;
            ait.deleteArg();
        } else if (ait.value() == QLatin1String("-cache")) {
            // We ignore -cache, because qmake contained a bug that it didn't
            // mention the -cache in the Makefile.
            // That means changing the -cache option in the additional arguments
            // does not automatically rerun qmake. Alas, we could try more
            // intelligent matching for -cache, but i guess people rarely
            // do use that.
            ignoreNext = true;
            ait.deleteArg();
        } else if (outArgs && ait.isSimple()) {
            outArgs->append(ait.value());
        }
    }

    if (parsedSpec.isEmpty())
        return FileName();

    FileName baseMkspecDir = FileName::fromUserInput(
            version->qmakeProperty("QT_HOST_DATA") + QLatin1String("/mkspecs"));
    baseMkspecDir = FileName::fromString(baseMkspecDir.toFileInfo().canonicalFilePath());

    // if the path is relative it can be
    // relative to the working directory (as found in the Makefiles)
    // or relatively to the mkspec directory
    // if it is the former we need to get the canonical form
    // for the other one we don't need to do anything
    if (parsedSpec.toFileInfo().isRelative()) {
        if (QFileInfo::exists(directory + QLatin1Char('/') + parsedSpec.toString()))
            parsedSpec = FileName::fromUserInput(directory + QLatin1Char('/') + parsedSpec.toString());
        else
            parsedSpec = FileName::fromUserInput(baseMkspecDir.toString() + QLatin1Char('/') + parsedSpec.toString());
    }

    QFileInfo f2 = parsedSpec.toFileInfo();
    while (f2.isSymLink()) {
        parsedSpec = FileName::fromString(f2.symLinkTarget());
        f2.setFile(parsedSpec.toString());
    }

    if (parsedSpec.isChildOf(baseMkspecDir)) {
        parsedSpec = parsedSpec.relativeChildPath(baseMkspecDir);
    } else {
        FileName sourceMkSpecPath = FileName::fromString(version->sourcePath().toString()
                                                         + QLatin1String("/mkspecs"));
        if (parsedSpec.isChildOf(sourceMkSpecPath))
            parsedSpec = parsedSpec.relativeChildPath(sourceMkSpecPath);
    }
    return parsedSpec;
}

bool QmakeBuildConfiguration::isEnabled() const
{
    return m_isEnabled;
}

QString QmakeBuildConfiguration::disabledReason() const
{
    if (!m_isEnabled)
        return tr("Parsing the .pro file");
    return QString();
}

void QmakeBuildConfiguration::setEnabled(bool enabled)
{
    if (m_isEnabled == enabled)
        return;
    m_isEnabled = enabled;
    emit enabledChanged();
}

/*!
  \class QmakeBuildConfigurationFactory
*/

QmakeBuildConfigurationFactory::QmakeBuildConfigurationFactory()
{
    registerBuildConfiguration<QmakeBuildConfiguration>(Constants::QMAKE_BC_ID);
    setSupportedProjectType(Constants::QMAKEPROJECT_ID);
    setSupportedProjectMimeTypeName(Constants::PROFILE_MIMETYPE);
    setIssueReporter([](Kit *k, const QString &projectPath, const QString &buildDir) {
        QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(k);
        QList<Task> issues;
        if (version)
            issues << version->reportIssues(projectPath, buildDir);

        QString tmpBuildDir = QDir(buildDir).absolutePath();
        const QChar slash = QLatin1Char('/');
        if (!tmpBuildDir.endsWith(slash))
            tmpBuildDir.append(slash);
        QString sourcePath = QFileInfo(projectPath).absolutePath();
        if (!sourcePath.endsWith(slash))
            sourcePath.append(slash);
        if (tmpBuildDir.count(slash) != sourcePath.count(slash)) {
            const QString msg = QCoreApplication::translate("QmakeProjectManager::QtVersion",
                                                            "The build directory needs to be at the same level as the source directory.");

            issues.append(Task(Task::Warning, msg, Utils::FileName(), -1,
                               ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        }
        return issues;
    });
}

BuildInfo QmakeBuildConfigurationFactory::createBuildInfo(const Kit *k,
                                                          const QString &projectPath,
                                                          BuildConfiguration::BuildType type) const
{
    BaseQtVersion *version = QtKitInformation::qtVersion(k);
    QmakeExtraBuildInfo extraInfo;
    BuildInfo info(this);
    QString suffix;
    if (type == BuildConfiguration::Release) {
        //: The name of the release build configuration created by default for a qmake project.
        info.displayName = tr("Release");
        //: Non-ASCII characters in directory suffix may cause build issues.
        suffix = tr("Release", "Shadow build directory suffix");
        if (version && version->isQtQuickCompilerSupported())
            extraInfo.config.useQtQuickCompiler = true;
    } else {
        if (type == BuildConfiguration::Debug) {
            //: The name of the debug build configuration created by default for a qmake project.
            info.displayName = tr("Debug");
            //: Non-ASCII characters in directory suffix may cause build issues.
            suffix = tr("Debug", "Shadow build directory suffix");
        } else if (type == BuildConfiguration::Profile) {
            //: The name of the profile build configuration created by default for a qmake project.
            info.displayName = tr("Profile");
            //: Non-ASCII characters in directory suffix may cause build issues.
            suffix = tr("Profile", "Shadow build directory suffix");
            extraInfo.config.separateDebugInfo = true;
            if (version && version->isQtQuickCompilerSupported())
                extraInfo.config.useQtQuickCompiler = true;
        }
        if (version && version->isQmlDebuggingSupported())
            extraInfo.config.linkQmlDebuggingQQ2 = true;
    }
    info.typeName = info.displayName;
    // Leave info.buildDirectory unset;
    info.kitId = k->id();

    // check if this project is in the source directory:
    FileName projectFilePath = FileName::fromString(projectPath);
    if (version && version->isInSourceDirectory(projectFilePath)) {
        // assemble build directory
        QString projectDirectory = projectFilePath.toFileInfo().absolutePath();
        QDir qtSourceDir = QDir(version->sourcePath().toString());
        QString relativeProjectPath = qtSourceDir.relativeFilePath(projectDirectory);
        QString qtBuildDir = version->qmakeProperty("QT_INSTALL_PREFIX");
        QString absoluteBuildPath = QDir::cleanPath(qtBuildDir + QLatin1Char('/') + relativeProjectPath);

        info.buildDirectory = FileName::fromString(absoluteBuildPath);
    } else {
        info.buildDirectory = defaultBuildDirectory(projectPath, k, suffix, type);
    }
    info.buildType = type;
    info.extraInfo = QVariant::fromValue(extraInfo);
    return info;
}

static QList<BuildConfiguration::BuildType> availableBuildTypes(const BaseQtVersion *version)
{
    QList<BuildConfiguration::BuildType> types = {BuildConfiguration::Debug,
                                                  BuildConfiguration::Release};
    if (version && version->qtVersion().majorVersion > 4)
        types << BuildConfiguration::Profile;
    return types;
}

QList<BuildInfo> QmakeBuildConfigurationFactory::availableBuilds(const Target *parent) const
{
    QList<BuildInfo> result;

    const QString projectFilePath = parent->project()->projectFilePath().toString();

    foreach (BuildConfiguration::BuildType buildType,
             availableBuildTypes(QtKitInformation::qtVersion(parent->kit()))) {
        BuildInfo info = createBuildInfo(parent->kit(), projectFilePath, buildType);
        info.displayName.clear(); // ask for a name
        info.buildDirectory.clear(); // This depends on the displayName
        result << info;
    }

    return result;
}

QList<BuildInfo> QmakeBuildConfigurationFactory::availableSetups(const Kit *k, const QString &projectPath) const
{
    QList<BuildInfo> result;
    BaseQtVersion *qtVersion = QtKitInformation::qtVersion(k);
    if (!qtVersion || !qtVersion->isValid())
        return result;

    foreach (BuildConfiguration::BuildType buildType, availableBuildTypes(qtVersion))
        result << createBuildInfo(k, projectPath, buildType);

    return result;
}

BuildConfiguration::BuildType QmakeBuildConfiguration::buildType() const
{
    QMakeStep *qs = qmakeStep();
    if (qmakeBuildConfiguration() & BaseQtVersion::DebugBuild)
        return Debug;
    else if (qs && qs->separateDebugInfo())
        return Profile;
    else
        return Release;
}

void QmakeBuildConfiguration::addToEnvironment(Environment &env) const
{
    setupBuildEnvironment(target()->kit(), env);
}

void QmakeBuildConfiguration::setupBuildEnvironment(Kit *k, Environment &env)
{
    prependCompilerPathToEnvironment(k, env);
    const BaseQtVersion *qt = QtKitInformation::qtVersion(k);
    if (qt && !qt->binPath().isEmpty())
        env.prependOrSetPath(qt->binPath().toString());
}

QmakeBuildConfiguration::LastKitState::LastKitState() = default;

QmakeBuildConfiguration::LastKitState::LastKitState(Kit *k)
    : m_qtVersion(QtKitInformation::qtVersionId(k)),
      m_sysroot(SysRootKitInformation::sysRoot(k).toString()),
      m_mkspec(QmakeKitInformation::mkspec(k).toString())
{
    ToolChain *tc = ToolChainKitInformation::toolChain(k, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    m_toolchain = tc ? tc->id() : QByteArray();
}

bool QmakeBuildConfiguration::LastKitState::operator ==(const LastKitState &other) const
{
    return m_qtVersion == other.m_qtVersion
            && m_toolchain == other.m_toolchain
            && m_sysroot == other.m_sysroot
            && m_mkspec == other.m_mkspec;
}

bool QmakeBuildConfiguration::LastKitState::operator !=(const LastKitState &other) const
{
    return !operator ==(other);
}

bool QmakeBuildConfiguration::regenerateBuildFiles(Node *node)
{
    QMakeStep *qs = qmakeStep();
    if (!qs)
        return false;

    qs->setForced(true);

    BuildManager::buildList(stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN));
    BuildManager::appendStep(qs, ProjectExplorerPlugin::displayNameForStepId(ProjectExplorer::Constants::BUILDSTEPS_CLEAN));

    QmakeProFileNode *proFile = nullptr;
    if (node && node != target()->project()->rootProjectNode())
        proFile = dynamic_cast<QmakeProFileNode *>(node);

    setSubNodeBuild(proFile);

    return true;
}

} // namespace QmakeProjectManager
