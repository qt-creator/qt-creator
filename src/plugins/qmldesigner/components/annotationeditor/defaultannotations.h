// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "annotation.h"

#include <QAbstractListModel>

QT_BEGIN_NAMESPACE
class QJsonDocument;
QT_END_NAMESPACE

namespace QmlDesigner {

// We need this proxy type to distinguish between a 'normal' QString
// and a 'richtext' string when they are stored in a QVariant
struct RichTextProxy
{
    QString text;

    QString plainText() const;
};

class DefaultAnnotationsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Role { Name = Qt::UserRole + 1, Type, Default };
    Q_ENUM(Role)

    DefaultAnnotationsModel(QObject *parent = nullptr);
    ~DefaultAnnotationsModel() override;

    int rowCount(const QModelIndex & = {}) const override;
    QVariant data(const QModelIndex &, int role) const override;

    QVariantMap fetchData() const;

    bool hasDefault(const Comment &comment) const;
    QMetaType::Type defaultType(const Comment &comment) const;
    QVariant defaultValue(const Comment &comment) const;
    bool isRichText(const Comment &comment) const;

    void loadFromFile(QString const &);
    void loadFromFile(QIODevice *);
    void loadFromJson(const QJsonDocument &);

    static QVariantMap asVariantMapFromJson(const QJsonDocument &);
    static QString defaultJsonFilePath();

private:
    std::vector<std::pair<QString, QVariant>> m_defaults;
    QVariantMap m_defaultMap;
};

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::RichTextProxy);
