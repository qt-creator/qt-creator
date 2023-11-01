// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmakebuildconfiguration.h"

#include "makefileparse.h"
#include "qmakebuildconfiguration.h"
#include "qmakebuildinfo.h"
#include "qmakekitaspect.h"
#include "qmakenodes.h"
#include "qmakeproject.h"
#include "qmakeprojectmanagerconstants.h"
#include "qmakeprojectmanagertr.h"
#include "qmakesettings.h"
#include "qmakestep.h"

#include <android/androidconstants.h>

#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>

#include <projectexplorer/buildaspects.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildpropertiessettings.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/makestep.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorertr.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <qtsupport/qtbuildaspects.h>
#include <qtsupport/qtkitaspect.h>
#include <qtsupport/qtversionmanager.h>

#include <utils/process.h>
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

QmakeExtraBuildInfo::QmakeExtraBuildInfo()
{
    const BuildPropertiesSettings &settings = buildPropertiesSettings();
    config.separateDebugInfo = settings.separateDebugInfo();
    config.linkQmlDebuggingQQ2 = settings.qmlDebugging();
    config.useQtQuickCompiler = settings.qtQuickCompiler();
}

// --------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------

FilePath QmakeBuildConfiguration::shadowBuildDirectory(const FilePath &proFilePath, const Kit *k,
                                                       const QString &suffix,
                                                       BuildConfiguration::BuildType buildType)
{
    if (proFilePath.isEmpty())
        return {};

    const QString projectName = proFilePath.completeBaseName();
    return buildDirectoryFromTemplate(Project::projectDirectory(proFilePath), proFilePath,
                                      projectName, k, suffix, buildType, "qmake");
}

const char BUILD_CONFIGURATION_KEY[] = "Qt4ProjectManager.Qt4BuildConfiguration.BuildConfiguration";

