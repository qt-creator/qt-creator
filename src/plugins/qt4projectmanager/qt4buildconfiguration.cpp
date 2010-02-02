/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qt4buildconfiguration.h"

#include "qt4project.h"

#include <utils/qtcassert.h>

#include <QtGui/QInputDialog>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;
using namespace ProjectExplorer;

namespace {
const char * const QT4_BC_ID_PREFIX("Qt4ProjectManager.Qt4BuildConfiguration.");
const char * const QT4_BC_ID("Qt4ProjectManager.Qt4BuildConfiguration");

const char * const CLEAR_SYSTEM_ENVIRONMENT_KEY("Qt4ProjectManager.Qt4BuildConfiguration.ClearSystemEnvironment");
const char * const USER_ENVIRONMENT_CHANGES_KEY("Qt4ProjectManager.Qt4BuildConfiguration.UserEnvironmentChanges");
const char * const USE_SHADOW_BUILD_KEY("Qt4ProjectManager.Qt4BuildConfiguration.UseShadowBuild");
const char * const BUILD_DIRECTORY_KEY("Qt4ProjectManager.Qt4BuildConfiguration.BuildDirectory");
const char * const TOOLCHAIN_KEY("Qt4ProjectManager.Qt4BuildConfiguration.ToolChain");
const char * const BUILD_CONFIGURATION_KEY("Qt4ProjectManager.Qt4BuildConfiguration.BuildConfiguration");
const char * const QT_VERSION_ID_KEY("Qt4ProjectManager.Qt4BuildConfiguration.QtVersionId");

enum { debug = 0 };
}

Qt4BuildConfiguration::Qt4BuildConfiguration(Qt4Project *pro) :
    BuildConfiguration(pro, QLatin1String(QT4_BC_ID)),
    m_clearSystemEnvironment(false),
    m_shadowBuild(false),
    m_qtVersion(0),
    m_toolChainType(-1), // toolChainType() makes sure to return the default toolchainType
    m_qmakeBuildConfiguration(0),
    m_subNodeBuild(0)
{
    ctor();
}

Qt4BuildConfiguration::Qt4BuildConfiguration(Qt4Project *pro, const QString &id) :
    BuildConfiguration(pro, id),
    m_clearSystemEnvironment(false),
    m_shadowBuild(false),
    m_qtVersion(0),
    m_toolChainType(-1), // toolChainType() makes sure to return the default toolchainType
    m_qmakeBuildConfiguration(0),
    m_subNodeBuild(0)
{
    ctor();
}

Qt4BuildConfiguration::Qt4BuildConfiguration(Qt4Project *pro, Qt4BuildConfiguration *source) :
    BuildConfiguration(pro, source),
    m_clearSystemEnvironment(source->m_clearSystemEnvironment),
    m_userEnvironmentChanges(source->m_userEnvironmentChanges),
    m_shadowBuild(source->m_shadowBuild),
    m_buildDirectory(source->m_buildDirectory),
    m_qtVersion(source->m_qtVersion),
    m_toolChainType(source->m_toolChainType),
    m_qmakeBuildConfiguration(source->m_qmakeBuildConfiguration),
    m_subNodeBuild(0) // temporary value, so not copied
{
    ctor();
}

Qt4BuildConfiguration::~Qt4BuildConfiguration()
{
}

QVariantMap Qt4BuildConfiguration::toMap() const
{
    QVariantMap map(BuildConfiguration::toMap());
    map.insert(QLatin1String(CLEAR_SYSTEM_ENVIRONMENT_KEY), m_clearSystemEnvironment);
    map.insert(QLatin1String(USER_ENVIRONMENT_CHANGES_KEY), EnvironmentItem::toStringList(m_userEnvironmentChanges));
    map.insert(QLatin1String(USE_SHADOW_BUILD_KEY), m_shadowBuild);
    map.insert(QLatin1String(BUILD_DIRECTORY_KEY), m_buildDirectory);
    map.insert(QLatin1String(QT_VERSION_ID_KEY), m_qtVersion);
    map.insert(QLatin1String(TOOLCHAIN_KEY), m_toolChainType);
    map.insert(QLatin1String(BUILD_CONFIGURATION_KEY), int(m_qmakeBuildConfiguration));
    return map;
}


