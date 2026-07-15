// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "inlinediff.h"

#include "diffeditoricons.h"
#include "diffeditortr.h"
#include "diffutils.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <coreplugin/vcsmanager.h>

#include <texteditor/fontsettings.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorconstants.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/differ.h>
#include <utils/infobar.h>
#include <utils/plaintextedit/texteditorlayout.h>
#include <utils/qtcassert.h>
#include <utils/qtdesignwidgets.h>
#include <utils/theme/theme.h>

#include <QtTaskTree/QSingleTaskTreeRunner>

#include <utils/utilsicons.h>

#include <QBrush>
#include <QEnterEvent>
#include <QEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QTextLayout>
#include <QVBoxLayout>

#include <tuple>
#include <QPointer>
#include <QScopeGuard>
#include <QScrollBar>
#include <QSplitter>
#include <QTextDocument>
#include <QTimer>
#include <QToolBar>

using namespace Core;
using namespace TextEditor;
using namespace Utils;

namespace DiffEditor {

namespace {
class InlineDiffEditor;
}

static QHash<TextDocument *, InlineDiffEditor *> &editorRegistry()
{
    static QHash<TextDocument *, InlineDiffEditor *> registry;
    return registry;
}

// changedPositions: <start position, end position>, -1 meaning a continuation
// from the previous or into the next line, see TextLineData
static InlineDiffDecorator::CharRanges toCharRanges(
    const QMap<int, int> &changedPositions, int lineLength)
{
    InlineDiffDecorator::CharRanges result;
    for (auto it = changedPositions.cbegin(); it != changedPositions.cend(); ++it) {
        const int start = qMax(0, it.key());
        const int end = it.value() < 0 ? lineLength : it.value();
        if (end > start)
            result.append({start, end - start});
    }
    return result;
}

InlineDiffRenderModel mapChunkToRenderModel(const ChunkData &chunk,
                                            bool baselineEndsWithNewline,
                                            bool editorEndsWithNewline)
{
    InlineDiffRenderModel model;
    model.baselineEndsWithNewline = baselineEndsWithNewline;
    model.editorEndsWithNewline = editorEndsWithNewline;

    // The "line" after a trailing newline is not a real line. It is the last
    // line of its side, so skip it when it ends up in a non-equal row, paired
    // with real additions or deletions at the end of the file.
    int phantomLeftRow = -1;
    int phantomRightRow = -1;
    for (int i = 0; i < chunk.rows.size(); ++i) {
        if (baselineEndsWithNewline
            && chunk.rows.at(i).line[LeftSide].textLineType == TextLineData::TextLine) {
            phantomLeftRow = i;
        }
        if (editorEndsWithNewline
            && chunk.rows.at(i).line[RightSide].textLineType == TextLineData::TextLine) {
            phantomRightRow = i;
        }
    }

    int leftLine = chunk.startingLineNumber[LeftSide] + 1;   // 1-based baseline line
    int rightLine = chunk.startingLineNumber[RightSide] + 1; // 1-based editor line
    int runStartLeftLine = -1;
    int runStartRightLine = -1;
    int runLeftCount = 0;  // real baseline lines in the run, without the phantom
    int runRightCount = 0; // real editor lines in the run, without the phantom
    InlineDiffDecorator::GhostBlock pendingGhost;
    InlineDiffDecorator::ChangedRange pendingChange;
    bool hasPendingChange = false;

    const auto flushRun = [&] {
        if (runStartRightLine < 0)
            return;
        if (runLeftCount == 0 && runRightCount == 0) {
            // the run consisted of phantom rows only: there is nothing to
            // show, and a hunk for it would offer unactionable buttons
            pendingGhost = {};
            pendingChange = {};
            hasPendingChange = false;
            runStartLeftLine = -1;
            runStartRightLine = -1;
            return;
        }
        if (!pendingGhost.lines.isEmpty()) {
            pendingGhost.anchorLine = runStartRightLine;
            model.ghosts.append(pendingGhost);
        }
        if (hasPendingChange)
            model.changes.append(pendingChange);
        if (runLeftCount > 0) {
            InlineDiffDecorator::ChangedRange baselineRange;
            baselineRange.startLine = runStartLeftLine;
            baselineRange.endLine = runStartLeftLine + runLeftCount - 1;
            for (int i = 0; i < runLeftCount && i < pendingGhost.charHighlights.size(); ++i) {
                if (!pendingGhost.charHighlights.at(i).isEmpty())
                    baselineRange.charHighlights.insert(runStartLeftLine + i,
                                                        pendingGhost.charHighlights.at(i));
            }
            model.baselineChanges.append(baselineRange);
        }
        InlineDiffChunk hunk;
        hunk.editorStartLine = runStartRightLine;
        hunk.editorLineCount = runRightCount;
        hunk.baselineStartLine = runStartLeftLine;
        hunk.baselineLines = pendingGhost.lines;
        model.hunks.append(hunk);
        pendingGhost = {};
        pendingChange = {};
        hasPendingChange = false;
        runStartLeftLine = -1;
        runStartRightLine = -1;
        runLeftCount = 0;
        runRightCount = 0;
    };

    for (int i = 0; i < chunk.rows.size(); ++i) {
        const RowData &row = chunk.rows.at(i);
        if (row.equal) {
            flushRun();
            ++leftLine;
            ++rightLine;
            continue;
        }
        if (runStartRightLine < 0) {
            runStartLeftLine = leftLine;
            runStartRightLine = rightLine;
        }
        const TextLineData &left = row.line[LeftSide];
        const TextLineData &right = row.line[RightSide];
        if (left.textLineType == TextLineData::TextLine) {
            if (i != phantomLeftRow) {
                ++runLeftCount;
                pendingGhost.lines.append(left.text);
                pendingGhost.charHighlights.append(
                    toCharRanges(left.changedPositions, int(left.text.size())));
            }
            ++leftLine;
        }
        if (right.textLineType == TextLineData::TextLine) {
            if (i != phantomRightRow) {
                ++runRightCount;
                if (!hasPendingChange) {
                    pendingChange.startLine = rightLine;
                    hasPendingChange = true;
                }
                pendingChange.endLine = rightLine;
                const InlineDiffDecorator::CharRanges ranges
                    = toCharRanges(right.changedPositions, int(right.text.size()));
                if (!ranges.isEmpty())
                    pendingChange.charHighlights.insert(rightLine, ranges);
            }
            ++rightLine;
        }
    }
    flushRun();
    return model;
}

static void computeRenderModel(QPromise<InlineDiffRenderModel> &promise,
                               const QString &baselineText, const QString &editorText)
{
    if (baselineText == editorText) {
        promise.addResult(InlineDiffRenderModel());
        return;
    }

    Differ differ(QFuture<void>(promise.future()));
    const QList<Diff> diffList = Differ::cleanupSemantics(differ.diff(baselineText, editorText));
    QList<Diff> leftDiffList;
    QList<Diff> rightDiffList;
    Differ::splitDiffList(diffList, &leftDiffList, &rightDiffList);
    const ChunkData chunkData = DiffUtils::calculateOriginalData(leftDiffList, rightDiffList);
    if (promise.isCanceled())
        return;
    promise.addResult(mapChunkToRenderModel(chunkData, baselineText.endsWith('\n'),
                                            editorText.endsWith('\n')));
}

namespace {

// A slim vertical bracket next to the text, marking the lines that a hunk's
// stage/revert buttons apply to. The horizontal end markers are only drawn
// while the respective end of the range is visible.
class HunkRangeBar final : public QWidget
{
public:
    explicit HunkRangeBar(QWidget *parent)
        : QWidget(parent)
    {}

    void setEndMarkers(bool top, bool bottom)
    {
        if (m_topMarker == top && m_bottomMarker == bottom)
            return;
        m_topMarker = top;
        m_bottomMarker = bottom;
        update();
    }

private:
    void paintEvent(QPaintEvent *) override
    {
        constexpr int thickness = Utils::StyleHelper::SpacingTokens::PrimitiveXxs;
        QPainter painter(this);
        const QColor color = Utils::creatorColor(Utils::Theme::Token_Stroke_Muted);
        painter.fillRect(width() - thickness, 0, thickness, height(), color);
        if (m_topMarker)
            painter.fillRect(0, 0, width(), thickness, color);
        if (m_bottomMarker)
            painter.fillRect(0, height() - thickness, width(), thickness, color);
    }

