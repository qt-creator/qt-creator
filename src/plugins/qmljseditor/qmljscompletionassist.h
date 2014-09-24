/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLJSCOMPLETIONASSIST_H
#define QMLJSCOMPLETIONASSIST_H

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
namespace Internal {

class QmlJSCompletionAssistInterface;

class QmlJSAssistProposalItem : public TextEditor::AssistProposalItem
{
public:
    bool prematurelyApplies(const QChar &c) const Q_DECL_OVERRIDE;
    void applyContextualContent(TextEditor::BaseTextEditorWidget *editorWidget,
                                int basePosition) const Q_DECL_OVERRIDE;
};


class QmlJSAssistProposalModel : public TextEditor::GenericProposalModel
{
public:
    QmlJSAssistProposalModel(const QList<TextEditor::AssistProposalItem *> &items)
    {
        loadContent(items);
    }

    void filter(const QString &prefix) Q_DECL_OVERRIDE;
    void sort(const QString &prefix) Q_DECL_OVERRIDE;
    bool keepPerfectMatch(TextEditor::AssistReason reason) const Q_DECL_OVERRIDE;
};


class QmlJSCompletionAssistProvider : public TextEditor::CompletionAssistProvider
{
    Q_OBJECT

public:
    bool supportsEditor(Core::Id editorId) const Q_DECL_OVERRIDE;
    TextEditor::IAssistProcessor *createProcessor() const Q_DECL_OVERRIDE;

    int activationCharSequenceLength() const Q_DECL_OVERRIDE;
    bool isActivationCharSequence(const QString &sequence) const Q_DECL_OVERRIDE;
    bool isContinuationChar(const QChar &c) const Q_DECL_OVERRIDE;
};


class QmlJSCompletionAssistProcessor : public TextEditor::IAssistProcessor
{
public:
    QmlJSCompletionAssistProcessor();
    ~QmlJSCompletionAssistProcessor();

    TextEditor::IAssistProposal *perform(const TextEditor::AssistInterface *interface) Q_DECL_OVERRIDE;

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
    QList<TextEditor::AssistProposalItem *> m_completions;
    TextEditor::SnippetAssistCollector m_snippetCollector;
};


class QmlJSCompletionAssistInterface : public TextEditor::AssistInterface
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

} // Internal
} // QmlJSEditor

#endif // QMLJSCOMPLETIONASSIST_H
