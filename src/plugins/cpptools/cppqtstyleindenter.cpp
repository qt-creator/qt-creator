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

#include "cppqtstyleindenter.h"

#include "cppcodeformatter.h"
#include "cpptoolssettings.h"
#include "cppcodestylepreferences.h"

#include <QChar>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextCursor>

using namespace CppTools;

CppQtStyleIndenter::CppQtStyleIndenter()
    : m_cppCodeStylePreferences(0)
{
    // Just for safety. setCodeStylePreferences should be called when the editor the
    // indenter belongs to gets initialized.
    m_cppCodeStylePreferences = CppToolsSettings::instance()->cppCodeStyle();
}

CppQtStyleIndenter::~CppQtStyleIndenter()
{}

bool CppQtStyleIndenter::isElectricCharacter(const QChar &ch) const
{
    switch (ch.toLatin1()) {
    case '{':
    case '}':
    case ':':
    case '#':
    case '<':
    case '>':
    case ';':
        return true;
    }
    return false;
}

static bool isElectricInLine(const QChar ch, const QString &text)
{
    switch (ch.toLatin1()) {
    case ';':
        return text.contains(QLatin1String("break"));
    case ':':
        // switch cases and access declarations should be reindented
        if (text.contains(QLatin1String("case"))
                || text.contains(QLatin1String("default"))
                || text.contains(QLatin1String("public"))
                || text.contains(QLatin1String("private"))
                || text.contains(QLatin1String("protected"))
                || text.contains(QLatin1String("signals"))
                || text.contains(QLatin1String("Q_SIGNALS"))) {
            return true;
        }

        // fall-through
        // lines that start with : might have a constructor initializer list
    case '<':
    case '>': {
        // Electric if at line beginning (after space indentation)
        for (int i = 0, len = text.count(); i < len; ++i) {
            if (!text.at(i).isSpace())
                return text.at(i) == ch;
        }
        return false;
    }
    }

    return true;
}

void CppQtStyleIndenter::indentBlock(QTextDocument *doc,
                                     const QTextBlock &block,
                                     const QChar &typedChar,
                                     const TextEditor::TabSettings &tabSettings)
{
    Q_UNUSED(doc)

    QtStyleCodeFormatter codeFormatter(tabSettings, codeStyleSettings());

    codeFormatter.updateStateUntil(block);
    int indent;
    int padding;
    codeFormatter.indentFor(block, &indent, &padding);

    if (isElectricCharacter(typedChar)) {
        // : should not be electric for labels
        if (!isElectricInLine(typedChar, block.text()))
            return;

        // only reindent the current line when typing electric characters if the
        // indent is the same it would be if the line were empty
        int newlineIndent;
        int newlinePadding;
        codeFormatter.indentForNewLineAfter(block.previous(), &newlineIndent, &newlinePadding);
        if (tabSettings.indentationColumn(block.text()) != newlineIndent + newlinePadding)
            return;
    }

    tabSettings.indentLine(block, indent + padding, padding);
}

void CppQtStyleIndenter::indent(QTextDocument *doc,
                                const QTextCursor &cursor,
                                const QChar &typedChar,
                                const TextEditor::TabSettings &tabSettings)
{
    if (cursor.hasSelection()) {
        QTextBlock block = doc->findBlock(cursor.selectionStart());
        const QTextBlock end = doc->findBlock(cursor.selectionEnd()).next();

        QtStyleCodeFormatter codeFormatter(tabSettings, codeStyleSettings());
        codeFormatter.updateStateUntil(block);

        QTextCursor tc = cursor;
        tc.beginEditBlock();
        do {
            int indent;
            int padding;
            codeFormatter.indentFor(block, &indent, &padding);
            tabSettings.indentLine(block, indent + padding, padding);
            codeFormatter.updateLineStateChange(block);
            block = block.next();
        } while (block.isValid() && block != end);
        tc.endEditBlock();
    } else {
        indentBlock(doc, cursor.block(), typedChar, tabSettings);
    }
}

void CppQtStyleIndenter::setCodeStylePreferences(TextEditor::ICodeStylePreferences *preferences)
{
    CppCodeStylePreferences *cppCodeStylePreferences
            = qobject_cast<CppCodeStylePreferences *>(preferences);
    if (cppCodeStylePreferences)
        m_cppCodeStylePreferences = cppCodeStylePreferences;
}

void CppQtStyleIndenter::invalidateCache(QTextDocument *doc)
{
    QtStyleCodeFormatter formatter;
    formatter.invalidateCache(doc);
}

CppCodeStyleSettings CppQtStyleIndenter::codeStyleSettings() const
{
    if (m_cppCodeStylePreferences)
        return m_cppCodeStylePreferences->currentCodeStyleSettings();
    return CppCodeStyleSettings();
}
