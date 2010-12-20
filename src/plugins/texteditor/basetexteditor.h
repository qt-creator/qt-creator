/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef BASETEXTEDITOR_H
#define BASETEXTEDITOR_H

#include "itexteditable.h"

#include <find/ifindsupport.h>

#include <coreplugin/editormanager/editormanager.h>

#include <QtGui/QPlainTextEdit>

QT_BEGIN_NAMESPACE
class QToolBar;
class QTimeLine;
QT_END_NAMESPACE

namespace Utils {
    class LineColumnLabel;
    class ChangeSet;
}

namespace TextEditor {
class TabSettings;
class RefactorOverlay;
struct RefactorMarker;

namespace Internal {
    class BaseTextEditorPrivate;
    class TextEditorOverlay;
    typedef QList<RefactorMarker> RefactorMarkers;
}

class ITextMarkable;

class BaseTextDocument;
class BaseTextEditorEditable;
class FontSettings;
class BehaviorSettings;
class CompletionSettings;
class DisplaySettings;
class StorageSettings;
class Indenter;
class AutoCompleter;

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
    Q_PROPERTY(int verticalBlockSelectionFirstColumn READ verticalBlockSelectionFirstColumn)
    Q_PROPERTY(int verticalBlockSelectionLastColumn READ verticalBlockSelectionLastColumn)
public:
    BaseTextEditor(QWidget *parent);
    ~BaseTextEditor();

    static ITextEditor *openEditorAt(const QString &fileName, int line, int column = 0,
                                     const QString &editorId =  QString(),
                                     Core::EditorManager::OpenEditorFlags flags = Core::EditorManager::IgnoreNavigationHistory,
                                     bool *newEditor = 0);

    const Utils::ChangeSet &changeSet() const;
    void setChangeSet(const Utils::ChangeSet &changeSet);

    // EditorInterface
    Core::IFile * file();
    bool createNew(const QString &contents);
    virtual bool open(const QString &fileName = QString());
    QByteArray saveState() const;
    bool restoreState(const QByteArray &state);
    QString displayName() const;

    // ITextEditor

    void gotoLine(int line, int column = 0);

    int position(
        ITextEditor::PositionOperation posOp = ITextEditor::Current
        , int at = -1) const;
    void convertPosition(int pos, int *line, int *column) const;

    BaseTextEditorEditable *editableInterface() const;
    ITextMarkable *markableInterface() const;

    virtual void triggerCompletions();
    virtual void triggerQuickFix();

    QChar characterAt(int pos) const;

    void print(QPrinter *);

    void setSuggestedFileName(const QString &suggestedFileName);
    QString mimeType() const;
    virtual void setMimeType(const QString &mt);


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

    void updateCodeFoldingVisible();
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

    void insertCodeSnippet(const QTextCursor &cursor, const QString &snippet);

    void setBlockSelection(bool on);
    bool hasBlockSelection() const;

    int verticalBlockSelectionFirstColumn() const;
    int verticalBlockSelectionLastColumn() const;

    QRegion translatedLineRegion(int lineStart, int lineEnd) const;

    void setIndenter(Indenter *indenter);
    Indenter *indenter() const;

    void setAutoCompleter(AutoCompleter *autoCompleter);
    AutoCompleter *autoCompleter() const;

public slots:
    void setDisplayName(const QString &title);

    virtual void paste();
    virtual void cut();

    void zoomIn(int range = 1);
    void zoomOut(int range = 1);
    void zoomReset();

    void cutLine();
    void deleteLine();
    void unfoldAll();
    void fold();
    void unfold();
    void selectEncoding();

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

    void selectBlockUp();
    void selectBlockDown();

    void moveLineUp();
    void moveLineDown();

    void copyLineUp();
    void copyLineDown();

    void joinLines();

    void insertLineAbove();
    void insertLineBelow();

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

    static QString msgTextTooLarge(quint64 size);

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
    void documentAboutToBeReloaded();
    void documentReloaded();
    void highlightSearchResults(const QString &txt, Find::FindFlags findFlags);
    void setFindScope(const QTextCursor &start, const QTextCursor &end, int, int);
    bool inFindScope(const QTextCursor &cursor);
    bool inFindScope(int selectionStart, int selectionEnd);
    void currentEditorChanged(Core::IEditor *editor);

private:
    Internal::BaseTextEditorPrivate *d;
    friend class Internal::BaseTextEditorPrivate;
    friend class Internal::TextEditorOverlay;
    friend class RefactorOverlay;

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
        UndefinedSymbolSelection,
        UnusedSymbolSelection,
        FakeVimSelection,
        OtherSelection,
        SnippetPlaceholderSelection,
        ObjCSelection,
        NExtraSelectionKinds
    };
    void setExtraSelections(ExtraSelectionKind kind, const QList<QTextEdit::ExtraSelection> &selections);
    QList<QTextEdit::ExtraSelection> extraSelections(ExtraSelectionKind kind) const;
    QString extraSelectionTooltip(int pos) const;


    void setRefactorMarkers(const Internal::RefactorMarkers &markers);
signals:
    void refactorMarkerClicked(const TextEditor::RefactorMarker &marker);

public:

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
    virtual void setCompletionSettings(const TextEditor::CompletionSettings &);

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

    void dragEnterEvent(QDragEnterEvent *e);

public:
    void indentInsertedText(const QTextCursor &tc);
    void indent(QTextDocument *doc, const QTextCursor &cursor, QChar typedChar);
    void reindent(QTextDocument *doc, const QTextCursor &cursor);

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

protected:
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

    void maybeClearSomeExtraSelections(const QTextCursor &cursor);

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
    void maybeRequestAutoCompletion(const QChar &ch);
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

    void drawCollapsedBlockPopup(QPainter &painter,
                                 const QTextBlock &block,
                                 QPointF offset,
                                 const QRect &clip);

    void toggleBlockVisible(const QTextBlock &block);
    QRect foldBox();

    QTextBlock foldedBlockAt(const QPoint &pos, QRect *box = 0) const;

    void updateLink(QMouseEvent *e);
    void showLink(const Link &);
    void clearLink();

    void universalHelper(); // test function for development

    bool cursorMoveKeyEvent(QKeyEvent *e);
    bool camelCaseRight(QTextCursor &cursor, QTextCursor::MoveMode mode);
    bool camelCaseLeft(QTextCursor &cursor, QTextCursor::MoveMode mode);


private slots:
    // auto completion
    void _q_requestAutoCompletion();

    void handleBlockSelection(int diff_row, int diff_col);

    // parentheses matcher
    void _q_matchParentheses();
    void _q_highlightBlocks();
    void slotSelectionChanged();
    void _q_animateUpdate(int position, QPointF lastPos, QRectF rect);
    void doFoo();

};


class TEXTEDITOR_EXPORT BaseTextEditorEditable : public ITextEditable
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
    inline void setDisplayName(const QString &title) { e->setDisplayName(title); emit changed(); }

    inline QByteArray saveState() const { return e->saveState(); }
    inline bool restoreState(const QByteArray &state) { return e->restoreState(state); }
    virtual QWidget *toolBar();

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

    inline void setTextCodec(QTextCodec *codec, TextCodecReason = TextCodecOtherReason) { e->setTextCodec(codec); }
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

Q_DECLARE_METATYPE(TextEditor::BaseTextEditor::Link)

#endif // BASETEXTEDITOR_H
