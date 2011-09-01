/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qt4desktoptargetfactory.h"
#include "buildconfigurationinfo.h"
#include "qt4projectmanagerconstants.h"
#include "qt4project.h"
#include "qt4runconfiguration.h"
#include "qt4desktoptarget.h"

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/customexecutablerunconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <qtsupport/qtversionmanager.h>

#include <QtGui/QVBoxLayout>
#include <QtGui/QApplication>
#include <QtGui/QStyle>
#include <QtGui/QLabel>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;
using ProjectExplorer::idFromMap;

Qt4DesktopTargetFactory::Qt4DesktopTargetFactory(QObject *parent) :
    Qt4BaseTargetFactory(parent)
{
    connect(QtSupport::QtVersionManager::instance(), SIGNAL(qtVersionsChanged(QList<int>)),
            this, SIGNAL(supportedTargetIdsChanged()));
}

Qt4DesktopTargetFactory::~Qt4DesktopTargetFactory()
{
}

bool Qt4DesktopTargetFactory::supportsTargetId(const QString &id) const
{
    return id == QLatin1String(Constants::DESKTOP_TARGET_ID);
}

QStringList Qt4DesktopTargetFactory::supportedTargetIds(ProjectExplorer::Project *parent) const
{
    if (parent && !qobject_cast<Qt4Project *>(parent))
        return QStringList();
    if (!QtSupport::QtVersionManager::instance()->supportsTargetId(Constants::DESKTOP_TARGET_ID))
        return QStringList();
    return QStringList() << QLatin1String(Constants::DESKTOP_TARGET_ID);
}

QString Qt4DesktopTargetFactory::displayNameForId(const QString &id) const
{
    if (id == QLatin1String(Constants::DESKTOP_TARGET_ID))
        return Qt4DesktopTarget::defaultDisplayName();
    return QString();
}

QString Qt4DesktopTargetFactory::buildNameForId(const QString &id) const
{
    if (id == QLatin1String(Constants::DESKTOP_TARGET_ID))
        return QLatin1String("desktop");
    return QString();
}

QIcon Qt4DesktopTargetFactory::iconForId(const QString &id) const
{
    if (id == QLatin1String(Constants::DESKTOP_TARGET_ID))
        return qApp->style()->standardIcon(QStyle::SP_ComputerIcon);
    return QIcon();
}

bool Qt4DesktopTargetFactory::canCreate(ProjectExplorer::Project *parent, const QString &id) const
{
    if (!qobject_cast<Qt4Project *>(parent))
        return false;
    return supportsTargetId(id);
}

bool Qt4DesktopTargetFactory::canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

ProjectExplorer::Target  *Qt4DesktopTargetFactory::restore(ProjectExplorer::Project *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    Qt4Project *qt4project = static_cast<Qt4Project *>(parent);
    Qt4DesktopTarget *target = new Qt4DesktopTarget(qt4project, QLatin1String("transient ID"));

    if (target->fromMap(map))
        return target;
    delete target;
    return 0;
}

Qt4TargetSetupWidget *Qt4DesktopTargetFactory::createTargetSetupWidget(const QString &id, const QString &proFilePath, const QtSupport::QtVersionNumber &number, bool importEnabled, QList<BuildConfigurationInfo> importInfos)
{

    QList<BuildConfigurationInfo> infos = this->availableBuildConfigurations(id, proFilePath, number);
    if (infos.isEmpty())
        return 0;
    Qt4DefaultTargetSetupWidget *widget = new Qt4DefaultTargetSetupWidget(this, id, proFilePath,  infos,
                                                                          number,  importEnabled,
                                                                          importInfos,
                                                                          Qt4DefaultTargetSetupWidget::USER);
    widget->setBuildConfiguraionComboBoxVisible(true);
    return widget;
}

ProjectExplorer::Target *Qt4DesktopTargetFactory::create(ProjectExplorer::Project *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;

    QList<QtSupport::BaseQtVersion *> knownVersions = QtSupport::QtVersionManager::instance()->versionsForTargetId(id);
    if (knownVersions.isEmpty())
        return 0;

    QtSupport::BaseQtVersion *qtVersion = knownVersions.first();
    QtSupport::BaseQtVersion::QmakeBuildConfigs config = qtVersion->defaultBuildConfig();

    QList<BuildConfigurationInfo> infos;
    infos.append(BuildConfigurationInfo(qtVersion, config, QString(), QString()));
    infos.append(BuildConfigurationInfo(qtVersion, config ^ QtSupport::BaseQtVersion::DebugBuild, QString(), QString()));

    return create(parent, id, infos);
}

QSet<QString> Qt4DesktopTargetFactory::targetFeatures(const QString & /*id*/) const
{
    QSet<QString> features;
    features << Constants::DESKTOP_TARGETFEATURE_ID;
    features << Constants::SHADOWBUILD_TARGETFEATURE_ID;
    // how to check check whether they component set is really installed?
    features << Constants::QTQUICKCOMPONENTS_SYMBIAN_TARGETFEATURE_ID;
    features << Constants::QTQUICKCOMPONENTS_MEEGO_TARGETFEATURE_ID;
    return features;
}

ProjectExplorer::Target *Qt4DesktopTargetFactory::create(ProjectExplorer::Project *parent, const QString &id, const QList<BuildConfigurationInfo> &infos)
{
    if (!canCreate(parent, id))
        return 0;
    if (infos.isEmpty())
        return 0;
    Qt4DesktopTarget *t = new Qt4DesktopTarget(static_cast<Qt4Project *>(parent), id);

    foreach (const BuildConfigurationInfo &info, infos)
        t->addQt4BuildConfiguration(msgBuildConfigurationName(info), QString(),
                                    info.version, info.buildConfig,
                                    info.additionalArguments, info.directory, info.importing);

    t->addDeployConfiguration(t->createDeployConfiguration(ProjectExplorer::Constants::DEFAULT_DEPLOYCONFIGURATION_ID));

    t->createApplicationProFiles();

    if (t->runConfigurations().isEmpty())
        t->addRunConfiguration(new ProjectExplorer::CustomExecutableRunConfiguration(t));
    return t;
}
