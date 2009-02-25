/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/


#ifndef BASETEXTEDITOR_H
#define BASETEXTEDITOR_H

#include "displaysettings.h"
#include "tabsettings.h"
#include "interactionsettings.h"
#include "itexteditable.h"

#include <QtGui/QPlainTextEdit>
#include <QtGui/QLabel>
#include <QtGui/QKeyEvent>

QT_BEGIN_NAMESPACE
class QLabel;
class QTextCharFormat;
class QToolBar;
QT_END_NAMESPACE

namespace Core {
    namespace Utils {
        class LineColumnLabel;
    }
}

namespace TextEditor {

namespace Internal {
    class BaseTextEditorPrivate;
}

class ITextMark;
class ITextMarkable;

class TextEditorActionHandler;
class BaseTextDocument;
class FontSettings;
struct StorageSettings;

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

    inline bool setIfdefedOut() { bool result = m_ifdefedOut; m_ifdefedOut = true; return !result; }
    inline bool clearIfdefedOut() { bool result = m_ifdefedOut; m_ifdefedOut = false; return result;}
    inline bool ifdefedOut() const { return m_ifdefedOut; }

    inline static TextBlockUserData *canCollapse(const QTextBlock& block) {
        TextBlockUserData *data = static_cast<TextBlockUserData*>(block.userData());
        if (!data || data->collapseMode() != CollapseAfter) {
            data = static_cast<TextBlockUserData*>(block.next().userData());
            if (!data || data->collapseMode() != TextBlockUserData::CollapseThis || data->m_ifdefedOut)
                data = 0;
        }
        return data;
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

    int collapseAtPos() const;

    enum MatchType { NoMatch, Match, Mismatch  };
    static MatchType checkOpenParenthesis(QTextCursor *cursor, QChar c);
    static MatchType checkClosedParenthesis(QTextCursor *cursor, QChar c);
    static MatchType matchCursorBackward(QTextCursor *cursor);
    static MatchType matchCursorForward(QTextCursor *cursor);
    static bool findPreviousOpenParenthesis(QTextCursor *cursor, bool select = false);
    static bool findNextClosingParenthesis(QTextCursor *cursor, bool select = false);


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

class TEXTEDITOR_EXPORT BaseTextEditor
  : public QPlainTextEdit
{
    Q_OBJECT

public:
    BaseTextEditor(QWidget *parent);
    ~BaseTextEditor();

    static ITextEditor *openEditorAt(const QString &fileName, int line, int column = 0,
                                     const QString &editorKind = QString());

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

    QChar characterAt(int pos) const;

    void print(QPrinter *);

    void setSuggestedFileName(const QString &suggestedFileName);
    QString mimeType() const;
    void setMimeType(const QString &mt);


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

public slots:
    void setDisplayName(const QString &title);
    virtual void setFontSettings(const TextEditor::FontSettings &);
    virtual void format();
    virtual void unCommentSelection();
    virtual void setStorageSettings(const TextEditor::StorageSettings &);

    void paste();
    void cut();

    void zoomIn(int range = 1);
    void zoomOut(int range = 1);

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

    // reimplemented to support block selection
    QMimeData *createMimeDataFromSelection() const;
    bool canInsertFromMimeData(const QMimeData *source) const;
    void insertFromMimeData(const QMimeData *source);

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
    void highlightSearchResults(const QString &txt, QTextDocument::FindFlags findFlags);
    void setFindScope(const QTextCursor &);
    void setCollapseIndicatorAlpha(int);
    void currentEditorChanged(Core::IEditor *editor);

private:
    Internal::BaseTextEditorPrivate *d;
    friend class Internal::BaseTextEditorPrivate;

public:
    QWidget *extraArea() const;
    virtual int extraAreaWidth(int *markWidthPtr = 0) const;
    virtual void extraAreaPaintEvent(QPaintEvent *);
    virtual void extraAreaMouseEvent(QMouseEvent *);
    virtual void extraAreaLeaveEvent(QEvent *);

    const TabSettings &tabSettings() const;
    const DisplaySettings &displaySettings() const;
    const InteractionSettings &interactionSettings() const;

    void markBlocksAsChanged(QList<int> blockNumbers);

    void ensureCursorVisible();

    enum ExtraSelectionKind {
        CurrentLineSelection,
        ParenthesesMatchingSelection,
        CodeWarningsSelection,
        CodeSemanticsSelection,
        OtherSelection,
        FakeVimSelection,
        NExtraSelectionKinds
    };
    void setExtraSelections(ExtraSelectionKind kind, const QList<QTextEdit::ExtraSelection> &selections);
    QList<QTextEdit::ExtraSelection> extraSelections(ExtraSelectionKind kind) const;

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
    virtual void setTabSettings(const TextEditor::TabSettings &);
    virtual void setDisplaySettings(const TextEditor::DisplaySettings &);

protected:
    bool viewportEvent(QEvent *event);

    void resizeEvent(QResizeEvent *);
    void paintEvent(QPaintEvent *);
    void timerEvent(QTimerEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void mousePressEvent(QMouseEvent *);

    // Rertuns true if key triggers an indent.
    virtual bool isElectricCharacter(const QChar &ch) const;
    // Indent a text block based on previous line. Default does nothing
    virtual void indentBlock(QTextDocument *doc, QTextBlock block, QChar typedChar);
    // Indent at cursor. Calls indentBlock for selection or current line.
    virtual void indent(QTextDocument *doc, const QTextCursor &cursor, QChar typedChar);


protected slots:
    virtual void slotUpdateExtraAreaWidth();
    virtual void slotModificationChanged(bool);
    virtual void slotUpdateRequest(const QRect &r, int dy);
    virtual void slotCursorPositionChanged();
    virtual void slotUpdateBlockNotify(const QTextBlock &);

signals:
    void requestBlockUpdate(const QTextBlock &);
    void requestAutoCompletion(ITextEditable *editor, bool forced);

private:
    void indentOrUnindent(bool doIndent);
    void handleHomeKey(bool anchor);
    void handleBackspaceKey();
    void moveLineUpDown(bool up);

    void toggleBlockVisible(const QTextBlock &block);
    QRect collapseBox(const QTextBlock &block);

    QTextBlock collapsedBlockAt(const QPoint &pos, QRect *box = 0) const;

    // parentheses matcher
private slots:
    void _q_matchParentheses();
    void slotSelectionChanged();
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
    QToolBar *toolBar();

    // ITextEditor
    int find(const QString &string) const;

    int currentLine() const;
    int currentColumn() const;
    inline void gotoLine(int line, int column = 0) { e->gotoLine(line, column); }

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
    Core::Utils::LineColumnLabel *m_cursorPositionLabel;
};

} // namespace TextEditor

#endif // BASETEXTEDITOR_H