    bool m_topMarker = true;
    bool m_bottomMarker = true;
};

// Floating stage/revert button rows in a reserved band directly left of the
// editable pane's viewport, one per hunk: that is next to the changes in the
// inline view, and in the middle between the panes in the side by side view.
class HunkControls final : public QObject
{
public:
    using HunkAction = std::function<void(const InlineDiffChunk &)>;

    explicit HunkControls(TextEditorWidget *widget)
        : QObject(widget)
        , m_widget(widget)
    {
        // updateRequest also fires on scrolling
        connect(m_widget, &PlainTextEdit::updateRequest, this, [this] { scheduleReposition(); });
        connect(m_widget->textDocument(), &TextDocument::fontSettingsChanged,
                this, [this] { rebuild(); });
        m_widget->viewport()->installEventFilter(this);
    }

    ~HunkControls() override
    {
        // Deliberately no clear() here: the destructor may run during the
        // teardown of the widget, whose internals are already destructed
        // then. The rows and the reserved margin die with the widget.
    }

    void setHunks(const QList<InlineDiffChunk> &hunks, const HunkAction &stage,
                  const HunkAction &revert)
    {
        m_hunks = hunks;
        m_stage = stage;
        m_revert = revert;
        rebuild();
    }

    void clear()
    {
        invalidate();
        if (m_widget)
            m_widget->setEditorTextMargin(kMarginId, Qt::LeftEdge, 0);
    }

    // Drops the rows but keeps the reserved band, e.g. while the hunks' line
    // numbers are stale during an edit. The next setHunks() rebuilds.
    void invalidate()
    {
        m_hunks.clear();
        m_stage = {};
        m_revert = {};
        clearRows();
    }

private:
    static constexpr char kMarginId[] = "DiffEditor.InlineDiff.HunkControls";

    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (m_widget && object == m_widget->viewport() && event->type() == QEvent::Resize)
            reposition();
        return QObject::eventFilter(object, event);
    }

    void clearRows()
    {
        for (const Row &row : std::as_const(m_rows)) {
            if (row.widget) {
                row.widget->hide();
                row.widget->deleteLater(); // an action button may be the caller
            }
            if (row.bar) {
                row.bar->hide();
                row.bar->deleteLater();
            }
        }
        m_rows.clear();
    }

    // (re-)creates the button rows for the current hunks, sized to the
    // current editor font: each button is one text line high, so the
    // controls follow font rescaling
    void rebuild()
    {
        clearRows();
        const int lineHeight = m_widget->fontMetrics().lineSpacing();
        int bandWidth = 0;
        for (const InlineDiffChunk &hunk : std::as_const(m_hunks)) {
            auto row = new QWidget(m_widget);
            auto layout = new QVBoxLayout(row);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(0);
            const auto addButton = [&](const QIcon &icon, const QString &toolTip,
                                       const HunkAction &action) {
                auto button = new Utils::QtcIconButton(row);
                button->setIcon(icon);
                button->setFixedSize(lineHeight, lineHeight);
                button->setToolTip(toolTip);
                connect(button, &QAbstractButton::clicked, this, [this, hunk, action] {
                    // the other rows' hunks are stale after this action until
                    // the diff is recomputed
                    clear();
                    action(hunk);
                });
                layout->addWidget(button);
            };
            if (m_stage)
                addButton(Utils::Icons::PLUS.icon(), Tr::tr("Stage Change"), m_stage);
            if (m_revert)
                addButton(Utils::Icons::RESET.icon(), Tr::tr("Revert Change"), m_revert);
            row->adjustSize();
            bandWidth = qMax(bandWidth, row->width() + kRangeBarGap + kRangeBarWidth);
            m_rows.append({row, new HunkRangeBar(m_widget), hunk});
        }
        m_widget->setEditorTextMargin(kMarginId, Qt::LeftEdge, bandWidth);
        reposition();
    }

    void reposition()
    {
        if (!m_widget)
            return;
        // the rows live in the reserved band between the extra area and the
        // viewport
        const QRect viewportGeometry = m_widget->viewport()->geometry();
        QTextDocument *doc = m_widget->document();
        for (const Row &row : std::as_const(m_rows)) {
            if (!row.widget)
                continue;
            const QTextBlock firstBlock = doc->findBlockByNumber(
                qMin(row.hunk.editorStartLine, doc->blockCount()) - 1);
            if (!firstBlock.isValid()) {
                row.widget->hide();
                continue;
            }
            // the hunk's visual extent, including ghost or spacer rows shown
            // above its first line
            int top = m_widget->cursorRect(QTextCursor(firstBlock)).top()
                      - m_widget->editorLayout()->mainLayoutOffset(firstBlock);
            int bottom = 0;
            if (row.hunk.editorLineCount > 0) {
                const QTextBlock lastBlock = doc->findBlockByNumber(
                    qMin(row.hunk.editorStartLine + row.hunk.editorLineCount - 1,
                         doc->blockCount()) - 1);
                // a cursor at the end of the block, so a word wrapped last line
                // extends the range to its final visual line
                QTextCursor endCursor(lastBlock.isValid() ? lastBlock : firstBlock);
                endCursor.movePosition(QTextCursor::EndOfBlock);
                bottom = m_widget->cursorRect(endCursor).bottom();
            } else {
                // pure removal: the ghost or spacer rows above the anchor line
                bottom = m_widget->cursorRect(QTextCursor(firstBlock)).top();
            }
            const int height = row.widget->height();
            const int visibleTop = qMax(top, 0);
            const int visibleBottom = qMin(bottom, viewportGeometry.height());
            const int centered = (top + bottom - height) / 2;
            int y = 0;
            if (height >= bottom - top) {
                // the row is taller than the hunk (e.g. a single changed
                // line): center it on the hunk, overhanging on both sides,
                // and just keep it inside the viewport
                y = qBound(0, centered, qMax(0, viewportGeometry.height() - height));
            } else {
                // centered on the hunk, but kept within its visible part
                y = qBound(visibleTop, centered, qMax(visibleTop, visibleBottom - height));
            }
            const bool visible = visibleBottom > visibleTop;
            row.widget->move(viewportGeometry.left() - kRangeBarWidth - kRangeBarGap
                                 - row.widget->width(),
                             viewportGeometry.top() + y);
            row.widget->setVisible(visible);
            if (row.bar) {
                // for hunks taller than the button column, mark the lines
                // the buttons apply to
                if (visible && bottom - top > height) {
                    row.bar->setGeometry(viewportGeometry.left() - kRangeBarWidth,
                                         viewportGeometry.top() + visibleTop,
                                         kRangeBarWidth, visibleBottom - visibleTop);
                    // no end markers on ends that are scrolled out
                    static_cast<HunkRangeBar *>(row.bar.data())
                        ->setEndMarkers(top >= 0, bottom <= viewportGeometry.height());
                    row.bar->setVisible(true);
                } else {
                    row.bar->setVisible(false);
                }
            }
        }
    }

    // updateRequest fires on every repaint, scroll, and incremental
    // rehighlight; coalesce the repositions into one per event loop pass so
    // editing does not spend O(edits x hunks x layout) in cursorRect().
    void scheduleReposition()
    {
        if (m_repositionScheduled)
            return;
        m_repositionScheduled = true;
        QMetaObject::invokeMethod(this, [this] {
            m_repositionScheduled = false;
            reposition();
        }, Qt::QueuedConnection);
    }

    class Row
    {
    public:
        QPointer<QWidget> widget;
        QPointer<QWidget> bar;
        InlineDiffChunk hunk;
    };

    // width of the range bracket including its horizontal end markers, and
    // the gap between the buttons and the bracket
    static constexpr int kRangeBarWidth = Utils::StyleHelper::SpacingTokens::PrimitiveS;
    static constexpr int kRangeBarGap = Utils::StyleHelper::SpacingTokens::PrimitiveXxs;

