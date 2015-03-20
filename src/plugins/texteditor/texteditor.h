/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef TEXTEDITOR_H
#define TEXTEDITOR_H

#include "texteditor_global.h"
#include "codeassist/assistenums.h"

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/ieditorfactory.h>

#include <utils/uncommentselection.h>

#include <QLabel>
#include <QPlainTextEdit>
#include <QSharedPointer>
#include <functional>

QT_BEGIN_NAMESPACE
class QToolBar;
class QPrinter;
class QMenu;
class QPainter;
class QPoint;
class QRect;
class QTextBlock;
QT_END_NAMESPACE

namespace TextEditor {
class TextDocument;
class BaseHoverHandler;
class TabSettings;
class RefactorOverlay;
struct RefactorMarker;
class SyntaxHighlighter;
class IAssistMonitorInterface;
class AssistInterface;
class IAssistProvider;
class ICodeStylePreferences;
class CompletionAssistProvider;
typedef QList<RefactorMarker> RefactorMarkers;

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
class Indenter;
class MarginSettings;
class StorageSettings;
class TypingSettings;

class TEXTEDITOR_EXPORT BlockRange
{
public:
    BlockRange() : _first(0), _last(-1) {}
    BlockRange(int firstPosition, int lastPosition)
      : _first(firstPosition), _last(lastPosition)
    {}

    inline bool isNull() const { return _last < _first; }

    int first() const { return _first; }
    int last() const { return _last; }

private:
    int _first;
    int _last;
};

enum TextPositionOperation
{
    CurrentPosition = 1,
    EndOfLinePosition = 2,
    StartOfLinePosition = 3,
    AnchorPosition = 4,
    EndOfDocPosition = 5
};

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
    ~BaseTextEditor();

    virtual void finalizeInitialization() {}

    static BaseTextEditor *currentTextEditor();

    TextEditorWidget *editorWidget() const;
    TextDocument *textDocument() const;

    // Some convenience text access
    void setTextCursor(const QTextCursor &cursor);
    QTextCursor textCursor() const;
    QChar characterAt(int pos) const;
    QString textAt(int from, int to) const;

    void addContext(Core::Id id);

    // IEditor
    Core::IDocument *document();
    bool open(QString *errorString, const QString &fileName, const QString &realFileName);

    IEditor *duplicate();

    QByteArray saveState() const;
    bool restoreState(const QByteArray &state);
    QWidget *toolBar();

    QString contextHelpId() const; // from IContext
    void setContextHelpId(const QString &id);

    int currentLine() const;
    int currentColumn() const;
    void gotoLine(int line, int column = 0, bool centerLine = true);

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
    Q_PROPERTY(int verticalBlockSelectionFirstColumn READ verticalBlockSelectionFirstColumn)
    Q_PROPERTY(int verticalBlockSelectionLastColumn READ verticalBlockSelectionLastColumn)

public:
    TextEditorWidget(QWidget *parent = 0);
    ~TextEditorWidget();

    void setTextDocument(const QSharedPointer<TextDocument> &doc);
    TextDocument *textDocument() const;
    QSharedPointer<TextDocument> textDocumentPtr() const;

    // IEditor
    virtual bool open(QString *errorString, const QString &fileName, const QString &realFileName);
    QByteArray saveState() const;
    bool restoreState(const QByteArray &state);
    void gotoLine(int line, int column = 0, bool centerLine = true);
    int position(TextPositionOperation posOp = CurrentPosition,
         int at = -1) const;
    void convertPosition(int pos, int *line, int *column) const;
    using QPlainTextEdit::cursorRect;
    QRect cursorRect(int pos) const;
    void setCursorPosition(int pos);

    void print(QPrinter *);

    void appendStandardContextMenuActions(QMenu *menu);

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

    void setTextCursor(const QTextCursor &cursor, bool keepBlockSelection);
    void setTextCursor(const QTextCursor &cursor);

    void insertCodeSnippet(const QTextCursor &cursor, const QString &snippet);

