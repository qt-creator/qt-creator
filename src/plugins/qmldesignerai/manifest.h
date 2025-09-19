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

        QString toString(const QMap<QByteArray, QString> &tagsMap) const;
    };

    [[nodiscard]] static Manifest fromJsonResource(const QString &resourcePath);

    void setTagsMap(const QMap<QByteArray, QString> &tagsMap);
    bool hasNoRules() const;

    QString toString() const;

    void addManifest(const Manifest &manifest);

private: // functions
    void addRule(const QJsonObject &ruleObject);

private: // variables
    QString m_role;
    QList<Rule> m_rules;
    QMap<QByteArray, QString> m_tagsMap;
};

} // namespace QmlDesigner