    QPointer<TextEditorWidget> m_widget;
    QList<Row> m_rows;
    QList<InlineDiffChunk> m_hunks;
    HunkAction m_stage;
    HunkAction m_revert;
    bool m_repositionScheduled = false;
};

// unchanged context lines kept visible on each side of a change; runs of
// unchanged lines longer than a placeholder replaces are collapsed
constexpr int kCollapseContextLines = 3;
// do not collapse runs this short: a placeholder would not save any rows
constexpr int kMinCollapsedLines = 2;

// layout items reserving the placeholder rows in a view's own layout
const Utils::Id INLINE_DIFF_COLLAPSE_CATEGORY("DiffEditor.InlineDiff.Collapse");

// One side's changed regions as <1-based start line, line count>; a count of 0
// is a pure insertion/removal that sits above startLine on this side.
using ChangeIntervals = QList<QPair<int, int>>;

// The 1-based line runs (inclusive) that can be collapsed on one side: the
// unchanged lines outside the context around any change, with the last line
// always kept visible so every run has a visible anchor line below it.
static QList<QPair<int, int>> collapsibleRuns(const ChangeIntervals &changes,
                                              int blockCount, int context)
{
    if (blockCount <= 0)
        return {};
    QList<bool> keep(blockCount + 2, false); // 1-based, guarded ends
    const auto keepRange = [&](int from, int to) {
        from = qMax(1, from - context);
        to = qMin(blockCount, to + context);
        for (int line = from; line <= to; ++line)
            keep[line] = true;
    };
    for (const QPair<int, int> &change : changes) {
        if (change.second > 0)
            keepRange(change.first, change.first + change.second - 1);
        else // a pure insertion/removal sits between startLine - 1 and startLine
            keepRange(change.first - 1, change.first);
    }

    QList<QPair<int, int>> runs;
    int line = 1;
    while (line <= blockCount) {
        if (keep[line]) {
            ++line;
            continue;
        }
        const int first = line;
        while (line <= blockCount && !keep[line])
            ++line;
        int last = line - 1;
        if (last == blockCount) // keep the last line as the anchor below the run
            --last;
        if (last >= first && (last - first + 1) >= kMinCollapsedLines)
            runs.append({first, last});
    }
    return runs;
}

// the changed regions of one side of the diff, feeding collapsibleRuns
static ChangeIntervals editorChanges(const InlineDiffRenderModel &model)
{
    ChangeIntervals result;
    for (const InlineDiffChunk &hunk : model.hunks)
        result.append({hunk.editorStartLine, hunk.editorLineCount});
    return result;
}

// Maps an unchanged editor line to the corresponding baseline line. Outside the
// hunks the two sides differ only by a constant offset that steps by each
// passed hunk's (baseline - editor) line delta, so a run collapsed on the editor
// side maps to the exact baseline lines to collapse alongside it. Feeding the
// run's own (unchanged) lines keeps the two sides aligned even for pure
// insertions or deletions, where the sides have different numbers of runs.
static int editorLineToBaseline(const InlineDiffRenderModel &model, int editorLine)
{
    int baselineLine = editorLine;
    for (const InlineDiffChunk &hunk : model.hunks) {
        const int editorAnchor = hunk.editorStartLine + qMax(hunk.editorLineCount, 0);
        if (editorAnchor > editorLine) // hunk is not entirely above the line
            break;
        const int baselineAnchor = hunk.baselineStartLine + int(hunk.baselineLines.size());
        baselineLine = editorLine + (baselineAnchor - editorAnchor);
    }
    return baselineLine;
}

// A clickable, full width placeholder row shown in place of a collapsed run of
// unchanged lines. Clicking it reveals the hidden lines.
class CollapsedRow final : public QWidget
{
public:
    CollapsedRow(QWidget *parent, int hiddenCount, const std::function<void()> &onClick)
        : QWidget(parent)
        , m_hiddenCount(hiddenCount)
        , m_onClick(onClick)
    {
        setObjectName("InlineDiffCollapsedRow"); // found by the autotest
        setCursor(Qt::PointingHandCursor);
        setToolTip(Tr::tr("Show the hidden unchanged lines"));
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.fillRect(rect(), Utils::creatorColor(m_hovered ? Utils::Theme::Token_Background_Subtle
                                                               : Utils::Theme::Token_Background_Muted));
        painter.setPen(Utils::creatorColor(Utils::Theme::Token_Text_Muted));
        painter.setFont(Utils::StyleHelper::uiFont(Utils::StyleHelper::UiElementCaption));
        painter.drawText(rect(), Qt::AlignCenter,
                         Tr::tr("Show %n hidden lines", nullptr, m_hiddenCount));
    }

    void enterEvent(QEnterEvent *) override { m_hovered = true; update(); }
    void leaveEvent(QEvent *) override { m_hovered = false; update(); }
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton && m_onClick)
            m_onClick();
    }

private:
    const int m_hiddenCount;
    const std::function<void()> m_onClick;
    bool m_hovered = false;
};

// Collapses runs of unchanged lines in the inline diff editor, hiding them in
// each view's own layout only - the shared document and any regular editors
// keep showing the full file - and floats a clickable placeholder row over
// each collapsed run that reveals it again. In the side by side view both
// sides are collapsed at their corresponding lines so the aligner keeps the
// two views lined up row by row.
class CollapseController final : public QObject
{
public:
    explicit CollapseController(TextEditorWidget *editor)
        : QObject(editor)
        , m_editor(editor)
    {
        connectView(m_editor);
        m_lastBlockCount = m_editor->document()->blockCount();
        // like the decorator, drop the hidden state on edits that recycle the
        // anchor blocks' fragment indexes, and wait for the next refresh()
        connect(m_editor->document(), &QTextDocument::contentsChange, this,
                [this](int, int charsRemoved, int) {
            if (!m_editor)
                return;
            const int blockCount = m_editor->document()->blockCount();
            if (charsRemoved > 0 && (blockCount < m_lastBlockCount || anchorsRecycled()))
                clearState();
            m_lastBlockCount = blockCount;
        });
    }

    ~CollapseController() override
    {
        // Deliberately no cleanup: the destructor may run during the teardown
        // of the widget, whose layout is already gone. The rows and the hidden
        // state die with the widget.
    }

    // A new diff result or view mode: recompute the collapsed runs. baseline is
    // the read only view to collapse alongside the editor in the side by side
    // view, or nullptr in the inline view.
    void update(const InlineDiffRenderModel &model, TextEditorWidget *baseline)
    {
        m_model = model;
        if (baseline && baseline != m_baseline) {
            m_baseline = baseline;
            connectView(baseline);
        }
        m_baselineActive = baseline != nullptr;
        refresh();
    }

    // the toolbar toggle
    void setEnabled(bool enabled)
    {
        if (m_enabled == enabled)
            return;
        m_enabled = enabled;
        refresh();
    }

    // recompute and apply the collapsed runs for the current model and state
    void refresh()
    {
        clearState();
        if (m_editor && m_enabled)
            collapse();
        relayout();
    }

private:
    // one collapsed run's placeholder within a single view
    class Placeholder
    {
    public:
        QPointer<TextEditorWidget> view;
        int hiddenCount = 0;
        QTextCursor anchor;    // the visible line the placeholder is shown above
        int anchorFragment = -1;
        QPointer<QWidget> row;
    };

    // a collapsed run, paired across both views so revealing it expands both
    class Unit
    {
    public:
        int id = 0;
        Placeholder editor;
        Placeholder baseline; // its view is null in the inline view
    };

