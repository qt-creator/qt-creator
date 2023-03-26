// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