    void setBlockSelection(bool on);
    void setBlockSelection(int positionBlock, int positionColumn, int anchhorBlock,
                           int anchorColumn);
    void setBlockSelection(const QTextCursor &cursor);
    QTextCursor blockSelection() const;
    bool hasBlockSelection() const;

    int verticalBlockSelectionFirstColumn() const;
    int verticalBlockSelectionLastColumn() const;

    QRegion translatedLineRegion(int lineStart, int lineEnd) const;

    QPoint toolTipPosition(const QTextCursor &c) const;

    void invokeAssist(AssistKind assistKind, IAssistProvider *provider = 0);

    virtual TextEditor::AssistInterface *createAssistInterface(AssistKind assistKind,
                                                    AssistReason assistReason) const;
    static QMimeData *duplicateMimeData(const QMimeData *source);

    static QString msgTextTooLarge(quint64 size);

    void insertPlainText(const QString &text);

    QWidget *extraArea() const;
    virtual int extraAreaWidth(int *markWidthPtr = 0) const;
    virtual void extraAreaPaintEvent(QPaintEvent *);
    virtual void extraAreaLeaveEvent(QEvent *);
    virtual void extraAreaContextMenuEvent(QContextMenuEvent *);
    virtual void extraAreaMouseEvent(QMouseEvent *);
    void updateFoldingHighlight(const QPoint &pos);

    void setLanguageSettingsId(Core::Id settingsId);
    Core::Id languageSettingsId() const;

    void setCodeStyle(ICodeStylePreferences *settings);

    const DisplaySettings &displaySettings() const;
    const MarginSettings &marginSettings() const;

    void ensureCursorVisible();

    enum ExtraSelectionKind {
        CurrentLineSelection,
        ParenthesesMatchingSelection,
        CodeWarningsSelection,
        CodeSemanticsSelection,
        UndefinedSymbolSelection,
        UnusedSymbolSelection,
        FakeVimSelection,
        OtherSelection,
        SnippetPlaceholderSelection,
        ObjCSelection,
        DebuggerExceptionSelection,
        NExtraSelectionKinds
    };
    void setExtraSelections(ExtraSelectionKind kind, const QList<QTextEdit::ExtraSelection> &selections);
    QList<QTextEdit::ExtraSelection> extraSelections(ExtraSelectionKind kind) const;
    void setExtraSelections(Core::Id kind, const QList<QTextEdit::ExtraSelection> &selections);
    QList<QTextEdit::ExtraSelection> extraSelections(Core::Id kind) const;
    QString extraSelectionTooltip(int pos) const;

    RefactorMarkers refactorMarkers() const;
    void setRefactorMarkers(const RefactorMarkers &markers);

    // the blocks list must be sorted
    void setIfdefedOutBlocks(const QList<BlockRange> &blocks);
    bool isMissingSyntaxDefinition() const;

    enum Side { Left, Right };
    void insertExtraToolBarWidget(Side side, QWidget *widget);

    virtual void copy();
    virtual void paste();
    virtual void cut();
    virtual void selectAll();

    virtual void format();
    virtual void rewrapParagraph();
    virtual void unCommentSelection();

public slots: // Qt4-style connect used in EditorConfiguration
    virtual void setDisplaySettings(const TextEditor::DisplaySettings &);
    virtual void setMarginSettings(const TextEditor::MarginSettings &);
    void setBehaviorSettings(const TextEditor::BehaviorSettings &);
    void setTypingSettings(const TextEditor::TypingSettings &);
    void setStorageSettings(const TextEditor::StorageSettings &);
    void setCompletionSettings(const TextEditor::CompletionSettings &);
    void setExtraEncodingSettings(const TextEditor::ExtraEncodingSettings &);

public:
    void circularPaste();
    void switchUtf8bom();

    void zoomIn();
    void zoomOut();
    void zoomReset();

    void cutLine();
    void copyLine();
    void deleteLine();
    void deleteEndOfWord();
    void deleteEndOfWordCamelCase();
    void deleteStartOfWord();
    void deleteStartOfWordCamelCase();
    void unfoldAll();
    void fold();
    void unfold();
    void selectEncoding();
    void updateTextCodecLabel();