bool Qt4BuildConfiguration::fromMap(const QVariantMap &map)
{
    m_clearSystemEnvironment = map.value(QLatin1String(CLEAR_SYSTEM_ENVIRONMENT_KEY)).toBool();
    m_userEnvironmentChanges = EnvironmentItem::fromStringList(map.value(QLatin1String(USER_ENVIRONMENT_CHANGES_KEY)).toStringList());
    m_shadowBuild = map.value(QLatin1String(USE_SHADOW_BUILD_KEY)).toBool();
    m_buildDirectory = map.value(QLatin1String(BUILD_DIRECTORY_KEY)).toString();
    m_qtVersion = map.value(QLatin1String(QT_VERSION_ID_KEY)).toInt();
    m_toolChainType = map.value(QLatin1String(TOOLCHAIN_KEY)).toInt();
    m_qmakeBuildConfiguration = QtVersion::QmakeBuildConfigs(map.value(QLatin1String(BUILD_CONFIGURATION_KEY)).toInt());

    return BuildConfiguration::fromMap(map);
}

void Qt4BuildConfiguration::ctor()
{
    QtVersionManager *vm = QtVersionManager::instance();
    connect(vm, SIGNAL(defaultQtVersionChanged()),
            this, SLOT(defaultQtVersionChanged()));
    connect(vm, SIGNAL(qtVersionsChanged(QList<int>)),
            this, SLOT(qtVersionsChanged(QList<int>)));
}

Qt4Project *Qt4BuildConfiguration::qt4Project() const
{
    return static_cast<Qt4Project *>(project());
}

QString Qt4BuildConfiguration::baseEnvironmentText() const
{
    return useSystemEnvironment() ? tr("System Environment") : tr("Clean Environment");
}

ProjectExplorer::Environment Qt4BuildConfiguration::baseEnvironment() const
{
    Environment env = useSystemEnvironment() ? Environment::systemEnvironment() : Environment();
    qtVersion()->addToEnvironment(env);
    ToolChain *tc = toolChain();
    if (tc)
        tc->addToEnvironment(env);
    return env;
}

ProjectExplorer::Environment Qt4BuildConfiguration::environment() const
{
    Environment env = baseEnvironment();
    env.modify(userEnvironmentChanges());
    return env;
}

void Qt4BuildConfiguration::setUseSystemEnvironment(bool b)
{
    if (useSystemEnvironment() == b)
        return;
    m_clearSystemEnvironment = !b;
    emit environmentChanged();
}

bool Qt4BuildConfiguration::useSystemEnvironment() const
{
    return !m_clearSystemEnvironment;
}

QList<ProjectExplorer::EnvironmentItem> Qt4BuildConfiguration::userEnvironmentChanges() const
{
    return m_userEnvironmentChanges;
}

void Qt4BuildConfiguration::setUserEnvironmentChanges(const QList<ProjectExplorer::EnvironmentItem> &diff)
{
    if (m_userEnvironmentChanges == diff)
        return;
    m_userEnvironmentChanges = diff;
    emit environmentChanged();
}

/// returns the build directory
QString Qt4BuildConfiguration::buildDirectory() const
{
    QString workingDirectory;
    if (m_shadowBuild)
        workingDirectory = m_buildDirectory;
    if (workingDirectory.isEmpty())
        workingDirectory = QFileInfo(project()->file()->fileName()).absolutePath();
    return workingDirectory;
}

/// If only a sub tree should be build this function returns which sub node
/// should be build
/// \see Qt4BuildConfiguration::setSubNodeBuild
Qt4ProjectManager::Internal::Qt4ProFileNode *Qt4BuildConfiguration::subNodeBuild() const
{
    return m_subNodeBuild;
}

/// A sub node build on builds a sub node of the project
/// That is triggered by a right click in the project explorer tree
/// The sub node to be build is set via this function immediately before
/// calling BuildManager::buildProject( BuildConfiguration * )
/// and reset immediately afterwards
/// That is m_subNodesBuild is set only temporarly
void Qt4BuildConfiguration::setSubNodeBuild(Qt4ProjectManager::Internal::Qt4ProFileNode *node)
{
    m_subNodeBuild = node;
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
    return m_buildDirectory;
}

