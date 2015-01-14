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

#include <QList>
#include <QString>

namespace QmlJS {

class QMLJS_EXPORT ConsoleItem
{
public:
    enum ItemType
    {
        UndefinedType = 0x01, // Can be used for unknown and for Return values
        DebugType     = 0x02,
        WarningType   = 0x04,
        ErrorType     = 0x08,
        InputType     = 0x10,
        DefaultTypes  = InputType | UndefinedType
    };
    Q_DECLARE_FLAGS(ItemTypes, ItemType)

    ConsoleItem(ConsoleItem *parent,
                   ConsoleItem::ItemType type = ConsoleItem::UndefinedType,
                   const QString &data = QString());
    ~ConsoleItem();

    ConsoleItem *child(int number);
    int childCount() const;
    bool insertChildren(int position, int count);
    void insertChild(ConsoleItem *item, bool sorted);
    bool insertChild(int position, ConsoleItem *item);
    ConsoleItem *parent();
    bool removeChildren(int position, int count);
    bool detachChild(int position);
    int childNumber() const;
    void setText(const QString &text);
    QString text() const;
    QString expression() const;

private:
    ConsoleItem *m_parentItem;
    QList<ConsoleItem *> m_childItems;
    QString m_text;

public:
    ConsoleItem::ItemType itemType;
    QString file;
    int line;
};

} // QmlJS

#endif // CONSOLEITEM_H
