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

QString getJsonText(const QJsonValue &value)
{
    if (value.isString())
        return value.toString();

    if (!value.isArray())
        return {};

    QString text;
    const QJsonArray &array = value.toArray();
    for (const QJsonValue &arrayValue : array) {
        if (!arrayValue.isString())
            return {};

        QString partText = arrayValue.toString();
        if (!text.isEmpty()) {
            if (partText.isEmpty())
                text.append(QChar::LineFeed);
            else if (!text.at(text.size() - 1).isSpace() && !partText.at(0).isSpace())
                text.append(QChar::Space);
        }
        text.append(partText);
    }
    return text;
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
    manifest.m_role = getJsonText(rootObject.value(u"role"));

    const QJsonArray groups = rootObject.value(u"groups").toArray();

    for (const QJsonValue &category : groups)
        manifest.addCategory(category.toObject());

    return manifest;
}

void Manifest::setTagsMap(const QMap<QByteArray, QString> &tagsMap)
{
    m_tagsMap = tagsMap;
}

bool Manifest::hasNoRules() const
{
    return m_groups.isEmpty();
}

QString Manifest::toJsonContent() const
{
    QJsonObject object;
    QJsonArray groups;

    for (const Category &category : std::as_const(m_groups))
        groups.append(toJson(category));

    object.insert(u"role", m_role);
    object.insert(u"groups", groups);

    return QString{"```json\n%1\n```"}.arg(QJsonDocument{object}.toJson(QJsonDocument::Compact));
}

void Manifest::addCategory(const QJsonObject &categoryObject)
{
    if (categoryObject.isEmpty()) {
        qWarning() << __FUNCTION__ << "Empty category";
        return;
    }

    const QString name = categoryObject.value("category").toString();
    const QJsonArray rules = categoryObject.value("rules").toArray();

    if (name.isEmpty() || rules.isEmpty()) {
        qWarning() << __FUNCTION__ << "Invalid category" << name;
        return;
    }

    Category category;
    category.name = name;

    for (const QJsonValue &rule : rules)
        addRule(category, rule.toObject());

    if (category.rules.isEmpty()) {
        qWarning() << __FUNCTION__ << "Empty category" << name;
        return;
    }

    m_groups.append(category);
}

void Manifest::addRule(Category &category, const QJsonObject &ruleObject)
{
    if (ruleObject.isEmpty()) {
        qWarning() << __FUNCTION__ << "Empty rule";
        return;
    }

    Manifest::Rule rule;
    rule.name = ruleObject.value(u"name").toString();
    rule.description = getJsonText(ruleObject.value(u"description"));

    if (rule.name.isEmpty() || rule.description.isEmpty()) {
        qWarning() << __FUNCTION__ << "Invalid rule" << rule.name << rule.description;
        return;
    }

    rule.hasTag = hasTag(rule.description);
    category.rules.append(rule);
}

QJsonObject Manifest::toJson(const Category &category) const
{
    QJsonObject categoryObject;
    QJsonArray rulesArray;

    for (const Rule &rule : category.rules) {
        const QJsonObject &ruleObject = toJson(rule);
        if (!ruleObject.isEmpty())
            rulesArray.append(toJson(rule));
    }

    categoryObject.insert(u"category", category.name);
    categoryObject.insert(u"rules", rulesArray);

    return categoryObject;
}

QJsonObject Manifest::toJson(const Rule &rule) const
{
    const QString &description = rule.hasTag ? resolveTags(rule.description) : rule.description;
    if (description.isEmpty())
        return {};

    return QJsonObject{
        {"name", rule.name},
        {"description", description},
    };
}

QString Manifest::resolveTags(const QString &content) const
{
    QString result = content;
    QRegularExpressionMatchIterator tag = tagRegex().globalMatch(content);
    QSet<QByteArray> tagNames;
    while (tag.hasNext())
        tagNames << tag.next().captured("tagName").toUtf8();

    for (const QByteArray &tagName : std::as_const(tagNames)) {
        const QString tag = QString("[[%1]]").arg(tagName);
        const QString value = m_tagsMap.value(tagName);
        if (value.isEmpty())
            return {};
        result.replace(tag, value);
    }
    return result;
}

} // namespace QmlDesigner
