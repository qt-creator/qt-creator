// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "disassembleragent.h"

#include "breakhandler.h"
#include "debuggeractions.h"
#include "debuggerengine.h"
#include "debuggerinternalconstants.h"
#include "debuggersourcepathmappingwidget.h"
#include "debuggertr.h"
#include "disassemblerlines.h"
#include "sourceutils.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>

#include <texteditor/fontsettings.h>
#include <texteditor/textmark.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/algorithm.h>
#include <utils/link.h>
#include <utils/macroexpander.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>

#include <QMouseEvent>
#include <QTextBlock>

using namespace Core;
using namespace TextEditor;
using namespace Utils;

namespace Debugger::Internal {

const char LinkedLinesSelection[] = "Debugger.Disassembler.LinkedLines";

///////////////////////////////////////////////////////////////////////
//
// DisassemblerBreakpointMarker
//
///////////////////////////////////////////////////////////////////////

// The red blob on the left side in the cpp editor.
class DisassemblerBreakpointMarker : public TextMark
{
public:
    DisassemblerBreakpointMarker(const Breakpoint &bp, int lineNumber)
        : TextMark(Utils::FilePath(), lineNumber, {Tr::tr("Breakpoint"), Constants::TEXT_MARK_CATEGORY_BREAKPOINT})
        , m_bp(bp)
    {
        setIcon(bp->icon());
        setPriority(TextMark::NormalPriority);
    }

    bool isClickable() const final { return true; }

    void clicked() final
    {
        QTC_ASSERT(m_bp, return);
        m_bp->deleteGlobalOrThisBreakpoint();
    }

public:
    Breakpoint m_bp;
};

///////////////////////////////////////////////////////////////////////
//
// FrameKey
//
///////////////////////////////////////////////////////////////////////

class FrameKey
{
public:
    FrameKey() = default;
    inline bool matches(const Location &loc) const;