void Qt4BuildConfiguration::setShadowBuildAndDirectory(bool shadowBuild, const QString &buildDirectory)
{
    if (m_shadowBuild == shadowBuild && m_buildDirectory == buildDirectory)
        return;
    m_shadowBuild = shadowBuild;
    m_buildDirectory = buildDirectory;
    emit buildDirectoryChanged();
    emit targetInformationChanged();
}

ProjectExplorer::ToolChain *Qt4BuildConfiguration::toolChain() const
{
    ToolChain::ToolChainType tct = toolChainType();
    return qtVersion()->toolChain(tct);
}

QString Qt4BuildConfiguration::makeCommand() const
{
    ToolChain *tc = toolChain();
    return tc ? tc->makeCommand() : "make";
}

static inline QString symbianMakeTarget(QtVersion::QmakeBuildConfigs buildConfig,
                                        const QString &type)
{
    QString rc = (buildConfig & QtVersion::DebugBuild) ?
                 QLatin1String("debug-") : QLatin1String("release-");
    rc += type;
    return rc;
}

QString Qt4BuildConfiguration::defaultMakeTarget() const
{
    ToolChain *tc = toolChain();
    if (!tc)
        return QString();
    const QtVersion::QmakeBuildConfigs buildConfig = qmakeBuildConfiguration();

    switch (tc->type()) {
    case ToolChain::GCCE:
    case ToolChain::GCCE_GNUPOC:
        return symbianMakeTarget(buildConfig, QLatin1String("gcce"));
    case ToolChain::RVCT_ARMV5:
        return symbianMakeTarget(buildConfig, QLatin1String("armv5"));
    case ToolChain::RVCT_ARMV6:
    case ToolChain::RVCT_ARMV6_GNUPOC:
        return symbianMakeTarget(buildConfig, QLatin1String("armv6"));
    default:
        break;
    }
    return QString();
}

QtVersion *Qt4BuildConfiguration::qtVersion() const
{
    return QtVersionManager::instance()->version(qtVersionId());
}

int Qt4BuildConfiguration::qtVersionId() const
{
    QtVersionManager *vm = QtVersionManager::instance();
    if (!vm->version(m_qtVersion)->isValid())
        m_qtVersion = 0;

    return m_qtVersion;
}

// TODO: This assumes there is always at least one Qt version... Is that valid?
void Qt4BuildConfiguration::setQtVersion(int id)
{
    if (m_qtVersion == id)
        return;

    if (!QtVersionManager::instance()->isValidId(id))
        return;

    QtVersionManager *vm = QtVersionManager::instance();
    if (!vm->version(id)->isValid())
        return;

    m_qtVersion = id;
    emit qtVersionChanged();
    emit targetInformationChanged();
    emit environmentChanged();
}

void Qt4BuildConfiguration::setToolChainType(ProjectExplorer::ToolChain::ToolChainType type)
{
    if (m_toolChainType == type)
        return;
    m_toolChainType = type;
    emit toolChainTypeChanged();
    emit targetInformationChanged();
    emit environmentChanged();
}

ProjectExplorer::ToolChain::ToolChainType Qt4BuildConfiguration::toolChainType() const
{
    ToolChain::ToolChainType originalType = ToolChain::ToolChainType(m_toolChainType);
    ToolChain::ToolChainType type = originalType;
    const QtVersion *version = qtVersion();
    if (!version->possibleToolChainTypes().contains(type)) {
        // Oh no the saved type is not valid for this qt version
        // use default tool chain
        type = version->defaultToolchainType();
        const_cast<Qt4BuildConfiguration *>(this)->setToolChainType(type);
    }
    return type;
}

QtVersion::QmakeBuildConfigs Qt4BuildConfiguration::qmakeBuildConfiguration() const
{
    return m_qmakeBuildConfiguration;
}