QmakeBuildConfiguration::QmakeBuildConfiguration(Target *target, Id id)
    : BuildConfiguration(target, id)
{
    setConfigWidgetDisplayName(Tr::tr("General"));
    setConfigWidgetHasFrame(true);

    m_buildSystem = new QmakeBuildSystem(this);

    appendInitialBuildStep(Constants::QMAKE_BS_ID);
    appendInitialBuildStep(Constants::MAKESTEP_BS_ID);
    appendInitialCleanStep(Constants::MAKESTEP_BS_ID);

    setInitializer([this, target](const BuildInfo &info) {
        QMakeStep *qmakeStep = buildSteps()->firstOfType<QMakeStep>();
        QTC_ASSERT(qmakeStep, return);

        const QmakeExtraBuildInfo qmakeExtra = info.extraInfo.value<QmakeExtraBuildInfo>();
        QtVersion *version = QtKitAspect::qtVersion(target->kit());

        QtVersion::QmakeBuildConfigs config = version->defaultBuildConfig();
        if (info.buildType == BuildConfiguration::Debug)
            config |= QtVersion::DebugBuild;
        else
            config &= ~QtVersion::DebugBuild;

        QString additionalArguments = qmakeExtra.additionalArguments;
        if (!additionalArguments.isEmpty())
            qmakeStep->userArguments.setArguments(additionalArguments);

        separateDebugInfo.setValue(qmakeExtra.config.separateDebugInfo);
        qmlDebugging.setValue(qmakeExtra.config.linkQmlDebuggingQQ2);
        useQtQuickCompiler.setValue(qmakeExtra.config.useQtQuickCompiler);

        setQMakeBuildConfiguration(config);

        FilePath directory = info.buildDirectory;
        if (directory.isEmpty()) {
            directory = shadowBuildDirectory(target->project()->projectFilePath(),
                                             target->kit(), info.displayName,
                                             info.buildType);
        }

        setBuildDirectory(directory);

        if (DeviceTypeKitAspect::deviceTypeId(target->kit())
                        == Android::Constants::ANDROID_DEVICE_TYPE) {
            buildSteps()->appendStep(Android::Constants::ANDROID_PACKAGE_INSTALL_STEP_ID);
            buildSteps()->appendStep(Android::Constants::ANDROID_BUILD_APK_ID);
        }

        updateCacheAndEmitEnvironmentChanged();
    });

    connect(target, &Target::kitChanged,
            this, &QmakeBuildConfiguration::kitChanged);
    MacroExpander *expander = macroExpander();
    expander->registerVariable("Qmake:Makefile", "Qmake makefile", [this]() -> QString {
        const FilePath file = makefile();
        if (!file.isEmpty())
            return file.path();
        return QLatin1String("Makefile");
    });

    buildDirectoryAspect()->allowInSourceBuilds(target->project()->projectDirectory());
    connect(this, &BuildConfiguration::buildDirectoryInitialized,
            this, &QmakeBuildConfiguration::updateProblemLabel);
    connect(this, &BuildConfiguration::buildDirectoryChanged,
            this, &QmakeBuildConfiguration::updateProblemLabel);
    connect(this, &QmakeBuildConfiguration::qmakeBuildConfigurationChanged,
            this, &QmakeBuildConfiguration::updateProblemLabel);
    connect(&settings(), &AspectContainer::changed,
            this, &QmakeBuildConfiguration::updateProblemLabel);
    connect(target, &Target::parsingFinished, this, &QmakeBuildConfiguration::updateProblemLabel);
    connect(target, &Target::kitChanged, this, &QmakeBuildConfiguration::updateProblemLabel);

    connect(&separateDebugInfo, &BaseAspect::changed, this, [this] {
        emit separateDebugInfoChanged();
        emit qmakeBuildConfigurationChanged();
        qmakeBuildSystem()->scheduleUpdateAllNowOrLater();
    });

    qmlDebugging.setBuildConfiguration(this);
    connect(&qmlDebugging, &BaseAspect::changed, this, [this] {
        emit qmlDebuggingChanged();
        emit qmakeBuildConfigurationChanged();
        qmakeBuildSystem()->scheduleUpdateAllNowOrLater();
    });

    useQtQuickCompiler.setBuildConfiguration(this);
    connect(&useQtQuickCompiler, &QtQuickCompilerAspect::changed, this, [this] {
        emit useQtQuickCompilerChanged();
        emit qmakeBuildConfigurationChanged();
        qmakeBuildSystem()->scheduleUpdateAllNowOrLater();
    });

    runSystemFunctions.setSettingsKey("RunSystemFunction");
    runSystemFunctions.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    runSystemFunctions.setDisplayName(Tr::tr("qmake system() behavior when parsing:"));
    runSystemFunctions.addOption(Tr::tr("Run"));
    runSystemFunctions.addOption(Tr::tr("Ignore"));
    runSystemFunctions.addOption(Tr::tr("Use global setting"));
    runSystemFunctions.setDefaultValue(2);
}

QmakeBuildConfiguration::~QmakeBuildConfiguration()
{
    delete m_buildSystem;
}

void QmakeBuildConfiguration::toMap(Store &map) const
{
    BuildConfiguration::toMap(map);
    map.insert(BUILD_CONFIGURATION_KEY, int(m_qmakeBuildConfiguration));
}

void QmakeBuildConfiguration::fromMap(const Store &map)
{
    BuildConfiguration::fromMap(map);
    if (hasError())
        return;

    m_qmakeBuildConfiguration = QtVersion::QmakeBuildConfigs(map.value(BUILD_CONFIGURATION_KEY).toInt());

    m_lastKitState = LastKitState(kit());
}

void QmakeBuildConfiguration::kitChanged()
{
    LastKitState newState = LastKitState(kit());
    if (newState != m_lastKitState) {
        // This only checks if the ids have changed!
        // For that reason the QmakeBuildConfiguration is also connected
        // to the toolchain and qtversion managers
        m_buildSystem->scheduleUpdateAllNowOrLater();
        m_lastKitState = newState;
    }
}

