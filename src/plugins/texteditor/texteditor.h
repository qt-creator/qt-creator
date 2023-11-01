// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include "codeassist/assistenums.h"
#include "indenter.h"
#include "refactoroverlay.h"
#include "snippets/snippetparser.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/helpitem.h>

#include <utils/elidinglabel.h>
#include <utils/link.h>
#include <utils/multitextcursor.h>
#include <utils/uncommentselection.h>

#include <QPlainTextEdit>
#include <QSharedPointer>

#include <functional>
#include <memory>

QT_BEGIN_NAMESPACE
class QToolBar;
class QPrinter;
class QMenu;
class QPainter;
class QPoint;
class QRect;
class QTextBlock;
QT_END_NAMESPACE

namespace Core {
class HighlightScrollBarController;
}

namespace TextEditor {
class AssistInterface;
class BaseHoverHandler;
class CompletionAssistProvider;
class IAssistProvider;
class ICodeStylePreferences;
class RefactorOverlay;
class SyntaxHighlighter;
class TextDocument;
class TextMark;
class TextSuggestion;
using RefactorMarkers = QList<RefactorMarker>;
using TextMarks = QList<TextMark *>;

namespace Internal {
class BaseTextEditorPrivate;
class TextEditorFactoryPrivate;
class TextEditorWidgetPrivate;
class TextEditorOverlay;
}

class AutoCompleter;
class BaseTextEditor;
class TextEditorFactory;
class TextEditorWidget;
class PlainTextEditorFactory;

class BehaviorSettings;
class CompletionSettings;
class DisplaySettings;
class ExtraEncodingSettings;
class FontSettings;
class MarginSettings;
class StorageSettings;
class TypingSettings;

enum TextMarkRequestKind
{
    BreakpointRequest,
    BookmarkRequest,
    TaskMarkRequest
};

class TEXTEDITOR_EXPORT BaseTextEditor : public Core::IEditor
{
    Q_OBJECT

public:
    BaseTextEditor();
    ~BaseTextEditor() override;

    virtual void finalizeInitialization() {}

    static BaseTextEditor *currentTextEditor();
    static QVector<BaseTextEditor *> textEditorsForDocument(TextDocument *textDocument);

    TextEditorWidget *editorWidget() const;
    TextDocument *textDocument() const;

    // Some convenience text access
    void setTextCursor(const QTextCursor &cursor);
    QTextCursor textCursor() const;
    QChar characterAt(int pos) const;
    QString textAt(int from, int to) const;

    void addContext(Utils::Id id);

    // IEditor
    Core::IDocument *document() const override;

    BaseTextEditor *duplicate() override;

    QByteArray saveState() const override;
    void restoreState(const QByteArray &state) override;
    QWidget *toolBar() override;

    void contextHelp(const HelpCallback &callback) const override; // from IContext
    void setContextHelp(const Core::HelpItem &item) override;

    int currentLine() const override;
    int currentColumn() const override;
    void gotoLine(int line, int column = 0, bool centerLine = true) override;

    /*! Returns the amount of visible columns (in characters) in the editor */
    int columnCount() const;

    /*! Returns the amount of visible lines (in characters) in the editor */
    int rowCount() const;

    /*! Returns the position at \a posOp in characters from the beginning of the document */
    virtual int position(TextPositionOperation posOp = CurrentPosition, int at = -1) const;

    /*! Converts the \a pos in characters from beginning of document to \a line and \a column */
    virtual void convertPosition(int pos, int *line, int *column) const;

    virtual QString selectedText() const;

    /*! Removes \a length characters to the right of the cursor. */
    virtual void remove(int length);
    /*! Inserts the given string to the right of the cursor. */
    virtual void insert(const QString &string);
    /*! Replaces \a length characters to the right of the cursor with the given string. */
    virtual void replace(int length, const QString &string);
    /*! Sets current cursor position to \a pos. */
    virtual void setCursorPosition(int pos);
    /*! Selects text between current cursor position and \a toPos. */
    virtual void select(int toPos);

private:
    friend class TextEditorFactory;
    friend class Internal::TextEditorFactoryPrivate;
    Internal::BaseTextEditorPrivate *d;
};

class TEXTEDITOR_EXPORT TextEditorWidget : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit TextEditorWidget(QWidget *parent = nullptr);
    ~TextEditorWidget() override;

