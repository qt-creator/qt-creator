/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "gitlabparameters.h"

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QStandardPaths>

namespace GitLab {

const char settingsGroupC[] = "GitLab";
const char curlKeyC[] = "Curl";
const char defaultUuidKeyC[] = "DefaultUuid";

GitLabServer::GitLabServer()
{
}

GitLabServer::GitLabServer(const Utils::Id &id, const QString &host, const QString &description,
                           const QString &token, unsigned short port)
    : id(id)
    , host(host)
    , description(description)
    , token(token)
    , port(port)
{
}

bool GitLabServer::operator==(const GitLabServer &other) const
{
    if (port && other.port && port != other.port)
        return false;
    return id == other.id && host == other.host && description == other.description
            && token == other.token ;
}

bool GitLabServer::operator!=(const GitLabServer &other) const
{
    return !(*this == other);
}

QJsonObject GitLabServer::toJson() const
{
    QJsonObject result;
    result.insert("id", id.toString());
    result.insert("host", host);
    result.insert("description", description);
    result.insert("port", port);
    result.insert("token", token);
    return result;
}

GitLabServer GitLabServer::fromJson(const QJsonObject &json)
{
    GitLabServer invalid{Utils::Id(), "", "", "", 0};
    const QJsonValue id = json.value("id");
    if (id == QJsonValue::Undefined)
        return invalid;
    const QJsonValue host = json.value("host");
    if (host == QJsonValue::Undefined)
        return invalid;
    const QJsonValue description = json.value("description");
    if (description == QJsonValue::Undefined)
        return invalid;
    const QJsonValue token = json.value("token");
    if (token == QJsonValue::Undefined)
        return invalid;
    const QJsonValue port = json.value("port");
    if (port == QJsonValue::Undefined)
        return invalid;
    return {Utils::Id::fromString(id.toString()), host.toString(), description.toString(),
                token.toString(), (unsigned short)port.toInt()};
}

QStringList GitLabServer::curlArguments() const
{
    // credentials from .netrc (?), no progress
    QStringList args = { "-nsS" };
    if (!validateCert)
        args << "-k";
    return args;
}

QString GitLabServer::displayString() const
{
    if (!description.isEmpty())
        return host + " (" + description + ')';
    return host;
}

GitLabParameters::GitLabParameters()
{
}

bool GitLabParameters::equals(const GitLabParameters &other) const
{
    return curl == other.curl && defaultGitLabServer == other.defaultGitLabServer
            && gitLabServers == other.gitLabServers;
}

bool GitLabParameters::isValid() const
{
    const GitLabServer found = currentDefaultServer();
    return found.id.isValid() && !found.host.isEmpty() && curl.isExecutableFile() ;
}

static void writeTokensFile(const Utils::FilePath &filePath, const QList<GitLabServer> &servers)
{
    QJsonDocument doc;
    QJsonArray array;
    for (const GitLabServer &server : servers)
        array.append(server.toJson());
    doc.setArray(array);
    filePath.writeFileContents(doc.toJson());
    filePath.setPermissions(QFile::ReadUser | QFile::WriteUser);
}

static QList<GitLabServer> readTokensFile(const Utils::FilePath &filePath)
{
    if (!filePath.exists())
        return {};
    const QByteArray content = filePath.fileContents();
    const QJsonDocument doc = QJsonDocument::fromJson(content);
    if (!doc.isArray())
        return {};

    QList<GitLabServer> result;
    const QJsonArray array = doc.array();
    for (const auto &it : array) {
        if (it.isObject())
            result.append(GitLabServer::fromJson(it.toObject()));
    }
    return result;
}

static Utils::FilePath tokensFilePath(const QSettings *s)
{
    return Utils::FilePath::fromString(s->fileName()).parentDir()
            .pathAppended("/qtcreator/gitlabtokens.json");
}

void GitLabParameters::toSettings(QSettings *s) const
{

    writeTokensFile(tokensFilePath(s), gitLabServers);
    s->beginGroup(settingsGroupC);
    s->setValue(curlKeyC, curl.toVariant());
    s->setValue(defaultUuidKeyC, defaultGitLabServer.toSetting());
    s->endGroup();
}

void GitLabParameters::fromSettings(const QSettings *s)
{
    const QString rootKey = QLatin1String(settingsGroupC) + '/';
    curl = Utils::FilePath::fromVariant(s->value(rootKey + curlKeyC));
    defaultGitLabServer = Utils::Id::fromSetting(s->value(rootKey + defaultUuidKeyC));

    gitLabServers = readTokensFile(tokensFilePath(s));

    if (gitLabServers.isEmpty())
        defaultGitLabServer = Utils::Id();

    if (curl.isEmpty() || !curl.exists()) {
        const QString curlPath = QStandardPaths::findExecutable(
                    Utils::HostOsInfo::withExecutableSuffix("curl"));
        if (!curlPath.isEmpty())
            curl = Utils::FilePath::fromString(curlPath);
    }
}

GitLabServer GitLabParameters::currentDefaultServer() const
{
    return serverForId(defaultGitLabServer);
}

GitLabServer GitLabParameters::serverForId(const Utils::Id &id) const
{
    return Utils::findOrDefault(gitLabServers, [id](const GitLabServer &s) {
        return id == s.id;
    });
}

} // namespace GitLab
