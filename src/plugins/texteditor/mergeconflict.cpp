// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mergeconflict.h"

#include "fontsettings.h"
#include "textdocument.h"
#include "texteditor.h"
#include "texteditorconstants.h"
#include "texteditortr.h"

#include <utils/plaintextedit/texteditorlayout.h>
#include <utils/qtcassert.h>

#include <QFont>
#include <QLabel>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>

#include <algorithm>

using namespace Utils;

namespace TextEditor {

const Id MERGE_CONFLICT_SPACER_CATEGORY("TextEditor.MergeConflict.Spacer");

// marks the full-width backgrounds behind the two conflict sides, so they can
// be swept from the main block layouts on refresh (UserProperty + 43 and + 44
// are the full line highlight and the inline diff formats)
constexpr int MERGE_CONFLICT_SECTION_PROPERTY_ID = QTextFormat::UserProperty + 45;

// Seven is the shortest run Git writes; the conflict-marker-size gitattribute
// makes it write longer ones, and that attribute is set on exactly the files
// whose own content looks like a marker - prose, documentation, diff fixtures.
// Pinning the length to seven would leave those files without resolution
// links, so any run of at least seven counts.
constexpr qsizetype MinMarkerSize = 7;

static qsizetype markerRun(const QString &text, QChar marker)
{
    qsizetype run = 0;
    while (run < text.size() && text.at(run) == marker)
        ++run;
    return run < MinMarkerSize ? 0 : run;
}

// Git names the side each marker opens ("<<<<<<< HEAD", "||||||| base",
// ">>>>>>> branch"), so a label has to follow. That is what keeps a row of
// less-than signs in ordinary text from passing for a marker, at the price of
// not recognizing the label-less markers "git merge-file" writes when its
// labels are empty.
static bool isSideMarker(const QString &text, QChar marker)
{
    const qsizetype run = markerRun(text, marker);
    return run > 0 && run < text.size() && text.at(run) == ' ';
}

static bool isCurrentSideMarker(const QString &text) { return isSideMarker(text, '<'); }
static bool isBaseMarker(const QString &text) { return isSideMarker(text, '|'); }
static bool isIncomingSideMarker(const QString &text) { return isSideMarker(text, '>'); }

// The separator carries no label, so only whitespace may follow it - a file
// that has been round-tripped through a tool which pads lines still has one.
static bool isSeparatorMarker(const QString &text)
{
    const qsizetype run = markerRun(text, '=');
    return run > 0 && QStringView(text).sliced(run).trimmed().isEmpty();
}

static bool isConflictMarker(const QString &text)
{
    return isCurrentSideMarker(text) || isBaseMarker(text) || isSeparatorMarker(text)
           || isIncomingSideMarker(text);
}

QList<MergeConflict> findMergeConflicts(const QTextDocument *doc)
{
    enum class State { Outside, CurrentSide, IncomingSide };

    QList<MergeConflict> result;
    MergeConflict current;
    State state = State::Outside;
    for (QTextBlock block = doc->firstBlock(); block.isValid(); block = block.next()) {
        const QString text = block.text();
        const int line = block.blockNumber() + 1;
        if (isCurrentSideMarker(text)) {
            current = {};
            current.startLine = line;
            state = State::CurrentSide;
        } else if (state == State::CurrentSide) {
            if (isBaseMarker(text))
                current.baseLine = line;
            else if (isSeparatorMarker(text)) {
                current.separatorLine = line;
                state = State::IncomingSide;
            }
        } else if (state == State::IncomingSide && isIncomingSideMarker(text)) {
            current.endLine = line;
            result.append(current);
            state = State::Outside;
        }
    }
    return result;
}

// Replaces the blocks firstLine..lastLine (1-based, inclusive) with the given
// lines, as a single undo step.
static void replaceLines(QTextDocument *doc, int firstLine, int lastLine, const QStringList &lines)
{
    const QTextBlock first = doc->findBlockByNumber(firstLine - 1);
    const QTextBlock last = doc->findBlockByNumber(lastLine - 1);
    QTC_ASSERT(first.isValid() && last.isValid(), return);
    QString replacement = lines.join('\n');
    QTextCursor cursor(doc);
    cursor.beginEditBlock();
    cursor.setPosition(first.position());
    if (last.next().isValid()) {
        // include the trailing newline of the replaced lines. An empty list
        // removes the block; a single empty line ({""}, empty replacement but
        // not empty list) still needs its own trailing newline to remain a line
        cursor.setPosition(last.next().position(), QTextCursor::KeepAnchor);
        if (!lines.isEmpty())
            replacement += '\n';
    } else if (lines.isEmpty() && first.previous().isValid()) {
        // removing the last lines removes the preceding newline, too
        const QTextBlock previous = first.previous();
        cursor.setPosition(previous.position() + previous.length() - 1);
        cursor.setPosition(last.position() + qMax(0, last.length() - 1), QTextCursor::KeepAnchor);
    } else {
        cursor.setPosition(last.position() + qMax(0, last.length() - 1), QTextCursor::KeepAnchor);
    }
    cursor.insertText(replacement);
    cursor.endEditBlock();
}

void resolveMergeConflict(QTextDocument *doc, const MergeConflict &conflict,
                          MergeConflictChoice choice)
{
    const auto lineText = [doc](int line) {
        return doc->findBlockByNumber(line - 1).text();
    };
    // the controls are dropped on edits, but be defensive about staleness
    QTC_ASSERT(conflict.startLine > 0 && conflict.separatorLine > conflict.startLine
                   && conflict.endLine >= conflict.separatorLine
                   && conflict.endLine <= doc->blockCount()
                   && isCurrentSideMarker(lineText(conflict.startLine))
                   && isIncomingSideMarker(lineText(conflict.endLine)),
               return);
    QStringList lines;
    if (choice == MergeConflictChoice::Current || choice == MergeConflictChoice::Both) {
        const int currentEnd
            = (conflict.baseLine > 0 ? conflict.baseLine : conflict.separatorLine) - 1;
        for (int line = conflict.startLine + 1; line <= currentEnd; ++line)
            lines.append(lineText(line));
    }
    if (choice == MergeConflictChoice::Incoming || choice == MergeConflictChoice::Both) {
        for (int line = conflict.separatorLine + 1; line <= conflict.endLine - 1; ++line)
            lines.append(lineText(line));
    }
    replaceLines(doc, conflict.startLine, conflict.endLine, lines);
}

// Paints distinct full-width backgrounds behind the "current" (<<<<<<< to
// =======) and "incoming" (======= to >>>>>>>) side of each conflict on the
// widget's editor layout, so the two sides can be told apart. Replaces any
// previously applied conflict backgrounds; an empty list clears them.
static void highlightMergeConflictSections(TextEditorWidget *widget,
                                           const QList<MergeConflict> &conflicts)
{
    if (!widget)
        return;
    TextEditorLayout *layout = widget->editorLayout();
    if (!layout)
        return;
    const int removed = layout->removeMainLayoutFormatsWithProperty(MERGE_CONFLICT_SECTION_PROPERTY_ID);
    if (conflicts.isEmpty()) {
        if (removed > 0)
            layout->requestUpdate();
        return;
    }

    const FontSettingsData &fontSettings = widget->textDocument()->fontSettings();
    const auto sideFormat = [&](TextStyle style) {
        QTextCharFormat format;
        // the merge conflict sides have their own (themed) editor colors
        format.setBackground(fontSettings.toTextCharFormat(style).background());
        format.setProperty(FULL_LINE_HIGHLIGHT_FORMAT_PROPERTY_ID, true);
        format.setProperty(MERGE_CONFLICT_SECTION_PROPERTY_ID, true);
        return format;
    };
    const QTextCharFormat currentFormat = sideFormat(C_MERGE_CONFLICT_CURRENT);
    const QTextCharFormat incomingFormat = sideFormat(C_MERGE_CONFLICT_INCOMING);
    const QTextCharFormat baseFormat = sideFormat(C_MERGE_CONFLICT_BASE);
    // the marker lines ("<<<<<<<", "|||||||", "=======", ">>>>>>>") are shown
    // in bold so the conflict's structure stands out from its contents
    const auto boldMarker = [](QTextCharFormat format) {
        format.setFontWeight(QFont::Bold);
        return format;
    };
    // the "=======" separator has no side of its own: bold, no background
    QTextCharFormat separatorFormat;
    separatorFormat.setFontWeight(QFont::Bold);
    separatorFormat.setProperty(MERGE_CONFLICT_SECTION_PROPERTY_ID, true);

    QTextDocument *doc = widget->document();
    const auto highlight = [&](int firstLine, int lastLine, const QTextCharFormat &format) {
        for (int line = firstLine; line <= lastLine; ++line) {
            const QTextBlock block = doc->findBlockByNumber(line - 1);
            if (!block.isValid())
                continue;
            QTextLayout::FormatRange range;
            range.start = 0;
            range.length = qMax(1, block.length());
            range.format = format;
            layout->addBlockEditorFormats(block, {range});
        }
    };
    for (const MergeConflict &conflict : conflicts) {
        // current side: "<<<<<<<" (bold) through the line before "|||||||"/"======="
        const int currentEnd = (conflict.baseLine > 0 ? conflict.baseLine : conflict.separatorLine) - 1;
        highlight(conflict.startLine, conflict.startLine, boldMarker(currentFormat));
        highlight(conflict.startLine + 1, currentEnd, currentFormat);
        // base side (diff3 only): "|||||||" (bold) through the line before "======="
        if (conflict.baseLine > 0) {
            highlight(conflict.baseLine, conflict.baseLine, boldMarker(baseFormat));
            highlight(conflict.baseLine + 1, conflict.separatorLine - 1, baseFormat);
        }
        highlight(conflict.separatorLine, conflict.separatorLine, separatorFormat);
        // incoming side: the line after "=======" through ">>>>>>>" (bold)
        highlight(conflict.separatorLine + 1, conflict.endLine - 1, incomingFormat);
        highlight(conflict.endLine, conflict.endLine, boldMarker(incomingFormat));
    }
    layout->requestUpdate();
}

class MergeConflictControllerPrivate
{
public:
    MergeConflictController *q = nullptr;
    QPointer<TextEditorWidget> widget;
    QList<MergeConflict> conflicts;

