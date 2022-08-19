// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "documentcontentcompletion.h"

#include "assistinterface.h"
#include "assistproposalitem.h"
#include "genericproposal.h"
#include "genericproposalmodel.h"
#include "iassistprocessor.h"
#include "../snippets/snippetassistcollector.h"
#include "../completionsettings.h"
#include "../texteditorsettings.h"

#include <utils/algorithm.h>
#include <utils/runextensions.h>

#include <QElapsedTimer>
#include <QRegularExpression>
#include <QSet>
#include <QTextBlock>
#include <QTextDocument>

using namespace TextEditor;

class DocumentContentCompletionProcessor final : public IAssistProcessor
{
public:
    DocumentContentCompletionProcessor(const QString &snippetGroupId);
    ~DocumentContentCompletionProcessor() final;

    IAssistProposal *perform(const AssistInterface *interface) override;
    bool running() final { return m_watcher.isRunning(); }
    void cancel() final;

private:
    QString m_snippetGroup;
    QFutureWatcher<QStringList> m_watcher;
};

DocumentContentCompletionProvider::DocumentContentCompletionProvider(const QString &snippetGroup)
    : m_snippetGroup(snippetGroup)
{ }

IAssistProvider::RunType DocumentContentCompletionProvider::runType() const
{
    return Asynchronous;
}

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

static void createProposal(QFutureInterface<QStringList> &future, const QString &text,
                           const QString &wordUnderCursor)
{
    const QRegularExpression wordRE("([\\p{L}_][\\p{L}0-9_]{2,})");

    QSet<QString> words;
    QRegularExpressionMatchIterator it = wordRE.globalMatch(text);
    int wordUnderCursorFound = 0;
    while (it.hasNext()) {
        if (future.isCanceled())
            return;
        QRegularExpressionMatch match = it.next();
        const QString &word = match.captured();
        if (word == wordUnderCursor) {
            // Only add the word under cursor if it
            // already appears elsewhere in the text
            if (++wordUnderCursorFound < 2)
                continue;
        }

        if (!words.contains(word))
            words.insert(word);
    }

    future.reportResult(Utils::toList(words));
}

IAssistProposal *DocumentContentCompletionProcessor::perform(const AssistInterface *interface)
{
    QScopedPointer<const AssistInterface> assistInterface(interface);
    if (running())
        return nullptr;

    int pos = interface->position();

    QChar chr;
    // Skip to the start of a name
    do {
        chr = interface->characterAt(--pos);
    } while (chr.isLetterOrNumber() || chr == '_');

    ++pos;
    int length = interface->position() - pos;

    if (interface->reason() == IdleEditor) {
        QChar characterUnderCursor = interface->characterAt(interface->position());
        if (characterUnderCursor.isLetterOrNumber()
                || length < TextEditorSettings::completionSettings().m_characterThreshold) {
            return nullptr;
        }
    }

    const QString wordUnderCursor = interface->textAt(pos, length);
    const QString text = interface->textDocument()->toPlainText();

    m_watcher.setFuture(Utils::runAsync(&createProposal, text, wordUnderCursor));
    QObject::connect(&m_watcher, &QFutureWatcher<QStringList>::resultReadyAt,
                     &m_watcher, [this, pos](int index){
        const TextEditor::SnippetAssistCollector snippetCollector(
                    m_snippetGroup, QIcon(":/texteditor/images/snippet.png"));
        QList<AssistProposalItemInterface *> items = snippetCollector.collect();
        for (const QString &word : m_watcher.resultAt(index)) {
            auto item = new AssistProposalItem();
            item->setText(word);
            items.append(item);
        }
        setAsyncProposalAvailable(new GenericProposal(pos, items));
    });
    return nullptr;
}

void DocumentContentCompletionProcessor::cancel()
{
    if (running())
        m_watcher.cancel();
}
