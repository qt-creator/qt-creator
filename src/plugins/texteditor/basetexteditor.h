/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef BASETEXTEDITOR_H
#define BASETEXTEDITOR_H

#include "itexteditor.h"
#include "codeassist/assistenums.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/id.h>
#include <find/ifindsupport.h>

#include <QPlainTextEdit>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QToolBar;
class QTimeLine;
class QPrinter;
QT_END_NAMESPACE

namespace Utils {
    class LineColumnLabel;
    class ChangeSet;
}

namespace TextEditor {
class TabSettings;
class RefactorOverlay;
struct RefactorMarker;
class IAssistMonitorInterface;
class IAssistInterface;
class IAssistProvider;
class ICodeStylePreferences;
typedef QList<RefactorMarker> RefactorMarkers;

namespace Internal {
    class BaseTextEditorWidgetPrivate;
    class TextEditorOverlay;
    typedef QString (QString::*TransformationMethod)() const;
}

class ITextMarkable;

class BaseTextDocument;
class BaseTextEditor;
class FontSettings;
class BehaviorSettings;
class CompletionSettings;
class DisplaySettings;
class TypingSettings;
class StorageSettings;
class Indenter;
class AutoCompleter;
class ExtraEncodingSettings;

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


class TEXTEDITOR_EXPORT BaseTextEditorWidget : public QPlainTextEdit
{
    Q_OBJECT
    Q_PROPERTY(int verticalBlockSelectionFirstColumn READ verticalBlockSelectionFirstColumn)
    Q_PROPERTY(int verticalBlockSelectionLastColumn READ verticalBlockSelectionLastColumn)
public:
    BaseTextEditorWidget(QWidget *parent);
    ~BaseTextEditorWidget();

    static Core::IEditor *openEditorAt(const QString &fileName, int line, int column = 0,
                                     const Core::Id &editorId =  Core::Id(),
                                     Core::EditorManager::OpenEditorFlags flags = Core::EditorManager::IgnoreNavigationHistory,
                                     bool *newEditor = 0);

    const Utils::ChangeSet &changeSet() const;
    void setChangeSet(const Utils::ChangeSet &changeSet);

    // EditorInterface
    Core::IDocument *editorDocument() const;
    bool createNew(const QString &contents);
    virtual bool open(QString *errorString, const QString &fileName, const QString &realFileName);
    QByteArray saveState() const;
    bool restoreState(const QByteArray &state);
    QString displayName() const;

    // ITextEditor

    void gotoLine(int line, int column = 0);

    int position(
        ITextEditor::PositionOperation posOp = ITextEditor::Current
        , int at = -1) const;
    void convertPosition(int pos, int *line, int *column) const;

    BaseTextEditor *editor() const;
    ITextMarkable *markableInterface() const;

    QChar characterAt(int pos) const;

    void print(QPrinter *);

    void setSuggestedFileName(const QString &suggestedFileName);
    QString mimeType() const;
    virtual void setMimeType(const QString &mt);

    void appendMenuActionsFromContext(QMenu *menu, const Core::Id menuContextId);
    void appendStandardContextMenuActions(QMenu *menu);

    // Works only in conjunction with a syntax highlighter that puts
    // parentheses into text block user data
    void setParenthesesMatchingEnabled(bool b);
    bool isParenthesesMatchingEnabled() const;

    void setHighlightCurrentLine(bool b);
    bool highlightCurrentLine() const;

    void setLineNumbersVisible(bool b);
    bool lineNumbersVisible() const;

    void setOpenLinksInNextSplit(bool b);
    bool openLinksInNextSplit() const;

    void setForceOpenLinksInNextSplit(bool b);
    bool forceOpenLinksInNextSplit() const;

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

    QPoint toolTipPosition(const QTextCursor &c) const;

    void invokeAssist(AssistKind assistKind, IAssistProvider *provider = 0);

