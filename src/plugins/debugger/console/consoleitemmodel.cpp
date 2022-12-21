// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "consoleitemmodel.h"

#include <QFontMetrics>
#include <QFont>

namespace Debugger::Internal {

ConsoleItemModel::ConsoleItemModel(QObject *parent) :
    Utils::TreeModel<>(new ConsoleItem, parent)
{
    clear();
}

void ConsoleItemModel::clear()
{
    Utils::TreeModel<>::clear();
    appendItem(new ConsoleItem(ConsoleItem::InputType));
    emit selectEditableRow(index(0, 0, QModelIndex()), QItemSelectionModel::ClearAndSelect);
}

void ConsoleItemModel::setCanFetchMore(bool canFetchMore)
{
    m_canFetchMore = canFetchMore;
}

bool ConsoleItemModel::canFetchMore(const QModelIndex &parent) const
{
    return m_canFetchMore && TreeModel::canFetchMore(parent);
}

void ConsoleItemModel::appendItem(ConsoleItem *item, int position)
{
    if (position < 0)
        position = rootItem()->childCount() - 1; // append before editable row

    if (position < 0)
        position = 0;

    rootItem()->insertChild(position, item);
}

void ConsoleItemModel::shiftEditableRow()
{
    int position = rootItem()->childCount();
    Q_ASSERT(position > 0);

    appendItem(new ConsoleItem(ConsoleItem::InputType), position);
    emit selectEditableRow(index(position, 0, QModelIndex()), QItemSelectionModel::ClearAndSelect);
}

int ConsoleItemModel::sizeOfFile(const QFont &font)
{
    int lastReadOnlyRow = rootItem()->childCount();
    lastReadOnlyRow -= 2; // skip editable row
    if (lastReadOnlyRow < 0)
        return 0;
    QString filename = static_cast<ConsoleItem *>(rootItem()->childAt(lastReadOnlyRow))->file();
    const int pos = filename.lastIndexOf('/');
    if (pos != -1)
        filename = filename.mid(pos + 1);

    QFontMetrics fm(font);
    m_maxSizeOfFileName = qMax(m_maxSizeOfFileName, fm.horizontalAdvance(filename));

    return m_maxSizeOfFileName;
}

int ConsoleItemModel::sizeOfLineNumber(const QFont &font)
{
    QFontMetrics fm(font);
    return fm.horizontalAdvance("88888");
}

} // Debugger::Internal
