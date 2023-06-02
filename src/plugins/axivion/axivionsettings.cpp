// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "axivionsettings.h"

#include "axiviontr.h"

#include <utils/filepath.h>
#include <utils/hostosinfo.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QStandardPaths>

namespace Axivion::Internal {

AxivionServer::AxivionServer(const Utils::Id &id, const QString &dashboard,
                             const QString &description, const QString &token)
    : id(id)
    , dashboard(dashboard)
    , description(description)
    , token(token)
{
}

bool AxivionServer::operator==(const AxivionServer &other) const
{
    return id == other.id && dashboard == other.dashboard
            && description == other.description && token == other.token;
}

bool AxivionServer::operator!=(const AxivionServer &other) const
{
    return !(*this == other);
}

QJsonObject AxivionServer::toJson() const
{
    QJsonObject result;
    result.insert("id", id.toString());
    result.insert("dashboard", dashboard);
    result.insert("description", description);
    result.insert("token", token);
    return result;
}

AxivionServer AxivionServer::fromJson(const QJsonObject &json)
{
    const AxivionServer invalidServer;
    const QJsonValue id = json.value("id");
    if (id == QJsonValue::Undefined)
        return invalidServer;
    const QJsonValue dashboard = json.value("dashboard");
    if (dashboard == QJsonValue::Undefined)
        return invalidServer;
    const QJsonValue description = json.value("description");
    if (description == QJsonValue::Undefined)
        return invalidServer;
    const QJsonValue token = json.value("token");
    if (token == QJsonValue::Undefined)
        return invalidServer;
    return { Utils::Id::fromString(id.toString()), dashboard.toString(),
                description.toString(), token.toString() };
}

QStringList AxivionServer::curlArguments() const
{
    QStringList args { "-sS" }; // silent, but show error
    if (dashboard.startsWith("https://") && !validateCert)
        args << "-k";
    return args;
}

AxivionSettings::AxivionSettings()
{
    setSettingsGroup("Axivion");

    curl.setSettingsKey("Curl");
    curl.setLabelText(Tr::tr("curl:"));
    curl.setExpectedKind(Utils::PathChooser::ExistingCommand);
}

static Utils::FilePath tokensFilePath(const QSettings *s)
{
    return Utils::FilePath::fromString(s->fileName()).parentDir()
            .pathAppended("qtcreator/axivion.json");
}

static void writeTokenFile(const Utils::FilePath &filePath, const AxivionServer &server)
{
    QJsonDocument doc;
    doc.setObject(server.toJson());
    // FIXME error handling?
    filePath.writeFileContents(doc.toJson());
    filePath.setPermissions(QFile::ReadUser | QFile::WriteUser);
}

static AxivionServer readTokenFile(const Utils::FilePath &filePath)
{
    if (!filePath.exists())
        return {};
    Utils::expected_str<QByteArray> contents = filePath.fileContents();
    if (!contents)
        return {};
    const QJsonDocument doc = QJsonDocument::fromJson(*contents);
    if (!doc.isObject())
        return {};
    return AxivionServer::fromJson(doc.object());
}

void AxivionSettings::toSettings(QSettings *s) const
{
    writeTokenFile(tokensFilePath(s), server);
    Utils::AspectContainer::writeSettings(s);
}

void AxivionSettings::fromSettings(QSettings *s)
{
    Utils::AspectContainer::readSettings(s);
    server = readTokenFile(tokensFilePath(s));

    if (curl().isEmpty() || !curl().exists()) {
        const QString curlPath = QStandardPaths::findExecutable(
                    Utils::HostOsInfo::withExecutableSuffix("curl"));
        if (!curlPath.isEmpty())
            curl.setFilePath(Utils::FilePath::fromString(curlPath));
    }
}

} // Axivion::Internal