    virtual IAssistInterface *createAssistInterface(AssistKind assistKind,
                                                    AssistReason assistReason) const;
    QMimeData *duplicateMimeData(const QMimeData *source) const;

public slots:
    void setDisplayName(const QString &title);

    virtual void copy();
    virtual void paste();
    virtual void cut();
    virtual void selectAll();

    void circularPaste();
    void switchUtf8bom();

    void zoomIn(int range = 1);
    void zoomOut(int range = 1);
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

    void openLinkUnderCursor();

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

    QMimeData *createMimeDataFromSelection() const;
    bool canInsertFromMimeData(const QMimeData *source) const;
    void insertFromMimeData(const QMimeData *source);

    virtual QString plainTextFromSelection() const;
    static QString convertToPlainText(const QString &txt);

    virtual QString lineNumber(int blockNumber) const;
    virtual int lineNumberTopPositionOffset(int blockNumber) const;
    virtual int lineNumberDigits() const;

    static QString msgTextTooLarge(quint64 size);

private:
    void maybeSelectLine();
    void updateCannotDecodeInfo();

public:
    void duplicateFrom(BaseTextEditorWidget *editor);

protected:
    QSharedPointer<BaseTextDocument> baseTextDocument() const;
    void setBaseTextDocument(const QSharedPointer<BaseTextDocument> &doc);

    void setDefaultPath(const QString &defaultPath);

    virtual BaseTextEditor *createEditor() = 0;

private slots:
    void editorContentsChange(int position, int charsRemoved, int charsAdded);
    void documentAboutToBeReloaded();
    void documentReloaded();
    void highlightSearchResults(const QString &txt, Find::FindFlags findFlags);
    void setFindScope(const QTextCursor &start, const QTextCursor &end, int, int);
    bool inFindScope(const QTextCursor &cursor);
    bool inFindScope(int selectionStart, int selectionEnd);
    void inSnippetMode(bool *active);
    void onCodeStylePreferencesDestroyed();

private:
    Internal::BaseTextEditorWidgetPrivate *d;
    friend class Internal::BaseTextEditorWidgetPrivate;
    friend class Internal::TextEditorOverlay;
    friend class RefactorOverlay;

public:
    QWidget *extraArea() const;
    virtual int extraAreaWidth(int *markWidthPtr = 0) const;
    virtual void extraAreaPaintEvent(QPaintEvent *);
    virtual void extraAreaLeaveEvent(QEvent *);
    virtual void extraAreaContextMenuEvent(QContextMenuEvent *);
    virtual void extraAreaMouseEvent(QMouseEvent *);

    const TabSettings &tabSettings() const;
    void setLanguageSettingsId(Core::Id settingsId);
    Core::Id languageSettingsId() const;

    void setCodeStyle(ICodeStylePreferences *settings);

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
        DebuggerExceptionSelection,
        NExtraSelectionKinds
    };
    void setExtraSelections(ExtraSelectionKind kind, const QList<QTextEdit::ExtraSelection> &selections);
    QList<QTextEdit::ExtraSelection> extraSelections(ExtraSelectionKind kind) const;
    QString extraSelectionTooltip(int pos) const;

    RefactorMarkers refactorMarkers() const;
    void setRefactorMarkers(const RefactorMarkers &markers);
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
    virtual void setTypingSettings(const TextEditor::TypingSettings &);
    virtual void setStorageSettings(const TextEditor::StorageSettings &);
    virtual void setCompletionSettings(const TextEditor::CompletionSettings &);
    virtual void setExtraEncodingSettings(const TextEditor::ExtraEncodingSettings &);

protected:
    bool viewportEvent(QEvent *event);