    void connectView(TextEditorWidget *view)
    {
        // updateRequest also fires on scrolling
        connect(view, &PlainTextEdit::updateRequest, this, [this] { scheduleReposition(); });
        connect(view->textDocument(), &TextDocument::fontSettingsChanged,
                this, [this] { refresh(); });
        view->viewport()->installEventFilter(this);
    }

    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (event->type() == QEvent::Resize
            && ((m_editor && object == m_editor->viewport())
                || (m_baseline && object == m_baseline->viewport()))) {
            reposition();
        }
        return QObject::eventFilter(object, event);
    }

    bool anchorsRecycled() const
    {
        for (const Unit &unit : m_units) {
            if (unit.editor.anchor.block().fragmentIndex() != unit.editor.anchorFragment)
                return true;
            if (unit.baseline.view
                && unit.baseline.anchor.block().fragmentIndex() != unit.baseline.anchorFragment) {
                return true;
            }
        }
        return false;
    }

    void collapse()
    {
        // with nothing changed there is no focus to keep, so show the full
        // file instead of collapsing it into a single placeholder
        if (m_model.hunks.isEmpty())
            return;
        const QList<QPair<int, int>> editorRuns = collapsibleRuns(
            editorChanges(m_model), m_editor->document()->blockCount(), kCollapseContextLines);

        const bool collapseBaseline = m_baselineActive && m_baseline;
        const int baselineBlockCount = collapseBaseline ? m_baseline->document()->blockCount() : 0;

        for (const QPair<int, int> &run : editorRuns) {
            Unit unit;
            unit.id = ++m_nextUnitId;
            if (collapseBaseline) {
                // a collapsible run is unchanged text, so it maps line for line
                // to the baseline; collapse those same lines there to keep the
                // two sides aligned regardless of how the sides' run counts differ
                const int first = editorLineToBaseline(m_model, run.first);
                const int last = editorLineToBaseline(m_model, run.second);
                if (first < 1 || last < first || last > baselineBlockCount)
                    continue; // out of range: leave this run expanded on both sides
                unit.baseline = makePlaceholder(m_baseline, {first, last}, unit.id);
            }
            unit.editor = makePlaceholder(m_editor, run, unit.id);
            m_units.append(unit);
        }
        reposition();
    }

    Placeholder makePlaceholder(TextEditorWidget *view, const QPair<int, int> &range, int unitId)
    {
        Placeholder placeholder;
        TextEditorLayout *layout = view->editorLayout();
        QTC_ASSERT(layout, return placeholder);
        QTextDocument *doc = view->document();
        const QTextBlock anchor = doc->findBlockByNumber(range.second); // range.second + 1, 0-based
        QTC_ASSERT(anchor.isValid(), return placeholder);
        for (int line = range.first; line <= range.second; ++line) {
            const QTextBlock block = doc->findBlockByNumber(line - 1);
            if (block.isValid())
                layout->setBlockVisibleInEditor(block, false);
        }
        auto item = std::make_unique<Utils::EmptyLayoutItem>(layout->lineSpacing(),
                                                            INLINE_DIFF_COLLAPSE_CATEGORY);
        item->setBackground(
            view->textDocument()->fontSettings().toTextCharFormat(C_LINE_NUMBER).background());
        layout->ensureBlockLayout(anchor);
        layout->prependLayoutItem(anchor, std::move(item));

        placeholder.view = view;
        placeholder.hiddenCount = range.second - range.first + 1;
        placeholder.anchor = QTextCursor(anchor);
        placeholder.anchorFragment = anchor.fragmentIndex();
        placeholder.row = new CollapsedRow(view->viewport(), placeholder.hiddenCount,
                                           [this, unitId] { expandUnit(unitId); });
        return placeholder;
    }

    void expandUnit(int unitId)
    {
        for (int i = 0; i < m_units.size(); ++i) {
            if (m_units.at(i).id != unitId)
                continue;
            expandPlaceholder(m_units.at(i).editor);
            expandPlaceholder(m_units.at(i).baseline);
            m_units.removeAt(i);
            break;
        }
        relayout();
    }

    static void expandPlaceholder(const Placeholder &placeholder)
    {
        TextEditorWidget *view = placeholder.view;
        if (!view)
            return;
        TextEditorLayout *layout = view->editorLayout();
        QTC_ASSERT(layout, return);
        QTextDocument *doc = view->document();
        const QTextBlock anchor = placeholder.anchor.block();
        // the hidden lines are the block above the anchor and its predecessors,
        // as many as were hidden
        const int anchorNumber = anchor.isValid() ? anchor.blockNumber() : doc->blockCount();
        for (int i = 1; i <= placeholder.hiddenCount; ++i) {
            const QTextBlock block = doc->findBlockByNumber(anchorNumber - i);
            if (block.isValid())
                layout->setBlockVisibleInEditor(block, true);
        }
        if (anchor.isValid())
            layout->removeLayoutItems(anchor, INLINE_DIFF_COLLAPSE_CATEGORY);
        if (placeholder.row) {
            placeholder.row->hide();
            placeholder.row->deleteLater(); // the placeholder may be the caller
        }
    }

    void clearState()
    {
        for (const Unit &unit : std::as_const(m_units)) {
            if (unit.editor.row) {
                unit.editor.row->hide();
                unit.editor.row->deleteLater();
            }
            if (unit.baseline.row) {
                unit.baseline.row->hide();
                unit.baseline.row->deleteLater();
            }
        }
        m_units.clear();
        for (TextEditorWidget *view : {m_editor.data(), m_baseline.data()}) {
            if (!view)
                continue;
            if (TextEditorLayout *layout = view->editorLayout()) {
                layout->clearEditorHiddenBlocks();
                layout->removeAllLayoutItems(INLINE_DIFF_COLLAPSE_CATEGORY);
            }
        }
    }

    void relayout()
    {
        for (TextEditorWidget *view : {m_editor.data(), m_baseline.data()}) {
            if (!view)
                continue;
            if (TextEditorLayout *layout = view->editorLayout()) {
                layout->emitDocumentSizeChanged();
                layout->requestUpdate();
            }
        }
    }

    // updateRequest fires on every repaint, scroll, and incremental
    // rehighlight; coalesce the repositions into one per event loop pass.
    void scheduleReposition()
    {
        if (m_repositionScheduled)
            return;
        m_repositionScheduled = true;
        QMetaObject::invokeMethod(this, [this] {
            m_repositionScheduled = false;
            reposition();
        }, Qt::QueuedConnection);
    }

    void reposition()
    {
        for (const Unit &unit : std::as_const(m_units)) {
            repositionPlaceholder(unit.editor);
            repositionPlaceholder(unit.baseline);
        }
    }

    static void repositionPlaceholder(const Placeholder &placeholder)
    {
        TextEditorWidget *view = placeholder.view;
        if (!view || !placeholder.row)
            return;
        TextEditorLayout *layout = view->editorLayout();
        if (!layout)
            return;
        const QTextBlock anchor = placeholder.anchor.block();
        if (!anchor.isValid() || !anchor.isVisible()) {
            placeholder.row->hide();
            return;
        }
        // the placeholder is the topmost item prepended above the anchor
        const int top = view->cursorRect(QTextCursor(anchor)).top()
                        - layout->mainLayoutOffset(anchor);
        const int height = layout->lineSpacing();
        if (top + height < 0 || top > view->viewport()->height()) {
            placeholder.row->hide();
            return;
        }
        const int margin = int(view->document()->documentMargin());
        placeholder.row->setGeometry(margin, top,
                                     qMax(0, view->viewport()->width() - 2 * margin), height);
        placeholder.row->show();
    }

    QPointer<TextEditorWidget> m_editor;
    QPointer<TextEditorWidget> m_baseline;
    QList<Unit> m_units;
    InlineDiffRenderModel m_model;
    bool m_enabled = true;
    bool m_baselineActive = false; // side by side view: collapse the baseline too
    int m_lastBlockCount = 0;
    int m_nextUnitId = 0;
    bool m_repositionScheduled = false;
};

// A thin document for the inline diff editor: it carries the diff title and
// forwards the modified state and saving to the source document, whose text
// buffer the inline diff editor shares.
class InlineDiffDocument final : public Core::IDocument
{
public:
    InlineDiffDocument(const TextDocumentPtr &source, const QString &title)
        : m_source(source)
    {
        setId("DiffEditor.InlineDiffDocument");
        setTemporary(true);
        setPreferredDisplayName(title);
        setMimeType(source->mimeType());
        connect(source.data(), &IDocument::changed, this, &IDocument::changed);
    }

