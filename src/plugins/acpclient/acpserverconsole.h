// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/terminal/searchableterminal.h>

namespace AcpClient::Internal {

class AcpServerConsole final : public Core::SearchableTerminal
{
    Q_OBJECT

public:
    explicit AcpServerConsole(QWidget *parent = nullptr);

    void reset();
    void appendOutput(const QByteArray &data);
    void setInputEnabled(bool enabled);

    // Hides what is typed, the way a terminal does for a password: there is no
    // pty here, so the server cannot ask for it and the user has to.
    void setInputMasked(bool masked);
    bool isInputMasked() const { return m_inputMasked; }

signals:
    void inputEntered(const QByteArray &data);
    // Ctrl+D and Ctrl+C: end of input, and abandon the startup.
    void endOfInputRequested();
    void interruptRequested();

protected:
    qint64 writeToPty(const QByteArray &data) override;

private:
    enum class EscapeState { None, Start, Csi, Ss3 };

    bool skipEscapeSequence(char c);
    QByteArray eraseLineSequence() const;
    void drawLine();

    QByteArray m_line;
    QPoint m_lineStart;
    EscapeState m_escape = EscapeState::None;
    char m_lastOutputByte = 0;
    bool m_inputEnabled = false;
    bool m_inputMasked = false;
};

} // namespace AcpClient::Internal
