// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <designsystem/dsstore.h>

#include <QObject>
class QAbstractItemModel;

namespace QmlDesigner {
class CollectionModel;
class DSThemeManager;

class DesignSystemInterface : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList collections READ collections NOTIFY collectionsChanged FINAL)

public:
    DesignSystemInterface();
    ~DesignSystemInterface();

    Q_INVOKABLE void loadDesignSystem();
    Q_INVOKABLE CollectionModel *model(const QString &typeName);

    Q_INVOKABLE QString generateCollectionName(const QString &hint) const;
    Q_INVOKABLE void addCollection(const QString &name);
    Q_INVOKABLE void removeCollection(const QString &name);
    Q_INVOKABLE void renameCollection(const QString &oldName, const QString &newName);

    Q_INVOKABLE ThemeProperty createThemeProperty(const QString &name,
                                                  const QVariant &value,
                                                  bool isBinding = false) const;

    QStringList collections() const;

    void setDSStore(DSStore *store);

signals:
    void collectionsChanged();

private:
    CollectionModel *createModel(const QString &typeName, DSThemeManager *collection);

private:
    class DSStore *m_store = nullptr;
    std::map<QString, CollectionModel> m_models;
};

} // namespace QmlDesigner
