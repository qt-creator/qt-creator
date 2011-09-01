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
#include "remotelinuxrunconfigurationfactory.h"

#include "remotelinuxrunconfiguration.h"
#include "remotelinuxutils.h"

#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4target.h>
#include <utils/qtcassert.h>

#include <QtCore/QFileInfo>
#include <QtCore/QString>
#include <QtCore/QStringList>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;

namespace RemoteLinux {
namespace Internal {

namespace {
QString pathFromId(const QString &id)
{
    if (!id.startsWith(RemoteLinuxRunConfiguration::Id))
        return QString();
    return id.mid(RemoteLinuxRunConfiguration::Id.size());
}

} // namespace

RemoteLinuxRunConfigurationFactory::RemoteLinuxRunConfigurationFactory(QObject *parent)
    : IRunConfigurationFactory(parent)
{
}

RemoteLinuxRunConfigurationFactory::~RemoteLinuxRunConfigurationFactory()
{
}

bool RemoteLinuxRunConfigurationFactory::canCreate(Target *parent,
    const QString &id) const
{
    if (!id.startsWith(RemoteLinuxRunConfiguration::Id))
        return false;
    return qobject_cast<Qt4BaseTarget *>(parent)->qt4Project()
        ->hasApplicationProFile(pathFromId(id));
}

bool RemoteLinuxRunConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    Q_UNUSED(parent);
    return ProjectExplorer::idFromMap(map).startsWith(RemoteLinuxRunConfiguration::Id);
}

bool RemoteLinuxRunConfigurationFactory::canClone(Target *parent, RunConfiguration *source) const
{
    const RemoteLinuxRunConfiguration * const rlrc
            = qobject_cast<RemoteLinuxRunConfiguration *>(source);
    return rlrc && canCreate(parent, source->id() + QLatin1Char('.') + rlrc->proFilePath());
}

QStringList RemoteLinuxRunConfigurationFactory::availableCreationIds(Target *parent) const
{
    if (Qt4BaseTarget *t = qobject_cast<Qt4BaseTarget *>(parent)) {
        if (t && RemoteLinuxUtils::hasUnixQt(t)) {
            return t->qt4Project()->applicationProFilePathes(RemoteLinuxRunConfiguration::Id);
        }
    }
    return QStringList();
}

QString RemoteLinuxRunConfigurationFactory::displayNameForId(const QString &id) const
{
    return QFileInfo(pathFromId(id)).completeBaseName()
        + tr(" (on Remote Generic Linux Host)");
}

RunConfiguration *RemoteLinuxRunConfigurationFactory::create(Target *parent, const QString &id)
{
    QTC_ASSERT(canCreate(parent, id), return 0);
    return new RemoteLinuxRunConfiguration(qobject_cast<Qt4BaseTarget *>(parent), id,
        pathFromId(id));
}

RunConfiguration *RemoteLinuxRunConfigurationFactory::restore(Target *parent,
    const QVariantMap &map)
{
    QTC_ASSERT(canRestore(parent, map), return 0);
    RemoteLinuxRunConfiguration *rc = new RemoteLinuxRunConfiguration(qobject_cast<Qt4BaseTarget *>(parent),
        RemoteLinuxRunConfiguration::Id, QString());
    if (rc->fromMap(map))
        return rc;

    delete rc;
    return 0;
}

RunConfiguration *RemoteLinuxRunConfigurationFactory::clone(Target *parent,
    RunConfiguration *source)
{
    QTC_ASSERT(canClone(parent, source), return 0);
    RemoteLinuxRunConfiguration *old = static_cast<RemoteLinuxRunConfiguration *>(source);
    return new RemoteLinuxRunConfiguration(static_cast<Qt4BaseTarget *>(parent), old);
}

} // namespace Internal
} // namespace RemoteLinux
