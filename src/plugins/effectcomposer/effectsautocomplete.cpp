// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectsautocomplete.h"

#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/texteditorsettings.h>

#include <utils/qtcassert.h>

#include <qmljseditor/qmljscompletionassist.h>

namespace {

enum CompletionOrder {
    EnumValueOrder = -5,
    SnippetOrder = -15,
    PropertyOrder = -10,
    SymbolOrder = -20,
    KeywordOrder = -25,
    TypeOrder = -30
};

/*!
  Returns a number defining how well \a searchStr matches \a str.

  Quite simplistic, looks only at the first match, and prefers contiguous
  matches, or matches to capitalized or separated words.
  Match to the last character is also preferred.
*/
int matchStrength(const QString &searchStr, const QString &str)
{
    QString::const_iterator i = searchStr.constBegin(), iEnd = searchStr.constEnd(),
                            j = str.constBegin(), jEnd = str.constEnd();
    bool lastWasNotUpper = true, lastWasSpacer = true, lastWasMatch = false, didJump = false;
    int res = 0;
    while (i != iEnd && j != jEnd) {
        bool thisIsUpper = (*j).isUpper();
        bool thisIsLetterOrNumber = (*j).isLetterOrNumber();
        if ((*i).toLower() == (*j).toLower()) {
            if (lastWasMatch || (lastWasNotUpper && thisIsUpper) || (thisIsUpper && (*i).isUpper())
                || (lastWasSpacer && thisIsLetterOrNumber))
                ++res;
            lastWasMatch = true;
            ++i;
        } else {
            didJump = true;
            lastWasMatch = false;
        }
        ++j;
        lastWasNotUpper = !thisIsUpper;
        lastWasSpacer = !thisIsLetterOrNumber;
    }
    if (i != iEnd)
        return i - iEnd;
    if (j == jEnd)
        ++res;
    if (!didJump)
        res += 2;
    return res;
}

bool isIdentifierChar(const QChar &c, bool atStart, bool acceptDollar)
{
    switch (c.unicode()) {
    case '_':
        return true;
    case '$':
        if (acceptDollar)
            return true;
        return false;

    default:
        if (atStart)
            return c.isLetter();
        else
            return c.isLetterOrNumber();
    }
}

class CompleteFunctionCall
{
public:
    CompleteFunctionCall(bool hasArguments = true)
        : hasArguments(hasArguments)
    {}
    bool hasArguments;
};

class QmlJSLessThan
{
    using AssistProposalItemInterface = TextEditor::AssistProposalItemInterface;

public:
    QmlJSLessThan(const QString &searchString)
        : m_searchString(searchString)
    {}
    bool operator()(const AssistProposalItemInterface *a, const AssistProposalItemInterface *b)
    {
        if (a->order() != b->order())
            return a->order() > b->order();
        else if (a->text().isEmpty() && !b->text().isEmpty())
            return true;
        else if (b->text().isEmpty())
            return false;
        else if (a->text().at(0).isUpper() && b->text().at(0).isLower())
            return false;
        else if (a->text().at(0).isLower() && b->text().at(0).isUpper())
            return true;
        int m1 = ::matchStrength(m_searchString, a->text());
        int m2 = ::matchStrength(m_searchString, b->text());
        if (m1 != m2)
            return m1 > m2;
        return a->text() < b->text();
    }

private:
    QString m_searchString;
};

} // namespace

namespace EffectComposer {

class EffectsCodeAssistProposalItem final : public TextEditor::AssistProposalItem
{
    using TextEditorSettings = TextEditor::TextEditorSettings;

public:
    bool prematurelyApplies(const QChar &c) const final
    {
        if (data().canConvert<QString>()) // snippet
            return false;

        return (text().endsWith(QLatin1String(": ")) && c == QLatin1Char(':'))
               || (text().endsWith(QLatin1Char('.')) && c == QLatin1Char('.'));
    }

