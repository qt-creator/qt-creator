/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qt4buildconfiguration.h"

#include "qt4project.h"
#include "qt4projectconfigwidget.h"
#include "qt4projectmanagerconstants.h"
#include "qt4nodes.h"
#include "qmakestep.h"
#include "makestep.h"

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <limits>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/profileinformation.h>
#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/qtversionfactory.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtprofileinformation.h>
#include <qtsupport/qtversionmanager.h>
#include <QDebug>

#include <QInputDialog>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;
using namespace ProjectExplorer;

namespace {
const char * const QT4_BC_ID("Qt4ProjectManager.Qt4BuildConfiguration");

const char * const USE_SHADOW_BUILD_KEY("Qt4ProjectManager.Qt4BuildConfiguration.UseShadowBuild");
const char * const BUILD_DIRECTORY_KEY("Qt4ProjectManager.Qt4BuildConfiguration.BuildDirectory");
const char * const BUILD_CONFIGURATION_KEY("Qt4ProjectManager.Qt4BuildConfiguration.BuildConfiguration");

enum { debug = 0 };
}

Qt4BuildConfiguration::Qt4BuildConfiguration(Target *target) :
    BuildConfiguration(target, Core::Id(QT4_BC_ID)),
    m_shadowBuild(true),
    m_isEnabled(false),
    m_qmakeBuildConfiguration(0),
    m_subNodeBuild(0),
    m_fileNodeBuild(0)
{
    ctor();
}

Qt4BuildConfiguration::Qt4BuildConfiguration(Target *target, const Core::Id id) :
    BuildConfiguration(target, id),
    m_shadowBuild(true),
    m_isEnabled(false),
    m_qmakeBuildConfiguration(0),
    m_subNodeBuild(0),
    m_fileNodeBuild(0)
{
    ctor();
}

Qt4BuildConfiguration::Qt4BuildConfiguration(Target *target, Qt4BuildConfiguration *source) :
    BuildConfiguration(target, source),
    m_shadowBuild(source->m_shadowBuild),
    m_isEnabled(false),
    m_buildDirectory(source->m_buildDirectory),
    m_qmakeBuildConfiguration(source->m_qmakeBuildConfiguration),
    m_subNodeBuild(0), // temporary value, so not copied
    m_fileNodeBuild(0)
{
    cloneSteps(source);
    ctor();
}

Qt4BuildConfiguration::~Qt4BuildConfiguration()
{
}

QVariantMap Qt4BuildConfiguration::toMap() const
{
    QVariantMap map(BuildConfiguration::toMap());
    map.insert(QLatin1String(USE_SHADOW_BUILD_KEY), m_shadowBuild);
    map.insert(QLatin1String(BUILD_DIRECTORY_KEY), m_buildDirectory);
    map.insert(QLatin1String(BUILD_CONFIGURATION_KEY), int(m_qmakeBuildConfiguration));
    return map;
}

bool Qt4BuildConfiguration::fromMap(const QVariantMap &map)
{
    if (!BuildConfiguration::fromMap(map))
        return false;

    m_shadowBuild = map.value(QLatin1String(USE_SHADOW_BUILD_KEY), true).toBool();
    m_qmakeBuildConfiguration = QtSupport::BaseQtVersion::QmakeBuildConfigs(map.value(QLatin1String(BUILD_CONFIGURATION_KEY)).toInt());
    m_buildDirectory = map.value(QLatin1String(BUILD_DIRECTORY_KEY), defaultShadowBuildDirectory()).toString();

    m_lastEmmitedBuildDirectory = buildDirectory();
    m_qtVersionSupportsShadowBuilds =  supportsShadowBuilds();

    return true;
}

void Qt4BuildConfiguration::ctor()
{
    connect(this, SIGNAL(environmentChanged()),
            this, SLOT(emitBuildDirectoryChanged()));
    connect(this, SIGNAL(environmentChanged()),
            this, SLOT(emitEvaluateBuildSystem()));
    connect(target(), SIGNAL(profileChanged()),
            this, SLOT(profileChanged()));
}