void QmakeBuildConfiguration::updateProblemLabel()
{
    ProjectExplorer::Kit * const k = kit();

    // Check for Qt version:
    QtSupport::QtVersion *version = QtSupport::QtKitAspect::qtVersion(k);
    if (!version) {
        buildDirectoryAspect()->setProblem(Tr::tr("This kit cannot build this project since it "
                                                  "does not define a Qt version."));
        return;
    }

    const auto bs = qmakeBuildSystem();
    if (QmakeProFile *rootProFile = bs->rootProFile()) {
        if (rootProFile->parseInProgress() || !rootProFile->validParse()) {
            buildDirectoryAspect()->setProblem({});
            return;
        }
    }

    bool targetMismatch = false;
    bool incompatibleBuild = false;
    bool allGood = false;
    // we only show if we actually have a qmake and makestep
    QString errorString;
    if (qmakeStep() && makeStep()) {
        const QString makeFile = this->makefile().isEmpty() ? "Makefile" : makefile().path();
        switch (compareToImportFrom(buildDirectory() / makeFile, &errorString)) {
        case QmakeBuildConfiguration::MakefileMatches:
            allGood = true;
            break;
        case QmakeBuildConfiguration::MakefileMissing:
            allGood = true;
            break;
        case QmakeBuildConfiguration::MakefileIncompatible:
            incompatibleBuild = true;
            break;
        case QmakeBuildConfiguration::MakefileForWrongProject:
            targetMismatch = true;
            break;
        }
    }

    const bool unalignedBuildDir = settings().warnAgainstUnalignedBuildDir()
            && !isBuildDirAtSafeLocation();
    if (unalignedBuildDir)
        allGood = false;

    if (allGood) {
        const Tasks issues = Utils::sorted(
            version->reportIssues(project()->projectFilePath(), buildDirectory()));
        if (!issues.isEmpty()) {
            QString text = QLatin1String("<nobr>");
            for (const ProjectExplorer::Task &task : issues) {
                QString type;
                switch (task.type) {
                case ProjectExplorer::Task::Error:
                    type = Tr::tr("Error:");
                    type += QLatin1Char(' ');
                    break;
                case ProjectExplorer::Task::Warning:
                    type = Tr::tr("Warning:");
                    type += QLatin1Char(' ');
                    break;
                case ProjectExplorer::Task::Unknown:
                default:
                    break;
                }
                if (!text.endsWith(QLatin1String("br>")))
                    text.append(QLatin1String("<br>"));
                text.append(type + task.description());
            }
            buildDirectoryAspect()->setProblem(text);
            return;
        }
    } else if (targetMismatch) {
        buildDirectoryAspect()->setProblem(Tr::tr("The build directory contains a build for "
                                                  "a different project, which will be overwritten."));
        return;
    } else if (incompatibleBuild) {
        buildDirectoryAspect()->setProblem(Tr::tr("%1 The build will be overwritten.",
                                                  "%1 error message")
                                           .arg(errorString));
        return;
    } else if (unalignedBuildDir) {
        buildDirectoryAspect()->setProblem(unalignedBuildDirWarning());
        return;
    }

    buildDirectoryAspect()->setProblem({});
}

