/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qstringparser.h"

// Possible Improvements
// %w: skip optional whitespaces in m_source
// more types: different int types, float, char, word, some character delimited string
// some Q Types: QColor (named, hex rgb)
// introduce public type ParseState which can be got from Parser and gives some info like error state, error index etc


QStringParser::QStringParser(const QString &source)
    : m_source(source)
{
}

QStringParser::~QStringParser()
{
}

QStringParser::Parser QStringParser::parse(const QString &pattern)
{
    return Parser(m_source, pattern);
}


QStringParser::Parser::Parser(const QString &source, const QString &pattern)
    : m_source(source),
      m_pattern(pattern),
      m_evaluated(false),
      m_evaluationFailed(false)
{
}

QStringParser::Parser::~Parser()
{
    evaluate();
    qDeleteAll(m_nodes);
}

bool QStringParser::Parser::failed()
{
    evaluate();
    return m_evaluationFailed;
}

bool QStringParser::Parser::scan(int *i, int *index)
{
    *i = 0;
    int sign = 1;
    while (*index < m_source.length() && m_source.at(*index).isSpace()) {
        ++(*index);
    }
    if (*index >= m_source.length()) {
        return false;
    }
    if (m_source.at(*index) == QLatin1Char('+')) {
        ++(*index);
    } else if (m_source.at(*index) == QLatin1Char('-')) {
        sign = -1;
        ++(*index);
    }
    if (*index >= m_source.length() || !m_source.at(*index).isDigit()) {
        return false;
    }
    while (*index < m_source.length() && m_source.at(*index).isDigit()) {
        *i = *i * 10 + m_source.at(*index).digitValue();
        ++(*index);
    }
    *i *= sign;
    return true;
}

bool QStringParser::Parser::scan(double *d, int *index)
{
    int start_index = *index;
    // skip whitespaces
    while (*index < m_source.length() && m_source.at(*index).isSpace()) {
        ++(*index);
    }
    if (*index >= m_source.length()) {
        return false;
    }
    // sign
    if (m_source.at(*index) == QLatin1Char('+')) {
        ++(*index);
    } else if (m_source.at(*index) == QLatin1Char('-')) {
        ++(*index);
    }
    // int
    while (*index < m_source.length() && m_source.at(*index).isDigit()) {
        ++(*index);
    }
    // point
    if (*index < m_source.length() && m_source.at(*index) == QLatin1Char('.')) {
        ++(*index);
    }
    // int
    while (*index < m_source.length() && m_source.at(*index).isDigit()) {
        ++(*index);
    }
    // exponent
    if (*index < m_source.length() && m_source.at(*index).toLower() == QLatin1Char('e')) {
        ++(*index);
        if (*index >= m_source.length()) {
            return false;
        }
        // sign
        if (m_source.at(*index) == QLatin1Char('+')) {
            ++(*index);
        } else if (m_source.at(*index) == QLatin1Char('-')) {
            ++(*index);
        }
        // int
        while (*index < m_source.length() && m_source.at(*index).isDigit()) {
            ++(*index);
        }
    }
    bool ok = false;
    *d = m_source.mid(start_index, *index - start_index).toDouble(&ok);
    return ok;
}

void QStringParser::Parser::evaluate()
{
    if (!m_evaluated) {
        m_evaluated = true;
        m_evaluationFailed = false;

        int p = 0;
        int i = 0;
        while (p < m_pattern.length()) {
            if (m_pattern.at(p) == QLatin1Char('%')) {
                ++p;
                // a % must be followed by a another char.
                if (p >= m_pattern.length()) {
                    // syntax error in pattern
                    m_evaluationFailed = true;
                    return;
                }
                if (m_pattern.at(p) == QLatin1Char('%')) {
                    // two %% are handled like a simple % in m_source
                    ++p;
                    if (i >= m_source.length() || m_source.at(i) != QLatin1Char('%')) {
                        m_evaluationFailed = true;
                        return;
                    }
                    ++i;
                } else if (m_pattern.at(p).isDigit()) {
                    // now extract a value matching the Nth node type
                    int N = 0;
                    while (p < m_pattern.length() && m_pattern.at(p).isDigit()) {
                        N = N * 10 + m_pattern.at(p).digitValue();
                        ++p;
                    }
                    if (N < 1 || N > m_nodes.length()) {
                        // argument out of bounds in pattern
                        m_evaluationFailed = true;
                        return;
                    }
                    if (!m_nodes.at(N-1)->accept(*this, &i)) {
                        m_evaluationFailed = true;
                        return;
                    }
                } else {
                    // any other % syntax is an error
                    m_evaluationFailed = true;
                    return;
                }
            } else {
                if (m_pattern.at(p).isSpace()) {
                    ++p;
                    // m_source must end or have at least one space
                    if (i < m_source.length() && !m_source.at(i).isSpace()) {
                        m_evaluationFailed = true;
                        return;
                    }
                    // skip spaces in m_pattern
                    while (p < m_pattern.length() && m_pattern.at(p).isSpace()) {
                        ++p;
                    }
                    // skip spaces in m_source
                    while (i < m_source.length() && m_source.at(i).isSpace()) {
                        ++i;
                    }
                } else if (i >= m_source.length() || m_source.at(i) != m_pattern.at(p)) {
                    m_evaluationFailed = true;
                    return;
                } else {
                    ++p;
                    ++i;
                }
            }
        }
        // m_source and m_pattern must both be scanned completely
        if (i < m_source.length() || p < m_pattern.length()) {
            m_evaluationFailed = true;
            return;
        }
    }
}
