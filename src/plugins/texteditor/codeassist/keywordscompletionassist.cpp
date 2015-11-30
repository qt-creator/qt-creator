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

#include "keywordscompletionassist.h"

#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/functionhintproposal.h>
#include <texteditor/codeassist/genericproposalmodel.h>
#include <texteditor/completionsettings.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/texteditor.h>

namespace TextEditor {

// --------------------------
// Keywords
// --------------------------
Keywords::Keywords()
{
}

// Note: variables and functions must be sorted
Keywords::Keywords(const QStringList &variabels, const QStringList &functions, const QMap<QString, QStringList> &functionArgs)
    : m_variables(variabels), m_functions(functions), m_functionArgs(functionArgs)
{

}

bool Keywords::isVariable(const QString &word) const
{
    return qBinaryFind(m_variables, word) != m_variables.constEnd();
}

bool Keywords::isFunction(const QString &word) const
{
    return qBinaryFind(m_functions, word) != m_functions.constEnd();
}

QStringList Keywords::variables() const
{
    return m_variables;
}

QStringList Keywords::functions() const
{
    return m_functions;
}

QStringList Keywords::argsForFunction(const QString &function) const
{
    return m_functionArgs.value(function);
}


// --------------------------
// KeywordsAssistProposalItem
// --------------------------
KeywordsAssistProposalItem::KeywordsAssistProposalItem(bool isFunction)
    : m_isFunction(isFunction)
{
}

KeywordsAssistProposalItem::~KeywordsAssistProposalItem()
{}

bool KeywordsAssistProposalItem::prematurelyApplies(const QChar &c) const
{
    // only '(' in case of a function
    return c == QLatin1Char('(') && m_isFunction;
}

void KeywordsAssistProposalItem::applyContextualContent(TextEditorWidget *editorWidget,
                                                        int basePosition) const
{
    const CompletionSettings &settings = TextEditorSettings::completionSettings();

    int replaceLength = editorWidget->position() - basePosition;
    QString toInsert = text();
    int cursorOffset = 0;
    if (m_isFunction && settings.m_autoInsertBrackets) {
        if (settings.m_spaceAfterFunctionName) {
            if (editorWidget->textAt(editorWidget->position(), 2) == QLatin1String(" (")) {
                cursorOffset = 2;
            } else if (editorWidget->characterAt(editorWidget->position()) == QLatin1Char('(')
                       || editorWidget->characterAt(editorWidget->position()) == QLatin1Char(' ')) {
                replaceLength += 1;
                toInsert += QLatin1String(" (");
            } else {
                toInsert += QLatin1String(" ()");
                cursorOffset = -1;
            }
        } else {
            if (editorWidget->characterAt(editorWidget->position()) == QLatin1Char('(')) {
                cursorOffset = 1;
            } else {
                toInsert += QLatin1String("()");
                cursorOffset = -1;
            }
        }
    }

    editorWidget->setCursorPosition(basePosition);
    editorWidget->replace(replaceLength, toInsert);
    if (cursorOffset)
        editorWidget->setCursorPosition(editorWidget->position() + cursorOffset);
}

// -------------------------
// KeywordsFunctionHintModel
// -------------------------
KeywordsFunctionHintModel::KeywordsFunctionHintModel(const QStringList &functionSymbols)
        : m_functionSymbols(functionSymbols)
{}

KeywordsFunctionHintModel::~KeywordsFunctionHintModel()
{}

void KeywordsFunctionHintModel::reset()
{}

int KeywordsFunctionHintModel::size() const
{
    return m_functionSymbols.size();
}

QString KeywordsFunctionHintModel::text(int index) const
{
    return m_functionSymbols.at(index);
}

int KeywordsFunctionHintModel::activeArgument(const QString &prefix) const
{
    Q_UNUSED(prefix);
    return 1;
}

// ---------------------------------
// KeywordsCompletionAssistProcessor
// ---------------------------------
KeywordsCompletionAssistProcessor::KeywordsCompletionAssistProcessor(Keywords keywords)
    : m_startPosition(-1)
    , m_variableIcon(QLatin1String(":/codemodel/images/keyword.png"))
    , m_functionIcon(QLatin1String(":/codemodel/images/func.png"))
    , m_keywords(keywords)
{}

KeywordsCompletionAssistProcessor::~KeywordsCompletionAssistProcessor()
{}

IAssistProposal *KeywordsCompletionAssistProcessor::perform(const AssistInterface *interface)
{
    m_interface.reset(interface);

    if (isInComment())
        return 0;

    if (interface->reason() == IdleEditor && !acceptsIdleEditor())
        return 0;

    if (m_startPosition == -1)
        m_startPosition = findStartOfName();

    int nextCharPos = m_startPosition + m_word.length();
    if (m_keywords.isFunction(m_word)
        && m_interface->characterAt(nextCharPos) == QLatin1Char('(')) {
        QStringList functionSymbols = m_keywords.argsForFunction(m_word);
        IFunctionHintProposalModel *model =
                new KeywordsFunctionHintModel(functionSymbols);
        IAssistProposal *proposal = new FunctionHintProposal(m_startPosition, model);
        return proposal;
    } else {
        QList<AssistProposalItem *> items;
        addWordsToProposalList(&items, m_keywords.variables(), m_variableIcon);
        addWordsToProposalList(&items, m_keywords.functions(), m_functionIcon);
        return new GenericProposal(m_startPosition, items);
    }
}

QChar KeywordsCompletionAssistProcessor::startOfCommentChar() const
{
    return QLatin1Char('#');
}

void KeywordsCompletionAssistProcessor::setKeywords(Keywords keywords)
{
    m_keywords = keywords;
}

bool KeywordsCompletionAssistProcessor::acceptsIdleEditor()
{
    const int pos = m_interface->position();
    QChar characterUnderCursor = m_interface->characterAt(pos);
    if (!characterUnderCursor.isLetterOrNumber()) {
        m_startPosition = findStartOfName();
        if (pos - m_startPosition >= 3 && !isInComment())
            return true;
    }
    return false;
}

int KeywordsCompletionAssistProcessor::findStartOfName(int pos)
{
    if (pos == -1)
        pos = m_interface->position();

    QChar chr = m_interface->characterAt(pos-1);
    if (chr == QLatin1Char('('))
        --pos;
    // Skip to the start of a name
    do {
        chr = m_interface->characterAt(--pos);
    } while (chr.isLetterOrNumber() || chr == QLatin1Char('_'));

    int start = ++pos;
    m_word.clear();
    do {
        m_word += m_interface->characterAt(pos);
        chr = m_interface->characterAt(++pos);
    } while ((chr.isLetterOrNumber() || chr == QLatin1Char('_'))
             && chr != QLatin1Char('('));

    return start;
}

bool KeywordsCompletionAssistProcessor::isInComment() const
{
    QTextCursor tc(m_interface->textDocument());
    tc.setPosition(m_interface->position());
    tc.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
    const QString &lineBeginning = tc.selectedText();
    if (lineBeginning.contains(startOfCommentChar()))
        return true;
    return false;
}

void KeywordsCompletionAssistProcessor::addWordsToProposalList(QList<AssistProposalItem *> *items, const QStringList &words, const QIcon &icon)
{
    if (!items)
        return;

    for (int i = 0; i < words.count(); ++i) {
        AssistProposalItem *item = new KeywordsAssistProposalItem(m_keywords.isFunction(words.at(i)));
        item->setText(words.at(i));
        item->setIcon(icon);
        items->append(item);
    }
}

} // namespace TextEditor
