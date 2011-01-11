/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmlprojecttarget.h"

#include "qmlproject.h"
#include "qmlprojectmanagerconstants.h"
#include "qmlprojectrunconfiguration.h"

#include <QtCore/QDebug>
#include <QtGui/QApplication>
#include <QtGui/QStyle>

namespace QmlProjectManager {
namespace Internal {

QmlProjectTarget::QmlProjectTarget(QmlProject *parent) :
    ProjectExplorer::Target(parent, QLatin1String(Constants::QML_VIEWER_TARGET_ID))
{
    setDisplayName(QApplication::translate("QmlProjectManager::QmlTarget",
                                           Constants::QML_VIEWER_TARGET_DISPLAY_NAME,
                                           "QML Viewer target display name"));
    setIcon(qApp->style()->standardIcon(QStyle::SP_ComputerIcon));
}

QmlProjectTarget::~QmlProjectTarget()
{
}

ProjectExplorer::BuildConfigWidget *QmlProjectTarget::createConfigWidget()
{
    return 0;
}

QmlProject *QmlProjectTarget::qmlProject() const
{
    return static_cast<QmlProject *>(project());
}

ProjectExplorer::IBuildConfigurationFactory *QmlProjectTarget::buildConfigurationFactory(void) const
{
    return 0;
}

ProjectExplorer::DeployConfigurationFactory *QmlProjectTarget::deployConfigurationFactory() const
{
    return 0;
}

bool QmlProjectTarget::fromMap(const QVariantMap &map)
{
    if (!Target::fromMap(map))
        return false;

    if (runConfigurations().isEmpty()) {
        qWarning() << "Failed to restore run configuration of QML project!";
        return false;
    }

    setDisplayName(QApplication::translate("QmlProjectManager::QmlTarget",
                                           Constants::QML_VIEWER_TARGET_DISPLAY_NAME,
                                           "QML Viewer target display name"));

    return true;
}

QmlProjectTargetFactory::QmlProjectTargetFactory(QObject *parent) :
    ITargetFactory(parent)
{
}

QmlProjectTargetFactory::~QmlProjectTargetFactory()
{
}

bool QmlProjectTargetFactory::supportsTargetId(const QString &id) const
{
    return id == QLatin1String(Constants::QML_VIEWER_TARGET_ID);
}

QStringList QmlProjectTargetFactory::availableCreationIds(ProjectExplorer::Project *parent) const
{
    if (!qobject_cast<QmlProject *>(parent))
        return QStringList();
    return QStringList() << QLatin1String(Constants::QML_VIEWER_TARGET_ID);
}

QString QmlProjectTargetFactory::displayNameForId(const QString &id) const
{
    if (id == QLatin1String(Constants::QML_VIEWER_TARGET_ID))
        return QCoreApplication::translate("QmlProjectManager::QmlTarget",
                                           Constants::QML_VIEWER_TARGET_DISPLAY_NAME,
                                           "QML Viewer target display name");
    return QString();
}

bool QmlProjectTargetFactory::canCreate(ProjectExplorer::Project *parent, const QString &id) const
{
    if (!qobject_cast<QmlProject *>(parent))
        return false;
    return id == QLatin1String(Constants::QML_VIEWER_TARGET_ID);
}

QmlProjectTarget *QmlProjectTargetFactory::create(ProjectExplorer::Project *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;
    QmlProject *qmlproject(static_cast<QmlProject *>(parent));
    QmlProjectTarget *target = new QmlProjectTarget(qmlproject);

    // Add RunConfiguration (QML does not have BuildConfigurations)
    QmlProjectRunConfiguration *runConf = new QmlProjectRunConfiguration(target);
    target->addRunConfiguration(runConf);

    return target;
}

bool QmlProjectTargetFactory::canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

QmlProjectTarget *QmlProjectTargetFactory::restore(ProjectExplorer::Project *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    QmlProject *qmlproject(static_cast<QmlProject *>(parent));
    QmlProjectTarget *target(new QmlProjectTarget(qmlproject));
    if (target->fromMap(map))
        return target;
    delete target;
    return 0;
}

} // namespace Internal
} // namespace QmlProjectManager
