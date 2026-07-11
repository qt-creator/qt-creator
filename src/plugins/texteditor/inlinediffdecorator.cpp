// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "inlinediffdecorator.h"

#include "fontsettings.h"
#include "textdocument.h"
#include "texteditor.h"
#include "texteditorconstants.h"
#include "texteditortr.h"

#include <utils/plaintextedit/texteditorlayout.h>
#include <utils/qtcassert.h>

#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextLayout>

using namespace Utils;

namespace TextEditor {

const Id INLINE_DIFF_GHOST_CATEGORY("TextEditor.InlineDiff.Ghost");
const Id INLINE_DIFF_SELECTION_ID("TextEditor.InlineDiff.Selection");

// marks the full width line background formats this decorator added to the
// main block layouts (FULL_LINE_HIGHLIGHT_FORMAT_PROPERTY_ID is UserProperty + 43)
constexpr int INLINE_DIFF_FORMAT_PROPERTY_ID = QTextFormat::UserProperty + 44;

// Deletion runs longer than this are elided to keep the ghost rows scannable.
constexpr int maxGhostLines = 100;

Utils::Id inlineDiffGhostCategory()
{
    return INLINE_DIFF_GHOST_CATEGORY;
}

Utils::Id inlineDiffSelectionKind()
{
    return INLINE_DIFF_SELECTION_ID;
}

static std::unique_ptr<QTextLayout> createGhostLayout(
    const InlineDiffDecorator::GhostBlock &ghost,
    const QTextDocument *doc,
    const QTextCharFormat &lineFormat,
    const QTextCharFormat &charFormat)
{
    QStringList lines = ghost.lines;
    QList<InlineDiffDecorator::CharRanges> charHighlights = ghost.charHighlights;
    if (lines.size() > maxGhostLines) {
        const int elided = int(lines.size()) - maxGhostLines;
        lines = lines.mid(0, maxGhostLines);
        lines << Tr::tr("... (%n more removed lines)", nullptr, elided);
        charHighlights.clear();
    }

    const QString text = lines.join(QChar::LineSeparator);
    auto layout = std::make_unique<QTextLayout>();
    layout->setText(text);
    layout->setFont(doc->defaultFont());

    QList<QTextLayout::FormatRange> formats;
    QTextLayout::FormatRange base;
    base.start = 0;
    base.length = int(text.size());
    base.format = lineFormat;
    base.format.setProperty(FULL_LINE_HIGHLIGHT_FORMAT_PROPERTY_ID, true);
    formats << base;

    int lineStart = 0;
    for (int i = 0; i < lines.size(); ++i) {
        if (i < charHighlights.size()) {
            for (const QPair<int, int> &range : charHighlights.at(i)) {
                QTextLayout::FormatRange r;
                r.start = lineStart + range.first;
                r.length = range.second;
                r.format = charFormat;
                if (range.first >= 0 && r.length > 0 && range.first + r.length <= lines.at(i).size())
                    formats << r;
            }
        }
        lineStart += int(lines.at(i).size()) + 1; // + line separator
    }
    layout->setFormats(formats);
    return layout;
}

InlineDiffDecorator::InlineDiffDecorator(TextEditorWidget *widget)
    : QObject(widget)
    , m_widget(widget)
{
    QTextDocument *doc = widget->document();
    m_lastBlockCount = doc->blockCount();
    connect(doc, &QTextDocument::contentsChange,
            this, [this](int, int charsRemoved, int) {
        if (!m_widget)
            return;
        // Ghost rows are keyed by their anchor block's fragment index, which can be
        // recycled once blocks are removed. Drop all ghost rows in that case and
        // wait for the next apply() instead of risking rows on unrelated blocks.
        const int blockCount = m_widget->document()->blockCount();
        if (charsRemoved > 0 && blockCount < m_lastBlockCount)
            clearGhostItems();
        m_lastBlockCount = blockCount;
    });
    // ghost layouts hold a copy of the editor font, so rebuild them on zoom
    // and font or color scheme changes
    connect(widget->textDocument(), &TextDocument::fontSettingsChanged,
            this, [this] { apply(m_ghosts, m_changes); });
}

InlineDiffDecorator::~InlineDiffDecorator()
{
    // Deliberately do not clear() here: the decorator may be destroyed during
    // the teardown of the editor widget, whose layout is already gone at that
    // point. Whoever detaches a decorator from a live widget has to call
    // clear() explicitly; otherwise the decorations vanish with the widget.
}

void InlineDiffDecorator::apply(const QList<GhostBlock> &ghosts, const QList<ChangedRange> &changes)
{
    if (!m_widget)
        return;

    if (&ghosts != &m_ghosts)
        m_ghosts = ghosts;
    if (&changes != &m_changes)
        m_changes = changes;

    TextEditorLayout *layout = m_widget->editorLayout();
    QTC_ASSERT(layout, return);
    layout->removeAllLayoutItems(INLINE_DIFF_GHOST_CATEGORY);
    stripChangedLineFormats();

    QTextDocument *doc = m_widget->document();
    const FontSettingsData &fontSettings = m_widget->textDocument()->fontSettings();
    const QTextCharFormat removedLineFormat
        = fontSettings.toTextCharFormat(C_DIFF_SOURCE_LINE);
    const QTextCharFormat removedCharFormat
        = fontSettings.toTextCharFormat(C_DIFF_SOURCE_CHAR);

    for (const GhostBlock &ghost : std::as_const(m_ghosts)) {
        // the model may refer to an older revision of the document, a fresh
        // model is applied once the diff of the current contents is finished
        if (ghost.lines.isEmpty() || ghost.anchorLine > doc->blockCount() + 1)
            continue;
        const bool afterLastBlock = ghost.anchorLine > doc->blockCount();
        const QTextBlock block = afterLastBlock ? doc->lastBlock()
                                                : doc->findBlockByNumber(ghost.anchorLine - 1);
        QTC_ASSERT(block.isValid(), continue);

        std::unique_ptr<QTextLayout> ghostLayout
            = createGhostLayout(ghost, doc, removedLineFormat, removedCharFormat);
        // the main layout has to exist, otherwise it would end up behind the
        // prepended ghost layout
        layout->ensureBlockLayout(block);
        if (afterLastBlock)
            layout->appendAdditionalLayouts(block, {ghostLayout.release()},
                                            INLINE_DIFF_GHOST_CATEGORY);
        else
            layout->prependAdditionalLayouts(block, {ghostLayout.release()},
                                             INLINE_DIFF_GHOST_CATEGORY);
    }

    // full width line backgrounds are painted by TextEditorLayout::paintBackground
    // for main layout formats carrying FULL_LINE_HIGHLIGHT_FORMAT_PROPERTY_ID
    QTextCharFormat addedLineFormat;
    addedLineFormat.setBackground(
        fontSettings.toTextCharFormat(C_DIFF_DEST_LINE).background());
    addedLineFormat.setProperty(FULL_LINE_HIGHLIGHT_FORMAT_PROPERTY_ID, true);
    addedLineFormat.setProperty(INLINE_DIFF_FORMAT_PROPERTY_ID, true);
    const QTextCharFormat addedCharFormat = fontSettings.toTextCharFormat(C_DIFF_DEST_CHAR);

    for (const ChangedRange &range : std::as_const(m_changes)) {
        for (int line = range.startLine; line <= range.endLine; ++line) {
            const QTextBlock block = doc->findBlockByNumber(line - 1);
            if (!block.isValid())
                continue;
            QTextLayout *blockLayout = layout->blockLayout(block);
            QTextLayout::FormatRange lineRange;
            lineRange.start = 0;
            lineRange.length = qMax(1, block.length());
            lineRange.format = addedLineFormat;
            blockLayout->setFormats(blockLayout->formats() + QList{lineRange});
        }
    }

    // the character level highlights use extra selections, their cursors keep
    // tracking the text while the user edits
    QList<QTextEdit::ExtraSelection> selections;
    for (const ChangedRange &range : std::as_const(m_changes)) {
        for (auto it = range.charHighlights.cbegin(); it != range.charHighlights.cend(); ++it) {
            const QTextBlock block = doc->findBlockByNumber(it.key() - 1);
            if (!block.isValid())
                continue;
            const int maxLength = qMax(0, block.length() - 1);
            for (const QPair<int, int> &charRange : it.value()) {
                if (charRange.first < 0 || charRange.second <= 0
                    || charRange.first >= maxLength) {
                    continue;
                }
                QTextEdit::ExtraSelection selection;
                selection.cursor = QTextCursor(doc);
                selection.cursor.setPosition(block.position() + charRange.first);
                selection.cursor.setPosition(
                    block.position() + qMin(charRange.first + charRange.second, maxLength),
                    QTextCursor::KeepAnchor);
                selection.format = addedCharFormat;
                selections << selection;
            }
        }
    }
    m_widget->setExtraSelections(INLINE_DIFF_SELECTION_ID, selections);

    layout->emitDocumentSizeChanged();
    layout->requestUpdate();
}

void InlineDiffDecorator::clear()
{
    m_ghosts.clear();
    m_changes.clear();
    if (!m_widget)
        return;
    clearGhostItems();
    stripChangedLineFormats();
    m_widget->setExtraSelections(INLINE_DIFF_SELECTION_ID, {});
}

void InlineDiffDecorator::stripChangedLineFormats()
{
    if (!m_widget)
        return;
    TextEditorLayout *layout = m_widget->editorLayout();
    if (!layout)
        return;
    QTextDocument *doc = m_widget->document();
    for (QTextBlock block = doc->firstBlock(); block.isValid(); block = block.next()) {
        QTextLayout *blockLayout = layout->existingBlockLayout(block);
        if (!blockLayout)
            continue;
        QList<QTextLayout::FormatRange> formats = blockLayout->formats();
        const auto removed = formats.removeIf([](const QTextLayout::FormatRange &range) {
            return range.format.boolProperty(INLINE_DIFF_FORMAT_PROPERTY_ID);
        });
        if (removed > 0)
            blockLayout->setFormats(formats);
    }
}

void InlineDiffDecorator::clearGhostItems()
{
    if (!m_widget)
        return;
    TextEditorLayout *layout = m_widget->editorLayout();
    if (!layout)
        return;
    if (layout->removeAllLayoutItems(INLINE_DIFF_GHOST_CATEGORY) > 0) {
        layout->emitDocumentSizeChanged();
        layout->requestUpdate();
    }
}

} // namespace TextEditor
