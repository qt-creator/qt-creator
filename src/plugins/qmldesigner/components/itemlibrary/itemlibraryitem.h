// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QString>
#include <QSize>
#include <QVariant>

#include "itemlibraryinfo.h"

namespace QmlDesigner {

class ItemLibraryItem: public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariant itemLibraryEntry READ itemLibraryEntry FINAL)
    Q_PROPERTY(QString itemName READ itemName FINAL)
    Q_PROPERTY(QString itemLibraryIconPath READ itemLibraryIconPath FINAL)
    Q_PROPERTY(bool itemVisible READ isVisible NOTIFY visibilityChanged FINAL)
    Q_PROPERTY(QString componentPath READ componentPath FINAL)
    Q_PROPERTY(bool itemUsable READ isUsable FINAL)
    Q_PROPERTY(QString itemRequiredImport READ requiredImport FINAL)
    Q_PROPERTY(QString itemComponentSource READ componentSource FINAL)
    Q_PROPERTY(QString toolTip READ toolTip FINAL)

public:
    ItemLibraryItem(const ItemLibraryEntry &itemLibraryEntry, bool isImported, QObject *parent);
    ~ItemLibraryItem() override;

    QString itemName() const;
    QString typeName() const;
    QString itemLibraryIconPath() const;
    QString componentPath() const;
    QString requiredImport() const;
    QString componentSource() const;
    QString toolTip() const;

    bool setVisible(bool isVisible);
    bool isVisible() const;
    bool isUsable() const;

    QVariant itemLibraryEntry() const;

signals:
    void visibilityChanged();

private:
    ItemLibraryEntry m_itemLibraryEntry;
    bool m_isVisible = true;
    bool m_isUsable = false;
};

} // namespace QmlDesigner