    QString functionName;
    QString fileName;
    quint64 startAddress = 0;
    quint64 endAddress = 0;
};

bool FrameKey::matches(const Location &loc) const
{
    return loc.address() >= startAddress
            && loc.address() <= endAddress
            && loc.fileName().toUrlishString() == fileName
            && loc.functionName() == functionName;
}

using CacheEntry = QPair<FrameKey, DisassemblerLines>;


///////////////////////////////////////////////////////////////////////
//
// DisassemblerAgentPrivate
//
///////////////////////////////////////////////////////////////////////

class DisassemblerAgentPrivate
{
public:
    DisassemblerAgentPrivate(DebuggerEngine *engine);
    ~DisassemblerAgentPrivate();
    void configureMimeType();
    int lineForAddress(quint64 address) const;
    FilePath sourceFile(const QString &fileName) const;
    void updateSourceForLine(const DisassemblerLines &contents);
    void trackEditor(IEditor *editor);
    std::optional<Link> sourceAt(TextEditorWidget *widget, int line) const;
    void linkFromCursor();
    void linkFromMouse(TextEditorWidget *widget, const QPoint &pos);
    QList<int> instructionLines(const Link &source) const;
    void highlight(const Link &source);
    void setLinkedLines(IDocument *document, const QList<int> &lines) const;
    void highlightEditor(IEditor *editor) const;

public:
    QPointer<TextDocument> document;
    Location location;
    QPointer<DebuggerEngine> engine;
    LocationMark locationMark;
    QList<DisassemblerBreakpointMarker *> breakpointMarks;
    QList<CacheEntry> cache;
    QString mimeType;
    bool resetLocationScheduled;
    QList<Link> sourceForLine; // Indexed by the line in the document, starting at 0.
    QHash<FilePath, QSet<int>> sourceLines;
    QList<QPair<QString, QString>> sourcePathMap;
    Link highlightedSource;
    Id linkedLinesId;
    QPointer<TextEditorWidget> trackedWidget;
    QMetaObject::Connection cursorConnection;
    bool hovering = false;
    bool closing = false;
};

DisassemblerAgentPrivate::DisassemblerAgentPrivate(DebuggerEngine *engine)
  : document(nullptr),
    engine(engine),
    locationMark(engine, Utils::FilePath(), 0),
    mimeType("text/x-qtcreator-generic-asm"),
    resetLocationScheduled(false)
{
    static int agentCount = 0;
    linkedLinesId = Id(LinkedLinesSelection).withSuffix(++agentCount);
}

DisassemblerAgentPrivate::~DisassemblerAgentPrivate()
{
    QObject::disconnect(cursorConnection);
    highlight({});
    if (document)
        EditorManager::closeDocuments({document});
    document = nullptr;
    qDeleteAll(breakpointMarks);
}

int DisassemblerAgentPrivate::lineForAddress(quint64 address) const
{
    for (int i = 0, n = cache.size(); i != n; ++i) {
        const CacheEntry &entry = cache.at(i);
        if (entry.first.matches(location))
            return entry.second.lineForAddress(address);
    }
    return 0;
}

// The debuggers name the source file of a line in the form the debug information
// has it, which may be relative, and which the source path mapping has not been
// applied to.
FilePath DisassemblerAgentPrivate::sourceFile(const QString &fileName) const
{
    const FilePath locationFile = location.fileName();
    if (fileName.isEmpty())
        return locationFile;
    const FilePath file = FilePath::fromUserInput(fileName);
    if (file.isAbsolutePath())
        return locationFile.withNewPath(mappedSourcePath(sourcePathMap, file.path(), true));
    if (locationFile.fileName() == file.path() || locationFile.path().endsWith('/' + file.path()))
        return locationFile;
    return locationFile.parentDir().resolvePath(file.path());
}

void DisassemblerAgentPrivate::updateSourceForLine(const DisassemblerLines &contents)
{
    sourceForLine.clear();
    sourceForLine.reserve(contents.size());
    sourceLines.clear();

    // The same mapping as the one the engines pass to the debugger.
    sourcePathMap.clear();
    if (engine) {
        const DebuggerRunParameters &rp = engine->runParameters();
        const SourcePathMap map = mergeStartParametersSourcePathMap(
            rp, mergePlatformQtPath(rp, settings().sourcePathMap()));
        for (auto it = map.cbegin(), end = map.cend(); it != end; ++it) {
            sourcePathMap.append(
                {it.key(), rp.macroExpander() ? rp.macroExpander()->expand(it.value()) : it.value()});
        }
    }

    Link source;
    for (int i = 0, n = contents.size(); i != n; ++i) {
        const DisassemblerLine &line = contents.at(i);
        // An instruction belongs to the source line above it.
        if (!line.isAssembler()) {
            source = line.isCode() ? Link(sourceFile(line.fileName), line.lineNumber) : Link();
            if (source.hasValidTarget())
                sourceLines[source.targetFilePath].insert(source.target.line);
        }
        sourceForLine.append(source);
    }
}

void DisassemblerAgentPrivate::trackEditor(IEditor *editor)
{
    QObject::disconnect(cursorConnection);
    TextEditorWidget *widget = editor ? TextEditorWidget::fromEditor(editor) : nullptr;
    trackedWidget = widget;
    if (widget) {
        cursorConnection = QObject::connect(widget, &TextEditorWidget::cursorPositionChanged,
                                            widget, [this] { linkFromCursor(); });
    }
    linkFromCursor();
}

// The source line that \a line of \a widget stands for, an invalid link for a
// line standing for none, or nothing if \a widget shows neither the disassembly
// nor a file it was compiled from.
std::optional<Link> DisassemblerAgentPrivate::sourceAt(TextEditorWidget *widget, int line) const
{
    if (widget->textDocument() == document)
        return sourceForLine.value(line);
    const FilePath file = widget->textDocument()->filePath();
    const auto lines = sourceLines.constFind(file);
    if (lines == sourceLines.constEnd())
        return {};
    return lines->contains(line + 1) ? Link(file, line + 1) : Link();
}

// Lets the line the cursor is on in the disassembly or in its source pick the
// lines on the other side that belong to it.
void DisassemblerAgentPrivate::linkFromCursor()
{
    hovering = false;
    if (!trackedWidget || !document || !settings().showSourceBesideDisassembly()) {
        highlight({});
        return;
    }
    highlight(sourceAt(trackedWidget, trackedWidget->textCursor().blockNumber()).value_or(Link()));
}

// As linkFromCursor(), for the line under the mouse, which takes precedence
// while it is over the disassembly or its source.
void DisassemblerAgentPrivate::linkFromMouse(TextEditorWidget *widget, const QPoint &pos)
{
    if (!document || !settings().showSourceBesideDisassembly())
        return;
    if (widget->textDocument() != document
            && !sourceLines.contains(widget->textDocument()->filePath())) {
        return;
    }
    const std::optional<Link> source
        = sourceAt(widget, widget->cursorForPosition(pos).blockNumber());
    if (!source)
        return;
    hovering = true;
    highlight(*source);
}

static QList<QTextEdit::ExtraSelection> linkedLineSelections(TextDocument *textDocument,
                                                              const QList<int> &lines)
{
    QTextCharFormat format = globalFontSettings().data().toTextCharFormat(C_OCCURRENCES);
    format.setProperty(QTextFormat::FullWidthSelection, true);
    QList<QTextEdit::ExtraSelection> selections;
    for (const int line : lines) {
        const QTextBlock block = textDocument->document()->findBlockByNumber(line);
        if (!block.isValid())
            continue;
        QTextEdit::ExtraSelection selection;
        selection.cursor = QTextCursor(block);
        // Taking in the line break makes the selection span the full width.
        if (!selection.cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor))
            selection.cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        selection.format = format;
        selections.append(selection);
    }
    return selections;
}

