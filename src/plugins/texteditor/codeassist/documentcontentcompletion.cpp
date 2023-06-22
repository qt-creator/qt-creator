// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "documentcontentcompletion.h"

#include "assistinterface.h"
#include "assistproposalitem.h"
#include "asyncprocessor.h"
#include "genericproposal.h"
#include "iassistprocessor.h"
#include "../snippets/snippetassistcollector.h"
#include "../completionsettings.h"
#include "../texteditorsettings.h"

#include <utils/algorithm.h>

#include <QElapsedTimer>
#include <QRegularExpression>
#include <QSet>
#include <QTextBlock>
#include <QTextDocument>

using namespace TextEditor;

class DocumentContentCompletionProcessor final : public AsyncProcessor
{
public:
    DocumentContentCompletionProcessor(const QString &snippetGroupId);
    ~DocumentContentCompletionProcessor() final;

    IAssistProposal *performAsync() override;

private:
    QString m_snippetGroup;
};

DocumentContentCompletionProvider::DocumentContentCompletionProvider(const QString &snippetGroup)
    : m_snippetGroup(snippetGroup)
{ }

IAssistProcessor *DocumentContentCompletionProvider::createProcessor(const AssistInterface *) const
{
    return new DocumentContentCompletionProcessor(m_snippetGroup);
}

DocumentContentCompletionProcessor::DocumentContentCompletionProcessor(const QString &snippetGroupId)
    : m_snippetGroup(snippetGroupId)
{ }

DocumentContentCompletionProcessor::~DocumentContentCompletionProcessor()
{
    cancel();
}

IAssistProposal *DocumentContentCompletionProcessor::performAsync()
{
    int pos = interface()->position();

    QChar chr;
    // Skip to the start of a name
    do {
        chr = interface()->characterAt(--pos);
    } while (chr.isLetterOrNumber() || chr == '_');

    ++pos;
    int length = interface()->position() - pos;

    if (interface()->reason() == IdleEditor) {
        QChar characterUnderCursor = interface()->characterAt(interface()->position());
        if (characterUnderCursor.isLetterOrNumber()
                || length < TextEditorSettings::completionSettings().m_characterThreshold) {
            return nullptr;
        }
    }

    const TextEditor::SnippetAssistCollector snippetCollector(
                m_snippetGroup, QIcon(":/texteditor/images/snippet.png"));
    QList<AssistProposalItemInterface *> items = snippetCollector.collect();

    const QString wordUnderCursor = interface()->textAt(pos, length);
    const QString text = interface()->textDocument()->toPlainText();

    const QRegularExpression wordRE("([\\p{L}_][\\p{L}0-9_]{2,})");
    QSet<QString> words;
    QRegularExpressionMatchIterator it = wordRE.globalMatch(text);
    int wordUnderCursorFound = 0;
    while (it.hasNext()) {
        if (isCanceled())
            return nullptr;
        QRegularExpressionMatch match = it.next();
        const QString &word = match.captured();
        if (word == wordUnderCursor) {
            // Only add the word under cursor if it
            // already appears elsewhere in the text
            if (++wordUnderCursorFound < 2)
                continue;
        }

        if (Utils::insert(words, word)) {
            auto item = new AssistProposalItem();
            item->setText(word);
            items.append(item);
        }
    }

    return new GenericProposal(pos, items);
}
