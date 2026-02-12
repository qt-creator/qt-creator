// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "mcpconfigloader.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>

namespace QmlDesigner {

namespace {

QStringList toStringList(const QJsonArray &arr)
{
    QStringList list;
    list.reserve(arr.size());
    for (const auto &v : arr)
        list << v.toString();
    return list;
}

QProcessEnvironment buildEnv(const QJsonObject &obj)
{
    auto env = QProcessEnvironment::systemEnvironment();
    for (auto it = obj.begin(); it != obj.end(); ++it)
        env.insert(it.key(), it.value().toString());
    return env;
}

QByteArray stripJsonComments(const QByteArray &in)
{
    // Remove // line comments
    QByteArray out = in;
    static const QRegularExpression reLine(R"(//[^\n\r]*)");
    out = QString::fromUtf8(out).remove(reLine).toUtf8();

    // Remove /* block comments */
    static const QRegularExpression
        reBlock(R"(/\*.*?\*/)", QRegularExpression::DotMatchesEverythingOption);
    out = QString::fromUtf8(out).remove(reBlock).toUtf8();
    return out;
}

QJsonObject deepMerge(const QJsonObject &base, const QJsonObject &over)
{
    QJsonObject out = base;
    for (auto it = over.begin(); it != over.end(); ++it) {
        const QString key = it.key();
        const QJsonValue v = it.value();

        if (!out.contains(key)) {
            out.insert(key, v);
            continue;
        }

        const QJsonValue existing = out.value(key);
        if (existing.isObject() && v.isObject())
            out.insert(key, deepMerge(existing.toObject(), v.toObject()));
        else
            out.insert(key, v); // override scalar/array entirely
    }
    return out;
}

} // namespace

McpConfigLoader::McpConfigLoader() = default;

void McpConfigLoader::setResolveMap(const QMap<QString, QString> &map)
{
    m_resolveMap = map;
}

bool McpConfigLoader::loadFile(const QString &path)
{
    QString err;
    const QJsonObject root = loadJsonObject(path, &err);
    if (root.isEmpty()) {
        qWarning() << "[McpConfigLoader] loadFile failed:" << err << "path=" << path;
        return false;
    }

    m_serversObj = root.value("mcpServers").toObject();
    if (m_serversObj.isEmpty()) {
        qWarning() << "[McpConfigLoader] No 'servers' object in" << path;
        return false;
    }

    return true;
}

bool McpConfigLoader::loadPrecedence(
    const QString &globalPath, const QString &projectPath)
{
    QString errGlobal, errProject;
    const QJsonObject glob = loadJsonObject(globalPath, &errGlobal);
    const QJsonObject proj = loadJsonObject(projectPath, &errProject);

    if (glob.isEmpty() && proj.isEmpty()) {
        qWarning() << "[McpConfigLoader] No config loaded. Global error:" << errGlobal
                   << ", Project error:" << errProject;
        return false;
    }

    // Precedence: global first, then project overrides
    QJsonObject combined;
    if (!glob.isEmpty())
        combined = deepMerge(combined, glob);
    if (!proj.isEmpty())
        combined = deepMerge(combined, proj);

    const QJsonObject serversObj = combined.value("mcpServers").toObject();
    if (serversObj.isEmpty()) {
        qWarning() << "[McpConfigLoader] Combined config missing 'servers'";
        return false;
    }

    return true;
}

QList<McpServerConfig> McpConfigLoader::getConfigs() const
{
    QList<McpServerConfig> configs;

    // Iterate entries: name -> serverConf
    for (auto it = m_serversObj.begin(); it != m_serversObj.end(); ++it) {
        const QString serverName = it.key();
        QJsonObject cfg = it.value().toObject();

        if (!cfg.value("enabled").toBool(true))
            continue;

        if (cfg.value("disabled").toBool(false))
            continue;

        // Transport
        const QString transportStr = cfg.value("transport").toString().toLower();
        McpServerConfig::Transport tr = (transportStr == "http") ? McpServerConfig::Http
                                                                 : McpServerConfig::Stdio;

        McpServerConfig serverConfig;
        serverConfig.name = serverName;
        serverConfig.transport = tr;

        if (tr == McpServerConfig::Stdio) {
            serverConfig.command = cfg.value("command").toString();
            serverConfig.args = toStringList(cfg.value("args").toArray());
            serverConfig.workingDir = cfg.value("workingDir").toString();
            serverConfig.env = buildEnv(cfg.value("env").toObject());
        } else { // http
            serverConfig.url = cfg.value("url").toString();
            const auto auth = cfg.value("auth").toObject();
            // If tokenEnv present, read env var now (optional)
            const QString tokenEnv = auth.value("tokenEnv").toString();
            if (!tokenEnv.isEmpty()) {
                const auto env = QProcessEnvironment::systemEnvironment();
                serverConfig.bearerToken = env.value(tokenEnv);
            }
        }

        // TODO: handle allowedTools and requireConsent

        configs.append(serverConfig);
    }
    return configs;
}

QJsonObject McpConfigLoader::loadJsonObject(const QString &path, QString *errOut) const
{
    if (errOut)
        *errOut = QString();

    QFile f(path);
    if (!f.exists()) {
        if (errOut)
            *errOut = QStringLiteral("File does not exist");
        return {};
    }

    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errOut)
            *errOut = QStringLiteral("Cannot open file: %1").arg(f.errorString());
        return {};
    }

    const QByteArray raw = f.readAll();
    f.close();

    // Support JSON with comments (jsonc)
    const QByteArray decommented = stripJsonComments(raw);

    QJsonParseError pe{};
    const QJsonDocument doc = QJsonDocument::fromJson(decommented, &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errOut)
            *errOut = QStringLiteral("Parse error: %1").arg(pe.errorString());
        return {};
    }

    QJsonObject object = doc.object();
    object = resolveStringTokens(object).toObject();
    return object;
}

// replace placeholders (tokens) in value with values from the m_resolveMap
QJsonValue McpConfigLoader::resolveStringTokens(const QJsonValue &value) const
{
    if (value.isString()) {
        QString valueStr = value.toString();
        for (const auto &resolvePair : m_resolveMap.asKeyValueRange())
            valueStr.replace(resolvePair.first, resolvePair.second);
        return valueStr;
    } else if (value.isObject()) {
        const QJsonObject object = value.toObject();
        const QStringList keys = object.keys();
        QJsonObject result;
        for (const QString &key : keys) {
            const QJsonValue unresolvedValue = object.value(key);
            const QJsonValue value = resolveStringTokens(unresolvedValue);
            result.insert(key, value);
        }
        return result;
    } else if (value.isArray()) {
        const QJsonArray array = value.toArray();
        QJsonArray result;
        for (const QJsonValue &item : array)
            result.append(resolveStringTokens(item));
        return result;
    }
    return value;
}

} // namespace QmlDesigner