void DisassemblerAgentPrivate::setLinkedLines(IDocument *document, const QList<int> &lines) const
{
    auto textDocument = qobject_cast<TextDocument *>(document);
    if (!textDocument)
        return;
    const QList<QTextEdit::ExtraSelection> selections = linkedLineSelections(textDocument, lines);
    for (IEditor *editor : DocumentModel::editorsForDocument(document)) {
        if (TextEditorWidget *widget = TextEditorWidget::fromEditor(editor))
            widget->setExtraSelections(linkedLinesId, selections);
    }
}

QList<int> DisassemblerAgentPrivate::instructionLines(const Link &source) const
{
    QList<int> lines;
    for (int i = 0, n = sourceForLine.size(); i != n; ++i) {
        if (sourceForLine.at(i) == source)
            lines.append(i);
    }
    return lines;
}

void DisassemblerAgentPrivate::highlight(const Link &source)
{
    if (source == highlightedSource)
        return;
    if (highlightedSource.hasValidTarget()) {
        setLinkedLines(DocumentModel::documentForFilePath(highlightedSource.targetFilePath), {});
        setLinkedLines(document, {});
    }
    highlightedSource = source;
    if (!source.hasValidTarget())
        return;

    setLinkedLines(DocumentModel::documentForFilePath(source.targetFilePath),
                   {source.target.line - 1});
    setLinkedLines(document, instructionLines(source));
}

// Gives an editor opened after the highlight was set the lines it shows.
void DisassemblerAgentPrivate::highlightEditor(IEditor *editor) const
{
    if (!highlightedSource.hasValidTarget())
        return;
    TextEditorWidget *widget = TextEditorWidget::fromEditor(editor);
    if (!widget)
        return;
    TextDocument *textDocument = widget->textDocument();
    QList<int> lines;
    if (textDocument == document)
        lines = instructionLines(highlightedSource);
    else if (textDocument->filePath() == highlightedSource.targetFilePath)
        lines = {highlightedSource.target.line - 1};
    else
        return;
    widget->setExtraSelections(linkedLinesId, linkedLineSelections(textDocument, lines));
}

///////////////////////////////////////////////////////////////////////
//
// DisassemblerAgent
//
///////////////////////////////////////////////////////////////////////

/*!
    \class Debugger::Internal::DisassemblerAgent

     Objects from this class are created in response to user actions in
     the Gui for showing disassembled memory from the inferior. After creation
     it handles communication between the engine and the editor.
*/

DisassemblerAgent::DisassemblerAgent(DebuggerEngine *engine)
    : d(new DisassemblerAgentPrivate(engine))
{
    connect(&settings().intelFlavor, &Utils::BaseAspect::changed,
            this, &DisassemblerAgent::reload);
    connect(&settings().showSourceBesideDisassembly, &Utils::BaseAspect::changed,
            this, [this] { d->linkFromCursor(); });
    connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
            this, [this](IEditor *editor) { d->trackEditor(editor); });

    const auto watchMouse = [this](IEditor *editor) {
        if (TextEditorWidget *widget = TextEditorWidget::fromEditor(editor))
            widget->viewport()->installEventFilter(this);
    };
    for (IEditor *editor : DocumentModel::editorsForOpenedDocuments())
        watchMouse(editor);
    connect(EditorManager::instance(), &EditorManager::editorCreated, this, watchMouse);
    connect(EditorManager::instance(), &EditorManager::editorOpened,
            this, [this](IEditor *editor) { d->highlightEditor(editor); });
}

