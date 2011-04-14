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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmlprojectmanagerconstants.h"
#include "qmlprojectrunconfiguration.h"
#include "qmlprojectrunconfigurationfactory.h"
#include "qmlprojecttarget.h"

#include <projectexplorer/projectconfiguration.h>
#include <projectexplorer/runconfiguration.h>

namespace QmlProjectManager {
namespace Internal {

QmlProjectRunConfigurationFactory::QmlProjectRunConfigurationFactory(QObject *parent) :
    ProjectExplorer::IRunConfigurationFactory(parent)
{
}

QmlProjectRunConfigurationFactory::~QmlProjectRunConfigurationFactory()
{
}

QStringList QmlProjectRunConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const
{
    if (!qobject_cast<QmlProjectTarget *>(parent))
        return QStringList();
    return QStringList() << QLatin1String(Constants::QML_RC_ID);
}

QString QmlProjectRunConfigurationFactory::displayNameForId(const QString &id) const
{
    if (id == QLatin1String(Constants::QML_RC_ID))
        return tr("Run QML Script");
    return QString();
}

bool QmlProjectRunConfigurationFactory::canCreate(ProjectExplorer::Target *parent, const QString &id) const
{
    if (!qobject_cast<QmlProjectTarget *>(parent))
        return false;
    return id == QLatin1String(Constants::QML_RC_ID);
}

ProjectExplorer::RunConfiguration *QmlProjectRunConfigurationFactory::create(ProjectExplorer::Target *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;
    QmlProjectTarget *qmlparent = static_cast<QmlProjectTarget *>(parent);
    return new QmlProjectRunConfiguration(qmlparent);
}

bool QmlProjectRunConfigurationFactory::canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const
{
    QString id = ProjectExplorer::idFromMap(map);
    return canCreate(parent, id);
}

ProjectExplorer::RunConfiguration *QmlProjectRunConfigurationFactory::restore(ProjectExplorer::Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    QmlProjectTarget *qmlparent = static_cast<QmlProjectTarget *>(parent);
    QmlProjectRunConfiguration *rc = new QmlProjectRunConfiguration(qmlparent);
    if (rc->fromMap(map))
        return rc;
    delete rc;
    return 0;
}

bool QmlProjectRunConfigurationFactory::canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source) const
{
    return canCreate(parent, source->id());
}

ProjectExplorer::RunConfiguration *QmlProjectRunConfigurationFactory::clone(ProjectExplorer::Target *parent,
                                                                     ProjectExplorer::RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    QmlProjectTarget *qmlparent = static_cast<QmlProjectTarget *>(parent);
    return new QmlProjectRunConfiguration(qmlparent, qobject_cast<QmlProjectRunConfiguration *>(source));
}

} // namespace Internal
} // namespace QmlProjectManager

