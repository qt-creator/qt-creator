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

#include "consoleitemmodel.h"

#include <QFontMetrics>
#include <QFont>

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// ConsoleItemModel
//
///////////////////////////////////////////////////////////////////////

ConsoleItemModel::ConsoleItemModel(QObject *parent) :
    Utils::TreeModel<>(new ConsoleItem, parent),
    m_maxSizeOfFileName(0), m_canFetchMore(false)
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
    const int pos = filename.lastIndexOf(QLatin1Char('/'));
    if (pos != -1)
        filename = filename.mid(pos + 1);

    QFontMetrics fm(font);
    m_maxSizeOfFileName = qMax(m_maxSizeOfFileName, fm.width(filename));

    return m_maxSizeOfFileName;
}

int ConsoleItemModel::sizeOfLineNumber(const QFont &font)
{
    QFontMetrics fm(font);
    return fm.width(QLatin1String("88888"));
}

} // Internal
} // Debugger