bool DisassemblerAgent::eventFilter(QObject *watched, QEvent *event)
{
    if (!d || d->closing)
        return false;
    if (event->type() == QEvent::MouseMove) {
        if (auto widget = qobject_cast<TextEditorWidget *>(watched->parent()))
            d->linkFromMouse(widget, static_cast<QMouseEvent *>(event)->position().toPoint());
    } else if (event->type() == QEvent::Leave && d->hovering) {
        d->linkFromCursor();
    }
    return false;
}

DisassemblerAgent::~DisassemblerAgent()
{
    // Closing the document makes another editor the current one, and moves the
    // mouse off it.
    disconnect(EditorManager::instance(), nullptr, this, nullptr);
    d->closing = true;
    delete d;
    d = nullptr;
}

int DisassemblerAgent::indexOf(const Location &loc) const
{
    for (int i = 0; i < d->cache.size(); i++)
        if (d->cache.at(i).first.matches(loc))
            return i;
    return -1;
}

void DisassemblerAgent::cleanup()
{
    d->cache.clear();
}

void DisassemblerAgent::scheduleResetLocation()
{
    d->resetLocationScheduled = true;
}

void DisassemblerAgent::resetLocation()
{
    if (!d->document)
        return;
    if (d->resetLocationScheduled) {
        d->resetLocationScheduled = false;
        d->document->removeMark(&d->locationMark);
    }
}

const Location &DisassemblerAgent::location() const
{
    return d->location;
}

void DisassemblerAgent::reload()
{
    d->cache.clear();
    d->engine->fetchDisassembler(this);
}

void DisassemblerAgent::setLocation(const Location &loc)
{
    d->location = loc;
    int index = indexOf(loc);
    if (index != -1) {
        // Refresh when not displaying a function and there is not sufficient
        // context left past the address.
        if (d->cache.at(index).first.endAddress - loc.address() < 24) {
            d->cache.removeAt(index);
            index = -1;
        }
    }
    if (index != -1) {
        const FrameKey &key = d->cache.at(index).first;
        const QString msg =
            QString("Using cached disassembly for 0x%1 (0x%2-0x%3) in \"%4\"/ \"%5\"")
                .arg(loc.address(), 0, 16)
                .arg(key.startAddress, 0, 16).arg(key.endAddress, 0, 16)
                .arg(loc.functionName(), loc.fileName().toUserOutput());
        d->engine->showMessage(msg);
        setContentsToDocument(d->cache.at(index).second);
        d->resetLocationScheduled = false; // In case reset from previous run still pending.
    } else {
        d->engine->fetchDisassembler(this);
    }
}

void DisassemblerAgentPrivate::configureMimeType()
{
    QTC_ASSERT(document, return);

    document->setMimeType(mimeType);

    Utils::MimeType mtype = Utils::mimeTypeForName(mimeType);
    if (mtype.isValid()) {
        const QList<IEditor *> editors = DocumentModel::editorsForDocument(document);
        for (IEditor *editor : editors)
            if (auto widget = TextEditorWidget::fromEditor(editor))
                widget->configureGenericHighlighter();
    } else {
        qWarning("Assembler mimetype '%s' not found.", qPrintable(mimeType));
    }
}

QString DisassemblerAgent::mimeType() const
{
    return d->mimeType;
}

void DisassemblerAgent::setMimeType(const QString &mt)
{
    if (mt == d->mimeType)
        return;
    d->mimeType = mt;
    if (d->document)
       d->configureMimeType();
}

// A pathological disassembly - e.g. accidentally requested for a frame without
// symbols so that it ends up spanning a whole module - can contain millions of
// lines. Materializing that as a single plain-text string plus a text document
// uses huge amounts of memory. Keep only a bounded window around the current
// instruction. See QTCREATORBUG-9786.
const int MaxDisassemblyLines = 50000;

