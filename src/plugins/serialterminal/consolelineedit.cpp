/****************************************************************************
**
** Copyright (C) 2018 Benjamin Balga
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

#include "consolelineedit.h"
#include "serialterminalconstants.h"

#include <QKeyEvent>

namespace SerialTerminal {
namespace Internal {

ConsoleLineEdit::ConsoleLineEdit(QWidget *parent) :
    QLineEdit(parent),
    m_maxEntries(Constants::DEFAULT_MAX_ENTRIES)
{
    connect(this, &QLineEdit::returnPressed, this, &ConsoleLineEdit::addHistoryEntry);
}

// Add current text to history entries, if not empty and different from last entry.
// Called when return key is pressed.
void ConsoleLineEdit::addHistoryEntry()
{
    m_currentEntryIndex = 0;
    const QString currentText = text();

    if (currentText.isEmpty())
        return;

    if (!m_history.isEmpty() && m_history.first() == currentText)
        return;

    m_history.prepend(currentText);
    if (m_history.size() > m_maxEntries)
        m_history.removeLast();
}

// Load a specific history entry: 0 = current, n = n-most last entry
void ConsoleLineEdit::loadHistoryEntry(int entryIndex)
{
    if (entryIndex < 0 || entryIndex > m_history.size())
        return;

    if (m_currentEntryIndex == 0)
        m_editingEntry = text();

    if (entryIndex <= 0 && m_currentEntryIndex > 0) {
        m_currentEntryIndex = 0;
        setText(m_editingEntry);
    } else if (entryIndex > 0) {
        m_currentEntryIndex = entryIndex;
        setText(m_history.at(entryIndex-1));
    }
}

void ConsoleLineEdit::keyPressEvent(QKeyEvent *event)
{
    // Navigate history with up/down keys
    if (event->key() == Qt::Key_Up) {
        loadHistoryEntry(m_currentEntryIndex+1);
        event->accept();
    } else if (event->key() == Qt::Key_Down) {
        loadHistoryEntry(m_currentEntryIndex-1);
        event->accept();
    } else {
        QLineEdit::keyPressEvent(event);
    }
}

} // namespace Internal
} // namespace SerialTerminal
