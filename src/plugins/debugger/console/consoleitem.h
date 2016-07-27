/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <utils/treemodel.h>

#include <QString>
#include <functional>

namespace Debugger {
namespace Internal {

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
    int m_line;

    std::function<void(ConsoleItem *)> m_doFetch;
};

} // Internal
} // Debugger
