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

#include "syntaxhighlighter.h"
#include "formats.h"

#include <texteditor/generichighlighter/highlighter.h>

using namespace TextEditor;
using namespace Internal;

QTextCharFormat SyntaxHighlighter::formatForCategory(int categoryIndex) const
{
    switch (categoryIndex) {
    case Highlighter::Keyword:      return Formats::instance().keywordFormat();
    case Highlighter::DataType:     return Formats::instance().dataTypeFormat();
    case Highlighter::Decimal:      return Formats::instance().decimalFormat();
    case Highlighter::BaseN:        return Formats::instance().baseNFormat();
    case Highlighter::Float:        return Formats::instance().floatFormat();
    case Highlighter::Char:         return Formats::instance().charFormat();
    case Highlighter::String:       return Formats::instance().stringFormat();
    case Highlighter::Comment:      return Formats::instance().commentFormat();
    case Highlighter::Alert:        return Formats::instance().alertFormat();
    case Highlighter::Error:        return Formats::instance().errorFormat();
    case Highlighter::Function:     return Formats::instance().functionFormat();
    case Highlighter::RegionMarker: return Formats::instance().regionMarketFormat();
    case Highlighter::Others:       return Formats::instance().othersFormat();
    default:                                  return QTextCharFormat();
    }
}

