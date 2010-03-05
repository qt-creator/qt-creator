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

#include "qmlprojecttarget.h"

#include "qmlproject.h"
#include "qmlprojectmanagerconstants.h"
#include "qmlprojectrunconfiguration.h"

#include <QtGui/QApplication>
#include <QtGui/QStyle>

namespace QmlProjectManager {
namespace Internal {

QmlProjectTarget::QmlProjectTarget(QmlProject *parent) :
    ProjectExplorer::Target(parent, QLatin1String(Constants::QML_VIEWER_TARGET_ID))
{
    setDisplayName(QApplication::translate("QmlProjectManager::QmlTarget",
                                           Constants::QML_VIEWER_TARGET_DISPLAY_NAME,
                                           "Qml Viewer target display name"));
    setIcon(qApp->style()->standardIcon(QStyle::SP_ComputerIcon));
}

QmlProjectTarget::~QmlProjectTarget()
{
}

QmlProject *QmlProjectTarget::qmlProject() const
{
    return static_cast<QmlProject *>(project());
}

ProjectExplorer::IBuildConfigurationFactory *QmlProjectTarget::buildConfigurationFactory(void) const
{
    return 0;
}

bool QmlProjectTarget::fromMap(const QVariantMap &map)
{
    if (!Target::fromMap(map))
        return false;

    setDisplayName(QApplication::translate("QmlProjectManager::QmlTarget",
                                           Constants::QML_VIEWER_TARGET_DISPLAY_NAME,
                                           "Qml Viewer target display name"));

    return true;
}

QmlProjectTargetFactory::QmlProjectTargetFactory(QObject *parent) :
    ITargetFactory(parent)
{
}

QmlProjectTargetFactory::~QmlProjectTargetFactory()
{
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
                                           "Qml Viewer target display name");
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
    QmlProjectTarget *t(new QmlProjectTarget(qmlproject));

    // Add RunConfiguration (Qml does not have BuildConfigurations)
    QmlProjectRunConfiguration *runConf(new QmlProjectRunConfiguration(t));
    t->addRunConfiguration(runConf);

    return t;
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
