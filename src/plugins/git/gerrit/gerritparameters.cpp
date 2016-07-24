/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
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
    QString ssh = QStandardPaths::findExecutable(defaultSshC);
    if (!ssh.isEmpty())
        return ssh;
    if (Utils::HostOsInfo::isWindowsHost()) { // Windows: Use ssh.exe from git if it cannot be found.
        Utils::FileName path = GerritPlugin::gitBinDirectory();
        if (!path.isEmpty())
            ssh = path.appendPath(defaultSshC).toString();
    }
    return ssh;
}

void GerritParameters::setPortFlagBySshType()
{
    bool isPlink = false;
    if (!ssh.isEmpty()) {
        const QString version = Utils::PathChooser::toolVersion(ssh, { "-V" });
        isPlink = version.contains("plink", Qt::CaseInsensitive);
    }
    portFlag = QLatin1String(isPlink ? "-P" : defaultPortFlag);
}

GerritParameters::GerritParameters()
    : host(defaultHostC)
    , port(defaultPort)
    , https(true)
    , portFlag(defaultPortFlag)
{
}

QStringList GerritParameters::baseCommandArguments() const
{
    return { ssh, portFlag, QString::number(port), sshHostArgument(), "gerrit" };
}

QString GerritParameters::sshHostArgument() const
{
    return user.isEmpty() ? host : (user + '@' + host);
}

bool GerritParameters::equals(const GerritParameters &rhs) const
{
    return port == rhs.port && host == rhs.host && user == rhs.user
           && ssh == rhs.ssh && https == rhs.https;
}

void GerritParameters::toSettings(QSettings *s) const
{
    s->beginGroup(settingsGroupC);
    s->setValue(hostKeyC, host);
    s->setValue(userKeyC, user);
    s->setValue(portKeyC, port);
    s->setValue(portFlagKeyC, portFlag);
    s->setValue(sshKeyC, ssh);
    s->setValue(httpsKeyC, https);
    s->endGroup();
}

void GerritParameters::saveQueries(QSettings *s) const
{
    s->beginGroup(settingsGroupC);
    s->setValue(savedQueriesKeyC, savedQueries.join(','));
    s->endGroup();
}

void GerritParameters::fromSettings(const QSettings *s)
{
    const QString rootKey = QLatin1String(settingsGroupC) + '/';
    host = s->value(rootKey + hostKeyC, defaultHostC).toString();
    user = s->value(rootKey + userKeyC, QString()).toString();
    ssh = s->value(rootKey + sshKeyC, QString()).toString();
    port = s->value(rootKey + portKeyC, QVariant(int(defaultPort))).toInt();
    portFlag = s->value(rootKey + portFlagKeyC, defaultPortFlag).toString();
    savedQueries = s->value(rootKey + savedQueriesKeyC, QString()).toString()
            .split(',');
    https = s->value(rootKey + httpsKeyC, QVariant(true)).toBool();
    if (ssh.isEmpty())
        ssh = detectSsh();
}

bool GerritParameters::isValid() const
{
    return !host.isEmpty() && !user.isEmpty() && !ssh.isEmpty();
}

} // namespace Internal
} // namespace Gerrit
