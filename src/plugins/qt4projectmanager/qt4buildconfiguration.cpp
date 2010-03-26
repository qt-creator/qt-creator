/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include "qt4target.h"
#include "qt4projectmanagerconstants.h"
#include "qt4nodes.h"
#include "qmakestep.h"
#include "makestep.h"

#include <utils/qtcassert.h>

#include <QtCore/QDebug>

#include <QtGui/QInputDialog>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;
using namespace ProjectExplorer;

namespace {
const char * const QT4_BC_ID_PREFIX("Qt4ProjectManager.Qt4BuildConfiguration.");
const char * const QT4_BC_ID("Qt4ProjectManager.Qt4BuildConfiguration");

const char * const USE_SHADOW_BUILD_KEY("Qt4ProjectManager.Qt4BuildConfiguration.UseShadowBuild");
const char * const BUILD_DIRECTORY_KEY("Qt4ProjectManager.Qt4BuildConfiguration.BuildDirectory");
const char * const TOOLCHAIN_KEY("Qt4ProjectManager.Qt4BuildConfiguration.ToolChain");
const char * const BUILD_CONFIGURATION_KEY("Qt4ProjectManager.Qt4BuildConfiguration.BuildConfiguration");
const char * const QT_VERSION_ID_KEY("Qt4ProjectManager.Qt4BuildConfiguration.QtVersionId");

enum { debug = 0 };
}

Qt4BuildConfiguration::Qt4BuildConfiguration(Qt4Target *target) :
    BuildConfiguration(target, QLatin1String(QT4_BC_ID)),
    m_shadowBuild(true),
    m_qtVersionId(-1),
    m_toolChainType(-1), // toolChainType() makes sure to return the default toolchainType
    m_qmakeBuildConfiguration(0),
    m_subNodeBuild(0)
{
    ctor();
}

Qt4BuildConfiguration::Qt4BuildConfiguration(Qt4Target *target, const QString &id) :
    BuildConfiguration(target, id),
    m_shadowBuild(true),
    m_qtVersionId(-1),
    m_toolChainType(-1), // toolChainType() makes sure to return the default toolchainType
    m_qmakeBuildConfiguration(0),
    m_subNodeBuild(0)
{
    ctor();
}

Qt4BuildConfiguration::Qt4BuildConfiguration(Qt4Target *target, Qt4BuildConfiguration *source) :
    BuildConfiguration(target, source),
    m_shadowBuild(source->m_shadowBuild),
    m_buildDirectory(source->m_buildDirectory),
    m_qtVersionId(source->m_qtVersionId),
    m_toolChainType(source->m_toolChainType),
    m_qmakeBuildConfiguration(source->m_qmakeBuildConfiguration),
    m_subNodeBuild(0) // temporary value, so not copied
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
    map.insert(QLatin1String(QT_VERSION_ID_KEY), m_qtVersionId);
    map.insert(QLatin1String(TOOLCHAIN_KEY), m_toolChainType);
    map.insert(QLatin1String(BUILD_CONFIGURATION_KEY), int(m_qmakeBuildConfiguration));
    return map;
}


bool Qt4BuildConfiguration::fromMap(const QVariantMap &map)
{
    if (!BuildConfiguration::fromMap(map))
        return false;

    m_shadowBuild = map.value(QLatin1String(USE_SHADOW_BUILD_KEY), true).toBool();
    m_buildDirectory = map.value(QLatin1String(BUILD_DIRECTORY_KEY), qt4Target()->defaultBuildDirectory()).toString();
    m_qtVersionId = map.value(QLatin1String(QT_VERSION_ID_KEY)).toInt();
    m_toolChainType = map.value(QLatin1String(TOOLCHAIN_KEY)).toInt();
    m_qmakeBuildConfiguration = QtVersion::QmakeBuildConfigs(map.value(QLatin1String(BUILD_CONFIGURATION_KEY)).toInt());

    // Pick a Qt version if the default version is used:
    // We assume that the default Qt version as used in earlier versions of Qt creator
    // was supporting a desktop flavor of Qt.
    if (m_qtVersionId == 0) {
        QList<QtVersion *> versions = QtVersionManager::instance()->versions();
        foreach (QtVersion *v, versions) {
            if (v->isValid() && v->supportsTargetId(QLatin1String(Constants::DESKTOP_TARGET_ID))) {
                m_qtVersionId = v->uniqueId();
                break;
            }
        }
        if (m_qtVersionId == 0)
            m_qtVersionId = versions.at(0)->uniqueId();
    }

    if (!qtVersion()->isValid() || !qtVersion()->supportedTargetIds().contains(target()->id())) {
        qWarning() << "Buildconfiguration" << displayName() << ": Qt" << qtVersion()->displayName() << "not supported by target" << target()->id();
        return false;
    }

    QList<ToolChain::ToolChainType> possibleTcs(qt4Target()->filterToolChainTypes(qtVersion()->possibleToolChainTypes()));
    if (!possibleTcs.contains(toolChainType()))
        setToolChainType(qt4Target()->preferredToolChainType(possibleTcs));

    if (toolChainType() == ToolChain::INVALID) {
        qWarning() << "No toolchain available for" << qtVersion()->displayName() << "used in" << target()->id() << "!";
        return false;
    }
    return true;
}

