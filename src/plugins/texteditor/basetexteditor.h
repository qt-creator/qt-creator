/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef BASETEXTEDITOR_H
#define BASETEXTEDITOR_H

#include "itexteditable.h"

#include <find/ifindsupport.h>

#include <QtGui/QPlainTextEdit>
#include <QtGui/QTextBlockUserData>

QT_BEGIN_NAMESPACE
class QKeyEvent;
class QToolBar;
class QTimeLine;
QT_END_NAMESPACE

namespace Utils {
    class LineColumnLabel;
    class ChangeSet;
}

namespace TextEditor {

namespace Internal {
    class BaseTextEditorPrivate;
    class TextEditorOverlay;
}

class ITextMark;
class ITextMarkable;

class TextEditorActionHandler;
class BaseTextDocument;
class FontSettings;
struct BehaviorSettings;
struct DisplaySettings;
struct StorageSettings;
struct TabSettings;

struct Parenthesis;
typedef QVector<Parenthesis> Parentheses;

struct TEXTEDITOR_EXPORT Parenthesis
{
    enum Type { Opened, Closed };

    inline Parenthesis() : type(Opened), pos(-1) {}
    inline Parenthesis(Type t, QChar c, int position)
        : type(t), chr(c), pos(position) {}
    Type type;
    QChar chr;
    int pos;
    static int collapseAtPos(const Parentheses &parentheses, QChar *character = 0);
    static int closeCollapseAtPos(const Parentheses &parentheses);
    static bool hasClosingCollapse(const Parentheses &parentheses);
};


class TEXTEDITOR_EXPORT TextBlockUserData : public QTextBlockUserData
{
public:

    enum CollapseMode { NoCollapse , CollapseThis, CollapseAfter };
    enum ClosingCollapseMode { NoClosingCollapse, ClosingCollapse, ClosingCollapseAtEnd };

    inline TextBlockUserData()
        : m_collapseIncludesClosure(false),
          m_collapseMode(NoCollapse),
          m_closingCollapseMode(NoClosingCollapse),
          m_collapsed(false),
          m_ifdefedOut(false) {}
    ~TextBlockUserData();

    inline TextMarks marks() const { return m_marks; }
    inline void addMark(ITextMark *mark) { m_marks += mark; }
    inline bool removeMark(ITextMark *mark) { return m_marks.removeAll(mark); }
    inline bool hasMark(ITextMark *mark) const { return m_marks.contains(mark); }
    inline void clearMarks() { m_marks.clear(); }
    inline void documentClosing() { Q_FOREACH(ITextMark *tm, m_marks) { tm->documentClosing(); } m_marks.clear();}

    inline CollapseMode collapseMode() const { return (CollapseMode)m_collapseMode; }
    inline void setCollapseMode(CollapseMode c) { m_collapseMode = c; }

    inline void setClosingCollapseMode(ClosingCollapseMode c) { m_closingCollapseMode = c; }
    inline ClosingCollapseMode closingCollapseMode() const { return (ClosingCollapseMode) m_closingCollapseMode; }

    inline bool hasClosingCollapse() const { return closingCollapseMode() != NoClosingCollapse; }
    inline bool hasClosingCollapseAtEnd() const { return closingCollapseMode() == ClosingCollapseAtEnd; }
    inline bool hasClosingCollapseInside() const { return closingCollapseMode() == ClosingCollapse; }

    inline void setCollapsed(bool b) { m_collapsed = b; }
    inline bool collapsed() const { return m_collapsed; }

    inline void setCollapseIncludesClosure(bool b) { m_collapseIncludesClosure = b; }
    inline bool collapseIncludesClosure() const { return m_collapseIncludesClosure; }

    inline void setParentheses(const Parentheses &parentheses) { m_parentheses = parentheses; }
    inline void clearParentheses() { m_parentheses.clear(); }
    inline const Parentheses &parentheses() const { return m_parentheses; }
    inline bool hasParentheses() const { return !m_parentheses.isEmpty(); }
    int braceDepthDelta() const;