    bool isModified() const override { return m_source->isModified(); }
    bool isSaveAsAllowed() const override { return false; }
    // saving delegates to the source document, no "Save As" needed even
    // though this document has no file path of its own
    bool isSaveAsNeeded() const override { return false; }
    ReloadBehavior reloadBehavior(ChangeTrigger, ChangeType) const override
    {
        return BehaviorSilent;
    }
    Utils::Result<> reload(ReloadFlag, ChangeType) override { return Utils::ResultOk; }

protected:
    Utils::Result<> saveImpl(const Utils::FilePath &filePath, SaveOption option) override
    {
        // DocumentManager::saveDocument makes the file watcher expect the
        // resulting change to the source document's file
        if (DocumentManager::saveDocument(m_source.data(),
                                          filePath.isEmpty() ? m_source->filePath() : filePath,
                                          option)) {
            return Utils::ResultOk;
        }
        return Utils::ResultError(
            Tr::tr("Failed to save \"%1\".").arg(m_source->displayName()));
    }

private:
    const TextDocumentPtr m_source;
};

const char VIEW_MODE_SETTINGS_KEY[] = "DiffEditor/InlineDiffViewMode";
const char COLLAPSE_SETTINGS_KEY[] = "DiffEditor/InlineDiffCollapseUnchanged";

// live diffing on every edit does not scale to arbitrarily large documents
constexpr qsizetype maxInlineDiffTextSize = 8 * 1000 * 1000;

// layout items the side by side aligner installs on both views
const Utils::Id INLINE_DIFF_ALIGN_CATEGORY("DiffEditor.InlineDiff.Align");

// An empty layout item whose height is computed on demand. Used to pad rows so
// that the baseline and the editor view keep the same pixel height per row even
// when lines word wrap differently on the two sides.
class DynamicSpacerItem final : public Utils::LayoutItem
{
public:
    DynamicSpacerItem(std::function<qreal()> heightFn, const Utils::Id &category,
                      const QBrush &background = {})
        : Utils::LayoutItem(category)
        , m_heightFn(std::move(heightFn))
        , m_background(background)
    {}

    qreal height() override { return m_heightFn ? qMax(0.0, m_heightFn()) : 0.0; }

    void paintBackground(QPainter *p, const QPointF &pos, const QRectF &clip) override
    {
        const qreal h = height();
        if (m_background.style() == Qt::NoBrush || h <= 0)
            return;
        QRectF rect(pos, QSizeF(0, h));
        rect.setRight(clip.right());
        p->fillRect(rect, m_background);
    }

private:
    std::function<qreal()> m_heightFn;
    QBrush m_background;
};

// The wrapped pixel height of a block's own text, excluding any additional
// layout items (so measuring one view's rows from the other view's spacers
// does not recurse). Blocks that are not laid out yet report a one line
// estimate, matching how the editor sizes not-yet-laid-out blocks.
static qreal naturalBlockHeight(const QPointer<TextEditorWidget> &widget, int line1)
{
    if (!widget)
        return 0.0;
    const QTextBlock block = widget->document()->findBlockByNumber(line1 - 1);
    if (!block.isValid() || !block.isVisible())
        return 0.0;
    TextEditorLayout *layout = widget->editorLayout();
    if (!layout)
        return 0.0;
    // a line collapsed by the inline diff has no height on its side, so the
    // aligner must not pad the other side for it
    if (!layout->isBlockVisibleInEditor(block))
        return 0.0;
    if (!layout->blockLayoutValid(block.fragmentIndex()))
        return layout->lineSpacing();
    const QTextLayout *tl = layout->existingBlockLayout(block);
    if (!tl || tl->lineCount() == 0)
        return layout->lineSpacing();
    return tl->boundingRect().height();
}

// Keeps the read only baseline view and the editor view of the side by side
// inline diff aligned row by row. Corresponding rows are padded to the taller
// of the two so that both sides have the same pixel height, which lets the
// mirrored pixel based scroll bars line the two views up regardless of word
// wrapping or differing column widths. The pads measure the other side on
// demand, so they follow resizes, splitter drags and font changes without
// recomputation.
class SideBySideAligner final : public QObject
{
public:
    SideBySideAligner(TextEditorWidget *baseline, TextEditorWidget *editor, QObject *parent)
        : QObject(parent)
        , m_baseline(baseline)
        , m_editor(editor)
    {
        const auto watch = [this](TextEditorWidget *changed, TextEditorWidget *other) {
            if (TextEditorLayout *l = changed->editorLayout()) {
                const QPointer<TextEditorWidget> otherPtr(other);
                connect(l, &PlainTextDocumentLayout::documentSizeChanged, this,
                        [this, otherPtr] { invalidate(otherPtr); });
            }
        };
        watch(m_baseline, m_editor);
        watch(m_editor, m_baseline);
    }

    // Deliberately do not clear the layout items here: the aligner is destroyed
    // together with the views during editor teardown, when their layouts are
    // already gone. The items are owned by the layouts and die with them; a
    // recreated aligner clears any leftovers on its next install().
    ~SideBySideAligner() override = default;

    void update(const QList<InlineDiffChunk> &hunks)
    {
        m_matches.clear();
        m_baselineInsertions.clear();
        m_editorInsertions.clear();
        if (!m_baseline || !m_editor)
            return;

        QList<InlineDiffChunk> sorted = hunks;
        // Ties are real: a pure deletion contributes no editor lines, so two
        // hunks can share an editor start line. Ordering by both keeps the walk
        // below from pairing the baseline sides in an arbitrary order.
        Utils::sort(sorted, [](const InlineDiffChunk &a, const InlineDiffChunk &b) {
            return std::tie(a.editorStartLine, a.baselineStartLine)
                   < std::tie(b.editorStartLine, b.baselineStartLine);
        });

        const int baselineLines = m_baseline->document()->blockCount();
        const int editorLines = m_editor->document()->blockCount();
        int bLine = 1;
        int eLine = 1;
        for (const InlineDiffChunk &hunk : std::as_const(sorted)) {
            // 1:1 equal region up to the hunk. The two sides of that region are
            // the same length by construction; if they ever are not, the loop
            // below stops early and the rest of the region silently loses its
            // pads, which shows up as the two views drifting apart.
            QTC_CHECK(hunk.editorStartLine - eLine == hunk.baselineStartLine - bLine);
            while (eLine < hunk.editorStartLine && bLine < hunk.baselineStartLine) {
                m_matches.append({bLine, eLine});
                ++bLine;
                ++eLine;
            }
            bLine = hunk.baselineStartLine;
            eLine = hunk.editorStartLine;

            const int baseCount = int(hunk.baselineLines.size());
            const int editCount = hunk.editorLineCount;
            const int paired = qMin(baseCount, editCount);
            for (int k = 0; k < paired; ++k)
                m_matches.append({hunk.baselineStartLine + k, hunk.editorStartLine + k});
            if (editCount > baseCount) {
                const int anchor = hunk.baselineStartLine + baseCount;
                for (int k = paired; k < editCount; ++k)
                    m_baselineInsertions.append({anchor, hunk.editorStartLine + k});
            } else if (baseCount > editCount) {
                const int anchor = hunk.editorStartLine + editCount;
                for (int k = paired; k < baseCount; ++k)
                    m_editorInsertions.append({anchor, hunk.baselineStartLine + k});
            }
            bLine = hunk.baselineStartLine + baseCount;
            eLine = hunk.editorStartLine + editCount;
        }
        // trailing 1:1 equal region
        while (eLine <= editorLines && bLine <= baselineLines) {
            m_matches.append({bLine, eLine});
            ++bLine;
            ++eLine;
        }
        install();
    }

