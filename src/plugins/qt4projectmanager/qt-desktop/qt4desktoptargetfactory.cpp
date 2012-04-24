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

#include "qt4desktoptargetfactory.h"
#include "buildconfigurationinfo.h"
#include "qt4buildconfiguration.h"
#include "qt4projectmanagerconstants.h"
#include "qt4project.h"
#include "qt4runconfiguration.h"
#include "qt4desktoptarget.h"

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <qtsupport/customexecutablerunconfiguration.h>
#include <qtsupport/qtprofileinformation.h>
#include <qtsupport/qtsupportconstants.h>

#include <qtsupport/qtversionmanager.h>

#include <QVBoxLayout>
#include <QApplication>
#include <QStyle>
#include <QLabel>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;
using ProjectExplorer::idFromMap;

Qt4DesktopTargetFactory::Qt4DesktopTargetFactory(QObject *parent) :
    Qt4BaseTargetFactory(parent)
{ }

Qt4DesktopTargetFactory::~Qt4DesktopTargetFactory()
{ }

bool Qt4DesktopTargetFactory::supportsProfile(const ProjectExplorer::Profile *p) const
{
    if (!p)
        return false;
    QtSupport::BaseQtVersion *version = QtSupport::QtProfileInformation::qtVersion(p);
    return version && version->type() == QLatin1String(QtSupport::Constants::DESKTOPQT);
}

bool Qt4DesktopTargetFactory::canCreate(ProjectExplorer::Project *parent, const ProjectExplorer::Profile *p) const
{
    if (!qobject_cast<Qt4Project *>(parent))
        return false;
    return supportsProfile(p);
}

bool Qt4DesktopTargetFactory::canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const
{
    return qobject_cast<Qt4Project *>(parent) && supportsProfile(profileFromMap(map));
}

ProjectExplorer::Target  *Qt4DesktopTargetFactory::restore(ProjectExplorer::Project *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    Qt4Project *qt4project = static_cast<Qt4Project *>(parent);
    ProjectExplorer::Profile *p = profileFromMap(map);
    if (!p)
        return 0;
    Qt4DesktopTarget *target = new Qt4DesktopTarget(qt4project, p);

    if (target->fromMap(map))
        return target;
    delete target;
    return 0;
}

ProjectExplorer::Target *Qt4DesktopTargetFactory::create(ProjectExplorer::Project *parent, ProjectExplorer::Profile *p)
{
    if (!canCreate(parent, p))
        return 0;

    QtSupport::BaseQtVersion *qtVersion = QtSupport::QtProfileInformation::qtVersion(p);
    QtSupport::BaseQtVersion::QmakeBuildConfigs config = qtVersion->defaultBuildConfig();

    QList<BuildConfigurationInfo> infos;
    infos.append(BuildConfigurationInfo(config, QString(), QString()));
    infos.append(BuildConfigurationInfo(config ^ QtSupport::BaseQtVersion::DebugBuild, QString(), QString()));

    return create(parent, p, infos);
}

QSet<QString> Qt4DesktopTargetFactory::targetFeatures(ProjectExplorer::Profile *s) const
{
    QSet<QString> features;
    if (supportsProfile(s))
        features << QLatin1String(Constants::DESKTOP_TARGETFEATURE_ID)
                 << QLatin1String(Constants::SHADOWBUILD_TARGETFEATURE_ID);

    return features;
}

ProjectExplorer::Target *Qt4DesktopTargetFactory::create(ProjectExplorer::Project *parent,
                                                         ProjectExplorer::Profile *p,
                                                         const QList<BuildConfigurationInfo> &infos)
{
    if (!canCreate(parent, p))
        return 0;
    if (infos.isEmpty())
        return 0;
    Qt4DesktopTarget *t = new Qt4DesktopTarget(static_cast<Qt4Project *>(parent), p);

    foreach (const BuildConfigurationInfo &info, infos)
        Qt4BuildConfiguration::setup(t, buildConfigurationDisplayName(info), QString(), info.buildConfig,
                                    info.additionalArguments, info.directory, info.importing);
    t->addDeployConfiguration(t->createDeployConfiguration(Core::Id(ProjectExplorer::Constants::DEFAULT_DEPLOYCONFIGURATION_ID)));

    t->createApplicationProFiles(false);

    if (t->runConfigurations().isEmpty())
        t->addRunConfiguration(new QtSupport::CustomExecutableRunConfiguration(t));
    return t;
}
