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

#ifndef KEYWORDSCOMPLETIONASSIST_H
#define KEYWORDSCOMPLETIONASSIST_H

#include "iassistprocessor.h"
#include "assistproposalitem.h"
#include "ifunctionhintproposalmodel.h"


namespace TextEditor {

class TEXTEDITOR_EXPORT Keywords
{
public:
    Keywords();
    Keywords(const QStringList &variabels, const QStringList &functions, const QMap<QString, QStringList> &functionArgs);
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
    ~KeywordsAssistProposalItem() Q_DECL_NOEXCEPT;

    bool prematurelyApplies(const QChar &c) const override;
    void applyContextualContent(TextDocumentManipulatorInterface &manipulator, int basePosition) const override;
private:
    bool m_isFunction;
};

class TEXTEDITOR_EXPORT KeywordsFunctionHintModel : public IFunctionHintProposalModel
{
public:
    KeywordsFunctionHintModel(const QStringList &functionSymbols);
    ~KeywordsFunctionHintModel();

    void reset() override;
    int size() const override;
    QString text(int index) const override;
    int activeArgument(const QString &prefix) const override;

private:
    QStringList m_functionSymbols;
};

class TEXTEDITOR_EXPORT KeywordsCompletionAssistProcessor : public IAssistProcessor
{
public:
    KeywordsCompletionAssistProcessor(Keywords keywords);
    ~KeywordsCompletionAssistProcessor();

    IAssistProposal *perform(const AssistInterface *interface) override;
    QChar startOfCommentChar() const;

protected:
    void setKeywords (Keywords keywords);

private:
    bool acceptsIdleEditor();
    int findStartOfName(int pos = -1);
    bool isInComment() const;
    void addWordsToProposalList(QList<AssistProposalItemInterface *> *items, const QStringList &words, const QIcon &icon);

    int m_startPosition;
    QString m_word;
    QScopedPointer<const AssistInterface> m_interface;
    const QIcon m_variableIcon;
    const QIcon m_functionIcon;
    Keywords m_keywords;
};

} // TextEditor

#endif // KEYWORDSCOMPLETIONASSISTPROCESSOR_H
