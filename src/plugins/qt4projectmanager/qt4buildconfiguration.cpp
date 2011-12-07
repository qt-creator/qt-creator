/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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
#include "qt4target.h"
#include "qt4projectmanagerconstants.h"
#include "qt4nodes.h"
#include "qmakestep.h"
#include "makestep.h"
#include "qt4basetargetfactory.h"

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <limits>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>
#include <qtsupport/qtversionfactory.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtversionmanager.h>
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
const char * const BUILD_CONFIGURATION_KEY("Qt4ProjectManager.Qt4BuildConfiguration.BuildConfiguration");
const char * const QT_VERSION_ID_KEY("Qt4ProjectManager.Qt4BuildConfiguration.QtVersionId");

enum { debug = 0 };
}

Qt4BuildConfiguration::Qt4BuildConfiguration(Qt4BaseTarget *target) :
    BuildConfiguration(target, QLatin1String(QT4_BC_ID)),
    m_shadowBuild(true),
    m_isEnabled(false),
    m_qtVersionId(-1),
    m_qmakeBuildConfiguration(0),
    m_subNodeBuild(0)
{
    ctor();
}

Qt4BuildConfiguration::Qt4BuildConfiguration(Qt4BaseTarget *target, const QString &id) :
    BuildConfiguration(target, id),
    m_shadowBuild(true),
    m_isEnabled(false),
    m_qtVersionId(-1),
    m_qmakeBuildConfiguration(0),
    m_subNodeBuild(0)
{
    ctor();
}

Qt4BuildConfiguration::Qt4BuildConfiguration(Qt4BaseTarget *target, Qt4BuildConfiguration *source) :
    BuildConfiguration(target, source),
    m_shadowBuild(source->m_shadowBuild),
    m_isEnabled(false),
    m_buildDirectory(source->m_buildDirectory),
    m_qtVersionId(source->m_qtVersionId),
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
    map.insert(QLatin1String(BUILD_CONFIGURATION_KEY), int(m_qmakeBuildConfiguration));
    return map;
}

static inline QString msgBuildConfigNotApplicable(const QString &d, const QtSupport::BaseQtVersion *qtVersion,
                                                  const Target *target)
{
    return QString::fromLatin1("Warning: Buildconfiguration '%1' : Qt '%2' from %3 not supported by target '%4'").
            arg(d, qtVersion->displayName(), qtVersion->qmakeCommand().toUserOutput(), target->id());
}

bool Qt4BuildConfiguration::fromMap(const QVariantMap &map)
{
    if (!BuildConfiguration::fromMap(map))
        return false;

    m_shadowBuild = map.value(QLatin1String(USE_SHADOW_BUILD_KEY), true).toBool();
    m_qtVersionId = map.value(QLatin1String(QT_VERSION_ID_KEY)).toInt();
    ProjectExplorer::ToolChain *tc = toolChain();
    m_qmakeBuildConfiguration = QtSupport::BaseQtVersion::QmakeBuildConfigs(map.value(QLatin1String(BUILD_CONFIGURATION_KEY)).toInt());
    m_buildDirectory = map.value(QLatin1String(BUILD_DIRECTORY_KEY), defaultShadowBuildDirectory()).toString();

    // Pick a Qt version if the default version is used:
    // We assume that the default Qt version was used in earlier versions of Qt creator.
    // Pick a Qt version that supports this target (usually desktop)
    if (m_qtVersionId == 0) {
        QList<QtSupport::BaseQtVersion *> versions = QtSupport::QtVersionManager::instance()->versionsForTargetId(target()->id());
        foreach (QtSupport::BaseQtVersion *v, versions) {
            if (v->isValid()) {
                m_qtVersionId = v->uniqueId();
                break;
            }
        }
        if (m_qtVersionId == 0)
            m_qtVersionId = -1;
    }

    QtSupport::BaseQtVersion *version = QtSupport::QtVersionManager::instance()->version(m_qtVersionId);
    if (!version || !version->supportsTargetId(target()->id())) {
        m_qtVersionId = -1;
        version = 0;
    }

    m_lastEmmitedBuildDirectory = buildDirectory();

    if (version && version->isValid()) {
        if (tc && !qt4Target()->possibleToolChains(this).contains(tc))
            setToolChain(0);
        if (!toolChain())
            setToolChain(qt4Target()->preferredToolChain(this));
        m_shadowBuild = (m_shadowBuild && version->supportsShadowBuilds());
    }

    if (!toolChain()) {
        if (version && version->isValid()) {
            qWarning("Warning: No tool chain available for '%s' from %s used in '%s'.",
                    qPrintable(version->displayName()), qPrintable(version->qmakeCommand().toUserOutput()),
                    qPrintable(target()->id()));
        } else {
            qWarning("Warning: No tool chain available for invalid Qt version used in '%s'.",
                     qPrintable(target()->id()));
        }
    }

    return true;
}