BuildSystem *QmakeBuildConfiguration::buildSystem() const
{
    return m_buildSystem;
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

FilePath QmakeBuildConfiguration::makefile() const
{
    return FilePath::fromString(m_buildSystem->rootProFile()->singleVariableValue(Variable::Makefile));
}

QtVersion::QmakeBuildConfigs QmakeBuildConfiguration::qmakeBuildConfiguration() const
{
    return m_qmakeBuildConfiguration;
}

void QmakeBuildConfiguration::setQMakeBuildConfiguration(QtVersion::QmakeBuildConfigs config)
{
    if (m_qmakeBuildConfiguration == config)
        return;
    m_qmakeBuildConfiguration = config;

    emit qmakeBuildConfigurationChanged();
    m_buildSystem->scheduleUpdateAllNowOrLater();
    emit buildTypeChanged();
}

QString QmakeBuildConfiguration::unalignedBuildDirWarning()
{
    return Tr::tr("The build directory should be at the same level as the source directory.");
}

bool QmakeBuildConfiguration::isBuildDirAtSafeLocation(const FilePath &sourceDir,
                                                       const FilePath &buildDir)
{
    return buildDir.path().count('/') == sourceDir.path().count('/');
}

bool QmakeBuildConfiguration::isBuildDirAtSafeLocation() const
{
    return isBuildDirAtSafeLocation(project()->projectDirectory(), buildDirectory());
}

void QmakeBuildConfiguration::forceSeparateDebugInfo(bool sepDebugInfo)
{
    separateDebugInfo.setValue(sepDebugInfo ? TriState::Enabled : TriState::Disabled);
}

void QmakeBuildConfiguration::forceQmlDebugging(bool enable)
{
    qmlDebugging.setValue(enable ? TriState::Enabled : TriState::Disabled);
}

void QmakeBuildConfiguration::forceQtQuickCompiler(bool enable)
{
    useQtQuickCompiler.setValue(enable ? TriState::Enabled : TriState::Disabled);
}

bool QmakeBuildConfiguration::runQmakeSystemFunctions() const
{
    const int sel = runSystemFunctions();
    if (sel == 0)
        return true;
    if (sel == 1)
        return false;
    return settings().runSystemFunction();
}

QStringList QmakeBuildConfiguration::configCommandLineArguments() const
{
    QStringList result;
    QtVersion *version = QtKitAspect::qtVersion(kit());
    QtVersion::QmakeBuildConfigs defaultBuildConfiguration =
            version ? version->defaultBuildConfig() : QtVersion::QmakeBuildConfigs(QtVersion::DebugBuild | QtVersion::BuildAll);
    QtVersion::QmakeBuildConfigs userBuildConfiguration = m_qmakeBuildConfiguration;
    if ((defaultBuildConfiguration & QtVersion::BuildAll) && !(userBuildConfiguration & QtVersion::BuildAll))
        result << QLatin1String("CONFIG-=debug_and_release");

    if (!(defaultBuildConfiguration & QtVersion::BuildAll) && (userBuildConfiguration & QtVersion::BuildAll))
        result << QLatin1String("CONFIG+=debug_and_release");
    if ((defaultBuildConfiguration & QtVersion::DebugBuild) && !(userBuildConfiguration & QtVersion::DebugBuild))
        result << QLatin1String("CONFIG+=release");
    if (!(defaultBuildConfiguration & QtVersion::DebugBuild) && (userBuildConfiguration & QtVersion::DebugBuild))
        result << QLatin1String("CONFIG+=debug");
    return result;
}

QMakeStep *QmakeBuildConfiguration::qmakeStep() const
{
    QMakeStep *qs = nullptr;
    BuildStepList *bsl = buildSteps();
    for (int i = 0; i < bsl->count(); ++i)
        if ((qs = qobject_cast<QMakeStep *>(bsl->at(i))) != nullptr)
            return qs;
    return nullptr;
}

MakeStep *QmakeBuildConfiguration::makeStep() const
{
    MakeStep *ms = nullptr;
    BuildStepList *bsl = buildSteps();
    for (int i = 0; i < bsl->count(); ++i)
        if ((ms = qobject_cast<MakeStep *>(bsl->at(i))) != nullptr)
            return ms;
    return nullptr;
}

QmakeBuildSystem *QmakeBuildConfiguration::qmakeBuildSystem() const
{
    return m_buildSystem;
}

// Returns true if both are equal.
QmakeBuildConfiguration::MakefileState QmakeBuildConfiguration::compareToImportFrom(const FilePath &makefile, QString *errorString)
{
    const QLoggingCategory &logs = MakeFileParse::logging();
    qCDebug(logs) << "QMakeBuildConfiguration::compareToImport";

    QMakeStep *qs = qmakeStep();
    MakeFileParse parse(makefile, MakeFileParse::Mode::DoNotFilterKnownConfigValues);

    if (parse.makeFileState() == MakeFileParse::MakefileMissing) {
        qCDebug(logs) << "**Makefile missing";
        return MakefileMissing;
    }
    if (parse.makeFileState() == MakeFileParse::CouldNotParse) {
        qCDebug(logs) << "**Makefile incompatible";
        if (errorString)
            *errorString = Tr::tr("Could not parse Makefile.");
        return MakefileIncompatible;
    }

    if (!qs) {
        qCDebug(logs) << "**No qmake step";
        return MakefileMissing;
    }

    QtVersion *version = QtKitAspect::qtVersion(kit());
    if (!version) {
        qCDebug(logs) << "**No qt version in kit";
        return MakefileForWrongProject;
    }

    const FilePath projectPath =
            m_subNodeBuild ? m_subNodeBuild->filePath() : qs->project()->projectFilePath();
    if (parse.srcProFile() != projectPath) {
        qCDebug(logs) << "**Different profile used to generate the Makefile:"
                      << parse.srcProFile() << " expected profile:" << projectPath;
        if (errorString)
            *errorString = Tr::tr("The Makefile is for a different project.");
        return MakefileIncompatible;
    }

    if (version->qmakeFilePath() != parse.qmakePath()) {
        qCDebug(logs) << "**Different Qt versions, buildconfiguration:" << version->qmakeFilePath()
                      << " Makefile:" << parse.qmakePath();
        return MakefileForWrongProject;
    }

    // same qtversion
    QtVersion::QmakeBuildConfigs buildConfig = parse.effectiveBuildConfig(version->defaultBuildConfig());
    if (qmakeBuildConfiguration() != buildConfig) {
        qCDebug(logs) << "**Different qmake buildconfigurations buildconfiguration:"
                      << qmakeBuildConfiguration() << " Makefile:" << buildConfig;
        if (errorString)
            *errorString = Tr::tr("The build type has changed.");
        return MakefileIncompatible;
    }

    // The qmake Build Configuration are the same,
    // now compare arguments lists
    // we have to compare without the spec/platform cmd argument
    // and compare that on its own
    FilePath workingDirectory = makefile.parentDir();
    QStringList actualArgs;
    QString allArgs = macroExpander()->expandProcessArgs(qs->allArguments(
        QtKitAspect::qtVersion(target()->kit()), QMakeStep::ArgumentFlag::Expand));
    // This copies the settings from allArgs to actualArgs (minus some we
    // are not interested in), splitting them up into individual strings:
    extractSpecFromArguments(&allArgs, workingDirectory, version, &actualArgs);
    actualArgs.removeFirst(); // Project file.
    const QString actualSpec = qs->mkspec();

    QString qmakeArgs = parse.unparsedArguments();
    QStringList parsedArgs;
    QString parsedSpec =
            extractSpecFromArguments(&qmakeArgs, workingDirectory, version, &parsedArgs);

    qCDebug(logs) << "  Actual args:" << actualArgs;
    qCDebug(logs) << "  Parsed args:" << parsedArgs;
    qCDebug(logs) << "  Actual spec:" << actualSpec;
    qCDebug(logs) << "  Parsed spec:" << parsedSpec;
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
            *errorString = Tr::tr("The qmake arguments have changed.");
        return MakefileIncompatible;
    }

    if (parse.config() != qs->deducedArguments()) {
        qCDebug(logs) << "**Mismatched config";
        if (errorString)
            *errorString = Tr::tr("The qmake arguments have changed.");
        return MakefileIncompatible;
    }

    // Specs match exactly
    if (actualSpec == parsedSpec) {
        qCDebug(logs) << "**Matched specs (1)";
        return MakefileMatches;
    }
    // Actual spec is the default one
//                    qDebug() << "AS vs VS" << actualSpec << version->mkspec();
    if ((actualSpec == version->mkspec() || actualSpec == "default")
            && (parsedSpec == version->mkspec() || parsedSpec == "default" || parsedSpec.isEmpty())) {
        qCDebug(logs) << "**Matched specs (2)";
        return MakefileMatches;
    }

    qCDebug(logs) << "**Incompatible specs";
    if (errorString)
        *errorString = Tr::tr("The mkspec has changed.");
    return MakefileIncompatible;
}