static DisassemblerLines cappedDisassembly(const DisassemblerLines &all, quint64 address)
{
    const int total = all.size();
    if (total <= MaxDisassemblyLines)
        return all;

    const int centerLine = all.lineForAddress(address); // 1-based, 0 if unknown
    const int center = centerLine > 0 ? centerLine - 1 : 0;
    int from = qMax(0, center - MaxDisassemblyLines / 2);
    const int to = qMin(total, from + MaxDisassemblyLines);
    from = qMax(0, to - MaxDisassemblyLines);

    DisassemblerLines capped;
    if (from > 0)
        capped.appendComment(Tr::tr("<%1 preceding lines not shown, disassembly truncated>").arg(from));
    for (int i = from; i < to; ++i)
        capped.appendLine(all.at(i));
    if (to < total)
        capped.appendComment(Tr::tr("<%1 following lines not shown, disassembly truncated>").arg(total - to));
    return capped;
}

void DisassemblerAgent::setContents(const DisassemblerLines &rawContents)
{
    QTC_ASSERT(d, return);
    const DisassemblerLines contents = cappedDisassembly(rawContents, d->location.address());
    if (contents.size()) {
        const quint64 startAddress = contents.startAddress();
        const quint64 endAddress = contents.endAddress();
        if (startAddress) {
            FrameKey key;
            key.fileName = d->location.fileName().toUrlishString();
            key.functionName = d->location.functionName();
            key.startAddress = startAddress;
            key.endAddress = endAddress;
            d->cache.append(CacheEntry(key, contents));
        }
    }
    if (contents.size() == 0) {
        // Whatever the reason - an address nothing is mapped at, a symbol the
        // debugger cannot reach - an empty view says less than the one that
        // is already there.
        d->engine->showMessage(Tr::tr("No disassembly for \"%1\".")
                                   .arg(d->location.functionName().isEmpty()
                                            ? QString::number(d->location.address(), 16)
                                            : d->location.functionName()),
                               LogWarning);
        return;
    }
    setContentsToDocument(contents);
}

void DisassemblerAgent::setContentsToDocument(const DisassemblerLines &contents)
{
    QTC_ASSERT(d, return);
    const bool besideSource = settings().showSourceBesideDisassembly();
    if (!d->document) {
        EditorManager::OpenEditorFlags flags;
        const FilePath sourceFile = d->location.fileName();
        IEditor *sourceEditor = besideSource && !sourceFile.isEmpty()
            ? Utils::findOrDefault(EditorManager::visibleEditors(), [sourceFile](IEditor *e) {
                  return e->document()->filePath() == sourceFile;
              })
            : nullptr;
        if (sourceEditor) {
            const int sourceViewId = EditorManager::viewIdForEditor(sourceEditor);
            if (EditorManager::otherViewId(sourceViewId) == 0)
                EditorManager::splitView(sourceViewId, Qt::Horizontal, EditorManager::LeaveEmpty);
            EditorManager::activateEditor(sourceEditor);
            flags |= EditorManager::OpenInOtherSplit;
        }
        QString titlePattern = "Disassembler";
        IEditor *editor = EditorManager::openEditorWithContents(
                Core::Constants::K_DEFAULT_TEXT_EDITOR_ID,
                &titlePattern, {}, {}, flags);
        QTC_ASSERT(editor, return);
        if (auto widget = TextEditorWidget::fromEditor(editor)) {
            widget->setReadOnly(true);
            widget->setRequestMarkEnabled(true);
        }
        d->document = qobject_cast<TextDocument *>(editor->document());
        QTC_ASSERT(d->document, return);
        d->document->setTemporary(true);
        // FIXME: This is accumulating quite a bit out-of-band data.
        // Make that a proper TextDocument reimplementation.
        d->document->setProperty(Debugger::Constants::OPENED_BY_DEBUGGER, true);
        d->document->setProperty(Debugger::Constants::OPENED_WITH_DISASSEMBLY, true);
        d->document->setProperty(Debugger::Constants::DISASSEMBLER_SOURCE_FILE, d->location.fileName().toUrlishString());
        d->configureMimeType();
    } else if (besideSource) {
        // In its own split, not in the one the source was just shown in.
        const QList<IEditor *> editors = DocumentModel::editorsForDocument(d->document);
        if (QTC_GUARD(!editors.isEmpty()))
            EditorManager::activateEditor(editors.first());
    } else {
        EditorManager::activateEditorForDocument(d->document);
    }

    // Stepping within a function shows the same text again, and replacing it
    // would redo its layout and highlighting.
    const QString text = contents.toString();
    const bool changed = text != d->document->plainText();
    if (changed)
        d->highlight({});
    d->updateSourceForLine(contents);
    if (changed)
        d->document->setPlainText(text);

    d->document->setPreferredDisplayName(QString("Disassembler (%1)")
        .arg(d->location.functionName()));

    const Breakpoints bps = d->engine->breakHandler()->breakpoints();
    for (const Breakpoint &bp : bps)
        updateBreakpointMarker(bp);

    updateLocationMarker();
}