    void applyContextualContent(
        TextEditor::TextDocumentManipulatorInterface &manipulator, int basePosition) const final
    {
        const int currentPosition = manipulator.currentPosition();
        manipulator.replace(basePosition, currentPosition - basePosition, QString());

        QString content = text();
        int cursorOffset = 0;

        const bool autoInsertBrackets
            = TextEditorSettings::completionSettings().m_autoInsertBrackets;

        if (autoInsertBrackets && data().canConvert<CompleteFunctionCall>()) {
            CompleteFunctionCall function = data().value<CompleteFunctionCall>();
            content += QLatin1String("()");
            if (function.hasArguments)
                cursorOffset = -1;
        }

        QString replaceable = content;
        int replacedLength = 0;
        for (int i = 0; i < replaceable.length(); ++i) {
            const QChar a = replaceable.at(i);
            const QChar b = manipulator.characterAt(manipulator.currentPosition() + i);
            if (a == b)
                ++replacedLength;
            else
                break;
        }
        const int length = manipulator.currentPosition() - basePosition + replacedLength;
        manipulator.replace(basePosition, length, content);
        if (cursorOffset) {
            manipulator.setCursorPosition(manipulator.currentPosition() + cursorOffset);
            manipulator.setAutoCompleteSkipPosition(manipulator.currentPosition());
        }
    }
};

class EffectsAssistProposalModel : public TextEditor::GenericProposalModel
{
    using AssistProposalItemInterface = TextEditor::AssistProposalItemInterface;
    using AssistReason = TextEditor::AssistReason;

public:
    EffectsAssistProposalModel(const QList<TextEditor::AssistProposalItemInterface *> &items)
    {
        loadContent(items);
    }

    void filter(const QString &prefix) override;
    void sort(const QString &prefix) override;
    bool keepPerfectMatch(TextEditor::AssistReason reason) const override;
};

void EffectsAssistProposalModel::filter(const QString &prefix)
{
    GenericProposalModel::filter(prefix);
    if (prefix.startsWith(QLatin1String("__")))
        return;
    QList<AssistProposalItemInterface *> newCurrentItems;
    newCurrentItems.reserve(m_currentItems.size());
    for (AssistProposalItemInterface *item : std::as_const(m_currentItems)) {
        if (!item->text().startsWith(QLatin1String("__")))
            newCurrentItems << item;
    }
    m_currentItems = newCurrentItems;
}

void EffectsAssistProposalModel::sort(const QString &prefix)
{
    std::sort(m_currentItems.begin(), m_currentItems.end(), QmlJSLessThan(prefix));
}

bool EffectsAssistProposalModel::keepPerfectMatch(AssistReason reason) const
{
    return reason == TextEditor::ExplicitlyInvoked;
}

void addCompletion(
    QList<TextEditor::AssistProposalItemInterface *> *completions,
    const QString &text,
    const QIcon &icon,
    int order,
    const QVariant &data = QVariant())
{
    if (text.isEmpty())
        return;

    TextEditor::AssistProposalItem *item = new EffectsCodeAssistProposalItem;
    item->setText(text);
    item->setIcon(icon);
    item->setOrder(order);
    item->setData(data);
    completions->append(item);
}

void addCompletions(
    QList<TextEditor::AssistProposalItemInterface *> *completions,
    const QStringList &newCompletions,
    const QIcon &icon,
    int order)
{
    for (const QString &text : newCompletions)
        addCompletion(completions, text, icon, order);
}

EffectsCompletionAssistProcessor::EffectsCompletionAssistProcessor()
    : m_startPosition(0)
{}

TextEditor::IAssistProposal *EffectsCompletionAssistProcessor::performAsync()
{
    using QmlJSEditor::QmlJSCompletionAssistInterface;

    auto completionInterface = static_cast<const EffectsCompletionAssistInterface *>(interface());
    QTC_ASSERT(completionInterface, return {});

    m_startPosition = completionInterface->position();
    QTextDocument *textDocument = completionInterface->textDocument();

    while (isIdentifierChar(textDocument->characterAt(m_startPosition - 1), false, false))
        --m_startPosition;

    m_completions.clear();

    // The completionOperator is the character under the cursor or directly before the
    // identifier under cursor. Use in conjunction with onIdentifier. Examples:
    // a + b<complete> -> ' '
    // a +<complete> -> '+'
    // a +b<complete> -> '+'
    QChar completionOperator;
    if (m_startPosition > 0)
        completionOperator = textDocument->characterAt(m_startPosition - 1);

    if (completionOperator != QLatin1Char('.')) {
        addCompletions(
            &m_completions,
            completionInterface->uniformNames(),
            QmlJSCompletionAssistInterface::keywordIcon(),
            KeywordOrder);
    }

    if (!m_completions.isEmpty()) {
        TextEditor::GenericProposalModelPtr model(new EffectsAssistProposalModel(m_completions));
        return new TextEditor::GenericProposal(m_startPosition, model);
    }
    return nullptr;
}

TextEditor::IAssistProcessor *EffectsCompeletionAssistProvider::createProcessor(
    const TextEditor::AssistInterface *) const
{
    return new EffectsCompletionAssistProcessor;
}

} // namespace EffectComposer
