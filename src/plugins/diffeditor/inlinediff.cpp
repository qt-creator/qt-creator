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
#include <utils/qtcassert.h>

#include <QtTaskTree/QSingleTaskTreeRunner>

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

class InlineDiffEditor final : public Core::IEditor
{
public:
    InlineDiffEditor(const TextDocumentPtr &source, const InlineDiffBaseline &baseline,
                     const QString &title)
        : m_source(source)
        , m_document(new InlineDiffDocument(source, title))
        , m_splitter(new QSplitter)
        , m_widget(new TextEditorWidget)
    {
        editorRegistry().insert(source.data(), this);
        m_document->setParent(this);

        setContext(Core::Context("DiffEditor.InlineDiffEditor"));
        m_widget->setTextDocument(source);
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
        fetchBaseline();
    }

    InlineDiffViewMode viewMode() const { return m_viewMode; }

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
        constexpr qsizetype maxTextSize = 8 * 1000 * 1000;
        if (m_baselineText->size() > maxTextSize || editorText.size() > maxTextSize) {
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
    }

    const TextDocumentPtr m_source;
    InlineDiffDocument *m_document = nullptr;
    QPointer<QSplitter> m_splitter;
    QPointer<TextEditorWidget> m_widget;
    QPointer<InlineDiffDecorator> m_decorator;
    QPointer<TextEditorWidget> m_baselineWidget;
    QPointer<InlineDiffDecorator> m_baselineDecorator;
    TextDocumentPtr m_baselineDocument;
    QPointer<QToolBar> m_toolBar;
    QAction *m_viewSwitcherAction = nullptr;
    InlineDiffViewMode m_viewMode = InlineDiffViewMode::Inline;
    InlineDiffBaseline m_baseline;
    std::optional<QString> m_baselineText;
    InlineDiffRenderModel m_model;
    QTimer m_updateTimer;
    QtTaskTree::QSingleTaskTreeRunner m_taskTreeRunner;
    int m_baselineRequestId = 0;
};

} // anonymous namespace

Core::IEditor *openInlineDiffEditor(const TextDocumentPtr &sourceDocument,
                                    const InlineDiffBaseline &baseline,
                                    const QString &title)
{
    QTC_ASSERT(sourceDocument, return nullptr);
    QTC_ASSERT(baseline.isValid(), return nullptr);
    if (InlineDiffEditor *editor = editorRegistry().value(sourceDocument.data())) {
        editor->setBaseline(baseline, title);
        EditorManager::activateEditor(editor);
        return editor;
    }
    auto editor = new InlineDiffEditor(sourceDocument, baseline, title);
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