void Qt4BuildConfiguration::setQMakeBuildConfiguration(QtVersion::QmakeBuildConfigs config)
{
    if (m_qmakeBuildConfiguration == config)
        return;
    m_qmakeBuildConfiguration = config;
    emit qmakeBuildConfigurationChanged();
    emit targetInformationChanged();
}

void Qt4BuildConfiguration::emitQMakeBuildConfigurationChanged()
{
    emit qmakeBuildConfigurationChanged();
}

void Qt4BuildConfiguration::getConfigCommandLineArguments(QStringList *addedUserConfigs, QStringList *removedUserConfigs) const
{
    QtVersion::QmakeBuildConfigs defaultBuildConfiguration = qtVersion()->defaultBuildConfig();
    QtVersion::QmakeBuildConfigs userBuildConfiguration = m_qmakeBuildConfiguration;
    if (removedUserConfigs) {
        if ((defaultBuildConfiguration & QtVersion::BuildAll) && !(userBuildConfiguration & QtVersion::BuildAll))
            (*removedUserConfigs) << "debug_and_release";
    }
    if (addedUserConfigs) {
        if (!(defaultBuildConfiguration & QtVersion::BuildAll) && (userBuildConfiguration & QtVersion::BuildAll))
            (*addedUserConfigs) << "debug_and_release";
        if ((defaultBuildConfiguration & QtVersion::DebugBuild) && !(userBuildConfiguration & QtVersion::DebugBuild))
            (*addedUserConfigs) << "release";
        if (!(defaultBuildConfiguration & QtVersion::DebugBuild) && (userBuildConfiguration & QtVersion::DebugBuild))
            (*addedUserConfigs) << "debug";
    }
}

QMakeStep *Qt4BuildConfiguration::qmakeStep() const
{
    QMakeStep *qs = 0;
    foreach(BuildStep *bs, buildSteps())
        if ((qs = qobject_cast<QMakeStep *>(bs)) != 0)
            return qs;
    return 0;
}

MakeStep *Qt4BuildConfiguration::makeStep() const
{
    MakeStep *qs = 0;
    foreach(BuildStep *bs, buildSteps())
        if ((qs = qobject_cast<MakeStep *>(bs)) != 0)
            return qs;
    return 0;
}

void Qt4BuildConfiguration::defaultQtVersionChanged()
{
    if (qtVersionId() == 0) {
        emit qtVersionChanged();
        emit targetInformationChanged();
        emit environmentChanged();
    }
}

void Qt4BuildConfiguration::qtVersionsChanged(const QList<int> &changedVersions)
{
    if (changedVersions.contains(qtVersionId())) {
        if (!qtVersion()->isValid())
            setQtVersion(0);
        emit qtVersionChanged();
        emit targetInformationChanged();
    }
}

// returns true if both are equal
bool Qt4BuildConfiguration::compareToImportFrom(const QString &workingDirectory)
{
    QMakeStep *qs = qmakeStep();
    if (QDir(workingDirectory).exists(QLatin1String("Makefile")) && qs) {
        QString qmakePath = QtVersionManager::findQMakeBinaryFromMakefile(workingDirectory);
        QtVersion *version = qtVersion();
        if (version->qmakeCommand() == qmakePath) {
            // same qtversion
            QPair<QtVersion::QmakeBuildConfigs, QStringList> result =
                    QtVersionManager::scanMakeFile(workingDirectory, version->defaultBuildConfig());
            if (qmakeBuildConfiguration() == result.first) {
                // The QMake Build Configuration are the same,
                // now compare arguments lists
                // we have to compare without the spec/platform cmd argument
                // and compare that on its own
                QString actualSpec = extractSpecFromArgumentList(qs->userArguments(), workingDirectory, version);
                if (actualSpec.isEmpty()) {
                    // Easy one: the user has chosen not to override the settings
                    actualSpec = version->mkspec();
                }


                QString parsedSpec = extractSpecFromArgumentList(result.second, workingDirectory, version);
                QStringList actualArgs = removeSpecFromArgumentList(qs->userArguments());
                QStringList parsedArgs = removeSpecFromArgumentList(result.second);

                if (debug) {
                    qDebug()<<"Actual args:"<<actualArgs;
                    qDebug()<<"Parsed args:"<<parsedArgs;
                    qDebug()<<"Actual spec:"<<actualSpec;
                    qDebug()<<"Parsed spec:"<<parsedSpec;
                }

                if (actualArgs == parsedArgs) {
                    // Specs match exactly
                    if (actualSpec == parsedSpec)
                        return true;
                    // Actual spec is the default one
//                    qDebug()<<"AS vs VS"<<actualSpec<<version->mkspec();
                    if ((actualSpec == version->mkspec() || actualSpec == "default")
                        && (parsedSpec == version->mkspec() || parsedSpec == "default" || parsedSpec.isEmpty()))
                        return true;
                }
            }
        }
    }
    return false;
}