void Qt4BuildConfiguration::ctor()
{
    connect(this, SIGNAL(environmentChanged()),
            this, SLOT(emitBuildDirectoryChanged()));
    connect(this, SIGNAL(environmentChanged()),
            this, SLOT(emitProFileEvaluateNeeded()));
    connect(qt4Target()->qt4Project(), SIGNAL(proFileUpdated(Qt4ProjectManager::Qt4ProFileNode*,bool,bool)),
            this, SLOT(proFileUpdated(Qt4ProjectManager::Qt4ProFileNode*,bool,bool)));

    QtSupport::QtVersionManager *vm = QtSupport::QtVersionManager::instance();
    connect(vm, SIGNAL(qtVersionsChanged(QList<int>)),
            this, SLOT(qtVersionsChanged(QList<int>)));
}

void Qt4BuildConfiguration::emitBuildDirectoryChanged()
{
    if (buildDirectory() != m_lastEmmitedBuildDirectory) {
        m_lastEmmitedBuildDirectory = buildDirectory();
        emit buildDirectoryChanged();
    }
}

void Qt4BuildConfiguration::proFileUpdated(Qt4ProjectManager::Qt4ProFileNode *, bool success, bool parseInProgress)
{
    // Changing the included Qt modules from 0 to at least one might have caused the
    // tool chain to become invalid.
    if (!success || parseInProgress)
        return;
    if (!qt4Target()->possibleToolChains(this).contains(toolChain()))
        setToolChain(qt4Target()->preferredToolChain(this));
}

Qt4BaseTarget *Qt4BuildConfiguration::qt4Target() const
{
    return static_cast<Qt4BaseTarget *>(target());
}

Utils::Environment Qt4BuildConfiguration::baseEnvironment() const
{
    Utils::Environment env = BuildConfiguration::baseEnvironment();
    if (qtVersion())
        qtVersion()->addToEnvironment(env);

    ToolChain *tc = toolChain();
    if (tc)
        tc->addToEnvironment(env);
    return env;
}

QString Qt4BuildConfiguration::defaultShadowBuildDirectory() const
{
    Qt4BaseTargetFactory *factory = Qt4BaseTargetFactory::qt4BaseTargetFactoryForId(qt4Target()->id());
    // todo displayName isn't ideal
    return factory->shadowBuildDirectory(qt4Target()->qt4Project()->file()->fileName(), qt4Target()->id(), displayName());
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
    QtSupport::BaseQtVersion *version = qtVersion();
    QString directoryToSet = buildDirectory;
    bool toSet = (shadowBuild && version && version->isValid() && version->supportsShadowBuilds());
    if (m_shadowBuild == toSet && m_buildDirectory == directoryToSet)
        return;

    m_shadowBuild = toSet;
    m_buildDirectory = directoryToSet;

    emit environmentChanged();
    emitBuildDirectoryChanged();
    emit proFileEvaluateNeeded(this);
}