    void setTextDocument(const QSharedPointer<TextDocument> &doc);
    TextDocument *textDocument() const;
    QSharedPointer<TextDocument> textDocumentPtr() const;

    virtual void aboutToOpen(const Utils::FilePath &filePath, const Utils::FilePath &realFilePath);
    virtual void openFinishedSuccessfully();
    // IEditor
    QByteArray saveState() const;
    virtual void restoreState(const QByteArray &state);
    void gotoLine(int line, int column = 0, bool centerLine = true, bool animate = false);
    int position(TextPositionOperation posOp = CurrentPosition,
         int at = -1) const;
    void convertPosition(int pos, int *line, int *column) const;
    using QPlainTextEdit::cursorRect;
    QRect cursorRect(int pos) const;
    void setCursorPosition(int pos);
    QToolBar *toolBar();

    void print(QPrinter *);

    void appendStandardContextMenuActions(QMenu *menu);

    uint optionalActions();
    void setOptionalActions(uint optionalActions);
    void addOptionalActions(uint optionalActions);

    void setAutoCompleter(AutoCompleter *autoCompleter);
    AutoCompleter *autoCompleter() const;

    // Works only in conjunction with a syntax highlighter that puts
    // parentheses into text block user data
    void setParenthesesMatchingEnabled(bool b);
    bool isParenthesesMatchingEnabled() const;

    void setHighlightCurrentLine(bool b);
    bool highlightCurrentLine() const;

    void setLineNumbersVisible(bool b);
    bool lineNumbersVisible() const;

    void setAlwaysOpenLinksInNextSplit(bool b);
    bool alwaysOpenLinksInNextSplit() const;

    void setMarksVisible(bool b);
    bool marksVisible() const;

    void setRequestMarkEnabled(bool b);
    bool requestMarkEnabled() const;

    void setLineSeparatorsAllowed(bool b);
    bool lineSeparatorsAllowed() const;

    bool codeFoldingVisible() const;

    void setCodeFoldingSupported(bool b);
    bool codeFoldingSupported() const;

    void setMouseNavigationEnabled(bool b);
    bool mouseNavigationEnabled() const;

    void setMouseHidingEnabled(bool b);
    bool mouseHidingEnabled() const;

    void setScrollWheelZoomingEnabled(bool b);
    bool scrollWheelZoomingEnabled() const;

    void setConstrainTooltips(bool b);
    bool constrainTooltips() const;

    void setCamelCaseNavigationEnabled(bool b);
    bool camelCaseNavigationEnabled() const;

    void setRevisionsVisible(bool b);
    bool revisionsVisible() const;

    void setVisibleWrapColumn(int column);
    int visibleWrapColumn() const;

    int columnCount() const;
    int rowCount() const;

    void setReadOnly(bool b);

    void insertCodeSnippet(const QTextCursor &cursor,
                           const QString &snippet,
                           const SnippetParser &parse);

    Utils::MultiTextCursor multiTextCursor() const;
    void setMultiTextCursor(const Utils::MultiTextCursor &cursor);

    QRegion translatedLineRegion(int lineStart, int lineEnd) const;

    QPoint toolTipPosition(const QTextCursor &c) const;
    void showTextMarksToolTip(const QPoint &pos,
                              const TextMarks &marks,
                              const TextMark *mainTextMark = nullptr) const;

    void invokeAssist(AssistKind assistKind, IAssistProvider *provider = nullptr);

    virtual std::unique_ptr<AssistInterface> createAssistInterface(AssistKind assistKind,
                                                                   AssistReason assistReason) const;
    static QMimeData *duplicateMimeData(const QMimeData *source);

