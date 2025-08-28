// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>
#include <QMap>
#include <QString>

QT_FORWARD_DECLARE_CLASS(QJsonObject)

namespace QmlDesigner {

class Manifest
{
public:
    struct Rule
    {
        QString name;
        QString description;
        bool hasTag = false;
    };

    struct Category
    {
        QString name;
        QList<Rule> rules;
    };

    [[nodiscard]] static Manifest fromJsonResource(const QString &resourcePath);

    void setTagsMap(const QMap<QByteArray, QString> &tagsMap);
    bool hasNoRules() const;

    QString toJsonContent() const;

private: // functions
    void addCategory(const QJsonObject &categoryObject);
    void addRule(Category &category, const QJsonObject &ruleObject);
    QJsonObject toJson(const Category &category) const;
    QJsonObject toJson(const Rule &rule) const;
    QString resolveTags(const QString &content) const;

private: // variables
    QString m_role;
    QList<Category> m_groups;
    QMap<QByteArray, QString> m_tagsMap;
};

} // namespace QmlDesigner
