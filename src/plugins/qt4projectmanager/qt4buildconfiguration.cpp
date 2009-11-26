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

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;
using namespace ProjectExplorer;

namespace {
    bool debug = false;
}

namespace {
    const char * const KEY_QT_VERSION_ID = "QtVersionId";
}

Qt4BuildConfiguration::Qt4BuildConfiguration(Qt4Project *pro)
    : BuildConfiguration(pro)
{

}

Qt4BuildConfiguration::Qt4BuildConfiguration(Qt4BuildConfiguration *source)
    : BuildConfiguration(source)
{

}

Qt4BuildConfiguration::~Qt4BuildConfiguration()
{

}

Qt4Project *Qt4BuildConfiguration::qt4Project() const
{
    return static_cast<Qt4Project *>(project());
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
    setValue("clearSystemEnvironment", !b);
    emit environmentChanged();
}

bool Qt4BuildConfiguration::useSystemEnvironment() const
{
    bool b = !(value("clearSystemEnvironment").isValid()
               && value("clearSystemEnvironment").toBool());
    return b;
}

QList<ProjectExplorer::EnvironmentItem> Qt4BuildConfiguration::userEnvironmentChanges() const
{
    return EnvironmentItem::fromStringList(value("userEnvironmentChanges").toStringList());
}

void Qt4BuildConfiguration::setUserEnvironmentChanges(const QList<ProjectExplorer::EnvironmentItem> &diff)
{
    QStringList list = EnvironmentItem::toStringList(diff);
    if (list == value("userEnvironmentChanges").toStringList())
        return;
    setValue("userEnvironmentChanges", list);
    emit environmentChanged();
}

QString Qt4BuildConfiguration::buildDirectory() const
{
    QString workingDirectory;
    if (value("useShadowBuild").toBool())
        workingDirectory = value("buildDirectory").toString();
    if (workingDirectory.isEmpty())
        workingDirectory = QFileInfo(project()->file()->fileName()).absolutePath();
    return workingDirectory;
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

#ifdef QTCREATOR_WITH_S60
static inline QString symbianMakeTarget(QtVersion::QmakeBuildConfig buildConfig,
                                        const QString &type)
{
    QString rc = (buildConfig & QtVersion::DebugBuild) ?
                 QLatin1String("debug-") : QLatin1String("release-");
    rc += type;
    return rc;
}
#endif

QString Qt4BuildConfiguration::defaultMakeTarget() const
{
#ifdef QTCREATOR_WITH_S60
    ToolChain *tc = toolChain();
    if (!tc)
        return QString::null;
    const QtVersion::QmakeBuildConfig buildConfig
            = QtVersion::QmakeBuildConfig(value("buildConfiguration").toInt());

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
#else

#endif
    return QString::null;
}

QString Qt4BuildConfiguration::qtDir() const
{
    QtVersion *version = qtVersion();
    if (version)
        return version->versionInfo().value("QT_INSTALL_DATA");
    return QString::null;
}

QtVersion *Qt4BuildConfiguration::qtVersion() const
{
    return QtVersionManager::instance()->version(qtVersionId());
}

int Qt4BuildConfiguration::qtVersionId() const
{
    QtVersionManager *vm = QtVersionManager::instance();
    if (debug)
        qDebug()<<"Looking for qtVersion ID of "<<displayName();
    int id = 0;
    QVariant vid = value(KEY_QT_VERSION_ID);
    if (vid.isValid()) {
        id = vid.toInt();
        if (vm->version(id)->isValid()) {
            return id;
        } else {
            const_cast<Qt4BuildConfiguration *>(this)->setValue(KEY_QT_VERSION_ID, 0);
            return 0;
        }
    } else {
        // Backward compatibilty, we might have just the name:
        QString vname = value("QtVersion").toString();
        if (debug)
            qDebug()<<"  Backward compatibility reading QtVersion"<<vname;
        if (!vname.isEmpty()) {
            const QList<QtVersion *> &versions = vm->versions();
            foreach (const QtVersion * const version, versions) {
                if (version->name() == vname) {
                    if (debug)
                        qDebug()<<"found name in versions";
                    const_cast<Qt4BuildConfiguration *>(this)->setValue(KEY_QT_VERSION_ID, version->uniqueId());
                    return version->uniqueId();
                }
            }
        }
    }
    if (debug)
        qDebug()<<"  using qtversion with id ="<<id;
    // Nothing found, reset to default
    const_cast<Qt4BuildConfiguration *>(this)->setValue(KEY_QT_VERSION_ID, id);
    return id;
}

void Qt4BuildConfiguration::setQtVersion(int id)
{
    setValue(KEY_QT_VERSION_ID, id);
    emit qtVersionChanged();
    qt4Project()->updateActiveRunConfiguration();
}

void Qt4BuildConfiguration::setToolChainType(ProjectExplorer::ToolChain::ToolChainType type)
{
    setValue("ToolChain", (int)type);
    qt4Project()->updateActiveRunConfiguration();
}

ProjectExplorer::ToolChain::ToolChainType Qt4BuildConfiguration::toolChainType() const
{
    ToolChain::ToolChainType originalType = ToolChain::ToolChainType(value("ToolChain").toInt());
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

// returns true if both are equal
bool Qt4BuildConfiguration::compareBuildConfigurationToImportFrom(const QString &workingDirectory)
{
    QMakeStep *qs = qmakeStep();
    if (QDir(workingDirectory).exists(QLatin1String("Makefile")) && qs) {
        QString qmakePath = QtVersionManager::findQMakeBinaryFromMakefile(workingDirectory);
        QtVersion *version = qtVersion();
        if (version->qmakeCommand() == qmakePath) {
            // same qtversion
            QPair<QtVersion::QmakeBuildConfigs, QStringList> result =
                    QtVersionManager::scanMakeFile(workingDirectory, version->defaultBuildConfig());
            if (QtVersion::QmakeBuildConfig(value("buildConfiguration").toInt()) == result.first) {
                // The QMake Build Configuration are the same,
                // now compare arguments lists
                // we have to compare without the spec/platform cmd argument
                // and compare that on its own
                QString actualSpec = Qt4Project::extractSpecFromArgumentList(qs->qmakeArguments(), workingDirectory, version);
                if (actualSpec.isEmpty()) {
                    // Easy one the user has choosen not to override the settings
                    actualSpec = version->mkspec();
                }


                QString parsedSpec = Qt4Project::extractSpecFromArgumentList(result.second, workingDirectory, version);
                QStringList actualArgs = Qt4Project::removeSpecFromArgumentList(qs->qmakeArguments());
                QStringList parsedArgs = Qt4Project::removeSpecFromArgumentList(result.second);

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