    inline bool setIfdefedOut() { bool result = m_ifdefedOut; m_ifdefedOut = true; return !result; }
    inline bool clearIfdefedOut() { bool result = m_ifdefedOut; m_ifdefedOut = false; return result;}
    inline bool ifdefedOut() const { return m_ifdefedOut; }

    inline static TextBlockUserData *canCollapse(const QTextBlock &block) {
        TextBlockUserData *data = static_cast<TextBlockUserData*>(block.userData());
        if (!data || data->collapseMode() != CollapseAfter) {
            data = static_cast<TextBlockUserData*>(block.next().userData());
            if (!data || data->collapseMode() != TextBlockUserData::CollapseThis)
                data = 0;
        }
        if (data && data->m_ifdefedOut)
            data = 0;
        return data;
    }

    inline static bool hasCollapseAfter(const QTextBlock & block)
    {
        if (!block.isValid()) {
            return false;
        } else if (block.next().isValid()) {
            TextBlockUserData *data = static_cast<TextBlockUserData*>(block.next().userData());
            if (data && data->collapseMode() == TextBlockUserData::CollapseThis &&  !data->m_ifdefedOut)
                return true;
        }
        return false;
    }

    inline static bool hasClosingCollapse(const QTextBlock &block) {
        TextBlockUserData *data = static_cast<TextBlockUserData*>(block.userData());
        return (data && data->hasClosingCollapse());
    }

    inline static bool hasClosingCollapseAtEnd(const QTextBlock &block) {
        TextBlockUserData *data = static_cast<TextBlockUserData*>(block.userData());
        return (data && data->hasClosingCollapseAtEnd());
    }

    inline static bool hasClosingCollapseInside(const QTextBlock &block) {
        TextBlockUserData *data = static_cast<TextBlockUserData*>(block.userData());
        return (data && data->hasClosingCollapseInside());
    }

    static QTextBlock testCollapse(const QTextBlock& block);
    static void doCollapse(const QTextBlock& block, bool visible);

    int collapseAtPos(QChar *character = 0) const;

    enum MatchType { NoMatch, Match, Mismatch  };
    static MatchType checkOpenParenthesis(QTextCursor *cursor, QChar c);
    static MatchType checkClosedParenthesis(QTextCursor *cursor, QChar c);
    static MatchType matchCursorBackward(QTextCursor *cursor);
    static MatchType matchCursorForward(QTextCursor *cursor);
    static bool findPreviousOpenParenthesis(QTextCursor *cursor, bool select = false);
    static bool findNextClosingParenthesis(QTextCursor *cursor, bool select = false);

    static bool findPreviousBlockOpenParenthesis(QTextCursor *cursor, bool checkStartPosition = false);
    static bool findNextBlockClosingParenthesis(QTextCursor *cursor);


private:
    TextMarks m_marks;
    uint m_collapseIncludesClosure : 1;
    uint m_collapseMode : 4;
    uint m_closingCollapseMode : 4;
    uint m_collapsed : 1;
    uint m_ifdefedOut : 1;
    Parentheses m_parentheses;
};


class TEXTEDITOR_EXPORT TextEditDocumentLayout : public QPlainTextDocumentLayout
{
    Q_OBJECT

public:
    TextEditDocumentLayout(QTextDocument *doc);
    ~TextEditDocumentLayout();

    QRectF blockBoundingRect(const QTextBlock &block) const;

    static void setParentheses(const QTextBlock &block, const Parentheses &parentheses);
    static void clearParentheses(const QTextBlock &block) { setParentheses(block, Parentheses());}
    static Parentheses parentheses(const QTextBlock &block);
    static bool hasParentheses(const QTextBlock &block);
    static bool setIfdefedOut(const QTextBlock &block);
    static bool clearIfdefedOut(const QTextBlock &block);
    static bool ifdefedOut(const QTextBlock &block);
    static int braceDepthDelta(const QTextBlock &block);
    static int braceDepth(const QTextBlock &block);
    static void setBraceDepth(QTextBlock &block, int depth);
    static void changeBraceDepth(QTextBlock &block, int delta);