void Qt4BuildConfiguration::profileChanged()
{
    // emit qtVersionChanged(); TODO what was connected to that
    emit requestBuildSystemEvaluation();
    emit environmentChanged();
    emitBuildDirectoryChanged();
}

void Qt4BuildConfiguration::emitBuildDirectoryChanged()
{
    // We also emit buildDirectoryChanged if the the qt version's supportShadowBuild changed
    if (buildDirectory() != m_lastEmmitedBuildDirectory
            || supportsShadowBuilds() != m_qtVersionSupportsShadowBuilds) {
        m_lastEmmitedBuildDirectory = buildDirectory();
        m_qtVersionSupportsShadowBuilds = supportsShadowBuilds();
        emit buildDirectoryChanged();
    }
}

Utils::Environment Qt4BuildConfiguration::baseEnvironment() const
{
    Utils::Environment env = BuildConfiguration::baseEnvironment();
    target()->profile()->addToEnvironment(env);
    return env;
}

BuildConfigWidget *Qt4BuildConfiguration::createConfigWidget()
{
    return new Qt4ProjectConfigWidget(target());
}

QString Qt4BuildConfiguration::defaultShadowBuildDirectory() const
{
    // todo displayName isn't ideal
    return Qt4Project::shadowBuildDirectory(target()->project()->document()->fileName(),
                                            target()->profile(), displayName());
}

/// returns the unexpanded build directory
QString Qt4BuildConfiguration::rawBuildDirectory() const
{
    QString workingDirectory;
    if (m_shadowBuild) {
        if (!m_buildDirectory.isEmpty())
            workingDirectory = m_buildDirectory;
        else
            workingDirectory = defaultShadowBuildDirectory();
    }
    if (workingDirectory.isEmpty())
        workingDirectory = target()->project()->projectDirectory();
    return workingDirectory;
}

/// returns the build directory
QString Qt4BuildConfiguration::buildDirectory() const
{
    return QDir::cleanPath(environment().expandVariables(rawBuildDirectory()));
}

bool Qt4BuildConfiguration::supportsShadowBuilds()
{
    QtSupport::BaseQtVersion *version = QtSupport::QtProfileInformation::qtVersion(target()->profile());
    return !version || version->supportsShadowBuilds();
}

/// If only a sub tree should be build this function returns which sub node
/// should be build
/// \see Qt4BuildConfiguration::setSubNodeBuild
Qt4ProjectManager::Qt4ProFileNode *Qt4BuildConfiguration::subNodeBuild() const
{
    return m_subNodeBuild;
}

/// A sub node build on builds a sub node of the project
/// That is triggered by a right click in the project explorer tree
/// The sub node to be build is set via this function immediately before
/// calling BuildManager::buildProject( BuildConfiguration * )
/// and reset immediately afterwards
/// That is m_subNodesBuild is set only temporarly
void Qt4BuildConfiguration::setSubNodeBuild(Qt4ProjectManager::Qt4ProFileNode *node)
{
    m_subNodeBuild = node;
}

FileNode *Qt4BuildConfiguration::fileNodeBuild() const
{
    return m_fileNodeBuild;
}

void Qt4BuildConfiguration::setFileNodeBuild(FileNode *node)
{
    m_fileNodeBuild = node;
}

/// returns whether this is a shadow build configuration or not
/// note, even if shadowBuild() returns true, it might be using the
/// source directory as the shadow build directory, thus it
/// still is a in-source build
bool Qt4BuildConfiguration::shadowBuild() const
{
    return m_shadowBuild;
}

/// returns the shadow build directory if set
/// \note buildDirectory() is probably the function you want to call
QString Qt4BuildConfiguration::shadowBuildDirectory() const
{
    if (m_buildDirectory.isEmpty())
        return defaultShadowBuildDirectory();
    return m_buildDirectory;
}