    void clear()
    {
        m_matches.clear();
        m_baselineInsertions.clear();
        m_editorInsertions.clear();
        clearItems(m_baseline);
        clearItems(m_editor);
        refresh(m_baseline);
        refresh(m_editor);
    }

private:
    void install()
    {
        clearItems(m_baseline);
        clearItems(m_editor);
        if (!m_baseline || !m_editor || !m_baseline->editorLayout() || !m_editor->editorLayout())
            return;

        const QBrush spacerBackground = m_editor->textDocument()->fontSettings()
                                            .toTextCharFormat(C_LINE_NUMBER).background();

        for (const QPair<int, int> &m : std::as_const(m_matches)) {
            addMatchPad(m_baseline, m.first, m_editor, m.second);
            addMatchPad(m_editor, m.second, m_baseline, m.first);
        }
        for (const QPair<int, int> &s : std::as_const(m_baselineInsertions))
            addInsertion(m_baseline, s.first, m_editor, s.second, spacerBackground);
        for (const QPair<int, int> &s : std::as_const(m_editorInsertions))
            addInsertion(m_editor, s.first, m_baseline, s.second, spacerBackground);

        refresh(m_baseline);
        refresh(m_editor);
    }

    // pad the row of 'ownLine' so it matches the taller of the two paired rows
    void addMatchPad(TextEditorWidget *own, int ownLine, TextEditorWidget *other, int otherLine)
    {
        const QTextBlock block = own->document()->findBlockByNumber(ownLine - 1);
        if (!block.isValid())
            return;
        const QPointer<TextEditorWidget> ownPtr(own);
        const QPointer<TextEditorWidget> otherPtr(other);
        auto heightFn = [ownPtr, ownLine, otherPtr, otherLine] {
            return naturalBlockHeight(otherPtr, otherLine) - naturalBlockHeight(ownPtr, ownLine);
        };
        own->editorLayout()->appendLayoutItem(
            block, std::make_unique<DynamicSpacerItem>(heightFn, INLINE_DIFF_ALIGN_CATEGORY));
    }

    // reserve a spacer standing in for a line the other side has but this one
    // does not, as tall as that line wraps on the other side
    void addInsertion(TextEditorWidget *own, int anchorLine, TextEditorWidget *other,
                      int otherLine, const QBrush &background)
    {
        QTextDocument *doc = own->document();
        const bool afterLast = anchorLine > doc->blockCount();
        const QTextBlock block = afterLast ? doc->lastBlock()
                                           : doc->findBlockByNumber(anchorLine - 1);
        if (!block.isValid())
            return;
        const QPointer<TextEditorWidget> otherPtr(other);
        auto heightFn = [otherPtr, otherLine] { return naturalBlockHeight(otherPtr, otherLine); };
        auto item = std::make_unique<DynamicSpacerItem>(heightFn, INLINE_DIFF_ALIGN_CATEGORY,
                                                        background);
        if (afterLast)
            own->editorLayout()->appendLayoutItem(block, std::move(item));
        else
            own->editorLayout()->prependLayoutItem(block, std::move(item));
    }

    static void clearItems(TextEditorWidget *widget)
    {
        if (widget) {
            if (TextEditorLayout *l = widget->editorLayout())
                l->removeAllLayoutItems(INLINE_DIFF_ALIGN_CATEGORY);
        }
    }

    void refresh(TextEditorWidget *widget)
    {
        if (!widget)
            return;
        if (TextEditorLayout *l = widget->editorLayout()) {
            l->emitDocumentSizeChanged();
            l->requestUpdate();
        }
    }

    // one side's rows just changed height (resize, font change, edit); the
    // other side's pads measure it, so drop its cached offsets and repaint
    void invalidate(const QPointer<TextEditorWidget> &widget)
    {
        if (m_syncing || !widget)
            return;
        TextEditorLayout *l = widget->editorLayout();
        if (!l)
            return;
        m_syncing = true;
        l->resetBlockSize(widget->document()->firstBlock());
        l->emitDocumentSizeChanged();
        l->requestUpdate();
        m_syncing = false;
    }

    QPointer<TextEditorWidget> m_baseline;
    QPointer<TextEditorWidget> m_editor;
    QList<QPair<int, int>> m_matches;            // (baselineLine, editorLine)
    QList<QPair<int, int>> m_baselineInsertions; // (anchorBaselineLine, editorLine)
    QList<QPair<int, int>> m_editorInsertions;   // (anchorEditorLine, baselineLine)
    bool m_syncing = false;
};

class InlineDiffEditor final : public Core::IEditor
{
public:
    InlineDiffEditor(const TextDocumentPtr &source, const InlineDiffBaseline &baseline,
                     const QString &title, bool readOnlySource)
        : m_source(source)
        , m_document(new InlineDiffDocument(source, title))
        , m_splitter(new QSplitter)
        , m_widget(new TextEditorWidget)
    {
        editorRegistry().insert(source.data(), this);
        m_document->setParent(this);

        setContext(Core::Context("DiffEditor.InlineDiffEditor"));
        m_widget->setTextDocument(source);
        if (readOnlySource) {
            m_widget->setReadOnly(true);
            m_widget->setupGenericHighlighter();
        } else {
            m_hunkControls = new HunkControls(m_widget);
        }
        m_splitter->setChildrenCollapsible(false);
        m_splitter->addWidget(m_widget);
        setWidget(m_splitter);
        m_decorator = new InlineDiffDecorator(m_widget);
        m_collapseController = new CollapseController(m_widget);

        m_toolBar = new QToolBar;
        // like the diff editor's view switcher, the icon shows the view that
        // a click switches to
        m_viewSwitcherAction = m_toolBar->addAction(QIcon(), QString());
        connect(m_viewSwitcherAction, &QAction::triggered, this, [this] {
            setViewMode(m_viewMode == InlineDiffViewMode::Inline
                            ? InlineDiffViewMode::SideBySide
                            : InlineDiffViewMode::Inline);
        });

        const bool collapse = Core::ICore::settings()
                                  ->value(COLLAPSE_SETTINGS_KEY, true).toBool();
        m_collapseAction = m_toolBar->addAction(Utils::Icons::COLLAPSE_TOOLBAR.icon(),
                                                Tr::tr("Hide Unchanged Lines"));
        m_collapseAction->setCheckable(true);
        m_collapseAction->setChecked(collapse);
        m_collapseAction->setToolTip(Tr::tr("Hide unchanged lines, keeping some context "
                                            "around each change."));
        m_collapseController->setEnabled(collapse);
        connect(m_collapseAction, &QAction::toggled, this, [this](bool on) {
            Core::ICore::settings()->setValue(COLLAPSE_SETTINGS_KEY, on);
            if (m_collapseController)
                m_collapseController->setEnabled(on);
        });

        m_updateTimer.setSingleShot(true);
        m_updateTimer.setInterval(500);
        connect(&m_updateTimer, &QTimer::timeout, this, &InlineDiffEditor::startUpdate);
        connect(source->document(), &QTextDocument::contentsChanged, this, [this] {
            // the hunks' line numbers are stale until the diff is recomputed
            if (m_hunkControls)
                m_hunkControls->invalidate();
            m_updateTimer.start();
        });

        // saving may change what the baseline refers to (e.g. a "diff against
        // the saved file" baseline), so refresh it
        connect(source.data(), &Core::IDocument::saved, this, [this] { fetchBaseline(); });

        // the inline diff editor is a companion of the source document; close
        // it once the document is closed
        connect(EditorManager::instance(), &EditorManager::documentClosed,
                this, [this](Core::IDocument *document) {
            if (document != m_source.data())
                return;
            QMetaObject::invokeMethod(this, [this] {
                EditorManager::closeEditors({this}, false);
            }, Qt::QueuedConnection);
        });

        const auto isBaselineAffected = [this](const Utils::FilePath &repository) {
            const Utils::FilePath &contextDirectory = m_baseline.contextDirectory;
            return !contextDirectory.isEmpty()
                   && (contextDirectory == repository || contextDirectory.isChildOf(repository));
        };
        connect(VcsManager::instance(), &VcsManager::repositoryChanged,
                this, [this, isBaselineAffected](const Utils::FilePath &repository) {
            if (isBaselineAffected(repository))
                fetchBaseline();
        });
        connect(VcsManager::instance(), &VcsManager::updateFileState, this,
                [this, isBaselineAffected](const Utils::FilePath &repository, const QStringList &) {
            if (isBaselineAffected(repository))
                fetchBaseline();
        });

        setBaseline(baseline, title);
        const int storedViewMode = Core::ICore::settings()
                                       ->value(VIEW_MODE_SETTINGS_KEY,
                                               int(InlineDiffViewMode::Inline))
                                       .toInt();
        setViewMode(storedViewMode == int(InlineDiffViewMode::SideBySide)
                        ? InlineDiffViewMode::SideBySide
                        : InlineDiffViewMode::Inline);
    }

