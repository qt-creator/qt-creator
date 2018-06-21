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

#include <QDir>
#include <QSettings>
#include <QStandardPaths>

using namespace Utils;

namespace Gerrit {
namespace Internal {

static const char settingsGroupC[] = "Gerrit";
static const char hostKeyC[] = "Host";
static const char userKeyC[] = "User";
static const char portKeyC[] = "Port";
static const char portFlagKeyC[] = "PortFlag";
static const char sshKeyC[] = "Ssh";
static const char curlKeyC[] = "Curl";
static const char httpsKeyC[] = "Https";
static const char savedQueriesKeyC[] = "SavedQueries";

static const char defaultPortFlag[] = "-p";

static inline QString detectApp(const char *defaultExe)
{
    const QString defaultApp = HostOsInfo::withExecutableSuffix(QLatin1String(defaultExe));
    QString app = QStandardPaths::findExecutable(defaultApp);
    if (!app.isEmpty() || !HostOsInfo::isWindowsHost())
        return app;
    // Windows: Use app.exe from git if it cannot be found.
    const FileName gitBinDir = GerritPlugin::gitBinDirectory();
    if (gitBinDir.isEmpty())
        return QString();
    FileName path = gitBinDir;
    path.appendPath(defaultApp);
    if (path.exists())
        return path.toString();

    // If app was not found, and git bin is Git/usr/bin (Git for Windows),
    // search also in Git/mingw{32,64}/bin
    if (!gitBinDir.endsWith("/usr/bin"))
        return QString();
    path = gitBinDir.parentDir().parentDir();
    QDir dir(path.toString());
    const QStringList entries = dir.entryList({"mingw*"});
    if (entries.isEmpty())
        return QString();
    path.appendPath(entries.first()).appendPath("bin").appendPath(defaultApp);
    if (path.exists())
        return path.toString();
    return QString();
}

static inline QString detectSsh()
{
    const QByteArray gitSsh = qgetenv("GIT_SSH");
    if (!gitSsh.isEmpty())
        return QString::fromLocal8Bit(gitSsh);
    return detectApp("ssh");
}

void GerritParameters::setPortFlagBySshType()
{
    bool isPlink = false;
    if (!ssh.isEmpty()) {
        const QString version = PathChooser::toolVersion(ssh, {"-V"});
        isPlink = version.contains("plink", Qt::CaseInsensitive);
    }
    portFlag = QLatin1String(isPlink ? "-P" : defaultPortFlag);
}

GerritParameters::GerritParameters()
    : https(true)
    , portFlag(defaultPortFlag)
{
}

bool GerritParameters::equals(const GerritParameters &rhs) const
{
    return server == rhs.server && ssh == rhs.ssh && curl == rhs.curl && https == rhs.https;
}

void GerritParameters::toSettings(QSettings *s) const
{
    s->beginGroup(settingsGroupC);
    s->setValue(hostKeyC, server.host);
    s->setValue(userKeyC, server.user.userName);
    s->setValue(portKeyC, server.port);
    s->setValue(portFlagKeyC, portFlag);
    s->setValue(sshKeyC, ssh);
    s->setValue(curlKeyC, curl);
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
    server.host = s->value(rootKey + hostKeyC, GerritServer::defaultHost()).toString();
    server.user.userName = s->value(rootKey + userKeyC, QString()).toString();
    ssh = s->value(rootKey + sshKeyC, QString()).toString();
    curl = s->value(rootKey + curlKeyC).toString();
    server.port = ushort(s->value(rootKey + portKeyC, QVariant(GerritServer::defaultPort)).toInt());
    portFlag = s->value(rootKey + portFlagKeyC, defaultPortFlag).toString();
    savedQueries = s->value(rootKey + savedQueriesKeyC, QString()).toString()
            .split(',');
    https = s->value(rootKey + httpsKeyC, QVariant(true)).toBool();
    if (ssh.isEmpty() || !QFile::exists(ssh))
        ssh = detectSsh();
    if (curl.isEmpty() || !QFile::exists(curl))
        curl = detectApp("curl");
}

bool GerritParameters::isValid() const
{
    return !server.host.isEmpty() && !server.user.userName.isEmpty() && !ssh.isEmpty();
}

} // namespace Internal
} // namespace Gerrit