void Qt4BuildConfiguration::setShadowBuildAndDirectory(bool shadowBuild, const QString &buildDirectory)
{
    QtSupport::BaseQtVersion *version = QtSupport::QtProfileInformation::qtVersion(target()->profile());
    QString directoryToSet = buildDirectory;
    bool toSet = (shadowBuild && version && version->isValid() && version->supportsShadowBuilds());
    if (m_shadowBuild == toSet && m_buildDirectory == directoryToSet)
        return;

    m_shadowBuild = toSet;
    m_buildDirectory = directoryToSet;

    emit environmentChanged();
    emitBuildDirectoryChanged();
    emitEvaluateBuildSystem();
}

static inline QString symbianMakeTarget(QtSupport::BaseQtVersion::QmakeBuildConfigs buildConfig,
                                        const QString &type)
{
    QString rc = (buildConfig & QtSupport::BaseQtVersion::DebugBuild) ?
                 QLatin1String("debug-") : QLatin1String("release-");
    rc += type;
    return rc;
}

QString Qt4BuildConfiguration::defaultMakeTarget() const
{
    ToolChain *tc = ProjectExplorer::ToolChainProfileInformation::toolChain(target()->profile());
    QtSupport::BaseQtVersion *version = QtSupport::QtProfileInformation::qtVersion(target()->profile());
    if (!tc || !version || version->type() != QtSupport::Constants::SYMBIANQT)
        return QString();

    const QtSupport::BaseQtVersion::QmakeBuildConfigs buildConfig = qmakeBuildConfiguration();
    return symbianMakeTarget(buildConfig, tc->defaultMakeTarget());
}

QString Qt4BuildConfiguration::makefile() const
{
    return static_cast<Qt4Project *>(target()->project())->rootQt4ProjectNode()->makefile();
}

QtSupport::BaseQtVersion::QmakeBuildConfigs Qt4BuildConfiguration::qmakeBuildConfiguration() const
{
    return m_qmakeBuildConfiguration;
}

void Qt4BuildConfiguration::setQMakeBuildConfiguration(QtSupport::BaseQtVersion::QmakeBuildConfigs config)
{
    if (m_qmakeBuildConfiguration == config)
        return;
    m_qmakeBuildConfiguration = config;

    emitEvaluateBuildSystem();
    emit qmakeBuildConfigurationChanged();
    emitBuildDirectoryChanged();
}

void Qt4BuildConfiguration::emitEvaluateBuildSystem()
{
    emit requestBuildSystemEvaluation();
}

void Qt4BuildConfiguration::emitQMakeBuildConfigurationChanged()
{
    emit qmakeBuildConfigurationChanged();
}

void Qt4BuildConfiguration::emitBuildDirectoryInitialized()
{
    emit buildDirectoryInitialized();
}

void Qt4BuildConfiguration::emitS60CreatesSmartInstallerChanged()
{
    emit s60CreatesSmartInstallerChanged();
}

QStringList Qt4BuildConfiguration::configCommandLineArguments() const
{
    QStringList result;
    QtSupport::BaseQtVersion *version = QtSupport::QtProfileInformation::qtVersion(target()->profile());
    QtSupport::BaseQtVersion::QmakeBuildConfigs defaultBuildConfiguration =  version ? version->defaultBuildConfig() : (QtSupport::BaseQtVersion::DebugBuild | QtSupport::BaseQtVersion::BuildAll);
    QtSupport::BaseQtVersion::QmakeBuildConfigs userBuildConfiguration = m_qmakeBuildConfiguration;
    if ((defaultBuildConfiguration & QtSupport::BaseQtVersion::BuildAll) && !(userBuildConfiguration & QtSupport::BaseQtVersion::BuildAll))
        result << QLatin1String("CONFIG-=debug_and_release");

    if (!(defaultBuildConfiguration & QtSupport::BaseQtVersion::BuildAll) && (userBuildConfiguration & QtSupport::BaseQtVersion::BuildAll))
        result << QLatin1String("CONFIG+=debug_and_release");
    if ((defaultBuildConfiguration & QtSupport::BaseQtVersion::DebugBuild) && !(userBuildConfiguration & QtSupport::BaseQtVersion::DebugBuild))
        result << QLatin1String("CONFIG+=release");
    if (!(defaultBuildConfiguration & QtSupport::BaseQtVersion::DebugBuild) && (userBuildConfiguration & QtSupport::BaseQtVersion::DebugBuild))
        result << QLatin1String("CONFIG+=debug");
    return result;
}

