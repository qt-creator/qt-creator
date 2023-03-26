// Copyright (C) 2016 Denis Mingulov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QSharedPointer>
#include <QStandardItem>

namespace Utils { class FilePath; }

namespace ClassView::Internal {

class ManagerPrivate;

class Manager : public QObject
{
    Q_OBJECT
public:
    explicit Manager(QObject *parent = nullptr);
    ~Manager() override;
    static Manager *instance();

    bool canFetchMore(QStandardItem *item, bool skipRoot = false) const;
    void fetchMore(QStandardItem *item, bool skipRoot = false);
    bool hasChildren(QStandardItem *item) const;

    void gotoLocation(const Utils::FilePath &filePath, int line = 0, int column = 0);
    void gotoLocations(const QList<QVariant> &locations);
    void setFlatMode(bool flat);
    void onWidgetVisibilityIsChanged(bool visibility);

signals:
    void treeDataUpdate(QSharedPointer<QStandardItem> result);

private:
    void initialize();

    inline bool state() const;
    void setState(bool state);

    ManagerPrivate *d;
};

} // ClassView::Internal
