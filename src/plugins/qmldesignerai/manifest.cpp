// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "manifest.h"

#include <utils/fileutils.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

namespace QmlDesigner {

namespace {

const QRegularExpression &tagRegex()
{
    static const QRegularExpression regex(R"(\[\[(?<tagName>[a-zA-Z0-9_]+)\]\])");
    return regex;
};

bool hasTag(const QString &content)
{
    return tagRegex().match(content).hasMatch();
}

} // namespace

Manifest Manifest::fromJsonResource(const QString &resourcePath)
{
    Manifest manifest;
    QString resourceData = Utils::FileUtils::fetchQrc(resourcePath);

    if (resourceData.isEmpty()) {
        qWarning() << __FUNCTION__ << QString{"Manifest `%1` is empty"}.arg(resourcePath);
        return manifest;
    }

    QJsonParseError jsonParseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(resourceData.toUtf8(), &jsonParseError);
    if (jsonParseError.error != QJsonParseError::NoError) {
        qWarning() << __FUNCTION__
                   << QString{"Manifest `%1` has invalid json format. %2"}
                          .arg(resourcePath, jsonParseError.errorString());
        return manifest;
    }

    if (!jsonDoc.isObject()) {
        qWarning() << __FUNCTION__
                   << QString{"Manifest should be a json object [`%1`]."}.arg(resourcePath);
        return manifest;
    }

    QJsonObject rootObject = jsonDoc.object();
    manifest.m_role = rootObject.value("role").toString();

    const QJsonArray rules = rootObject.value("rules").toArray();
    for (const QJsonValue &rule : rules)
        manifest.addRule(rule.toObject());

    return manifest;
}

void Manifest::setTagsMap(const QMap<QByteArray, QString> &tagsMap)
{
    m_tagsMap = tagsMap;
}

bool Manifest::hasNoRules() const
{
    return m_rules.isEmpty();
}

QString Manifest::toString() const
{
    QString result = QString("%1\n").arg(m_role);

    for (const Rule &rule : std::as_const(m_rules))
        result.append(rule.toString(m_tagsMap)).append('\n');

    return result;
}

void Manifest::addRule(const QJsonObject &ruleObject)
{
    QTC_ASSERT(!ruleObject.isEmpty(), return);

    Manifest::Rule rule;
    rule.name = ruleObject.value("name").toString();
    rule.description = ruleObject.value("description").toString();

    if (rule.name.isEmpty() || rule.description.isEmpty()) {
        qWarning() << __FUNCTION__ << "Invalid rule" << rule.name << rule.description;
        return;
    }

    rule.hasTag = hasTag(rule.description);
    m_rules.append(rule);
}

void Manifest::addManifest(const Manifest &manifest)
{
    if (!manifest.m_role.isEmpty())
        m_role = manifest.m_role;

    m_rules.append(manifest.m_rules); // note: assumption is no duplicate rules
}

QString resolveTags(const QString &content, const QMap<QByteArray, QString> &tagsMap)
{
    QString result = content;
    QRegularExpressionMatchIterator tag = tagRegex().globalMatch(content);
    QSet<QByteArray> tagNames;
    while (tag.hasNext())
        tagNames << tag.next().captured("tagName").toUtf8();

    for (const QByteArray &tagName : std::as_const(tagNames)) {
        const QString tag = QString("[[%1]]").arg(tagName);
        const QString value = tagsMap.value(tagName);
        if (value.isEmpty())
            return {};
        result.replace(tag, value);
    }
    return result;
}

QString Manifest::Rule::toString(const QMap<QByteArray, QString> &tagsMap) const
{
    const QString &desc = hasTag ? resolveTags(description, tagsMap) : description;
    if (description.isEmpty())
        return {};

    return QString("**%1**: %2").arg(name, desc);
}

} // namespace QmlDesigner