QMakeStep *Qt4BuildConfiguration::qmakeStep() const
{
    QMakeStep *qs = 0;
    BuildStepList *bsl = stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD));
    Q_ASSERT(bsl);
    for (int i = 0; i < bsl->count(); ++i)
        if ((qs = qobject_cast<QMakeStep *>(bsl->at(i))) != 0)
            return qs;
    return 0;
}

MakeStep *Qt4BuildConfiguration::makeStep() const
{
    MakeStep *ms = 0;
    BuildStepList *bsl = stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD));
    Q_ASSERT(bsl);
    for (int i = 0; i < bsl->count(); ++i)
        if ((ms = qobject_cast<MakeStep *>(bsl->at(i))) != 0)
            return ms;
    return 0;
}

// returns true if both are equal
Qt4BuildConfiguration::MakefileState Qt4BuildConfiguration::compareToImportFrom(const QString &makefile)
{
    QMakeStep *qs = qmakeStep();
    if (QFileInfo(makefile).exists() && qs) {
        Utils::FileName qmakePath = QtSupport::QtVersionManager::findQMakeBinaryFromMakefile(makefile);
        QtSupport::BaseQtVersion *version = QtSupport::QtProfileInformation::qtVersion(target()->profile());
        if (!version)
            return MakefileForWrongProject;
        if (version->qmakeCommand() == qmakePath) {
            // same qtversion
            QPair<QtSupport::BaseQtVersion::QmakeBuildConfigs, QString> result =
                    QtSupport::QtVersionManager::scanMakeFile(makefile, version->defaultBuildConfig());
            if (qmakeBuildConfiguration() == result.first) {
                // The qmake Build Configuration are the same,
                // now compare arguments lists
                // we have to compare without the spec/platform cmd argument
                // and compare that on its own
                QString workingDirectory = QFileInfo(makefile).absolutePath();
                QStringList actualArgs;
                QString userArgs = qs->userArguments();
                // This copies the settings from userArgs to actualArgs (minus some we
                // are not interested in), splitting them up into individual strings:
                extractSpecFromArguments(&userArgs, workingDirectory, version, &actualArgs);
                actualArgs = qs->deducedArguments() + actualArgs + qs->deducedArgumentsAfter();
                Utils::FileName actualSpec = qs->mkspec();

                QString qmakeArgs = result.second;
                QStringList parsedArgs;
                Utils::FileName parsedSpec = extractSpecFromArguments(&qmakeArgs, workingDirectory, version, &parsedArgs);

                if (debug) {
                    qDebug()<<"Actual args:"<<actualArgs;
                    qDebug()<<"Parsed args:"<<parsedArgs;
                    qDebug()<<"Actual spec:"<<actualSpec.toString();
                    qDebug()<<"Parsed spec:"<<parsedSpec.toString();
                }

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
                if (actualArgs == parsedArgs) {
                    // Specs match exactly
                    if (actualSpec == parsedSpec)
                        return MakefileMatches;
                    // Actual spec is the default one
//                    qDebug()<<"AS vs VS"<<actualSpec<<version->mkspec();
                    if ((actualSpec == version->mkspec() || actualSpec == Utils::FileName::fromString(QLatin1String("default")))
                        && (parsedSpec == version->mkspec() || parsedSpec == Utils::FileName::fromString(QLatin1String("default")) || parsedSpec.isEmpty()))
                        return MakefileMatches;
                }
                return MakefileIncompatible;
            } else {
                if (debug)
                    qDebug()<<"different qmake buildconfigurations buildconfiguration:"<<qmakeBuildConfiguration()<<" Makefile:"<<result.first;
                return MakefileIncompatible;
            }
        } else {
            if (debug)
                qDebug()<<"diffrent qt versions, buildconfiguration:"<<version->qmakeCommand().toString()<<" Makefile:"<<qmakePath.toString();
            return MakefileForWrongProject;
        }
    }
    return MakefileMissing;
}

