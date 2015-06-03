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

class QmlJSCompletionAssistInterface;

namespace Internal {

class QmlJSAssistProposalItem : public TextEditor::AssistProposalItem
{
public:
    bool prematurelyApplies(const QChar &c) const override;
    void applyContextualContent(TextEditor::TextEditorWidget *editorWidget,
                                int basePosition) const override;
};


class QmlJSAssistProposalModel : public TextEditor::GenericProposalModel
{
public:
    QmlJSAssistProposalModel(const QList<TextEditor::AssistProposalItem *> &items)
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
    QList<TextEditor::AssistProposalItem *> m_completions;
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

} // QmlJSEditor

#endif // QMLJSCOMPLETIONASSIST_H
