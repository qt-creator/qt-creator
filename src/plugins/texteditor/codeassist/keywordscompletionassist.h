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

#pragma once

#include "iassistprocessor.h"
#include "assistproposalitem.h"
#include "ifunctionhintproposalmodel.h"
#include "../snippets/snippetassistcollector.h"

namespace TextEditor {

class AssistInterface;

class TEXTEDITOR_EXPORT Keywords
{
public:
    Keywords() = default;
    Keywords(const QStringList &variables, const QStringList &functions,
             const QMap<QString, QStringList> &functionArgs);
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

class TEXTEDITOR_EXPORT KeywordsFunctionHintModel : public IFunctionHintProposalModel
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

class TEXTEDITOR_EXPORT KeywordsCompletionAssistProcessor : public IAssistProcessor
{
public:
    KeywordsCompletionAssistProcessor(Keywords keywords);
    ~KeywordsCompletionAssistProcessor() override = default;

    IAssistProposal *perform(const AssistInterface *interface) override;
    QChar startOfCommentChar() const;

    void setSnippetGroup(const QString &id);

protected:
    void setKeywords (Keywords keywords);

private:
    bool acceptsIdleEditor();
    int findStartOfName(int pos = -1);
    bool isInComment() const;
    QList<AssistProposalItemInterface *> generateProposalList(const QStringList &words, const QIcon &icon);

    int m_startPosition = -1;
    TextEditor::SnippetAssistCollector m_snippetCollector;
    QString m_word;
    QScopedPointer<const AssistInterface> m_interface;
    const QIcon m_variableIcon;
    const QIcon m_functionIcon;
    Keywords m_keywords;
};

} // TextEditor