    ~InlineDiffEditor() override
    {
        editorRegistry().remove(m_source.data());
        delete m_splitter.data(); // deletes the decorators, which must not clear()
        delete m_toolBar.data();
    }

    void setBaseline(const InlineDiffBaseline &baseline, const QString &title)
    {
        QTC_ASSERT(baseline.isValid(), return);
        m_baseline = baseline;
        m_baselineText.reset();
        m_document->setPreferredDisplayName(title);
        if (m_baselineWidget) {
            // recreate the baseline view, it may carry baseline specific
            // attachments like revision annotations
            delete m_baselineWidget.data();
            m_baselineDocument.reset();
            if (m_viewMode == InlineDiffViewMode::SideBySide) {
                ensureBaselineView();
                m_baselineWidget->show();
            }
        }
        fetchBaseline();
    }

    InlineDiffViewMode viewMode() const { return m_viewMode; }
    TextEditorWidget *editorWidget() const { return m_widget; }

    void setViewMode(InlineDiffViewMode mode)
    {
        m_viewMode = mode;
        if (mode == InlineDiffViewMode::SideBySide) {
            ensureBaselineView();
            m_baselineWidget->show();
        } else if (m_baselineWidget) {
            m_baselineWidget->hide();
        }
        const bool isInline = mode == InlineDiffViewMode::Inline;
        m_viewSwitcherAction->setIcon(
            (isInline ? Icons::SIDEBYSIDE_DIFF : Icons::UNIFIED_DIFF).icon());
        const QString switchText = isInline ? Tr::tr("Switch to Side by Side Diff View")
                                            : Tr::tr("Switch to Inline Diff View");
        m_viewSwitcherAction->setToolTip(switchText);
        m_viewSwitcherAction->setText(switchText);
        Core::ICore::settings()->setValue(VIEW_MODE_SETTINGS_KEY, int(mode));
        applyDecorations();
        if (mode == InlineDiffViewMode::SideBySide && m_baselineWidget) {
            // catch up on scrolling that happened while the mirror was off
            m_baselineWidget->verticalScrollBar()->setValue(
                m_widget->verticalScrollBar()->value());
        }
    }

    Core::IDocument *document() const override { return m_document; }
    QWidget *toolBar() override { return m_toolBar; }

    int currentLine() const override { return m_widget->textCursor().blockNumber() + 1; }
    int currentColumn() const override { return m_widget->textCursor().positionInBlock(); }
    void gotoLine(int line, int column, bool centerLine) override
    {
        m_widget->gotoLine(line, column, centerLine);
        // decorations arriving later insert rows above the line and push it
        // away, so re-center once when the next diff result is applied
        m_centerOnNextModel = centerLine;
    }

private:
    void fetchBaseline()
    {
        const int requestId = ++m_baselineRequestId;
        m_baseline.fetchText([guard = QPointer<InlineDiffEditor>(this),
                              requestId](const Result<QString> &result) {
            if (!guard || guard->m_baselineRequestId != requestId)
                return;
            const Utils::Id infoId("DiffEditor.InlineDiff.BaselineError");
            guard->m_document->infoBar()->removeInfo(infoId);
            if (result) {
                QString text = *result;
                text.replace("\r\n", "\n");
                guard->m_baselineText = text;
                guard->updateBaselineDocument();
                guard->startUpdate();
            } else {
                // an empty diff would wrongly suggest that there are no
                // differences, so say why there is nothing to show
                guard->m_document->infoBar()->addInfo(Utils::InfoBarEntry(
                    infoId,
                    Tr::tr("The diff baseline is not available: %1")
                        .arg(result.error().trimmed())));
                guard->m_baselineText.reset();
                guard->updateBaselineDocument();
                guard->applyModel({});
            }
        });
    }

    void ensureBaselineView()
    {
        if (m_baselineWidget)
            return;
        m_baselineDocument = TextDocumentPtr(new TextEditor::TextDocument);
        m_baselineDocument->setMimeType(m_source->mimeType());
        m_baselineWidget = new TextEditorWidget;
        m_baselineWidget->setTextDocument(m_baselineDocument);
        m_baselineWidget->setReadOnly(true);
        m_baselineWidget->setupGenericHighlighter();
        m_baselineDecorator = new InlineDiffDecorator(m_baselineWidget,
                                                      InlineDiffDecorator::DiffSide::Baseline);
        m_splitter->insertWidget(0, m_baselineWidget);
        updateBaselineDocument();
        if (m_baseline.setupBaselineView)
            m_baseline.setupBaselineView(m_baselineWidget);

        // The aligner keeps both sides at the same pixel height per row, and
        // the vertical scroll bars work on pixel offsets, so mirroring the
        // values keeps the views aligned. Only mirror while side by side: the
        // hidden baseline view of the inline mode has a smaller range and
        // would bounce back a clamped value.
        m_aligner = new SideBySideAligner(m_baselineWidget, m_widget, m_baselineWidget);
        const auto syncScrollBars = [this](TextEditorWidget *from, TextEditorWidget *to) {
            connect(from->verticalScrollBar(), &QAbstractSlider::valueChanged,
                    to->verticalScrollBar(), [this, to](int value) {
                if (m_viewMode == InlineDiffViewMode::SideBySide)
                    to->verticalScrollBar()->setValue(value);
            });
        };
        syncScrollBars(m_baselineWidget, m_widget);
        syncScrollBars(m_widget, m_baselineWidget);
    }

    void updateBaselineDocument()
    {
        if (!m_baselineWidget)
            return;
        const QString text = m_baselineText.value_or(QString());
        if (m_baselineDocument->plainText() != text)
            m_baselineDocument->document()->setPlainText(text);
    }

    void startUpdate()
    {
        if (!m_baselineText)
            return; // still fetching, an update is started once the baseline arrived
        m_updateTimer.stop();

        const QString editorText = m_source->plainText();
        if (m_baselineText->size() > maxInlineDiffTextSize
            || editorText.size() > maxInlineDiffTextSize) {
            applyModel({});
            return;
        }

        using namespace QtTaskTree;
        const auto onSetup = [baselineText = *m_baselineText,
                              editorText](Async<InlineDiffRenderModel> &async) {
            async.setConcurrentCallData(computeRenderModel, baselineText, editorText);
        };
        const auto onDone = [this](const Async<InlineDiffRenderModel> &async) {
            applyModel(async.isResultAvailable() ? async.result() : InlineDiffRenderModel());
        };
        m_taskTreeRunner.start(
            {AsyncTask<InlineDiffRenderModel>(onSetup, onDone, CallDoneFlag::OnSuccess)});
    }

    void applyModel(const InlineDiffRenderModel &model)
    {
        m_model = model;
        applyDecorations();
        if (m_centerOnNextModel) {
            m_centerOnNextModel = false;
            m_widget->centerCursor();
        }
    }

    void applyDecorations()
    {
        if (!m_decorator)
            return;
        if (m_viewMode == InlineDiffViewMode::Inline) {
            m_decorator->apply(m_model.ghosts, m_model.changes);
            if (m_baselineDecorator)
                m_baselineDecorator->clear();
            if (m_aligner)
                m_aligner->clear();
        } else {
            // the per-row height alignment of the two views is handled by the
            // aligner; the decorators only add the change highlights
            m_decorator->apply({}, m_model.changes);
            if (m_baselineDecorator) {
                m_baselineDecorator->apply({}, m_model.baselineChanges);
            }
            if (m_aligner)
                m_aligner->update(m_model.hunks);
        }
        updateHunkControls();
        // the collapser runs last so its placeholder rows sit above any ghost
        // rows the decorator prepended on the same anchor line; in the side by
        // side view it also collapses the baseline so the aligner stays in sync
        if (m_collapseController) {
            TextEditorWidget *baseline = m_viewMode == InlineDiffViewMode::SideBySide
                                             ? m_baselineWidget.data()
                                             : nullptr;
            m_collapseController->update(m_model, baseline);
        }
    }

