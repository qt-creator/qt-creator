// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "consoleitem.h"

namespace Debugger::Internal {

static QString addZeroWidthSpace(QString text)
{
    for (int i = 0; i < text.length(); ++i) {
        if (text.at(i).isPunct())
            text.insert(++i, QChar(0x200b)); // ZERO WIDTH SPACE
    }
    return text;
}

ConsoleItem::ConsoleItem(ItemType itemType, const QString &expression, const QString &file,
                         int line) :
    m_itemType(itemType), m_text(addZeroWidthSpace(expression)), m_file(file), m_line(line)
{}

ConsoleItem::ConsoleItem(ConsoleItem::ItemType itemType, const QString &expression,
                         std::function<void(ConsoleItem *)> doFetch) :
    m_itemType(itemType), m_text(addZeroWidthSpace(expression)), m_doFetch(doFetch)
{}

ConsoleItem::ItemType ConsoleItem::itemType() const
{
    return m_itemType;
}

QString ConsoleItem::text() const
{
    return m_text;
}

QString ConsoleItem::file() const
{
    return m_file;
}

int ConsoleItem::line() const
{
    return m_line;
}

Qt::ItemFlags ConsoleItem::flags(int) const
{
    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    // Disable editing for old editable row
    if (m_itemType == InputType && parent()->lastChild() == this)
          f |= Qt::ItemIsEditable;
    return f;
}

QVariant ConsoleItem::data(int column, int role) const
{
    if (column != 0)
        return QVariant();

    switch (role)
    {
    case TypeRole:
        return m_itemType;
    case FileRole:
        return m_file;
    case LineRole:
        return m_line;
    case ExpressionRole:
        return expression();
    case Qt::DisplayRole:
        return m_text;
    default:
        return TreeItem::data(column, role);
    }
}

bool ConsoleItem::setData(int column, const QVariant &data, int role)
{
    if (column != 0)
        return false;

    switch (role)
    {
    case TypeRole:
        m_itemType = ItemType(data.toInt());
        return true;
    case FileRole:
        m_file = data.toString();
        return true;
    case LineRole:
        m_line = data.toInt();
        return true;
    case ExpressionRole:
        m_text = addZeroWidthSpace(data.toString());
        return true;
    case Qt::DisplayRole:
        m_text = data.toString();
        return true;
    default:
        return TreeItem::setData(column, data, role);
    }
}

bool ConsoleItem::canFetchMore() const
{
    // Always fetch all children, too, as the labels depend on them.
    for (TreeItem *child : *this) {
        if (static_cast<ConsoleItem *>(child)->m_doFetch)
            return true;
    }

    return bool(m_doFetch);
}

void ConsoleItem::fetchMore()
{
    if (m_doFetch) {
        m_doFetch(this);
        m_doFetch = std::function<void(ConsoleItem *)>();
    }

    for (TreeItem *child : *this) {
        auto item = static_cast<ConsoleItem*>(child);
        if (item->m_doFetch) {
            item->m_doFetch(item);
            item->m_doFetch = m_doFetch;
        }
    }
}

QString ConsoleItem::expression() const
{
    return text().remove(QChar(0x200b));  // ZERO WIDTH SPACE
}

} // Debugger::Internal
