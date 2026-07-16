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

// marks the added/changed line and character highlight formats this decorator
// added to the main block layouts, so stripChangedLineFormats() can sweep them
// (FULL_LINE_HIGHLIGHT_FORMAT_PROPERTY_ID is UserProperty + 43)
constexpr int INLINE_DIFF_FORMAT_PROPERTY_ID = QTextFormat::UserProperty + 44;

// Deletion runs longer than this are elided to keep the ghost rows scannable.
constexpr int maxGhostLines = 100;

Utils::Id inlineDiffGhostCategory()
{
    return INLINE_DIFF_GHOST_CATEGORY;
}

QStringList inlineDiffChangedCharTexts(TextEditorWidget *widget)
{
    QStringList texts;
    if (!widget)
        return texts;
    TextEditorLayout *layout = widget->editorLayout();
    if (!layout)
        return texts;
    for (QTextBlock block = widget->document()->firstBlock(); block.isValid();
         block = block.next()) {
        const QString text = block.text();
        for (const QTextLayout::FormatRange &range : layout->blockSelectionHighlights(block))
            texts << text.mid(range.start, range.length);
    }
    return texts;
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

InlineDiffDecorator::InlineDiffDecorator(TextEditorWidget *widget, DiffSide side)
    : QObject(widget)
    , m_widget(widget)
    , m_side(side)
{
    QTextDocument *doc = widget->document();
    m_lastBlockCount = doc->blockCount();
    connect(doc, &QTextDocument::contentsChange,
            this, [this](int, int charsRemoved, int) {
        if (!m_widget)
            return;
        // Ghost rows are keyed by their anchor block's fragment index, which
        // can be recycled once blocks are removed - also by edits that remove
        // and insert blocks in one revision (e.g. moving lines). Drop all
        // ghost rows in that case and wait for the next apply() instead of
        // risking rows on unrelated blocks.
        const int blockCount = m_widget->document()->blockCount();
        if (charsRemoved > 0 && (blockCount < m_lastBlockCount || anchorsRecycled()))
            clearGhostItems();
        m_lastBlockCount = blockCount;
    });
    // ghost layouts hold a copy of the editor font, so rebuild them on zoom
    // and font or color scheme changes
    connect(widget->textDocument(), &TextDocument::fontSettingsChanged,
            this, [this] { apply(m_ghosts, m_changes, m_spacers); });
    // ... and when the font reaches the widget only later, which happens
    // when the settings change while the widget is not visible
    widget->installEventFilter(this);
}

bool InlineDiffDecorator::eventFilter(QObject *object, QEvent *event)
{
    if (m_widget && object == m_widget && event->type() == QEvent::FontChange)
        apply(m_ghosts, m_changes, m_spacers);
    return QObject::eventFilter(object, event);
}

bool InlineDiffDecorator::anchorsRecycled() const
{
    for (const QPair<QTextCursor, int> &anchor : m_itemAnchors) {
        if (anchor.first.block().fragmentIndex() != anchor.second)
            return true;
    }
    return false;
}

InlineDiffDecorator::~InlineDiffDecorator()
{
    // Deliberately do not clear() here: the decorator may be destroyed during
    // the teardown of the editor widget, whose layout is already gone at that
    // point. Whoever detaches a decorator from a live widget has to call
    // clear() explicitly; otherwise the decorations vanish with the widget.
}

void InlineDiffDecorator::apply(const QList<GhostBlock> &ghosts, const QList<ChangedRange> &changes,
                                const QList<Spacer> &spacers)
{
    if (!m_widget)
        return;

    if (&ghosts != &m_ghosts)
        m_ghosts = ghosts;
    if (&changes != &m_changes)
        m_changes = changes;
    if (&spacers != &m_spacers)
        m_spacers = spacers;

    TextEditorLayout *layout = m_widget->editorLayout();
    QTC_ASSERT(layout, return);
    layout->removeAllLayoutItems(INLINE_DIFF_GHOST_CATEGORY);
    stripChangedLineFormats();
    layout->clearSelectionHighlights();
    m_itemAnchors.clear();

    QTextDocument *doc = m_widget->document();
    const FontSettingsData &fontSettings = m_widget->textDocument()->fontSettings();
    const QTextCharFormat removedLineFormat
        = fontSettings.toTextCharFormat(C_DIFF_SOURCE_LINE);
    const QTextCharFormat removedCharFormat
        = fontSettings.toTextCharFormat(C_DIFF_SOURCE_CHAR);

    bool hasGhostRows = false;
    for (const GhostBlock &ghost : std::as_const(m_ghosts)) {
        // the model may refer to an older revision of the document, a fresh
        // model is applied once the diff of the current contents is finished
        if (ghost.lines.isEmpty() || ghost.anchorLine > doc->blockCount() + 1)
            continue;
        hasGhostRows = true;
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
        m_itemAnchors.append({QTextCursor(block), block.fragmentIndex()});
    }

    // empty rows that align this widget with the one showing the other side
    // of the diff
    const QBrush spacerBackground
        = fontSettings.toTextCharFormat(C_LINE_NUMBER).background();
    for (const Spacer &spacer : std::as_const(m_spacers)) {
        if (spacer.lineCount <= 0 || spacer.anchorLine > doc->blockCount() + 1)
            continue;
        const bool afterLastBlock = spacer.anchorLine > doc->blockCount();
        const QTextBlock block = afterLastBlock ? doc->lastBlock()
                                                : doc->findBlockByNumber(spacer.anchorLine - 1);
        QTC_ASSERT(block.isValid(), continue);

        auto item = std::make_unique<Utils::EmptyLayoutItem>(
            spacer.lineCount * layout->lineSpacing(), INLINE_DIFF_GHOST_CATEGORY);
        item->setBackground(spacerBackground);
        layout->ensureBlockLayout(block);
        if (afterLastBlock)
            layout->appendLayoutItem(block, std::move(item));
        else
            layout->prependLayoutItem(block, std::move(item));
        m_itemAnchors.append({QTextCursor(block), block.fragmentIndex()});
    }

    // full width line backgrounds are painted by TextEditorLayout::paintBackground
    // for main layout formats carrying FULL_LINE_HIGHLIGHT_FORMAT_PROPERTY_ID
    const bool isBaseline = m_side == DiffSide::Baseline;
    QTextCharFormat addedLineFormat;
    addedLineFormat.setBackground(
        fontSettings.toTextCharFormat(isBaseline ? C_DIFF_SOURCE_LINE : C_DIFF_DEST_LINE)
            .background());
    addedLineFormat.setProperty(FULL_LINE_HIGHLIGHT_FORMAT_PROPERTY_ID, true);
    addedLineFormat.setProperty(INLINE_DIFF_FORMAT_PROPERTY_ID, true);
    const QTextCharFormat addedCharFormat
        = fontSettings.toTextCharFormat(isBaseline ? C_DIFF_SOURCE_CHAR : C_DIFF_DEST_CHAR);

    for (const ChangedRange &range : std::as_const(m_changes)) {
        for (int line = range.startLine; line <= range.endLine; ++line) {
            const QTextBlock block = doc->findBlockByNumber(line - 1);
            if (!block.isValid())
                continue;
            QTextLayout::FormatRange lineRange;
            lineRange.start = 0;
            lineRange.length = qMax(1, block.length());
            lineRange.format = addedLineFormat;
            layout->addBlockEditorFormats(block, {lineRange});
        }
    }

    // the character level highlights are applied as format ranges on the block
    // layouts, the same lazy mechanism as the added line highlights above and
    // the ghost rows. Using extra selections here instead would build a
    // QTextCursor per highlight, and QTextCursor::setPosition forces a
    // synchronous layout of the cursor's block - laying out (and shaping) every
    // changed block of the whole document up front, even the off-screen ones,
    // which stalls the UI on large diffs.
    for (const ChangedRange &range : std::as_const(m_changes)) {
        for (auto it = range.charHighlights.cbegin(); it != range.charHighlights.cend(); ++it) {
            const QTextBlock block = doc->findBlockByNumber(it.key() - 1);
            if (!block.isValid())
                continue;
            const int maxLength = qMax(0, block.length() - 1);
            QList<QTextLayout::FormatRange> charRanges;
            for (const QPair<int, int> &charRange : it.value()) {
                if (charRange.first < 0 || charRange.second <= 0
                    || charRange.first >= maxLength) {
                    continue;
                }
                QTextLayout::FormatRange formatRange;
                formatRange.start = charRange.first;
                formatRange.length = qMin(charRange.second, maxLength - charRange.first);
                formatRange.format = addedCharFormat;
                charRanges << formatRange;
            }
            if (charRanges.isEmpty())
                continue;
            // drawn as selections (not format ranges) so the highlight fills
            // the whole changed range continuously, including the whitespace
            // between words even when a document format (e.g. a comment)
            // overlaps it
            layout->setBlockSelectionHighlights(block, charRanges);
        }
    }

    // publish the +/- gutter signs: changed real lines carry '+' on the editor
    // side and '-' on the baseline side; removed lines rendered as ghost rows
    // are marked '-' by the widget from the layout (hasGhostRows)
    QHash<int, QChar> signs;
    const QChar changedSign = isBaseline ? u'-' : u'+';
    for (const ChangedRange &range : std::as_const(m_changes)) {
        for (int line = range.startLine; line <= range.endLine; ++line)
            signs.insert(line - 1, changedSign);
    }
    m_widget->setDiffChangeSigns(signs, hasGhostRows);

    layout->emitDocumentSizeChanged();
    layout->requestUpdate();
}

void InlineDiffDecorator::clear()
{
    m_ghosts.clear();
    m_changes.clear();
    m_spacers.clear();
    if (!m_widget)
        return;
    clearGhostItems();
    TextEditorLayout *layout = m_widget->editorLayout();
    const int cleared = stripChangedLineFormats() + (layout ? layout->clearSelectionHighlights() : 0);
    if (cleared > 0 && layout)
        layout->requestUpdate();
    m_widget->setDiffChangeSigns({}, false);
}

int InlineDiffDecorator::stripChangedLineFormats()
{
    if (!m_widget)
        return 0;
    TextEditorLayout *layout = m_widget->editorLayout();
    if (!layout)
        return 0;
    // sweeps all layout data, including entries whose fragment index has no
    // block at the moment - recycling such an index must not resurrect bands
    return layout->removeMainLayoutFormatsWithProperty(INLINE_DIFF_FORMAT_PROPERTY_ID);
}

void InlineDiffDecorator::clearGhostItems()
{
    if (!m_widget)
        return;
    m_itemAnchors.clear();
    TextEditorLayout *layout = m_widget->editorLayout();
    if (!layout)
        return;
    if (layout->removeAllLayoutItems(INLINE_DIFF_GHOST_CATEGORY) > 0) {
        layout->emitDocumentSizeChanged();
        layout->requestUpdate();
    }
}

} // namespace TextEditor
