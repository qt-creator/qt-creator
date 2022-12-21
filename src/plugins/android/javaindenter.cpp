// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "javaindenter.h"
#include <texteditor/tabsettings.h>

#include <QTextDocument>

using namespace Android;
using namespace Android::Internal;

JavaIndenter::JavaIndenter(QTextDocument *doc)
    : TextEditor::TextIndenter(doc)
{}

JavaIndenter::~JavaIndenter() = default;

bool JavaIndenter::isElectricCharacter(const QChar &ch) const
{
    if (ch == QLatin1Char('{')
            || ch == QLatin1Char('}')) {
        return true;
    }
    return false;
}

void JavaIndenter::indentBlock(const QTextBlock &block,
                               const QChar &typedChar,
                               const TextEditor::TabSettings &tabSettings,
                               int /*cursorPositionInEditor*/)
{
    int indent = indentFor(block, tabSettings);
    if (typedChar == QLatin1Char('}'))
        indent -= tabSettings.m_indentSize;
    tabSettings.indentLine(block, qMax(0, indent));
}

int JavaIndenter::indentFor(const QTextBlock &block,
                            const TextEditor::TabSettings &tabSettings,
                            int /*cursorPositionInEditor*/)
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
