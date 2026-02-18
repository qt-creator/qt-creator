// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QPlainTextEdit>

namespace Utils { class HistoryCompleter; }

namespace AcpClient::Internal {

class ChatInputEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit ChatInputEdit(QWidget *parent = nullptr);

signals:
    void sendRequested();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void updateHeight();
    void historyUp();
    void historyDown();

    Utils::HistoryCompleter *m_history = nullptr;
    int m_historyIndex = -1;
    QString m_editBuffer;
};

} // namespace AcpClient::Internal
