/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "documentcontentcompletion.h"

#include "assistinterface.h"
#include "assistproposalitem.h"
#include "genericproposal.h"
#include "genericproposalmodel.h"
#include "iassistprocessor.h"
#include "../snippets/snippetassistcollector.h"

#include "utils/runextensions.h"

#include <QElapsedTimer>
#include <QRegularExpression>
#include <QSet>
#include <QTextBlock>
#include <QTextDocument>

using namespace TextEditor;

class DocumentContentCompletionProcessor : public IAssistProcessor
{
public:
    DocumentContentCompletionProcessor(const QString &snippetGroupId);

    IAssistProposal *perform(const AssistInterface *interface) override;
    bool running() final { return m_running; }

private:
    TextEditor::SnippetAssistCollector m_snippetCollector;
    IAssistProposal *createProposal(const AssistInterface *interface);
    bool m_running = false;
};

DocumentContentCompletionProvider::DocumentContentCompletionProvider(const QString &snippetGroup)
    : m_snippetGroup(snippetGroup)
{ }

IAssistProvider::RunType DocumentContentCompletionProvider::runType() const
{
    return Asynchronous;
}

IAssistProcessor *DocumentContentCompletionProvider::createProcessor() const
{
    return new DocumentContentCompletionProcessor(m_snippetGroup);
}

DocumentContentCompletionProcessor::DocumentContentCompletionProcessor(const QString &snippetGroupId)
    : m_snippetCollector(snippetGroupId, QIcon(":/texteditor/images/snippet.png"))
{ }

IAssistProposal *DocumentContentCompletionProcessor::perform(const AssistInterface *interface)
{
    Utils::onResultReady(Utils::runAsync(
                             &DocumentContentCompletionProcessor::createProposal, this, interface),
                         [this](IAssistProposal *proposal){
        m_running = false;
        setAsyncProposalAvailable(proposal);
    });
    m_running = true;
    return nullptr;
}

static void generateProposalItems(const QString &text, QSet<QString> &words,
                                  QList<AssistProposalItemInterface *> &items)
{
    static const QRegularExpression wordRE("([a-zA-Z_][a-zA-Z0-9_]{2,})");

    QRegularExpressionMatch match;
    int index = text.indexOf(wordRE, 0, &match);
    while (index >= 0) {
        const QString &word = match.captured();
        if (!words.contains(word)) {
            auto item = new AssistProposalItem();
            item->setText(word);
            items.append(item);
            words.insert(word);
        }
        index += word.size();
        index = text.indexOf(wordRE, index, &match);
    }
}

IAssistProposal *DocumentContentCompletionProcessor::createProposal(
        const AssistInterface *interface)
{
    QScopedPointer<const AssistInterface> assistInterface(interface);
    int pos = interface->position();

    QChar chr;
    // Skip to the start of a name
    do {
        chr = interface->characterAt(--pos);
    } while (chr.isLetterOrNumber() || chr == '_');

    ++pos;

    if (interface->reason() == IdleEditor) {
        QChar characterUnderCursor = interface->characterAt(interface->position());
        if (characterUnderCursor.isLetterOrNumber() || interface->position() - pos < 3)
            return nullptr;
    }

    QSet<QString> words;
    QList<AssistProposalItemInterface *> items = m_snippetCollector.collect();
    QTextBlock block = interface->textDocument()->firstBlock();

    while (block.isValid()) {
        generateProposalItems(block.text(), words, items);
        block = block.next();
    }

    return new GenericProposal(pos, items);
}
