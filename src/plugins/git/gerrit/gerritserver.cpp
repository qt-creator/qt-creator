// Copyright (C) 2017 Orgad Shaneh <orgads@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gerritserver.h"

#include "authenticationdialog.h"
#include "gerritparameters.h"
#include "../gitclient.h"
#include "../gittr.h"

#include <coreplugin/icore.h>

#include <utils/commandline.h>
#include <utils/hostosinfo.h>

#include <vcsbase/vcscommand.h>

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QRegularExpression>

using namespace Git::Internal;
using namespace Utils;
using namespace VcsBase;

namespace Gerrit {
namespace Internal {

static const char defaultHostC[] = "codereview.qt-project.org";
static const char accountUrlC[] = "/accounts/self";
static const char versionUrlC[] = "/config/server/version";
static const char isGerritKey[] = "IsGerrit";
static const char rootPathKey[] = "RootPath";
static const char userNameKey[] = "UserName";
static const char fullNameKey[] = "FullName";
static const char isAuthenticatedKey[] = "IsAuthenticated";
static const char validateCertKey[] = "ValidateCert";
static const char versionKey[] = "Version";

enum ErrorCodes
{
    CertificateError = 60,
    Success = 200,
    UnknownError = 400,
    AuthenticationFailure = 401,
    PageNotFound = 404
};

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
    return host == other.host && user.isSameAs(other.user) && type == other.type
            && authenticated == other.authenticated;
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
    if (type == Ssh || urlType == UrlWithHttpUser)
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
    if (type == GerritServer::Ssh) {
        return resolveVersion(parameters, forceReload);
    }
    curlBinary = parameters.curl;
    if (curlBinary.isEmpty() || !curlBinary.exists())
        return false;
    const StoredHostValidity validity = forceReload ? Invalid : loadSettings();
    switch (validity) {
    case Invalid:
        rootPath = r.path;
        // Strip the last part of the path, which is always the repo name
        // The rest of the path needs to be inspected to find the root path
        // (can be http://example.net/review)
        ascendPath();
        if (resolveRoot()) {
            if (!resolveVersion(parameters, forceReload))
                return false;
            saveSettings(Valid);
            return true;
        }
        return false;
    case NotGerrit:
        return false;
    case Valid:
        return resolveVersion(parameters, false);
    }
    return true;
}

GerritServer::StoredHostValidity GerritServer::loadSettings()
{
    StoredHostValidity validity = Invalid;
    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup("Gerrit/" + keyFromString(host));
    if (!settings->value(isGerritKey, true).toBool()) {
        validity = NotGerrit;
    } else if (settings->contains(isAuthenticatedKey)) {
        rootPath = settings->value(rootPathKey).toString();
        user.userName = settings->value(userNameKey).toString();
        user.fullName = settings->value(fullNameKey).toString();
        authenticated = settings->value(isAuthenticatedKey).toBool();
        validateCert = settings->value(validateCertKey, true).toBool();
        validity = Valid;
    }
    settings->endGroup();
    return validity;
}

void GerritServer::saveSettings(StoredHostValidity validity) const
{
    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup("Gerrit/" + keyFromString(host));
    switch (validity) {
    case NotGerrit:
        settings->setValue(isGerritKey, false);
        break;
    case Valid:
        settings->setValue(rootPathKey, rootPath);
        settings->setValue(userNameKey, user.userName);
        settings->setValue(fullNameKey, user.fullName);
        settings->setValue(isAuthenticatedKey, authenticated);
        settings->setValue(validateCertKey, validateCert);
        break;
    case Invalid:
        settings->clear();
        break;
    }

    settings->endGroup();
}

QStringList GerritServer::curlArguments() const
{
    // -f - fail silently on server error
    // -n - use credentials from ~/.netrc (or ~/_netrc on Windows)
    // -sS - silent, except server error (no progress)
    // --basic, --digest - try both authentication types
    QStringList res = {"-fnsS", "--basic", "--digest"};
    if (!validateCert)
        res << "-k"; // -k - insecure - do not validate certificate
    return res;
}

int GerritServer::testConnection()
{
    const QStringList arguments = curlArguments() << (url(RestUrl) + accountUrlC);
    const CommandResult result = gitClient().vcsSynchronousExec({}, {curlBinary, arguments});
    if (result.result() == ProcessResult::FinishedWithSuccess) {
        QString output = result.cleanedStdOut();
        // Gerrit returns an empty response for /p/qt-creator/a/accounts/self
        // so consider this as 404.
        if (output.isEmpty())
            return PageNotFound;
        output.remove(0, output.indexOf('\n')); // Strip first line
        QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
        if (!doc.isNull()) {
            const QJsonObject obj = doc.object();
            user.fullName = obj.value("name").toString();
            const QString userName = obj.value("username").toString();
            if (!userName.isEmpty())
                user.userName = userName;
        }
        return Success;
    }
    if (result.exitCode() == CertificateError)
        return CertificateError;
    const QRegularExpression errorRegexp("returned error: (\\d+)");
    QRegularExpressionMatch match = errorRegexp.match(result.cleanedStdErr());
    if (match.hasMatch())
        return match.captured(1).toInt();
    return UnknownError;
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
        case Success:
            saveSettings(Valid);
            return true;
        case AuthenticationFailure:
            return setupAuthentication();
        case CertificateError:
            if (QMessageBox::question(
                        Core::ICore::dialogParent(),
                        ::Git::Tr::tr("Certificate Error"),
                        ::Git::Tr::tr(
                            "Server certificate for %1 cannot be authenticated.\n"
                            "Do you want to disable SSL verification for this server?\n"
                            "Note: This can expose you to man-in-the-middle attack.")
                        .arg(host))
                    == QMessageBox::Yes) {
                validateCert = false;
            } else {
                return false;
            }
            break;
        case PageNotFound:
            if (!ascendPath()) {
                saveSettings(NotGerrit);
                return false;
            }
            break;
        default: // unknown error - fail
            authenticated = false;
            return false;
        }
    }
    return false;
}

bool GerritServer::resolveVersion(const GerritParameters &p, bool forceReload)
{
    QtcSettings *settings = Core::ICore::settings();
    const Key fullVersionKey = "Gerrit/" + keyFromString(host) + '/' + versionKey;
    version = settings->value(fullVersionKey).toString();
    if (!version.isEmpty() && !forceReload)
        return true;
    if (type == Ssh) {
        QStringList arguments;
        if (port)
            arguments << p.portFlag << QString::number(port);
        arguments << hostArgument() << "gerrit" << "version";
        const CommandResult result = gitClient().vcsSynchronousExec({}, {p.ssh, arguments},
                                                                    RunFlags::NoOutput);
        QString stdOut = result.cleanedStdOut().trimmed();
        stdOut.remove("gerrit version ");
        version = stdOut;
        if (version.isEmpty())
            return false;
    } else {
        const QStringList arguments = curlArguments() << (url(RestUrl) + versionUrlC);
        const CommandResult result = gitClient().vcsSynchronousExec({}, {curlBinary, arguments},
                                                                    RunFlags::NoOutput);
        // REST endpoint for version is only available from 2.8 and up. Do not consider invalid
        // if it fails.
        if (result.result() == ProcessResult::FinishedWithSuccess) {
            QString output = result.cleanedStdOut();
            if (output.isEmpty())
                return false;
            output.remove(0, output.indexOf('\n')); // Strip first line
            output.remove('\n');
            output.remove('"');
            version = output;
        }
    }
    settings->setValue(fullVersionKey, version);
    return true;
}

} // namespace Internal
} // namespace Gerrit
