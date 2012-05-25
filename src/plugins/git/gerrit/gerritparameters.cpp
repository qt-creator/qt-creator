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

#include "gerritparameters.h"
#include "gerritplugin.h"

#if QT_VERSION >= 0x050000
#  include <QStandardPaths>
#else
#  include <utils/environment.h>
#endif
#include <utils/pathchooser.h>
#include <QDebug>
#include <QFileInfo>
#include <QSettings>

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
static const char additionalQueriesKeyC[] = "AdditionalQueries";

static const char defaultPortFlag[] = "-p";

enum { defaultPort = 29418 };

static inline QString detectSsh()
{
    const QByteArray gitSsh = qgetenv("GIT_SSH");
    if (!gitSsh.isEmpty())
        return QString::fromLocal8Bit(gitSsh);
#if QT_VERSION >= 0x050000
    QString ssh = QStandardPaths::findExecutable(QLatin1String(defaultSshC));
#else
    const Utils::Environment env = Utils::Environment::systemEnvironment();
    QString ssh = env.searchInPath(QLatin1String(defaultSshC));
#endif
    if (!ssh.isEmpty())
        return ssh;
#ifdef Q_OS_WIN // Windows: Use ssh.exe from git if it cannot be found.
    const QString git = GerritPlugin::gitBinary();
    if (!git.isEmpty()) {
        // Is 'git\cmd' in the path (folder containing .bats)?
        QString path = QFileInfo(git).absolutePath();
        if (path.endsWith(QLatin1String("cmd"), Qt::CaseInsensitive))
            path.replace(path.size() - 3, 3, QLatin1String("bin"));
        ssh = path + QLatin1Char('/') + QLatin1String(defaultSshC);
    }
#endif
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
           && ssh == rhs.ssh && additionalQueries == rhs.additionalQueries
           && https == rhs.https;
}

void GerritParameters::toSettings(QSettings *s) const
{
    s->beginGroup(QLatin1String(settingsGroupC));
    s->setValue(QLatin1String(hostKeyC), host);
    s->setValue(QLatin1String(userKeyC), user);
    s->setValue(QLatin1String(portKeyC), port);
    s->setValue(QLatin1String(portFlagKeyC), portFlag);
    s->setValue(QLatin1String(sshKeyC), ssh);
    s->setValue(QLatin1String(additionalQueriesKeyC), additionalQueries);
    s->setValue(QLatin1String(httpsKeyC), https);
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
    additionalQueries = s->value(rootKey + QLatin1String(additionalQueriesKeyC), QString()).toString();
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