void DisassemblerAgent::updateLocationMarker()
{
    if (!d->document)
        return;

    const int lineNumber = d->lineForAddress(d->location.address());
    if (d->location.needsMarker()) {
        d->document->removeMark(&d->locationMark);
        // A disassembly that does not cover the address has no line standing
        // for it, and the line before the first is not a line.
        if (lineNumber >= 1) {
            d->locationMark.updateLineNumber(lineNumber);
            d->document->addMark(&d->locationMark);
        }
    }

    d->locationMark.updateIcon();

    // Center cursor.
    if (lineNumber >= 1 && EditorManager::currentDocument() == d->document) {
        if (auto textEditor = BaseTextEditor::currentTextEditor())
            textEditor->gotoLine(lineNumber);
    }
}

void DisassemblerAgent::removeBreakpointMarker(const Breakpoint &bp)
{
    if (!d->document)
        return;

    for (DisassemblerBreakpointMarker *marker : std::as_const(d->breakpointMarks)) {
        if (marker->m_bp == bp) {
            d->breakpointMarks.removeOne(marker);
            d->document->removeMark(marker);
            delete marker;
            return;
        }
    }
}

void DisassemblerAgent::updateBreakpointMarker(const Breakpoint &bp)
{
    removeBreakpointMarker(bp);
    const quint64 address = bp->address();
    if (!address)
        return;

    int lineNumber = d->lineForAddress(address);
    if (!lineNumber)
        return;

    // HACK: If it's a FileAndLine breakpoint, and there's a source line
    // above, move the marker up there. That allows setting and removing
    // normal breakpoints from within the disassembler view.
    if (bp->type() == BreakpointByFileAndLine) {
        ContextData context = getLocationContext(d->document, lineNumber - 1);
        if (context.type == LocationByFile)
            --lineNumber;
    }

    auto marker = new DisassemblerBreakpointMarker(bp, lineNumber);
    d->breakpointMarks.append(marker);
    QTC_ASSERT(d->document, return);
    d->document->addMark(marker);
}

quint64 DisassemblerAgent::address() const
{
    return d->location.address();
}

/*!
    Returns the kind of the extra selections that mark the lines of the
    disassembly and of its source that belong together.
*/
Id DisassemblerAgent::linkedLinesSelection() const
{
    return d->linkedLinesId;
}

/*!
    Returns the split to show \a sourceFile beside the disassembly in: one
    that shows it already, or else the one next to the disassembly, splitting
    the one the disassembly is in if it is the only one. Returns 0 if the
    disassembly is not shown.
*/
int DisassemblerAgent::sourceViewId(const FilePath &sourceFile)
{
    if (!d->document)
        return 0;
    const QList<IEditor *> disassemblyEditors = DocumentModel::editorsForDocument(d->document);
    const QList<int> disassemblyViewIds
        = Utils::transform(disassemblyEditors, &EditorManager::viewIdForEditor);
    for (IEditor *editor : EditorManager::visibleEditors()) {
        if (editor->document()->filePath() != sourceFile)
            continue;
        const int viewId = EditorManager::viewIdForEditor(editor);
        if (viewId != 0 && !disassemblyViewIds.contains(viewId))
            return viewId;
    }
    for (IEditor *editor : disassemblyEditors) {
        const int viewId = EditorManager::viewIdForEditor(editor);
        if (viewId == 0)
            continue;
        if (const int otherViewId = EditorManager::otherViewId(viewId))
            return otherViewId;
        return EditorManager::splitView(viewId, Qt::Horizontal, EditorManager::LeaveEmpty);
    }
    return 0;
}

} // Debugger::Internal