    static QString msgTextTooLarge(quint64 size);

    void insertPlainText(const QString &text);

    QWidget *extraArea() const;
    virtual int extraAreaWidth(int *markWidthPtr = nullptr) const;
    virtual void extraAreaPaintEvent(QPaintEvent *);
    virtual void extraAreaLeaveEvent(QEvent *);
    virtual void extraAreaContextMenuEvent(QContextMenuEvent *);
    virtual void extraAreaMouseEvent(QMouseEvent *);
    void updateFoldingHighlight(const QPoint &pos);

    void setLanguageSettingsId(Utils::Id settingsId);
    Utils::Id languageSettingsId() const;

    void setCodeStyle(ICodeStylePreferences *settings);

    const DisplaySettings &displaySettings() const;
    const MarginSettings &marginSettings() const;
    const BehaviorSettings &behaviorSettings() const;

    void ensureCursorVisible();
    void ensureBlockIsUnfolded(QTextBlock block);

    static Utils::Id FakeVimSelection;
    static Utils::Id SnippetPlaceholderSelection;
    static Utils::Id CurrentLineSelection;
    static Utils::Id ParenthesesMatchingSelection;
    static Utils::Id AutoCompleteSelection;
    static Utils::Id CodeWarningsSelection;
    static Utils::Id CodeSemanticsSelection;
    static Utils::Id CursorSelection;
    static Utils::Id UndefinedSymbolSelection;
    static Utils::Id UnusedSymbolSelection;
    static Utils::Id OtherSelection;
    static Utils::Id ObjCSelection;
    static Utils::Id DebuggerExceptionSelection;

    void setExtraSelections(Utils::Id kind, const QList<QTextEdit::ExtraSelection> &selections);
    QList<QTextEdit::ExtraSelection> extraSelections(Utils::Id kind) const;
    QString extraSelectionTooltip(int pos) const;

    RefactorMarkers refactorMarkers() const;
    void setRefactorMarkers(const RefactorMarkers &markers);
    void setRefactorMarkers(const RefactorMarkers &markers, const Utils::Id &type);
    void clearRefactorMarkers(const Utils::Id &type);

    enum Side { Left, Right };
    QAction *insertExtraToolBarWidget(Side side, QWidget *widget);
    void setToolbarOutline(QWidget* widget);
    const QWidget *toolbarOutlineWidget();

    // keep the auto completion even if the focus is lost
    void keepAutoCompletionHighlight(bool keepHighlight);
    void setAutoCompleteSkipPosition(const QTextCursor &cursor);

    virtual void copy();
    virtual void paste();
    virtual void cut();
    virtual void selectAll();

    virtual void autoIndent();
    virtual void rewrapParagraph();
    virtual void unCommentSelection();

    virtual void autoFormat();

    virtual void encourageApply();

    virtual void setDisplaySettings(const TextEditor::DisplaySettings &);
    virtual void setMarginSettings(const TextEditor::MarginSettings &);
    void setBehaviorSettings(const TextEditor::BehaviorSettings &);
    void setTypingSettings(const TextEditor::TypingSettings &);
    void setStorageSettings(const TextEditor::StorageSettings &);
    void setCompletionSettings(const TextEditor::CompletionSettings &);
    void setExtraEncodingSettings(const TextEditor::ExtraEncodingSettings &);

    void circularPaste();
    void pasteWithoutFormat();
    void switchUtf8bom();

    void zoomF(float delta);
    void zoomReset();

    void cutLine();
    void copyLine();
    void copyWithHtml();
    void duplicateSelection();
    void duplicateSelectionAndComment();
    void deleteLine();
    void deleteEndOfLine();
    void deleteEndOfWord();
    void deleteEndOfWordCamelCase();
    void deleteStartOfLine();
    void deleteStartOfWord();
    void deleteStartOfWordCamelCase();
    void unfoldAll();
    void fold(const QTextBlock &block);
    void foldCurrentBlock();
    void unfold(const QTextBlock &block);
    void unfoldCurrentBlock();
    void selectEncoding();
    void updateTextCodecLabel();
    void selectLineEnding(Utils::TextFileFormat::LineTerminationMode lineEnding);
    void updateTextLineEndingLabel();
    void addSelectionNextFindMatch();
    void addCursorsToLineEnds();