    void gotoBlockStart();
    void gotoBlockEnd();
    void gotoBlockStartWithSelection();
    void gotoBlockEndWithSelection();

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

    bool selectBlockUp();
    bool selectBlockDown();

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

    void cleanWhitespace();

    void indent();
    void unindent();

    void undo();
    void redo();

    void openLinkUnderCursor();
    void openLinkUnderCursorInNextSplit();

    /// Abort code assistant if it is running.
    void abortAssist();

    void configureGenericHighlighter();

    Q_INVOKABLE void inSnippetMode(bool *active); // Used by FakeVim.

signals:
    void assistFinished(); // Used in tests.
    void readOnlyChanged();

    void requestFontZoom(int zoom);
    void requestZoomReset();
    void requestBlockUpdate(const QTextBlock &);

protected:
    bool event(QEvent *e);
    void inputMethodEvent(QInputMethodEvent *e);
    void keyPressEvent(QKeyEvent *e);
    void wheelEvent(QWheelEvent *e);
    void changeEvent(QEvent *e);
    void focusInEvent(QFocusEvent *e);
    void focusOutEvent(QFocusEvent *e);
    void showEvent(QShowEvent *);
    bool viewportEvent(QEvent *event);
    void resizeEvent(QResizeEvent *);
    void paintEvent(QPaintEvent *);
    virtual void paintBlock(QPainter *painter,
                            const QTextBlock &block,
                            const QPointF &offset,
                            const QVector<QTextLayout::FormatRange> &selections,
                            const QRect &clipRect) const;
    void timerEvent(QTimerEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseDoubleClickEvent(QMouseEvent *);
    void leaveEvent(QEvent *);
    void keyReleaseEvent(QKeyEvent *);
    void dragEnterEvent(QDragEnterEvent *e);

    QMimeData *createMimeDataFromSelection() const;
    bool canInsertFromMimeData(const QMimeData *source) const;
    void insertFromMimeData(const QMimeData *source);

    virtual QString plainTextFromSelection(const QTextCursor &cursor) const;
    static QString convertToPlainText(const QString &txt);

    virtual QString lineNumber(int blockNumber) const;
    virtual int lineNumberDigits() const;
    virtual bool selectionVisible(int blockNumber) const;
    virtual bool replacementVisible(int blockNumber) const;
    virtual QColor replacementPenColor(int blockNumber) const;

    virtual void triggerPendingUpdates();
    virtual void applyFontSettings();

    virtual void onRefactorMarkerClicked(const RefactorMarker &) {}

    void showDefaultContextMenu(QContextMenuEvent *e, Core::Id menuContextId);
    virtual void finalizeInitialization() {}
    virtual void finalizeInitializationAfterDuplication(TextEditorWidget *) {}

public:
    struct Link
    {
        Link(const QString &fileName = QString(), int line = 0, int column = 0)
            : linkTextStart(-1)
            , linkTextEnd(-1)
            , targetFileName(fileName)
            , targetLine(line)
            , targetColumn(column)
        {}

        bool hasValidTarget() const
        { return !targetFileName.isEmpty(); }

        bool hasValidLinkText() const
        { return linkTextStart != linkTextEnd; }

        bool operator==(const Link &other) const
        { return linkTextStart == other.linkTextStart && linkTextEnd == other.linkTextEnd; }

        int linkTextStart;
        int linkTextEnd;

        QString targetFileName;
        int targetLine;
        int targetColumn;
    };

    QString selectedText() const;

    void setupGenericHighlighter();
    void setupFallBackEditor(Core::Id id);

    void remove(int length);
    void replace(int length, const QString &string);
    QChar characterAt(int pos) const;
    QString textAt(int from, int to) const;

    QString contextHelpId();
    void setContextHelpId(const QString &id);

    static TextEditorWidget *currentTextEditorWidget();

protected:
    /*!
       Reimplement this function to enable code navigation.

       \a resolveTarget is set to true when the target of the link is relevant
       (it isn't until the link is used).
     */
    virtual Link findLinkAt(const QTextCursor &, bool resolveTarget = true,
                            bool inNextSplit = false);

    /*!
       Returns whether the link was opened successfully.
     */
    bool openLink(const Link &link, bool inNextSplit = false);

    /*!
      Reimplement this function to change the default replacement text.
      */
    virtual QString foldReplacementText(const QTextBlock &block) const;
    virtual void drawCollapsedBlockPopup(QPainter &painter,
                                         const QTextBlock &block,
                                         QPointF offset,
                                         const QRect &clip);
    int visibleFoldedBlockNumber() const;


signals:
    void markRequested(TextEditor::TextEditorWidget *widget,
        int line, TextEditor::TextMarkRequestKind kind);
    void markContextMenuRequested(TextEditor::TextEditorWidget *widget,
        int line, QMenu *menu);
    void tooltipOverrideRequested(TextEditor::TextEditorWidget *widget,
        const QPoint &globalPos, int position, bool *handled);
    void tooltipRequested(const QPoint &globalPos, int position);
    void markTooltipRequested(TextEditor::TextEditorWidget *widget,
        const QPoint &globalPos, int line);
    void activateEditor();

protected slots:
    virtual void slotCursorPositionChanged(); // Used in VcsBase
    virtual void slotCodeStyleSettingsChanged(const QVariant &); // Used in CppEditor

    void doFoo();

private:
    Internal::TextEditorWidgetPrivate *d;
    friend class BaseTextEditor;
    friend class TextEditorFactory;
    friend class Internal::TextEditorFactoryPrivate;
    friend class Internal::TextEditorWidgetPrivate;
    friend class Internal::TextEditorOverlay;
    friend class RefactorOverlay;
};

class TEXTEDITOR_EXPORT TextEditorLinkLabel : public QLabel
{
public:
    TextEditorLinkLabel(QWidget *parent = 0);