    class Row
    {
    public:
        QPointer<QWidget> label;
        // start of the "<<<<<<<" block. A cursor is moved by the document's
        // own edits, so the row stays anchored to its conflict while the line
        // numbers found by the last scan are still stale
        QTextCursor anchor;

        int anchorLine() const { return anchor.blockNumber() + 1; }
    };
    QList<Row> rows;
    // one cursor per conflict, selecting it from its "<<<<<<<" to its
    // ">>>>>>>" line, so an edit can be told to fall inside a conflict without
    // consulting the line numbers of the last scan
    QList<QTextCursor> conflictRegions;

    QTimer rescanTimer;
    bool repositionScheduled = false;

    TextEditorLayout *layout() const { return widget ? widget->editorLayout() : nullptr; }

    void clearRows()
    {
        for (const Row &row : std::as_const(rows)) {
            if (row.label) {
                row.label->hide();
                row.label->deleteLater(); // a resolution link may be the caller
            }
        }
        rows.clear();
    }

    void clearSpacers()
    {
        if (TextEditorLayout *l = layout()) {
            if (l->removeAllLayoutItems(MERGE_CONFLICT_SPACER_CATEGORY) > 0) {
                l->emitDocumentSizeChanged();
                l->requestUpdate();
            }
        }
    }