    void gotoBlockStart();
    void gotoBlockEnd();
    void gotoBlockStartWithSelection();
    void gotoBlockEndWithSelection();

    void gotoDocumentStart();
    void gotoDocumentEnd();
    void gotoLineStart();
    void gotoLineStartWithSelection();
    void gotoLineEnd();
    void gotoLineEndWithSelection();
    void gotoNextLine();
    void gotoNextLineWithSelection();
    void gotoPreviousLine();
    void gotoPreviousLineWithSelection();
    void gotoPreviousCharacter();
    void gotoPreviousCharacterWithSelection();
    void gotoNextCharacter();
    void gotoNextCharacterWithSelection();
    void gotoPreviousWord();
    void gotoPreviousWordWithSelection();
    void gotoNextWord();
    void gotoNextWordWithSelection();
    void gotoPreviousWordCamelCase();
    void gotoPreviousWordCamelCaseWithSelection();
    void gotoNextWordCamelCase();
    void gotoNextWordCamelCaseWithSelection();

    virtual bool selectBlockUp();
    virtual bool selectBlockDown();
    void selectWordUnderCursor();

    void showContextMenu();

    void moveLineUp();
    void moveLineDown();

    void viewPageUp();
    void viewPageDown();
    void viewLineUp();
    void viewLineDown();

    void copyLineUp();
    void copyLineDown();

    void joinLines();

    void insertLineAbove();
    void insertLineBelow();

    void uppercaseSelection();
    void lowercaseSelection();

    void sortSelectedLines();

    void cleanWhitespace();

    void indent();
    void unindent();

    virtual void undo();
    virtual void redo();

    void openLinkUnderCursor();
    void openLinkUnderCursorInNextSplit();
    void openTypeUnderCursor();
    void openTypeUnderCursorInNextSplit();

    virtual void findUsages();
    virtual void renameSymbolUnderCursor();
    virtual void openCallHierarchy();

    /// Abort code assistant if it is running.
    void abortAssist();

    /// Overwrite the current highlighter with a new generic highlighter based on the mimetype of
    /// the current document
    void configureGenericHighlighter();
    /// Overwrite the current highlighter with a new generic highlighter based on the given mimetype
    void configureGenericHighlighter(const Utils::MimeType &mimeType);

    /// Overwrite the current highlighter with a new generic highlighter based on the given definition
    Utils::expected_str<void> configureGenericHighlighter(const QString &definitionName);

    Q_INVOKABLE void inSnippetMode(bool *active); // Used by FakeVim.

    /*! Returns the document line number for the visible \a row.
     *
     * The first visible row is 0, the last visible row is rowCount() - 1.
     *
     * Any invalid row will return -1 as line number.
     */
    int blockNumberForVisibleRow(int row) const;

    /*! Returns the first visible line of the document. */
    int firstVisibleBlockNumber() const;
    /*! Returns the last visible line of the document. */
    int lastVisibleBlockNumber() const;
    /*! Returns the line visible closest to the vertical center of the editor. */
    int centerVisibleBlockNumber() const;

    Core::HighlightScrollBarController *highlightScrollBarController() const;

    void addHoverHandler(BaseHoverHandler *handler);
    void removeHoverHandler(BaseHoverHandler *handler);

    void insertSuggestion(std::unique_ptr<TextSuggestion> &&suggestion);
    void clearSuggestion();
    TextSuggestion *currentSuggestion() const;
    bool suggestionVisible() const;
    bool suggestionsBlocked() const;

    using SuggestionBlocker = std::shared_ptr<void>;
    // Returns an object that blocks suggestions until it is destroyed.
    SuggestionBlocker blockSuggestions();

#ifdef WITH_TESTS
    void processTooltipRequest(const QTextCursor &c);
#endif

signals:
    void assistFinished(); // Used in tests.
    void readOnlyChanged();