QString Qt4BuildConfiguration::makeCommand() const
{
    ToolChain *tc = toolChain();
    return tc ? tc->makeCommand() : "make";
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
    ToolChain *tc = toolChain();
    if (!tc || target()->id() != Constants::S60_DEVICE_TARGET_ID)
        return QString();
    const QtSupport::BaseQtVersion::QmakeBuildConfigs buildConfig = qmakeBuildConfiguration();

    return symbianMakeTarget(buildConfig, tc->defaultMakeTarget());
}

QString Qt4BuildConfiguration::makefile() const
{
    return qt4Target()->qt4Project()->rootQt4ProjectNode()->makefile();
}

QtSupport::BaseQtVersion *Qt4BuildConfiguration::qtVersion() const
{
    QtSupport::QtVersionManager *vm = QtSupport::QtVersionManager::instance();
    return vm->version(m_qtVersionId);
}

void Qt4BuildConfiguration::setQtVersion(QtSupport::BaseQtVersion *version)
{
    if (version == 0) {
        m_qtVersionId = -1;
        m_shadowBuild = false;
        setToolChain(0);
        emit proFileEvaluateNeeded(this);
        emit qtVersionChanged();
        emit environmentChanged();
        emitBuildDirectoryChanged();
        return;
    }
    if (m_qtVersionId == version->uniqueId())
            return;

    m_qtVersionId = version->uniqueId();

    if (!qt4Target()->possibleToolChains(this).contains(toolChain()))
        setToolChain(qt4Target()->preferredToolChain(this));
    m_shadowBuild = m_shadowBuild && qtVersion()->supportsShadowBuilds();

    emit proFileEvaluateNeeded(this);
    emit qtVersionChanged();
    emit environmentChanged();
    emitBuildDirectoryChanged();
}

void Qt4BuildConfiguration::setToolChain(ProjectExplorer::ToolChain *tc)
{
    if (tc != 0 && !qt4Target()->possibleToolChains(this).contains(tc))
        return;

    if (toolChain() == tc)
        return;

    BuildConfiguration::setToolChain(tc);

    emit proFileEvaluateNeeded(this);
    emitBuildDirectoryChanged();
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

    emit proFileEvaluateNeeded(this);
    emit qmakeBuildConfigurationChanged();
    emitBuildDirectoryChanged();
}

void Qt4BuildConfiguration::emitProFileEvaluateNeeded()
{
    emit proFileEvaluateNeeded(this);
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
    QtSupport::BaseQtVersion::QmakeBuildConfigs defaultBuildConfiguration =  qtVersion() ? qtVersion()->defaultBuildConfig() : (QtSupport::BaseQtVersion::DebugBuild | QtSupport::BaseQtVersion::BuildAll);
    QtSupport::BaseQtVersion::QmakeBuildConfigs userBuildConfiguration = m_qmakeBuildConfiguration;
    if ((defaultBuildConfiguration & QtSupport::BaseQtVersion::BuildAll) && !(userBuildConfiguration & QtSupport::BaseQtVersion::BuildAll))
        result << "CONFIG-=debug_and_release";

    if (!(defaultBuildConfiguration & QtSupport::BaseQtVersion::BuildAll) && (userBuildConfiguration & QtSupport::BaseQtVersion::BuildAll))
        result << "CONFIG+=debug_and_release";
    if ((defaultBuildConfiguration & QtSupport::BaseQtVersion::DebugBuild) && !(userBuildConfiguration & QtSupport::BaseQtVersion::DebugBuild))
        result << "CONFIG+=release";
    if (!(defaultBuildConfiguration & QtSupport::BaseQtVersion::DebugBuild) && (userBuildConfiguration & QtSupport::BaseQtVersion::DebugBuild))
        result << "CONFIG+=debug";
    return result;
}

QMakeStep *Qt4BuildConfiguration::qmakeStep() const
{
    QMakeStep *qs = 0;
    BuildStepList *bsl = stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    Q_ASSERT(bsl);
    for (int i = 0; i < bsl->count(); ++i)
        if ((qs = qobject_cast<QMakeStep *>(bsl->at(i))) != 0)
            return qs;
    return 0;
}

