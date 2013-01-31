/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "consoleitem.h"

namespace QmlJS {

///////////////////////////////////////////////////////////////////////
//
// ConsoleItem
//
///////////////////////////////////////////////////////////////////////

ConsoleItem::ConsoleItem(ConsoleItem *parent, ConsoleItem::ItemType itemType,
                               const QString &text)
    : m_parentItem(parent),
      itemType(itemType),
      line(-1)

{
    setText(text);
}

ConsoleItem::~ConsoleItem()
{
    qDeleteAll(m_childItems);
}

ConsoleItem *ConsoleItem::child(int number)
{
    return m_childItems.value(number);
}

int ConsoleItem::childCount() const
{
    return m_childItems.size();
}

int ConsoleItem::childNumber() const
{
    if (m_parentItem)
        return m_parentItem->m_childItems.indexOf(const_cast<ConsoleItem *>(this));

    return 0;
}

bool ConsoleItem::insertChildren(int position, int count)
{
    if (position < 0 || position > m_childItems.size())
        return false;

    for (int row = 0; row < count; ++row) {
        ConsoleItem *item = new ConsoleItem(this, ConsoleItem::UndefinedType,
                                                  QString());
        m_childItems.insert(position, item);
    }

    return true;
}

void ConsoleItem::insertChild(ConsoleItem *item, bool sorted)
{
    if (!sorted) {
        m_childItems.insert(m_childItems.count(), item);
        return;
    }

    int i = 0;
    for (; i < m_childItems.count(); i++) {
        if (item->m_text < m_childItems[i]->m_text)
            break;
    }
    m_childItems.insert(i, item);
}

bool ConsoleItem::insertChild(int position, ConsoleItem *item)
{
    if (position < 0 || position > m_childItems.size())
        return false;

    m_childItems.insert(position, item);

    return true;
}

ConsoleItem *ConsoleItem::parent()
{
    return m_parentItem;
}

bool ConsoleItem::removeChildren(int position, int count)
{
    if (position < 0 || position + count > m_childItems.size())
        return false;

    for (int row = 0; row < count; ++row)
        delete m_childItems.takeAt(position);

    return true;
}

bool ConsoleItem::detachChild(int position)
{
    if (position < 0 || position > m_childItems.size())
        return false;

    m_childItems.removeAt(position);

    return true;
}

void ConsoleItem::setText(const QString &text)
{
    m_text = text;
    for (int i = 0; i < m_text.length(); ++i) {
        if (m_text.at(i).isPunct())
            m_text.insert(++i, QChar(0x200b)); // ZERO WIDTH SPACE
    }
}

const QString &ConsoleItem::text() const
{
    return m_text;
}

} // QmlJS