QString QmakeBuildConfiguration::extractSpecFromArguments(QString *args,
                                                         const FilePath &directory,
                                                         const QtVersion *version,
                                                         QStringList *outArgs)
{
    FilePath parsedSpec;

    bool ignoreNext = false;
    bool nextIsSpec = false;
    for (ProcessArgs::ArgIterator ait(args); ait.next(); ) {
        if (ignoreNext) {
            ignoreNext = false;
            ait.deleteArg();
        } else if (nextIsSpec) {
            nextIsSpec = false;
            parsedSpec = FilePath::fromUserInput(ait.value());
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
        return {};

    const FilePath baseMkspecDir = version->hostDataPath().pathAppended("mkspecs")
        .canonicalPath();

    // if the path is relative it can be
    // relative to the working directory (as found in the Makefiles)
    // or relatively to the mkspec directory
    // if it is the former we need to get the canonical form
    // for the other one we don't need to do anything
    if (parsedSpec.isRelativePath()) {
        FilePath mkspecs = directory.pathAppended(parsedSpec.path());
        if (mkspecs.exists())
            parsedSpec = mkspecs;
        else
            parsedSpec = baseMkspecDir.pathAppended(parsedSpec.path());
    }

    for (int i = 0; i < 5 && parsedSpec.isSymLink(); ++i)
        parsedSpec = parsedSpec.symLinkTarget();

    if (parsedSpec.isChildOf(baseMkspecDir)) {
        parsedSpec = parsedSpec.relativeChildPath(baseMkspecDir);
    } else {
        FilePath sourceMkSpecPath = version->sourcePath().pathAppended("mkspecs");
        if (parsedSpec.isChildOf(sourceMkSpecPath))
            parsedSpec = parsedSpec.relativeChildPath(sourceMkSpecPath);
    }
    return parsedSpec.toString();
}

/*!
  \class QmakeBuildConfigurationFactory
*/

static BuildInfo createBuildInfo(const Kit *k, const FilePath &projectPath,
                 BuildConfiguration::BuildType type)
{
    const BuildPropertiesSettings &settings = buildPropertiesSettings();
    QtVersion *version = QtKitAspect::qtVersion(k);
    QmakeExtraBuildInfo extraInfo;
    BuildInfo info;
    QString suffix;

    if (type == BuildConfiguration::Release) {
        //: The name of the release build configuration created by default for a qmake project.
        info.displayName = ::ProjectExplorer::Tr::tr("Release");
        //: Non-ASCII characters in directory suffix may cause build issues.
        suffix = Tr::tr("Release", "Shadow build directory suffix");
        if (settings.qtQuickCompiler() == TriState::Default) {
            if (version && version->isQtQuickCompilerSupported())
                extraInfo.config.useQtQuickCompiler = TriState::Enabled;
        }
    } else {
        if (type == BuildConfiguration::Debug) {
            //: The name of the debug build configuration created by default for a qmake project.
            info.displayName = ::ProjectExplorer::Tr::tr("Debug");
            //: Non-ASCII characters in directory suffix may cause build issues.
            suffix = Tr::tr("Debug", "Shadow build directory suffix");
        } else if (type == BuildConfiguration::Profile) {
            //: The name of the profile build configuration created by default for a qmake project.
            info.displayName = ::ProjectExplorer::Tr::tr("Profile");
            //: Non-ASCII characters in directory suffix may cause build issues.
            suffix = Tr::tr("Profile", "Shadow build directory suffix");
            if (settings.separateDebugInfo() == TriState::Default)
                extraInfo.config.separateDebugInfo = TriState::Enabled;

            if (settings.qtQuickCompiler() == TriState::Default) {
                if (version && version->isQtQuickCompilerSupported())
                    extraInfo.config.useQtQuickCompiler = TriState::Enabled;
            }
        }
        if (settings.qmlDebugging() == TriState::Default) {
            if (version && version->isQmlDebuggingSupported())
                extraInfo.config.linkQmlDebuggingQQ2 = TriState::Enabled;
        }
    }
    info.typeName = info.displayName;
    // Leave info.buildDirectory unset;

    // check if this project is in the source directory:
    if (version && version->isInQtSourceDirectory(projectPath)) {
        // assemble build directory
        QString projectDirectory = projectPath.toFileInfo().absolutePath();
        QDir qtSourceDir = QDir(version->sourcePath().toString());
        QString relativeProjectPath = qtSourceDir.relativeFilePath(projectDirectory);
        QString qtBuildDir = version->prefix().toString();
        QString absoluteBuildPath = QDir::cleanPath(qtBuildDir + QLatin1Char('/') + relativeProjectPath);

        info.buildDirectory = FilePath::fromString(absoluteBuildPath);
    } else {
        info.buildDirectory =
                QmakeBuildConfiguration::shadowBuildDirectory(projectPath, k, suffix, type);
    }
    info.buildType = type;
    info.extraInfo = QVariant::fromValue(extraInfo);
    return info;
}

QmakeBuildConfigurationFactory::QmakeBuildConfigurationFactory()
{
    registerBuildConfiguration<QmakeBuildConfiguration>(Constants::QMAKE_BC_ID);
    setSupportedProjectType(Constants::QMAKEPROJECT_ID);
    setSupportedProjectMimeTypeName(Constants::PROFILE_MIMETYPE);
    setIssueReporter([](Kit *k, const FilePath &projectPath, const FilePath &buildDir) {
        QtSupport::QtVersion *version = QtSupport::QtKitAspect::qtVersion(k);
        Tasks issues;
        if (version)
            issues << version->reportIssues(projectPath, buildDir);
        if (settings().warnAgainstUnalignedBuildDir()
                && !QmakeBuildConfiguration::isBuildDirAtSafeLocation(
                    projectPath.absolutePath(), buildDir.absoluteFilePath())) {
            issues.append(BuildSystemTask(Task::Warning,
                                          QmakeBuildConfiguration::unalignedBuildDirWarning()));
        }
        return issues;
    });

    setBuildGenerator([](const Kit *k, const FilePath &projectPath, bool forSetup) {
        QList<BuildInfo> result;

        QtVersion *qtVersion = QtKitAspect::qtVersion(k);

        if (forSetup && (!qtVersion || !qtVersion->isValid()))
            return result;

        const auto addBuild = [&](BuildConfiguration::BuildType buildType) {
            BuildInfo info = createBuildInfo(k, projectPath, buildType);
            if (!forSetup) {
                info.displayName.clear(); // ask for a name
                info.buildDirectory.clear(); // This depends on the displayName
            }
            result << info;
        };

        addBuild(BuildConfiguration::Debug);
        addBuild(BuildConfiguration::Release);
        if (qtVersion && qtVersion->qtVersion().majorVersion() > 4)
            addBuild(BuildConfiguration::Profile);

        return result;
    });
}

BuildConfiguration::BuildType QmakeBuildConfiguration::buildType() const
{
    if (qmakeBuildConfiguration() & QtVersion::DebugBuild)
        return Debug;
    if (separateDebugInfo() == TriState::Enabled)
        return Profile;
    return Release;
}

void QmakeBuildConfiguration::addToEnvironment(Environment &env) const
{
    QtSupport::QtKitAspect::addHostBinariesToPath(kit(), env);
}

QmakeBuildConfiguration::LastKitState::LastKitState() = default;

QmakeBuildConfiguration::LastKitState::LastKitState(Kit *k)
    : m_qtVersion(QtKitAspect::qtVersionId(k)),
      m_sysroot(SysRootKitAspect::sysRoot(k).toString()),
      m_mkspec(QmakeKitAspect::mkspec(k))
{
    ToolChain *tc = ToolChainKitAspect::cxxToolChain(k);
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

    BuildManager::buildList(cleanSteps());
    BuildManager::appendStep(qs, BuildManager::displayNameForStepId(ProjectExplorer::Constants::BUILDSTEPS_CLEAN));

    QmakeProFileNode *proFile = nullptr;
    if (node && node != project()->rootProjectNode())
        proFile = dynamic_cast<QmakeProFileNode *>(node);

    setSubNodeBuild(proFile);

    return true;
}

void QmakeBuildConfiguration::restrictNextBuild(const RunConfiguration *rc)
{
    if (!rc) {
        setSubNodeBuild(nullptr);
        return;
    }
    const auto productNode = dynamic_cast<QmakeProFileNode *>(rc->productNode());
    QTC_ASSERT(productNode, return);
    setSubNodeBuild(productNode);
}

} // namespace QmakeProjectManager