    // Resolving edits the document, so a read-only view gets the conflict
    // highlighting but no links to resolve with.
    bool linksWanted() const { return widget && !widget->isReadOnly(); }

    // The reserved lines and their links only depend on where the conflicts
    // start, so they can stay in place while an edit moves them around.
    bool rowsAnchorConflicts() const
    {
        if (rows.size() != (linksWanted() ? conflicts.size() : 0))
            return false;
        for (int i = 0; i < rows.size(); ++i) {
            const Row &row = rows.at(i);
            if (!row.label || row.anchorLine() != conflicts.at(i).startLine)
                return false;
        }
        return true;
    }

    void rescan()
    {
        conflicts = widget ? findMergeConflicts(widget->document()) : QList<MergeConflict>();
        trackConflictRegions();
        // rebuilding the rows takes the reserved lines out of the layout and
        // puts them back, which moves the text below them; only do it when the
        // conflicts actually moved, appeared or disappeared
        if (!rowsAnchorConflicts())
            rebuildRows();
        // the highlights are swept and reapplied in one go, so they can be
        // refreshed unconditionally without the sections flickering
        highlightMergeConflictSections(widget, conflicts);
        reposition();
    }

    void trackConflictRegions()
    {
        conflictRegions.clear();
        if (!widget)
            return;
        QTextDocument *doc = widget->document();
        for (const MergeConflict &conflict : std::as_const(conflicts)) {
            const QTextBlock first = doc->findBlockByNumber(conflict.startLine - 1);
            const QTextBlock last = doc->findBlockByNumber(conflict.endLine - 1);
            if (!first.isValid() || !last.isValid())
                continue;
            QTextCursor region(doc);
            region.setPosition(first.position());
            region.setPosition(last.position() + last.length() - 1, QTextCursor::KeepAnchor);
            conflictRegions.append(region);
        }
    }

