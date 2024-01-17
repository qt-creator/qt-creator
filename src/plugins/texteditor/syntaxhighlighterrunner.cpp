// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "syntaxhighlighterrunner.h"

#include "fontsettings.h"
#include "textdocumentlayout.h"
#include "texteditorsettings.h"
#include "highlighter.h"

#include <utils/textutils.h>

#include <QMetaObject>
#include <QTextCursor>
#include <QTextDocument>
#include <QThread>

namespace TextEditor {

class SyntaxHighlighterRunnerPrivate : public QObject
{
    Q_OBJECT
public:
    SyntaxHighlighterRunnerPrivate(BaseSyntaxHighlighterRunner::SyntaxHighLighterCreator creator,
                                   QTextDocument *document,
                                   FontSettings fontSettings)
        : m_document(document)
        , m_fontSettings(fontSettings)
    {
        if (!m_document) {
            m_document = new QTextDocument(this);
            m_document->setDocumentLayout(new TextDocumentLayout(m_document));
        }

        m_highlighter.reset(creator());
        m_highlighter->setFontSettings(m_fontSettings);
        m_highlighter->setDocument(m_document);
        m_highlighter->setParent(m_document);

        connect(m_highlighter.get(),
                &SyntaxHighlighter::resultsReady,
                this,
                &SyntaxHighlighterRunnerPrivate::resultsReady);
    }

    void changeDocument(int from,
                        int charsRemoved,
                        const QString textAdded,
                        const QMap<int, BaseSyntaxHighlighterRunner::BlockPreeditData> &blocksPreedit)
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

    std::unique_ptr<SyntaxHighlighter> m_highlighter;
    QTextDocument *m_document = nullptr;
    FontSettings m_fontSettings;

signals:
    void resultsReady(const QList<SyntaxHighlighter::Result> &result);

};

// ----------------------------- BaseSyntaxHighlighterRunner --------------------------------------

BaseSyntaxHighlighterRunner::BaseSyntaxHighlighterRunner(
    BaseSyntaxHighlighterRunner::SyntaxHighLighterCreator creator,
    QTextDocument *document,
    const TextEditor::FontSettings &fontSettings)
    : d(new SyntaxHighlighterRunnerPrivate(creator, document, fontSettings))
    , m_document(document)
{
    m_useGenericHighlighter = qobject_cast<Highlighter *>(d->m_highlighter.get());

    if (document == nullptr)
        return;

    connect(d.get(),
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

BaseSyntaxHighlighterRunner::~BaseSyntaxHighlighterRunner() = default;

void BaseSyntaxHighlighterRunner::applyFormatRanges(const QList<SyntaxHighlighter::Result> &results)
{
    if (m_document == nullptr)
        return;

    for (const SyntaxHighlighter::Result &result : results) {
        m_syntaxInfoUpdated = result.m_state;
        if (m_syntaxInfoUpdated == SyntaxHighlighter::State::Done) {
            emit highlightingFinished();
            return;
        }

        QTextBlock docBlock = m_document->findBlock(result.m_blockNumber);
        if (!docBlock.isValid())
            return;

        result.copyToBlock(docBlock);

        if (!result.m_formatRanges.empty()) {
            TextDocumentLayout::FoldValidator foldValidator;
            foldValidator.setup(qobject_cast<TextDocumentLayout *>(m_document->documentLayout()));
            docBlock.layout()->setFormats(result.m_formatRanges);
            m_document->markContentsDirty(docBlock.position(), docBlock.length());
            foldValidator.process(docBlock);
        }
    }
}

void BaseSyntaxHighlighterRunner::changeDocument(int from, int charsRemoved, int charsAdded)
{
    QTC_ASSERT(m_document, return);
    m_syntaxInfoUpdated = SyntaxHighlighter::State::InProgress;
    QMap<int, BaseSyntaxHighlighterRunner::BlockPreeditData> blocksPreedit;
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
    QMetaObject::invokeMethod(d.get(), [this, from, charsRemoved, text, blocksPreedit] {
        d->changeDocument(from, charsRemoved, text, blocksPreedit);
    });
}

bool BaseSyntaxHighlighterRunner::useGenericHighlighter() const
{
    return m_useGenericHighlighter;
}

void BaseSyntaxHighlighterRunner::setExtraFormats(
    const QMap<int, QList<QTextLayout::FormatRange>> &formatMap)
{
    QMetaObject::invokeMethod(d.get(), [this, formatMap] { d->setExtraFormats(formatMap); });
}

void BaseSyntaxHighlighterRunner::clearExtraFormats(const QList<int> &blockNumbers)
{
    QMetaObject::invokeMethod(d.get(), [this, blockNumbers] { d->clearExtraFormats(blockNumbers); });
}

void BaseSyntaxHighlighterRunner::clearAllExtraFormats()
{
    QMetaObject::invokeMethod(d.get(), [this] { d->clearAllExtraFormats(); });
}

void BaseSyntaxHighlighterRunner::setFontSettings(const TextEditor::FontSettings &fontSettings)
{
    QMetaObject::invokeMethod(d.get(), [this, fontSettings] { d->setFontSettings(fontSettings); });
}

void BaseSyntaxHighlighterRunner::setLanguageFeaturesFlags(unsigned int flags)
{
    QMetaObject::invokeMethod(d.get(), [this, flags] { d->setLanguageFeaturesFlags(flags); });
}

void BaseSyntaxHighlighterRunner::setEnabled(bool enabled)
{
    QMetaObject::invokeMethod(d.get(), [this, enabled] { d->setEnabled(enabled); });
}

void BaseSyntaxHighlighterRunner::rehighlight()
{
    QMetaObject::invokeMethod(d.get(), [this] { d->rehighlight(); });
}

QString BaseSyntaxHighlighterRunner::definitionName()
{
    return m_definitionName;
}

void BaseSyntaxHighlighterRunner::setDefinitionName(const QString &name)
{
    if (name.isEmpty())
        return;

    m_definitionName = name;
    QMetaObject::invokeMethod(d.get(), [this, name] { d->setDefinitionName(name); });
}

// --------------------------- ThreadedSyntaxHighlighterRunner ------------------------------------

ThreadedSyntaxHighlighterRunner::ThreadedSyntaxHighlighterRunner(SyntaxHighLighterCreator creator,
                                                                 QTextDocument *document)
    : BaseSyntaxHighlighterRunner(creator, nullptr)
{
    QTC_ASSERT(document, return);

    d->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, d.get(), &QObject::deleteLater);
    m_thread.start();

    m_document = document;
    connect(d.get(),
            &SyntaxHighlighterRunnerPrivate::resultsReady,
            this,
            &ThreadedSyntaxHighlighterRunner::applyFormatRanges);

    changeDocument(0, 0, document->characterCount());
    connect(document,
            &QTextDocument::contentsChange,
            this,
            &ThreadedSyntaxHighlighterRunner::changeDocument);
}

ThreadedSyntaxHighlighterRunner::~ThreadedSyntaxHighlighterRunner()
{
    m_thread.requestInterruption();
    m_thread.quit();
    m_thread.wait();
    d.release();
}

} // namespace TextEditor

#include "syntaxhighlighterrunner.moc"
