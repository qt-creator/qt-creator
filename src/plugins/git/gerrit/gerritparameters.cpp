/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "gerritparameters.h"
#include "gerritplugin.h"

#include <utils/hostosinfo.h>
#include <utils/pathchooser.h>

#include <QDebug>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

namespace Gerrit {
namespace Internal {

static const char settingsGroupC[] = "Gerrit";
static const char hostKeyC[] = "Host";
static const char userKeyC[] = "User";
static const char portKeyC[] = "Port";
static const char portFlagKeyC[] = "PortFlag";
static const char sshKeyC[] = "Ssh";
static const char httpsKeyC[] = "Https";
static const char defaultHostC[] = "codereview.qt-project.org";
static const char defaultSshC[] = "ssh";
static const char savedQueriesKeyC[] = "SavedQueries";

static const char defaultPortFlag[] = "-p";

enum { defaultPort = 29418 };

static inline QString detectSsh()
{
    const QByteArray gitSsh = qgetenv("GIT_SSH");
    if (!gitSsh.isEmpty())
        return QString::fromLocal8Bit(gitSsh);
    QString ssh = QStandardPaths::findExecutable(QLatin1String(defaultSshC));
    if (!ssh.isEmpty())
        return ssh;
    if (Utils::HostOsInfo::isWindowsHost()) { // Windows: Use ssh.exe from git if it cannot be found.
        Utils::FileName path = GerritPlugin::gitBinDirectory();
        if (!path.isEmpty())
            ssh = path.appendPath(QLatin1String(defaultSshC)).toString();
    }
    return ssh;
}

void GerritParameters::setPortFlagBySshType()
{
    bool isPlink = false;
    if (!ssh.isEmpty()) {
        const QString version = Utils::PathChooser::toolVersion(ssh, QStringList(QLatin1String("-V")));
        isPlink = version.contains(QLatin1String("plink"), Qt::CaseInsensitive);
    }
    portFlag = isPlink ? QLatin1String("-P") : QLatin1String(defaultPortFlag);
}

GerritParameters::GerritParameters()
    : host(QLatin1String(defaultHostC))
    , port(defaultPort)
    , https(true)
    , portFlag(QLatin1String(defaultPortFlag))
{
}

QStringList GerritParameters::baseCommandArguments() const
{
    QStringList result;
    result << ssh << portFlag << QString::number(port)
           << sshHostArgument() << QLatin1String("gerrit");
    return result;
}

QString GerritParameters::sshHostArgument() const
{
    return user.isEmpty() ? host : (user + QLatin1Char('@') + host);
}

bool GerritParameters::equals(const GerritParameters &rhs) const
{
    return port == rhs.port && host == rhs.host && user == rhs.user
           && ssh == rhs.ssh && https == rhs.https;
}

void GerritParameters::toSettings(QSettings *s) const
{
    s->beginGroup(QLatin1String(settingsGroupC));
    s->setValue(QLatin1String(hostKeyC), host);
    s->setValue(QLatin1String(userKeyC), user);
    s->setValue(QLatin1String(portKeyC), port);
    s->setValue(QLatin1String(portFlagKeyC), portFlag);
    s->setValue(QLatin1String(sshKeyC), ssh);
    s->setValue(QLatin1String(httpsKeyC), https);
    s->endGroup();
}

void GerritParameters::saveQueries(QSettings *s) const
{
    s->beginGroup(QLatin1String(settingsGroupC));
    s->setValue(QLatin1String(savedQueriesKeyC), savedQueries.join(QLatin1Char(',')));
    s->endGroup();
}

void GerritParameters::fromSettings(const QSettings *s)
{
    const QString rootKey = QLatin1String(settingsGroupC) + QLatin1Char('/');
    host = s->value(rootKey + QLatin1String(hostKeyC), QLatin1String(defaultHostC)).toString();
    user = s->value(rootKey + QLatin1String(userKeyC), QString()).toString();
    ssh = s->value(rootKey + QLatin1String(sshKeyC), QString()).toString();
    port = s->value(rootKey + QLatin1String(portKeyC), QVariant(int(defaultPort))).toInt();
    portFlag = s->value(rootKey + QLatin1String(portFlagKeyC), QLatin1String(defaultPortFlag)).toString();
    savedQueries = s->value(rootKey + QLatin1String(savedQueriesKeyC), QString()).toString()
            .split(QLatin1Char(','));
    https = s->value(rootKey + QLatin1String(httpsKeyC), QVariant(true)).toBool();
    if (ssh.isEmpty())
        ssh = detectSsh();
}

bool GerritParameters::isValid() const
{
    return !host.isEmpty() && !user.isEmpty() && !ssh.isEmpty();
}

} // namespace Internal
} // namespace Gerrit