    void requestBlockUpdate(const QTextBlock &);

    void requestLinkAt(const QTextCursor &cursor, const Utils::LinkHandler &callback,
                       bool resolveTarget, bool inNextSplit);
    void requestTypeAt(const QTextCursor &cursor, const Utils::LinkHandler &callback,
                       bool resolveTarget, bool inNextSplit);
    void requestUsages(const QTextCursor &cursor);
    void requestRename(const QTextCursor &cursor);
    void requestCallHierarchy(const QTextCursor &cursor);
    void optionalActionMaskChanged();
    void toolbarOutlineChanged(QWidget *newOutline);

protected:
    QTextBlock blockForVisibleRow(int row) const;
    QTextBlock blockForVerticalOffset(int offset) const;
    bool event(QEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void changeEvent(QEvent *e) override;
    void focusInEvent(QFocusEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;
    void showEvent(QShowEvent *) override;
    bool viewportEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *) override;
    void paintEvent(QPaintEvent *) override;
    virtual void paintBlock(QPainter *painter,
                            const QTextBlock &block,
                            const QPointF &offset,
                            const QVector<QTextLayout::FormatRange> &selections,
                            const QRect &clipRect) const;
    void timerEvent(QTimerEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;
    void leaveEvent(QEvent *) override;
    void keyReleaseEvent(QKeyEvent *) override;
    void dragEnterEvent(QDragEnterEvent *e) override;

    QMimeData *createMimeDataFromSelection() const override;
    QMimeData *createMimeDataFromSelection(bool withHtml) const;
    bool canInsertFromMimeData(const QMimeData *source) const override;
    void insertFromMimeData(const QMimeData *source) override;
    void dragLeaveEvent(QDragLeaveEvent *e) override;
    void dragMoveEvent(QDragMoveEvent *e) override;
    void dropEvent(QDropEvent *e) override;

    virtual QString plainTextFromSelection(const QTextCursor &cursor) const;
    virtual QString plainTextFromSelection(const Utils::MultiTextCursor &cursor) const;

    virtual QString lineNumber(int blockNumber) const;
    virtual int lineNumberDigits() const;
    virtual bool selectionVisible(int blockNumber) const;
    virtual bool replacementVisible(int blockNumber) const;
    virtual QColor replacementPenColor(int blockNumber) const;

    virtual void triggerPendingUpdates();
    virtual void applyFontSettings();

    void showDefaultContextMenu(QContextMenuEvent *e, Utils::Id menuContextId);
    virtual void finalizeInitialization() {}
    virtual void finalizeInitializationAfterDuplication(TextEditorWidget *) {}
    static QTextCursor flippedCursor(const QTextCursor &cursor);

    void setVisualIndentOffset(int offset);

public:
    QString selectedText() const;

    void setupGenericHighlighter();
    void setupFallBackEditor(Utils::Id id);

    void remove(int length);
    void replace(int length, const QString &string);
    QChar characterAt(int pos) const;
    QString textAt(int from, int to) const;

    void contextHelpItem(const Core::IContext::HelpCallback &callback);
    void setContextHelpItem(const Core::HelpItem &item);

    Q_INVOKABLE bool inFindScope(const QTextCursor &cursor) const;

    static TextEditorWidget *currentTextEditorWidget();
    static TextEditorWidget *fromEditor(const Core::IEditor *editor);

protected:
    /*!
       Reimplement this function to enable code navigation.

       \a resolveTarget is set to true when the target of the link is relevant
       (it isn't until the link is used).
     */
    virtual void findLinkAt(const QTextCursor &,
                            const Utils::LinkHandler &processLinkCallback,
                            bool resolveTarget = true,
                            bool inNextSplit = false);

    virtual void findTypeAt(const QTextCursor &,
                            const Utils::LinkHandler &processLinkCallback,
                            bool resolveTarget = true,
                            bool inNextSplit = false);

    /*!
       Returns whether the link was opened successfully.
     */
    bool openLink(const Utils::Link &link, bool inNextSplit = false);

    /*!
      Reimplement this function to change the default replacement text.
      */
    virtual QString foldReplacementText(const QTextBlock &block) const;
    virtual void drawCollapsedBlockPopup(QPainter &painter,
                                         const QTextBlock &block,
                                         QPointF offset,
                                         const QRect &clip);
    int visibleFoldedBlockNumber() const;
    void doSetTextCursor(const QTextCursor &cursor) override;
    void doSetTextCursor(const QTextCursor &cursor, bool keepMultiSelection);

signals:
    void markRequested(TextEditor::TextEditorWidget *widget,
        int line, TextEditor::TextMarkRequestKind kind);
    void markContextMenuRequested(TextEditor::TextEditorWidget *widget,
        int line, QMenu *menu);
    void tooltipOverrideRequested(TextEditor::TextEditorWidget *widget,
        const QPoint &globalPos, int position, bool *handled);
    void tooltipRequested(const QPoint &globalPos, int position);
    void activateEditor(Core::EditorManager::OpenEditorFlags flags = {});

protected:
    virtual void slotCursorPositionChanged(); // Used in VcsBase
    virtual void slotCodeStyleSettingsChanged(const QVariant &); // Used in CppEditor

private:
    Internal::TextEditorWidgetPrivate *d;
    friend class BaseTextEditor;
    friend class TextEditorFactory;
    friend class Internal::TextEditorFactoryPrivate;
    friend class Internal::TextEditorWidgetPrivate;
    friend class Internal::TextEditorOverlay;
    friend class RefactorOverlay;

    void updateVisualWrapColumn();
};

class TEXTEDITOR_EXPORT TextEditorLinkLabel : public Utils::ElidingLabel
{
public:
    TextEditorLinkLabel(QWidget *parent = nullptr);

    void setLink(Utils::Link link);
    Utils::Link link() const;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QPoint m_dragStartPosition;
    Utils::Link m_link;
};

class TEXTEDITOR_EXPORT TextEditorFactory : public Core::IEditorFactory
{

public:
    TextEditorFactory();
    ~TextEditorFactory() override;

    using EditorCreator = std::function<BaseTextEditor *()>;
    using DocumentCreator = std::function<TextDocument *()>;
    // editor widget must be castable (qobject_cast or Aggregate::query) to TextEditorWidget
    using EditorWidgetCreator = std::function<QWidget *()>;
    using SyntaxHighLighterCreator = std::function<SyntaxHighlighter *()>;
    using IndenterCreator = std::function<Indenter *(QTextDocument *)>;
    using AutoCompleterCreator = std::function<AutoCompleter *()>;

    void setDocumentCreator(const DocumentCreator &creator);
    void setEditorWidgetCreator(const EditorWidgetCreator &creator);
    void setEditorCreator(const EditorCreator &creator);
    void setIndenterCreator(const IndenterCreator &creator);
    void setSyntaxHighlighterCreator(const SyntaxHighLighterCreator &creator);
    void setUseGenericHighlighter(bool enabled);
    void setAutoCompleterCreator(const AutoCompleterCreator &creator);
    void setEditorActionHandlers(uint optionalActions);

    void addHoverHandler(BaseHoverHandler *handler);
    void setCompletionAssistProvider(CompletionAssistProvider *provider);

    void setCommentDefinition(Utils::CommentDefinition definition);
    void setDuplicatedSupported(bool on);
    void setMarksVisible(bool on);
    void setParenthesesMatchingEnabled(bool on);
    void setCodeFoldingSupported(bool on);

private:
    friend class BaseTextEditor;
    friend class PlainTextEditorFactory;
    Internal::TextEditorFactoryPrivate *d;
};

} // namespace TextEditor

QT_BEGIN_NAMESPACE

size_t qHash(const QColor &color);

QT_END_NAMESPACE