    // A scan only ever looks at the marker lines, so an edit that touches
    // neither a line carrying a marker now nor one of the conflicts found by
    // the last scan cannot change its outcome. Skipping those keeps the
    // whole-document scan off the editing path of the documents that have no
    // conflict markers at all, which is nearly all of them.
    bool changeMayAffectConflicts(int position, int charsAdded) const
    {
        if (!widget)
            return false;
        const int end = position + charsAdded;
        for (const QTextCursor &region : conflictRegions) {
            if (position <= region.selectionEnd() && end >= region.selectionStart())
                return true;
        }
        const QTextDocument *doc = widget->document();
        for (QTextBlock block = doc->findBlock(position);
             block.isValid() && block.position() <= end; block = block.next()) {
            if (isConflictMarker(block.text()))
                return true;
        }
        return false;
    }

    void rebuildRows()
    {
        clearRows();
        clearSpacers();
        TextEditorLayout *l = layout();
        if (!l || !linksWanted() || conflicts.isEmpty())
            return;
        QTextDocument *doc = widget->document();
        const int lineHeight = widget->fontMetrics().lineSpacing();
        for (const MergeConflict &conflict : std::as_const(conflicts)) {
            const QTextBlock block = doc->findBlockByNumber(conflict.startLine - 1);
            if (!block.isValid())
                continue;
            // reserve a line above the "<<<<<<<" marker for the links to float on
            l->ensureBlockLayout(block);
            l->prependLayoutItem(block,
                                 std::make_unique<EmptyLayoutItem>(lineHeight,
                                                                   MERGE_CONFLICT_SPACER_CATEGORY));
            auto label = new QLabel(widget->viewport());
            label->setObjectName(QLatin1StringView(MERGE_CONFLICT_CHOICES_OBJECT_NAME));
            label->setText(QString("<a href=\"current\">%1</a> | <a href=\"incoming\">%2</a>"
                                   " | <a href=\"both\">%3</a>")
                               .arg(Tr::tr("Choose Current Change").toHtmlEscaped(),
                                    Tr::tr("Choose Incoming Change").toHtmlEscaped(),
                                    Tr::tr("Choose Both").toHtmlEscaped()));
            const QTextCursor anchor(block);
            QObject::connect(label, &QLabel::linkActivated, q, [this, anchor](const QString &link) {
                MergeConflictChoice choice = MergeConflictChoice::Both;
                if (link == "current")
                    choice = MergeConflictChoice::Current;
                else if (link == "incoming")
                    choice = MergeConflictChoice::Incoming;
                else
                    QTC_ASSERT(link == "both", return);
                resolveAt(anchor.blockNumber() + 1, choice);
            });
            label->adjustSize();
            label->setFixedHeight(lineHeight);
            rows.append({label, anchor});
        }
        l->emitDocumentSizeChanged();
        l->requestUpdate();
    }

