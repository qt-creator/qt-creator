// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "defaultannotations.h"

#include <QColor>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>

namespace QmlDesigner {
DefaultAnnotationsModel::DefaultAnnotationsModel(QObject *parent)
    : QAbstractListModel(parent)
{
    qRegisterMetaType<RichTextProxy>();
}

DefaultAnnotationsModel::~DefaultAnnotationsModel() {}

int DefaultAnnotationsModel::rowCount(const QModelIndex &) const
{
    return static_cast<int>(m_defaults.size());
}

QVariant DefaultAnnotationsModel::data(const QModelIndex &index, int role) const
{
    const auto row = static_cast<size_t>(index.row());
    if (!index.isValid() || m_defaults.size() < row)
        return {};

    auto &item = m_defaults[row];

    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
    case Name:
        return item.first;
    case Type:
        return item.second.typeName();
    case Default:
        return item.second;
    }

    return {};
}

QVariantMap DefaultAnnotationsModel::fetchData() const
{
    return m_defaultMap;
}

bool DefaultAnnotationsModel::hasDefault(const Comment &comment) const
{
    return m_defaultMap.count(comment.title().toLower());
}

QMetaType::Type DefaultAnnotationsModel::defaultType(const Comment &comment) const
{
    return hasDefault(comment) ? QMetaType::Type(m_defaultMap[comment.title().toLower()].typeId())
                               : QMetaType::UnknownType;
}

QVariant DefaultAnnotationsModel::defaultValue(const Comment &comment) const
{
    return hasDefault(comment) ? m_defaultMap.value(comment.title().toLower()) : QVariant();
}

bool DefaultAnnotationsModel::isRichText(const Comment &comment) const
{
    const auto type = defaultType(comment);
    return type == QMetaType::UnknownType || type == qMetaTypeId<RichTextProxy>();
}

void DefaultAnnotationsModel::loadFromFile(const QString &filename)
{
    QFile file(filename);
    if (file.open(QFile::ReadOnly)) {
        loadFromFile(&file);
    }
}

void DefaultAnnotationsModel::loadFromFile(QIODevice *io)
{
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(io->readAll(), &error);

    if (error.error == QJsonParseError::NoError)
        loadFromJson(doc);
    else {
    } // TODO: Error handling
}

void DefaultAnnotationsModel::loadFromJson(const QJsonDocument &doc)
{
    beginResetModel();
    m_defaultMap = asVariantMapFromJson(doc);
    m_defaults.clear();
    m_defaults.reserve(m_defaultMap.size());

    for (auto &key : m_defaultMap.keys())
        m_defaults.emplace_back(key, m_defaultMap.value(key));

    endResetModel();
}

QVariantMap DefaultAnnotationsModel::asVariantMapFromJson(const QJsonDocument &doc)
{
    QVariantMap map;
    QJsonObject obj = doc.object();
    for (auto key : obj.keys()) {
        key = key.toLower();
        auto val = obj[key];

        switch (val.type()) {
        case QJsonValue::Double:
            map[key] = double{0.0};
            break;
        case QJsonValue::Bool:
            map[key] = false;
            break;
        case QJsonValue::Object: {
            auto o = val.toObject();
            auto type = o["type"].toString().toLower();
            auto val = o["value"].toVariant();

            if (type == QStringLiteral("richtext"))
                map[key] = QVariant::fromValue(RichTextProxy{val.toString()});
            else if (type == QStringLiteral("string"))
                map[key] = QVariant::fromValue(val.toString());
            else if (type == QStringLiteral("bool"))
                map[key] = QVariant::fromValue(val.toBool());
            else if (type == QStringLiteral("double"))
                map[key] = QVariant::fromValue(val.toDouble());
            else if (type == QStringLiteral("color"))
                map[key] = QVariant::fromValue(QColor(val.toString()));
            break;
        }
        case QJsonValue::String:
            map[key] = QString{};
            break;
        default:
            break;
        }
    }

    return map;
}

QString DefaultAnnotationsModel::defaultJsonFilePath()
{
    return QStringLiteral(":/annotationeditor/defaultannotations.json");
}

QString RichTextProxy::plainText() const
{
    QString plainText(text);
    plainText.remove(QRegularExpression("<.*?>"));
    return plainText.mid(plainText.indexOf("}") + 1);
}

} // namespace QmlDesigner
