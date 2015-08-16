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

#include "stringtextsource.h"

#include "sourcepos.h"
#include "qmt/infrastructure/qmtassert.h"

namespace qmt {

StringTextSource::StringTextSource()
    : _source_id(-1),
      _index(-1),
      _line_number(-1),
      _column_number(-1)
{
}

StringTextSource::~StringTextSource()
{
}

void StringTextSource::setText(const QString &text)
{
    _text = text;
    _index = 0;
    _line_number = 1;
    _column_number = 1;
}

void StringTextSource::setSourceId(int source_id)
{
    _source_id = source_id;
}

SourceChar StringTextSource::readNextChar()
{
    QMT_CHECK(_source_id >= 0);
    QMT_CHECK(_index >= 0);
    QMT_CHECK(_line_number >= 0);
    QMT_CHECK(_column_number >= 0);

    if (_index >= _text.length()) {
        return SourceChar(QChar(), SourcePos(_source_id, _line_number, _column_number));
    }

    SourcePos pos(_source_id, _line_number, _column_number);
    QChar ch(_text.at(_index));
    ++_index;
    if (ch == QChar::LineFeed) {
        ++_line_number;
        _column_number = 1;
    } else {
        ++_column_number;
    }
    return SourceChar(ch, pos);
}

} // namespace qmt
