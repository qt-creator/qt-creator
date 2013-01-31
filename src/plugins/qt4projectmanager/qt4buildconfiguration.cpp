/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

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
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/kitinformation.h>
#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/qtversionfactory.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>
#include <qt4projectmanager/qmakekitinformation.h>
#include <QDebug>

#include <QInputDialog>

namespace Qt4ProjectManager {

using namespace Internal;
using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;

const char QT4_BC_ID[] = "Qt4ProjectManager.Qt4BuildConfiguration";
const char USE_SHADOW_BUILD_KEY[] = "Qt4ProjectManager.Qt4BuildConfiguration.UseShadowBuild";
const char BUILD_DIRECTORY_KEY[] = "Qt4ProjectManager.Qt4BuildConfiguration.BuildDirectory";
const char BUILD_CONFIGURATION_KEY[] = "Qt4ProjectManager.Qt4BuildConfiguration.BuildConfiguration";

enum { debug = 0 };

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
    m_qmakeBuildConfiguration = BaseQtVersion::QmakeBuildConfigs(map.value(QLatin1String(BUILD_CONFIGURATION_KEY)).toInt());
    m_buildDirectory = map.value(QLatin1String(BUILD_DIRECTORY_KEY), defaultShadowBuildDirectory()).toString();

    m_lastEmmitedBuildDirectory = buildDirectory();
    m_qtVersionSupportsShadowBuilds =  supportsShadowBuilds();

    m_lastKitState = LastKitState(target()->kit());

    connect(ProjectExplorer::ToolChainManager::instance(), SIGNAL(toolChainUpdated(ProjectExplorer::ToolChain *)),
            this, SLOT(toolChainUpdated(ProjectExplorer::ToolChain *)));
    connect(QtSupport::QtVersionManager::instance(), SIGNAL(qtVersionsChanged(QList<int>,QList<int>,QList<int>)),
            this, SLOT(qtVersionsChanged(QList<int>,QList<int>,QList<int>)));
    return true;
}

void Qt4BuildConfiguration::ctor()
{
    connect(this, SIGNAL(environmentChanged()),
            this, SLOT(emitBuildDirectoryChanged()));
    connect(this, SIGNAL(environmentChanged()),
            this, SLOT(emitProFileEvaluateNeeded()));
    connect(target(), SIGNAL(kitChanged()),
            this, SLOT(kitChanged()));
}

void Qt4BuildConfiguration::kitChanged()
{
    LastKitState newState = LastKitState(target()->kit());
    if (newState != m_lastKitState) {
        // This only checks if the ids have changed!
        // For that reason the Qt4BuildConfiguration is also connected
        // to the toolchain and qtversion managers
        emitProFileEvaluateNeeded();
        emitBuildDirectoryChanged();
        m_lastKitState = newState;
    }
}

void Qt4BuildConfiguration::toolChainUpdated(ProjectExplorer::ToolChain *tc)
{
    if (ToolChainKitInformation::toolChain(target()->kit()) == tc)
        emitProFileEvaluateNeeded();
}

void Qt4BuildConfiguration::qtVersionsChanged(const QList<int> &,const QList<int> &, const QList<int> &changed)
{
    if (changed.contains(QtKitInformation::qtVersionId(target()->kit())))
        emitProFileEvaluateNeeded();
}

void Qt4BuildConfiguration::emitBuildDirectoryChanged()
{
    // We also emit buildDirectoryChanged if the the Qt version's supportShadowBuild changed
    if (buildDirectory() != m_lastEmmitedBuildDirectory
            || supportsShadowBuilds() != m_qtVersionSupportsShadowBuilds) {
        m_lastEmmitedBuildDirectory = buildDirectory();
        m_qtVersionSupportsShadowBuilds = supportsShadowBuilds();
        emit buildDirectoryChanged();
    }
}

NamedWidget *Qt4BuildConfiguration::createConfigWidget()
{
    return new Qt4ProjectConfigWidget(this);
}

