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
    Q_UNUSED(doc);
    int indent = indentFor(block, tabSettings);
    if (typedChar == QLatin1Char('}'))
        indent -= tabSettings.m_indentSize;
    tabSettings.indentLine(block, qMax(0, indent));
}

int JavaIndenter::indentFor(const QTextBlock &block,
                            const TextEditor::TabSettings &tabSettings)
{
    QTextBlock previous = block.previous();
    if (!previous.isValid())
        return 0;

    QString previousText = previous.text();
    while (previousText.trimmed().isEmpty()) {
        previous = previous.previous();
        if (!previous.isValid())
            return 0;
        previousText = previous.text();
    }

    int indent = tabSettings.indentationColumn(previousText);

    int adjust = previousText.count(QLatin1Char('{')) - previousText.count(QLatin1Char('}'));
    adjust *= tabSettings.m_indentSize;

    return qMax(0, indent + adjust);
}