    // Resolves the conflict starting on the given line. The conflict is looked
    // up again instead of taken from the last scan, whose line numbers may have
    // moved on since the links were created.
    void resolveAt(int startLine, MergeConflictChoice choice)
    {
        if (!widget)
            return;
        QTextDocument *doc = widget->document();
        const QList<MergeConflict> current = findMergeConflicts(doc);
        const auto it = std::find_if(current.cbegin(), current.cend(),
                                     [startLine](const MergeConflict &conflict) {
                                         return conflict.startLine == startLine;
                                     });
        if (it == current.cend()) // the conflict was edited away under the links
            return;
        resolveMergeConflict(doc, *it, choice);
    }

    void reposition()
    {
        TextEditorLayout *l = layout();
        if (!l)
            return;
        QTextDocument *doc = widget->document();
        for (const Row &row : std::as_const(rows)) {
            if (!row.label)
                continue;
            const QTextBlock block = row.anchor.block();
            if (!block.isValid() || !block.isVisible()) {
                row.label->hide();
                continue;
            }
            // float on the spacer line reserved above the marker
            const int top = widget->cursorRect(QTextCursor(block)).top() - l->mainLayoutOffset(block);
            row.label->move(int(doc->documentMargin()), top);
            row.label->show();
        }
    }

    // updateRequest fires on every repaint, scroll and incremental rehighlight;
    // coalesce the repositions into one per event loop pass.
    void scheduleReposition()
    {
        if (repositionScheduled)
            return;
        repositionScheduled = true;
        QMetaObject::invokeMethod(q, [this] {
            repositionScheduled = false;
            reposition();
        }, Qt::QueuedConnection);
    }
};

MergeConflictController::MergeConflictController(TextEditorWidget *widget)
    : QObject(widget)
    , d(new MergeConflictControllerPrivate)
{
    d->q = this;
    d->widget = widget;
    d->rescanTimer.setSingleShot(true);
    d->rescanTimer.setInterval(150);
    connect(&d->rescanTimer, &QTimer::timeout, this, [this] { d->rescan(); });

    // the controls keep floating over their conflicts, anchored by cursors,
    // until the debounced rescan refreshes the conflicts' line numbers
    connect(widget->document(), &QTextDocument::contentsChange, this,
            [this](int position, int /*charsRemoved*/, int charsAdded) {
        if (d->changeMayAffectConflicts(position, charsAdded))
            d->rescanTimer.start();
    });
    // updateRequest also fires on scrolling
    connect(widget, &PlainTextEdit::updateRequest, this, [this] { d->scheduleReposition(); });
    connect(widget->textDocument(), &TextDocument::fontSettingsChanged, this, [this] {
        d->clearRows(); // the reserved lines follow the new line height
        d->rescan();
    });
    // the read-only state decides whether the links are offered, and has no
    // signal of its own
    widget->installEventFilter(this);

    d->rescan();
}

MergeConflictController::~MergeConflictController()
{
    // No cleanup here: the destructor usually runs during the widget's
    // teardown, where the widget is half gone already. The rows die with the
    // viewport and the layout items with the layout. A controller that
    // outlives its widget's document, or is switched off while the widget
    // lives on, is detached by TextEditorWidget beforehand.
    delete d;
}

bool MergeConflictController::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::ReadOnlyChange)
        d->rescan();
    return QObject::eventFilter(watched, event);
}

void MergeConflictController::detach()
{
    d->clearRows();
    d->clearSpacers();
    highlightMergeConflictSections(d->widget, {});
    d->conflicts.clear();
    d->conflictRegions.clear();
}

} // namespace TextEditor
