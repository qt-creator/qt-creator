/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "stringtextsource.h"

#include "sourcepos.h"
#include "qmt/infrastructure/qmtassert.h"

namespace qmt {

StringTextSource::StringTextSource()
{
}

StringTextSource::~StringTextSource()
{
}

void StringTextSource::setText(const QString &text)
{
    m_text = text;
    m_index = 0;
    m_lineNumber = 1;
    m_columnNumber = 1;
}

void StringTextSource::setSourceId(int sourceId)
{
    m_sourceId = sourceId;
}

SourceChar StringTextSource::readNextChar()
{
    QMT_ASSERT(m_sourceId >= 0, return SourceChar());
    QMT_ASSERT(m_index >= 0, return SourceChar());
    QMT_ASSERT(m_lineNumber >= 0, return SourceChar());
    QMT_ASSERT(m_columnNumber >= 0, return SourceChar());

    if (m_index >= m_text.length())
        return SourceChar(QChar(), SourcePos(m_sourceId, m_lineNumber, m_columnNumber));

    SourcePos pos(m_sourceId, m_lineNumber, m_columnNumber);
    QChar ch(m_text.at(m_index));
    ++m_index;
    if (ch == QChar::LineFeed) {
        ++m_lineNumber;
        m_columnNumber = 1;
    } else {
        ++m_columnNumber;
    }
    return SourceChar(ch, pos);
}

} // namespace qmt