QString Qt4BuildConfiguration::defaultShadowBuildDirectory() const
{
    // todo displayName isn't ideal
    return Qt4Project::shadowBuildDirectory(target()->project()->document()->fileName(),
                                            target()->kit(), displayName());
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

/// Returns the build directory.
QString Qt4BuildConfiguration::buildDirectory() const
{
    QString path = QDir::cleanPath(environment().expandVariables(rawBuildDirectory()));
    return QDir::cleanPath(QDir(target()->project()->projectDirectory()).absoluteFilePath(path));
}

bool Qt4BuildConfiguration::supportsShadowBuilds()
{
    BaseQtVersion *version = QtKitInformation::qtVersion(target()->kit());
    return !version || version->supportsShadowBuilds();
}

/// If only a sub tree should be build this function returns which sub node
/// should be build
/// \see Qt4BuildConfiguration::setSubNodeBuild
Qt4ProFileNode *Qt4BuildConfiguration::subNodeBuild() const
{
    return m_subNodeBuild;
}

/// A sub node build on builds a sub node of the project
/// That is triggered by a right click in the project explorer tree
/// The sub node to be build is set via this function immediately before
/// calling BuildManager::buildProject( BuildConfiguration * )
/// and reset immediately afterwards
/// That is m_subNodesBuild is set only temporarly
void Qt4BuildConfiguration::setSubNodeBuild(Qt4ProFileNode *node)
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
    BaseQtVersion *version = QtKitInformation::qtVersion(target()->kit());
    QString directoryToSet = buildDirectory;
    bool toSet = (shadowBuild && version && version->isValid() && version->supportsShadowBuilds());
    if (m_shadowBuild == toSet && m_buildDirectory == directoryToSet)
        return;

    m_shadowBuild = toSet;
    m_buildDirectory = directoryToSet;

    emitBuildDirectoryChanged();
    emitProFileEvaluateNeeded();
}

QString Qt4BuildConfiguration::makefile() const
{
    return static_cast<Qt4Project *>(target()->project())->rootQt4ProjectNode()->makefile();
}

BaseQtVersion::QmakeBuildConfigs Qt4BuildConfiguration::qmakeBuildConfiguration() const
{
    return m_qmakeBuildConfiguration;
}

void Qt4BuildConfiguration::setQMakeBuildConfiguration(BaseQtVersion::QmakeBuildConfigs config)
{
    if (m_qmakeBuildConfiguration == config)
        return;
    m_qmakeBuildConfiguration = config;

    emit qmakeBuildConfigurationChanged();
    emitBuildDirectoryChanged();
    emitProFileEvaluateNeeded();
}

void Qt4BuildConfiguration::emitProFileEvaluateNeeded()
{
    Target *t = target();
    Project *p = t->project();
    if (t->activeBuildConfiguration() == this && p->activeTarget() == t)
        static_cast<Qt4Project *>(p)->scheduleAsyncUpdate();
}

void Qt4BuildConfiguration::emitQMakeBuildConfigurationChanged()
{
    emit qmakeBuildConfigurationChanged();
}

QStringList Qt4BuildConfiguration::configCommandLineArguments() const
{
    QStringList result;
    BaseQtVersion *version = QtKitInformation::qtVersion(target()->kit());
    BaseQtVersion::QmakeBuildConfigs defaultBuildConfiguration =
            version ? version->defaultBuildConfig() : (BaseQtVersion::DebugBuild | BaseQtVersion::BuildAll);
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

// Returns true if both are equal.
Qt4BuildConfiguration::MakefileState Qt4BuildConfiguration::compareToImportFrom(const QString &makefile)
{
    QMakeStep *qs = qmakeStep();
    if (QFileInfo(makefile).exists() && qs) {
        FileName qmakePath = QtVersionManager::findQMakeBinaryFromMakefile(makefile);
        BaseQtVersion *version = QtKitInformation::qtVersion(target()->kit());
        if (!version)
            return MakefileForWrongProject;
        if (version->qmakeCommand() == qmakePath) {
            // same qtversion
            QPair<BaseQtVersion::QmakeBuildConfigs, QString> result =
                    QtVersionManager::scanMakeFile(makefile, version->defaultBuildConfig());
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
                FileName actualSpec = qs->mkspec();

                QString qmakeArgs = result.second;
                QStringList parsedArgs;
                FileName parsedSpec = extractSpecFromArguments(&qmakeArgs, workingDirectory, version, &parsedArgs);

                if (debug) {
                    qDebug() << "Actual args:" << actualArgs;
                    qDebug() << "Parsed args:" << parsedArgs;
                    qDebug() << "Actual spec:" << actualSpec.toString();
                    qDebug() << "Parsed spec:" << parsedSpec.toString();
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
//                    qDebug() << "AS vs VS" << actualSpec << version->mkspec();
                    if ((actualSpec == version->mkspec() || actualSpec == FileName::fromString(QLatin1String("default")))
                        && (parsedSpec == version->mkspec() || parsedSpec == FileName::fromString(QLatin1String("default")) || parsedSpec.isEmpty()))
                        return MakefileMatches;
                }
                return MakefileIncompatible;
            } else {
                if (debug)
                    qDebug() << "different qmake buildconfigurations buildconfiguration:"
                             << qmakeBuildConfiguration() << " Makefile:" << result.first;
                return MakefileIncompatible;
            }
        } else {
            if (debug)
                qDebug() << "different Qt versions, buildconfiguration:" << version->qmakeCommand().toString()
                         << " Makefile:"<< qmakePath.toString();
            return MakefileForWrongProject;
        }
    }
    return MakefileMissing;
}

