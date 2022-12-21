// Copyright (C) 2017 Orgad Shaneh <orgads@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

namespace Gerrit {
namespace Internal {

class GerritParameters;

class GerritUser
{
public:
    bool isSameAs(const GerritUser &other) const;

    QString userName;
    QString fullName;
    QString email;
};

class GerritServer
{
public:
    enum { defaultPort = 29418 };

    enum HostType
    {
        Http,
        Https,
        Ssh
    };

    enum UrlType
    {
        DefaultUrl,
        UrlWithHttpUser,
        RestUrl
    };

    enum StoredHostValidity
    {
        Invalid,
        NotGerrit,
        Valid
    };

    GerritServer();
    GerritServer(const QString &host, unsigned short port, const QString &userName, HostType type);
    bool operator==(const GerritServer &other) const;
    static QString defaultHost();
    QString hostArgument() const;
    QString url(UrlType urlType = DefaultUrl) const;
    bool fillFromRemote(const QString &remote, const GerritParameters &parameters, bool forceReload);
    int testConnection();
    QStringList curlArguments() const;

    QString host;
    GerritUser user;
    QString rootPath; // for http
    QString version;
    unsigned short port = 0;
    HostType type = Ssh;
    bool authenticated = true;
    bool validateCert = true;

private:
    Utils::FilePath curlBinary;
    StoredHostValidity loadSettings();
    void saveSettings(StoredHostValidity validity) const;
    bool setupAuthentication();
    bool ascendPath();
    bool resolveRoot();
    bool resolveVersion(const GerritParameters &p, bool forceReload);
};

} // namespace Internal
} // namespace Gerrit
