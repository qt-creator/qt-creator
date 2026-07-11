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

#include <texteditor/texteditor.h>

#include <utils/async.h>
#include <utils/differ.h>
#include <utils/plaintextedit/texteditorlayout.h>
#include <utils/qtcassert.h>
#include <utils/qtdesignwidgets.h>
#include <utils/theme/theme.h>

#include <QtTaskTree/QSingleTaskTreeRunner>

#include <utils/utilsicons.h>

#include <QEvent>
#include <QPainter>
#include <QVBoxLayout>
#include <QPointer>
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
        // align the side with fewer lines by an empty spacer after its lines
        if (runRightCount > runLeftCount) {
            model.baselineSpacers.append(
                {runStartLeftLine + runLeftCount, runRightCount - runLeftCount});
        } else if (runLeftCount > runRightCount) {
            model.editorSpacers.append(
                {runStartRightLine + runRightCount, runLeftCount - runRightCount});
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
        connect(m_widget->verticalScrollBar(), &QAbstractSlider::valueChanged,
                this, [this] { reposition(); });
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
        m_hunks.clear();
        m_stage = {};
        m_revert = {};
        clearRows();
        if (m_widget)
            m_widget->setEditorTextMargin(kMarginId, Qt::LeftEdge, 0);
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
                bottom = m_widget
                             ->cursorRect(QTextCursor(lastBlock.isValid() ? lastBlock
                                                                          : firstBlock))
                             .bottom();
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

// live diffing on every edit does not scale to arbitrarily large documents
constexpr qsizetype maxInlineDiffTextSize = 8 * 1000 * 1000;

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

        m_toolBar = new QToolBar;
        // like the diff editor's view switcher, the icon shows the view that
        // a click switches to
        m_viewSwitcherAction = m_toolBar->addAction(QIcon(), QString());
        connect(m_viewSwitcherAction, &QAction::triggered, this, [this] {
            setViewMode(m_viewMode == InlineDiffViewMode::Inline
                            ? InlineDiffViewMode::SideBySide
                            : InlineDiffViewMode::Inline);
        });

        m_updateTimer.setSingleShot(true);
        m_updateTimer.setInterval(500);
        connect(&m_updateTimer, &QTimer::timeout, this, &InlineDiffEditor::startUpdate);
        connect(source->document(), &QTextDocument::contentsChanged,
                this, [this] { m_updateTimer.start(); });

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
        setViewMode(InlineDiffViewMode(
            Core::ICore::settings()
                ->value(VIEW_MODE_SETTINGS_KEY, int(InlineDiffViewMode::Inline))
                .toInt()));
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
            if (result) {
                QString text = *result;
                text.replace("\r\n", "\n");
                guard->m_baselineText = text;
                guard->updateBaselineDocument();
                guard->startUpdate();
            } else {
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

        // Both sides have the same pixel height thanks to the alignment
        // spacers, and the vertical scroll bars work on pixel offsets, so
        // mirroring the values keeps the views aligned.
        const auto syncScrollBars = [](TextEditorWidget *from, TextEditorWidget *to) {
            connect(from->verticalScrollBar(), &QAbstractSlider::valueChanged,
                    to->verticalScrollBar(), &QAbstractSlider::setValue);
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
        } else {
            m_decorator->apply({}, m_model.changes, m_model.editorSpacers);
            if (m_baselineDecorator) {
                m_baselineDecorator->apply({}, m_model.baselineChanges,
                                           m_model.baselineSpacers);
            }
        }
        updateHunkControls();
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
        HunkControls::HunkAction stage;
        if (m_baseline.stageHunk) {
            stage = [this](const InlineDiffChunk &hunk) {
                m_baseline.stageHunk(hunk, m_source->plainText());
            };
        }
        const auto revert = [this](const InlineDiffChunk &hunk) { revertHunk(hunk); };
        m_hunkControls->setHunks(hunks, stage, revert);
    }

    void revertHunk(const InlineDiffChunk &hunk)
    {
        QTextDocument *doc = m_source->document();
        QString replacement = hunk.baselineLines.join('\n');
        QTextCursor cursor(doc);
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

    const TextDocumentPtr m_source;
    InlineDiffDocument *m_document = nullptr;
    QPointer<QSplitter> m_splitter;
    QPointer<TextEditorWidget> m_widget;
    QPointer<InlineDiffDecorator> m_decorator;
    QPointer<HunkControls> m_hunkControls;
    QPointer<TextEditorWidget> m_baselineWidget;
    QPointer<InlineDiffDecorator> m_baselineDecorator;
    TextDocumentPtr m_baselineDocument;
    QPointer<QToolBar> m_toolBar;
    QAction *m_viewSwitcherAction = nullptr;
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