MakeStep *Qt4BuildConfiguration::makeStep() const
{
    MakeStep *ms = 0;
    BuildStepList *bsl = stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    Q_ASSERT(bsl);
    for (int i = 0; i < bsl->count(); ++i)
        if ((ms = qobject_cast<MakeStep *>(bsl->at(i))) != 0)
            return ms;
    return 0;
}

void Qt4BuildConfiguration::qtVersionsChanged(const QList<int> &changedVersions)
{
    if (!changedVersions.contains(m_qtVersionId))
        return;
    emit environmentChanged(); // Our qt version changed, that might have changed the environment
}

// returns true if both are equal
bool Qt4BuildConfiguration::compareToImportFrom(const QString &makefile)
{
    QMakeStep *qs = qmakeStep();
    if (QFileInfo(makefile).exists() && qs) {
        Utils::FileName qmakePath = QtSupport::QtVersionManager::findQMakeBinaryFromMakefile(makefile);
        QtSupport::BaseQtVersion *version = qtVersion();
        if (!version)
            return false;
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
                extractSpecFromArguments(&userArgs, workingDirectory, version, &actualArgs),
                actualArgs = qs->moreArguments() + actualArgs + qs->moreArgumentsAfter();
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
                        return true;
                    // Actual spec is the default one
//                    qDebug()<<"AS vs VS"<<actualSpec<<version->mkspec();
                    if ((actualSpec == version->mkspec() || actualSpec == Utils::FileName::fromString(QLatin1String("default")))
                        && (parsedSpec == version->mkspec() || parsedSpec == Utils::FileName::fromString(QLatin1String("default")) || parsedSpec.isEmpty()))
                        return true;
                }
            } else if (debug) {
                qDebug()<<"different qmake buildconfigurations buildconfiguration:"<<qmakeBuildConfiguration()<<" Makefile:"<<result.first;
            }
        } else if (debug) {
            qDebug()<<"diffrent qt versions, buildconfiguration:"<<version->qmakeCommand().toString()<<" Makefile:"<<qmakePath.toString();
        }
    }
    return false;
}

