// Copyright (C) 2018 Benjamin Balga
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QLineEdit>

namespace SerialTerminal {
namespace Internal {

class ConsoleLineEdit : public QLineEdit
{
public:
    explicit ConsoleLineEdit(QWidget *parent = nullptr);

    void addHistoryEntry();
    void loadHistoryEntry(int entryIndex);

protected:
    void keyPressEvent(QKeyEvent *event) final;

private:
    QStringList m_history;
    int m_maxEntries; // TODO: add to settings
    int m_currentEntryIndex = 0;
    QString m_editingEntry;
};

} // namespace Internal
} // namespace SerialTerminal