    static TextBlockUserData *testUserData(const QTextBlock &block) {
        return static_cast<TextBlockUserData*>(block.userData());
    }
    static TextBlockUserData *userData(const QTextBlock &block) {
        TextBlockUserData *data = static_cast<TextBlockUserData*>(block.userData());
        if (!data && block.isValid())
            const_cast<QTextBlock &>(block).setUserData((data = new TextBlockUserData));
        return data;
    }


    void emitDocumentSizeChanged() { emit documentSizeChanged(documentSize()); }
    int lastSaveRevision;
    bool hasMarks;
};


class BaseTextEditorEditable;

class TEXTEDITOR_EXPORT BaseTextEditorAnimator : public QObject
{
    Q_OBJECT

public:
    BaseTextEditorAnimator(QObject *parent);

    inline void setPosition(int position) { m_position = position; }
    inline int position() const { return m_position; }

    void setData(QFont f, QPalette pal, const QString &text);

    void draw(QPainter *p, const QPointF &pos);
    QRectF rect() const;

    inline qreal value() const { return m_value; }
    inline QPointF lastDrawPos() const { return m_lastDrawPos; }

    void finish();

    bool isRunning() const;

signals:
    void updateRequest(int position, QPointF lastPos, QRectF rect);


private slots:
    void step(qreal v);

private:
    QTimeLine *m_timeline;
    qreal m_value;
    int m_position;
    QPointF m_lastDrawPos;
    QFont m_font;
    QPalette m_palette;
    QString m_text;
    QSizeF m_size;
};


class TEXTEDITOR_EXPORT BaseTextEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    BaseTextEditor(QWidget *parent);
    ~BaseTextEditor();

    static ITextEditor *openEditorAt(const QString &fileName, int line, int column = 0,
                                     const QString &editorId = QString());

    const Utils::ChangeSet &changeSet() const;
    void setChangeSet(const Utils::ChangeSet &changeSet);

    // EditorInterface
    Core::IFile * file();
    bool createNew(const QString &contents);
    bool open(const QString &fileName = QString());
    QByteArray saveState() const;
    bool restoreState(const QByteArray &state);
    QString displayName() const;

    // ITextEditor

    void gotoLine(int line, int column = 0);

    int position(
        ITextEditor::PositionOperation posOp = ITextEditor::Current
        , int at = -1) const;
    void convertPosition(int pos, int *line, int *column) const;

    ITextEditable *editableInterface() const;
    ITextMarkable *markableInterface() const;

    virtual void triggerCompletions();
    virtual void triggerQuickFix();

    QChar characterAt(int pos) const;

    void print(QPrinter *);

    void setSuggestedFileName(const QString &suggestedFileName);
    QString mimeType() const;
    void setMimeType(const QString &mt);


    void appendStandardContextMenuActions(QMenu *menu);

    // Works only in conjunction with a syntax highlighter that puts
    // parentheses into text block user data
    void setParenthesesMatchingEnabled(bool b);
    bool isParenthesesMatchingEnabled() const;

    void setHighlightCurrentLine(bool b);
    bool highlightCurrentLine() const;

    void setLineNumbersVisible(bool b);
    bool lineNumbersVisible() const;


    void setMarksVisible(bool b);
    bool marksVisible() const;

    void setRequestMarkEnabled(bool b);
    bool requestMarkEnabled() const;

    void setLineSeparatorsAllowed(bool b);
    bool lineSeparatorsAllowed() const;

    void setCodeFoldingVisible(bool b);
    bool codeFoldingVisible() const;

    void setCodeFoldingSupported(bool b);
    bool codeFoldingSupported() const;

    void setMouseNavigationEnabled(bool b);
    bool mouseNavigationEnabled() const;

    void setScrollWheelZoomingEnabled(bool b);
    bool scrollWheelZoomingEnabled() const;