bool Qt4BuildConfiguration::removeQMLInspectorFromArguments(QString *args)
{
    bool removedArgument = false;
    for (QtcProcess::ArgIterator ait(args); ait.next(); ) {
        const QString arg = ait.value();
        if (arg.contains(QLatin1String(Constants::QMAKEVAR_QMLJSDEBUGGER_PATH))
            || arg.contains(QLatin1String(Constants::QMAKEVAR_QUICK1_DEBUG))
            || arg.contains(QLatin1String(Constants::QMAKEVAR_QUICK2_DEBUG))) {
            ait.deleteArg();
            removedArgument = true;
        }
    }
    return removedArgument;
}

FileName Qt4BuildConfiguration::extractSpecFromArguments(QString *args,
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
    baseMkspecDir = Utils::FileName::fromString(baseMkspecDir.toFileInfo().canonicalFilePath());

    // if the path is relative it can be
    // relative to the working directory (as found in the Makefiles)
    // or relatively to the mkspec directory
    // if it is the former we need to get the canonical form
    // for the other one we don't need to do anything
    if (parsedSpec.toFileInfo().isRelative()) {
        if (QFileInfo(directory + QLatin1Char('/') + parsedSpec.toString()).exists())
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
    IBuildConfigurationFactory(parent)
{
    update();

    QtVersionManager *vm = QtVersionManager::instance();
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
    if (!t->project()->supportsKit(t->kit()))
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
    if (id == QT4_BC_ID)
        return tr("Qmake based build");
    return QString();
}

bool Qt4BuildConfigurationFactory::canCreate(const Target *parent, const Core::Id id) const
{
    if (!canHandle(parent))
        return false;
    return id == QT4_BC_ID;
}

BuildConfiguration *Qt4BuildConfigurationFactory::create(Target *parent, const Core::Id id, const QString &name)
{
    if (!canCreate(parent, id))
        return 0;

    BaseQtVersion *version = QtKitInformation::qtVersion(parent->kit());
    Q_ASSERT(version);

    bool ok = true;
    QString buildConfigurationName = name;
    if (buildConfigurationName.isNull())
        buildConfigurationName = QInputDialog::getText(0,
                                                       tr("New Configuration"),
                                                       tr("New configuration name:"),
                                                       QLineEdit::Normal,
                                                       version->displayName(), &ok);
    buildConfigurationName = buildConfigurationName.trimmed();
    if (!ok || buildConfigurationName.isEmpty())
        return 0;

    //: Debug build configuration. We recommend not translating it.
    QString defaultFirstName = tr("%1 Debug").arg(version->displayName()).trimmed();
    QString customFirstName;
    if (buildConfigurationName != version->displayName())
        customFirstName = tr("%1 Debug").arg(buildConfigurationName).trimmed();

    //: Release build configuration. We recommend not translating it.
    QString defaultSecondName = tr("%1 Release").arg(version->displayName()).trimmed();
    QString customSecondName;
    if (buildConfigurationName != version->displayName())
        customSecondName = tr("%1 Release").arg(buildConfigurationName).trimmed();

    BuildConfiguration *bc
            = Qt4BuildConfiguration::setup(parent, defaultFirstName, customFirstName,
                                           version->defaultBuildConfig(), QString(), QString(), false);
    parent->addBuildConfiguration(
                Qt4BuildConfiguration::setup(parent, defaultSecondName, customSecondName,
                                             (version->defaultBuildConfig() ^ BaseQtVersion::DebugBuild),
                                             QString(), QString(), false));
    return bc;
}

bool Qt4BuildConfigurationFactory::canClone(const Target *parent, BuildConfiguration *source) const
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
    return ProjectExplorer::idFromMap(map) == QT4_BC_ID;
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

QList<BuildConfigurationInfo> Qt4BuildConfigurationFactory::availableBuildConfigurations(const Kit *k,
                                                                                         const QString &proFilePath)
{
    QList<BuildConfigurationInfo> infoList;

    BaseQtVersion *version = QtKitInformation::qtVersion(k);
    if (!version || !version->isValid())
        return infoList;
    BaseQtVersion::QmakeBuildConfigs config = version->defaultBuildConfig() | QtSupport::BaseQtVersion::DebugBuild;
    BuildConfigurationInfo info = BuildConfigurationInfo(config, QString(), QString(), false);
    info.directory = Qt4Project::shadowBuildDirectory(proFilePath, k, buildConfigurationDisplayName(info));
    infoList.append(info);

    info.buildConfig = config ^ BaseQtVersion::DebugBuild;
    info.directory = Qt4Project::shadowBuildDirectory(proFilePath, k, buildConfigurationDisplayName(info));
    infoList.append(info);
    return infoList;
}

// Return name of a build configuration.
QString Qt4BuildConfigurationFactory::buildConfigurationDisplayName(const BuildConfigurationInfo &info)
{
    return (info.buildConfig & BaseQtVersion::DebugBuild) ?
                //: Name of a debug build configuration to created by a project wizard. We recommend not translating it.
                tr("Debug") :
                //: Name of a release build configuration to be created by a project wizard. We recommend not translating it.
                tr("Release");
}

BuildConfiguration::BuildType Qt4BuildConfiguration::buildType() const
{
    if (qmakeBuildConfiguration() & BaseQtVersion::DebugBuild)
        return Debug;
    else
        return Release;
}

Qt4BuildConfiguration *Qt4BuildConfiguration::setup(Target *t, QString defaultDisplayName,
                                                    QString displayName,
                                                    BaseQtVersion::QmakeBuildConfigs qmakeBuildConfiguration,
                                                    QString additionalArguments, QString directory,
                                                    bool importing)
{
    bool debug = qmakeBuildConfiguration & BaseQtVersion::DebugBuild;

    // Add the build configuration.
    Qt4BuildConfiguration *bc = new Qt4BuildConfiguration(t);
    bc->setDefaultDisplayName(defaultDisplayName);
    bc->setDisplayName(displayName);

    BuildStepList *buildSteps = bc->stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD));
    BuildStepList *cleanSteps = bc->stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_CLEAN));
    Q_ASSERT(buildSteps);
    Q_ASSERT(cleanSteps);

    QMakeStep *qmakeStep = new QMakeStep(buildSteps);
    buildSteps->insertStep(0, qmakeStep);

    MakeStep *makeStep = new MakeStep(buildSteps);
    buildSteps->insertStep(1, makeStep);

    MakeStep *cleanStep = new MakeStep(cleanSteps);
    cleanStep->setClean(true);
    cleanStep->setUserArguments(QLatin1String("clean"));
    cleanSteps->insertStep(0, cleanStep);

    bool enableQmlDebugger
            = Qt4BuildConfiguration::removeQMLInspectorFromArguments(&additionalArguments);
    if (!additionalArguments.isEmpty())
        qmakeStep->setUserArguments(additionalArguments);
    if (importing)
        qmakeStep->setLinkQmlDebuggingLibrary(enableQmlDebugger);

    // Set some options for qmake and make.
    if (qmakeBuildConfiguration & BaseQtVersion::BuildAll) // debug_and_release => explicit targets
        makeStep->setUserArguments(debug ? QLatin1String("debug") : QLatin1String("release"));

    bc->setQMakeBuildConfiguration(qmakeBuildConfiguration);

    if (!directory.isEmpty())
        bc->setShadowBuildAndDirectory(directory != t->project()->projectDirectory(), directory);

    return bc;
}

Qt4BuildConfiguration::LastKitState::LastKitState()
{

}

Qt4BuildConfiguration::LastKitState::LastKitState(Kit *k)
    : m_qtVersion(QtKitInformation::qtVersionId(k)),
      m_sysroot(SysRootKitInformation::sysRoot(k).toString()),
      m_mkspec(QmakeKitInformation::mkspec(k).toString())
{
    ToolChain *tc = ToolChainKitInformation::toolChain(k);
    m_toolchain = tc ? tc->id() : QString();
}

bool Qt4BuildConfiguration::LastKitState::operator ==(const LastKitState &other)
{
    return m_qtVersion == other.m_qtVersion
            && m_toolchain == other.m_toolchain
            && m_sysroot == other.m_sysroot
            && m_mkspec == other.m_mkspec;
}

bool Qt4BuildConfiguration::LastKitState::operator !=(const LastKitState &other)
{
    return !operator ==(other);
}



} // namespace Qt4ProjectManager
