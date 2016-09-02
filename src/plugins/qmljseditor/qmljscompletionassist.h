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

#include "qmljseditor.h"

#include <texteditor/codeassist/assistproposalitem.h>
#include <texteditor/codeassist/genericproposalmodel.h>
#include <texteditor/codeassist/completionassistprovider.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/snippets/snippetassistcollector.h>
#include <texteditor/codeassist/assistinterface.h>


#include <QStringList>
#include <QScopedPointer>
#include <QVariant>
#include <QIcon>

namespace QmlJS { class Value; }

namespace QmlJSEditor {

class QmlJSCompletionAssistInterface;

namespace Internal {

class QmlJSAssistProposalItem final : public TextEditor::AssistProposalItem
{
public:
    bool prematurelyApplies(const QChar &c) const final;
    void applyContextualContent(TextEditor::TextDocumentManipulatorInterface &manipulator,
                                int basePosition) const final;
};


class QmlJSAssistProposalModel : public TextEditor::GenericProposalModel
{
public:
    QmlJSAssistProposalModel(const QList<TextEditor::AssistProposalItemInterface *> &items)
    {
        loadContent(items);
    }

    void filter(const QString &prefix) override;
    void sort(const QString &prefix) override;
    bool keepPerfectMatch(TextEditor::AssistReason reason) const override;
};


class QmlJSCompletionAssistProvider : public TextEditor::CompletionAssistProvider
{
    Q_OBJECT

public:
    bool supportsEditor(Core::Id editorId) const override;
    TextEditor::IAssistProcessor *createProcessor() const override;

    int activationCharSequenceLength() const override;
    bool isActivationCharSequence(const QString &sequence) const override;
    bool isContinuationChar(const QChar &c) const override;
};


class QmlJSCompletionAssistProcessor : public TextEditor::IAssistProcessor
{
public:
    QmlJSCompletionAssistProcessor();
    ~QmlJSCompletionAssistProcessor();

    TextEditor::IAssistProposal *perform(const TextEditor::AssistInterface *interface) override;

private:
    TextEditor::IAssistProposal *createContentProposal() const;
    TextEditor::IAssistProposal *createHintProposal(
            const QString &functionName, const QStringList &namedArguments,
            int optionalNamedArguments, bool isVariadic) const;

    bool acceptsIdleEditor() const;

    bool completeUrl(const QString &relativeBasePath, const QString &urlString);
    bool completeFileName(const QString &relativeBasePath,
                          const QString &fileName,
                          const QStringList &patterns = QStringList());

    int m_startPosition;
    QScopedPointer<const QmlJSCompletionAssistInterface> m_interface;
    QList<TextEditor::AssistProposalItemInterface *> m_completions;
    TextEditor::SnippetAssistCollector m_snippetCollector;
};

} // Internal

class QMLJSEDITOR_EXPORT QmlJSCompletionAssistInterface : public TextEditor::AssistInterface
{
public:
    QmlJSCompletionAssistInterface(QTextDocument *textDocument,
                                   int position,
                                   const QString &fileName,
                                   TextEditor::AssistReason reason,
                                   const QmlJSTools::SemanticInfo &info);
    const QmlJSTools::SemanticInfo &semanticInfo() const;
    const QIcon &fileNameIcon() const { return m_darkBlueIcon; }
    const QIcon &keywordIcon() const { return m_darkYellowIcon; }
    const QIcon &symbolIcon() const { return m_darkCyanIcon; }

private:
    QmlJSTools::SemanticInfo m_semanticInfo;
    QIcon m_darkBlueIcon;
    QIcon m_darkYellowIcon;
    QIcon m_darkCyanIcon;
};

QStringList QMLJSEDITOR_EXPORT qmlJSAutoComplete(QTextDocument *textDocument,
                                                 int position,
                                                 const QString &fileName,
                                                 TextEditor::AssistReason reason,
                                                 const QmlJSTools::SemanticInfo &info);

} // QmlJSEditor