void Qt4BuildConfiguration::ctor()
{
    m_buildDirectory = qt4Target()->defaultBuildDirectory();
    if (m_buildDirectory == target()->project()->projectDirectory())
        m_shadowBuild = false;

    QtVersionManager *vm = QtVersionManager::instance();
    connect(vm, SIGNAL(qtVersionsChanged(QList<int>)),
            this, SLOT(qtVersionsChanged(QList<int>)));
}

void Qt4BuildConfiguration::pickValidQtVersion()
{
    QList<QtVersion *> versions = QtVersionManager::instance()->versionsForTargetId(qt4Target()->id());
    if (!versions.isEmpty())
        setQtVersion(versions.at(0));
    else
        setQtVersion(QtVersionManager::instance()->emptyVersion());
}

Qt4Target *Qt4BuildConfiguration::qt4Target() const
{
    return static_cast<Qt4Target *>(target());
}

ProjectExplorer::Environment Qt4BuildConfiguration::baseEnvironment() const
{
    Environment env = BuildConfiguration::baseEnvironment();
    qtVersion()->addToEnvironment(env);
    ToolChain *tc = toolChain();
    if (tc)
        tc->addToEnvironment(env);
    return env;
}

/// returns the build directory
QString Qt4BuildConfiguration::buildDirectory() const
{
    QString workingDirectory;
    if (m_shadowBuild)
        workingDirectory = m_buildDirectory;
    if (workingDirectory.isEmpty())
        workingDirectory = target()->project()->projectDirectory();
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
    emit proFileEvaluateNeeded(this);
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
        return symbianMakeTarget(buildConfig, QLatin1String("gcce"));
    case ToolChain::RVCT_ARMV5:
        return symbianMakeTarget(buildConfig, QLatin1String("armv5"));
    case ToolChain::RVCT_ARMV6:
        return symbianMakeTarget(buildConfig, QLatin1String("armv6"));
    case ToolChain::RVCT_ARMV5_GNUPOC:
    case ToolChain::GCCE_GNUPOC:
    default:
        break;
    }
    return QString();
}

QtVersion *Qt4BuildConfiguration::qtVersion() const
{
    QtVersionManager *vm = QtVersionManager::instance();
    return vm->version(m_qtVersionId);
}

void Qt4BuildConfiguration::setQtVersion(QtVersion *version)
{
    Q_ASSERT(version);

    if (m_qtVersionId == version->uniqueId())
        return;

    m_qtVersionId = version->uniqueId();

    emit proFileEvaluateNeeded(this);
    emit qtVersionChanged();
    emit environmentChanged();
}

void Qt4BuildConfiguration::setToolChainType(ProjectExplorer::ToolChain::ToolChainType type)
{
    if (!qt4Target()->filterToolChainTypes(qtVersion()->possibleToolChainTypes()).contains(type) ||
        m_toolChainType == type)
        return;
    m_toolChainType = type;

    emit proFileEvaluateNeeded(this);
    emit toolChainTypeChanged();
    emit environmentChanged();
}

ProjectExplorer::ToolChain::ToolChainType Qt4BuildConfiguration::toolChainType() const
{
    return ToolChain::ToolChainType(m_toolChainType);
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

    emit proFileEvaluateNeeded(this);
    emit qmakeBuildConfigurationChanged();
}

void Qt4BuildConfiguration::emitQMakeBuildConfigurationChanged()
{
    emit qmakeBuildConfigurationChanged();
}

void Qt4BuildConfiguration::emitBuildDirectoryInitialized()
{
    emit buildDirectoryInitialized();
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
    foreach(BuildStep *bs, steps(ProjectExplorer::Build))
        if ((qs = qobject_cast<QMakeStep *>(bs)) != 0)
            return qs;
    return 0;
}

MakeStep *Qt4BuildConfiguration::makeStep() const
{
    MakeStep *qs = 0;
    foreach(BuildStep *bs, steps(ProjectExplorer::Build))
        if ((qs = qobject_cast<MakeStep *>(bs)) != 0)
            return qs;
    return 0;
}