// We match -spec and -platfrom separetly
// We ignore -cache, because qmake contained a bug that it didn't
// mention the -cache in the Makefile
// That means changing the -cache option in the additional arguments
// does not automatically rerun qmake. Alas, we could try more
// intelligent matching for -cache, but i guess people rarely
// do use that.

QStringList Qt4BuildConfiguration::removeSpecFromArgumentList(const QStringList &old)
{
    if (!old.contains("-spec") && !old.contains("-platform") && !old.contains("-cache"))
        return old;
    QStringList newList;
    bool ignoreNext = false;
    foreach(const QString &item, old) {
        if (ignoreNext) {
            ignoreNext = false;
        } else if (item == "-spec" || item == "-platform" || item == "-cache") {
            ignoreNext = true;
        } else {
            newList << item;
        }
    }
    return newList;
}

QString Qt4BuildConfiguration::extractSpecFromArgumentList(const QStringList &list, QString directory, QtVersion *version)
{
    int index = list.indexOf("-spec");
    if (index == -1)
        index = list.indexOf("-platform");
    if (index == -1)
        return QString();

    ++index;

    if (index >= list.length())
        return QString();

    QString baseMkspecDir = version->versionInfo().value("QMAKE_MKSPECS");
    if (baseMkspecDir.isEmpty())
        baseMkspecDir = version->versionInfo().value("QT_INSTALL_DATA") + "/mkspecs";

    QString parsedSpec = QDir::cleanPath(list.at(index));
#ifdef Q_OS_WIN
    baseMkspecDir = baseMkspecDir.toLower();
    parsedSpec = parsedSpec.toLower();
#endif
    // if the path is relative it can be
    // relative to the working directory (as found in the Makefiles)
    // or relatively to the mkspec directory
    // if it is the former we need to get the canonical form
    // for the other one we don't need to do anything
    if (QFileInfo(parsedSpec).isRelative()) {
        if(QFileInfo(directory + QLatin1Char('/') + parsedSpec).exists()) {
            parsedSpec = QDir::cleanPath(directory + QLatin1Char('/') + parsedSpec);
#ifdef Q_OS_WIN
            parsedSpec = parsedSpec.toLower();
#endif
        } else {
            parsedSpec = baseMkspecDir + QLatin1Char('/') + parsedSpec;
        }
    }

    QFileInfo f2(parsedSpec);
    while (f2.isSymLink()) {
        parsedSpec = f2.symLinkTarget();
        f2.setFile(parsedSpec);
    }

    if (parsedSpec.startsWith(baseMkspecDir)) {
        parsedSpec = parsedSpec.mid(baseMkspecDir.length() + 1);
    } else {
        QString sourceMkSpecPath = version->sourcePath() + "/mkspecs";
        if (parsedSpec.startsWith(sourceMkSpecPath)) {
            parsedSpec = parsedSpec.mid(sourceMkSpecPath.length() + 1);
        }
    }
#ifdef Q_OS_WIN
    parsedSpec = parsedSpec.toLower();
#endif
    return parsedSpec;
}

/*!
  \class Qt4BuildConfigurationFactory
*/

Qt4BuildConfigurationFactory::Qt4BuildConfigurationFactory(QObject *parent) :
    ProjectExplorer::IBuildConfigurationFactory(parent)
{
    update();

    QtVersionManager *vm = QtVersionManager::instance();
    connect(vm, SIGNAL(defaultQtVersionChanged()),
            this, SLOT(update()));
    connect(vm, SIGNAL(qtVersionsChanged(QList<int>)),
            this, SLOT(update()));
}

