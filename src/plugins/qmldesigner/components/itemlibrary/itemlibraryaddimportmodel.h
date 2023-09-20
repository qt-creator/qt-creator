// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractListModel>
#include <import.h>

#include <QSet>

namespace QmlDesigner {

class ItemLibraryEntry;

class ItemLibraryAddImportModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit ItemLibraryAddImportModel(QObject *parent = nullptr);
    ~ItemLibraryAddImportModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void update(const Imports &possibleImports);
    void setSearchText(const QString &searchText);
    Import getImportAt(int index) const;

    Import getImport(const QString &importUrl) const;

private:
    QString m_searchText;
    Imports m_importList;
    QSet<QString> m_importFilterList;
    QHash<int, QByteArray> m_roleNames;
    QSet<QString> m_priorityImports;
};

} // namespace QmlDesigner
