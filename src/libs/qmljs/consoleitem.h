/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CONSOLEITEM_H
#define CONSOLEITEM_H

#include "qmljs_global.h"
#include <utils/treemodel.h>

#include <QString>
#include <functional>

namespace QmlJS {

class QMLJS_EXPORT ConsoleItem : public Utils::TreeItem
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
    QVariant data(int column, int role) const;
    bool setData(int column, const QVariant &data, int role);

    bool canFetchMore() const;
    void fetchMore();

private:
    ItemType m_itemType;
    QString m_text;
    QString m_file;
    int m_line;

    std::function<void(ConsoleItem *)> m_doFetch;
};

} // QmlJS

#endif // CONSOLEITEM_H
