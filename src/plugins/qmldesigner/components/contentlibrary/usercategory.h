// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QObject>

namespace QmlDesigner {

class UserCategory : public QObject
{
    Q_OBJECT

public:
    UserCategory(const QString &title, const Utils::FilePath &bundlePath);

    QString title() const;
    QObjectList items() const;

    bool isEmpty() const;
    void setIsEmpty(bool val);

    bool noMatch() const;
    void setNoMatch(bool val);

    virtual void loadBundle(bool force = false) = 0;
    virtual void filter(const QString &searchText) = 0;

    void addItem(QObject *item);
    void removeItem(QObject *item);

    Utils::FilePath bundlePath() const;

signals:
    void itemsChanged();
    void isEmptyChanged();
    void noMatchChanged();

protected:
    QString m_title;
    Utils::FilePath m_bundlePath;
    QObjectList m_items;
    bool m_isEmpty = true;
    bool m_noMatch = true;
    bool m_bundleLoaded = false;
};

} // namespace QmlDesigner
