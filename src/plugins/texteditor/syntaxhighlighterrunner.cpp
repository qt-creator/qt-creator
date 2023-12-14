// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "syntaxhighlighterrunner.h"

#include "fontsettings.h"
#include "textdocumentlayout.h"
#include "texteditorsettings.h"

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
    void createHighlighter()
    {
        m_highlighter.reset(m_creator());
        m_highlighter->setFontSettings(m_fontSettings);
        m_highlighter->setDocument(m_document);
    }

    SyntaxHighlighterRunnerPrivate(BaseSyntaxHighlighterRunner::SyntaxHighLighterCreator creator,
                                   QTextDocument *document, FontSettings fontSettings)
        : m_creator(creator)
        , m_document(document)
        , m_fontSettings(fontSettings)
    {
        m_highlighter.reset(m_creator());
        createHighlighter();
    }

    void create()
    {
        if (m_document != nullptr)
            return;

        m_document = new QTextDocument(this);
        m_document->setDocumentLayout(new TextDocumentLayout(m_document));

        createHighlighter();

        connect(m_highlighter.get(),
                &SyntaxHighlighter::resultsReady,
                this,
                &SyntaxHighlighterRunnerPrivate::resultsReady);
    }

    void cloneDocument(int from,
                       int charsRemoved,
                       const QString textAdded,
                       const QMap<int, BaseSyntaxHighlighterRunner::BlockPreeditData> &blocksPreedit)
    {
        QTextCursor cursor(m_document);
        cursor.setPosition(qMin(m_document->characterCount() - 1, from + charsRemoved));
        cursor.setPosition(from, QTextCursor::KeepAnchor);
        cursor.insertText(textAdded);

        for (auto it = blocksPreedit.cbegin(); it != blocksPreedit.cend(); ++it) {
            const QTextBlock block = m_document->findBlock(it.key());
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
        m_highlighter->rehighlight();
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

signals:
    void resultsReady(const QList<SyntaxHighlighter::Result> &result);

private:
    BaseSyntaxHighlighterRunner::SyntaxHighLighterCreator m_creator;
    std::unique_ptr<SyntaxHighlighter> m_highlighter;
    QTextDocument *m_document = nullptr;
    FontSettings m_fontSettings;
};

// ----------------------------- BaseSyntaxHighlighterRunner --------------------------------------

BaseSyntaxHighlighterRunner::BaseSyntaxHighlighterRunner(
    BaseSyntaxHighlighterRunner::SyntaxHighLighterCreator creator,
    QTextDocument *document,
    const TextEditor::FontSettings &fontSettings)
    : d(new SyntaxHighlighterRunnerPrivate(creator, document, fontSettings))
{
    m_document = document;
}

BaseSyntaxHighlighterRunner::~BaseSyntaxHighlighterRunner() = default;

void BaseSyntaxHighlighterRunner::applyFormatRanges(const SyntaxHighlighter::Result &result)
{
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

void BaseSyntaxHighlighterRunner::cloneDocumentData(int from, int charsRemoved, int charsAdded)
{
    QMap<int, BaseSyntaxHighlighterRunner::BlockPreeditData> blocksPreedit;
    QTextBlock firstBlock = m_document->findBlock(from);
    QTextBlock endBlock = m_document->findBlock(from + charsAdded);
    while (firstBlock.isValid() && firstBlock.position() < endBlock.position()) {
        const int position = firstBlock.position();
        if (firstBlock.layout()) {
            const int preeditAreaPosition = firstBlock.layout()->preeditAreaPosition();
            const QString preeditAreaText = firstBlock.layout()->preeditAreaText();
            if (preeditAreaPosition != -1)
                blocksPreedit[position] = {preeditAreaPosition, preeditAreaText};
        }
        firstBlock = firstBlock.next();
    }

    const QString text = Utils::Text::textAt(QTextCursor(m_document), from, charsAdded);
    cloneDocument(from, charsRemoved, text, blocksPreedit);
}

void BaseSyntaxHighlighterRunner::cloneDocument(int from,
                                                int charsRemoved,
                                                const QString textAdded,
                                                const QMap<int, BlockPreeditData> &blocksPreedit)
{
    QMetaObject::invokeMethod(d.get(), [this, from, charsRemoved, textAdded, blocksPreedit] {
        d->cloneDocument(from, charsRemoved, textAdded, blocksPreedit);
    });
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
    connect(&m_thread, &QThread::finished, d.get(), [this] { d.release()->deleteLater(); });
    m_thread.start();

    QMetaObject::invokeMethod(d.get(), &SyntaxHighlighterRunnerPrivate::create);

    m_document = document;
    connect(d.get(),
            &SyntaxHighlighterRunnerPrivate::resultsReady,
            document,
            [this](const QList<SyntaxHighlighter::Result> &result) {
                for (const SyntaxHighlighter::Result &res : result)
                    applyFormatRanges(res);
            });
    cloneDocumentData(0, 0, document->characterCount());
    connect(document,
            &QTextDocument::contentsChange,
            this,
            [this](int from, int charsRemoved, int charsAdded) {
                if (!this->document())
                    return;

                cloneDocumentData(from, charsRemoved, charsAdded);
            });
}

ThreadedSyntaxHighlighterRunner::~ThreadedSyntaxHighlighterRunner()
{
    m_thread.quit();
    m_thread.wait();
}

} // namespace TextEditor

#include "syntaxhighlighterrunner.moc"