    void setRevisionsVisible(bool b);
    bool revisionsVisible() const;

    void setVisibleWrapColumn(int column);
    int visibleWrapColumn() const;

    void setActionHack(QObject *);
    QObject *actionHack() const;

    void setTextCodec(QTextCodec *codec);
    QTextCodec *textCodec() const;

    void setReadOnly(bool b);

    void setTextCursor(const QTextCursor &cursor);

    void insertCodeSnippet(const QString &snippet);

public slots:
    void setDisplayName(const QString &title);

    virtual void paste();
    virtual void cut();

    void zoomIn(int range = 1);
    void zoomOut(int range = 1);
    void zoomReset();

    void cutLine();
    void deleteLine();
    void unCollapseAll();
    void collapse();
    void expand();
    void selectEncoding();

    void gotoBlockStart();
    void gotoBlockEnd();
    void gotoBlockStartWithSelection();
    void gotoBlockEndWithSelection();

    void selectBlockUp();
    void selectBlockDown();

    void moveLineUp();
    void moveLineDown();

    void copyLineUp();
    void copyLineDown();

    void joinLines();

    void cleanWhitespace();

signals:
    void changed();

    // ITextEditor
    void contentsChanged();

protected:
    bool event(QEvent *e);
    void keyPressEvent(QKeyEvent *e);
    void wheelEvent(QWheelEvent *e);
    void changeEvent(QEvent *e);
    void focusInEvent(QFocusEvent *e);
    void focusOutEvent(QFocusEvent *e);

    void showEvent(QShowEvent *);

    // reimplemented to support block selection
    QMimeData *createMimeDataFromSelection() const;
    bool canInsertFromMimeData(const QMimeData *source) const;
    void insertFromMimeData(const QMimeData *source);

private:
    void maybeSelectLine();

public:
    void duplicateFrom(BaseTextEditor *editor);

protected:
    BaseTextDocument *baseTextDocument() const;
    void setBaseTextDocument(BaseTextDocument *doc);

    void setDefaultPath(const QString &defaultPath);

    virtual BaseTextEditorEditable *createEditableInterface() = 0;

private slots:
    void editorContentsChange(int position, int charsRemoved, int charsAdded);
    void memorizeCursorPosition();
    void restoreCursorPosition();
    void highlightSearchResults(const QString &txt, Find::IFindSupport::FindFlags findFlags);
    void setFindScope(const QTextCursor &);
    void currentEditorChanged(Core::IEditor *editor);

private:
    Internal::BaseTextEditorPrivate *d;
    friend class Internal::BaseTextEditorPrivate;
    friend class Internal::TextEditorOverlay;

public:
    QWidget *extraArea() const;
    virtual int extraAreaWidth(int *markWidthPtr = 0) const;
    virtual void extraAreaPaintEvent(QPaintEvent *);
    virtual void extraAreaLeaveEvent(QEvent *);
    virtual void extraAreaMouseEvent(QMouseEvent *);

    const TabSettings &tabSettings() const;
    const DisplaySettings &displaySettings() const;

    void markBlocksAsChanged(QList<int> blockNumbers);

    void ensureCursorVisible();

    enum ExtraSelectionKind {
        CurrentLineSelection,
        ParenthesesMatchingSelection,
        CodeWarningsSelection,
        CodeSemanticsSelection,
        UnusedSymbolSelection,
        FakeVimSelection,
        OtherSelection,
        SnippetPlaceholderSelection,
        NExtraSelectionKinds
    };
    void setExtraSelections(ExtraSelectionKind kind, const QList<QTextEdit::ExtraSelection> &selections);
    QList<QTextEdit::ExtraSelection> extraSelections(ExtraSelectionKind kind) const;
    QString extraSelectionTooltip(int pos) const;

    struct BlockRange
    {
        BlockRange() : first(0), last(-1) {}
        BlockRange(int first_position, int last_position)
          : first(first_position), last(last_position)
        {}
        int first;
        int last;
        inline bool isNull() const { return last < first; }
    };

