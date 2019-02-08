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

#include <utils/runextensions.h>

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
    ~DocumentContentCompletionProcessor() override;

    IAssistProposal *perform(const AssistInterface *interface) override;
    bool running() final { return m_watcher.isRunning(); }

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

IAssistProcessor *DocumentContentCompletionProvider::createProcessor() const
{
    return new DocumentContentCompletionProcessor(m_snippetGroup);
}

DocumentContentCompletionProcessor::DocumentContentCompletionProcessor(const QString &snippetGroupId)
    : m_snippetGroup(snippetGroupId)
{ }

DocumentContentCompletionProcessor::~DocumentContentCompletionProcessor()
{
    if (m_watcher.isRunning())
        m_watcher.cancel();
}

static void createProposal(QFutureInterface<QStringList> &future, const QString &text,
                           const QString &wordUnderCursor)
{
    const QRegularExpression wordRE("([a-zA-Z_][a-zA-Z0-9_]{2,})");

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

    future.reportResult(words.toList());
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
        if (characterUnderCursor.isLetterOrNumber() || length < 3)
            return nullptr;
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
