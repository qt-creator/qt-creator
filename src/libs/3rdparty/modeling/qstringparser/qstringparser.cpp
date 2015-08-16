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
// %w: skip optional whitespaces in _source
// more types: different int types, float, char, word, some character delimited string
// some Q Types: QColor (named, hex rgb)
// introduce public type ParseState which can be got from Parser and gives some info like error state, error index etc


QStringParser::QStringParser(const QString &source)
    : _source(source)
{
}

QStringParser::~QStringParser()
{
}

QStringParser::Parser QStringParser::parse(const QString &pattern)
{
    return Parser(_source, pattern);
}


QStringParser::Parser::Parser(const QString &source, const QString &pattern)
    : _source(source),
      _pattern(pattern),
      _evaluated(false),
      _evaluation_failed(false)
{
}

QStringParser::Parser::~Parser()
{
    evaluate();
    qDeleteAll(_nodes);
}

bool QStringParser::Parser::failed()
{
    evaluate();
    return _evaluation_failed;
}

bool QStringParser::Parser::scan(int *i, int *index)
{
    *i = 0;
    int sign = 1;
    while (*index < _source.length() && _source.at(*index).isSpace()) {
        ++(*index);
    }
    if (*index >= _source.length()) {
        return false;
    }
    if (_source.at(*index) == QLatin1Char('+')) {
        ++(*index);
    } else if (_source.at(*index) == QLatin1Char('-')) {
        sign = -1;
        ++(*index);
    }
    if (*index >= _source.length() || !_source.at(*index).isDigit()) {
        return false;
    }
    while (*index < _source.length() && _source.at(*index).isDigit()) {
        *i = *i * 10 + _source.at(*index).digitValue();
        ++(*index);
    }
    *i *= sign;
    return true;
}

bool QStringParser::Parser::scan(double *d, int *index)
{
    int start_index = *index;
    // skip whitespaces
    while (*index < _source.length() && _source.at(*index).isSpace()) {
        ++(*index);
    }
    if (*index >= _source.length()) {
        return false;
    }
    // sign
    if (_source.at(*index) == QLatin1Char('+')) {
        ++(*index);
    } else if (_source.at(*index) == QLatin1Char('-')) {
        ++(*index);
    }
    // int
    while (*index < _source.length() && _source.at(*index).isDigit()) {
        ++(*index);
    }
    // point
    if (*index < _source.length() && _source.at(*index) == QLatin1Char('.')) {
        ++(*index);
    }
    // int
    while (*index < _source.length() && _source.at(*index).isDigit()) {
        ++(*index);
    }
    // exponent
    if (*index < _source.length() && _source.at(*index).toLower() == QLatin1Char('e')) {
        ++(*index);
        if (*index >= _source.length()) {
            return false;
        }
        // sign
        if (_source.at(*index) == QLatin1Char('+')) {
            ++(*index);
        } else if (_source.at(*index) == QLatin1Char('-')) {
            ++(*index);
        }
        // int
        while (*index < _source.length() && _source.at(*index).isDigit()) {
            ++(*index);
        }
    }
    bool ok = false;
    *d = _source.mid(start_index, *index - start_index).toDouble(&ok);
    return ok;
}

void QStringParser::Parser::evaluate()
{
    if (!_evaluated) {
        _evaluated = true;
        _evaluation_failed = false;

        int p = 0;
        int i = 0;
        while (p < _pattern.length()) {
            if (_pattern.at(p) == QLatin1Char('%')) {
                ++p;
                // a % must be followed by a another char.
                if (p >= _pattern.length()) {
                    // syntax error in pattern
                    _evaluation_failed = true;
                    return;
                }
                if (_pattern.at(p) == QLatin1Char('%')) {
                    // two %% are handled like a simple % in _source
                    ++p;
                    if (i >= _source.length() || _source.at(i) != QLatin1Char('%')) {
                        _evaluation_failed = true;
                        return;
                    }
                    ++i;
                } else if (_pattern.at(p).isDigit()) {
                    // now extract a value matching the Nth node type
                    int N = 0;
                    while (p < _pattern.length() && _pattern.at(p).isDigit()) {
                        N = N * 10 + _pattern.at(p).digitValue();
                        ++p;
                    }
                    if (N < 1 || N > _nodes.length()) {
                        // argument out of bounds in pattern
                        _evaluation_failed = true;
                        return;
                    }
                    if (!_nodes.at(N-1)->accept(*this, &i)) {
                        _evaluation_failed = true;
                        return;
                    }
                } else {
                    // any other % syntax is an error
                    _evaluation_failed = true;
                    return;
                }
            } else {
                if (_pattern.at(p).isSpace()) {
                    ++p;
                    // _source must end or have at least one space
                    if (i < _source.length() && !_source.at(i).isSpace()) {
                        _evaluation_failed = true;
                        return;
                    }
                    // skip spaces in _pattern
                    while (p < _pattern.length() && _pattern.at(p).isSpace()) {
                        ++p;
                    }
                    // skip spaces in _source
                    while (i < _source.length() && _source.at(i).isSpace()) {
                        ++i;
                    }
                } else if (i >= _source.length() || _source.at(i) != _pattern.at(p)) {
                    _evaluation_failed = true;
                    return;
                } else {
                    ++p;
                    ++i;
                }
            }
        }
        // _source and _pattern must both be scanned completely
        if (i < _source.length() || p < _pattern.length()) {
            _evaluation_failed = true;
            return;
        }
    }
}
