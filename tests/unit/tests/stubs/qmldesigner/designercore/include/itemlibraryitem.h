// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QSize>
#include <QString>
#include <QVariant>

#include "itemlibraryinfo.h"

namespace QmlDesigner {

class ItemLibraryItem : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariant itemLibraryEntry READ itemLibraryEntry FINAL)
    Q_PROPERTY(QString itemName READ itemName FINAL)
    Q_PROPERTY(QString itemLibraryIconPath READ itemLibraryIconPath FINAL)
    Q_PROPERTY(bool itemVisible READ isVisible NOTIFY visibilityChanged FINAL)

public:
    ItemLibraryItem(QObject *) {}
    ~ItemLibraryItem() override {}

    QString itemName() const { return {}; }
    QString typeName() const { return {}; }
    QString itemLibraryIconPath() const { return {}; }

    bool setVisible(bool) { return {}; }
    bool isVisible() const { return {}; }

    void setItemLibraryEntry(const ItemLibraryEntry &) {}
    QVariant itemLibraryEntry() const { return {}; }

signals:
    void visibilityChanged();
};

} // namespace QmlDesigner