    static bool hunkIsActionable(const InlineDiffChunk &hunk, const InlineDiffLineRanges &ranges)
    {
        const int first = hunk.editorStartLine;
        const int last = first + qMax(hunk.editorLineCount, 1) - 1;
        for (const auto &range : ranges) {
            if (range.first <= last && first <= range.second)
                return true;
        }
        return false;
    }

    void updateHunkControls()
    {
        if (!m_hunkControls)
            return;
        const int requestId = ++m_actionableRequestId;
        if (m_baseline.fetchActionableLines && !m_model.hunks.isEmpty()) {
            m_hunkControls->setHunks({}, {}, {});
            m_baseline.fetchActionableLines(
                m_source->plainText(),
                [guard = QPointer<InlineDiffEditor>(this),
                 requestId](const InlineDiffLineRanges &ranges) {
                    if (!guard || guard->m_actionableRequestId != requestId)
                        return;
                    QList<InlineDiffChunk> actionable;
                    for (const InlineDiffChunk &hunk : std::as_const(guard->m_model.hunks)) {
                        if (hunkIsActionable(hunk, ranges))
                            actionable.append(hunk);
                    }
                    guard->applyHunkControls(actionable);
                });
        } else {
            applyHunkControls(m_model.hunks);
        }
    }

    void applyHunkControls(const QList<InlineDiffChunk> &hunks)
    {
        if (!m_hunkControls)
            return;
        // both actions force a recompute, which rebuilds the controls even
        // when the action itself did not change anything
        HunkControls::HunkAction stage;
        if (m_baseline.stageHunk) {
            stage = [this](const InlineDiffChunk &hunk) {
                m_baseline.stageHunk(hunk, m_source->plainText());
                m_updateTimer.start();
            };
        }
        const auto revert = [this](const InlineDiffChunk &hunk) {
            revertHunk(hunk);
            m_updateTimer.start();
        };
        m_hunkControls->setHunks(hunks, stage, revert);
    }

    void revertHunk(const InlineDiffChunk &hunk)
    {
        QTextDocument *doc = m_source->document();
        // hunks at the end of the file implicitly cover the trailing newline
        // state, which has no visible line of its own
        const bool touchesEnd = hunk.editorStartLine + qMax(hunk.editorLineCount, 1) - 1
                                >= doc->blockCount();
        QString replacement = hunk.baselineLines.join('\n');
        QTextCursor cursor(doc);
        cursor.beginEditBlock();
        const QScopeGuard endEditBlock([&cursor, &doc, touchesEnd, this] {
            if (touchesEnd)
                alignTrailingNewline(doc);
            cursor.endEditBlock();
        });
        if (hunk.editorLineCount > 0) {
            const QTextBlock first = doc->findBlockByNumber(hunk.editorStartLine - 1);
            const QTextBlock last = doc->findBlockByNumber(hunk.editorStartLine - 1
                                                           + hunk.editorLineCount - 1);
            QTC_ASSERT(first.isValid() && last.isValid(), return);
            cursor.setPosition(first.position());
            if (last.next().isValid()) {
                // include the trailing newline of the hunk
                cursor.setPosition(last.next().position(), QTextCursor::KeepAnchor);
                if (!replacement.isEmpty())
                    replacement += '\n';
            } else if (replacement.isEmpty() && first.previous().isValid()) {
                // removing the last lines removes the preceding newline, too
                const QTextBlock previous = first.previous();
                cursor.setPosition(previous.position() + previous.length() - 1);
                cursor.setPosition(last.position() + qMax(0, last.length() - 1),
                                   QTextCursor::KeepAnchor);
            } else {
                cursor.setPosition(last.position() + qMax(0, last.length() - 1),
                                   QTextCursor::KeepAnchor);
            }
            cursor.insertText(replacement);
        } else if (!replacement.isEmpty()) {
            // pure removal: re-insert the baseline lines above the anchor
            if (hunk.editorStartLine > doc->blockCount()) {
                cursor.movePosition(QTextCursor::End);
                cursor.insertText('\n' + replacement);
            } else {
                const QTextBlock block = doc->findBlockByNumber(hunk.editorStartLine - 1);
                QTC_ASSERT(block.isValid(), return);
                cursor.setPosition(block.position());
                cursor.insertText(replacement + '\n');
            }
        }
    }

    // makes reverts of hunks at the end of the file converge on the
    // baseline's trailing newline state
    void alignTrailingNewline(QTextDocument *doc)
    {
        const bool docEndsWithNewline = doc->blockCount() > 1
                                        && doc->lastBlock().text().isEmpty();
        if (docEndsWithNewline == m_model.baselineEndsWithNewline)
            return;
        QTextCursor cursor(doc);
        cursor.movePosition(QTextCursor::End);
        if (m_model.baselineEndsWithNewline) {
            cursor.insertText("\n");
        } else if (cursor.position() > 0) {
            cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
        }
    }

    const TextDocumentPtr m_source;
    InlineDiffDocument *m_document = nullptr;
    QPointer<QSplitter> m_splitter;
    QPointer<TextEditorWidget> m_widget;
    QPointer<InlineDiffDecorator> m_decorator;
    QPointer<CollapseController> m_collapseController;
    QPointer<HunkControls> m_hunkControls;
    QPointer<TextEditorWidget> m_baselineWidget;
    QPointer<InlineDiffDecorator> m_baselineDecorator;
    QPointer<SideBySideAligner> m_aligner;
    TextDocumentPtr m_baselineDocument;
    QPointer<QToolBar> m_toolBar;
    QAction *m_viewSwitcherAction = nullptr;
    QAction *m_collapseAction = nullptr;
    InlineDiffViewMode m_viewMode = InlineDiffViewMode::Inline;
    bool m_centerOnNextModel = false;
    InlineDiffBaseline m_baseline;
    std::optional<QString> m_baselineText;
    InlineDiffRenderModel m_model;
    QTimer m_updateTimer;
    QtTaskTree::QSingleTaskTreeRunner m_taskTreeRunner;
    int m_baselineRequestId = 0;
    int m_actionableRequestId = 0;
};

} // anonymous namespace

Core::IEditor *openInlineDiffEditor(const TextDocumentPtr &sourceDocument,
                                    const InlineDiffBaseline &baseline,
                                    const QString &title,
                                    bool readOnlySource)
{
    QTC_ASSERT(sourceDocument, return nullptr);
    QTC_ASSERT(baseline.isValid(), return nullptr);
    if (sourceDocument->document()->characterCount() > maxInlineDiffTextSize)
        return nullptr; // too large for live diffing, let the caller fall back
    if (InlineDiffEditor *editor = editorRegistry().value(sourceDocument.data())) {
        editor->setBaseline(baseline, title);
        EditorManager::activateEditor(editor);
        return editor;
    }
    auto editor = new InlineDiffEditor(sourceDocument, baseline, title, readOnlySource);
    EditorManager::addEditor(editor);
    return editor;
}

static InlineDiffEditor *inlineDiffEditor(Core::IEditor *editor)
{
    for (InlineDiffEditor *candidate : std::as_const(editorRegistry())) {
        if (candidate == editor)
            return candidate;
    }
    return nullptr;
}

TextEditorWidget *inlineDiffEditorWidget(Core::IEditor *editor)
{
    if (InlineDiffEditor *diffEditor = inlineDiffEditor(editor))
        return diffEditor->editorWidget();
    return nullptr;
}

void setInlineDiffViewMode(Core::IEditor *editor, InlineDiffViewMode mode)
{
    if (InlineDiffEditor *diffEditor = inlineDiffEditor(editor))
        diffEditor->setViewMode(mode);
}

InlineDiffViewMode inlineDiffViewMode(Core::IEditor *editor)
{
    if (InlineDiffEditor *diffEditor = inlineDiffEditor(editor))
        return diffEditor->viewMode();
    return InlineDiffViewMode::Inline;
}

} // namespace DiffEditor
