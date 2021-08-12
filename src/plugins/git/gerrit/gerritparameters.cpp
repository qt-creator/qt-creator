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

#include <utils/commandline.h>
#include <utils/hostosinfo.h>
#include <utils/pathchooser.h>

#include <QDir>
#include <QSettings>
#include <QStandardPaths>

using namespace Utils;

namespace Gerrit {
namespace Internal {

const char settingsGroupC[] = "Gerrit";
const char hostKeyC[] = "Host";
const char userKeyC[] = "User";
const char portKeyC[] = "Port";
const char portFlagKeyC[] = "PortFlag";
const char sshKeyC[] = "Ssh";
const char curlKeyC[] = "Curl";
const char httpsKeyC[] = "Https";
const char savedQueriesKeyC[] = "SavedQueries";

const char defaultPortFlag[] = "-p";

static FilePath detectApp(const QString &defaultExe)
{
    const QString defaultApp = HostOsInfo::withExecutableSuffix(defaultExe);
    const QString app = QStandardPaths::findExecutable(defaultApp);
    if (!app.isEmpty() || !HostOsInfo::isWindowsHost())
        return FilePath::fromString(app);
    // Windows: Use app.exe from git if it cannot be found.
    const FilePath gitBinDir = GerritPlugin::gitBinDirectory();
    if (gitBinDir.isEmpty())
        return {};
    FilePath path = gitBinDir.pathAppended(defaultApp);
    if (path.exists())
        return path;

    // If app was not found, and git bin is Git/usr/bin (Git for Windows),
    // search also in Git/mingw{32,64}/bin
    if (!gitBinDir.endsWith("/usr/bin"))
        return {};
    path = gitBinDir.parentDir().parentDir();
    const FilePaths entries = path.dirEntries({"mingw*"}, {});
    if (entries.isEmpty())
        return {};
    path = entries.first() / "bin" / defaultApp;
    if (path.exists())
        return path;
    return {};
}

static FilePath detectSsh()
{
    const QString gitSsh = qEnvironmentVariable("GIT_SSH");
    if (!gitSsh.isEmpty())
        return FilePath::fromString(gitSsh);
    return detectApp("ssh");
}

void GerritParameters::setPortFlagBySshType()
{
    bool isPlink = false;
    if (!ssh.isEmpty()) {
        const QString version = PathChooser::toolVersion({ssh, {"-V"}});
        isPlink = version.contains("plink", Qt::CaseInsensitive);
    }
    portFlag = QLatin1String(isPlink ? "-P" : defaultPortFlag);
}

GerritParameters::GerritParameters()
    : portFlag(defaultPortFlag)
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
    s->setValue(sshKeyC, ssh.toVariant());
    s->setValue(curlKeyC, curl.toVariant());
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
    ssh = FilePath::fromVariant(s->value(rootKey + sshKeyC, QString()));
    curl = FilePath::fromVariant(s->value(rootKey + curlKeyC));
    server.port = ushort(s->value(rootKey + portKeyC, QVariant(GerritServer::defaultPort)).toInt());
    portFlag = s->value(rootKey + portFlagKeyC, defaultPortFlag).toString();
    savedQueries = s->value(rootKey + savedQueriesKeyC, QString()).toString()
            .split(',');
    https = s->value(rootKey + httpsKeyC, QVariant(true)).toBool();
    if (ssh.isEmpty() || !ssh.exists())
        ssh = detectSsh();
    if (curl.isEmpty() || !curl.exists())
        curl = detectApp("curl");
}

bool GerritParameters::isValid() const
{
    return !server.host.isEmpty() && !server.user.userName.isEmpty() && !ssh.isEmpty();
}

} // namespace Internal
} // namespace Gerrit
