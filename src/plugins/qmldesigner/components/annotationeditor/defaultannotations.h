/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