Qt4BuildConfigurationFactory::~Qt4BuildConfigurationFactory()
{
}

void Qt4BuildConfigurationFactory::update()
{
    m_versions.clear();
    m_versions.insert(QString::fromLatin1(QT4_BC_ID_PREFIX) + QLatin1String("DefaultQt"), VersionInfo(tr("Using Default Qt Version"), 0));
    QtVersionManager *vm = QtVersionManager::instance();
    foreach (const QtVersion *version, vm->versions()) {
        m_versions.insert(QString::fromLatin1(QT4_BC_ID_PREFIX) + QString::fromLatin1("Qt%1").arg(version->uniqueId()),
                          VersionInfo(tr("Using Qt Version \"%1\"").arg(version->displayName()), version->uniqueId()));
    }
    emit availableCreationIdsChanged();
}

QStringList Qt4BuildConfigurationFactory::availableCreationIds(ProjectExplorer::Project *parent) const
{
    if (!qobject_cast<Qt4Project *>(parent))
        return QStringList();
    return m_versions.keys();
}

QString Qt4BuildConfigurationFactory::displayNameForId(const QString &id) const
{
    if (!m_versions.contains(id))
        return QString();
    return m_versions.value(id).displayName;
}

bool Qt4BuildConfigurationFactory::canCreate(ProjectExplorer::Project *parent, const QString &id) const
{
    if (!qobject_cast<Qt4Project *>(parent))
        return false;
    if (!m_versions.contains(id))
        return false;
    const VersionInfo &info = m_versions.value(id);
    QtVersion *version = QtVersionManager::instance()->version(info.versionId);
    if (!version)
        return false;
    return true;
}

BuildConfiguration *Qt4BuildConfigurationFactory::create(ProjectExplorer::Project *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;

    const VersionInfo &info = m_versions.value(id);
    QtVersion *version = QtVersionManager::instance()->version(info.versionId);
    Q_ASSERT(version);

    Qt4Project *qt4project(static_cast<Qt4Project *>(parent));

    bool ok;
    QString buildConfigurationName = QInputDialog::getText(0,
                          tr("New configuration"),
                          tr("New Configuration Name:"),
                          QLineEdit::Normal,
                          version->displayName(),
                          &ok);
    if (!ok || buildConfigurationName.isEmpty())
        return false;

    qt4project->addQt4BuildConfiguration(tr("%1 Debug").arg(buildConfigurationName),
                                         version,
                                         (version->defaultBuildConfig() | QtVersion::DebugBuild));
    BuildConfiguration *bc =
    qt4project->addQt4BuildConfiguration(tr("%1 Release").arg(buildConfigurationName),
                                         version,
                                         (version->defaultBuildConfig() & ~QtVersion::DebugBuild));
    return bc;
}

bool Qt4BuildConfigurationFactory::canClone(ProjectExplorer::Project *parent, ProjectExplorer::BuildConfiguration *source) const
{
    return canCreate(parent, source->id());
}

BuildConfiguration *Qt4BuildConfigurationFactory::clone(ProjectExplorer::Project *parent, BuildConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    Qt4Project *project(static_cast<Qt4Project *>(parent));
    Qt4BuildConfiguration *oldbc(static_cast<Qt4BuildConfiguration *>(source));
    return new Qt4BuildConfiguration(project, oldbc);
}

bool Qt4BuildConfigurationFactory::canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const
{
    QString id(ProjectExplorer::idFromMap(map));
    if (!qobject_cast<Qt4Project *>(parent))
        return false;
    return id.startsWith(QLatin1String(QT4_BC_ID_PREFIX)) ||
           id == QLatin1String(QT4_BC_ID);
}

BuildConfiguration *Qt4BuildConfigurationFactory::restore(ProjectExplorer::Project *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    Qt4Project *project(static_cast<Qt4Project *>(parent));
    Qt4BuildConfiguration *bc(new Qt4BuildConfiguration(project));
    if (bc->fromMap(map))
        return bc;
    delete bc;
    return 0;
}