void Qt4BuildConfiguration::qtVersionsChanged(const QList<int> &changedVersions)
{
    if (!changedVersions.contains(m_qtVersionId) ||
        qtVersion()->isValid())
        return;

    pickValidQtVersion();
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
                // The qmake Build Configuration are the same,
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
    connect(vm, SIGNAL(qtVersionsChanged(QList<int>)),
            this, SLOT(update()));
}

Qt4BuildConfigurationFactory::~Qt4BuildConfigurationFactory()
{
}

void Qt4BuildConfigurationFactory::update()
{
    m_versions.clear();
    QtVersionManager *vm = QtVersionManager::instance();
    foreach (const QtVersion *version, vm->versions()) {
        m_versions.insert(QString::fromLatin1(QT4_BC_ID_PREFIX) + QString::fromLatin1("Qt%1").arg(version->uniqueId()),
                          VersionInfo(tr("Using Qt Version \"%1\"").arg(version->displayName()), version->uniqueId()));
    }
    emit availableCreationIdsChanged();
}

QStringList Qt4BuildConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const
{
    if (!qobject_cast<Qt4Target *>(parent))
        return QStringList();

    QStringList results;
    QtVersionManager *vm = QtVersionManager::instance();
    for (QMap<QString, VersionInfo>::const_iterator i = m_versions.constBegin();
         i != m_versions.constEnd(); ++i) {
        if (vm->version(i.value().versionId)->supportsTargetId(parent->id()))
            results.append(i.key());
    }
    return results;
}

QString Qt4BuildConfigurationFactory::displayNameForId(const QString &id) const
{
    if (!m_versions.contains(id))
        return QString();
    return m_versions.value(id).displayName;
}

bool Qt4BuildConfigurationFactory::canCreate(ProjectExplorer::Target *parent, const QString &id) const
{
    if (!qobject_cast<Qt4Target *>(parent))
        return false;
    if (!m_versions.contains(id))
        return false;
    const VersionInfo &info = m_versions.value(id);
    QtVersion *version = QtVersionManager::instance()->version(info.versionId);
    if (!version ||
        !version->supportsTargetId(parent->id()))
        return false;
    return true;
}

BuildConfiguration *Qt4BuildConfigurationFactory::create(ProjectExplorer::Target *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;

    const VersionInfo &info = m_versions.value(id);
    QtVersion *version = QtVersionManager::instance()->version(info.versionId);
    Q_ASSERT(version);

    Qt4Target *qt4Target(static_cast<Qt4Target *>(parent));

    bool ok;
    QString buildConfigurationName = QInputDialog::getText(0,
                          tr("New configuration"),
                          tr("New Configuration Name:"),
                          QLineEdit::Normal,
                          version->displayName(),
                          &ok);
    if (!ok || buildConfigurationName.isEmpty())
        return false;

    qt4Target->addQt4BuildConfiguration(tr("%1 Debug").arg(buildConfigurationName),
                                        version,
                                        (version->defaultBuildConfig() | QtVersion::DebugBuild),
                                        QStringList(), QString());
    BuildConfiguration *bc =
    qt4Target->addQt4BuildConfiguration(tr("%1 Release").arg(buildConfigurationName),
                                        version,
                                        (version->defaultBuildConfig() & ~QtVersion::DebugBuild),
                                        QStringList(), QString());
    return bc;
}

bool Qt4BuildConfigurationFactory::canClone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) const
{
    if (!qobject_cast<Qt4Target *>(parent))
        return false;
    Qt4BuildConfiguration *qt4bc(qobject_cast<Qt4BuildConfiguration *>(source));
    if (!qt4bc)
        return false;

    QtVersion *version = qt4bc->qtVersion();
    if (!version ||
        !version->supportsTargetId(parent->id()))
        return false;
    return true;
}

BuildConfiguration *Qt4BuildConfigurationFactory::clone(Target *parent, BuildConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    Qt4Target *target(static_cast<Qt4Target *>(parent));
    Qt4BuildConfiguration *oldbc(static_cast<Qt4BuildConfiguration *>(source));
    return new Qt4BuildConfiguration(target, oldbc);
}

bool Qt4BuildConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    QString id(ProjectExplorer::idFromMap(map));
    if (!qobject_cast<Qt4Target *>(parent))
        return false;
    return id.startsWith(QLatin1String(QT4_BC_ID_PREFIX)) ||
           id == QLatin1String(QT4_BC_ID);
}

BuildConfiguration *Qt4BuildConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    Qt4Target *target(static_cast<Qt4Target *>(parent));
    Qt4BuildConfiguration *bc(new Qt4BuildConfiguration(target));
    if (bc->fromMap(map))
        return bc;
    delete bc;
    return 0;
}
