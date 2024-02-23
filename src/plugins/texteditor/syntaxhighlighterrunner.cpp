// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "syntaxhighlighterrunner.h"

#include "fontsettings.h"
#include "textdocumentlayout.h"
#include "texteditorsettings.h"
#include "highlighter.h"

#include <utils/algorithm.h>
#include <utils/textutils.h>

#include <QMetaObject>
#include <QTextCursor>
#include <QTextDocument>
#include <QThread>

namespace TextEditor {

struct BlockPreeditData {
    int position;
    QString text;
};

class SyntaxHighlighterRunnerPrivate : public QObject
{
    Q_OBJECT
public:
    SyntaxHighlighterRunnerPrivate(SyntaxHighlighter *highlighter,
                                   QTextDocument *document,
                                   bool async)
        : m_highlighter(highlighter)
    {
        if (async) {
            m_document = new QTextDocument(this);
            m_document->setDocumentLayout(new TextDocumentLayout(m_document));
            m_highlighter->setParent(m_document);
        } else {
            m_document = document;
        }

        m_highlighter->setDocument(m_document);

        connect(m_highlighter,
                &SyntaxHighlighter::resultsReady,
                this,
                &SyntaxHighlighterRunnerPrivate::resultsReady);
    }

    void changeDocument(int from,
                        int charsRemoved,
                        const QString textAdded,
                        const QMap<int, BlockPreeditData> &blocksPreedit)
    {
        QTextCursor cursor(m_document);
        cursor.setPosition(qMin(m_document->characterCount() - 1, from + charsRemoved));
        cursor.setPosition(from, QTextCursor::KeepAnchor);
        cursor.insertText(textAdded);

        for (auto it = blocksPreedit.cbegin(); it != blocksPreedit.cend(); ++it) {
            const QTextBlock block = m_document->findBlockByNumber(it.key());
            block.layout()->setPreeditArea(it.value().position, it.value().text);
        }
    }

    void setExtraFormats(const QMap<int, QList<QTextLayout::FormatRange>> &formatMap)
    {
        for (auto it = formatMap.cbegin(); it != formatMap.cend(); ++it)
            m_highlighter->setExtraFormats(m_document->findBlockByNumber(it.key()), it.value());
    }

    void clearExtraFormats(const QList<int> &blockNumbers)
    {
        for (auto it = blockNumbers.cbegin(); it != blockNumbers.cend(); ++it)
            m_highlighter->clearExtraFormats(m_document->findBlockByNumber(*it));
    }

    void clearAllExtraFormats() { m_highlighter->clearAllExtraFormats(); }

    void setFontSettings(const TextEditor::FontSettings &fontSettings)
    {
        m_highlighter->setFontSettings(fontSettings);
        rehighlight();
    }

    void setDefinitionName(const QString &name)
    {
        return m_highlighter->setDefinitionName(name);
    }

    void setLanguageFeaturesFlags(unsigned int flags)
    {
        m_highlighter->setLanguageFeaturesFlags(flags);
    }

    void setEnabled(bool enabled) { m_highlighter->setEnabled(enabled); }

    void rehighlight() { m_highlighter->rehighlight(); }