bool Qt4BuildConfiguration::removeQMLInspectorFromArguments(QString *args)
{
    bool removedArgument = false;
    for (Utils::QtcProcess::ArgIterator ait(args); ait.next(); ) {
        const QString arg = ait.value();
        if (arg.contains(QLatin1String(Constants::QMAKEVAR_QMLJSDEBUGGER_PATH))
                || arg.contains(Constants::QMAKEVAR_DECLARATIVE_DEBUG)) {
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

    Utils::FileName baseMkspecDir = Utils::FileName::fromUserInput(version->versionInfo().value("QMAKE_MKSPECS"));
    if (baseMkspecDir.isEmpty())
        baseMkspecDir = Utils::FileName::fromUserInput(version->versionInfo().value("QT_INSTALL_DATA") + "/mkspecs");

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
        Utils::FileName sourceMkSpecPath = Utils::FileName::fromString(version->sourcePath().toString() + "/mkspecs");
        if (parsedSpec.isChildOf(sourceMkSpecPath)) {
            parsedSpec = parsedSpec.relativeChildPath(sourceMkSpecPath);
        }
    }
    return parsedSpec;
}

ProjectExplorer::IOutputParser *Qt4BuildConfiguration::createOutputParser() const
{
    ToolChain *tc = toolChain();
    if (tc)
        return toolChain()->outputParser();
    return 0;
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
    connect(vm, SIGNAL(qtVersionsChanged(QList<int>)),
            this, SLOT(update()));
}

Qt4BuildConfigurationFactory::~Qt4BuildConfigurationFactory()
{
}

void Qt4BuildConfigurationFactory::update()
{
    m_versions.clear();
    QtSupport::QtVersionManager *vm = QtSupport::QtVersionManager::instance();
    foreach (QtSupport::BaseQtVersion *version, vm->validVersions()) {
        QString key = QString::fromLatin1(QT4_BC_ID_PREFIX)
                + QString::fromLatin1("Qt%1").arg(version->uniqueId());
        VersionInfo info(tr("Using Qt Version \"%1\"").arg(version->displayName()), version->uniqueId());
        m_versions.insert(key, info);
    }
    emit availableCreationIdsChanged();
}

QStringList Qt4BuildConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const
{
    if (!qobject_cast<Qt4BaseTarget *>(parent))
        return QStringList();

    QStringList results;
    QtSupport::QtVersionManager *vm = QtSupport::QtVersionManager::instance();
    for (QMap<QString, VersionInfo>::const_iterator i = m_versions.constBegin();
         i != m_versions.constEnd(); ++i) {
        if (vm->version(i.value().versionId)->supportsTargetId(parent->id())
                && vm->version(i.value().versionId)->toolChainAvailable(parent->id()))
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
    if (!qobject_cast<Qt4BaseTarget *>(parent))
        return false;
    if (!m_versions.contains(id))
        return false;
    const VersionInfo &info = m_versions.value(id);
    QtSupport::BaseQtVersion *version = QtSupport::QtVersionManager::instance()->version(info.versionId);
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
    QtSupport::BaseQtVersion *version = QtSupport::QtVersionManager::instance()->version(info.versionId);
    Q_ASSERT(version);

    Qt4BaseTarget *qt4Target = static_cast<Qt4BaseTarget *>(parent);

    bool ok;
    QString buildConfigurationName = QInputDialog::getText(0,
                          tr("New Configuration"),
                          tr("New configuration name:"),
                          QLineEdit::Normal,
                          version->displayName(),
                          &ok);
    buildConfigurationName = buildConfigurationName.trimmed();
    if (!ok || buildConfigurationName.isEmpty())
        return 0;

    //: Debug build configuration. We recommend not translating it.
    QString defaultDebugName = tr("%1 Debug").arg(version->displayName());
    QString customDebugName;
    if (buildConfigurationName != version->displayName())
        customDebugName = tr("%1 Debug").arg(buildConfigurationName);

    BuildConfiguration *bc = qt4Target->addQt4BuildConfiguration(defaultDebugName, customDebugName,
                                        version,
                                        (version->defaultBuildConfig() | QtSupport::BaseQtVersion::DebugBuild),
                                        QString(), QString(), false);

    if (qt4Target->id() != Constants::S60_EMULATOR_TARGET_ID) {
        //: Release build configuration. We recommend not translating it.
        QString defaultReleaseName = tr("%1 Release").arg(version->displayName());
        QString customReleaseName;
        if (buildConfigurationName != version->displayName())
            customReleaseName = tr("%1 Release").arg(buildConfigurationName);

        bc = qt4Target->addQt4BuildConfiguration(defaultReleaseName, customReleaseName,
                                                 version,
                                                 (version->defaultBuildConfig() & ~QtSupport::BaseQtVersion::DebugBuild),
                                                 QString(), QString(), false);
    }
    return bc;
}

bool Qt4BuildConfigurationFactory::canClone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) const
{
    if (!qobject_cast<Qt4BaseTarget *>(parent))
        return false;
    Qt4BuildConfiguration *qt4bc(qobject_cast<Qt4BuildConfiguration *>(source));
    if (!qt4bc)
        return false;

    QtSupport::BaseQtVersion *version = qt4bc->qtVersion();
    if (!version ||
        !version->supportsTargetId(parent->id()))
        return false;
    return true;
}

BuildConfiguration *Qt4BuildConfigurationFactory::clone(Target *parent, BuildConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    Qt4BaseTarget *target = static_cast<Qt4BaseTarget *>(parent);
    Qt4BuildConfiguration *oldbc(static_cast<Qt4BuildConfiguration *>(source));
    return new Qt4BuildConfiguration(target, oldbc);
}

bool Qt4BuildConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    QString id = ProjectExplorer::idFromMap(map);
    if (!qobject_cast<Qt4BaseTarget *>(parent))
        return false;
    return id.startsWith(QLatin1String(QT4_BC_ID_PREFIX)) ||
           id == QLatin1String(QT4_BC_ID);
}

