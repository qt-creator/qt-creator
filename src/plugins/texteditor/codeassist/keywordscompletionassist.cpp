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

#include "keywordscompletionassist.h"

#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/functionhintproposal.h>
#include <texteditor/codeassist/genericproposalmodel.h>
#include <texteditor/completionsettings.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditor.h>

#include <utils/algorithm.h>

namespace TextEditor {

// --------------------------
// Keywords
// --------------------------
// Note: variables and functions must be sorted
Keywords::Keywords(const QStringList &variables, const QStringList &functions, const QMap<QString, QStringList> &functionArgs)
    : m_variables(variables), m_functions(functions), m_functionArgs(functionArgs)
{
    Utils::sort(m_variables);
    Utils::sort(m_functions);
}

bool Keywords::isVariable(const QString &word) const
{
    return std::binary_search(m_variables.constBegin(), m_variables.constEnd(), word);
}

bool Keywords::isFunction(const QString &word) const
{
    return std::binary_search(m_functions.constBegin(), m_functions.constEnd(), word);
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

bool KeywordsAssistProposalItem::prematurelyApplies(const QChar &c) const
{
    // only '(' in case of a function
    return c == QLatin1Char('(') && m_isFunction;
}

void KeywordsAssistProposalItem::applyContextualContent(TextDocumentManipulatorInterface &manipulator,
                                                        int basePosition) const
{
    const CompletionSettings &settings = TextEditorSettings::completionSettings();

    int replaceLength = manipulator.currentPosition() - basePosition;
    QString toInsert = text();
    int cursorOffset = 0;
    const QChar characterAtCurrentPosition = manipulator.characterAt(manipulator.currentPosition());
    bool setAutoCompleteSkipPosition = false;

    if (m_isFunction && settings.m_autoInsertBrackets) {
        if (settings.m_spaceAfterFunctionName) {
            if (manipulator.textAt(manipulator.currentPosition(), 2) == QLatin1String(" (")) {
                cursorOffset = 2;
            } else if ( characterAtCurrentPosition == QLatin1Char('(')
                       || characterAtCurrentPosition == QLatin1Char(' ')) {
                replaceLength += 1;
                toInsert += QLatin1String(" (");
            } else {
                toInsert += QLatin1String(" ()");
                cursorOffset = -1;
                setAutoCompleteSkipPosition = true;
            }
        } else {
            if (characterAtCurrentPosition == QLatin1Char('(')) {
                cursorOffset = 1;
            } else {
                toInsert += QLatin1String("()");
                cursorOffset = -1;
                setAutoCompleteSkipPosition = true;
            }
        }
    }

    manipulator.replace(basePosition, replaceLength, toInsert);
    if (cursorOffset)
        manipulator.setCursorPosition(manipulator.currentPosition() + cursorOffset);
    if (setAutoCompleteSkipPosition)
        manipulator.setAutoCompleteSkipPosition(manipulator.currentPosition());
}

// -------------------------
// KeywordsFunctionHintModel
// -------------------------
KeywordsFunctionHintModel::KeywordsFunctionHintModel(const QStringList &functionSymbols)
        : m_functionSymbols(functionSymbols)
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
    : m_snippetCollector(QString(), QIcon(":/texteditor/images/snippet.png"))
    , m_variableIcon(QLatin1String(":/codemodel/images/keyword.png"))
    , m_functionIcon(QLatin1String(":/codemodel/images/member.png"))
    , m_keywords(keywords)
{}

IAssistProposal *KeywordsCompletionAssistProcessor::perform(const AssistInterface *interface)
{
    QScopedPointer<const AssistInterface> assistInterface(interface);
    if (isInComment(interface))
        return nullptr;

    int pos = interface->position();

    // Find start position
    QChar chr = interface->characterAt(pos - 1);
    if (chr == '(')
        --pos;
    // Skip to the start of a name
    do {
        chr = interface->characterAt(--pos);
    } while (chr.isLetterOrNumber() || chr == '_');

    ++pos;

    int startPosition = pos;

    if (interface->reason() == IdleEditor) {
        QChar characterUnderCursor = interface->characterAt(interface->position());
        if (characterUnderCursor.isLetterOrNumber())
            return nullptr;
        if (interface->position() - startPosition < 3)
            return 0;
    }

    // extract word
    QString word;
    do {
        word += interface->characterAt(pos);
        chr = interface->characterAt(++pos);
    } while ((chr.isLetterOrNumber() || chr == '_') && chr != '(');

    if (m_keywords.isFunction(word) && interface->characterAt(pos) == '(') {
        QStringList functionSymbols = m_keywords.argsForFunction(word);
        if (functionSymbols.size() == 0)
            return nullptr;
        IFunctionHintProposalModel *model = new KeywordsFunctionHintModel(functionSymbols);
        return new FunctionHintProposal(startPosition, model);
    } else {
        QList<AssistProposalItemInterface *> items = m_snippetCollector.collect();
        items.append(generateProposalList(m_keywords.variables(), m_variableIcon));
        items.append(generateProposalList(m_keywords.variables(), m_variableIcon));
        return new GenericProposal(startPosition, items);
    }
}

void KeywordsCompletionAssistProcessor::setSnippetGroup(const QString &id)
{
    m_snippetCollector.setGroupId(id);
}

void KeywordsCompletionAssistProcessor::setKeywords(Keywords keywords)
{
    m_keywords = keywords;
}

bool KeywordsCompletionAssistProcessor::isInComment(const AssistInterface *interface) const
{
    QTextCursor tc(interface->textDocument());
    tc.setPosition(interface->position());
    tc.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
    return tc.selectedText().contains('#');
}

QList<AssistProposalItemInterface *>
KeywordsCompletionAssistProcessor::generateProposalList(const QStringList &words, const QIcon &icon)
{
    return Utils::transform(words, [this, &icon](const QString &word) -> AssistProposalItemInterface * {
        AssistProposalItem *item = new KeywordsAssistProposalItem(m_keywords.isFunction(word));
        item->setText(word);
        item->setIcon(icon);
        return item;
    });
}

KeywordsCompletionAssistProvider::KeywordsCompletionAssistProvider(const Keywords &keyWords,
                                                                   const QString &snippetGroup)
    : m_keyWords(keyWords)
    , m_snippetGroup(snippetGroup)
{ }

IAssistProvider::RunType KeywordsCompletionAssistProvider::runType() const
{
    return Synchronous;
}

IAssistProcessor *KeywordsCompletionAssistProvider::createProcessor() const
{
    auto processor = new KeywordsCompletionAssistProcessor(m_keyWords);
    processor->setSnippetGroup(m_snippetGroup);
    return processor;
}

} // namespace TextEditor