    SyntaxHighlighter *m_highlighter = nullptr;
    QTextDocument *m_document = nullptr;
signals:
    void resultsReady(const QList<SyntaxHighlighter::Result> &result);

};

SyntaxHighlighterRunner::SyntaxHighlighterRunner(SyntaxHighlighter *highlighter,
                                                 QTextDocument *document,
                                                 bool async)
    : d(new SyntaxHighlighterRunnerPrivate(highlighter, document, async))
    , m_document(document)
{
    m_useGenericHighlighter = qobject_cast<Highlighter *>(d->m_highlighter);

    if (async) {
        m_thread.emplace();
        d->moveToThread(&*m_thread);
        connect(&*m_thread, &QThread::finished, d, &QObject::deleteLater);
        m_thread->start();

        connect(d,
                &SyntaxHighlighterRunnerPrivate::resultsReady,
                this,
                &SyntaxHighlighterRunner::applyFormatRanges);

        changeDocument(0, 0, document->characterCount());
        connect(document,
                &QTextDocument::contentsChange,
                this,
                &SyntaxHighlighterRunner::changeDocument);

        m_foldValidator.setup(qobject_cast<TextDocumentLayout *>(document->documentLayout()));
    } else {
        connect(d,
                &SyntaxHighlighterRunnerPrivate::resultsReady,
                this,
                [this](const QList<SyntaxHighlighter::Result> &result) {
                    auto done = std::find_if(result.cbegin(),
                                             result.cend(),
                                             [](const SyntaxHighlighter::Result &res) {
                                                 return res.m_state == SyntaxHighlighter::State::Done;
                                             });
                    if (done != result.cend()) {
                        m_syntaxInfoUpdated = SyntaxHighlighter::State::Done;
                        emit highlightingFinished();
                        return;
                    }
                    m_syntaxInfoUpdated = SyntaxHighlighter::State::InProgress;
                });
    }
}

SyntaxHighlighterRunner::~SyntaxHighlighterRunner()
{
    if (m_thread) {
        m_thread->requestInterruption();
        m_thread->quit();
        m_thread->wait();
    } else {
        delete d->m_highlighter;
        delete d;
    }
}

void SyntaxHighlighterRunner::applyFormatRanges(const QList<SyntaxHighlighter::Result> &results)
{
    if (m_document == nullptr)
        return;

    for (const SyntaxHighlighter::Result &result : results) {
        m_syntaxInfoUpdated = result.m_state;
        if (m_syntaxInfoUpdated == SyntaxHighlighter::State::Start) {
            m_foldValidator.reset();
            continue;
        }
        if (m_syntaxInfoUpdated == SyntaxHighlighter::State::Done) {
            m_foldValidator.finalize();
            emit highlightingFinished();
            return;
        }

        QTextBlock docBlock = m_document->findBlock(result.m_blockNumber);
        if (!docBlock.isValid())
            return;

        result.copyToBlock(docBlock);

        if (result.m_formatRanges != docBlock.layout()->formats()) {
            docBlock.layout()->setFormats(result.m_formatRanges);
            m_document->markContentsDirty(docBlock.position(), docBlock.length());
        }
        if (m_syntaxInfoUpdated != SyntaxHighlighter::State::Extras)
            m_foldValidator.process(docBlock);
    }
}

void SyntaxHighlighterRunner::changeDocument(int from, int charsRemoved, int charsAdded)
{
    QTC_ASSERT(m_document, return);
    m_syntaxInfoUpdated = SyntaxHighlighter::State::InProgress;
    QMap<int, BlockPreeditData> blocksPreedit;
    QTextBlock block = m_document->findBlock(from);
    const QTextBlock endBlock = m_document->findBlock(from + charsAdded);
    while (block.isValid() && block != endBlock) {
        if (QTextLayout *layout = block.layout()) {
            if (const int pos = layout->preeditAreaPosition(); pos != -1)
                blocksPreedit[block.blockNumber()] = {pos, layout->preeditAreaText()};
        }
        block = block.next();
    }
    const QString text = Utils::Text::textAt(QTextCursor(m_document), from, charsAdded);
    QMetaObject::invokeMethod(d, [this, from, charsRemoved, text, blocksPreedit] {
        d->changeDocument(from, charsRemoved, text, blocksPreedit);
    });
}

bool SyntaxHighlighterRunner::useGenericHighlighter() const
{
    return m_useGenericHighlighter;
}

void SyntaxHighlighterRunner::setExtraFormats(
    const QMap<int, QList<QTextLayout::FormatRange>> &formatMap)
{
    QMetaObject::invokeMethod(d, [this, formatMap] { d->setExtraFormats(formatMap); });
}

void SyntaxHighlighterRunner::clearExtraFormats(const QList<int> &blockNumbers)
{
    QMetaObject::invokeMethod(d, [this, blockNumbers] { d->clearExtraFormats(blockNumbers); });
}

void SyntaxHighlighterRunner::clearAllExtraFormats()
{
    QMetaObject::invokeMethod(d, [this] { d->clearAllExtraFormats(); });
}

void SyntaxHighlighterRunner::setFontSettings(const TextEditor::FontSettings &fontSettings)
{
    QMetaObject::invokeMethod(d, [this, fontSettings] { d->setFontSettings(fontSettings); });
}

void SyntaxHighlighterRunner::setLanguageFeaturesFlags(unsigned int flags)
{
    QMetaObject::invokeMethod(d, [this, flags] { d->setLanguageFeaturesFlags(flags); });
}

void SyntaxHighlighterRunner::setEnabled(bool enabled)
{
    QMetaObject::invokeMethod(d, [this, enabled] { d->setEnabled(enabled); });
}

void SyntaxHighlighterRunner::rehighlight()
{
    QMetaObject::invokeMethod(d, [this] { d->rehighlight(); });
}

QString SyntaxHighlighterRunner::definitionName()
{
    return m_definitionName;
}

void SyntaxHighlighterRunner::setDefinitionName(const QString &name)
{
    if (name.isEmpty())
        return;

    m_definitionName = name;
    QMetaObject::invokeMethod(d, [this, name] { d->setDefinitionName(name); });
}

} // namespace TextEditor

#include "syntaxhighlighterrunner.moc"
