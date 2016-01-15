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