bool Qt4BuildConfiguration::removeQMLInspectorFromArguments(QString *args)
{
    bool removedArgument = false;
    for (Utils::QtcProcess::ArgIterator ait(args); ait.next(); ) {
        const QString arg = ait.value();
        if (arg.contains(QLatin1String(Constants::QMAKEVAR_QMLJSDEBUGGER_PATH))
            || arg.contains(QLatin1String(Constants::QMAKEVAR_DECLARATIVE_DEBUG4))
            || arg.contains(QLatin1String(Constants::QMAKEVAR_DECLARATIVE_DEBUG5))) {
            ait.deleteArg();
            removedArgument = true;
        }
    }
    return removedArgument;
}

Utils::FileName Qt4BuildConfiguration::extractSpecFromArguments(QString *args,
                                                                const QString &directory, const QtSupport::BaseQtVersion *version,
                                                                QStringList *outArgs)
{
    Utils::FileName parsedSpec;

    bool ignoreNext = false;
    bool nextIsSpec = false;
    for (Utils::QtcProcess::ArgIterator ait(args); ait.next(); ) {
        if (ignoreNext) {
            ignoreNext = false;
            ait.deleteArg();
        } else if (nextIsSpec) {
            nextIsSpec = false;
            parsedSpec = Utils::FileName::fromUserInput(ait.value());
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
        return Utils::FileName();

    Utils::FileName baseMkspecDir = Utils::FileName::fromUserInput(version->versionInfo().value(QLatin1String("QMAKE_MKSPECS")));
    if (baseMkspecDir.isEmpty())
        baseMkspecDir = Utils::FileName::fromUserInput(version->versionInfo().value(QLatin1String("QT_INSTALL_DATA"))
                                                       + QLatin1String("/mkspecs"));

    // if the path is relative it can be
    // relative to the working directory (as found in the Makefiles)
    // or relatively to the mkspec directory
    // if it is the former we need to get the canonical form
    // for the other one we don't need to do anything
    if (parsedSpec.toFileInfo().isRelative()) {
        if (QFileInfo(directory + QLatin1Char('/') + parsedSpec.toString()).exists()) {
            parsedSpec = Utils::FileName::fromUserInput(directory + QLatin1Char('/') + parsedSpec.toString());
        } else {
            parsedSpec = Utils::FileName::fromUserInput(baseMkspecDir.toString() + QLatin1Char('/') + parsedSpec.toString());
        }
    }

    QFileInfo f2 = parsedSpec.toFileInfo();
    while (f2.isSymLink()) {
        parsedSpec = Utils::FileName::fromString(f2.symLinkTarget());
        f2.setFile(parsedSpec.toString());
    }

    if (parsedSpec.isChildOf(baseMkspecDir)) {
        parsedSpec = parsedSpec.relativeChildPath(baseMkspecDir);
    } else {
        Utils::FileName sourceMkSpecPath = Utils::FileName::fromString(version->sourcePath().toString()
                                                                       + QLatin1String("/mkspecs"));
        if (parsedSpec.isChildOf(sourceMkSpecPath)) {
            parsedSpec = parsedSpec.relativeChildPath(sourceMkSpecPath);
        }
    }
    return parsedSpec;
}

ProjectExplorer::IOutputParser *Qt4BuildConfiguration::createOutputParser() const
{
    ToolChain *tc = ToolChainProfileInformation::toolChain(target()->profile());
    return tc ? tc->outputParser() : 0;
}

bool Qt4BuildConfiguration::isEnabled() const
{
    return m_isEnabled;
}

QString Qt4BuildConfiguration::disabledReason() const
{
    if (!m_isEnabled)
        return tr("Parsing the .pro file");
    return QString();
}

void Qt4BuildConfiguration::setEnabled(bool enabled)
{
    if (m_isEnabled == enabled)
        return;
    m_isEnabled = enabled;
    emit enabledChanged();
}

/*!
  \class Qt4BuildConfigurationFactory
*/

Qt4BuildConfigurationFactory::Qt4BuildConfigurationFactory(QObject *parent) :
    ProjectExplorer::IBuildConfigurationFactory(parent)
{
    update();

    QtSupport::QtVersionManager *vm = QtSupport::QtVersionManager::instance();
    connect(vm, SIGNAL(qtVersionsChanged(QList<int>,QList<int>,QList<int>)),
            this, SLOT(update()));
}

Qt4BuildConfigurationFactory::~Qt4BuildConfigurationFactory()
{
}

void Qt4BuildConfigurationFactory::update()
{
    emit availableCreationIdsChanged();
}

bool Qt4BuildConfigurationFactory::canHandle(const Target *t) const
{
    if (!t->project()->supportsProfile(t->profile()))
        return false;
    return qobject_cast<Qt4Project *>(t->project());
}

QList<Core::Id> Qt4BuildConfigurationFactory::availableCreationIds(const Target *parent) const
{
    if (!canHandle(parent))
        return QList<Core::Id>();
    return QList<Core::Id>() << Core::Id(QT4_BC_ID);
}

QString Qt4BuildConfigurationFactory::displayNameForId(const Core::Id id) const
{
    if (id != Core::Id(QT4_BC_ID))
        return QString();

    return tr("Qmake based build");
}

bool Qt4BuildConfigurationFactory::canCreate(const Target *parent, const Core::Id id) const
{
    if (!canHandle(parent))
        return false;
    return id == QT4_BC_ID;
}

BuildConfiguration *Qt4BuildConfigurationFactory::create(ProjectExplorer::Target *parent, const Core::Id id, const QString &name)
{
    if (!canCreate(parent, id))
        return 0;

    QtSupport::BaseQtVersion *version = QtSupport::QtProfileInformation::qtVersion(parent->profile());
    Q_ASSERT(version);

    bool ok = true;
    QString buildConfigurationName = name;
    if (buildConfigurationName.isEmpty())
        buildConfigurationName = QInputDialog::getText(0,
                                                       tr("New Configuration"),
                                                       tr("New configuration name:"),
                                                       QLineEdit::Normal,
                                                       version->displayName(), &ok);
    buildConfigurationName = buildConfigurationName.trimmed();
    if (!ok || buildConfigurationName.isEmpty())
        return 0;

    //: Debug build configuration. We recommend not translating it.
    QString defaultFirstName = tr("%1 Debug").arg(version->displayName());
    QString customFirstName;
    if (buildConfigurationName != version->displayName())
        customFirstName = tr("%1 Debug").arg(buildConfigurationName);

    //: Release build configuration. We recommend not translating it.
    QString defaultSecondName = tr("%1 Release").arg(version->displayName());
    QString customSecondName;
    if (buildConfigurationName != version->displayName())
        customSecondName = tr("%1 Release").arg(buildConfigurationName);

    if (!(version->defaultBuildConfig() & QtSupport::BaseQtVersion::DebugBuild)) {
        qSwap(defaultFirstName, defaultSecondName);
        qSwap(customFirstName, customSecondName);
    }

    BuildConfiguration *bc
            = Qt4BuildConfiguration::setup(parent, defaultFirstName, customFirstName,
                                           version->defaultBuildConfig(), QString(), QString(), false);
    parent->addBuildConfiguration(
                Qt4BuildConfiguration::setup(parent, defaultSecondName, customSecondName,
                                             (version->defaultBuildConfig() ^ QtSupport::BaseQtVersion::DebugBuild),
                                             QString(), QString(), false));
    return bc;
}

bool Qt4BuildConfigurationFactory::canClone(const Target *parent, ProjectExplorer::BuildConfiguration *source) const
{
    return canHandle(parent) && qobject_cast<Qt4BuildConfiguration *>(source);
}

BuildConfiguration *Qt4BuildConfigurationFactory::clone(Target *parent, BuildConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    Qt4BuildConfiguration *oldbc(static_cast<Qt4BuildConfiguration *>(source));
    return new Qt4BuildConfiguration(parent, oldbc);
}

bool Qt4BuildConfigurationFactory::canRestore(const Target *parent, const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;
    return ProjectExplorer::idFromMap(map).toString() == QLatin1String(QT4_BC_ID);
}

BuildConfiguration *Qt4BuildConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    Qt4BuildConfiguration *bc = new Qt4BuildConfiguration(parent);
    if (bc->fromMap(map))
        return bc;
    delete bc;
    return 0;
}

QList<BuildConfigurationInfo> Qt4BuildConfigurationFactory::availableBuildConfigurations(const ProjectExplorer::Profile *p,
                                                                                         const QString &proFilePath)
{
    QList<BuildConfigurationInfo> infoList;

    QtSupport::BaseQtVersion *version = QtSupport::QtProfileInformation::qtVersion(p);
    if (!version || !version->isValid())
        return infoList;
    QtSupport::BaseQtVersion::QmakeBuildConfigs config = version->defaultBuildConfig();
    BuildConfigurationInfo info = BuildConfigurationInfo(config, QString(), QString(), false);
    info.directory = Qt4Project::shadowBuildDirectory(proFilePath, p, buildConfigurationDisplayName(info));
    infoList.append(info);

    info.buildConfig = config ^ QtSupport::BaseQtVersion::DebugBuild;
    info.directory = Qt4Project::shadowBuildDirectory(proFilePath, p, buildConfigurationDisplayName(info));
    if (!QFileInfo(info.directory).exists())
        infoList.append(info);

    return infoList;
}

// Return name of a build configuration.
QString Qt4BuildConfigurationFactory::buildConfigurationDisplayName(const BuildConfigurationInfo &info)
{
    return (info.buildConfig & QtSupport::BaseQtVersion::DebugBuild) ?
                //: Name of a debug build configuration to created by a project wizard. We recommend not translating it.
                tr("Debug") :
                //: Name of a release build configuration to be created by a project wizard. We recommend not translating it.
                tr("Release");
}

BuildConfiguration::BuildType Qt4BuildConfiguration::buildType() const
{
    if (qmakeBuildConfiguration() & QtSupport::BaseQtVersion::DebugBuild)
        return Debug;
    else
        return Release;
}

Qt4BuildConfiguration *Qt4BuildConfiguration::setup(Target *t, QString defaultDisplayName,
                                                    QString displayName,
                                                    QtSupport::BaseQtVersion::QmakeBuildConfigs qmakeBuildConfiguration,
                                                    QString additionalArguments, QString directory,
                                                    bool importing)
{
    bool debug = qmakeBuildConfiguration & QtSupport::BaseQtVersion::DebugBuild;

    // Add the buildconfiguration
    Qt4BuildConfiguration *bc = new Qt4BuildConfiguration(t);
    bc->setDefaultDisplayName(defaultDisplayName);
    bc->setDisplayName(displayName);

    ProjectExplorer::BuildStepList *buildSteps =
            bc->stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD));
    ProjectExplorer::BuildStepList *cleanSteps =
            bc->stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_CLEAN));
    Q_ASSERT(buildSteps);
    Q_ASSERT(cleanSteps);

    QMakeStep *qmakeStep = new QMakeStep(buildSteps);
    buildSteps->insertStep(0, qmakeStep);

    MakeStep *makeStep = new MakeStep(buildSteps);
    buildSteps->insertStep(1, makeStep);

    MakeStep* cleanStep = new MakeStep(cleanSteps);
    cleanStep->setClean(true);
    cleanStep->setUserArguments(QLatin1String("clean"));
    cleanSteps->insertStep(0, cleanStep);

    bool enableQmlDebugger
            = Qt4BuildConfiguration::removeQMLInspectorFromArguments(&additionalArguments);
    if (!additionalArguments.isEmpty())
        qmakeStep->setUserArguments(additionalArguments);
    if (importing)
        qmakeStep->setLinkQmlDebuggingLibrary(enableQmlDebugger);

    // set some options for qmake and make
    if (qmakeBuildConfiguration & QtSupport::BaseQtVersion::BuildAll) // debug_and_release => explicit targets
        makeStep->setUserArguments(debug ? QLatin1String("debug") : QLatin1String("release"));

    bc->setQMakeBuildConfiguration(qmakeBuildConfiguration);

    if (!directory.isEmpty())
        bc->setShadowBuildAndDirectory(directory != t->project()->projectDirectory(), directory);

    return bc;
}
