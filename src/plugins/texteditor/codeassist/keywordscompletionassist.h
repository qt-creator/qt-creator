// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "assistproposalitem.h"
#include "asyncprocessor.h"
#include "completionassistprovider.h"
#include "ifunctionhintproposalmodel.h"
#include "../snippets/snippetassistcollector.h"

#include "texteditor/texteditorconstants.h"

namespace TextEditor {

class AssistInterface;

class TEXTEDITOR_EXPORT Keywords
{
public:
    Keywords() = default;
    Keywords(const QStringList &variables, const QStringList &functions = QStringList(),
             const QMap<QString, QStringList> &functionArgs = QMap<QString, QStringList>());
    bool isVariable(const QString &word) const;
    bool isFunction(const QString &word) const;

    QStringList variables() const;
    QStringList functions() const;
    QStringList argsForFunction(const QString &function) const;

private:
    QStringList m_variables;
    QStringList m_functions;
    QMap<QString, QStringList> m_functionArgs;
};

class TEXTEDITOR_EXPORT KeywordsAssistProposalItem : public AssistProposalItem
{
public:
    KeywordsAssistProposalItem(bool isFunction);

    bool prematurelyApplies(const QChar &c) const final;
    void applyContextualContent(TextDocumentManipulatorInterface &manipulator, int basePosition) const final;
private:
    bool m_isFunction;
};

class TEXTEDITOR_EXPORT KeywordsFunctionHintModel final : public IFunctionHintProposalModel
{
public:
    KeywordsFunctionHintModel(const QStringList &functionSymbols);
    ~KeywordsFunctionHintModel() final = default;

    void reset() final;
    int size() const final;
    QString text(int index) const final;
    int activeArgument(const QString &prefix) const final;

private:
    QStringList m_functionSymbols;
};

using DynamicCompletionFunction
    = std::function<void (const AssistInterface *, QList<AssistProposalItemInterface *> *, int &)>;

class TEXTEDITOR_EXPORT KeywordsCompletionAssistProvider : public CompletionAssistProvider
{
public:
    KeywordsCompletionAssistProvider(const Keywords &keyWords = Keywords(),
            const QString &snippetGroup = QString(Constants::TEXT_SNIPPET_GROUP_ID));

    void setDynamicCompletionFunction(const DynamicCompletionFunction &func);

    IAssistProcessor *createProcessor(const AssistInterface *) const override;

private:
    Keywords m_keyWords;
    QString m_snippetGroup;
    DynamicCompletionFunction m_completionFunc;
};

class TEXTEDITOR_EXPORT KeywordsCompletionAssistProcessor : public AsyncProcessor
{
public:
    KeywordsCompletionAssistProcessor(const Keywords &keywords);
    ~KeywordsCompletionAssistProcessor() override = default;

    IAssistProposal *performAsync() override;

    void setSnippetGroup(const QString &id);

    void setDynamicCompletionFunction(DynamicCompletionFunction func);

protected:
    void setKeywords (const Keywords &keywords);

private:
    bool isInComment(const AssistInterface *interface) const;
    QList<AssistProposalItemInterface *> generateProposalList(const QStringList &words, const QIcon &icon);

    TextEditor::SnippetAssistCollector m_snippetCollector;
    const QIcon m_variableIcon;
    const QIcon m_functionIcon;
    Keywords m_keywords;
    DynamicCompletionFunction m_dynamicCompletionFunction;
};

TEXTEDITOR_EXPORT void pathComplete(const AssistInterface *interface,
                                    QList<AssistProposalItemInterface *> *items,
                                    int &startPosition);

} // TextEditor
