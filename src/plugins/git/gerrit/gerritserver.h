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

#pragma once

#include <QStringList>

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
    QString curlBinary;
    StoredHostValidity loadSettings();
    void saveSettings(StoredHostValidity validity) const;
    bool setupAuthentication();
    bool ascendPath();
    bool resolveRoot();
    void resolveVersion(const GerritParameters &p, bool forceReload);
};

} // namespace Internal
} // namespace Gerrit