    void resizeEvent(QResizeEvent *);
    void paintEvent(QPaintEvent *);
    void timerEvent(QTimerEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseDoubleClickEvent(QMouseEvent *);
    void leaveEvent(QEvent *);
    void keyReleaseEvent(QKeyEvent *);

    void dragEnterEvent(QDragEnterEvent *e);

    void showDefaultContextMenu(QContextMenuEvent *e, const Core::Id menuContextId);

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

    /*!
      Reimplement this function to change the default replacement text.
      */
    virtual QString foldReplacementText(const QTextBlock &block) const;

protected slots:
    virtual void slotUpdateExtraArea();
    virtual void slotUpdateExtraAreaWidth();
    virtual void slotModificationChanged(bool);
    virtual void slotUpdateRequest(const QRect &r, int dy);
    virtual void slotCursorPositionChanged();
    virtual void slotUpdateBlockNotify(const QTextBlock &);
    virtual void slotCodeStyleSettingsChanged(const QVariant &);

signals:
    void requestFontZoom(int zoom);
    void requestZoomReset();
    void requestBlockUpdate(const QTextBlock &);

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

    void processTooltipRequest(const QTextCursor &c);

    void transformSelection(Internal::TransformationMethod method);
    void transformBlockSelection(Internal::TransformationMethod method);

private slots:
    void handleBlockSelection(int diff_row, int diff_col);

    // parentheses matcher
    void _q_matchParentheses();
    void _q_highlightBlocks();
    void slotSelectionChanged();
    void _q_animateUpdate(int position, QPointF lastPos, QRectF rect);
    void doFoo();
};


class TEXTEDITOR_EXPORT BaseTextEditor : public ITextEditor
{
    Q_OBJECT

public:
    BaseTextEditor(BaseTextEditorWidget *editorWidget);
    ~BaseTextEditor();

    friend class BaseTextEditorWidget;
    BaseTextEditorWidget *editorWidget() const { return e; }

    // EditorInterface
    //QWidget *widget() { return e; }
    Core::IDocument * document() { return e->editorDocument(); }
    bool createNew(const QString &contents) { return e->createNew(contents); }
    bool open(QString *errorString, const QString &fileName, const QString &realFileName) { return e->open(errorString, fileName, realFileName); }
    QString displayName() const { return e->displayName(); }
    void setDisplayName(const QString &title) { e->setDisplayName(title); emit changed(); }

    QByteArray saveState() const { return e->saveState(); }
    bool restoreState(const QByteArray &state) { return e->restoreState(state); }
    QWidget *toolBar();

    enum Side { Left, Right };
    void insertExtraToolBarWidget(Side side, QWidget *widget);

    // ITextEditor
    int find(const QString &string) const;
    int currentLine() const;
    int currentColumn() const;
    void gotoLine(int line, int column = 0) { e->gotoLine(line, column); }
    int columnCount() const;
    int rowCount() const;

    int position(PositionOperation posOp = Current, int at = -1) const
    { return e->position(posOp, at); }
    void convertPosition(int pos, int *line, int *column) const
    { e->convertPosition(pos, line, column); }
    QRect cursorRect(int pos = -1) const;

    QString contents() const;
    QString selectedText() const;
    QString textAt(int pos, int length) const;
    inline QChar characterAt(int pos) const { return e->characterAt(pos); }

    inline ITextMarkable *markableInterface() { return e->markableInterface(); }

    QString contextHelpId() const; // from IContext

    inline void setTextCodec(QTextCodec *codec, TextCodecReason = TextCodecOtherReason) { e->setTextCodec(codec); }
    inline QTextCodec *textCodec() const { return e->textCodec(); }


    // ITextEditor
    void remove(int length);
    void insert(const QString &string);
    void replace(int length, const QString &string);
    void setCursorPosition(int pos);
    void select(int toPos);

private slots:
    void updateCursorPosition();

private:
    BaseTextEditorWidget *e;
    QToolBar *m_toolBar;
    QWidget *m_stretchWidget;
    QAction *m_cursorPositionLabelAction;
    Utils::LineColumnLabel *m_cursorPositionLabel;
};

} // namespace TextEditor

Q_DECLARE_METATYPE(TextEditor::BaseTextEditorWidget::Link)

#endif // BASETEXTEDITOR_H