BuildConfiguration *Qt4BuildConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    Qt4BaseTarget *target = static_cast<Qt4BaseTarget *>(parent);
    Qt4BuildConfiguration *bc = new Qt4BuildConfiguration(target);
    if (bc->fromMap(map))
        return bc;
    delete bc;
    return 0;
}

void Qt4BuildConfiguration::importFromBuildDirectory()
{
    QString directory = buildDirectory();
    if (!directory.isEmpty()) {
        QString mkfile = directory;
        if (makefile().isEmpty())
            mkfile.append("/Makefile");
        else
            mkfile.append(makefile());

        Utils::FileName qmakePath = QtSupport::QtVersionManager::findQMakeBinaryFromMakefile(mkfile);
        if (!qmakePath.isEmpty()) {
            QtSupport::QtVersionManager *vm = QtSupport::QtVersionManager::instance();
            QtSupport::BaseQtVersion *version = vm->qtVersionForQMakeBinary(qmakePath);
            if (!version) {
                version = QtSupport::QtVersionFactory::createQtVersionFromQMakePath(qmakePath);
                vm->addVersion(version);
            }

            QPair<QtSupport::BaseQtVersion::QmakeBuildConfigs, QString> result =
                    QtSupport::QtVersionManager::scanMakeFile(mkfile, version->defaultBuildConfig());
            QtSupport::BaseQtVersion::QmakeBuildConfigs qmakeBuildConfig = result.first;

            QString additionalArguments = result.second;
            Utils::FileName parsedSpec = Qt4BuildConfiguration::extractSpecFromArguments(&additionalArguments, directory, version);
            const bool enableQmlDebugger =
                    Qt4BuildConfiguration::removeQMLInspectorFromArguments(&additionalArguments);

            // So we got all the information now apply it...
            setQtVersion(version);

            QMakeStep *qs = qmakeStep();
            qs->setUserArguments(additionalArguments);
            qs->setLinkQmlDebuggingLibrary(enableQmlDebugger);
            if (!parsedSpec.isEmpty() && parsedSpec != Utils::FileName::fromString(QLatin1String("default")) && qs->mkspec() != parsedSpec) {
                Utils::QtcProcess::addArgs(&additionalArguments, (QStringList() << QLatin1String("-spec") << parsedSpec.toUserOutput()));
                qs->setUserArguments(additionalArguments);
            }

            setQMakeBuildConfiguration(qmakeBuildConfig);
            // Adjust command line arguments, this is ugly as hell
            // If we are switching to BuildAll we want "release" in there and no "debug"
            // or "debug" in there and no "release"
            // If we are switching to not BuildAll we want neither "release" nor "debug" in there
            bool debug = qmakeBuildConfig & QtSupport::BaseQtVersion::DebugBuild;
            bool haveTag = !(qmakeBuildConfig & QtSupport::BaseQtVersion::BuildAll);
            QString makeCmdArguments = makeStep()->userArguments();
            Utils::QtcProcess::ArgIterator ait(&makeCmdArguments);
            while (ait.next()) {
                if (ait.value() == QLatin1String("debug")) {
                    if (!haveTag && debug)
                        haveTag = true;
                    else
                        ait.deleteArg();
                } else if (ait.value() == QLatin1String("release")) {
                    if (!haveTag && !debug)
                        haveTag = true;
                    else
                        ait.deleteArg();
                }
            }
            if (!haveTag)
                ait.appendArg(QLatin1String(debug ? "debug" : "release"));
            makeStep()->setUserArguments(makeCmdArguments);
        }
    }
}

BuildConfiguration::BuildType Qt4BuildConfiguration::buildType() const
{
    if (qmakeBuildConfiguration() & QtSupport::BaseQtVersion::DebugBuild)
        return Debug;
    else
        return Release;
}
