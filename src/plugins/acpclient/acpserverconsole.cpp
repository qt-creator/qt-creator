// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acpserverconsole.h"

#include <utils/theme/theme.h>

#include <QFont>

using namespace Utils;

namespace AcpClient::Internal {

// The control characters the console acts on; everything else below space is
// dropped.
enum ControlChar : char {
    Interrupt = '\x03',
    EndOfTransmission = '\x04',
    Backspace = '\x08',
    Escape = '\x1b',
    Delete = '\x7f',
};

static std::array<QColor, 20> consoleColors()
{
    return {creatorColor(Theme::TerminalAnsi0),
            creatorColor(Theme::TerminalAnsi1),
            creatorColor(Theme::TerminalAnsi2),
            creatorColor(Theme::TerminalAnsi3),
            creatorColor(Theme::TerminalAnsi4),
            creatorColor(Theme::TerminalAnsi5),
            creatorColor(Theme::TerminalAnsi6),
            creatorColor(Theme::TerminalAnsi7),
            creatorColor(Theme::TerminalAnsi8),
            creatorColor(Theme::TerminalAnsi9),
            creatorColor(Theme::TerminalAnsi10),
            creatorColor(Theme::TerminalAnsi11),
            creatorColor(Theme::TerminalAnsi12),
            creatorColor(Theme::TerminalAnsi13),
            creatorColor(Theme::TerminalAnsi14),
            creatorColor(Theme::TerminalAnsi15),
            creatorColor(Theme::TerminalForeground),
            creatorColor(Theme::TerminalBackground),
            creatorColor(Theme::TerminalSelection),
            creatorColor(Theme::TerminalFindMatch)};
}

AcpServerConsole::AcpServerConsole(QWidget *parent)
    : Core::SearchableTerminal(parent)
{
    setColors(consoleColors());

    QFont font(TerminalSolution::defaultFontFamily(), TerminalSolution::defaultFontSize());
    font.setFixedPitch(true);
    setFont(font);
}

void AcpServerConsole::reset()
{
    m_line.clear();
    m_lineStart = {};
    m_escape = EscapeState::None;
    m_lastOutputByte = 0;
    restart();
}

void AcpServerConsole::appendOutput(const QByteArray &data)
{
    // The server writes to a pipe, so its output carries bare line feeds. The
    // terminal needs the carriage return to return to the first column.
    QByteArray text;
    text.reserve(data.size());
    for (const char c : data) {
        if (c == '\n' && m_lastOutputByte != '\r')
            text.append('\r');
        text.append(c);
        m_lastOutputByte = c;
    }

    if (m_line.isEmpty()) {
        writeToTerminal(text, false);
        return;
    }

    // A line being typed stays below what the server writes, the way a shell
    // redraws its prompt: take it off, write the output, and put it back
    // where the output ended.
    writeToTerminal(eraseLineSequence() + text, false);
    m_lineStart = surface()->cursor().position;
    drawLine();
}

void AcpServerConsole::setInputEnabled(bool enabled)
{
    m_inputEnabled = enabled;
    if (!enabled) {
        m_line.clear();
        m_escape = EscapeState::None;
    }
}

void AcpServerConsole::setInputMasked(bool masked)
{
    if (m_inputMasked == masked)
        return;
    m_inputMasked = masked;
    setPasswordMode(masked);
    if (!m_line.isEmpty())
        drawLine();
}

qint64 AcpServerConsole::writeToPty(const QByteArray &data)
{
    // Consumed either way: returning less would make the surface keep the
    // bytes and offer them again, forever.
    if (!m_inputEnabled)
        return data.size();

    // The server's stdin is a pipe, so nothing echoes what is typed and
    // nothing edits it; both happen here. The line is handed over on Return,
    // terminated by the line feed a pipe reader expects. A batch - a paste -
    // is drawn once, not once per byte.
    bool changed = false;
    for (const char c : data) {
        if (skipEscapeSequence(c))
            continue;
        if (c == '\r' || c == '\n') {
            if (changed)
                drawLine();
            changed = false;
            m_line.append('\n');
            writeToTerminal("\r\n", true);
            emit inputEntered(m_line);
            m_line.clear();
        } else if (c == Backspace || c == Delete) {
            if (m_line.isEmpty())
                continue;
            while (m_line.size() > 1 && (static_cast<uchar>(m_line.back()) & 0xC0) == 0x80)
                m_line.chop(1);
            m_line.chop(1);
            changed = true;
        } else if (c == Interrupt) {
            if (changed)
                drawLine();
            changed = false;
            writeToTerminal("^C\r\n", true);
            m_line.clear();
            emit interruptRequested();
        } else if (c == EndOfTransmission) {
            // As in a terminal, end of input only means that on an empty line.
            if (!m_line.isEmpty())
                continue;
            writeToTerminal("^D\r\n", true);
            emit endOfInputRequested();
        } else if (static_cast<uchar>(c) >= ' ') {
            if (m_line.isEmpty())
                m_lineStart = surface()->cursor().position;
            m_line.append(c);
            changed = true;
        }
    }
    if (changed)
        drawLine();
    return data.size();
}

// The keys that do not type anything - arrows, Home, End, function keys -
// arrive as escape sequences. There is no cursor to move within the line, so
// they are dropped whole: letting through what follows the ESC would put "[D"
// into the line, unseen when the input is masked.
bool AcpServerConsole::skipEscapeSequence(char c)
{
    switch (m_escape) {
    case EscapeState::None:
        if (c != Escape)
            return false;
        m_escape = EscapeState::Start;
        return true;
    case EscapeState::Start:
        // CSI and SS3 go on; anything else is Alt with a key, which ends here.
        if (c == '[')
            m_escape = EscapeState::Csi;
        else if (c == 'O')
            m_escape = EscapeState::Ss3;
        else
            m_escape = EscapeState::None;
        return true;
    case EscapeState::Csi:
        // Parameter and intermediate bytes, up to the final byte.
        if (static_cast<uchar>(c) >= 0x40 && static_cast<uchar>(c) <= 0x7e)
            m_escape = EscapeState::None;
        return true;
    case EscapeState::Ss3:
        m_escape = EscapeState::None;
        return true;
    }
    return false;
}

// Returns the cursor to where the line started and clears from there to the
// end of the screen. Going by cursor positions rather than by what was drawn
// holds for characters two cells wide, and for a line that has wrapped.
QByteArray AcpServerConsole::eraseLineSequence() const
{
    const int rowsUp = surface()->cursor().position.y() - m_lineStart.y();
    QByteArray sequence;
    if (rowsUp > 0)
        sequence += "\x1b[" + QByteArray::number(rowsUp) + 'A';
    sequence += "\x1b[" + QByteArray::number(m_lineStart.x() + 1) + "G\x1b[J";
    return sequence;
}

void AcpServerConsole::drawLine()
{
    // Masked, what is drawn is nothing at all - the terminal's own answer to a
    // password, and the cursor carries the lock.
    QByteArray sequence = eraseLineSequence();
    if (!m_inputMasked)
        sequence += m_line;
    writeToTerminal(sequence, true);
}

} // namespace AcpClient::Internal