    void setLink(TextEditorWidget::Link link);
    TextEditorWidget::Link link() const;

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private:
    QPoint m_dragStartPosition;
    TextEditorWidget::Link m_link;
};

class TEXTEDITOR_EXPORT TextEditorFactory : public Core::IEditorFactory
{
    Q_OBJECT

public:
    TextEditorFactory(QObject *parent = 0);
    ~TextEditorFactory();

    typedef std::function<BaseTextEditor *()> EditorCreator;
    typedef std::function<TextDocument *()> DocumentCreator;
    typedef std::function<TextEditorWidget *()> EditorWidgetCreator;
    typedef std::function<SyntaxHighlighter *()> SyntaxHighLighterCreator;
    typedef std::function<Indenter *()> IndenterCreator;
    typedef std::function<AutoCompleter *()> AutoCompleterCreator;

    void setDocumentCreator(DocumentCreator &&creator);
    void setEditorWidgetCreator(EditorWidgetCreator &&creator);
    void setEditorCreator(EditorCreator &&creator);
    void setIndenterCreator(IndenterCreator &&creator);
    void setSyntaxHighlighterCreator(SyntaxHighLighterCreator &&creator);
    void setUseGenericHighlighter(bool enabled);
    void setAutoCompleterCreator(AutoCompleterCreator &&creator);

    void setEditorActionHandlers(Core::Id contextId, uint optionalActions);
    void setEditorActionHandlers(uint optionalActions);

    void addHoverHandler(BaseHoverHandler *handler);
    void setCompletionAssistProvider(CompletionAssistProvider *provider);

    void setCommentStyle(Utils::CommentDefinition::Style style);
    void setDuplicatedSupported(bool on);
    void setMarksVisible(bool on);
    void setParenthesesMatchingEnabled(bool on);
    void setCodeFoldingSupported(bool on);

    Core::IEditor *createEditor();

private:
    friend class BaseTextEditor;
    friend class PlainTextEditorFactory;
    Internal::TextEditorFactoryPrivate *d;
};

} // namespace TextEditor

Q_DECLARE_METATYPE(TextEditor::TextEditorWidget::Link)

#endif // TEXTEDITOR_H
