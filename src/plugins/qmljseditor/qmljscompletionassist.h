// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljseditor_global.h"

#include <qmljstools/qmljssemanticinfo.h>
#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/assistproposalitem.h>
#include <texteditor/codeassist/asyncprocessor.h>
#include <texteditor/codeassist/completionassistprovider.h>
#include <texteditor/codeassist/genericproposalmodel.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/snippets/snippetassistcollector.h>

#include <QStringList>
#include <QScopedPointer>
#include <QVariant>
#include <QIcon>

namespace QmlJS { class Value; }

namespace QmlJSEditor {

class QmlJSCompletionAssistInterface;
class QmlJSCompletionAssistProvider;

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


class QmlJSCompletionAssistProcessor : public TextEditor::AsyncProcessor
{
public:
    QmlJSCompletionAssistProcessor();
    ~QmlJSCompletionAssistProcessor() override;

    TextEditor::IAssistProposal *performAsync() override;

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
    QList<TextEditor::AssistProposalItemInterface *> m_completions;
    TextEditor::SnippetAssistCollector m_snippetCollector;
};

} // Internal

class QMLJSEDITOR_EXPORT QmlJSCompletionAssistInterface : public TextEditor::AssistInterface
{
public:
    QmlJSCompletionAssistInterface(const QTextCursor &cursor,
                                   const Utils::FilePath &fileName,
                                   TextEditor::AssistReason reason,
                                   const QmlJSTools::SemanticInfo &info);
    const QmlJSTools::SemanticInfo &semanticInfo() const;
    static const QIcon &fileNameIcon();
    static const QIcon &keywordIcon();
    static const QIcon &symbolIcon();

private:
    QmlJSTools::SemanticInfo m_semanticInfo;
};


class QMLJSEDITOR_EXPORT QmlJSCompletionAssistProvider : public TextEditor::CompletionAssistProvider
{
    Q_OBJECT

public:
    TextEditor::IAssistProcessor *createProcessor(const TextEditor::AssistInterface *) const override;

    int activationCharSequenceLength() const override;
    bool isActivationCharSequence(const QString &sequence) const override;
    bool isContinuationChar(const QChar &c) const override;
};


QStringList QMLJSEDITOR_EXPORT qmlJSAutoComplete(QTextDocument *textDocument,
                                                 int position,
                                                 const Utils::FilePath &fileName,
                                                 TextEditor::AssistReason reason,
                                                 const QmlJSTools::SemanticInfo &info);

} // QmlJSEditor
