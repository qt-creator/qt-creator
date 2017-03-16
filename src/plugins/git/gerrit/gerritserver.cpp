/****************************************************************************
**
** Copyright (C) 2017 Orgad Shaneh <orgads@gmail.com>.
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

#include "authenticationdialog.h"
#include "gerritparameters.h"
#include "gerritserver.h"
#include "../gitplugin.h"
#include "../gitclient.h"

#include <coreplugin/icore.h>
#include <coreplugin/shellcommand.h>
#include <utils/hostosinfo.h>

#include <QFile>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSettings>

using namespace Utils;
using namespace Git::Internal;

namespace Gerrit {
namespace Internal {

static const char defaultHostC[] = "codereview.qt-project.org";
static const char accountUrlC[] = "/accounts/self";
static const char isGerritKey[] = "IsGerrit";
static const char rootPathKey[] = "RootPath";
static const char userNameKey[] = "UserName";
static const char fullNameKey[] = "FullName";
static const char isAuthenticatedKey[] = "IsAuthenticated";

bool GerritUser::isSameAs(const GerritUser &other) const
{
    if (!userName.isEmpty() && !other.userName.isEmpty())
        return userName == other.userName;
    if (!fullName.isEmpty() && !other.fullName.isEmpty())
        return fullName == other.fullName;
    return false;
}

GerritServer::GerritServer()
    : host(defaultHost())
    , port(defaultPort)
{
}

GerritServer::GerritServer(const QString &host, unsigned short port,
                           const QString &userName, HostType type)
    : host(host)
    , port(port)
    , type(type)
{
    user.userName = userName;
}

bool GerritServer::operator==(const GerritServer &other) const
{
    if (port && other.port && port != other.port)
        return false;
    return host == other.host && user.isSameAs(other.user) && type == other.type;
}

QString GerritServer::defaultHost()
{
    return QLatin1String(defaultHostC);
}

QString GerritServer::hostArgument() const
{
    if (!authenticated || user.userName.isEmpty())
        return host;
    return user.userName + '@' + host;
}

QString GerritServer::url(UrlType urlType) const
{
    QString protocol;
    switch (type) {
        case Ssh:   protocol = "ssh"; break;
        case Http:  protocol = "http"; break;
        case Https: protocol = "https"; break;
    }
    QString res = protocol + "://";
    if (type == Ssh || urlType != DefaultUrl)
        res += hostArgument();
    else
        res += host;
    if (port)
        res += ':' + QString::number(port);
    if (type != Ssh) {
        res += rootPath;
        if (authenticated && urlType == RestUrl)
            res += "/a";
    }
    return res;
}

bool GerritServer::fillFromRemote(const QString &remote,
                                  const GerritParameters &parameters,
                                  bool forceReload)
{
    const GitRemote r(remote);
    if (!r.isValid)
        return false;

    if (r.protocol == "https")
        type = GerritServer::Https;
    else if (r.protocol == "http")
        type = GerritServer::Http;
    else if (r.protocol.isEmpty() || r.protocol == "ssh")
        type = GerritServer::Ssh;
    else
        return false;

    if (r.host.contains("github.com")) // Clearly not gerrit
        return false;
    host = r.host;
    port = r.port;
    user.userName = r.userName.isEmpty() ? parameters.server.user.userName : r.userName;
    if (type == GerritServer::Ssh)
        return true;
    curlBinary = parameters.curl;
    if (curlBinary.isEmpty() || !QFile::exists(curlBinary))
        return false;
    const StoredHostValidity validity = forceReload ? Invalid : loadSettings();
    switch (validity) {
    case Invalid:
        rootPath = r.path;
        // Strip the last part of the path, which is always the repo name
        // The rest of the path needs to be inspected to find the root path
        // (can be http://example.net/review)
        ascendPath();
        return resolveRoot();
    case NotGerrit:
        return false;
    case Valid:
        return true;
    }
    return true;
}

GerritServer::StoredHostValidity GerritServer::loadSettings()
{
    StoredHostValidity validity = Invalid;
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup("Gerrit/" + host);
    if (!settings->value(isGerritKey, true).toBool()) {
        validity = NotGerrit;
    } else if (settings->contains(isAuthenticatedKey)) {
        rootPath = settings->value(rootPathKey).toString();
        user.userName = settings->value(userNameKey).toString();
        user.fullName = settings->value(fullNameKey).toString();
        authenticated = settings->value(isAuthenticatedKey).toBool();
        validity = Valid;
    }
    settings->endGroup();
    return validity;
}

void GerritServer::saveSettings(StoredHostValidity validity) const
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup("Gerrit/" + host);
    switch (validity) {
    case NotGerrit:
        settings->setValue(isGerritKey, false);
        break;
    case Valid:
        settings->setValue(rootPathKey, rootPath);
        settings->setValue(userNameKey, user.userName);
        settings->setValue(fullNameKey, user.fullName);
        settings->setValue(isAuthenticatedKey, authenticated);
        break;
    case Invalid:
        settings->clear();
        break;
    }

    settings->endGroup();
}

QStringList GerritServer::curlArguments()
{
    // -k - insecure - do not validate certificate
    // -f - fail silently on server error
    // -n - use credentials from ~/.netrc (or ~/_netrc on Windows)
    // -sS - silent, except server error (no progress)
    // --basic, --digest - try both authentication types
    return {"-kfnsS", "--basic", "--digest"};
}

int GerritServer::testConnection()
{
    static GitClient *const client = GitPlugin::client();
    const QStringList arguments = curlArguments() << (url(RestUrl) + accountUrlC);
    const SynchronousProcessResponse resp = client->vcsFullySynchronousExec(
                QString(), FileName::fromString(curlBinary), arguments,
                Core::ShellCommand::NoOutput);
    if (resp.result == SynchronousProcessResponse::Finished) {
        QString output = resp.stdOut();
        output.remove(0, output.indexOf('\n')); // Strip first line
        QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
        if (!doc.isNull()) {
            const QJsonObject obj = doc.object();
            user.fullName = obj.value("name").toString();
            const QString userName = obj.value("username").toString();
            if (!userName.isEmpty())
                user.userName = userName;
        }
        return 200;
    }
    const QRegularExpression errorRegexp("returned error: (\\d+)");
    QRegularExpressionMatch match = errorRegexp.match(resp.stdErr());
    if (match.hasMatch())
        return match.captured(1).toInt();
    return 400;
}

bool GerritServer::setupAuthentication()
{
    AuthenticationDialog dialog(this);
    if (!dialog.exec())
        return false;
    authenticated = dialog.isAuthenticated();
    saveSettings(Valid);
    return true;
}

bool GerritServer::ascendPath()
{
    const int lastSlash = rootPath.lastIndexOf('/');
    if (lastSlash == -1)
        return false;
    rootPath = rootPath.left(lastSlash);
    return true;
}

bool GerritServer::resolveRoot()
{
    for (;;) {
        switch (testConnection()) {
        case 200:
            saveSettings(Valid);
            return true;
        case 401:
            return setupAuthentication();
        case 404:
            if (!ascendPath()) {
                saveSettings(NotGerrit);
                return false;
            }
            break;
        default: // unknown error - fail
            return false;
        }
    }
    return false;
}

} // namespace Internal
} // namespace Gerrit
