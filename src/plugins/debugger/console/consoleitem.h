// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/treemodel.h>

namespace Debugger::Internal {

class ConsoleItem : public Utils::TreeItem
{
public:
    enum Roles {
        TypeRole = Qt::UserRole,
        FileRole,
        LineRole,
        ExpressionRole
    };

    enum ItemType
    {
        DefaultType  = 0x01, // Can be used for unknown and for Return values
        DebugType    = 0x02,
        WarningType  = 0x04,
        ErrorType    = 0x08,
        InputType    = 0x10,
        AllTypes     = DefaultType | DebugType | WarningType | ErrorType | InputType
    };
    Q_DECLARE_FLAGS(ItemTypes, ItemType)

    ConsoleItem(ItemType itemType = ConsoleItem::DefaultType, const QString &expression = QString(),
                const QString &file = QString(), int line = -1);
    ConsoleItem(ItemType itemType, const QString &expression,
                std::function<void(ConsoleItem *)> doFetch);

    ItemType itemType() const;
    QString expression() const;
    QString text() const;
    QString file() const;
    int line() const;
    Qt::ItemFlags flags(int column) const override;
    QVariant data(int column, int role) const override;
    bool setData(int column, const QVariant &data, int role) override;

    bool canFetchMore() const override;
    void fetchMore() override;

private:
    ItemType m_itemType;
    QString m_text;
    QString m_file;
    int m_line = -1;

    std::function<void(ConsoleItem *)> m_doFetch;
};

} // Debugger::Internal
