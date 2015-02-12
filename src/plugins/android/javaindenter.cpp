/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include "javaindenter.h"
#include <texteditor/tabsettings.h>

#include <QTextDocument>

using namespace Android;
using namespace Android::Internal;

JavaIndenter::JavaIndenter()
{
}

JavaIndenter::~JavaIndenter()
{}

bool JavaIndenter::isElectricCharacter(const QChar &ch) const
{
    if (ch == QLatin1Char('{')
            || ch == QLatin1Char('}')) {
        return true;
    }
    return false;
}

void JavaIndenter::indentBlock(QTextDocument *doc,
                                 const QTextBlock &block,
                                 const QChar &typedChar,
                                 const TextEditor::TabSettings &tabSettings)
{
    // At beginning: Leave as is.
    if (block == doc->begin())
        return;

    const int tabsize = tabSettings.m_indentSize;

    QTextBlock previous = block.previous();
    QString previousText = previous.text();
    while (previousText.trimmed().isEmpty()) {
        previous = previous.previous();
        if (previous == doc->begin())
            return;
        previousText = previous.text();
    }

    int adjust = 0;
    if (previousText.contains(QLatin1Char('{')))
        adjust = tabsize;

    if (block.text().contains(QLatin1Char('}')) || typedChar == QLatin1Char('}'))
        adjust += -tabsize;

    // Count the indentation of the previous line.
    int i = 0;
    while (i < previousText.size()) {
        if (!previousText.at(i).isSpace()) {
            tabSettings.indentLine(block, tabSettings.columnAt(previousText, i)
                                   + adjust);
            break;
        }
        ++i;
    }
}
