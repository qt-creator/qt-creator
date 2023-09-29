// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gitlabparameters.h"

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcsettings.h>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

using namespace Utils;

namespace GitLab {

const char settingsGroupC[] = "GitLab";
const char curlKeyC[] = "Curl";
const char defaultUuidKeyC[] = "DefaultUuid";

GitLabServer::GitLabServer()
{
}

GitLabServer::GitLabServer(const Utils::Id &id, const QString &host, const QString &description,
                           const QString &token, unsigned short port, bool secure)
    : id(id)
    , host(host)
    , description(description)
    , token(token)
    , port(port)
    , secure(secure)
{
}

bool GitLabServer::operator==(const GitLabServer &other) const
{
    if (port && other.port && port != other.port)
        return false;
    return secure == other.secure && id == other.id && host == other.host
            && description == other.description && token == other.token ;
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
    result.insert("secure", secure);
    return result;
}

GitLabServer GitLabServer::fromJson(const QJsonObject &json)
{
    GitLabServer invalid{Utils::Id(), "", "", "", 0, true};
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
    const bool secure = json.value("secure").toBool(true);
    return {Utils::Id::fromString(id.toString()), host.toString(), description.toString(),
                token.toString(), (unsigned short)port.toInt(), secure};
}

QStringList GitLabServer::curlArguments() const
{
    // credentials from .netrc (?), no progress
    QStringList args = { "-nsS" };
    if (secure && !validateCert)
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

void GitLabParameters::assign(const GitLabParameters &other)
{
    curl = other.curl;
    defaultGitLabServer = other.defaultGitLabServer;
    gitLabServers = other.gitLabServers;
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
    const Utils::expected_str<QByteArray> contents = filePath.fileContents();
    if (!contents)
        return {};
    const QByteArray content = *contents;
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

static FilePath tokensFilePath(const QtcSettings *s)
{
    return FilePath::fromString(s->fileName()).parentDir()
            .pathAppended("/qtcreator/gitlabtokens.json");
}

void GitLabParameters::toSettings(QtcSettings *s) const
{

    writeTokensFile(tokensFilePath(s), gitLabServers);
    s->beginGroup(settingsGroupC);
    s->setValue(curlKeyC, curl.toSettings());
    s->setValue(defaultUuidKeyC, defaultGitLabServer.toSetting());
    s->endGroup();
}

void GitLabParameters::fromSettings(const QtcSettings *s)
{
    const Key rootKey = Key(settingsGroupC) + '/';
    curl = FilePath::fromSettings(s->value(rootKey + curlKeyC));
    defaultGitLabServer = Id::fromSetting(s->value(rootKey + defaultUuidKeyC));

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