    // the blocks list must be sorted
    void setIfdefedOutBlocks(const QList<BlockRange> &blocks);

public slots:
    virtual void format();
    virtual void rewrapParagraph();
    virtual void unCommentSelection();
    virtual void setFontSettings(const TextEditor::FontSettings &);
    void setFontSettingsIfVisible(const TextEditor::FontSettings &);
    virtual void setTabSettings(const TextEditor::TabSettings &);
    virtual void setDisplaySettings(const TextEditor::DisplaySettings &);
    virtual void setBehaviorSettings(const TextEditor::BehaviorSettings &);
    virtual void setStorageSettings(const TextEditor::StorageSettings &);

protected:
    bool viewportEvent(QEvent *event);

    void resizeEvent(QResizeEvent *);
    void paintEvent(QPaintEvent *);
    void timerEvent(QTimerEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void leaveEvent(QEvent *);
    void keyReleaseEvent(QKeyEvent *);

public:
    // Returns true if key triggers an indent.
    virtual bool isElectricCharacter(const QChar &ch) const;

    void indentInsertedText(const QTextCursor &tc);

protected:
    // Returns the text to complete at the cursor position, or an empty string
    virtual QString autoComplete(QTextCursor &cursor, const QString &text) const;
    // Handles backspace. When returning true, backspace processing is stopped
    virtual bool autoBackspace(QTextCursor &cursor);
    // Hook to insert special characters on enter. Returns the number of extra blocks inserted.
    virtual int paragraphSeparatorAboutToBeInserted(QTextCursor &cursor);
    // Indent a text block based on previous line. Default does nothing
    virtual void indentBlock(QTextDocument *doc, QTextBlock block, QChar typedChar);
    // Indent at cursor. Calls indentBlock for selection or current line.
    virtual void indent(QTextDocument *doc, const QTextCursor &cursor, QChar typedChar);
    // Reindent at cursor. Selection will be adjusted according to the indentation change of the first block
    virtual void reindent(QTextDocument *doc, const QTextCursor &cursor);

    virtual bool contextAllowsAutoParentheses(const QTextCursor &cursor, const QString &textToInsert = QString()) const;

    // Returns true if the cursor is inside a comment.
    virtual bool isInComment(const QTextCursor &cursor) const;

    virtual QString insertMatchingBrace(const QTextCursor &tc, const QString &text, const QChar &la, int *skippedChars) const;

    // Returns the text that needs to be inserted
    virtual QString insertParagraphSeparator(const QTextCursor &tc) const;

    static void countBracket(QChar open, QChar close, QChar c, int *errors, int *stillopen);

    static void countBrackets(QTextCursor cursor, int from, int end, QChar open, QChar close,
                              int *errors, int *stillopen);

    struct Link
    {
        Link(const QString &fileName = QString(),
             int line = 0,
             int column = 0)
            : begin(-1)
            , end(-1)
            , fileName(fileName)
            , line(line)
            , column(column)
        {}

        bool isValid() const
        { return begin != end; }

        bool operator==(const Link &other) const
        { return begin == other.begin && end == other.end; }

        int begin;           // Link position
        int end;           // Link end position

        QString fileName;  // Target file
        int line;          // Target line
        int column;        // Target column
    };

    /*!
       Reimplement this function to enable code navigation.

       \a resolveTarget is set to true when the target of the link is relevant
       (it isn't until the link is used).
     */
    virtual Link findLinkAt(const QTextCursor &, bool resolveTarget = true);

    /*!
       Reimplement this function if you want to customize the way a link is
       opened. Returns whether the link was opened successfully.
     */
    virtual bool openLink(const Link &link);

protected slots:
    virtual void slotUpdateExtraAreaWidth();
    virtual void slotModificationChanged(bool);
    virtual void slotUpdateRequest(const QRect &r, int dy);
    virtual void slotCursorPositionChanged();
    virtual void slotUpdateBlockNotify(const QTextBlock &);

signals:
    void requestFontZoom(int zoom);
    void requestZoomReset();
    void requestBlockUpdate(const QTextBlock &);
    void requestAutoCompletion(TextEditor::ITextEditable *editor, bool forced);
    void requestQuickFix(TextEditor::ITextEditable *editor);

private:
    void indentOrUnindent(bool doIndent);
    void handleHomeKey(bool anchor);
    void handleBackspaceKey();
    void moveLineUpDown(bool up);
    void copyLineUpDown(bool up);
    void saveCurrentCursorPositionForNavigation();
    void updateHighlights();
    void updateCurrentLineHighlight();

    void drawFoldingMarker(QPainter *painter, const QPalette &pal,
                           const QRect &rect,
                           bool expanded,
                           bool active,
                           bool hovered) const;

    void toggleBlockVisible(const QTextBlock &block);
    QRect collapseBox();

    QTextBlock collapsedBlockAt(const QPoint &pos, QRect *box = 0) const;

    void updateLink(QMouseEvent *e);
    void showLink(const Link &);
    void clearLink();

    void universalHelper(); // test function for development

    // parentheses matcher
private slots:
    void _q_matchParentheses();
    void _q_highlightBlocks();
    void slotSelectionChanged();
    void _q_animateUpdate(int position, QPointF lastPos, QRectF rect);
};


class TEXTEDITOR_EXPORT BaseTextEditorEditable
  : public ITextEditable
{
    Q_OBJECT
    friend class BaseTextEditor;
public:
    BaseTextEditorEditable(BaseTextEditor *editor);
    ~BaseTextEditorEditable();

    inline BaseTextEditor *editor() const { return e; }

    // EditorInterface
    inline QWidget *widget() { return e; }
    inline Core::IFile * file() { return e->file(); }
    inline bool createNew(const QString &contents) { return e->createNew(contents); }
    inline bool open(const QString &fileName = QString())
    {
        return e->open(fileName);
    }
    inline QString displayName() const { return e->displayName(); }
    inline void setDisplayName(const QString &title) { e->setDisplayName(title); }

    inline QByteArray saveState() const { return e->saveState(); }
    inline bool restoreState(const QByteArray &state) { return e->restoreState(state); }
    QWidget *toolBar();

    // ITextEditor
    int find(const QString &string) const;

    int currentLine() const;
    int currentColumn() const;
    void gotoLine(int line, int column = 0) { e->gotoLine(line, column); }

    inline int position(
        ITextEditor::PositionOperation posOp = ITextEditor::Current
        , int at = -1) const { return e->position(posOp, at); }
    inline void convertPosition(int pos, int *line, int *column) const { e->convertPosition(pos, line, column); }
    QRect cursorRect(int pos = -1) const;

    QString contents() const;
    QString selectedText() const;
    QString textAt(int pos, int length) const;
    inline QChar characterAt(int pos) const { return e->characterAt(pos); }

    inline void triggerCompletions() { e->triggerCompletions(); } // slot?
    inline void triggerQuickFix() { e->triggerQuickFix(); } // slot?

    inline ITextMarkable *markableInterface() { return e->markableInterface(); }

    void setContextHelpId(const QString &id) { m_contextHelpId = id; }
    QString contextHelpId() const; // from IContext

    inline void setTextCodec(QTextCodec *codec) { e->setTextCodec(codec); }
    inline QTextCodec *textCodec() const { return e->textCodec(); }


    // ITextEditable
    void remove(int length);
    void insert(const QString &string);
    void replace(int length, const QString &string);
    void setCurPos(int pos);
    void select(int toPos);

private slots:
    void updateCursorPosition();

private:
    BaseTextEditor *e;
    mutable QString m_contextHelpId;
    QToolBar *m_toolBar;
    Utils::LineColumnLabel *m_cursorPositionLabel;
};

} // namespace TextEditor

#endif // BASETEXTEDITOR_H
