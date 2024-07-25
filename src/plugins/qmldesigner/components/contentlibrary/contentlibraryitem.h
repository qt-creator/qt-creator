// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "modelfwd.h"

#include <QObject>
#include <QUrl>

namespace QmlDesigner {

class ContentLibraryItem : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString bundleItemName MEMBER m_name CONSTANT)
    Q_PROPERTY(QUrl bundleItemIcon MEMBER m_icon CONSTANT)
    Q_PROPERTY(bool bundleItemVisible MEMBER m_visible NOTIFY itemVisibleChanged)
    Q_PROPERTY(bool bundleItemImported READ imported WRITE setImported NOTIFY itemImportedChanged)
    Q_PROPERTY(QString bundleId READ bundleId CONSTANT)

public:
    ContentLibraryItem(QObject *parent,
                       const QString &name,
                       const QString &qml,
                       const TypeName &type,
                       const QUrl &icon,
                       const QStringList &files,
                       const QString &bundleId);

    bool filter(const QString &searchText);

    QUrl icon() const;
    QString qml() const;
    TypeName type() const;
    QStringList files() const;
    bool visible() const;

    QString bundleId() const;

    bool setImported(bool imported);
    bool imported() const;

signals:
    void itemVisibleChanged();
    void itemImportedChanged();

private:
    QString m_name;
    QString m_qml;
    TypeName m_type;
    QUrl m_icon;
    QStringList m_files;

    bool m_visible = true;
    bool m_imported = false;

    const QString m_bundleId;
};

} // namespace QmlDesigner
