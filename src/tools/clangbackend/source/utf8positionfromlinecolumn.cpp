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

#include "utf8positionfromlinecolumn.h"

#include <QtGlobal>

namespace ClangBackEnd {

Utf8PositionFromLineColumn::Utf8PositionFromLineColumn(const char *utf8Text)
    : m_utf8Text(utf8Text)
    , m_currentByte(utf8Text)
{
}

bool Utf8PositionFromLineColumn::find(uint line, uint column)
{
    if (!m_utf8Text || *m_utf8Text == '\0' || line == 0 || column == 0)
        return false;

    return advanceToLine(line)
        && advanceToColumn(column);
}

uint Utf8PositionFromLineColumn::position() const
{
    return m_previousByte - m_utf8Text;
}

bool Utf8PositionFromLineColumn::advanceToLine(uint line)
{
    if (line == 1)
        return true;

    uint currentLine = 1;
    do {
        if (*m_currentByte == '\n' && ++currentLine == line) {
            advanceCodePoint();
            return true;
        }
    } while (advanceCodePoint());

    return false;
}

bool Utf8PositionFromLineColumn::advanceToColumn(uint column)
{
    while (column) {
        if (advanceCodePoint(/*stopOnNewLine=*/ true))
            --column;
        else
            break;
    }

    return column == 0;
}

static bool isByteOfMultiByteCodePoint(unsigned char byte)
{
    return byte & 0x80; // Check if most significant bit is set
}

bool Utf8PositionFromLineColumn::advanceCodePoint(bool stopOnNewLine)
{
    if (Q_UNLIKELY(*m_currentByte == '\0') || (stopOnNewLine && *m_currentByte == '\n'))
        return false;

    m_previousByte = m_currentByte;

    // Process multi-byte UTF-8 code point (non-latin1)
    if (Q_UNLIKELY(isByteOfMultiByteCodePoint(*m_currentByte))) {
        unsigned trailingBytesCurrentCodePoint = 1;
        for (unsigned char c = (*m_currentByte) << 2; isByteOfMultiByteCodePoint(c); c <<= 1)
            ++trailingBytesCurrentCodePoint;
        m_currentByte += trailingBytesCurrentCodePoint + 1;

    // Process single-byte UTF-8 code point (latin1)
    } else {
        ++m_currentByte;
    }

    return true;
}

} // namespace ClangBackEnd
