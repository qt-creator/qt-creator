// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QModelIndex>
#include <QString>
#include <QTextEdit>

namespace Debugger::Internal {

class ConsoleEdit : public QTextEdit
{
    Q_OBJECT

public:
    ConsoleEdit(const QModelIndex &index, QWidget *parent);
    QString getCurrentScript() const;
    void setInputOffset(int left, int top) { setViewportMargins(left, top, 0, 0); }

protected:
    void keyPressEvent(QKeyEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;

signals:
    void editingFinished();

protected:
    void handleUpKey();
    void handleDownKey();
    void replaceCurrentScript(const QString &script);

private:
    void updateHistoryContext();

    QModelIndex m_historyIndex;
    QString m_cachedScript;
    int m_historyContext = 0;
};

} // Debugger::Internal
