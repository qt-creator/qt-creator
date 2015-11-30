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

#include "texteditor.h"
#include "texteditor_p.h"
#include "displaysettings.h"
#include "marginsettings.h"
#include "fontsettings.h"
#include "texteditoractionhandler.h"

#include "autocompleter.h"
#include "basehoverhandler.h"
#include "behaviorsettings.h"
#include "circularclipboard.h"
#include "circularclipboardassist.h"
#include "codecselector.h"
#include "completionsettings.h"
#include "convenience.h"
#include "highlighterutils.h"
#include "icodestylepreferences.h"
#include "indenter.h"
#include "snippets/snippet.h"
#include "syntaxhighlighter.h"
#include "tabsettings.h"
#include "textdocument.h"
#include "textdocumentlayout.h"
#include "texteditoroverlay.h"
#include "refactoroverlay.h"
#include "texteditorsettings.h"
#include "typingsettings.h"
#include "extraencodingsettings.h"
#include "storagesettings.h"

#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/codeassistant.h>
#include <texteditor/codeassist/completionassistprovider.h>
#include <texteditor/generichighlighter/context.h>
#include <texteditor/generichighlighter/highlightdefinition.h>
#include <texteditor/generichighlighter/highlighter.h>
#include <texteditor/generichighlighter/highlightersettings.h>
#include <texteditor/generichighlighter/manager.h>

#include <coreplugin/icore.h>
#include <aggregation/aggregate.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/infobar.h>
#include <coreplugin/manhattanstyle.h>
#include <coreplugin/find/basetextfind.h>
#include <coreplugin/find/highlightscrollbar.h>
#include <utils/linecolumnlabel.h>
#include <utils/fileutils.h>
#include <utils/dropsupport.h>
#include <utils/filesearch.h>
#include <utils/hostosinfo.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>
#include <utils/tooltip/tooltip.h>
#include <utils/uncommentselection.h>
#include <utils/theme/theme.h>

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QClipboard>
#include <QCoreApplication>
#include <QDebug>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QPrintDialog>
#include <QPrinter>
#include <QDrag>
#include <QScrollBar>
#include <QShortcut>
#include <QStyle>
#include <QTextBlock>
#include <QTextCodec>
#include <QTextCursor>
#include <QTextDocumentFragment>
#include <QTextLayout>
#include <QTime>
#include <QTimeLine>
#include <QTimer>
#include <QToolBar>

//#define DO_FOO

/*!
    \namespace TextEditor
    \brief The TextEditor namespace contains the base text editor and several classes which
    provide supporting functionality like snippets, highlighting, \l{CodeAssist}{code assist},
    indentation and style, and others.
*/

/*!
    \namespace TextEditor::Internal
    \internal
*/

/*!
    \class TextEditor::BaseTextEditor
    \brief The BaseTextEditor class is base implementation for QPlainTextEdit-based
    text editors. It can use the Kate text highlighting definitions, and some basic
    auto indentation.

    The corresponding document base class is BaseTextDocument, the corresponding
    widget base class is BaseTextEditorWidget.

    It is the default editor for text files used by \QC, if no other editor
    implementation matches the MIME type.
*/


using namespace Core;
using namespace Utils;

namespace TextEditor {
namespace Internal {

enum { NExtraSelectionKinds = 12 };

typedef QString (TransformationMethod)(const QString &);

static QString QString_toUpper(const QString &str)
{
    return str.toUpper();
}

static QString QString_toLower(const QString &str)
{
    return str.toLower();
}

class TextEditorAnimator : public QObject
{
    Q_OBJECT

public:
    TextEditorAnimator(QObject *parent);

    inline void setPosition(int position) { m_position = position; }
    inline int position() const { return m_position; }

    void setData(const QFont &f, const QPalette &pal, const QString &text);

    void draw(QPainter *p, const QPointF &pos);
    QRectF rect() const;

    inline qreal value() const { return m_value; }
    inline QPointF lastDrawPos() const { return m_lastDrawPos; }

    void finish();

    bool isRunning() const;

signals:
    void updateRequest(int position, QPointF lastPos, QRectF rect);

private:
    void step(qreal v);

    QTimeLine m_timeline;
    qreal m_value;
    int m_position;
    QPointF m_lastDrawPos;
    QFont m_font;
    QPalette m_palette;
    QString m_text;
    QSizeF m_size;
};

class TextEditExtraArea : public QWidget
{
public:
    TextEditExtraArea(TextEditorWidget *edit)
        : QWidget(edit)
    {
        textEdit = edit;
        setAutoFillBackground(true);
    }

protected:
    QSize sizeHint() const {
        return QSize(textEdit->extraAreaWidth(), 0);
    }
    void paintEvent(QPaintEvent *event) {
        textEdit->extraAreaPaintEvent(event);
    }
    void mousePressEvent(QMouseEvent *event) {
        textEdit->extraAreaMouseEvent(event);
    }
    void mouseMoveEvent(QMouseEvent *event) {
        textEdit->extraAreaMouseEvent(event);
    }
    void mouseReleaseEvent(QMouseEvent *event) {
        textEdit->extraAreaMouseEvent(event);
    }
    void leaveEvent(QEvent *event) {
        textEdit->extraAreaLeaveEvent(event);
    }
    void contextMenuEvent(QContextMenuEvent *event) {
        textEdit->extraAreaContextMenuEvent(event);
    }

    void wheelEvent(QWheelEvent *event) {
        QCoreApplication::sendEvent(textEdit->viewport(), event);
    }

private:
    TextEditorWidget *textEdit;
};

class BaseTextEditorPrivate
{
public:
    BaseTextEditorPrivate() {}

    TextEditorFactoryPrivate *m_origin;
};

class TextEditorWidgetPrivate : public QObject
{
public:
    TextEditorWidgetPrivate(TextEditorWidget *parent);
    ~TextEditorWidgetPrivate();

    void setupDocumentSignals();
    void updateLineSelectionColor();

    void print(QPrinter *printer);

    void maybeSelectLine();
    void updateCannotDecodeInfo();
    void collectToCircularClipboard();

    void ctor(const QSharedPointer<TextDocument> &doc);
    void handleHomeKey(bool anchor);
    void handleBackspaceKey();
    void moveLineUpDown(bool up);
    void copyLineUpDown(bool up);
    void saveCurrentCursorPositionForNavigation();
    void updateHighlights();
    void updateCurrentLineInScrollbar();
    void updateCurrentLineHighlight();

    void drawFoldingMarker(QPainter *painter, const QPalette &pal,
                           const QRect &rect,
                           bool expanded,
                           bool active,
                           bool hovered) const;

    void toggleBlockVisible(const QTextBlock &block);
    QRect foldBox();

    QTextBlock foldedBlockAt(const QPoint &pos, QRect *box = 0) const;

    void requestUpdateLink(QMouseEvent *e, bool immediate = false);
    void updateLink();
    void showLink(const TextEditorWidget::Link &);
    void clearLink();

    void universalHelper(); // test function for development

    bool cursorMoveKeyEvent(QKeyEvent *e);
    bool camelCaseRight(QTextCursor &cursor, QTextCursor::MoveMode mode);
    bool camelCaseLeft(QTextCursor &cursor, QTextCursor::MoveMode mode);

    void processTooltipRequest(const QTextCursor &c);

    void transformSelection(TransformationMethod method);
    void transformBlockSelection(TransformationMethod method);

    void slotUpdateExtraAreaWidth();
    void slotUpdateRequest(const QRect &r, int dy);
    void slotUpdateBlockNotify(const QTextBlock &);
    void updateTabStops();
    void applyFontSettingsDelayed();

    void editorContentsChange(int position, int charsRemoved, int charsAdded);
    void documentAboutToBeReloaded();
    void documentReloadFinished(bool success);
    void highlightSearchResultsSlot(const QString &txt, FindFlags findFlags);
    void searchResultsReady(int beginIndex, int endIndex);
    void searchFinished();
    void setupScrollBar();
    void highlightSearchResultsInScrollBar();
    void scheduleUpdateHighlightScrollBar();
    void updateHighlightScrollBarNow();
    struct SearchResult {
        int start;
        int length;
    };
    void addSearchResultsToScrollBar(QVector<SearchResult> results);
    void adjustScrollBarRanges();

    void setFindScope(const QTextCursor &start, const QTextCursor &end, int, int);

    void updateCursorPosition();

    // parentheses matcher
    void _q_matchParentheses();
    void _q_highlightBlocks();
    void slotSelectionChanged();
    void _q_animateUpdate(int position, QPointF lastPos, QRectF rect);
    void updateCodeFoldingVisible();

    void reconfigure();

public:
    TextEditorWidget *q;
    QToolBar *m_toolBar;
    QWidget *m_stretchWidget;
    LineColumnLabel *m_cursorPositionLabel;
    LineColumnLabel *m_fileEncodingLabel;
    QAction *m_cursorPositionLabelAction;
    QAction *m_fileEncodingLabelAction;

    bool m_contentsChanged;
    bool m_lastCursorChangeWasInteresting;

    QSharedPointer<TextDocument> m_document;
    QByteArray m_tempState;
    QByteArray m_tempNavigationState;

    bool m_parenthesesMatchingEnabled;

    // parentheses matcher
    bool m_formatRange;
    QTimer m_parenthesesMatchingTimer;
    // end parentheses matcher

    QWidget *m_extraArea;

    Id m_tabSettingsId;
    ICodeStylePreferences *m_codeStylePreferences;
    DisplaySettings m_displaySettings;
    MarginSettings m_marginSettings;
    bool m_fontSettingsNeedsApply;
    BehaviorSettings m_behaviorSettings;

    int extraAreaSelectionAnchorBlockNumber;
    int extraAreaToggleMarkBlockNumber;
    int extraAreaHighlightFoldedBlockNumber;

    TextEditorOverlay *m_overlay;
    TextEditorOverlay *m_snippetOverlay;
    TextEditorOverlay *m_searchResultOverlay;
    bool snippetCheckCursor(const QTextCursor &cursor);
    void snippetTabOrBacktab(bool forward);

    RefactorOverlay *m_refactorOverlay;
    QString m_contextHelpId;

    QBasicTimer foldedBlockTimer;
    int visibleFoldedBlockNumber;
    int suggestedVisibleFoldedBlockNumber;
    void clearVisibleFoldedBlock();
    bool m_mouseOnFoldedMarker;
    void foldLicenseHeader();

    QBasicTimer autoScrollTimer;
    uint m_marksVisible : 1;
    uint m_codeFoldingVisible : 1;
    uint m_codeFoldingSupported : 1;
    uint m_revisionsVisible : 1;
    uint m_lineNumbersVisible : 1;
    uint m_highlightCurrentLine : 1;
    uint m_requestMarkEnabled : 1;
    uint m_lineSeparatorsAllowed : 1;
    uint autoParenthesisOverwriteBackup : 1;
    uint surroundWithEnabledOverwriteBackup : 1;
    uint m_maybeFakeTooltipEvent : 1;
    int m_visibleWrapColumn;

    TextEditorWidget::Link m_currentLink;
    bool m_linkPressed;
    QTextCursor m_pendingLinkUpdate;
    QTextCursor m_lastLinkUpdate;

    QRegExp m_searchExpr;
    FindFlags m_findFlags;
    void highlightSearchResults(const QTextBlock &block, TextEditorOverlay *overlay);
    QTimer m_delayedUpdateTimer;

    void setExtraSelections(Core::Id kind, const QList<QTextEdit::ExtraSelection> &selections);
    QHash<Core::Id, QList<QTextEdit::ExtraSelection>> m_extraSelections;

    // block selection mode
    bool m_inBlockSelectionMode;
    QString copyBlockSelection();
    void insertIntoBlockSelection(const QString &text = QString());
    void setCursorToColumn(QTextCursor &cursor, int column,
                          QTextCursor::MoveMode moveMode = QTextCursor::MoveAnchor);
    void removeBlockSelection();
    void enableBlockSelection(const QTextCursor &cursor);
    void enableBlockSelection(int positionBlock, int positionColumn,
                              int anchorBlock, int anchorColumn);
    void disableBlockSelection(bool keepSelection = true);
    void resetCursorFlashTimer();
    QBasicTimer m_cursorFlashTimer;
    bool m_cursorVisible;
    bool m_moveLineUndoHack;

    QTextCursor m_findScopeStart;
    QTextCursor m_findScopeEnd;
    int m_findScopeVerticalBlockSelectionFirstColumn;
    int m_findScopeVerticalBlockSelectionLastColumn;

    QTextCursor m_selectBlockAnchor;

    TextBlockSelection m_blockSelection;

    void moveCursorVisible(bool ensureVisible = true);

    int visualIndent(const QTextBlock &block) const;
    TextEditorPrivateHighlightBlocks m_highlightBlocksInfo;
    QTimer m_highlightBlocksTimer;

    CodeAssistant m_codeAssistant;
    bool m_assistRelevantContentAdded;
    QList<BaseHoverHandler *> m_hoverHandlers; // Not owned

    QPointer<TextEditorAnimator> m_animator;
    int m_cursorBlockNumber;
    int m_blockCount;

    QPoint m_markDragStart;
    bool m_markDragging;

    QScopedPointer<ClipboardAssistProvider> m_clipboardAssistProvider;

    bool m_isMissingSyntaxDefinition;

    QScopedPointer<AutoCompleter> m_autoCompleter;
    CommentDefinition m_commentDefinition;

    QFutureWatcher<FileSearchResultList> *m_searchWatcher;
    QVector<SearchResult> m_searchResults;
    QTimer m_scrollBarUpdateTimer;
    HighlightScrollBar *m_highlightScrollBar;
    bool m_scrollBarUpdateScheduled;
};

TextEditorWidgetPrivate::TextEditorWidgetPrivate(TextEditorWidget *parent)
  : q(parent),
    m_toolBar(0),
    m_stretchWidget(0),
    m_cursorPositionLabel(0),
    m_fileEncodingLabel(0),
    m_cursorPositionLabelAction(0),
    m_fileEncodingLabelAction(0),
    m_contentsChanged(false),
    m_lastCursorChangeWasInteresting(false),
    m_parenthesesMatchingEnabled(false),
    m_formatRange(false),
    m_parenthesesMatchingTimer(0),
    m_extraArea(0),
    m_codeStylePreferences(0),
    m_fontSettingsNeedsApply(true), // apply when making visible the first time, for the split case
    extraAreaSelectionAnchorBlockNumber(-1),
    extraAreaToggleMarkBlockNumber(-1),
    extraAreaHighlightFoldedBlockNumber(-1),
    m_overlay(0),
    m_snippetOverlay(0),
    m_searchResultOverlay(0),
    m_refactorOverlay(0),
    visibleFoldedBlockNumber(-1),
    suggestedVisibleFoldedBlockNumber(-1),
    m_mouseOnFoldedMarker(false),
    m_marksVisible(false),
    m_codeFoldingVisible(false),
    m_codeFoldingSupported(false),
    m_revisionsVisible(false),
    m_lineNumbersVisible(true),
    m_highlightCurrentLine(true),
    m_requestMarkEnabled(true),
    m_lineSeparatorsAllowed(false),
    m_maybeFakeTooltipEvent(false),
    m_visibleWrapColumn(0),
    m_linkPressed(false),
    m_delayedUpdateTimer(0),
    m_inBlockSelectionMode(false),
    m_moveLineUndoHack(false),
    m_findScopeVerticalBlockSelectionFirstColumn(-1),
    m_findScopeVerticalBlockSelectionLastColumn(-1),
    m_highlightBlocksTimer(0),
    m_assistRelevantContentAdded(false),
    m_cursorBlockNumber(-1),
    m_blockCount(0),
    m_markDragging(false),
    m_clipboardAssistProvider(new ClipboardAssistProvider),
    m_isMissingSyntaxDefinition(false),
    m_autoCompleter(new AutoCompleter),
    m_searchWatcher(0),
    m_scrollBarUpdateTimer(0),
    m_highlightScrollBar(0),
    m_scrollBarUpdateScheduled(false)
{
    Aggregation::Aggregate *aggregate = new Aggregation::Aggregate;
    BaseTextFind *baseTextFind = new BaseTextFind(q);
    connect(baseTextFind, &BaseTextFind::highlightAllRequested,
            this, &TextEditorWidgetPrivate::highlightSearchResultsSlot);
    connect(baseTextFind, &BaseTextFind::findScopeChanged,
            this, &TextEditorWidgetPrivate::setFindScope);
    aggregate->add(baseTextFind);
    aggregate->add(q);

    m_extraArea = new TextEditExtraArea(q);
    m_extraArea->setMouseTracking(true);

    m_stretchWidget = new QWidget;
    m_stretchWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_toolBar = new QToolBar;
    m_toolBar->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    m_toolBar->addWidget(m_stretchWidget);

    m_cursorPositionLabel = new LineColumnLabel;
    const int spacing = q->style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing) / 2;
    m_cursorPositionLabel->setContentsMargins(spacing, 0, spacing, 0);

    m_fileEncodingLabel = new LineColumnLabel;
    m_fileEncodingLabel->setContentsMargins(spacing, 0, spacing, 0);

    m_cursorPositionLabelAction = m_toolBar->addWidget(m_cursorPositionLabel);
    m_fileEncodingLabelAction = m_toolBar->addWidget(m_fileEncodingLabel);
    m_extraSelections.reserve(NExtraSelectionKinds);
}

TextEditorWidgetPrivate::~TextEditorWidgetPrivate()
{
    q->disconnect(this);
    delete m_toolBar;
}

} // namespace Internal

using namespace Internal;

/*!
 * Test if syntax highlighter is available (or unneeded) for \a widget.
 * If not found, show a warning with a link to the relevant settings page.
 */
static void updateEditorInfoBar(TextEditorWidget *widget)
{
    Id infoSyntaxDefinition(Constants::INFO_SYNTAX_DEFINITION);
    InfoBar *infoBar = widget->textDocument()->infoBar();
    if (!widget->isMissingSyntaxDefinition()) {
        infoBar->removeInfo(infoSyntaxDefinition);
    } else if (infoBar->canInfoBeAdded(infoSyntaxDefinition)) {
        InfoBarEntry info(infoSyntaxDefinition,
                          BaseTextEditor::tr("A highlight definition was not found for this file. "
                                             "Would you like to try to find one?"),
                          InfoBarEntry::GlobalSuppressionEnabled);
        info.setCustomButtonInfo(BaseTextEditor::tr("Show Highlighter Options..."), [widget]() {
            ICore::showOptionsDialog(Constants::TEXT_EDITOR_HIGHLIGHTER_SETTINGS, widget);
        });

        infoBar->addInfo(info);
    }
}

QString TextEditorWidget::plainTextFromSelection(const QTextCursor &cursor) const
{
    // Copy the selected text as plain text
    QString text = cursor.selectedText();
    return convertToPlainText(text);
}

QString TextEditorWidget::convertToPlainText(const QString &txt)
{
    QString ret = txt;
    QChar *uc = ret.data();
    QChar *e = uc + ret.size();

    for (; uc != e; ++uc) {
        switch (uc->unicode()) {
        case 0xfdd0: // QTextBeginningOfFrame
        case 0xfdd1: // QTextEndOfFrame
        case QChar::ParagraphSeparator:
        case QChar::LineSeparator:
            *uc = QLatin1Char('\n');
            break;
        case QChar::Nbsp:
            *uc = QLatin1Char(' ');
            break;
        default:
            ;
        }
    }
    return ret;
}

static const char kTextBlockMimeType[] = "application/vnd.qtcreator.blocktext";

Id TextEditorWidget::SnippetPlaceholderSelection("TextEdit.SnippetPlaceHolderSelection");
Id TextEditorWidget::CurrentLineSelection("TextEdit.CurrentLineSelection");
Id TextEditorWidget::ParenthesesMatchingSelection("TextEdit.ParenthesesMatchingSelection");
Id TextEditorWidget::CodeWarningsSelection("TextEdit.CodeWarningsSelection");
Id TextEditorWidget::CodeSemanticsSelection("TextEdit.CodeSemanticsSelection");
Id TextEditorWidget::UndefinedSymbolSelection("TextEdit.UndefinedSymbolSelection");
Id TextEditorWidget::UnusedSymbolSelection("TextEdit.UnusedSymbolSelection");
Id TextEditorWidget::OtherSelection("TextEdit.OtherSelection");
Id TextEditorWidget::ObjCSelection("TextEdit.ObjCSelection");
Id TextEditorWidget::DebuggerExceptionSelection("TextEdit.DebuggerExceptionSelection");
Id TextEditorWidget::FakeVimSelection("TextEdit.FakeVimSelection");

TextEditorWidget::TextEditorWidget(QWidget *parent)
    : QPlainTextEdit(parent)
{
    // "Needed", as the creation below triggers ChildEvents that are
    // passed to this object's event() which uses 'd'.
    d = 0;
    d = new TextEditorWidgetPrivate(this);
}

void TextEditorWidget::setTextDocument(const QSharedPointer<TextDocument> &doc)
{
    d->ctor(doc);
}

void TextEditorWidgetPrivate::setupScrollBar()
{
    if (m_displaySettings.m_scrollBarHighlights) {
        if (m_highlightScrollBar)
            return;
        m_highlightScrollBar = new HighlightScrollBar(Qt::Vertical, q);
        m_highlightScrollBar->setColor(Constants::SCROLL_BAR_SEARCH_RESULT,
                                       Theme::TextEditor_SearchResult_ScrollBarColor);
        m_highlightScrollBar->setColor(Constants::SCROLL_BAR_CURRENT_LINE,
                                       Theme::TextEditor_CurrentLine_ScrollBarColor);
        m_highlightScrollBar->setPriority(
                    Constants::SCROLL_BAR_SEARCH_RESULT, HighlightScrollBar::HighPriority);
        m_highlightScrollBar->setPriority(
                    Constants::SCROLL_BAR_CURRENT_LINE, HighlightScrollBar::HighestPriority);
        q->setVerticalScrollBar(m_highlightScrollBar);
        highlightSearchResultsInScrollBar();
        scheduleUpdateHighlightScrollBar();
    } else if (m_highlightScrollBar) {
        q->setVerticalScrollBar(new QScrollBar(Qt::Vertical, q));
        m_highlightScrollBar = 0;
    }
}

void TextEditorWidgetPrivate::ctor(const QSharedPointer<TextDocument> &doc)
{
    q->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    m_overlay = new TextEditorOverlay(q);
    m_snippetOverlay = new TextEditorOverlay(q);
    m_searchResultOverlay = new TextEditorOverlay(q);
    m_refactorOverlay = new RefactorOverlay(q);

    m_document = doc;
    setupDocumentSignals();

    // from RESEARCH

    q->setLayoutDirection(Qt::LeftToRight);
    q->viewport()->setMouseTracking(true);

    extraAreaSelectionAnchorBlockNumber = -1;
    extraAreaToggleMarkBlockNumber = -1;
    extraAreaHighlightFoldedBlockNumber = -1;
    visibleFoldedBlockNumber = -1;
    suggestedVisibleFoldedBlockNumber = -1;

    QObject::connect(&m_codeAssistant, &CodeAssistant::finished,
                     q, &TextEditorWidget::assistFinished);

    QObject::connect(q, &QPlainTextEdit::blockCountChanged,
                     this, &TextEditorWidgetPrivate::slotUpdateExtraAreaWidth);

    QObject::connect(q, &QPlainTextEdit::modificationChanged, m_extraArea,
                     static_cast<void (QWidget::*)()>(&QWidget::update));

    QObject::connect(q, &QPlainTextEdit::cursorPositionChanged,
                     q, &TextEditorWidget::slotCursorPositionChanged);

    QObject::connect(q, &QPlainTextEdit::cursorPositionChanged,
                     this, &TextEditorWidgetPrivate::updateCursorPosition);

    QObject::connect(q, &QPlainTextEdit::updateRequest,
                     this, &TextEditorWidgetPrivate::slotUpdateRequest);

    QObject::connect(q, &QPlainTextEdit::selectionChanged,
                     this, &TextEditorWidgetPrivate::slotSelectionChanged);

//     (void) new QShortcut(tr("CTRL+L"), this, SLOT(centerCursor()), 0, Qt::WidgetShortcut);
//     (void) new QShortcut(tr("F9"), this, SLOT(slotToggleMark()), 0, Qt::WidgetShortcut);
//     (void) new QShortcut(tr("F11"), this, SLOT(slotToggleBlockVisible()));

#ifdef DO_FOO
    (void) new QShortcut(TextEditorWidget::tr("CTRL+D"), this, SLOT(doFoo()));
#endif

    // parentheses matcher
    m_formatRange = true;
    m_parenthesesMatchingTimer.setSingleShot(true);
    QObject::connect(&m_parenthesesMatchingTimer, &QTimer::timeout,
                     this, &TextEditorWidgetPrivate::_q_matchParentheses);

    m_highlightBlocksTimer.setSingleShot(true);
    QObject::connect(&m_highlightBlocksTimer, &QTimer::timeout,
                     this, &TextEditorWidgetPrivate::_q_highlightBlocks);

    m_scrollBarUpdateTimer.setSingleShot(true);
    QObject::connect(&m_scrollBarUpdateTimer, &QTimer::timeout,
                     this, &TextEditorWidgetPrivate::highlightSearchResultsInScrollBar);

    m_animator = 0;

    slotUpdateExtraAreaWidth();
    updateHighlights();
    q->setFrameStyle(QFrame::NoFrame);

    m_delayedUpdateTimer.setSingleShot(true);
    QObject::connect(&m_delayedUpdateTimer, &QTimer::timeout, q->viewport(),
                     static_cast<void (QWidget::*)()>(&QWidget::update));

    m_moveLineUndoHack = false;

    updateCannotDecodeInfo();

    QObject::connect(m_document.data(), &TextDocument::aboutToOpen,
                     q, &TextEditorWidget::aboutToOpen);
    QObject::connect(m_document.data(), &TextDocument::openFinishedSuccessfully,
                     q, &TextEditorWidget::openFinishedSuccessfully);
    connect(m_fileEncodingLabel, &LineColumnLabel::clicked,
            q, &TextEditorWidget::selectEncoding);
    connect(m_document->document(), &QTextDocument::modificationChanged,
            q, &TextEditorWidget::updateTextCodecLabel);
    q->updateTextCodecLabel();
}

TextEditorWidget::~TextEditorWidget()
{
    delete d;
    d = 0;
}

void TextEditorWidget::print(QPrinter *printer)
{
    const bool oldFullPage = printer->fullPage();
    printer->setFullPage(true);
    QPrintDialog *dlg = new QPrintDialog(printer, this);
    dlg->setWindowTitle(tr("Print Document"));
    if (dlg->exec() == QDialog::Accepted)
        d->print(printer);
    printer->setFullPage(oldFullPage);
    delete dlg;
}

static int foldBoxWidth(const QFontMetrics &fm)
{
    const int lineSpacing = fm.lineSpacing();
    return lineSpacing + lineSpacing % 2 + 1;
}

static void printPage(int index, QPainter *painter, const QTextDocument *doc,
                      const QRectF &body, const QRectF &titleBox,
                      const QString &title)
{
    painter->save();

    painter->translate(body.left(), body.top() - (index - 1) * body.height());
    QRectF view(0, (index - 1) * body.height(), body.width(), body.height());

    QAbstractTextDocumentLayout *layout = doc->documentLayout();
    QAbstractTextDocumentLayout::PaintContext ctx;

    painter->setFont(QFont(doc->defaultFont()));
    QRectF box = titleBox.translated(0, view.top());
    int dpix = painter->device()->logicalDpiX();
    int dpiy = painter->device()->logicalDpiY();
    int mx = 5 * dpix / 72.0;
    int my = 2 * dpiy / 72.0;
    painter->fillRect(box.adjusted(-mx, -my, mx, my), QColor(210, 210, 210));
    if (!title.isEmpty())
        painter->drawText(box, Qt::AlignCenter, title);
    const QString pageString = QString::number(index);
    painter->drawText(box, Qt::AlignRight, pageString);

    painter->setClipRect(view);
    ctx.clip = view;
    // don't use the system palette text as default text color, on HP/UX
    // for example that's white, and white text on white paper doesn't
    // look that nice
    ctx.palette.setColor(QPalette::Text, Qt::black);

    layout->draw(painter, ctx);

    painter->restore();
}

void TextEditorWidgetPrivate::print(QPrinter *printer)
{
    QTextDocument *doc = q->document();

    QString title = m_document->displayName();
    if (!title.isEmpty())
        printer->setDocName(title);


    QPainter p(printer);

    // Check that there is a valid device to print to.
    if (!p.isActive())
        return;

    doc = doc->clone(doc);

    QTextOption opt = doc->defaultTextOption();
    opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    doc->setDefaultTextOption(opt);

    (void)doc->documentLayout(); // make sure that there is a layout


    QColor background = q->palette().color(QPalette::Base);
    bool backgroundIsDark = background.value() < 128;

    for (QTextBlock srcBlock = q->document()->firstBlock(), dstBlock = doc->firstBlock();
         srcBlock.isValid() && dstBlock.isValid();
         srcBlock = srcBlock.next(), dstBlock = dstBlock.next()) {


        QList<QTextLayout::FormatRange> formatList = srcBlock.layout()->additionalFormats();
        if (backgroundIsDark) {
            // adjust syntax highlighting colors for better contrast
            for (int i = formatList.count() - 1; i >= 0; --i) {
                QTextCharFormat &format = formatList[i].format;
                if (format.background().color() == background) {
                    QBrush brush = format.foreground();
                    QColor color = brush.color();
                    int h,s,v,a;
                    color.getHsv(&h, &s, &v, &a);
                    color.setHsv(h, s, qMin(128, v), a);
                    brush.setColor(color);
                    format.setForeground(brush);
                }
                format.setBackground(Qt::white);
            }
        }

        dstBlock.layout()->setAdditionalFormats(formatList);
    }

    QAbstractTextDocumentLayout *layout = doc->documentLayout();
    layout->setPaintDevice(p.device());

    int dpiy = p.device()->logicalDpiY();
    int margin = (int) ((2/2.54)*dpiy); // 2 cm margins

    QTextFrameFormat fmt = doc->rootFrame()->frameFormat();
    fmt.setMargin(margin);
    doc->rootFrame()->setFrameFormat(fmt);

    QRectF pageRect(printer->pageRect());
    QRectF body = QRectF(0, 0, pageRect.width(), pageRect.height());
    QFontMetrics fontMetrics(doc->defaultFont(), p.device());

    QRectF titleBox(margin,
                    body.top() + margin
                    - fontMetrics.height()
                    - 6 * dpiy / 72.0,
                    body.width() - 2*margin,
                    fontMetrics.height());
    doc->setPageSize(body.size());

    int docCopies;
    int pageCopies;
    if (printer->collateCopies() == true) {
        docCopies = 1;
        pageCopies = printer->numCopies();
    } else {
        docCopies = printer->numCopies();
        pageCopies = 1;
    }

    int fromPage = printer->fromPage();
    int toPage = printer->toPage();
    bool ascending = true;

    if (fromPage == 0 && toPage == 0) {
        fromPage = 1;
        toPage = doc->pageCount();
    }
    // paranoia check
    fromPage = qMax(1, fromPage);
    toPage = qMin(doc->pageCount(), toPage);

    if (printer->pageOrder() == QPrinter::LastPageFirst) {
        int tmp = fromPage;
        fromPage = toPage;
        toPage = tmp;
        ascending = false;
    }

    for (int i = 0; i < docCopies; ++i) {

        int page = fromPage;
        while (true) {
            for (int j = 0; j < pageCopies; ++j) {
                if (printer->printerState() == QPrinter::Aborted
                    || printer->printerState() == QPrinter::Error)
                    goto UserCanceled;
                printPage(page, &p, doc, body, titleBox, title);
                if (j < pageCopies - 1)
                    printer->newPage();
            }

            if (page == toPage)
                break;

            if (ascending)
                ++page;
            else
                --page;

            printer->newPage();
        }

        if ( i < docCopies - 1)
            printer->newPage();
    }

UserCanceled:
    delete doc;
}


int TextEditorWidgetPrivate::visualIndent(const QTextBlock &block) const
{
    if (!block.isValid())
        return 0;
    const QTextDocument *document = block.document();
    int i = 0;
    while (i < block.length()) {
        if (!document->characterAt(block.position() + i).isSpace()) {
            QTextCursor cursor(block);
            cursor.setPosition(block.position() + i);
            return q->cursorRect(cursor).x();
        }
        ++i;
    }

    return 0;
}

void TextEditorWidget::selectEncoding()
{
    TextDocument *doc = d->m_document.data();
    CodecSelector codecSelector(this, doc);

    switch (codecSelector.exec()) {
    case CodecSelector::Reload: {
        QString errorString;
        if (!doc->reload(&errorString, codecSelector.selectedCodec())) {
            QMessageBox::critical(this, tr("File Error"), errorString);
            break;
        }
        break; }
    case CodecSelector::Save:
        doc->setCodec(codecSelector.selectedCodec());
        EditorManager::saveDocument(textDocument());
        updateTextCodecLabel();
        break;
    case CodecSelector::Cancel:
        break;
    }
}

void TextEditorWidget::updateTextCodecLabel()
{
    QString text = QString::fromLatin1(d->m_document->codec()->name());
    d->m_fileEncodingLabel->setText(text, text);
}

QString TextEditorWidget::msgTextTooLarge(quint64 size)
{
    return tr("The text is too large to be displayed (%1 MB).").
           arg(size >> 20);
}

void TextEditorWidget::insertPlainText(const QString &text)
{
    if (d->m_inBlockSelectionMode)
        d->insertIntoBlockSelection(text);
    else
        QPlainTextEdit::insertPlainText(text);
}

QString TextEditorWidget::selectedText() const
{
    if (d->m_inBlockSelectionMode)
        return d->copyBlockSelection();
    else
        return textCursor().selectedText();
}

void TextEditorWidgetPrivate::updateCannotDecodeInfo()
{
    q->setReadOnly(m_document->hasDecodingError());
    InfoBar *infoBar = m_document->infoBar();
    Id selectEncodingId(Constants::SELECT_ENCODING);
    if (m_document->hasDecodingError()) {
        if (!infoBar->canInfoBeAdded(selectEncodingId))
            return;
        InfoBarEntry info(selectEncodingId,
            TextEditorWidget::tr("<b>Error:</b> Could not decode \"%1\" with \"%2\"-encoding. Editing not possible.")
            .arg(m_document->displayName()).arg(QString::fromLatin1(m_document->codec()->name())));
        info.setCustomButtonInfo(TextEditorWidget::tr("Select Encoding"), [this]() { q->selectEncoding(); });
        infoBar->addInfo(info);
    } else {
        infoBar->removeInfo(selectEncodingId);
    }
}

/*
  Collapses the first comment in a file, if there is only whitespace above
  */
void TextEditorWidgetPrivate::foldLicenseHeader()
{
    QTextDocument *doc = q->document();
    TextDocumentLayout *documentLayout = qobject_cast<TextDocumentLayout*>(doc->documentLayout());
    QTC_ASSERT(documentLayout, return);
    QTextBlock block = doc->firstBlock();
    while (block.isValid() && block.isVisible()) {
        QString text = block.text();
        if (TextDocumentLayout::canFold(block) && block.next().isVisible()) {
            if (text.trimmed().startsWith(QLatin1String("/*"))) {
                TextDocumentLayout::doFoldOrUnfold(block, false);
                moveCursorVisible();
                documentLayout->requestUpdate();
                documentLayout->emitDocumentSizeChanged();
                break;
            }
        }
        if (TabSettings::firstNonSpace(text) < text.size())
            break;
        block = block.next();
    }
}

TextDocument *TextEditorWidget::textDocument() const
{
    return d->m_document.data();
}

void TextEditorWidget::aboutToOpen(const QString &fileName, const QString &realFileName)
{
    Q_UNUSED(fileName)
    Q_UNUSED(realFileName)
}

void TextEditorWidget::openFinishedSuccessfully()
{
    moveCursor(QTextCursor::Start);
    d->updateCannotDecodeInfo();
    updateTextCodecLabel();
}

TextDocumentPtr TextEditorWidget::textDocumentPtr() const
{
    return d->m_document;
}

TextEditorWidget *TextEditorWidget::currentTextEditorWidget()
{
    BaseTextEditor *editor = qobject_cast<BaseTextEditor *>(EditorManager::currentEditor());
    return editor ? editor->editorWidget() : 0;
}

void TextEditorWidgetPrivate::editorContentsChange(int position, int charsRemoved, int charsAdded)
{
    if (m_animator)
        m_animator->finish();

    m_contentsChanged = true;
    QTextDocument *doc = q->document();
    TextDocumentLayout *documentLayout = static_cast<TextDocumentLayout*>(doc->documentLayout());
    const QTextBlock posBlock = doc->findBlock(position);

    // Keep the line numbers and the block information for the text marks updated
    if (charsRemoved != 0) {
        documentLayout->updateMarksLineNumber();
        documentLayout->updateMarksBlock(posBlock);
    } else {
        const QTextBlock nextBlock = doc->findBlock(position + charsAdded);
        if (posBlock != nextBlock) {
            documentLayout->updateMarksLineNumber();
            documentLayout->updateMarksBlock(posBlock);
            documentLayout->updateMarksBlock(nextBlock);
        } else {
            documentLayout->updateMarksBlock(posBlock);
        }
    }

    if (m_snippetOverlay->isVisible()) {
        QTextCursor cursor = q->textCursor();
        cursor.setPosition(position);
        snippetCheckCursor(cursor);
    }

    if (charsAdded != 0 && q->document()->characterAt(position + charsAdded - 1).isPrint())
        m_assistRelevantContentAdded = true;

    int newBlockCount = doc->blockCount();
    if (!q->hasFocus() && newBlockCount != m_blockCount) {
        // lines were inserted or removed from outside, keep viewport on same part of text
        if (q->firstVisibleBlock().blockNumber() > posBlock.blockNumber())
            q->verticalScrollBar()->setValue(q->verticalScrollBar()->value() + newBlockCount - m_blockCount);
    }
    m_blockCount = newBlockCount;
    m_scrollBarUpdateTimer.start(500);
}

void TextEditorWidgetPrivate::slotSelectionChanged()
{
    if (!q->textCursor().hasSelection() && !m_selectBlockAnchor.isNull())
        m_selectBlockAnchor = QTextCursor();
    // Clear any link which might be showing when the selection changes
    clearLink();
}

void TextEditorWidget::gotoBlockStart()
{
    QTextCursor cursor = textCursor();
    if (TextBlockUserData::findPreviousOpenParenthesis(&cursor, false)) {
        setTextCursor(cursor);
        d->_q_matchParentheses();
    }
}

void TextEditorWidget::gotoBlockEnd()
{
    QTextCursor cursor = textCursor();
    if (TextBlockUserData::findNextClosingParenthesis(&cursor, false)) {
        setTextCursor(cursor);
        d->_q_matchParentheses();
    }
}

void TextEditorWidget::gotoBlockStartWithSelection()
{
    QTextCursor cursor = textCursor();
    if (TextBlockUserData::findPreviousOpenParenthesis(&cursor, true)) {
        setTextCursor(cursor);
        d->_q_matchParentheses();
    }
}

void TextEditorWidget::gotoBlockEndWithSelection()
{
    QTextCursor cursor = textCursor();
    if (TextBlockUserData::findNextClosingParenthesis(&cursor, true)) {
        setTextCursor(cursor);
        d->_q_matchParentheses();
    }
}

void TextEditorWidget::gotoLineStart()
{
    d->handleHomeKey(false);
}

void TextEditorWidget::gotoLineStartWithSelection()
{
    d->handleHomeKey(true);
}

void TextEditorWidget::gotoLineEnd()
{
    moveCursor(QTextCursor::EndOfLine);
}

void TextEditorWidget::gotoLineEndWithSelection()
{
    moveCursor(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
}

void TextEditorWidget::gotoNextLine()
{
    moveCursor(QTextCursor::Down);
}

void TextEditorWidget::gotoNextLineWithSelection()
{
    moveCursor(QTextCursor::Down, QTextCursor::KeepAnchor);
}

void TextEditorWidget::gotoPreviousLine()
{
    moveCursor(QTextCursor::Up);
}

void TextEditorWidget::gotoPreviousLineWithSelection()
{
    moveCursor(QTextCursor::Up, QTextCursor::KeepAnchor);
}

void TextEditorWidget::gotoPreviousCharacter()
{
    moveCursor(QTextCursor::PreviousCharacter);
}

void TextEditorWidget::gotoPreviousCharacterWithSelection()
{
    moveCursor(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
}

void TextEditorWidget::gotoNextCharacter()
{
    moveCursor(QTextCursor::NextCharacter);
}

void TextEditorWidget::gotoNextCharacterWithSelection()
{
    moveCursor(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
}

void TextEditorWidget::gotoPreviousWord()
{
    moveCursor(QTextCursor::PreviousWord);
    setTextCursor(textCursor());
}

void TextEditorWidget::gotoPreviousWordWithSelection()
{
    moveCursor(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
    setTextCursor(textCursor());
}

void TextEditorWidget::gotoNextWord()
{
    moveCursor(QTextCursor::NextWord);
    setTextCursor(textCursor());
}

void TextEditorWidget::gotoNextWordWithSelection()
{
    moveCursor(QTextCursor::NextWord, QTextCursor::KeepAnchor);
    setTextCursor(textCursor());
}

void TextEditorWidget::gotoPreviousWordCamelCase()
{
    QTextCursor c = textCursor();
    d->camelCaseLeft(c, QTextCursor::MoveAnchor);
    setTextCursor(c);
}

void TextEditorWidget::gotoPreviousWordCamelCaseWithSelection()
{
    QTextCursor c = textCursor();
    d->camelCaseLeft(c, QTextCursor::KeepAnchor);
    setTextCursor(c);
}

void TextEditorWidget::gotoNextWordCamelCase()
{
    QTextCursor c = textCursor();
    d->camelCaseRight(c, QTextCursor::MoveAnchor);
    setTextCursor(c);
}

void TextEditorWidget::gotoNextWordCamelCaseWithSelection()
{
    QTextCursor c = textCursor();
    d->camelCaseRight(c, QTextCursor::KeepAnchor);
    setTextCursor(c);
}

static QTextCursor flippedCursor(const QTextCursor &cursor)
{
    QTextCursor flipped = cursor;
    flipped.clearSelection();
    flipped.setPosition(cursor.anchor(), QTextCursor::KeepAnchor);
    return flipped;
}

bool TextEditorWidget::selectBlockUp()
{
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection())
        d->m_selectBlockAnchor = cursor;
    else
        cursor.setPosition(cursor.selectionStart());

    if (!TextBlockUserData::findPreviousOpenParenthesis(&cursor, false))
        return false;
    if (!TextBlockUserData::findNextClosingParenthesis(&cursor, true))
        return false;

    setTextCursor(flippedCursor(cursor));
    d->_q_matchParentheses();
    return true;
}

bool TextEditorWidget::selectBlockDown()
{
    QTextCursor tc = textCursor();
    QTextCursor cursor = d->m_selectBlockAnchor;

    if (!tc.hasSelection() || cursor.isNull())
        return false;
    tc.setPosition(tc.selectionStart());

    forever {
        QTextCursor ahead = cursor;
        if (!TextBlockUserData::findPreviousOpenParenthesis(&ahead, false))
            break;
        if (ahead.position() <= tc.position())
            break;
        cursor = ahead;
    }
    if ( cursor != d->m_selectBlockAnchor)
        TextBlockUserData::findNextClosingParenthesis(&cursor, true);

    setTextCursor(flippedCursor(cursor));
    d->_q_matchParentheses();
    return true;
}

void TextEditorWidget::copyLineUp()
{
    d->copyLineUpDown(true);
}

void TextEditorWidget::copyLineDown()
{
    d->copyLineUpDown(false);
}

// @todo: Potential reuse of some code around the following functions...
void TextEditorWidgetPrivate::copyLineUpDown(bool up)
{
    QTextCursor cursor = q->textCursor();
    QTextCursor move = cursor;
    move.beginEditBlock();

    bool hasSelection = cursor.hasSelection();

    if (hasSelection) {
        move.setPosition(cursor.selectionStart());
        move.movePosition(QTextCursor::StartOfBlock);
        move.setPosition(cursor.selectionEnd(), QTextCursor::KeepAnchor);
        move.movePosition(move.atBlockStart() ? QTextCursor::Left: QTextCursor::EndOfBlock,
                          QTextCursor::KeepAnchor);
    } else {
        move.movePosition(QTextCursor::StartOfBlock);
        move.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    }

    QString text = move.selectedText();

    if (up) {
        move.setPosition(cursor.selectionStart());
        move.movePosition(QTextCursor::StartOfBlock);
        move.insertBlock();
        move.movePosition(QTextCursor::Left);
    } else {
        move.movePosition(QTextCursor::EndOfBlock);
        if (move.atBlockStart()) {
            move.movePosition(QTextCursor::NextBlock);
            move.insertBlock();
            move.movePosition(QTextCursor::Left);
        } else {
            move.insertBlock();
        }
    }

    int start = move.position();
    move.clearSelection();
    move.insertText(text);
    int end = move.position();

    move.setPosition(start);
    move.setPosition(end, QTextCursor::KeepAnchor);

    m_document->autoIndent(move);
    move.endEditBlock();

    q->setTextCursor(move);
}

void TextEditorWidget::joinLines()
{
    QTextCursor cursor = textCursor();
    QTextCursor start = cursor;
    QTextCursor end = cursor;

    start.setPosition(cursor.selectionStart());
    end.setPosition(cursor.selectionEnd() - 1);

    int lineCount = qMax(1, end.blockNumber() - start.blockNumber());

    cursor.beginEditBlock();
    cursor.setPosition(cursor.selectionStart());
    while (lineCount--) {
        cursor.movePosition(QTextCursor::NextBlock);
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        QString cutLine = cursor.selectedText();

        // Collapse leading whitespaces to one or insert whitespace
        cutLine.replace(QRegExp(QLatin1String("^\\s*")), QLatin1String(" "));
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();

        cursor.movePosition(QTextCursor::PreviousBlock);
        cursor.movePosition(QTextCursor::EndOfBlock);

        cursor.insertText(cutLine);
    }
    cursor.endEditBlock();

    setTextCursor(cursor);
}

void TextEditorWidget::insertLineAbove()
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();
    // If the cursor is at the beginning of the document,
    // it should still insert a line above the current line.
    cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
    cursor.insertBlock();
    cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::MoveAnchor);
    d->m_document->autoIndent(cursor);
    cursor.endEditBlock();
    setTextCursor(cursor);
}

void TextEditorWidget::insertLineBelow()
{
    if (d->m_inBlockSelectionMode)
        d->disableBlockSelection(false);
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::MoveAnchor);
    cursor.insertBlock();
    d->m_document->autoIndent(cursor);
    cursor.endEditBlock();
    setTextCursor(cursor);
}

void TextEditorWidget::moveLineUp()
{
    d->moveLineUpDown(true);
}

void TextEditorWidget::moveLineDown()
{
    d->moveLineUpDown(false);
}

void TextEditorWidget::uppercaseSelection()
{
    d->transformSelection(&QString_toUpper);
}

void TextEditorWidget::lowercaseSelection()
{
    d->transformSelection(&QString_toLower);
}

void TextEditorWidget::indent()
{
    int offset = 0;
    doSetTextCursor(textDocument()->indent(textCursor(), d->m_inBlockSelectionMode,
                                           d->m_blockSelection.positionColumn, &offset),
                    d->m_inBlockSelectionMode);
    if (d->m_inBlockSelectionMode) {
        d->m_blockSelection.anchorColumn += offset;
        d->m_blockSelection.positionColumn += offset;
    }
}

void TextEditorWidget::unindent()
{
    int offset = 0;
    doSetTextCursor(textDocument()->unindent(textCursor(), d->m_inBlockSelectionMode,
                                             d->m_blockSelection.positionColumn, &offset),
                    d->m_inBlockSelectionMode);
    if (d->m_inBlockSelectionMode) {
        d->m_blockSelection.anchorColumn += offset;
        d->m_blockSelection.positionColumn += offset;
    }
}

void TextEditorWidget::undo()
{
    if (d->m_inBlockSelectionMode)
        d->disableBlockSelection(false);
    QPlainTextEdit::undo();
}

void TextEditorWidget::redo()
{
    if (d->m_inBlockSelectionMode)
        d->disableBlockSelection(false);
    QPlainTextEdit::redo();
}

void TextEditorWidget::openLinkUnderCursor()
{
    const bool openInNextSplit = alwaysOpenLinksInNextSplit();
    Link symbolLink = findLinkAt(textCursor(), true, openInNextSplit);
    openLink(symbolLink, openInNextSplit);
}

void TextEditorWidget::openLinkUnderCursorInNextSplit()
{
    const bool openInNextSplit = !alwaysOpenLinksInNextSplit();
    Link symbolLink = findLinkAt(textCursor(), true, openInNextSplit);
    openLink(symbolLink, openInNextSplit);
}

void TextEditorWidget::abortAssist()
{
    d->m_codeAssistant.destroyContext();
}

void TextEditorWidgetPrivate::moveLineUpDown(bool up)
{
    QTextCursor cursor = q->textCursor();
    QTextCursor move = cursor;

    move.setVisualNavigation(false); // this opens folded items instead of destroying them

    if (m_moveLineUndoHack)
        move.joinPreviousEditBlock();
    else
        move.beginEditBlock();

    bool hasSelection = cursor.hasSelection();

    if (hasSelection) {
        if (m_inBlockSelectionMode)
            disableBlockSelection(true);
        move.setPosition(cursor.selectionStart());
        move.movePosition(QTextCursor::StartOfBlock);
        move.setPosition(cursor.selectionEnd(), QTextCursor::KeepAnchor);
        move.movePosition(move.atBlockStart() ? QTextCursor::Left: QTextCursor::EndOfBlock,
                          QTextCursor::KeepAnchor);
    } else {
        move.movePosition(QTextCursor::StartOfBlock);
        move.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    }
    QString text = move.selectedText();

    RefactorMarkers affectedMarkers;
    RefactorMarkers nonAffectedMarkers;
    QList<int> markerOffsets;

    foreach (const RefactorMarker &marker, m_refactorOverlay->markers()) {
        //test if marker is part of the selection to be moved
        if ((move.selectionStart() <= marker.cursor.position())
                && (move.selectionEnd() >= marker.cursor.position())) {
            affectedMarkers.append(marker);
            //remember the offset of markers in text
            int offset = marker.cursor.position() - move.selectionStart();
            markerOffsets.append(offset);
        } else {
            nonAffectedMarkers.append(marker);
        }
    }

    move.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
    move.removeSelectedText();

    if (up) {
        move.movePosition(QTextCursor::PreviousBlock);
        move.insertBlock();
        move.movePosition(QTextCursor::Left);
    } else {
        move.movePosition(QTextCursor::EndOfBlock);
        if (move.atBlockStart()) { // empty block
            move.movePosition(QTextCursor::NextBlock);
            move.insertBlock();
            move.movePosition(QTextCursor::Left);
        } else {
            move.insertBlock();
        }
    }

    int start = move.position();
    move.clearSelection();
    move.insertText(text);
    int end = move.position();

    if (hasSelection) {
        move.setPosition(end);
        move.setPosition(start, QTextCursor::KeepAnchor);
    } else {
        move.setPosition(start);
    }

    //update positions of affectedMarkers
    for (int i=0;i < affectedMarkers.count(); i++) {
        int newPosition = start + markerOffsets.at(i);
        affectedMarkers[i].cursor.setPosition(newPosition);
    }
    m_refactorOverlay->setMarkers(nonAffectedMarkers + affectedMarkers);

    bool shouldReindent = true;
    if (m_commentDefinition.isValid()) {
        QString trimmedText(text.trimmed());

        if (m_commentDefinition.hasSingleLineStyle()) {
            if (trimmedText.startsWith(m_commentDefinition.singleLine))
                shouldReindent = false;
        }
        if (shouldReindent && m_commentDefinition.hasMultiLineStyle()) {
            // Don't have any single line comments; try multi line.
            if (trimmedText.startsWith(m_commentDefinition.multiLineStart)
                && trimmedText.endsWith(m_commentDefinition.multiLineEnd)) {
                shouldReindent = false;
            }
        }
    }

    if (shouldReindent) {
        // The text was not commented at all; re-indent.
        m_document->autoReindent(move);
    }
    move.endEditBlock();

    q->setTextCursor(move);
    m_moveLineUndoHack = true;
}

void TextEditorWidget::cleanWhitespace()
{
    d->m_document->cleanWhitespace(textCursor());
}


// could go into QTextCursor...
static QTextLine currentTextLine(const QTextCursor &cursor)
{
    const QTextBlock block = cursor.block();
    if (!block.isValid())
        return QTextLine();

    const QTextLayout *layout = block.layout();
    if (!layout)
        return QTextLine();

    const int relativePos = cursor.position() - block.position();
    return layout->lineForTextPosition(relativePos);
}

bool TextEditorWidgetPrivate::camelCaseLeft(QTextCursor &cursor, QTextCursor::MoveMode mode)
{
    int state = 0;
    enum Input {
        Input_U,
        Input_l,
        Input_underscore,
        Input_space,
        Input_other
    };

    if (!cursor.movePosition(QTextCursor::Left, mode))
        return false;

    forever {
        QChar c = q->document()->characterAt(cursor.position());
        Input input = Input_other;
        if (c.isUpper())
            input = Input_U;
        else if (c.isLower() || c.isDigit())
            input = Input_l;
        else if (c == QLatin1Char('_'))
            input = Input_underscore;
        else if (c.isSpace() && c != QChar::ParagraphSeparator)
            input = Input_space;
        else
            input = Input_other;

        switch (state) {
        case 0:
            switch (input) {
            case Input_U:
                state = 1;
                break;
            case Input_l:
                state = 2;
                break;
            case Input_underscore:
                state = 3;
                break;
            case Input_space:
                state = 4;
                break;
            default:
                cursor.movePosition(QTextCursor::Right, mode);
                return cursor.movePosition(QTextCursor::WordLeft, mode);
            }
            break;
        case 1:
            switch (input) {
            case Input_U:
                break;
            default:
                cursor.movePosition(QTextCursor::Right, mode);
                return true;
            }
            break;

        case 2:
            switch (input) {
            case Input_U:
                return true;
            case Input_l:
                break;
            default:
                cursor.movePosition(QTextCursor::Right, mode);
                return true;
            }
            break;
        case 3:
            switch (input) {
            case Input_underscore:
                break;
            case Input_U:
                state = 1;
                break;
            case Input_l:
                state = 2;
                break;
            default:
                cursor.movePosition(QTextCursor::Right, mode);
                return true;
            }
            break;
        case 4:
            switch (input) {
            case Input_space:
                break;
            case Input_U:
                state = 1;
                break;
            case Input_l:
                state = 2;
                break;
            case Input_underscore:
                state = 3;
                break;
            default:
                cursor.movePosition(QTextCursor::Right, mode);
                if (cursor.positionInBlock() == 0)
                    return true;
                return cursor.movePosition(QTextCursor::WordLeft, mode);
            }
        }

        if (!cursor.movePosition(QTextCursor::Left, mode))
            return true;
    }
}

bool TextEditorWidgetPrivate::camelCaseRight(QTextCursor &cursor, QTextCursor::MoveMode mode)
{
    int state = 0;
    enum Input {
        Input_U,
        Input_l,
        Input_underscore,
        Input_space,
        Input_other
    };

    forever {
        QChar c = q->document()->characterAt(cursor.position());
        Input input = Input_other;
        if (c.isUpper())
            input = Input_U;
        else if (c.isLower() || c.isDigit())
            input = Input_l;
        else if (c == QLatin1Char('_'))
            input = Input_underscore;
        else if (c.isSpace() && c != QChar::ParagraphSeparator)
            input = Input_space;
        else
            input = Input_other;

        switch (state) {
        case 0:
            switch (input) {
            case Input_U:
                state = 4;
                break;
            case Input_l:
                state = 1;
                break;
            case Input_underscore:
                state = 6;
                break;
            default:
                return cursor.movePosition(QTextCursor::WordRight, mode);
            }
            break;
        case 1:
            switch (input) {
            case Input_U:
                return true;
            case Input_l:
                break;
            case Input_underscore:
                state = 6;
                break;
            case Input_space:
                state = 7;
                break;
            default:
                return true;
            }
            break;
        case 2:
            switch (input) {
            case Input_U:
                break;
            case Input_l:
                cursor.movePosition(QTextCursor::Left, mode);
                return true;
            case Input_underscore:
                state = 6;
                break;
            case Input_space:
                state = 7;
                break;
            default:
                return true;
            }
            break;
        case 4:
            switch (input) {
            case Input_U:
                state = 2;
                break;
            case Input_l:
                state = 1;
                break;
            case Input_underscore:
                state = 6;
                break;
            case Input_space:
                state = 7;
                break;
            default:
                return true;
            }
            break;
        case 6:
            switch (input) {
            case Input_underscore:
                break;
            case Input_space:
                state = 7;
                break;
            default:
                return true;
            }
            break;
        case 7:
            switch (input) {
            case Input_space:
                break;
            default:
                return true;
            }
            break;
        }
        cursor.movePosition(QTextCursor::Right, mode);
    }
}

bool TextEditorWidgetPrivate::cursorMoveKeyEvent(QKeyEvent *e)
{
    QTextCursor cursor = q->textCursor();

    QTextCursor::MoveMode mode = QTextCursor::MoveAnchor;
    QTextCursor::MoveOperation op = QTextCursor::NoMove;

    if (e == QKeySequence::MoveToNextChar) {
            op = QTextCursor::Right;
    } else if (e == QKeySequence::MoveToPreviousChar) {
            op = QTextCursor::Left;
    } else if (e == QKeySequence::SelectNextChar) {
           op = QTextCursor::Right;
           mode = QTextCursor::KeepAnchor;
    } else if (e == QKeySequence::SelectPreviousChar) {
            op = QTextCursor::Left;
            mode = QTextCursor::KeepAnchor;
    } else if (e == QKeySequence::SelectNextWord) {
            op = QTextCursor::WordRight;
            mode = QTextCursor::KeepAnchor;
    } else if (e == QKeySequence::SelectPreviousWord) {
            op = QTextCursor::WordLeft;
            mode = QTextCursor::KeepAnchor;
    } else if (e == QKeySequence::SelectStartOfLine) {
            op = QTextCursor::StartOfLine;
            mode = QTextCursor::KeepAnchor;
    } else if (e == QKeySequence::SelectEndOfLine) {
            op = QTextCursor::EndOfLine;
            mode = QTextCursor::KeepAnchor;
    } else if (e == QKeySequence::SelectStartOfBlock) {
            op = QTextCursor::StartOfBlock;
            mode = QTextCursor::KeepAnchor;
    } else if (e == QKeySequence::SelectEndOfBlock) {
            op = QTextCursor::EndOfBlock;
            mode = QTextCursor::KeepAnchor;
    } else if (e == QKeySequence::SelectStartOfDocument) {
            op = QTextCursor::Start;
            mode = QTextCursor::KeepAnchor;
    } else if (e == QKeySequence::SelectEndOfDocument) {
            op = QTextCursor::End;
            mode = QTextCursor::KeepAnchor;
    } else if (e == QKeySequence::SelectPreviousLine) {
            op = QTextCursor::Up;
            mode = QTextCursor::KeepAnchor;
    } else if (e == QKeySequence::SelectNextLine) {
            op = QTextCursor::Down;
            mode = QTextCursor::KeepAnchor;
            {
                QTextBlock block = cursor.block();
                QTextLine line = currentTextLine(cursor);
                if (!block.next().isValid()
                    && line.isValid()
                    && line.lineNumber() == block.layout()->lineCount() - 1)
                    op = QTextCursor::End;
            }
    } else if (e == QKeySequence::MoveToNextWord) {
            op = QTextCursor::WordRight;
    } else if (e == QKeySequence::MoveToPreviousWord) {
            op = QTextCursor::WordLeft;
    } else if (e == QKeySequence::MoveToEndOfBlock) {
            op = QTextCursor::EndOfBlock;
    } else if (e == QKeySequence::MoveToStartOfBlock) {
            op = QTextCursor::StartOfBlock;
    } else if (e == QKeySequence::MoveToNextLine) {
            op = QTextCursor::Down;
    } else if (e == QKeySequence::MoveToPreviousLine) {
            op = QTextCursor::Up;
    } else if (e == QKeySequence::MoveToStartOfLine) {
            op = QTextCursor::StartOfLine;
    } else if (e == QKeySequence::MoveToEndOfLine) {
            op = QTextCursor::EndOfLine;
    } else if (e == QKeySequence::MoveToStartOfDocument) {
            op = QTextCursor::Start;
    } else if (e == QKeySequence::MoveToEndOfDocument) {
            op = QTextCursor::End;
    } else {
        return false;
    }


// Except for pageup and pagedown, Mac OS X has very different behavior, we don't do it all, but
// here's the breakdown:
// Shift still works as an anchor, but only one of the other keys can be down Ctrl (Command),
// Alt (Option), or Meta (Control).
// Command/Control + Left/Right -- Move to left or right of the line
//                 + Up/Down -- Move to top bottom of the file. (Control doesn't move the cursor)
// Option + Left/Right -- Move one word Left/right.
//        + Up/Down  -- Begin/End of Paragraph.
// Home/End Top/Bottom of file. (usually don't move the cursor, but will select)

    bool visualNavigation = cursor.visualNavigation();
    cursor.setVisualNavigation(true);

    if (q->camelCaseNavigationEnabled() && op == QTextCursor::WordRight)
        camelCaseRight(cursor, mode);
    else if (q->camelCaseNavigationEnabled() && op == QTextCursor::WordLeft)
        camelCaseLeft(cursor, mode);
    else if (!cursor.movePosition(op, mode) && mode == QTextCursor::MoveAnchor)
        cursor.clearSelection();
    cursor.setVisualNavigation(visualNavigation);

    q->setTextCursor(cursor);
    q->ensureCursorVisible();
    return true;
}

void TextEditorWidget::viewPageUp()
{
    verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepSub);
}

void TextEditorWidget::viewPageDown()
{
    verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepAdd);
}

void TextEditorWidget::viewLineUp()
{
    verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
}

void TextEditorWidget::viewLineDown()
{
    verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
}

static inline bool isModifier(QKeyEvent *e)
{
    if (!e)
        return false;
    switch (e->key()) {
    case Qt::Key_Shift:
    case Qt::Key_Control:
    case Qt::Key_Meta:
    case Qt::Key_Alt:
        return true;
    default:
        return false;
    }
}

static inline bool isPrintableText(const QString &text)
{
    return !text.isEmpty() && (text.at(0).isPrint() || text.at(0) == QLatin1Char('\t'));
}

void TextEditorWidget::keyPressEvent(QKeyEvent *e)
{
    if (!isModifier(e) && mouseHidingEnabled())
        viewport()->setCursor(Qt::BlankCursor);
    ToolTip::hide();

    d->m_moveLineUndoHack = false;
    d->clearVisibleFoldedBlock();

    if (e->key() == Qt::Key_Alt
            && d->m_behaviorSettings.m_keyboardTooltips) {
        d->m_maybeFakeTooltipEvent = true;
    } else {
        d->m_maybeFakeTooltipEvent = false;
        if (e->key() == Qt::Key_Escape
                && d->m_snippetOverlay->isVisible()) {
            e->accept();
            d->m_snippetOverlay->hide();
            d->m_snippetOverlay->mangle();
            d->m_snippetOverlay->clear();
            QTextCursor cursor = textCursor();
            cursor.clearSelection();
            setTextCursor(cursor);
            return;
        }
    }

    bool ro = isReadOnly();
    const bool inOverwriteMode = overwriteMode();

    if (!ro && d->m_inBlockSelectionMode) {
        if (e == QKeySequence::Cut) {
            cut();
            e->accept();
            return;
        } else if (e == QKeySequence::Delete || e->key() == Qt::Key_Backspace) {
            if (d->m_blockSelection.positionColumn == d->m_blockSelection.anchorColumn) {
                if (e == QKeySequence::Delete)
                    ++d->m_blockSelection.positionColumn;
                else if (d->m_blockSelection.positionColumn > 0)
                    --d->m_blockSelection.positionColumn;
            }
            d->removeBlockSelection();
            e->accept();
            return;
        } else if (e == QKeySequence::Paste) {
            d->removeBlockSelection();
            // continue
        }
    }


    if (!ro
        && (e == QKeySequence::InsertParagraphSeparator
            || (!d->m_lineSeparatorsAllowed && e == QKeySequence::InsertLineSeparator))) {
        if (d->m_inBlockSelectionMode) {
            d->disableBlockSelection(false);
            e->accept();
            return;
        }
        if (d->m_snippetOverlay->isVisible()) {
            e->accept();
            d->m_snippetOverlay->hide();
            d->m_snippetOverlay->mangle();
            d->m_snippetOverlay->clear();
            QTextCursor cursor = textCursor();
            cursor.movePosition(QTextCursor::EndOfBlock);
            setTextCursor(cursor);
            return;
        }

        QTextCursor cursor = textCursor();
        const TabSettings &ts = d->m_document->tabSettings();
        const TypingSettings &tps = d->m_document->typingSettings();
        cursor.beginEditBlock();

        int extraBlocks =
            d->m_autoCompleter->paragraphSeparatorAboutToBeInserted(cursor,
                                                                    d->m_document->tabSettings());

        QString previousIndentationString;
        if (tps.m_autoIndent) {
            cursor.insertBlock();
            d->m_document->autoIndent(cursor);
        } else {
            cursor.insertBlock();

            // After inserting the block, to avoid duplicating whitespace on the same line
            const QString &previousBlockText = cursor.block().previous().text();
            previousIndentationString = ts.indentationString(previousBlockText);
            if (!previousIndentationString.isEmpty())
                cursor.insertText(previousIndentationString);
        }
        cursor.endEditBlock();
        e->accept();

        if (extraBlocks > 0) {
            QTextCursor ensureVisible = cursor;
            while (extraBlocks > 0) {
                --extraBlocks;
                ensureVisible.movePosition(QTextCursor::NextBlock);
                if (tps.m_autoIndent)
                    d->m_document->autoIndent(ensureVisible);
                else if (!previousIndentationString.isEmpty())
                    ensureVisible.insertText(previousIndentationString);
            }
            setTextCursor(ensureVisible);
        }

        setTextCursor(cursor);
        return;
    } else if (!ro
               && (e == QKeySequence::MoveToStartOfBlock
                   || e == QKeySequence::SelectStartOfBlock)){
        if ((e->modifiers() & (Qt::AltModifier | Qt::ShiftModifier)) == (Qt::AltModifier | Qt::ShiftModifier)) {
            e->accept();
            return;
        }
        d->handleHomeKey(e == QKeySequence::SelectStartOfBlock);
        e->accept();
        return;
    } else if (!ro
               && (e == QKeySequence::MoveToStartOfLine
                   || e == QKeySequence::SelectStartOfLine)){
        if ((e->modifiers() & (Qt::AltModifier | Qt::ShiftModifier)) == (Qt::AltModifier | Qt::ShiftModifier)) {
                e->accept();
                return;
        }
        QTextCursor cursor = textCursor();
        if (QTextLayout *layout = cursor.block().layout()) {
            if (layout->lineForTextPosition(cursor.position() - cursor.block().position()).lineNumber() == 0) {
                d->handleHomeKey(e == QKeySequence::SelectStartOfLine);
                e->accept();
                return;
            }
        }
    } else if (!ro
               && e == QKeySequence::DeleteStartOfWord
               && d->m_document->typingSettings().m_autoIndent
               && !textCursor().hasSelection()){
        e->accept();
        QTextCursor c = textCursor();
        int pos = c.position();
        if (camelCaseNavigationEnabled())
            d->camelCaseLeft(c, QTextCursor::MoveAnchor);
        else
            c.movePosition(QTextCursor::StartOfWord, QTextCursor::MoveAnchor);
        int targetpos = c.position();
        forever {
            d->handleBackspaceKey();
            int cpos = textCursor().position();
            if (cpos == pos || cpos <= targetpos)
                break;
            pos = cpos;
        }
        return;
    } else if (!ro && e == QKeySequence::DeleteStartOfWord && !textCursor().hasSelection()) {
        e->accept();
        QTextCursor c = textCursor();
        if (camelCaseNavigationEnabled())
            d->camelCaseLeft(c, QTextCursor::KeepAnchor);
        else
            c.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
        c.removeSelectedText();
        return;
    } else if (!ro && e == QKeySequence::DeleteEndOfWord && !textCursor().hasSelection()) {
        e->accept();
        QTextCursor c = textCursor();
        if (camelCaseNavigationEnabled())
            d->camelCaseRight(c, QTextCursor::KeepAnchor);
        else
            c.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
        c.removeSelectedText();
        return;
    } else if (!ro && (e == QKeySequence::MoveToNextPage || e == QKeySequence::MoveToPreviousPage)
               && d->m_inBlockSelectionMode) {
        d->disableBlockSelection(false);
        QPlainTextEdit::keyPressEvent(e);
        return;
    } else if (!ro && (e == QKeySequence::SelectNextPage || e == QKeySequence::SelectPreviousPage)
               && d->m_inBlockSelectionMode) {
        QPlainTextEdit::keyPressEvent(e);
        d->m_blockSelection.positionBlock = QPlainTextEdit::textCursor().blockNumber();
        doSetTextCursor(d->m_blockSelection.selection(d->m_document.data()), true);
        viewport()->update();
        e->accept();
        return;
    } else switch (e->key()) {


#if 0
    case Qt::Key_Dollar: {
            d->m_overlay->setVisible(!d->m_overlay->isVisible());
            d->m_overlay->setCursor(textCursor());
            e->accept();
        return;

    } break;
#endif
    case Qt::Key_Tab:
    case Qt::Key_Backtab: {
        if (ro) break;
        if (d->m_snippetOverlay->isVisible() && !d->m_snippetOverlay->isEmpty()) {
            d->snippetTabOrBacktab(e->key() == Qt::Key_Tab);
            e->accept();
            return;
        }
        QTextCursor cursor = textCursor();
        int newPosition;
        if (d->m_document->typingSettings().tabShouldIndent(document(), cursor, &newPosition)) {
            if (newPosition != cursor.position() && !cursor.hasSelection()) {
                cursor.setPosition(newPosition);
                setTextCursor(cursor);
            }
            d->m_document->autoIndent(cursor);
        } else {
            if (d->m_inBlockSelectionMode
                    && d->m_blockSelection.firstVisualColumn() != d->m_blockSelection.lastVisualColumn()) {
                d->removeBlockSelection();
            } else {
                if (e->key() == Qt::Key_Tab)
                    indent();
                else
                    unindent();
            }
        }
        e->accept();
        return;
    } break;
    case Qt::Key_Backspace:
        if (ro) break;
        if ((e->modifiers() & (Qt::ControlModifier
                               | Qt::ShiftModifier
                               | Qt::AltModifier
                               | Qt::MetaModifier)) == Qt::NoModifier
            && !textCursor().hasSelection()) {
            d->handleBackspaceKey();
            e->accept();
            return;
        }
        break;
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Right:
    case Qt::Key_Left:
        if (HostOsInfo::isMacHost())
            break;
        if ((e->modifiers()
             & (Qt::AltModifier | Qt::ShiftModifier)) == (Qt::AltModifier | Qt::ShiftModifier)) {
            if (!d->m_inBlockSelectionMode)
                d->enableBlockSelection(textCursor());
            switch (e->key()) {
            case Qt::Key_Up:
                if (d->m_blockSelection.positionBlock > 0)
                    --d->m_blockSelection.positionBlock;
                break;
            case Qt::Key_Down:
                if (d->m_blockSelection.positionBlock < document()->blockCount() - 1)
                    ++d->m_blockSelection.positionBlock;
                break;
            case Qt::Key_Left:
                if (d->m_blockSelection.positionColumn > 0)
                    --d->m_blockSelection.positionColumn;
                break;
            case Qt::Key_Right:
                ++d->m_blockSelection.positionColumn;
                break;
            default:
                break;
            }
            d->resetCursorFlashTimer();
            doSetTextCursor(d->m_blockSelection.selection(d->m_document.data()), true);
            viewport()->update();
            e->accept();
            return;
        } else if (d->m_inBlockSelectionMode) { // leave block selection mode
            d->disableBlockSelection();
        }
        break;
    case Qt::Key_Insert:
        if (ro) break;
        if (e->modifiers() == Qt::NoModifier) {
            AutoCompleter *ac = d->m_autoCompleter.data();
            if (inOverwriteMode) {
                ac->setAutoParenthesesEnabled(d->autoParenthesisOverwriteBackup);
                ac->setSurroundWithEnabled(d->surroundWithEnabledOverwriteBackup);
                setOverwriteMode(false);
                viewport()->update();
            } else {
                d->autoParenthesisOverwriteBackup = ac->isAutoParenthesesEnabled();
                d->surroundWithEnabledOverwriteBackup = ac->isSurroundWithEnabled();
                ac->setAutoParenthesesEnabled(false);
                ac->setSurroundWithEnabled(false);
                setOverwriteMode(true);
            }
            e->accept();
            return;
        }
        break;

    default:
        break;
    }

    const QString eventText = e->text();
    if (!ro && d->m_inBlockSelectionMode) {
        if (isPrintableText(eventText)) {
            d->insertIntoBlockSelection(eventText);
            goto skip_event;
        }
    }

    if (e->key() == Qt::Key_H
            && e->modifiers() == Qt::KeyboardModifiers(HostOsInfo::controlModifier())) {
        d->universalHelper();
        e->accept();
        return;
    }

    if (ro || !isPrintableText(eventText)) {
        if (!d->cursorMoveKeyEvent(e)) {
            QTextCursor cursor = textCursor();
            bool cursorWithinSnippet = false;
            if (d->m_snippetOverlay->isVisible()
                && (e->key() == Qt::Key_Delete || e->key() == Qt::Key_Backspace)) {
                cursorWithinSnippet = d->snippetCheckCursor(cursor);
            }
            if (cursorWithinSnippet)
                cursor.beginEditBlock();

            QPlainTextEdit::keyPressEvent(e);

            if (cursorWithinSnippet) {
                cursor.endEditBlock();
                d->m_snippetOverlay->updateEquivalentSelections(textCursor());
            }
        }
    } else if ((e->modifiers() & (Qt::ControlModifier|Qt::AltModifier)) != Qt::ControlModifier){
        // only go here if control is not pressed, except if also alt is pressed
        // because AltGr maps to Alt + Ctrl
        QTextCursor cursor = textCursor();
        const QString &autoText = d->m_autoCompleter->autoComplete(cursor, eventText);

        QChar electricChar;
        if (d->m_document->typingSettings().m_autoIndent) {
            foreach (QChar c, eventText) {
                if (d->m_document->indenter()->isElectricCharacter(c)) {
                    electricChar = c;
                    break;
                }
            }
        }

        bool cursorWithinSnippet = false;
        if (d->m_snippetOverlay->isVisible())
            cursorWithinSnippet = d->snippetCheckCursor(cursor);

        bool doEditBlock = !electricChar.isNull() || !autoText.isEmpty() || cursorWithinSnippet;
        if (doEditBlock)
            cursor.beginEditBlock();

        if (inOverwriteMode) {
            if (!doEditBlock)
                cursor.beginEditBlock();
            QTextBlock block = cursor.block();
            int eolPos = block.position() + block.length() - 1;
            int selEndPos = qMin(cursor.position() + eventText.length(), eolPos);
            cursor.setPosition(selEndPos, QTextCursor::KeepAnchor);
            cursor.insertText(eventText);
            if (!doEditBlock)
                cursor.endEditBlock();
        } else {
            cursor.insertText(eventText);
        }

        if (!autoText.isEmpty()) {
            int pos = cursor.position();
            cursor.insertText(autoText);
            //Select the inserted text, to be able to re-indent the inserted text
            cursor.setPosition(pos, QTextCursor::KeepAnchor);
        }
        if (!electricChar.isNull() && d->m_autoCompleter->contextAllowsElectricCharacters(cursor))
            d->m_document->autoIndent(cursor, electricChar);
        if (!autoText.isEmpty())
            cursor.setPosition(autoText.length() == 1 ? cursor.position() : cursor.anchor());

        if (doEditBlock) {
            cursor.endEditBlock();
            if (cursorWithinSnippet)
                d->m_snippetOverlay->updateEquivalentSelections(textCursor());
        }

        setTextCursor(cursor);
    }

    skip_event:
    if (!ro && e->key() == Qt::Key_Delete && d->m_parenthesesMatchingEnabled)
        d->m_parenthesesMatchingTimer.start(50);

    if (!ro && d->m_contentsChanged && isPrintableText(eventText) && !inOverwriteMode)
        d->m_codeAssistant.process();
}

void TextEditorWidget::insertCodeSnippet(const QTextCursor &cursor_arg, const QString &snippet)
{
    Snippet::ParsedSnippet data = Snippet::parse(snippet);

    if (!data.success) {
        QString message = QString::fromLatin1("Cannot parse snippet \"%1\".").arg(snippet);
        if (!data.errorMessage.isEmpty())
            message += QLatin1String("\nParse error: ") + data.errorMessage;
        QMessageBox::warning(this, QLatin1String("Snippet Parse Error"), message);
        return;
    }

    QTextCursor cursor = cursor_arg;
    cursor.beginEditBlock();
    cursor.removeSelectedText();
    const int startCursorPosition = cursor.position();

    cursor.insertText(data.text);
    QList<QTextEdit::ExtraSelection> selections;

    QList<NameMangler *> manglers;
    for (int i = 0; i < data.ranges.count(); ++i) {
        int position = data.ranges.at(i).start + startCursorPosition;
        int length = data.ranges.at(i).length;

        QTextCursor tc(document());
        tc.setPosition(position);
        tc.setPosition(position + length, QTextCursor::KeepAnchor);
        QTextEdit::ExtraSelection selection;
        selection.cursor = tc;
        selection.format = (length
                            ? textDocument()->fontSettings().toTextCharFormat(C_OCCURRENCES)
                            : textDocument()->fontSettings().toTextCharFormat(C_OCCURRENCES_RENAME));
        selections.append(selection);
        manglers << data.ranges.at(i).mangler;
    }

    cursor.setPosition(startCursorPosition, QTextCursor::KeepAnchor);
    d->m_document->autoIndent(cursor);
    cursor.endEditBlock();

    setExtraSelections(TextEditorWidget::SnippetPlaceholderSelection, selections);
    d->m_snippetOverlay->setNameMangler(manglers);

    if (!selections.isEmpty()) {
        const QTextEdit::ExtraSelection &selection = selections.first();

        cursor = textCursor();
        if (selection.cursor.hasSelection()) {
            cursor.setPosition(selection.cursor.selectionStart());
            cursor.setPosition(selection.cursor.selectionEnd(), QTextCursor::KeepAnchor);
        } else {
            cursor.setPosition(selection.cursor.position());
        }
        setTextCursor(cursor);
    }
}

void TextEditorWidgetPrivate::universalHelper()
{
    // Test function for development. Place your new fangled experiment here to
    // give it proper scrutiny before pushing it onto others.
}

void TextEditorWidget::doSetTextCursor(const QTextCursor &cursor, bool keepBlockSelection)
{
    // workaround for QTextControl bug
    bool selectionChange = cursor.hasSelection() || textCursor().hasSelection();
    if (!keepBlockSelection && d->m_inBlockSelectionMode)
        d->disableBlockSelection(false);
    QTextCursor c = cursor;
    c.setVisualNavigation(true);
    QPlainTextEdit::doSetTextCursor(c);
    if (selectionChange)
        d->slotSelectionChanged();
}

void TextEditorWidget::doSetTextCursor(const QTextCursor &cursor)
{
    doSetTextCursor(cursor, false);
}

void TextEditorWidget::gotoLine(int line, int column, bool centerLine)
{
    d->m_lastCursorChangeWasInteresting = false; // avoid adding the previous position to history
    const int blockNumber = qMin(line, document()->blockCount()) - 1;
    const QTextBlock &block = document()->findBlockByNumber(blockNumber);
    if (block.isValid()) {
        QTextCursor cursor(block);
        if (column > 0) {
            cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, column);
        } else {
            int pos = cursor.position();
            while (document()->characterAt(pos).category() == QChar::Separator_Space) {
                ++pos;
            }
            cursor.setPosition(pos);
        }
        setTextCursor(cursor);

        if (centerLine)
            centerCursor();
        else
            ensureCursorVisible();
    }
    d->saveCurrentCursorPositionForNavigation();
}

int TextEditorWidget::position(TextPositionOperation posOp, int at) const
{
    QTextCursor tc = textCursor();

    if (at != -1)
        tc.setPosition(at);

    if (posOp == CurrentPosition)
        return tc.position();

    switch (posOp) {
    case EndOfLinePosition:
        tc.movePosition(QTextCursor::EndOfLine);
        return tc.position();
    case StartOfLinePosition:
        tc.movePosition(QTextCursor::StartOfLine);
        return tc.position();
    case AnchorPosition:
        if (tc.hasSelection())
            return tc.anchor();
        break;
    case EndOfDocPosition:
        tc.movePosition(QTextCursor::End);
        return tc.position();
    default:
        break;
    }

    return -1;
}

QRect TextEditorWidget::cursorRect(int pos) const
{
    QTextCursor tc = textCursor();
    if (pos >= 0)
        tc.setPosition(pos);
    QRect result = cursorRect(tc);
    result.moveTo(viewport()->mapToGlobal(result.topLeft()));
    return result;
}

void TextEditorWidget::convertPosition(int pos, int *line, int *column) const
{
    Convenience::convertPosition(document(), pos, line, column);
}

bool TextEditorWidget::event(QEvent *e)
{
    // FIXME: That's far too heavy, and triggers e.g for ChildEvent
    if (d && e->type() != QEvent::InputMethodQuery)
        d->m_contentsChanged = false;
    switch (e->type()) {
    case QEvent::ShortcutOverride:
        if (static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape && d->m_snippetOverlay->isVisible()) {
            e->accept();
            return true;
        }
        e->ignore(); // we are a really nice citizen
        d->m_maybeFakeTooltipEvent = false;
        return true;
    case QEvent::ApplicationPaletteChange: {
        // slight hack: ignore palette changes
        // at this point the palette has changed already,
        // so undo it by re-setting the palette:
        applyFontSettings();
        return true;
    }
    default:
        break;
    }

    return QPlainTextEdit::event(e);
}

void TextEditorWidget::inputMethodEvent(QInputMethodEvent *e)
{
    if (d->m_inBlockSelectionMode) {
        if (!e->commitString().isEmpty())
            d->insertIntoBlockSelection(e->commitString());
        return;
    }
    QPlainTextEdit::inputMethodEvent(e);
}

void TextEditorWidgetPrivate::documentAboutToBeReloaded()
{
    //memorize cursor position
    m_tempState = q->saveState();

    // remove extra selections (loads of QTextCursor objects)

    m_extraSelections.clear();
    m_extraSelections.reserve(NExtraSelectionKinds);
    q->QPlainTextEdit::setExtraSelections(QList<QTextEdit::ExtraSelection>());

    // clear all overlays
    m_overlay->clear();
    m_snippetOverlay->clear();
    m_searchResultOverlay->clear();
    m_refactorOverlay->clear();

    // clear search results
    m_searchResults.clear();
}

void TextEditorWidgetPrivate::documentReloadFinished(bool success)
{
    if (!success)
        return;

    // restore cursor position
    q->restoreState(m_tempState);
    updateCannotDecodeInfo();
}

QByteArray TextEditorWidget::saveState() const
{
    QByteArray state;
    QDataStream stream(&state, QIODevice::WriteOnly);
    stream << 1; // version number
    stream << verticalScrollBar()->value();
    stream << horizontalScrollBar()->value();
    int line, column;
    convertPosition(textCursor().position(), &line, &column);
    stream << line;
    stream << column;

    // store code folding state
    QList<int> foldedBlocks;
    QTextBlock block = document()->firstBlock();
    while (block.isValid()) {
        if (block.userData() && static_cast<TextBlockUserData*>(block.userData())->folded()) {
            int number = block.blockNumber();
            foldedBlocks += number;
        }
        block = block.next();
    }
    stream << foldedBlocks;

    return state;
}

bool TextEditorWidget::restoreState(const QByteArray &state)
{
    if (state.isEmpty()) {
        if (d->m_displaySettings.m_autoFoldFirstComment)
            d->foldLicenseHeader();
        return false;
    }
    int version;
    int vval;
    int hval;
    int lval;
    int cval;
    QDataStream stream(state);
    stream >> version;
    stream >> vval;
    stream >> hval;
    stream >> lval;
    stream >> cval;

    if (version >= 1) {
        QList<int> collapsedBlocks;
        stream >> collapsedBlocks;
        QTextDocument *doc = document();
        bool layoutChanged = false;
        foreach (int blockNumber, collapsedBlocks) {
            QTextBlock block = doc->findBlockByNumber(qMax(0, blockNumber));
            if (block.isValid()) {
                TextDocumentLayout::doFoldOrUnfold(block, false);
                layoutChanged = true;
            }
        }
        if (layoutChanged) {
            TextDocumentLayout *documentLayout =
                    qobject_cast<TextDocumentLayout*>(doc->documentLayout());
            QTC_ASSERT(documentLayout, return false);
            documentLayout->requestUpdate();
            documentLayout->emitDocumentSizeChanged();
        }
    } else {
        if (d->m_displaySettings.m_autoFoldFirstComment)
            d->foldLicenseHeader();
    }

    d->m_lastCursorChangeWasInteresting = false; // avoid adding last position to history
    gotoLine(lval, cval);
    verticalScrollBar()->setValue(vval);
    horizontalScrollBar()->setValue(hval);
    d->saveCurrentCursorPositionForNavigation();
    return true;
}

void TextEditorWidget::setParenthesesMatchingEnabled(bool b)
{
    d->m_parenthesesMatchingEnabled = b;
}

bool TextEditorWidget::isParenthesesMatchingEnabled() const
{
    return d->m_parenthesesMatchingEnabled;
}

void TextEditorWidget::setHighlightCurrentLine(bool b)
{
    d->m_highlightCurrentLine = b;
    d->updateCurrentLineHighlight();
}

bool TextEditorWidget::highlightCurrentLine() const
{
    return d->m_highlightCurrentLine;
}

void TextEditorWidget::setLineNumbersVisible(bool b)
{
    d->m_lineNumbersVisible = b;
    d->slotUpdateExtraAreaWidth();
}

bool TextEditorWidget::lineNumbersVisible() const
{
    return d->m_lineNumbersVisible;
}

void TextEditorWidget::setAlwaysOpenLinksInNextSplit(bool b)
{
    d->m_displaySettings.m_openLinksInNextSplit = b;
}

bool TextEditorWidget::alwaysOpenLinksInNextSplit() const
{
    return d->m_displaySettings.m_openLinksInNextSplit;
}

void TextEditorWidget::setMarksVisible(bool b)
{
    d->m_marksVisible = b;
    d->slotUpdateExtraAreaWidth();
}

bool TextEditorWidget::marksVisible() const
{
    return d->m_marksVisible;
}

void TextEditorWidget::setRequestMarkEnabled(bool b)
{
    d->m_requestMarkEnabled = b;
}

bool TextEditorWidget::requestMarkEnabled() const
{
    return d->m_requestMarkEnabled;
}

void TextEditorWidget::setLineSeparatorsAllowed(bool b)
{
    d->m_lineSeparatorsAllowed = b;
}

bool TextEditorWidget::lineSeparatorsAllowed() const
{
    return d->m_lineSeparatorsAllowed;
}

void TextEditorWidgetPrivate::updateCodeFoldingVisible()
{
    const bool visible = m_codeFoldingSupported && m_displaySettings.m_displayFoldingMarkers;
    if (m_codeFoldingVisible != visible) {
        m_codeFoldingVisible = visible;
        slotUpdateExtraAreaWidth();
    }
}

void TextEditorWidgetPrivate::reconfigure()
{
    Utils::MimeDatabase mdb;
    m_document->setMimeType(mdb.mimeTypeForFile(m_document->filePath().toString()).name());
    q->configureGenericHighlighter();
}

bool TextEditorWidget::codeFoldingVisible() const
{
    return d->m_codeFoldingVisible;
}

/**
 * Sets whether code folding is supported by the syntax highlighter. When not
 * supported (the default), this makes sure the code folding is not shown.
 *
 * Needs to be called before calling setCodeFoldingVisible.
 */
void TextEditorWidget::setCodeFoldingSupported(bool b)
{
    d->m_codeFoldingSupported = b;
    d->updateCodeFoldingVisible();
}

bool TextEditorWidget::codeFoldingSupported() const
{
    return d->m_codeFoldingSupported;
}

void TextEditorWidget::setMouseNavigationEnabled(bool b)
{
    d->m_behaviorSettings.m_mouseNavigation = b;
}

bool TextEditorWidget::mouseNavigationEnabled() const
{
    return d->m_behaviorSettings.m_mouseNavigation;
}

void TextEditorWidget::setMouseHidingEnabled(bool b)
{
    d->m_behaviorSettings.m_mouseHiding = b;
}

bool TextEditorWidget::mouseHidingEnabled() const
{
    return d->m_behaviorSettings.m_mouseHiding;
}

void TextEditorWidget::setScrollWheelZoomingEnabled(bool b)
{
    d->m_behaviorSettings.m_scrollWheelZooming = b;
}

bool TextEditorWidget::scrollWheelZoomingEnabled() const
{
    return d->m_behaviorSettings.m_scrollWheelZooming;
}

void TextEditorWidget::setConstrainTooltips(bool b)
{
    d->m_behaviorSettings.m_constrainHoverTooltips = b;
}

bool TextEditorWidget::constrainTooltips() const
{
    return d->m_behaviorSettings.m_constrainHoverTooltips;
}

void TextEditorWidget::setCamelCaseNavigationEnabled(bool b)
{
    d->m_behaviorSettings.m_camelCaseNavigation = b;
}

bool TextEditorWidget::camelCaseNavigationEnabled() const
{
    return d->m_behaviorSettings.m_camelCaseNavigation;
}

void TextEditorWidget::setRevisionsVisible(bool b)
{
    d->m_revisionsVisible = b;
    d->slotUpdateExtraAreaWidth();
}

bool TextEditorWidget::revisionsVisible() const
{
    return d->m_revisionsVisible;
}

void TextEditorWidget::setVisibleWrapColumn(int column)
{
    d->m_visibleWrapColumn = column;
    viewport()->update();
}

int TextEditorWidget::visibleWrapColumn() const
{
    return d->m_visibleWrapColumn;
}

void TextEditorWidget::setAutoCompleter(AutoCompleter *autoCompleter)
{
    d->m_autoCompleter.reset(autoCompleter);
}

AutoCompleter *TextEditorWidget::autoCompleter() const
{
    return d->m_autoCompleter.data();
}

//
// TextEditorWidgetPrivate
//

void TextEditorWidgetPrivate::setupDocumentSignals()
{
    QTextDocument *doc = m_document->document();
    q->QPlainTextEdit::setDocument(doc);
    q->setCursorWidth(2); // Applies to the document layout

    TextDocumentLayout *documentLayout = qobject_cast<TextDocumentLayout*>(doc->documentLayout());
    QTC_CHECK(documentLayout);

    QObject::connect(documentLayout, &QPlainTextDocumentLayout::updateBlock,
                     this, &TextEditorWidgetPrivate::slotUpdateBlockNotify);

    QObject::connect(documentLayout, &TextDocumentLayout::updateExtraArea,
                     m_extraArea, static_cast<void (QWidget::*)()>(&QWidget::update));

    QObject::connect(q, &TextEditorWidget::requestBlockUpdate,
                     documentLayout, &QPlainTextDocumentLayout::updateBlock);

    QObject::connect(documentLayout, &TextDocumentLayout::updateExtraArea,
                     this, &TextEditorWidgetPrivate::scheduleUpdateHighlightScrollBar);

    QObject::connect(documentLayout, &QAbstractTextDocumentLayout::documentSizeChanged,
                     this, &TextEditorWidgetPrivate::scheduleUpdateHighlightScrollBar);

    QObject::connect(documentLayout, &QAbstractTextDocumentLayout::update,
                     this, &TextEditorWidgetPrivate::scheduleUpdateHighlightScrollBar);

    QObject::connect(doc, &QTextDocument::contentsChange,
                     this, &TextEditorWidgetPrivate::editorContentsChange);

    QObject::connect(m_document.data(), &TextDocument::aboutToReload,
                     this, &TextEditorWidgetPrivate::documentAboutToBeReloaded);

    QObject::connect(m_document.data(), &TextDocument::reloadFinished,
                     this, &TextEditorWidgetPrivate::documentReloadFinished);

    QObject::connect(m_document.data(), &TextDocument::tabSettingsChanged,
                     this, &TextEditorWidgetPrivate::updateTabStops);

    QObject::connect(m_document.data(), &TextDocument::fontSettingsChanged,
                     this, &TextEditorWidgetPrivate::applyFontSettingsDelayed);

    slotUpdateExtraAreaWidth();

    TextEditorSettings *settings = TextEditorSettings::instance();

    // Connect to settings change signals
    connect(settings, &TextEditorSettings::fontSettingsChanged,
            m_document.data(), &TextDocument::setFontSettings);
    connect(settings, &TextEditorSettings::typingSettingsChanged,
            q, &TextEditorWidget::setTypingSettings);
    connect(settings, &TextEditorSettings::storageSettingsChanged,
            q, &TextEditorWidget::setStorageSettings);
    connect(settings, &TextEditorSettings::behaviorSettingsChanged,
            q, &TextEditorWidget::setBehaviorSettings);
    connect(settings, &TextEditorSettings::marginSettingsChanged,
            q, &TextEditorWidget::setMarginSettings);
    connect(settings, &TextEditorSettings::displaySettingsChanged,
            q, &TextEditorWidget::setDisplaySettings);
    connect(settings, &TextEditorSettings::completionSettingsChanged,
            q, &TextEditorWidget::setCompletionSettings);
    connect(settings, &TextEditorSettings::extraEncodingSettingsChanged,
            q, &TextEditorWidget::setExtraEncodingSettings);

    connect(q, &TextEditorWidget::requestFontZoom,
            settings, &TextEditorSettings::fontZoomRequested);
    connect(q, &TextEditorWidget::requestZoomReset,
            settings, &TextEditorSettings::zoomResetRequested);

    // Apply current settings
    m_document->setFontSettings(settings->fontSettings());
    m_document->setTabSettings(settings->codeStyle()->tabSettings()); // also set through code style ???
    q->setTypingSettings(settings->typingSettings());
    q->setStorageSettings(settings->storageSettings());
    q->setBehaviorSettings(settings->behaviorSettings());
    q->setMarginSettings(settings->marginSettings());
    q->setDisplaySettings(settings->displaySettings());
    q->setCompletionSettings(settings->completionSettings());
    q->setExtraEncodingSettings(settings->extraEncodingSettings());
    q->setCodeStyle(settings->codeStyle(m_tabSettingsId));
}

bool TextEditorWidgetPrivate::snippetCheckCursor(const QTextCursor &cursor)
{
    if (!m_snippetOverlay->isVisible() || m_snippetOverlay->isEmpty())
        return false;

    QTextCursor start = cursor;
    start.setPosition(cursor.selectionStart());
    QTextCursor end = cursor;
    end.setPosition(cursor.selectionEnd());
    if (!m_snippetOverlay->hasCursorInSelection(start)
        || !m_snippetOverlay->hasCursorInSelection(end)
        || m_snippetOverlay->hasFirstSelectionBeginMoved()) {
        m_snippetOverlay->setVisible(false);
        m_snippetOverlay->mangle();
        m_snippetOverlay->clear();
        return false;
    }
    return true;
}

void TextEditorWidgetPrivate::snippetTabOrBacktab(bool forward)
{
    if (!m_snippetOverlay->isVisible() || m_snippetOverlay->isEmpty())
        return;
    QTextCursor cursor = q->textCursor();
    OverlaySelection final;
    if (forward) {
        for (int i = 0; i < m_snippetOverlay->selections().count(); ++i){
            const OverlaySelection &selection = m_snippetOverlay->selections().at(i);
            if (selection.m_cursor_begin.position() >= cursor.position()
                && selection.m_cursor_end.position() > cursor.position()) {
                final = selection;
                break;
            }
        }
    } else {
        for (int i = m_snippetOverlay->selections().count()-1; i >= 0; --i){
            const OverlaySelection &selection = m_snippetOverlay->selections().at(i);
            if (selection.m_cursor_end.position() < cursor.position()) {
                final = selection;
                break;
            }
        }

    }
    if (final.m_cursor_begin.isNull())
        final = forward ? m_snippetOverlay->selections().first() : m_snippetOverlay->selections().last();

    if (final.m_cursor_begin.position() == final.m_cursor_end.position()) { // empty tab stop
        cursor.setPosition(final.m_cursor_end.position());
    } else {
        cursor.setPosition(final.m_cursor_begin.position());
        cursor.setPosition(final.m_cursor_end.position(), QTextCursor::KeepAnchor);
    }
    q->setTextCursor(cursor);
}

// Calculate global position for a tooltip considering the left extra area.
QPoint TextEditorWidget::toolTipPosition(const QTextCursor &c) const
{
    const QPoint cursorPos = mapToGlobal(cursorRect(c).bottomRight() + QPoint(1,1));
    return cursorPos + QPoint(d->m_extraArea->width(), HostOsInfo::isWindowsHost() ? -24 : -16);
}

void TextEditorWidgetPrivate::processTooltipRequest(const QTextCursor &c)
{
    const QPoint toolTipPoint = q->toolTipPosition(c);
    bool handled = false;
    emit q->tooltipOverrideRequested(q, toolTipPoint, c.position(), &handled);
    if (handled)
        return;
    if (!m_hoverHandlers.isEmpty()) {
        m_hoverHandlers.first()->showToolTip(q, toolTipPoint, c.position());
        return;
    }
    emit q->tooltipRequested(toolTipPoint, c.position());
}

bool TextEditorWidget::viewportEvent(QEvent *event)
{
    d->m_contentsChanged = false;
    if (event->type() == QEvent::ToolTip) {
        if (QApplication::keyboardModifiers() & Qt::ControlModifier
                || (!(QApplication::keyboardModifiers() & Qt::ShiftModifier)
                    && d->m_behaviorSettings.m_constrainHoverTooltips)) {
            // Tooltips should be eaten when either control is pressed (so they don't get in the
            // way of code navigation) or if they are in constrained mode and shift is not pressed.
            return true;
        }
        const QHelpEvent *he = static_cast<QHelpEvent*>(event);
        const QPoint &pos = he->pos();

        RefactorMarker refactorMarker = d->m_refactorOverlay->markerAt(pos);
        if (refactorMarker.isValid() && !refactorMarker.tooltip.isEmpty()) {
            ToolTip::show(he->globalPos(), refactorMarker.tooltip,
                          viewport(), QString(), refactorMarker.rect);
            return true;
        }

        QTextCursor tc = cursorForPosition(pos);
        QTextBlock block = tc.block();
        QTextLine line = block.layout()->lineForTextPosition(tc.positionInBlock());
        QTC_CHECK(line.isValid());
        // Only handle tool tip for text cursor if mouse is within the block for the text cursor,
        // and not if the mouse is e.g. in the empty space behind a short line.
        if (line.isValid()
                && pos.x() <= blockBoundingGeometry(block).left() + line.naturalTextRect().right()) {
            d->processTooltipRequest(tc);
            return true;
        }
    }
    return QPlainTextEdit::viewportEvent(event);
}


void TextEditorWidget::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);
    QRect cr = rect();
    d->m_extraArea->setGeometry(
        QStyle::visualRect(layoutDirection(), cr,
                           QRect(cr.left(), cr.top(), extraAreaWidth(), cr.height())));
    d->adjustScrollBarRanges();
    d->updateCurrentLineInScrollbar();
}

QRect TextEditorWidgetPrivate::foldBox()
{
    if (m_highlightBlocksInfo.isEmpty() || extraAreaHighlightFoldedBlockNumber < 0)
        return QRect();

    QTextBlock begin = q->document()->findBlockByNumber(m_highlightBlocksInfo.open.last());

    QTextBlock end = q->document()->findBlockByNumber(m_highlightBlocksInfo.close.first());
    if (!begin.isValid() || !end.isValid())
        return QRect();
    QRectF br = q->blockBoundingGeometry(begin).translated(q->contentOffset());
    QRectF er = q->blockBoundingGeometry(end).translated(q->contentOffset());

    return QRect(m_extraArea->width() - foldBoxWidth(q->fontMetrics()),
                 int(br.top()),
                 foldBoxWidth(q->fontMetrics()),
                 er.bottom() - br.top());
}

QTextBlock TextEditorWidgetPrivate::foldedBlockAt(const QPoint &pos, QRect *box) const
{
    QPointF offset = q->contentOffset();
    QTextBlock block = q->firstVisibleBlock();
    qreal top = q->blockBoundingGeometry(block).translated(offset).top();
    qreal bottom = top + q->blockBoundingRect(block).height();

    int viewportHeight = q->viewport()->height();

    while (block.isValid() && top <= viewportHeight) {
        QTextBlock nextBlock = block.next();
        if (block.isVisible() && bottom >= 0 && q->replacementVisible(block.blockNumber())) {
            if (nextBlock.isValid() && !nextBlock.isVisible()) {
                QTextLayout *layout = block.layout();
                QTextLine line = layout->lineAt(layout->lineCount()-1);
                QRectF lineRect = line.naturalTextRect().translated(offset.x(), top);
                lineRect.adjust(0, 0, -1, -1);

                QString replacement = QLatin1String(" {") + q->foldReplacementText(block)
                        + QLatin1String("}; ");

                QRectF collapseRect(lineRect.right() + 12,
                                    lineRect.top(),
                                    q->fontMetrics().width(replacement),
                                    lineRect.height());
                if (collapseRect.contains(pos)) {
                    QTextBlock result = block;
                    if (box)
                        *box = collapseRect.toAlignedRect();
                    return result;
                } else {
                    block = nextBlock;
                    while (nextBlock.isValid() && !nextBlock.isVisible()) {
                        block = nextBlock;
                        nextBlock = block.next();
                    }
                }
            }
        }

        block = nextBlock;
        top = bottom;
        bottom = top + q->blockBoundingRect(block).height();
    }
    return QTextBlock();
}

void TextEditorWidgetPrivate::highlightSearchResults(const QTextBlock &block,
                                                   TextEditorOverlay *overlay)
{
    if (m_searchExpr.isEmpty())
        return;

    int blockPosition = block.position();

    QTextCursor cursor = q->textCursor();
    QString text = block.text();
    text.replace(QChar::Nbsp, QLatin1Char(' '));
    int idx = -1;
    int l = 1;

    while (idx < text.length()) {
        idx = m_searchExpr.indexIn(text, idx + l);
        if (idx < 0)
            break;
        l = m_searchExpr.matchedLength();
        if (l == 0)
            break;
        if ((m_findFlags & FindWholeWords)
            && ((idx && text.at(idx-1).isLetterOrNumber())
                || (idx + l < text.length() && text.at(idx + l).isLetterOrNumber())))
            continue;

        if (!q->inFindScope(blockPosition + idx, blockPosition + idx + l))
            continue;

        const QTextCharFormat &searchResultFormat
                = m_document->fontSettings().toTextCharFormat(C_SEARCH_RESULT);
        overlay->addOverlaySelection(blockPosition + idx,
                                     blockPosition + idx + l,
                                     searchResultFormat.background().color().darker(120),
                                     QColor(),
                                     (idx == cursor.selectionStart() - blockPosition
                                      && idx + l == cursor.selectionEnd() - blockPosition)?
                                     TextEditorOverlay::DropShadow : 0);

    }
}

QString TextEditorWidgetPrivate::copyBlockSelection()
{
    if (!m_inBlockSelectionMode)
        return QString();
    QString selection;
    const TabSettings &ts = m_document->tabSettings();
    QTextBlock block =
            m_document->document()->findBlockByNumber(m_blockSelection.firstBlockNumber());
    const QTextBlock &lastBlock =
            m_document->document()->findBlockByNumber(m_blockSelection.lastBlockNumber());
    bool textInserted = false;
    for (;;) {
        if (q->selectionVisible(block.blockNumber())) {
            if (textInserted)
                selection += QLatin1Char('\n');
            textInserted = true;

            QString text = block.text();
            int startOffset = 0;
            int startPos = ts.positionAtColumn(text, m_blockSelection.firstVisualColumn(), &startOffset);
            int endOffset = 0;
            int endPos = ts.positionAtColumn(text, m_blockSelection.lastVisualColumn(), &endOffset);

            if (startPos == endPos) {
                selection += QString(endOffset - startOffset, QLatin1Char(' '));
            } else {
                if (startOffset < 0)
                    selection += QString(-startOffset, QLatin1Char(' '));
                if (endOffset < 0)
                    --endPos;
                selection += text.mid(startPos, endPos - startPos);
                if (endOffset < 0)
                    selection += QString(ts.m_tabSize + endOffset, QLatin1Char(' '));
                else if (endOffset > 0)
                    selection += QString(endOffset, QLatin1Char(' '));
            }
        }
        if (block == lastBlock)
            break;

        block = block.next();
    }
    return selection;
}

void TextEditorWidgetPrivate::setCursorToColumn(QTextCursor &cursor, int column, QTextCursor::MoveMode moveMode)
{
    const TabSettings &ts = m_document->tabSettings();
    int offset = 0;
    const int cursorPosition = cursor.position();
    const int pos = ts.positionAtColumn(cursor.block().text(), column, &offset);
    cursor.setPosition(cursor.block().position() + pos, offset == 0 ? moveMode : QTextCursor::MoveAnchor);
    if (offset == 0)
        return;
    if (offset < 0) {
        // the column is inside a tab so it is replaced with spaces
        cursor.setPosition(cursor.block().position() + pos - 1, QTextCursor::KeepAnchor);
        cursor.insertText(ts.indentationString(
                              ts.columnAt(cursor.block().text(), pos - 1),
                              ts.columnAt(cursor.block().text(), pos), cursor.block()));
    } else {
        // column is behind the last position
        cursor.insertText(ts.indentationString(ts.columnAt(cursor.block().text(), pos),
                                               column, cursor.block()));
    }
    if (moveMode == QTextCursor::KeepAnchor)
        cursor.setPosition(cursorPosition);
    cursor.setPosition(cursor.block().position() + ts.positionAtColumn(
                           cursor.block().text(), column), moveMode);
}

void TextEditorWidgetPrivate::insertIntoBlockSelection(const QString &text)
{
    // TODO: add autocompleter support
    QTextCursor cursor = q->textCursor();
    cursor.beginEditBlock();

    if (q->overwriteMode() && m_blockSelection.lastVisualColumn() == m_blockSelection.positionColumn)
        ++m_blockSelection.positionColumn;

    if (m_blockSelection.positionColumn != m_blockSelection.anchorColumn) {
        removeBlockSelection();
        if (!m_inBlockSelectionMode) {
            q->insertPlainText(text);
            cursor.endEditBlock();
            return;
        }
    }

    if (text.isEmpty()) {
        cursor.endEditBlock();
        return;
    }

    int positionBlock = m_blockSelection.positionBlock;
    int anchorBlock = m_blockSelection.anchorBlock;
    int column = m_blockSelection.positionColumn;

    const QTextBlock &firstBlock =
            m_document->document()->findBlockByNumber(m_blockSelection.firstBlockNumber());
    QTextBlock block =
            m_document->document()->findBlockByNumber(m_blockSelection.lastBlockNumber());

    // unify the length of all lines in a multiline text
    const int selectionLineCount = m_blockSelection.lastBlockNumber()
            - m_blockSelection.firstBlockNumber();
    const int textNewLineCount = text.count(QLatin1Char('\n')) ;
    QStringList textLines = text.split(QLatin1Char('\n'));
    const TabSettings &ts = m_document->tabSettings();
    int textLength = 0;
    const QStringList::const_iterator endLine = textLines.constEnd();
    for (QStringList::const_iterator textLine = textLines.constBegin(); textLine != endLine; ++textLine)
        textLength += qMax(0, ts.columnCountForText(*textLine, column) - textLength);
    for (QStringList::iterator textLine = textLines.begin(); textLine != textLines.end(); ++textLine)
        textLine->append(QString(qMax(0, textLength - ts.columnCountForText(*textLine, column)), QLatin1Char(' ')));

    // insert Text
    for (;;) {
        // If the number of lines to be inserted equals the number of the selected lines the
        // lines of the copy paste buffer are inserted in the corresponding lines of the selection.
        // Otherwise the complete buffer is inserted in each of the selected lines.
        cursor.setPosition(block.position());
        if (selectionLineCount == textNewLineCount) {
            setCursorToColumn(cursor, column);
            cursor.insertText(textLines.at(block.blockNumber()
                                           - m_blockSelection.firstBlockNumber()));
        } else {
            QStringList::const_iterator textLine = textLines.constBegin();
            while (true) {
                setCursorToColumn(cursor, column);
                cursor.insertText(*textLine);
                ++textLine;
                if (textLine == endLine)
                    break;
                cursor.movePosition(QTextCursor::EndOfBlock);
                cursor.insertText(QLatin1String("\n"));
                if (qMax(anchorBlock, positionBlock) == anchorBlock)
                    ++anchorBlock;
                else
                    ++positionBlock;
            }
        }
        if (block == firstBlock)
            break;
        block = block.previous();
    }
    cursor.endEditBlock();

    column += textLength;
    m_blockSelection.fromPostition(positionBlock, column, anchorBlock, column);
    q->doSetTextCursor(m_blockSelection.selection(m_document.data()), true);
}

void TextEditorWidgetPrivate::removeBlockSelection()
{
    QTextCursor cursor = q->textCursor();
    if (!cursor.hasSelection() || !m_inBlockSelectionMode)
        return;

    const int firstColumn = m_blockSelection.firstVisualColumn();
    const int lastColumn = m_blockSelection.lastVisualColumn();
    if (firstColumn == lastColumn)
        return;
    const int positionBlock = m_blockSelection.positionBlock;
    const int anchorBlock = m_blockSelection.anchorBlock;

    int cursorPosition = cursor.selectionStart();
    cursor.clearSelection();
    cursor.beginEditBlock();

    const TabSettings &ts = m_document->tabSettings();
    QTextBlock block = m_document->document()->findBlockByNumber(m_blockSelection.firstBlockNumber());
    const QTextBlock &lastBlock = m_document->document()->findBlockByNumber(m_blockSelection.lastBlockNumber());
    for (;;) {
        int startOffset = 0;
        const int startPos = ts.positionAtColumn(block.text(), firstColumn, &startOffset);
        // removing stuff doesn't make sense if the cursor is behind the code
        if (startPos < block.length() - 1 || startOffset < 0) {
            cursor.setPosition(block.position());
            setCursorToColumn(cursor, firstColumn);
            setCursorToColumn(cursor, lastColumn, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
        }
        if (block == lastBlock)
            break;
        block = block.next();
    }

    cursor.setPosition(cursorPosition);
    cursor.endEditBlock();
    m_blockSelection.fromPostition(positionBlock, firstColumn, anchorBlock, firstColumn);
    cursor = m_blockSelection.selection(m_document.data());
    q->doSetTextCursor(cursor, m_blockSelection.hasSelection());
}

void TextEditorWidgetPrivate::enableBlockSelection(const QTextCursor &cursor)
{
    const TabSettings &ts = m_document->tabSettings();
    const QTextBlock &positionTextBlock = cursor.block();
    int positionBlock = positionTextBlock.blockNumber();
    int positionColumn = ts.columnAt(positionTextBlock.text(),
                                     cursor.position() - positionTextBlock.position());

    const QTextDocument *document = cursor.document();
    const QTextBlock &anchorTextBlock = document->findBlock(cursor.anchor());
    int anchorBlock = anchorTextBlock.blockNumber();
    int anchorColumn = ts.columnAt(anchorTextBlock.text(),
                                   cursor.anchor() - anchorTextBlock.position());
    enableBlockSelection(positionBlock, anchorColumn, anchorBlock, positionColumn);
}

void TextEditorWidgetPrivate::enableBlockSelection(int positionBlock, int positionColumn,
                                                       int anchorBlock, int anchorColumn)
{
    m_blockSelection.fromPostition(positionBlock, anchorColumn, anchorBlock, positionColumn);
    resetCursorFlashTimer();
    m_inBlockSelectionMode = true;
    q->doSetTextCursor(m_blockSelection.selection(m_document.data()), true);
    q->viewport()->update();
}

void TextEditorWidgetPrivate::disableBlockSelection(bool keepSelection)
{
    m_inBlockSelectionMode = false;
    m_cursorFlashTimer.stop();
    QTextCursor cursor = m_blockSelection.selection(m_document.data());
    m_blockSelection.clear();
    if (!keepSelection)
        cursor.clearSelection();
    q->setTextCursor(cursor);
    q->viewport()->update();
}

void TextEditorWidgetPrivate::resetCursorFlashTimer()
{
    m_cursorVisible = true;
    const int flashTime = qApp->cursorFlashTime();
    if (flashTime > 0) {
        m_cursorFlashTimer.stop();
        m_cursorFlashTimer.start(flashTime / 2, q);
    }
}

void TextEditorWidgetPrivate::moveCursorVisible(bool ensureVisible)
{
    QTextCursor cursor = q->textCursor();
    if (!cursor.block().isVisible()) {
        cursor.setVisualNavigation(true);
        cursor.movePosition(QTextCursor::Up);
        q->setTextCursor(cursor);
    }
    if (ensureVisible)
        q->ensureCursorVisible();
}

static QColor blendColors(const QColor &a, const QColor &b, int alpha)
{
    return QColor((a.red()   * (256 - alpha) + b.red()   * alpha) / 256,
                  (a.green() * (256 - alpha) + b.green() * alpha) / 256,
                  (a.blue()  * (256 - alpha) + b.blue()  * alpha) / 256);
}

static QColor calcBlendColor(const QColor &baseColor, int level, int count)
{
    QColor color80;
    QColor color90;

    if (baseColor.value() > 128) {
        const int f90 = 15;
        const int f80 = 30;
        color80.setRgb(qMax(0, baseColor.red() - f80),
                       qMax(0, baseColor.green() - f80),
                       qMax(0, baseColor.blue() - f80));
        color90.setRgb(qMax(0, baseColor.red() - f90),
                       qMax(0, baseColor.green() - f90),
                       qMax(0, baseColor.blue() - f90));
    } else {
        const int f90 = 20;
        const int f80 = 40;
        color80.setRgb(qMin(255, baseColor.red() + f80),
                       qMin(255, baseColor.green() + f80),
                       qMin(255, baseColor.blue() + f80));
        color90.setRgb(qMin(255, baseColor.red() + f90),
                       qMin(255, baseColor.green() + f90),
                       qMin(255, baseColor.blue() + f90));
    }

    if (level == count)
        return baseColor;
    if (level == 0)
        return color80;
    if (level == count - 1)
        return color90;

    const int blendFactor = level * (256 / (count - 2));

    return blendColors(color80, color90, blendFactor);
}

static QTextLayout::FormatRange createBlockCursorCharFormatRange(int pos, const QPalette &palette)
{
    QTextLayout::FormatRange o;
    o.start = pos;
    o.length = 1;
    o.format.setForeground(palette.base());
    o.format.setBackground(palette.text());
    return o;
}

void TextEditorWidget::paintEvent(QPaintEvent *e)
{
    // draw backgrond to the right of the wrap column before everything else
    qreal lineX = 0;
    QPointF offset(contentOffset());
    const QRect &viewportRect = viewport()->rect();
    const QRect &er = e->rect();

    const FontSettings &fs = textDocument()->fontSettings();
    const QTextCharFormat &searchScopeFormat = fs.toTextCharFormat(C_SEARCH_SCOPE);
    const QTextCharFormat &ifdefedOutFormat = fs.toTextCharFormat(C_DISABLED_CODE);

    if (d->m_visibleWrapColumn > 0) {
        QPainter painter(viewport());
        // Don't use QFontMetricsF::averageCharWidth here, due to it returning
        // a fractional size even when this is not supported by the platform.
        lineX = QFontMetricsF(font()).width(QLatin1Char('x')) * d->m_visibleWrapColumn + offset.x() + 4;
        if (lineX < viewportRect.width())
            painter.fillRect(QRectF(lineX, er.top(), viewportRect.width() - lineX, er.height()),
                             ifdefedOutFormat.background());
    }

    /*
      Here comes an almost verbatim copy of
      QPlainTextEdit::paintEvent() so we can adjust the extra
      selections dynamically to indicate all search results.
    */
    //begin QPlainTextEdit::paintEvent()

    QPainter painter(viewport());
    QTextDocument *doc = document();
    TextDocumentLayout *documentLayout = qobject_cast<TextDocumentLayout*>(doc->documentLayout());
    QTC_ASSERT(documentLayout, return);

    QTextBlock textCursorBlock = textCursor().block();

    bool hasMainSelection = textCursor().hasSelection();
    bool suppressSyntaxInIfdefedOutBlock = (ifdefedOutFormat.foreground()
                                           != palette().foreground());

    // Set a brush origin so that the WaveUnderline knows where the wave started
    painter.setBrushOrigin(offset);

//    // keep right margin clean from full-width selection
//    int maxX = offset.x() + qMax((qreal)viewportRect.width(), documentLayout->documentSize().width())
//               - doc->documentMargin();
//    er.setRight(qMin(er.right(), maxX));
//    painter.setClipRect(er);

    bool editable = !isReadOnly();
    QTextBlock block = firstVisibleBlock();

    QAbstractTextDocumentLayout::PaintContext context = getPaintContext();

    int documentWidth = int(document()->size().width());

    if (!d->m_highlightBlocksInfo.isEmpty()) {
        const QColor baseColor = palette().base().color();

        // extra pass for the block highlight

        const int margin = 5;
        QTextBlock blockFP = block;
        QPointF offsetFP = offset;
        while (blockFP.isValid()) {
            QRectF r = blockBoundingRect(blockFP).translated(offsetFP);

            int n = blockFP.blockNumber();
            int depth = 0;
            foreach (int i, d->m_highlightBlocksInfo.open)
                if (n >= i)
                    ++depth;
            foreach (int i, d->m_highlightBlocksInfo.close)
                if (n > i)
                    --depth;

            int count = d->m_highlightBlocksInfo.count();
            if (count) {
                for (int i = 0; i <= depth; ++i) {
                    const QColor &blendedColor = calcBlendColor(baseColor, i, count);
                    int vi = i > 0 ? d->m_highlightBlocksInfo.visualIndent.at(i-1) : 0;
                    QRectF oneRect = r;
                    oneRect.setWidth(qMax(viewport()->width(), documentWidth));
                    oneRect.adjust(vi, 0, 0, 0);
                    if (oneRect.left() >= oneRect.right())
                        continue;
                    if (lineX > 0 && oneRect.left() < lineX && oneRect.right() > lineX) {
                        QRectF otherRect = r;
                        otherRect.setLeft(lineX + 1);
                        otherRect.setRight(oneRect.right());
                        oneRect.setRight(lineX - 1);
                        painter.fillRect(otherRect, blendedColor);
                    }
                    painter.fillRect(oneRect, blendedColor);
                }
            }
            offsetFP.ry() += r.height();

            if (offsetFP.y() > viewportRect.height() + margin)
                break;

            blockFP = blockFP.next();
            if (!blockFP.isVisible()) {
                // invisible blocks do have zero line count
                blockFP = doc->findBlockByLineNumber(blockFP.firstLineNumber());
            }
        }
    }

    int blockSelectionIndex = -1;

    if (d->m_inBlockSelectionMode && context.selections.count()
            && context.selections.last().cursor == textCursor()) {
        blockSelectionIndex = context.selections.size()-1;
        context.selections[blockSelectionIndex].format.clearBackground();
    }

    QTextBlock visibleCollapsedBlock;
    QPointF visibleCollapsedBlockOffset;

    QTextLayout *cursor_layout = 0;
    QPointF cursor_offset;
    int cursor_cpos = 0;
    QPen cursor_pen;

    d->m_searchResultOverlay->clear();
    if (!d->m_searchExpr.isEmpty()) { // first pass for the search result overlays

        const int margin = 5;
        QTextBlock blockFP = block;
        QPointF offsetFP = offset;
        while (blockFP.isValid()) {
            QRectF r = blockBoundingRect(blockFP).translated(offsetFP);

            if (r.bottom() >= er.top() - margin && r.top() <= er.bottom() + margin) {
                d->highlightSearchResults(blockFP,
                                          d->m_searchResultOverlay);
            }
            offsetFP.ry() += r.height();

            if (offsetFP.y() > viewportRect.height() + margin)
                break;

            blockFP = blockFP.next();
            if (!blockFP.isVisible()) {
                // invisible blocks do have zero line count
                blockFP = doc->findBlockByLineNumber(blockFP.firstLineNumber());
            }
        }

    } // end first pass


    { // extra pass for ifdefed out blocks
        QTextBlock blockIDO = block;
        QPointF offsetIDO = offset;
        while (blockIDO.isValid()) {

            QRectF r = blockBoundingRect(blockIDO).translated(offsetIDO);

            if (r.bottom() >= er.top() && r.top() <= er.bottom()) {
                if (TextDocumentLayout::ifdefedOut(blockIDO)) {
                    QRectF rr = r;
                    rr.setRight(viewportRect.width() - offset.x());
                    if (lineX > 0)
                        rr.setRight(qMin(lineX, rr.right()));
                    painter.fillRect(rr, ifdefedOutFormat.background());
                }
            }
            offsetIDO.ry() += r.height();

            if (offsetIDO.y() > viewportRect.height())
                break;

            blockIDO = blockIDO.next();
            if (!blockIDO.isVisible()) {
                // invisible blocks do have zero line count
                blockIDO = doc->findBlockByLineNumber(blockIDO.firstLineNumber());
            }

        }
    }

    // draw wrap column after ifdefed out blocks
    if (d->m_visibleWrapColumn > 0) {
        if (lineX < viewportRect.width()) {
            const QBrush background = ifdefedOutFormat.background();
            const QColor col = (palette().base().color().value() > 128) ? Qt::black : Qt::white;
            const QPen pen = painter.pen();
            painter.setPen(blendColors(background.isOpaque() ? background.color() : palette().base().color(),
                                       col, 32));
            painter.drawLine(QPointF(lineX, er.top()), QPointF(lineX, er.bottom()));
            painter.setPen(pen);
        }
    }

    // possible extra pass for the block selection find scope
    if (!d->m_findScopeStart.isNull() && d->m_findScopeVerticalBlockSelectionFirstColumn >= 0) {
        QTextBlock blockFS = block;
        QPointF offsetFS = offset;
        while (blockFS.isValid()) {

            QRectF r = blockBoundingRect(blockFS).translated(offsetFS);

            if (r.bottom() >= er.top() && r.top() <= er.bottom()) {

                if (blockFS.position() >= d->m_findScopeStart.block().position()
                        && blockFS.position() <= d->m_findScopeEnd.block().position()) {
                    QTextLayout *layout = blockFS.layout();
                    QString text = blockFS.text();
                    const TabSettings &ts = d->m_document->tabSettings();
                    qreal spacew = QFontMetricsF(font()).width(QLatin1Char(' '));

                    int offset = 0;
                    int relativePos  =  ts.positionAtColumn(text,
                                                            d->m_findScopeVerticalBlockSelectionFirstColumn,
                                                            &offset);
                    QTextLine line = layout->lineForTextPosition(relativePos);
                    qreal x = line.cursorToX(relativePos) + offset * spacew;

                    int eoffset = 0;
                    int erelativePos  =  ts.positionAtColumn(text,
                                                             d->m_findScopeVerticalBlockSelectionLastColumn,
                                                             &eoffset);
                    QTextLine eline = layout->lineForTextPosition(erelativePos);
                    qreal ex = eline.cursorToX(erelativePos) + eoffset * spacew;

                    QRectF rr = line.naturalTextRect();
                    rr.moveTop(rr.top() + r.top());
                    rr.setLeft(r.left() + x);
                    if (line.lineNumber() == eline.lineNumber())
                        rr.setRight(r.left() + ex);
                    painter.fillRect(rr, searchScopeFormat.background());

                    QColor lineCol = searchScopeFormat.foreground().color();
                    QPen pen = painter.pen();
                    painter.setPen(lineCol);
                    if (blockFS == d->m_findScopeStart.block())
                        painter.drawLine(rr.topLeft(), rr.topRight());
                    if (blockFS == d->m_findScopeEnd.block())
                        painter.drawLine(rr.bottomLeft(), rr.bottomRight());
                    painter.drawLine(rr.topLeft(), rr.bottomLeft());
                    painter.drawLine(rr.topRight(), rr.bottomRight());
                    painter.setPen(pen);
                }
            }
            offsetFS.ry() += r.height();

            if (offsetFS.y() > viewportRect.height())
                break;

            blockFS = blockFS.next();
            if (!blockFS.isVisible()) {
                // invisible blocks do have zero line count
                blockFS = doc->findBlockByLineNumber(blockFS.firstLineNumber());
            }

        }
    }

    if (!d->m_findScopeStart.isNull() && d->m_findScopeVerticalBlockSelectionFirstColumn < 0) {

        TextEditorOverlay *overlay = new TextEditorOverlay(this);
        overlay->addOverlaySelection(d->m_findScopeStart.position(),
                                     d->m_findScopeEnd.position(),
                                     searchScopeFormat.foreground().color(),
                                     searchScopeFormat.background().color(),
                                     TextEditorOverlay::ExpandBegin);
        overlay->setAlpha(false);
        overlay->paint(&painter, e->rect());
        delete overlay;
    }



    d->m_searchResultOverlay->fill(&painter,
                                   fs.toTextCharFormat(C_SEARCH_RESULT).background().color(),
                                   e->rect());


    while (block.isValid()) {

        QRectF r = blockBoundingRect(block).translated(offset);

        if (r.bottom() >= er.top() && r.top() <= er.bottom()) {

            QTextLayout *layout = block.layout();

            QTextOption option = layout->textOption();
            if (suppressSyntaxInIfdefedOutBlock && TextDocumentLayout::ifdefedOut(block)) {
                option.setFlags(option.flags() | QTextOption::SuppressColors);
                painter.setPen(ifdefedOutFormat.foreground().color());
            } else {
                option.setFlags(option.flags() & ~QTextOption::SuppressColors);
                painter.setPen(context.palette.text().color());
            }
            layout->setTextOption(option);
            layout->setFont(doc->defaultFont()); // this really should be in qplaintextedit when creating the layout!

            int blpos = block.position();
            int bllen = block.length();

            QVector<QTextLayout::FormatRange> selections;
            QVector<QTextLayout::FormatRange> prioritySelections;

            for (int i = 0; i < context.selections.size(); ++i) {
                const QAbstractTextDocumentLayout::Selection &range = context.selections.at(i);
                const int selStart = range.cursor.selectionStart() - blpos;
                const int selEnd = range.cursor.selectionEnd() - blpos;
                if (selStart < bllen && selEnd >= 0
                    && selEnd >= selStart) {
                    QTextLayout::FormatRange o;
                    o.start = selStart;
                    o.length = selEnd - selStart;
                    o.format = range.format;
                    if (i == blockSelectionIndex) {
                        QString text = block.text();
                        const TabSettings &ts = d->m_document->tabSettings();
                        o.start = ts.positionAtColumn(text, d->m_blockSelection.firstVisualColumn());
                        o.length = ts.positionAtColumn(text, d->m_blockSelection.lastVisualColumn()) - o.start;
                    }
                    if ((hasMainSelection && i == context.selections.size()-1)
                        || (o.format.foreground().style() == Qt::NoBrush
                        && o.format.underlineStyle() != QTextCharFormat::NoUnderline
                        && o.format.background() == Qt::NoBrush)) {
                        if (selectionVisible(block.blockNumber()))
                            prioritySelections.append(o);
                    }
                    else
                        selections.append(o);
                }
#if 0
                // we disable fullwidth selection. It's only used for m_highlightCurrentLine which we
                // do differently now
                else if (!range.cursor.hasSelection() && range.format.hasProperty(QTextFormat::FullWidthSelection)
                    && block.contains(range.cursor.position())) {
                    // for full width selections we don't require an actual selection, just
                    // a position to specify the line. that's more convenience in usage.
                    QTextLayout::FormatRange o;
                    QTextLine l = layout->lineForTextPosition(range.cursor.position() - blpos);
                    o.start = l.textStart();
                    o.length = l.textLength();
                    if (o.start + o.length == bllen - 1)
                        ++o.length; // include newline
                    o.format = range.format;
                    selections.append(o);
                }
#endif
            }
            selections += prioritySelections;

            if (d->m_highlightCurrentLine && block == textCursorBlock) {

                QRectF rr = layout->lineForTextPosition(textCursor().positionInBlock()).rect();
                rr.moveTop(rr.top() + r.top());
                rr.setLeft(0);
                rr.setRight(viewportRect.width() - offset.x());
                QColor color = fs.toTextCharFormat(C_CURRENT_LINE).background().color();
                // set alpha, otherwise we cannot see block highlighting and find scope underneath
                color.setAlpha(128);
                if (!editable && !er.contains(rr.toRect())) {
                    QRect updateRect = er;
                    updateRect.setLeft(0);
                    updateRect.setRight(viewportRect.width() - offset.x());
                    viewport()->update(updateRect);
                }
                painter.fillRect(rr, color);
            }


            QRectF blockSelectionCursorRect;
            if (d->m_inBlockSelectionMode
                    && block.blockNumber() >= d->m_blockSelection.firstBlockNumber()
                    && block.blockNumber() <= d->m_blockSelection.lastBlockNumber()) {
                QString text = block.text();
                const TabSettings &ts = d->m_document->tabSettings();
                const qreal spacew = QFontMetricsF(font()).width(QLatin1Char(' '));
                const int cursorw = overwriteMode() ? QFontMetrics(font()).width(QLatin1Char(' '))
                                                    : cursorWidth();

                int offset = 0;
                int relativePos = ts.positionAtColumn(text, d->m_blockSelection.firstVisualColumn(),
                                                      &offset);
                const QTextLine line = layout->lineForTextPosition(relativePos);
                const qreal x = line.cursorToX(relativePos) + offset * spacew;

                int eoffset = 0;
                int erelativePos = ts.positionAtColumn(text, d->m_blockSelection.lastVisualColumn(),
                                                       &eoffset);
                const QTextLine eline = layout->lineForTextPosition(erelativePos);
                const qreal ex = eline.cursorToX(erelativePos) + eoffset * spacew;

                QRectF rr = line.naturalTextRect();
                rr.moveTop(rr.top() + r.top());
                rr.setLeft(r.left() + x);
                if (line.lineNumber() == eline.lineNumber())
                    rr.setRight(r.left() + ex);
                painter.fillRect(rr, palette().highlight());
                if (d->m_cursorVisible
                        && d->m_blockSelection.firstVisualColumn()
                        == d->m_blockSelection.positionColumn) {
                    if (overwriteMode() && offset == 0
                            && relativePos < text.length()
                            && text.at(relativePos) != QLatin1Char('\t')
                            && text.at(relativePos) != QLatin1Char('\n')) {
                        selections.append(createBlockCursorCharFormatRange(relativePos, palette()));
                    } else {
                        blockSelectionCursorRect = rr;
                        blockSelectionCursorRect.setRight(rr.left() + cursorw);
                    }
                }
                for (int i = line.lineNumber() + 1; i < eline.lineNumber(); ++i) {
                    rr = layout->lineAt(i).naturalTextRect();
                    rr.moveTop(rr.top() + r.top());
                    rr.setLeft(r.left() + x);
                    painter.fillRect(rr, palette().highlight());
                }

                rr = eline.naturalTextRect();
                rr.moveTop(rr.top() + r.top());
                rr.setRight(r.left() + ex);
                if (line.lineNumber() != eline.lineNumber())
                    painter.fillRect(rr, palette().highlight());
                if (d->m_cursorVisible
                        && d->m_blockSelection.lastVisualColumn()
                        == d->m_blockSelection.positionColumn) {
                    if (overwriteMode() && eoffset == 0
                            && erelativePos < text.length()
                            && text.at(erelativePos) != QLatin1Char('\t')
                            && text.at(erelativePos) != QLatin1Char('\n')) {
                        selections.append(createBlockCursorCharFormatRange(erelativePos, palette()));
                    } else {
                        blockSelectionCursorRect = rr;
                        blockSelectionCursorRect.setLeft(rr.right());
                        blockSelectionCursorRect.setRight(rr.right() + cursorw);
                    }
                }
            }


            bool drawCursor = ((editable || true) // we want the cursor in read-only mode
                               && context.cursorPosition >= blpos
                               && context.cursorPosition < blpos + bllen);

            bool drawCursorAsBlock = drawCursor && overwriteMode() && !d->m_inBlockSelectionMode;

            if (drawCursorAsBlock) {
                int relativePos = context.cursorPosition - blpos;
                bool doSelection = true;
                QTextLine line = layout->lineForTextPosition(relativePos);
                qreal x = line.cursorToX(relativePos);
                qreal w = 0;
                if (relativePos < line.textLength() - line.textStart()) {
                    w = line.cursorToX(relativePos + 1) - x;
                    if (doc->characterAt(context.cursorPosition) == QLatin1Char('\t')) {
                        doSelection = false;
                        qreal space = QFontMetricsF(layout->font()).width(QLatin1Char(' '));
                        if (w > space) {
                            x += w-space;
                            w = space;
                        }
                    }
                } else
                    w = QFontMetrics(layout->font()).width(QLatin1Char(' ')); // in sync with QTextLine::draw()

                QRectF rr = line.rect();
                rr.moveTop(rr.top() + r.top());
                rr.moveLeft(r.left() + x);
                rr.setWidth(w);
                painter.fillRect(rr, palette().text());
                if (doSelection)
                    selections.append(createBlockCursorCharFormatRange(relativePos, palette()));
            }


            paintBlock(&painter, block, offset, selections, er);

            if ((drawCursor && !drawCursorAsBlock)
                    || (editable && context.cursorPosition < -1 && !layout->preeditAreaText().isEmpty())) {
                int cpos = context.cursorPosition;
                if (cpos < -1)
                    cpos = layout->preeditAreaPosition() - (cpos + 2);
                else
                    cpos -= blpos;
                cursor_layout = layout;
                cursor_offset = offset;
                cursor_cpos = cpos;
                cursor_pen = painter.pen();
            }

            if ((!HostOsInfo::isMacHost()
                 || d->m_blockSelection.positionColumn == d->m_blockSelection.anchorColumn)
                    && blockSelectionCursorRect.isValid())
                painter.fillRect(blockSelectionCursorRect, palette().text());
        }

        offset.ry() += r.height();

        if (offset.y() > viewportRect.height())
            break;

        block = block.next();

        if (!block.isVisible()) {
            if (block.blockNumber() == d->visibleFoldedBlockNumber) {
                visibleCollapsedBlock = block;
                visibleCollapsedBlockOffset = offset;
            }

            // invisible blocks do have zero line count
            block = doc->findBlockByLineNumber(block.firstLineNumber());
        }
    }
    painter.setPen(context.palette.text().color());

    if (backgroundVisible() && !block.isValid() && offset.y() <= er.bottom()
        && (centerOnScroll() || verticalScrollBar()->maximum() == verticalScrollBar()->minimum())) {
        painter.fillRect(QRect(QPoint((int)er.left(), (int)offset.y()), er.bottomRight()), palette().background());
    }

    //end QPlainTextEdit::paintEvent()

    offset = contentOffset();
    block = firstVisibleBlock();

    qreal top = blockBoundingGeometry(block).translated(offset).top();
    qreal bottom = top + blockBoundingRect(block).height();

    QTextCursor cursor = textCursor();
    bool hasSelection = cursor.hasSelection();
    int selectionStart = cursor.selectionStart();
    int selectionEnd = cursor.selectionEnd();

    while (block.isValid() && top <= e->rect().bottom()) {
        QTextBlock nextBlock = block.next();
        QTextBlock nextVisibleBlock = nextBlock;

        if (!nextVisibleBlock.isVisible()) {
            // invisible blocks do have zero line count
            nextVisibleBlock = doc->findBlockByLineNumber(nextVisibleBlock.firstLineNumber());
            // paranoia in case our code somewhere did not set the line count
            // of the invisible block to 0
            while (nextVisibleBlock.isValid() && !nextVisibleBlock.isVisible())
                nextVisibleBlock = nextVisibleBlock.next();
        }
        if (block.isVisible() && bottom >= e->rect().top()) {
            if (d->m_displaySettings.m_visualizeWhitespace) {
                QTextLayout *layout = block.layout();
                int lineCount = layout->lineCount();
                if (lineCount >= 2 || !nextBlock.isValid()) {
                    painter.save();
                    painter.setPen(Qt::lightGray);
                    for (int i = 0; i < lineCount-1; ++i) { // paint line wrap indicator
                        QTextLine line = layout->lineAt(i);
                        QRectF lineRect = line.naturalTextRect().translated(offset.x(), top);
                        QChar visualArrow((ushort)0x21b5);
                        painter.drawText(QPointF(lineRect.right(),
                                                 lineRect.top() + line.ascent()),
                                         visualArrow);
                    }
                    if (!nextBlock.isValid()) { // paint EOF symbol
                        QTextLine line = layout->lineAt(lineCount-1);
                        QRectF lineRect = line.naturalTextRect().translated(offset.x(), top);
                        int h = 4;
                        lineRect.adjust(0, 0, -1, -1);
                        QPainterPath path;
                        QPointF pos(lineRect.topRight() + QPointF(h+4, line.ascent()));
                        path.moveTo(pos);
                        path.lineTo(pos + QPointF(-h, -h));
                        path.lineTo(pos + QPointF(0, -2*h));
                        path.lineTo(pos + QPointF(h, -h));
                        path.closeSubpath();
                        painter.setBrush(painter.pen().color());
                        painter.drawPath(path);
                    }
                    painter.restore();
                }
            }

            if (nextBlock.isValid() && !nextBlock.isVisible() && replacementVisible(block.blockNumber())) {

                bool selectThis = (hasSelection
                                   && nextBlock.position() >= selectionStart
                                   && nextBlock.position() < selectionEnd);
                painter.save();
                if (selectThis) {
                    painter.setBrush(palette().highlight());
                } else {
                    QColor rc = replacementPenColor(block.blockNumber());
                    if (rc.isValid())
                        painter.setPen(rc);
                }

                QTextLayout *layout = block.layout();
                QTextLine line = layout->lineAt(layout->lineCount()-1);
                QRectF lineRect = line.naturalTextRect().translated(offset.x(), top);
                lineRect.adjust(0, 0, -1, -1);

                QString replacement = foldReplacementText(block);
                QString rectReplacement = QLatin1String(" {") + replacement + QLatin1String("}; ");

                QRectF collapseRect(lineRect.right() + 12,
                                    lineRect.top(),
                                    fontMetrics().width(rectReplacement),
                                    lineRect.height());
                painter.setRenderHint(QPainter::Antialiasing, true);
                painter.translate(.5, .5);
                painter.drawRoundedRect(collapseRect.adjusted(0, 0, 0, -1), 3, 3);
                painter.setRenderHint(QPainter::Antialiasing, false);
                painter.translate(-.5, -.5);

                if (TextBlockUserData *nextBlockUserData = TextDocumentLayout::testUserData(nextBlock)) {
                    if (nextBlockUserData->foldingStartIncluded())
                        replacement.prepend(nextBlock.text().trimmed().left(1));
                }

                block = nextVisibleBlock.previous();
                if (!block.isValid())
                    block = doc->lastBlock();

                if (TextBlockUserData *blockUserData = TextDocumentLayout::testUserData(block)) {
                    if (blockUserData->foldingEndIncluded()) {
                        QString right = block.text().trimmed();
                        if (right.endsWith(QLatin1Char(';'))) {
                            right.chop(1);
                            right = right.trimmed();
                            replacement.append(right.right(right.endsWith(QLatin1Char('/')) ? 2 : 1));
                            replacement.append(QLatin1Char(';'));
                        } else {
                            replacement.append(right.right(right.endsWith(QLatin1Char('/')) ? 2 : 1));
                        }
                    }
                }

                if (selectThis)
                    painter.setPen(palette().highlightedText().color());
                painter.drawText(collapseRect, Qt::AlignCenter, replacement);
                painter.restore();
            }
        }

        block = nextVisibleBlock;
        top = bottom;
        bottom = top + blockBoundingRect(block).height();
    }

    if (d->m_animator && d->m_animator->isRunning()) {
        QTextCursor cursor = textCursor();
        cursor.setPosition(d->m_animator->position());
        d->m_animator->draw(&painter, cursorRect(cursor).topLeft());
    }

    // draw the overlays, but only if we do not have a find scope, otherwise the
    // view becomes too noisy.
    if (d->m_findScopeStart.isNull()) {
        if (d->m_overlay->isVisible())
            d->m_overlay->paint(&painter, e->rect());

        if (d->m_snippetOverlay->isVisible())
            d->m_snippetOverlay->paint(&painter, e->rect());

        if (!d->m_refactorOverlay->isEmpty())
            d->m_refactorOverlay->paint(&painter, e->rect());
    }

    if (!d->m_searchResultOverlay->isEmpty()) {
        d->m_searchResultOverlay->paint(&painter, e->rect());
        d->m_searchResultOverlay->clear();
    }


    // draw the cursor last, on top of everything
    if (cursor_layout && !d->m_inBlockSelectionMode) {
        painter.setPen(cursor_pen);
        cursor_layout->drawCursor(&painter, cursor_offset, cursor_cpos, cursorWidth());
    }

    if (visibleCollapsedBlock.isValid()) {
        drawCollapsedBlockPopup(painter,
                                visibleCollapsedBlock,
                                visibleCollapsedBlockOffset,
                                er);
    }
}

void TextEditorWidget::paintBlock(QPainter *painter,
                                  const QTextBlock &block,
                                  const QPointF &offset,
                                  const QVector<QTextLayout::FormatRange> &selections,
                                  const QRect &clipRect) const
{
    block.layout()->draw(painter, offset, selections, clipRect);
}

int TextEditorWidget::visibleFoldedBlockNumber() const
{
    return d->visibleFoldedBlockNumber;
}

void TextEditorWidget::drawCollapsedBlockPopup(QPainter &painter,
                                             const QTextBlock &block,
                                             QPointF offset,
                                             const QRect &clip)
{
    int margin = block.document()->documentMargin();
    qreal maxWidth = 0;
    qreal blockHeight = 0;
    QTextBlock b = block;

    while (!b.isVisible()) {
        b.setVisible(true); // make sure block bounding rect works
        QRectF r = blockBoundingRect(b).translated(offset);

        QTextLayout *layout = b.layout();
        for (int i = layout->lineCount()-1; i >= 0; --i)
            maxWidth = qMax(maxWidth, layout->lineAt(i).naturalTextWidth() + 2*margin);

        blockHeight += r.height();

        b.setVisible(false); // restore previous state
        b.setLineCount(0); // restore 0 line count for invisible block
        b = b.next();
    }

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.translate(.5, .5);
    QBrush brush = palette().base();
    const QTextCharFormat &ifdefedOutFormat
            = textDocument()->fontSettings().toTextCharFormat(C_DISABLED_CODE);
    if (ifdefedOutFormat.hasProperty(QTextFormat::BackgroundBrush))
        brush = ifdefedOutFormat.background();
    painter.setBrush(brush);
    painter.drawRoundedRect(QRectF(offset.x(),
                                   offset.y(),
                                   maxWidth, blockHeight).adjusted(0, 0, 0, 0), 3, 3);
    painter.restore();

    QTextBlock end = b;
    b = block;
    while (b != end) {
        b.setVisible(true); // make sure block bounding rect works
        QRectF r = blockBoundingRect(b).translated(offset);
        QTextLayout *layout = b.layout();
        QVector<QTextLayout::FormatRange> selections;
        layout->draw(&painter, offset, selections, clip);

        b.setVisible(false); // restore previous state
        b.setLineCount(0); // restore 0 line count for invisible block
        offset.ry() += r.height();
        b = b.next();
    }
}

QWidget *TextEditorWidget::extraArea() const
{
    return d->m_extraArea;
}

int TextEditorWidget::extraAreaWidth(int *markWidthPtr) const
{
    TextDocumentLayout *documentLayout = qobject_cast<TextDocumentLayout*>(document()->documentLayout());
    if (!documentLayout)
        return 0;

    if (!d->m_marksVisible && documentLayout->hasMarks)
        d->m_marksVisible = true;

    int space = 0;
    const QFontMetrics fm(d->m_extraArea->fontMetrics());

    if (d->m_lineNumbersVisible) {
        QFont fnt = d->m_extraArea->font();
        // this works under the assumption that bold or italic
        // can only make a font wider
        const QTextCharFormat &currentLineNumberFormat
                = textDocument()->fontSettings().toTextCharFormat(C_CURRENT_LINE_NUMBER);
        fnt.setBold(currentLineNumberFormat.font().bold());
        fnt.setItalic(currentLineNumberFormat.font().italic());
        const QFontMetrics linefm(fnt);

        space += linefm.width(QLatin1Char('9')) * lineNumberDigits();
    }
    int markWidth = 0;

    if (d->m_marksVisible) {
        markWidth += documentLayout->maxMarkWidthFactor * fm.lineSpacing() + 2;

//     if (documentLayout->doubleMarkCount)
//         markWidth += fm.lineSpacing() / 3;
        space += markWidth;
    } else {
        space += 2;
    }

    if (markWidthPtr)
        *markWidthPtr = markWidth;

    space += 4;

    if (d->m_codeFoldingVisible)
        space += foldBoxWidth(fm);
    return space;
}

void TextEditorWidgetPrivate::slotUpdateExtraAreaWidth()
{
    if (q->isLeftToRight())
        q->setViewportMargins(q->extraAreaWidth(), 0, 0, 0);
    else
        q->setViewportMargins(0, 0, q->extraAreaWidth(), 0);
}

static void drawRectBox(QPainter *painter, const QRect &rect, bool start, bool end,
                        const QPalette &pal)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false);

    QRgb b = pal.base().color().rgb();
    QRgb h = pal.highlight().color().rgb();
    QColor c = StyleHelper::mergedColors(b,h, 50);

    QLinearGradient grad(rect.topLeft(), rect.topRight());
    grad.setColorAt(0, c.lighter(110));
    grad.setColorAt(1, c.lighter(130));
    QColor outline = c;

    painter->fillRect(rect, grad);
    painter->setPen(outline);
    if (start)
        painter->drawLine(rect.topLeft() + QPoint(1, 0), rect.topRight() -  QPoint(1, 0));
    if (end)
        painter->drawLine(rect.bottomLeft() + QPoint(1, 0), rect.bottomRight() -  QPoint(1, 0));

    painter->drawLine(rect.topRight() + QPoint(0, start ? 1 : 0), rect.bottomRight() - QPoint(0, end ? 1 : 0));
    painter->drawLine(rect.topLeft() + QPoint(0, start ? 1 : 0), rect.bottomLeft() - QPoint(0, end ? 1 : 0));

    painter->restore();
}

void TextEditorWidget::extraAreaPaintEvent(QPaintEvent *e)
{
    QTextDocument *doc = document();
    TextDocumentLayout *documentLayout = qobject_cast<TextDocumentLayout*>(doc->documentLayout());
    QTC_ASSERT(documentLayout, return);

    int selStart = textCursor().selectionStart();
    int selEnd = textCursor().selectionEnd();

    QPalette pal = d->m_extraArea->palette();
    pal.setCurrentColorGroup(QPalette::Active);
    QPainter painter(d->m_extraArea);
    const QFontMetrics fm(d->m_extraArea->font());
    int fmLineSpacing = fm.lineSpacing();

    int markWidth = 0;
    if (d->m_marksVisible)
        markWidth += fm.lineSpacing();

    const int collapseColumnWidth = d->m_codeFoldingVisible ? foldBoxWidth(fm): 0;
    const int extraAreaWidth = d->m_extraArea->width() - collapseColumnWidth;

    painter.fillRect(e->rect(), pal.color(QPalette::Background));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    qreal top = blockBoundingGeometry(block).translated(contentOffset()).top();
    qreal bottom = top;

    while (block.isValid() && top <= e->rect().bottom()) {

        top = bottom;
        const qreal height = blockBoundingRect(block).height();
        bottom = top + height;
        QTextBlock nextBlock = block.next();

        QTextBlock nextVisibleBlock = nextBlock;
        int nextVisibleBlockNumber = blockNumber + 1;

        if (!nextVisibleBlock.isVisible()) {
            // invisible blocks do have zero line count
            nextVisibleBlock = doc->findBlockByLineNumber(nextVisibleBlock.firstLineNumber());
            nextVisibleBlockNumber = nextVisibleBlock.blockNumber();
        }

        if (bottom < e->rect().top()) {
            block = nextVisibleBlock;
            blockNumber = nextVisibleBlockNumber;
            continue;
        }

        painter.setPen(pal.color(QPalette::Dark));

        if (d->m_lineNumbersVisible) {
            const QString &number = lineNumber(blockNumber);
            bool selected = (
                    (selStart < block.position() + block.length()

                    && selEnd > block.position())
                    || (selStart == selEnd && selStart == block.position())
                    );
            if (selected) {
                painter.save();
                QFont f = painter.font();
                const QTextCharFormat &currentLineNumberFormat
                        = textDocument()->fontSettings().toTextCharFormat(C_CURRENT_LINE_NUMBER);
                f.setBold(currentLineNumberFormat.font().bold());
                f.setItalic(currentLineNumberFormat.font().italic());
                painter.setFont(f);
                painter.setPen(currentLineNumberFormat.foreground().color());
                if (currentLineNumberFormat.background() != Qt::NoBrush)
                    painter.fillRect(QRect(0, top, extraAreaWidth, height), currentLineNumberFormat.background().color());
            }
            painter.drawText(QRectF(markWidth, top, extraAreaWidth - markWidth - 4, height), Qt::AlignRight, number);
            if (selected)
                painter.restore();
        }

        if (d->m_codeFoldingVisible || d->m_marksVisible) {
            painter.save();
            painter.setRenderHint(QPainter::Antialiasing, false);

            if (TextBlockUserData *userData = static_cast<TextBlockUserData*>(block.userData())) {
                if (d->m_marksVisible) {
                    int xoffset = 0;
                    TextMarks marks = userData->marks();
                    TextMarks::const_iterator it = marks.constBegin();
                    if (marks.size() > 3) {
                        // We want the 3 with the highest priority so iterate from the back
                        int count = 0;
                        it = marks.constEnd() - 1;
                        while (it != marks.constBegin()) {
                            if ((*it)->isVisible())
                                ++count;
                            if (count == 3)
                                break;
                            --it;
                        }
                    }
                    TextMarks::const_iterator end = marks.constEnd();
                    for ( ; it != end; ++it) {
                        TextMark *mark = *it;
                        if (!mark->isVisible())
                            continue;
                        const int height = fmLineSpacing - 1;
                        const int width = int(.5 + height * mark->widthFactor());
                        const QRect r(xoffset, top, width, height);
                        mark->paint(&painter, r);
                        xoffset += 2;
                    }
                }
            }

            if (d->m_codeFoldingVisible) {

                int extraAreaHighlightFoldBlockNumber = -1;
                int extraAreaHighlightFoldEndBlockNumber = -1;
                bool endIsVisible = false;
                if (!d->m_highlightBlocksInfo.isEmpty()) {
                    extraAreaHighlightFoldBlockNumber =  d->m_highlightBlocksInfo.open.last();
                    extraAreaHighlightFoldEndBlockNumber =  d->m_highlightBlocksInfo.close.first();
                    endIsVisible = doc->findBlockByNumber(extraAreaHighlightFoldEndBlockNumber).isVisible();

//                    QTextBlock before = doc->findBlockByNumber(extraAreaHighlightCollapseBlockNumber-1);
//                    if (TextBlockUserData::hasCollapseAfter(before)) {
//                        extraAreaHighlightCollapseBlockNumber--;
//                    }
                }

                TextBlockUserData *nextBlockUserData = TextDocumentLayout::testUserData(nextBlock);

                bool drawBox = nextBlockUserData
                               && TextDocumentLayout::foldingIndent(block) < nextBlockUserData->foldingIndent();



                bool active = blockNumber == extraAreaHighlightFoldBlockNumber;

                bool drawStart = active;
                bool drawEnd = blockNumber == extraAreaHighlightFoldEndBlockNumber || (drawStart && !endIsVisible);
                bool hovered = blockNumber >= extraAreaHighlightFoldBlockNumber
                               && blockNumber <= extraAreaHighlightFoldEndBlockNumber;

                int boxWidth = foldBoxWidth(fm);
                if (hovered) {
                    int itop = qRound(top);
                    int ibottom = qRound(bottom);
                    QRect box = QRect(extraAreaWidth + 1, itop, boxWidth - 2, ibottom - itop);
                    drawRectBox(&painter, box, drawStart, drawEnd, pal);
                }

                if (drawBox) {
                    bool expanded = nextBlock.isVisible();
                    int size = boxWidth/4;
                    QRect box(extraAreaWidth + size, top + size,
                              2 * (size) + 1, 2 * (size) + 1);
                    d->drawFoldingMarker(&painter, pal, box, expanded, active, hovered);
                }
            }

            painter.restore();
        }


        if (d->m_revisionsVisible && block.revision() != documentLayout->lastSaveRevision) {
            painter.save();
            painter.setRenderHint(QPainter::Antialiasing, false);
            if (block.revision() < 0)
                painter.setPen(QPen(Qt::darkGreen, 2));
            else
                painter.setPen(QPen(Qt::red, 2));
            painter.drawLine(extraAreaWidth - 1, top, extraAreaWidth - 1, bottom - 1);
            painter.restore();
        }

        block = nextVisibleBlock;
        blockNumber = nextVisibleBlockNumber;
    }
}

void TextEditorWidgetPrivate::drawFoldingMarker(QPainter *painter, const QPalette &pal,
                                       const QRect &rect,
                                       bool expanded,
                                       bool active,
                                       bool hovered) const
{
    QStyle *s = q->style();
    if (ManhattanStyle *ms = qobject_cast<ManhattanStyle*>(s))
        s = ms->baseStyle();

    if (!qstrcmp(s->metaObject()->className(), "OxygenStyle")) {
        painter->save();
        painter->setPen(Qt::NoPen);
        int size = rect.size().width();
        int sqsize = 2*(size/2);

        QColor textColor = pal.buttonText().color();
        QColor brushColor = textColor;

        textColor.setAlpha(100);
        brushColor.setAlpha(100);

        QPolygon a;
        if (expanded) {
            // down arrow
            a.setPoints(3, 0, sqsize/3,  sqsize/2, sqsize  - sqsize/3,  sqsize, sqsize/3);
        } else {
            // right arrow
            a.setPoints(3, sqsize - sqsize/3, sqsize/2,  sqsize/2 - sqsize/3, 0,  sqsize/2 - sqsize/3, sqsize);
            painter->setBrush(brushColor);
        }
        painter->translate(0.5, 0.5);
        painter->setRenderHint(QPainter::Antialiasing);
        painter->translate(rect.topLeft());
        painter->setPen(textColor);
        painter->setBrush(textColor);
        painter->drawPolygon(a);
        painter->restore();
    } else {
        QStyleOptionViewItemV2 opt;
        opt.rect = rect;
        opt.state = QStyle::State_Active | QStyle::State_Item | QStyle::State_Children;
        if (expanded)
            opt.state |= QStyle::State_Open;
        if (active)
            opt.state |= QStyle::State_MouseOver | QStyle::State_Enabled | QStyle::State_Selected;
        if (hovered)
            opt.palette.setBrush(QPalette::Window, pal.highlight());

         // QGtkStyle needs a small correction to draw the marker in the right place
        if (!qstrcmp(s->metaObject()->className(), "QGtkStyle"))
           opt.rect.translate(-2, 0);
        else if (!qstrcmp(s->metaObject()->className(), "QMacStyle"))
            opt.rect.translate(-1, 0);

        s->drawPrimitive(QStyle::PE_IndicatorBranch, &opt, painter, q);
    }
}

void TextEditorWidgetPrivate::slotUpdateRequest(const QRect &r, int dy)
{
    if (dy) {
        m_extraArea->scroll(0, dy);
    } else if (r.width() > 4) { // wider than cursor width, not just cursor blinking
        m_extraArea->update(0, r.y(), m_extraArea->width(), r.height());
        if (!m_searchExpr.isEmpty()) {
            const int m = m_searchResultOverlay->dropShadowWidth();
            q->viewport()->update(r.adjusted(-m, -m, m, m));
        }
    }

    if (r.contains(q->viewport()->rect()))
        slotUpdateExtraAreaWidth();
}

void TextEditorWidgetPrivate::saveCurrentCursorPositionForNavigation()
{
    m_lastCursorChangeWasInteresting = true;
    m_tempNavigationState = q->saveState();
}

void TextEditorWidgetPrivate::updateCurrentLineHighlight()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (m_highlightCurrentLine) {
        QTextEdit::ExtraSelection sel;
        sel.format.setBackground(q->textDocument()->fontSettings()
                                 .toTextCharFormat(C_CURRENT_LINE).background());
        sel.format.setProperty(QTextFormat::FullWidthSelection, true);
        sel.cursor = q->textCursor();
        sel.cursor.clearSelection();
        extraSelections.append(sel);
    }
    updateCurrentLineInScrollbar();

    q->setExtraSelections(TextEditorWidget::CurrentLineSelection, extraSelections);

    // the extra area shows information for the entire current block, not just the currentline.
    // This is why we must force a bigger update region.
    int cursorBlockNumber = q->textCursor().blockNumber();
    if (cursorBlockNumber != m_cursorBlockNumber) {
        QPointF offset = q->contentOffset();
        QTextBlock block = q->document()->findBlockByNumber(m_cursorBlockNumber);
        if (block.isValid())
            m_extraArea->update(q->blockBoundingGeometry(block).translated(offset).toAlignedRect());
        block = q->document()->findBlockByNumber(cursorBlockNumber);
        if (block.isValid() && block.isVisible())
            m_extraArea->update(q->blockBoundingGeometry(block).translated(offset).toAlignedRect());
        m_cursorBlockNumber = cursorBlockNumber;
    }
}

void TextEditorWidget::slotCursorPositionChanged()
{
#if 0
    qDebug() << "block" << textCursor().blockNumber()+1
            << "brace depth:" << BaseTextDocumentLayout::braceDepth(textCursor().block())
            << "indent:" << BaseTextDocumentLayout::userData(textCursor().block())->foldingIndent();
#endif
    if (!d->m_contentsChanged && d->m_lastCursorChangeWasInteresting) {
        if (EditorManager::currentEditor() && EditorManager::currentEditor()->widget() == this)
            EditorManager::addCurrentPositionToNavigationHistory(d->m_tempNavigationState);
        d->m_lastCursorChangeWasInteresting = false;
    } else if (d->m_contentsChanged) {
        d->saveCurrentCursorPositionForNavigation();
    }

    d->updateHighlights();
}

void TextEditorWidgetPrivate::updateHighlights()
{
    if (m_parenthesesMatchingEnabled && q->hasFocus()) {
        // Delay update when no matching is displayed yet, to avoid flicker
        if (q->extraSelections(TextEditorWidget::ParenthesesMatchingSelection).isEmpty()
            && m_animator == 0) {
            m_parenthesesMatchingTimer.start(50);
        } else {
            // when we uncheck "highlight matching parentheses"
            // we need clear current selection before viewport update
            // otherwise we get sticky highlighted parentheses
            if (!m_displaySettings.m_highlightMatchingParentheses)
                q->setExtraSelections(TextEditorWidget::ParenthesesMatchingSelection, QList<QTextEdit::ExtraSelection>());

            // use 0-timer, not direct call, to give the syntax highlighter a chance
            // to update the parentheses information
            m_parenthesesMatchingTimer.start(0);
        }
    }

    updateCurrentLineHighlight();

    if (m_displaySettings.m_highlightBlocks) {
        QTextCursor cursor = q->textCursor();
        extraAreaHighlightFoldedBlockNumber = cursor.blockNumber();
        m_highlightBlocksTimer.start(100);
    }
}

void TextEditorWidgetPrivate::updateCurrentLineInScrollbar()
{
    if (m_highlightCurrentLine && m_highlightScrollBar) {
        m_highlightScrollBar->removeHighlights(Constants::SCROLL_BAR_CURRENT_LINE);
        if (m_highlightScrollBar->maximum() > 0) {
            const QTextCursor &tc = q->textCursor();
            const int lineNumberInBlock =
                    tc.block().layout()->lineForTextPosition(tc.positionInBlock()).lineNumber();
            m_highlightScrollBar->addHighlight(Constants::SCROLL_BAR_CURRENT_LINE,
                                               q->textCursor().block().firstLineNumber() + lineNumberInBlock);
        }
    }
}

void TextEditorWidgetPrivate::slotUpdateBlockNotify(const QTextBlock &block)
{
    static bool blockRecursion = false;
    if (blockRecursion)
        return;
    blockRecursion = true;
    if (m_overlay->isVisible()) {
        /* an overlay might draw outside the block bounderies, force
           complete viewport update */
        q->viewport()->update();
    } else {
        if (block.previous().isValid() && block.userState() != block.previous().userState()) {
        /* The syntax highlighting state changes. This opens up for
           the possibility that the paragraph has braces that support
           code folding. In this case, do the save thing and also
           update the previous block, which might contain a fold
           box which now is invalid.*/
            emit q->requestBlockUpdate(block.previous());
        }
        if (!m_findScopeStart.isNull()) {
            if (block.position() < m_findScopeEnd.position()
                && block.position() + block.length() >= m_findScopeStart.position()) {
                QTextBlock b = block.document()->findBlock(m_findScopeStart.position());
                do {
                    emit q->requestBlockUpdate(b);
                    b = b.next();
                } while (b.isValid() && b.position() < m_findScopeEnd.position());
            }
        }
    }
    blockRecursion = false;
}

void TextEditorWidget::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == d->autoScrollTimer.timerId()) {
        const QPoint globalPos = QCursor::pos();
        const QPoint pos = d->m_extraArea->mapFromGlobal(globalPos);
        QRect visible = d->m_extraArea->rect();
        verticalScrollBar()->triggerAction( pos.y() < visible.center().y() ?
                                            QAbstractSlider::SliderSingleStepSub
                                            : QAbstractSlider::SliderSingleStepAdd);
        QMouseEvent ev(QEvent::MouseMove, pos, globalPos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        extraAreaMouseEvent(&ev);
        int delta = qMax(pos.y() - visible.top(), visible.bottom() - pos.y()) - visible.height();
        if (delta < 7)
            delta = 7;
        int timeout = 4900 / (delta * delta);
        d->autoScrollTimer.start(timeout, this);

    } else if (e->timerId() == d->foldedBlockTimer.timerId()) {
        d->visibleFoldedBlockNumber = d->suggestedVisibleFoldedBlockNumber;
        d->suggestedVisibleFoldedBlockNumber = -1;
        d->foldedBlockTimer.stop();
        viewport()->update();
    } else if (e->timerId() == d->m_cursorFlashTimer.timerId()) {
        d->m_cursorVisible = !d->m_cursorVisible;
        viewport()->update();
    }
    QPlainTextEdit::timerEvent(e);
}


void TextEditorWidgetPrivate::clearVisibleFoldedBlock()
{
    if (suggestedVisibleFoldedBlockNumber) {
        suggestedVisibleFoldedBlockNumber = -1;
        foldedBlockTimer.stop();
    }
    if (visibleFoldedBlockNumber >= 0) {
        visibleFoldedBlockNumber = -1;
        q->viewport()->update();
    }
}

void TextEditorWidget::mouseMoveEvent(QMouseEvent *e)
{
    d->requestUpdateLink(e);

    if (e->buttons() == Qt::NoButton) {
        const QTextBlock collapsedBlock = d->foldedBlockAt(e->pos());
        const int blockNumber = collapsedBlock.next().blockNumber();
        if (blockNumber < 0) {
            d->clearVisibleFoldedBlock();
        } else if (blockNumber != d->visibleFoldedBlockNumber) {
            d->suggestedVisibleFoldedBlockNumber = blockNumber;
            d->foldedBlockTimer.start(40, this);
        }

        const RefactorMarker refactorMarker = d->m_refactorOverlay->markerAt(e->pos());

        // Update the mouse cursor
        if ((collapsedBlock.isValid() || refactorMarker.isValid()) && !d->m_mouseOnFoldedMarker) {
            d->m_mouseOnFoldedMarker = true;
            viewport()->setCursor(Qt::PointingHandCursor);
        } else if (!collapsedBlock.isValid() && !refactorMarker.isValid() && d->m_mouseOnFoldedMarker) {
            d->m_mouseOnFoldedMarker = false;
            viewport()->setCursor(Qt::IBeamCursor);
        }
    } else {
        QPlainTextEdit::mouseMoveEvent(e);

        if (e->modifiers() & Qt::AltModifier) {
            if (!d->m_inBlockSelectionMode) {
                if (textCursor().hasSelection()) {
                    d->enableBlockSelection(textCursor());
                } else {
                    const QTextCursor &cursor = cursorForPosition(e->pos());
                    int column = d->m_document->tabSettings().columnAt(
                                cursor.block().text(), cursor.positionInBlock());
                    if (cursor.positionInBlock() == cursor.block().length()-1)
                        column += (e->pos().x() - cursorRect().center().x()) / QFontMetricsF(font()).width(QLatin1Char(' '));
                    int block = cursor.blockNumber();
                    if (block == blockCount() - 1)
                        block += (e->pos().y() - cursorRect().center().y()) / QFontMetricsF(font()).lineSpacing();
                    d->enableBlockSelection(block, column, block, column);
                }
            } else {
                const QTextCursor &cursor = textCursor();

                // get visual column
                int column = d->m_document->tabSettings().columnAt(
                            cursor.block().text(), cursor.positionInBlock());
                if (cursor.positionInBlock() == cursor.block().length()-1)
                    column += (e->pos().x() - cursorRect().center().x()) / QFontMetricsF(font()).width(QLatin1Char(' '));

                d->m_blockSelection.positionBlock = cursor.blockNumber();
                d->m_blockSelection.positionColumn = column;

                doSetTextCursor(d->m_blockSelection.selection(d->m_document.data()), true);
                viewport()->update();
            }
        } else if (d->m_inBlockSelectionMode) {
            d->disableBlockSelection();
        }
    }
    if (viewport()->cursor().shape() == Qt::BlankCursor)
        viewport()->setCursor(Qt::IBeamCursor);
}

static bool handleForwardBackwardMouseButtons(QMouseEvent *e)
{
    if (e->button() == Qt::XButton1) {
        EditorManager::goBackInNavigationHistory();
        return true;
    }
    if (e->button() == Qt::XButton2) {
        EditorManager::goForwardInNavigationHistory();
        return true;
    }

    return false;
}

void TextEditorWidget::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        if (e->modifiers() == Qt::AltModifier) {
            const QTextCursor &cursor = cursorForPosition(e->pos());
            int column = d->m_document->tabSettings().columnAt(
                        cursor.block().text(), cursor.positionInBlock());
            if (cursor.positionInBlock() == cursor.block().length()-1)
                column += (e->pos().x() - cursorRect(cursor).center().x()) / QFontMetricsF(font()).width(QLatin1Char(' '));
            int block = cursor.blockNumber();
            if (block == blockCount() - 1)
                block += (e->pos().y() - cursorRect(cursor).center().y()) / QFontMetricsF(font()).lineSpacing();
            if (d->m_inBlockSelectionMode) {
                d->m_blockSelection.positionBlock = block;
                d->m_blockSelection.positionColumn = column;

                doSetTextCursor(d->m_blockSelection.selection(d->m_document.data()), true);
                viewport()->update();
            } else {
                d->enableBlockSelection(block, column, block, column);
            }
        } else {
            if (d->m_inBlockSelectionMode)
                d->disableBlockSelection(false); // just in case, otherwise we might get strange drag and drop

            QTextBlock foldedBlock = d->foldedBlockAt(e->pos());
            if (foldedBlock.isValid()) {
                d->toggleBlockVisible(foldedBlock);
                viewport()->setCursor(Qt::IBeamCursor);
            }

            RefactorMarker refactorMarker = d->m_refactorOverlay->markerAt(e->pos());
            if (refactorMarker.isValid()) {
                onRefactorMarkerClicked(refactorMarker);
            } else {
                d->requestUpdateLink(e, true);

                if (d->m_currentLink.hasValidLinkText())
                    d->m_linkPressed = true;
            }
        }
    } else if (e->button() == Qt::RightButton) {
        int eventCursorPosition = cursorForPosition(e->pos()).position();
        if (eventCursorPosition < textCursor().selectionStart()
                || eventCursorPosition > textCursor().selectionEnd()) {
            setTextCursor(cursorForPosition(e->pos()));
        }
    }

    if (HostOsInfo::isLinuxHost() && handleForwardBackwardMouseButtons(e))
        return;

    QPlainTextEdit::mousePressEvent(e);
}

void TextEditorWidget::mouseReleaseEvent(QMouseEvent *e)
{
    if (mouseNavigationEnabled()
            && d->m_linkPressed
            && e->modifiers() & Qt::ControlModifier
            && !(e->modifiers() & Qt::ShiftModifier)
            && e->button() == Qt::LeftButton
            ) {

        EditorManager::addCurrentPositionToNavigationHistory();
        bool inNextSplit = ((e->modifiers() & Qt::AltModifier) && !alwaysOpenLinksInNextSplit())
                || (alwaysOpenLinksInNextSplit() && !(e->modifiers() & Qt::AltModifier));
        if (openLink(findLinkAt(cursorForPosition(e->pos())), inNextSplit)) {
            d->clearLink();
            return;
        }
    }

    if (!HostOsInfo::isLinuxHost() && handleForwardBackwardMouseButtons(e))
        return;

    QPlainTextEdit::mouseReleaseEvent(e);
}

void TextEditorWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        QTextCursor cursor = textCursor();
        const int position = cursor.position();
        if (TextBlockUserData::findPreviousOpenParenthesis(&cursor, false, true)) {
            if (position - cursor.position() == 1 && selectBlockUp())
                return;
        }
    }

    QPlainTextEdit::mouseDoubleClickEvent(e);
}

void TextEditorWidget::leaveEvent(QEvent *e)
{
    // Clear link emulation when the mouse leaves the editor
    d->clearLink();
    QPlainTextEdit::leaveEvent(e);
}

void TextEditorWidget::keyReleaseEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Control) {
        d->clearLink();
    } else if (e->key() == Qt::Key_Shift
             && d->m_behaviorSettings.m_constrainHoverTooltips
             && ToolTip::isVisible()) {
        ToolTip::hide();
    } else if (e->key() == Qt::Key_Alt
               && d->m_maybeFakeTooltipEvent) {
        d->m_maybeFakeTooltipEvent = false;
        d->processTooltipRequest(textCursor());
    }

    QPlainTextEdit::keyReleaseEvent(e);
}

void TextEditorWidget::dragEnterEvent(QDragEnterEvent *e)
{
    // If the drag event contains URLs, we don't want to insert them as text
    if (e->mimeData()->hasUrls()) {
        e->ignore();
        return;
    }

    QPlainTextEdit::dragEnterEvent(e);
}

static void appendMenuActionsFromContext(QMenu *menu, Id menuContextId)
{
    ActionContainer *mcontext = ActionManager::actionContainer(menuContextId);
    QMenu *contextMenu = mcontext->menu();

    foreach (QAction *action, contextMenu->actions())
        menu->addAction(action);
}

void TextEditorWidget::showDefaultContextMenu(QContextMenuEvent *e, Id menuContextId)
{
    QMenu menu;
    appendMenuActionsFromContext(&menu, menuContextId);
    appendStandardContextMenuActions(&menu);
    menu.exec(e->globalPos());
}

void TextEditorWidget::extraAreaLeaveEvent(QEvent *)
{
    // fake missing mouse move event from Qt
    QMouseEvent me(QEvent::MouseMove, QPoint(-1, -1), Qt::NoButton, 0, 0);
    extraAreaMouseEvent(&me);
}

void TextEditorWidget::extraAreaContextMenuEvent(QContextMenuEvent *e)
{
    if (d->m_marksVisible) {
        QTextCursor cursor = cursorForPosition(QPoint(0, e->pos().y()));
        QMenu * contextMenu = new QMenu(this);
        emit markContextMenuRequested(this, cursor.blockNumber() + 1, contextMenu);
        if (!contextMenu->isEmpty())
            contextMenu->exec(e->globalPos());
        delete contextMenu;
        e->accept();
    }
}

void TextEditorWidget::updateFoldingHighlight(const QPoint &pos)
{
    if (!d->m_codeFoldingVisible)
        return;

    QTextCursor cursor = cursorForPosition(QPoint(0, pos.y()));

    // Update which folder marker is highlighted
    const int highlightBlockNumber = d->extraAreaHighlightFoldedBlockNumber;
    d->extraAreaHighlightFoldedBlockNumber = -1;

    if (pos.x() > extraArea()->width() - foldBoxWidth(fontMetrics())) {
        d->extraAreaHighlightFoldedBlockNumber = cursor.blockNumber();
    } else if (d->m_displaySettings.m_highlightBlocks) {
        QTextCursor cursor = textCursor();
        d->extraAreaHighlightFoldedBlockNumber = cursor.blockNumber();
    }

    if (highlightBlockNumber != d->extraAreaHighlightFoldedBlockNumber)
        d->m_highlightBlocksTimer.start(d->m_highlightBlocksInfo.isEmpty() ? 120 : 0);
}

void TextEditorWidget::extraAreaMouseEvent(QMouseEvent *e)
{
    QTextCursor cursor = cursorForPosition(QPoint(0, e->pos().y()));

    int markWidth;
    extraAreaWidth(&markWidth);
    const bool inMarkArea = e->pos().x() <= markWidth && e->pos().x() >= 0;

    if (d->m_codeFoldingVisible
            && e->type() == QEvent::MouseMove && e->buttons() == 0) { // mouse tracking
        updateFoldingHighlight(e->pos());
    }

    // Set whether the mouse cursor is a hand or normal arrow
    if (e->type() == QEvent::MouseMove) {
        if (inMarkArea) {
            //Find line by cursor position
            int line = cursor.blockNumber() + 1;
            emit markTooltipRequested(this, mapToGlobal(e->pos()), line);
        }

        if (e->buttons() & Qt::LeftButton && !d->m_markDragStart.isNull()) {
            int dist = (e->pos() - d->m_markDragStart).manhattanLength();
            if (dist > QApplication::startDragDistance())
                d->m_markDragging = true;
        }

        if (d->m_markDragging)
            d->m_extraArea->setCursor(inMarkArea ? Qt::DragMoveCursor : Qt::ForbiddenCursor);
        else if (inMarkArea != (d->m_extraArea->cursor().shape() == Qt::PointingHandCursor))
            d->m_extraArea->setCursor(inMarkArea ? Qt::PointingHandCursor : Qt::ArrowCursor);
    }

    if (e->type() == QEvent::MouseButtonPress || e->type() == QEvent::MouseButtonDblClick) {
        if (e->button() == Qt::LeftButton) {
            int boxWidth = foldBoxWidth(fontMetrics());
            if (d->m_codeFoldingVisible && e->pos().x() > extraArea()->width() - boxWidth) {
                if (!cursor.block().next().isVisible()) {
                    d->toggleBlockVisible(cursor.block());
                    d->moveCursorVisible(false);
                } else if (d->foldBox().contains(e->pos())) {
                    cursor.setPosition(
                            document()->findBlockByNumber(d->m_highlightBlocksInfo.open.last()).position()
                            );
                    QTextBlock c = cursor.block();
                    d->toggleBlockVisible(c);
                    d->moveCursorVisible(false);
                }
            } else if (d->m_lineNumbersVisible && !inMarkArea) {
                QTextCursor selection = cursor;
                selection.setVisualNavigation(true);
                d->extraAreaSelectionAnchorBlockNumber = selection.blockNumber();
                selection.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                selection.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
                setTextCursor(selection);
            } else {
                d->extraAreaToggleMarkBlockNumber = cursor.blockNumber();
                d->m_markDragging = false;
                QTextBlock block = cursor.document()->findBlockByNumber(d->extraAreaToggleMarkBlockNumber);
                if (TextBlockUserData *data = static_cast<TextBlockUserData *>(block.userData())) {
                    TextMarks marks = data->marks();
                    for (int i = marks.size(); --i >= 0; ) {
                        TextMark *mark = marks.at(i);
                        if (mark->isDraggable()) {
                            d->m_markDragStart = e->pos();
                            break;
                        }
                    }
                }
            }
        }
    } else if (d->extraAreaSelectionAnchorBlockNumber >= 0) {
        QTextCursor selection = cursor;
        selection.setVisualNavigation(true);
        if (e->type() == QEvent::MouseMove) {
            QTextBlock anchorBlock = document()->findBlockByNumber(d->extraAreaSelectionAnchorBlockNumber);
            selection.setPosition(anchorBlock.position());
            if (cursor.blockNumber() < d->extraAreaSelectionAnchorBlockNumber) {
                selection.movePosition(QTextCursor::EndOfBlock);
                selection.movePosition(QTextCursor::Right);
            }
            selection.setPosition(cursor.block().position(), QTextCursor::KeepAnchor);
            if (cursor.blockNumber() >= d->extraAreaSelectionAnchorBlockNumber) {
                selection.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                selection.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
            }

            if (e->pos().y() >= 0 && e->pos().y() <= d->m_extraArea->height())
                d->autoScrollTimer.stop();
            else if (!d->autoScrollTimer.isActive())
                d->autoScrollTimer.start(100, this);

        } else {
            d->autoScrollTimer.stop();
            d->extraAreaSelectionAnchorBlockNumber = -1;
            return;
        }
        setTextCursor(selection);
    } else if (d->extraAreaToggleMarkBlockNumber >= 0 && d->m_marksVisible && d->m_requestMarkEnabled) {
        if (e->type() == QEvent::MouseButtonRelease && e->button() == Qt::LeftButton) {
            int n = d->extraAreaToggleMarkBlockNumber;
            d->extraAreaToggleMarkBlockNumber = -1;
            const bool sameLine = cursor.blockNumber() == n;
            const bool wasDragging = d->m_markDragging;
            d->m_markDragging = false;
            d->m_markDragStart = QPoint();
            QTextBlock block = cursor.document()->findBlockByNumber(n);
            if (TextBlockUserData *data = static_cast<TextBlockUserData *>(block.userData())) {
                TextMarks marks = data->marks();
                for (int i = marks.size(); --i >= 0; ) {
                    TextMark *mark = marks.at(i);
                    if (sameLine) {
                        if (mark->isClickable()) {
                            mark->clicked();
                            return;
                        }
                    } else {
                        if (wasDragging && mark->isDraggable()) {
                            if (inMarkArea) {
                                mark->dragToLine(cursor.blockNumber() + 1);
                                d->m_extraArea->setCursor(Qt::PointingHandCursor);
                            } else {
                                d->m_extraArea->setCursor(Qt::ArrowCursor);
                            }
                            return;
                        }
                    }
                }
            }
            int line = n + 1;
            TextMarkRequestKind kind;
            if (QApplication::keyboardModifiers() & Qt::ShiftModifier)
                kind = BookmarkRequest;
            else
                kind = BreakpointRequest;

            emit markRequested(this, line, kind);
        }
    }
}

void TextEditorWidget::ensureCursorVisible()
{
    QTextBlock block = textCursor().block();
    if (!block.isVisible()) {
        TextDocumentLayout *documentLayout = qobject_cast<TextDocumentLayout*>(document()->documentLayout());
        QTC_ASSERT(documentLayout, return);

        // Open all parent folds of current line.
        int indent = TextDocumentLayout::foldingIndent(block);
        block = block.previous();
        while (block.isValid()) {
            const int indent2 = TextDocumentLayout::foldingIndent(block);
            if (TextDocumentLayout::canFold(block) && indent2 < indent) {
                TextDocumentLayout::doFoldOrUnfold(block, /* unfold = */ true);
                if (block.isVisible())
                    break;
                indent = indent2;
            }
            block = block.previous();
        }

        documentLayout->requestUpdate();
        documentLayout->emitDocumentSizeChanged();
    }
    QPlainTextEdit::ensureCursorVisible();
}

void TextEditorWidgetPrivate::toggleBlockVisible(const QTextBlock &block)
{
    auto documentLayout = qobject_cast<TextDocumentLayout*>(q->document()->documentLayout());
    QTC_ASSERT(documentLayout, return);

    TextDocumentLayout::doFoldOrUnfold(block, TextDocumentLayout::isFolded(block));
    documentLayout->requestUpdate();
    documentLayout->emitDocumentSizeChanged();
}

void TextEditorWidget::setLanguageSettingsId(Id settingsId)
{
    d->m_tabSettingsId = settingsId;
}

Id TextEditorWidget::languageSettingsId() const
{
    return d->m_tabSettingsId;
}

void TextEditorWidget::setCodeStyle(ICodeStylePreferences *preferences)
{
    textDocument()->indenter()->setCodeStylePreferences(preferences);
    if (d->m_codeStylePreferences) {
        disconnect(d->m_codeStylePreferences, SIGNAL(currentTabSettingsChanged(TextEditor::TabSettings)),
                d->m_document.data(), SLOT(setTabSettings(TextEditor::TabSettings)));
        disconnect(d->m_codeStylePreferences, SIGNAL(currentValueChanged(QVariant)),
                this, SLOT(slotCodeStyleSettingsChanged(QVariant)));
    }
    d->m_codeStylePreferences = preferences;
    if (d->m_codeStylePreferences) {
        connect(d->m_codeStylePreferences, SIGNAL(currentTabSettingsChanged(TextEditor::TabSettings)),
                d->m_document.data(), SLOT(setTabSettings(TextEditor::TabSettings)));
        connect(d->m_codeStylePreferences, SIGNAL(currentValueChanged(QVariant)),
                this, SLOT(slotCodeStyleSettingsChanged(QVariant)));
        d->m_document->setTabSettings(d->m_codeStylePreferences->currentTabSettings());
        slotCodeStyleSettingsChanged(d->m_codeStylePreferences->currentValue());
    }
}

void TextEditorWidget::slotCodeStyleSettingsChanged(const QVariant &)
{

}

const DisplaySettings &TextEditorWidget::displaySettings() const
{
    return d->m_displaySettings;
}

const MarginSettings &TextEditorWidget::marginSettings() const
{
    return d->m_marginSettings;
}

void TextEditorWidgetPrivate::handleHomeKey(bool anchor)
{
    QTextCursor cursor = q->textCursor();
    QTextCursor::MoveMode mode = QTextCursor::MoveAnchor;

    if (anchor)
        mode = QTextCursor::KeepAnchor;

    const int initpos = cursor.position();
    int pos = cursor.block().position();
    QChar character = q->document()->characterAt(pos);
    const QLatin1Char tab = QLatin1Char('\t');

    while (character == tab || character.category() == QChar::Separator_Space) {
        ++pos;
        if (pos == initpos)
            break;
        character = q->document()->characterAt(pos);
    }

    // Go to the start of the block when we're already at the start of the text
    if (pos == initpos)
        pos = cursor.block().position();

    cursor.setPosition(pos, mode);
    q->setTextCursor(cursor);
}

void TextEditorWidgetPrivate::handleBackspaceKey()
{
    QTextCursor cursor = q->textCursor();
    QTC_ASSERT(!cursor.hasSelection(), return);

    const int pos = cursor.position();
    if (!pos)
        return;

    bool cursorWithinSnippet = false;
    if (m_snippetOverlay->isVisible()) {
        QTextCursor snippetCursor = cursor;
        snippetCursor.movePosition(QTextCursor::Left);
        cursorWithinSnippet = snippetCheckCursor(snippetCursor);
    }

    const TabSettings &tabSettings = m_document->tabSettings();
    const TypingSettings &typingSettings = m_document->typingSettings();

    if (typingSettings.m_autoIndent && m_autoCompleter->autoBackspace(cursor))
        return;

    bool handled = false;
    if (typingSettings.m_smartBackspaceBehavior == TypingSettings::BackspaceNeverIndents) {
        if (cursorWithinSnippet)
            cursor.beginEditBlock();
        cursor.deletePreviousChar();
        handled = true;
    } else if (typingSettings.m_smartBackspaceBehavior == TypingSettings::BackspaceFollowsPreviousIndents) {
        QTextBlock currentBlock = cursor.block();
        int positionInBlock = pos - currentBlock.position();
        const QString blockText = currentBlock.text();
        if (cursor.atBlockStart() || tabSettings.firstNonSpace(blockText) < positionInBlock) {
            if (cursorWithinSnippet)
                cursor.beginEditBlock();
            cursor.deletePreviousChar();
            handled = true;
        } else {
            if (cursorWithinSnippet) {
                m_snippetOverlay->mangle();
                m_snippetOverlay->clear();
                cursorWithinSnippet = false;
            }
            int previousIndent = 0;
            const int indent = tabSettings.columnAt(blockText, positionInBlock);
            for (QTextBlock previousNonEmptyBlock = currentBlock.previous();
                 previousNonEmptyBlock.isValid();
                 previousNonEmptyBlock = previousNonEmptyBlock.previous()) {
                QString previousNonEmptyBlockText = previousNonEmptyBlock.text();
                if (previousNonEmptyBlockText.trimmed().isEmpty())
                    continue;
                previousIndent =
                        tabSettings.columnAt(previousNonEmptyBlockText,
                                             tabSettings.firstNonSpace(previousNonEmptyBlockText));
                if (previousIndent < indent) {
                    cursor.beginEditBlock();
                    cursor.setPosition(currentBlock.position(), QTextCursor::KeepAnchor);
                    cursor.insertText(tabSettings.indentationString(previousNonEmptyBlockText));
                    cursor.endEditBlock();
                    handled = true;
                    break;
                }
            }
        }
    } else if (typingSettings.m_smartBackspaceBehavior == TypingSettings::BackspaceUnindents) {
        const QChar c = q->document()->characterAt(pos - 1);
        if (!(c == QLatin1Char(' ') || c == QLatin1Char('\t'))) {
            if (cursorWithinSnippet)
                cursor.beginEditBlock();
            cursor.deletePreviousChar();
        } else {
            if (cursorWithinSnippet) {
                m_snippetOverlay->mangle();
                m_snippetOverlay->clear();
                cursorWithinSnippet = false;
            }
            q->unindent();
        }
        handled = true;
    }

    if (!handled) {
        if (cursorWithinSnippet)
            cursor.beginEditBlock();
        cursor.deletePreviousChar();
    }

    if (cursorWithinSnippet) {
        cursor.endEditBlock();
        m_snippetOverlay->updateEquivalentSelections(cursor);
    }

    q->setTextCursor(cursor);
}

void TextEditorWidget::wheelEvent(QWheelEvent *e)
{
    d->clearVisibleFoldedBlock();
    if (e->modifiers() & Qt::ControlModifier) {
        if (!scrollWheelZoomingEnabled()) {
            // When the setting is disabled globally,
            // we have to skip calling QPlainTextEdit::wheelEvent()
            // that changes zoom in it.
            return;
        }

        const int delta = e->delta();
        if (delta < 0)
            zoomOut();
        else if (delta > 0)
            zoomIn();
        return;
    }
    QPlainTextEdit::wheelEvent(e);
}

void TextEditorWidget::zoomIn()
{
    d->clearVisibleFoldedBlock();
    emit requestFontZoom(10);
}

void TextEditorWidget::zoomOut()
{
    d->clearVisibleFoldedBlock();
    emit requestFontZoom(-10);
}

void TextEditorWidget::zoomReset()
{
    emit requestZoomReset();
}

TextEditorWidget::Link TextEditorWidget::findLinkAt(const QTextCursor &, bool, bool)
{
    return Link();
}

bool TextEditorWidget::openLink(const Link &link, bool inNextSplit)
{
    if (!link.hasValidTarget())
        return false;

    if (!inNextSplit && textDocument()->filePath().toString() == link.targetFileName) {
        EditorManager::addCurrentPositionToNavigationHistory();
        gotoLine(link.targetLine, link.targetColumn);
        setFocus();
        return true;
    }
    EditorManager::OpenEditorFlags flags;
    if (inNextSplit)
        flags |= EditorManager::OpenInOtherSplit;

    return EditorManager::openEditorAt(link.targetFileName, link.targetLine, link.targetColumn,
                                       Id(), flags);
}

void TextEditorWidgetPrivate::requestUpdateLink(QMouseEvent *e, bool immediate)
{
    if (!q->mouseNavigationEnabled())
        return;
    if (e->modifiers() & Qt::ControlModifier) {
        // Link emulation behaviour for 'go to definition'
        const QTextCursor cursor = q->cursorForPosition(e->pos());

        // Avoid updating the link we already found
        if (cursor.position() >= m_currentLink.linkTextStart
                && cursor.position() <= m_currentLink.linkTextEnd)
            return;

        // Check that the mouse was actually on the text somewhere
        bool onText = q->cursorRect(cursor).right() >= e->x();
        if (!onText) {
            QTextCursor nextPos = cursor;
            nextPos.movePosition(QTextCursor::Right);
            onText = q->cursorRect(nextPos).right() >= e->x();
        }

        if (onText) {
            m_pendingLinkUpdate = cursor;

            if (immediate)
                updateLink();
            else
                QTimer::singleShot(0, this, &TextEditorWidgetPrivate::updateLink);

            return;
        }
    }

    clearLink();
}

void TextEditorWidgetPrivate::updateLink()
{
    if (m_pendingLinkUpdate.isNull())
        return;
    if (m_pendingLinkUpdate == m_lastLinkUpdate)
        return;

    m_lastLinkUpdate = m_pendingLinkUpdate;
    const TextEditorWidget::Link link = q->findLinkAt(m_pendingLinkUpdate, false);
    if (link.hasValidLinkText())
        showLink(link);
    else
        clearLink();
}

void TextEditorWidgetPrivate::showLink(const TextEditorWidget::Link &link)
{
    if (m_currentLink == link)
        return;

    QTextEdit::ExtraSelection sel;
    sel.cursor = q->textCursor();
    sel.cursor.setPosition(link.linkTextStart);
    sel.cursor.setPosition(link.linkTextEnd, QTextCursor::KeepAnchor);
    sel.format = q->textDocument()->fontSettings().toTextCharFormat(C_LINK);
    sel.format.setFontUnderline(true);
    q->setExtraSelections(TextEditorWidget::OtherSelection, QList<QTextEdit::ExtraSelection>() << sel);
    q->viewport()->setCursor(Qt::PointingHandCursor);
    m_currentLink = link;
    m_linkPressed = false;
}

void TextEditorWidgetPrivate::clearLink()
{
    m_pendingLinkUpdate = QTextCursor();
    m_lastLinkUpdate = QTextCursor();
    if (!m_currentLink.hasValidLinkText())
        return;

    q->setExtraSelections(TextEditorWidget::OtherSelection, QList<QTextEdit::ExtraSelection>());
    q->viewport()->setCursor(Qt::IBeamCursor);
    m_currentLink = TextEditorWidget::Link();
    m_linkPressed = false;
}

void TextEditorWidgetPrivate::highlightSearchResultsSlot(const QString &txt, FindFlags findFlags)
{
    if (m_searchExpr.pattern() == txt)
        return;
    m_searchExpr.setPattern(txt);
    m_searchExpr.setPatternSyntax((findFlags & FindRegularExpression) ?
                                     QRegExp::RegExp : QRegExp::FixedString);
    m_searchExpr.setCaseSensitivity((findFlags & FindCaseSensitively) ?
                                       Qt::CaseSensitive : Qt::CaseInsensitive);
    m_findFlags = findFlags;

    m_delayedUpdateTimer.start(50);

    if (m_highlightScrollBar)
        m_scrollBarUpdateTimer.start(50);
}

void TextEditorWidgetPrivate::searchResultsReady(int beginIndex, int endIndex)
{
    QVector<SearchResult> results;
    for (int index = beginIndex; index < endIndex; ++index) {
        foreach (Utils::FileSearchResult result, m_searchWatcher->resultAt(index)) {
            const QTextBlock &block = q->document()->findBlockByNumber(result.lineNumber - 1);
            const int matchStart = block.position() + result.matchStart;
            if (!q->inFindScope(matchStart, matchStart + result.matchLength))
                continue;
            results << SearchResult{matchStart, result.matchLength};
        }
    }
    m_searchResults << results;
    addSearchResultsToScrollBar(results);
}

void TextEditorWidgetPrivate::searchFinished()
{
    delete m_searchWatcher;
    m_searchWatcher = 0;
}

void TextEditorWidgetPrivate::adjustScrollBarRanges()
{
    if (!m_highlightScrollBar)
        return;
    const float lineSpacing = QFontMetricsF(q->font()).lineSpacing();
    if (lineSpacing == 0)
        return;

    const float offset = q->contentOffset().y();
    m_highlightScrollBar->setVisibleRange((q->viewport()->rect().height() - offset) / lineSpacing);
    m_highlightScrollBar->setRangeOffset(offset / lineSpacing);
}

void TextEditorWidgetPrivate::highlightSearchResultsInScrollBar()
{
    if (!m_highlightScrollBar)
        return;
    m_highlightScrollBar->removeHighlights(Constants::SCROLL_BAR_SEARCH_RESULT);
    m_searchResults.clear();

    if (m_searchWatcher) {
        m_searchWatcher->disconnect();
        m_searchWatcher->cancel();
        m_searchWatcher->deleteLater();
        m_searchWatcher = 0;
    }

    const QString &txt = m_searchExpr.pattern();
    if (txt.isEmpty())
        return;

    adjustScrollBarRanges();

    m_searchWatcher = new QFutureWatcher<FileSearchResultList>();
    connect(m_searchWatcher, &QFutureWatcher<Utils::FileSearchResultList>::resultsReadyAt,
            this, &TextEditorWidgetPrivate::searchResultsReady);
    connect(m_searchWatcher, &QFutureWatcher<Utils::FileSearchResultList>::finished,
            this, &TextEditorWidgetPrivate::searchFinished);
    m_searchWatcher->setPendingResultsLimit(10);

    const QTextDocument::FindFlags findFlags = textDocumentFlagsForFindFlags(m_findFlags);

    const QString &fileName = m_document->filePath().toString();
    FileListIterator *it =
            new FileListIterator( { fileName } , { const_cast<QTextCodec *>(m_document->codec()) } );
    QMap<QString, QString> fileToContentsMap;
    fileToContentsMap[fileName] = m_document->plainText();

    if (m_findFlags & FindRegularExpression)
        m_searchWatcher->setFuture(findInFilesRegExp(txt, it, findFlags, fileToContentsMap));
    else
        m_searchWatcher->setFuture(findInFiles(txt, it, findFlags, fileToContentsMap));
}

void TextEditorWidgetPrivate::scheduleUpdateHighlightScrollBar()
{
    if (m_scrollBarUpdateScheduled)
        return;

    m_scrollBarUpdateScheduled = true;
    QTimer::singleShot(0, this, &TextEditorWidgetPrivate::updateHighlightScrollBarNow);
}

HighlightScrollBar::Priority textMarkPrioToScrollBarPrio(const TextMark::Priority &prio)
{
    switch (prio) {
    case TextMark::LowPriority:
        return HighlightScrollBar::LowPriority;
    case TextMark::NormalPriority:
        return HighlightScrollBar::NormalPriority;
    case TextMark::HighPriority:
        return HighlightScrollBar::HighPriority;
    default:
        return HighlightScrollBar::NormalPriority;
    }
}

void TextEditorWidgetPrivate::addSearchResultsToScrollBar(QVector<SearchResult> results)
{
    QSet<int> searchResults;
    foreach (SearchResult result, results) {
        const QTextBlock &block = q->document()->findBlock(result.start);
        if (block.isValid() && block.isVisible()) {
            const int firstLine = block.layout()->lineForTextPosition(result.start - block.position()).lineNumber();
            const int lastLine = block.layout()->lineForTextPosition(result.start - block.position() + result.length).lineNumber();
            for (int line = firstLine; line <= lastLine; ++line)
                searchResults << block.firstLineNumber() + line;
        }
    }
    if (m_highlightScrollBar)
        m_highlightScrollBar->addHighlights(Constants::SCROLL_BAR_SEARCH_RESULT, searchResults);
}

void TextEditorWidgetPrivate::updateHighlightScrollBarNow()
{
    typedef QSet<int> IntSet;

    m_scrollBarUpdateScheduled = false;
    if (!m_highlightScrollBar)
        return;

    m_highlightScrollBar->removeAllHighlights();

    updateCurrentLineInScrollbar();

    // update search results
    addSearchResultsToScrollBar(m_searchResults);

    // update text marks
    QHash<Id, IntSet> marks;
    foreach (TextMark *mark, m_document->marks()) {
        Id category = mark->category();
        if (!mark->isVisible() || !TextMark::categoryHasColor(category))
            continue;
        m_highlightScrollBar->setPriority(category, textMarkPrioToScrollBarPrio(mark->priority()));
        const QTextBlock &block = q->document()->findBlockByNumber(mark->lineNumber() - 1);
        if (block.isVisible())
            marks[category] << block.firstLineNumber();
    }
    QHashIterator<Id, IntSet> it(marks);
    while (it.hasNext()) {
        it.next();
        m_highlightScrollBar->setColor(it.key(), TextMark::categoryColor(it.key()));
        m_highlightScrollBar->addHighlights(it.key(), it.value());
    }
}

int TextEditorWidget::verticalBlockSelectionFirstColumn() const
{
    return d->m_inBlockSelectionMode ? d->m_blockSelection.firstVisualColumn() : -1;
}

int TextEditorWidget::verticalBlockSelectionLastColumn() const
{
    return d->m_inBlockSelectionMode ? d->m_blockSelection.lastVisualColumn() : -1;
}

QRegion TextEditorWidget::translatedLineRegion(int lineStart, int lineEnd) const
{
    QRegion region;
    for (int i = lineStart ; i <= lineEnd; i++) {
        QTextBlock block = document()->findBlockByNumber(i);
        QPoint topLeft = blockBoundingGeometry(block).translated(contentOffset()).topLeft().toPoint();

        if (block.isValid()) {
            QTextLayout *layout = block.layout();

            for (int i = 0; i < layout->lineCount();i++) {
                QTextLine line = layout->lineAt(i);
                region += line.naturalTextRect().translated(topLeft).toRect();
            }
        }
    }
    return region;
}

void TextEditorWidgetPrivate::setFindScope(const QTextCursor &start, const QTextCursor &end,
                                  int verticalBlockSelectionFirstColumn,
                                  int verticalBlockSelectionLastColumn)
{
    if (start != m_findScopeStart
            || end != m_findScopeEnd
            || verticalBlockSelectionFirstColumn != m_findScopeVerticalBlockSelectionFirstColumn
            || verticalBlockSelectionLastColumn != m_findScopeVerticalBlockSelectionLastColumn) {
        m_findScopeStart = start;
        m_findScopeEnd = end;
        m_findScopeVerticalBlockSelectionFirstColumn = verticalBlockSelectionFirstColumn;
        m_findScopeVerticalBlockSelectionLastColumn = verticalBlockSelectionLastColumn;
        q->viewport()->update();
        highlightSearchResultsInScrollBar();
    }
}

void TextEditorWidgetPrivate::_q_animateUpdate(int position, QPointF lastPos, QRectF rect)
{
    QTextCursor cursor = q->textCursor();
    cursor.setPosition(position);
    q->viewport()->update(QRectF(q->cursorRect(cursor).topLeft() + rect.topLeft(), rect.size()).toAlignedRect());
    if (!lastPos.isNull())
        q->viewport()->update(QRectF(lastPos + rect.topLeft(), rect.size()).toAlignedRect());
}


TextEditorAnimator::TextEditorAnimator(QObject *parent)
    : QObject(parent), m_timeline(256)
{
    m_value = 0;
    m_timeline.setCurveShape(QTimeLine::SineCurve);
    connect(&m_timeline, &QTimeLine::valueChanged, this, &TextEditorAnimator::step);
    connect(&m_timeline, &QTimeLine::finished, this, &QObject::deleteLater);
    m_timeline.start();
}

void TextEditorAnimator::setData(const QFont &f, const QPalette &pal, const QString &text)
{
    m_font = f;
    m_palette = pal;
    m_text = text;
    QFontMetrics fm(m_font);
    m_size = QSizeF(fm.width(m_text), fm.height());
}

void TextEditorAnimator::draw(QPainter *p, const QPointF &pos)
{
    m_lastDrawPos = pos;
    p->setPen(m_palette.text().color());
    QFont f = m_font;
    f.setPointSizeF(f.pointSizeF() * (1.0 + m_value/2));
    QFontMetrics fm(f);
    int width = fm.width(m_text);
    QRectF r((m_size.width()-width)/2, (m_size.height() - fm.height())/2, width, fm.height());
    r.translate(pos);
    p->fillRect(r, m_palette.base());
    p->setFont(f);
    p->drawText(r, m_text);
}

bool TextEditorAnimator::isRunning() const
{
    return m_timeline.state() == QTimeLine::Running;
}

QRectF TextEditorAnimator::rect() const
{
    QFont f = m_font;
    f.setPointSizeF(f.pointSizeF() * (1.0 + m_value/2));
    QFontMetrics fm(f);
    int width = fm.width(m_text);
    return QRectF((m_size.width()-width)/2, (m_size.height() - fm.height())/2, width, fm.height());
}

void TextEditorAnimator::step(qreal v)
{
    QRectF before = rect();
    m_value = v;
    QRectF after = rect();
    emit updateRequest(m_position, m_lastDrawPos, before.united(after));
}

void TextEditorAnimator::finish()
{
    m_timeline.stop();
    step(0);
    deleteLater();
}

void TextEditorWidgetPrivate::_q_matchParentheses()
{
    if (q->isReadOnly()
        || !(m_displaySettings.m_highlightMatchingParentheses
             || m_displaySettings.m_animateMatchingParentheses))
        return;

    QTextCursor backwardMatch = q->textCursor();
    QTextCursor forwardMatch = q->textCursor();
    if (q->overwriteMode())
        backwardMatch.movePosition(QTextCursor::Right);
    const TextBlockUserData::MatchType backwardMatchType = TextBlockUserData::matchCursorBackward(&backwardMatch);
    const TextBlockUserData::MatchType forwardMatchType = TextBlockUserData::matchCursorForward(&forwardMatch);

    QList<QTextEdit::ExtraSelection> extraSelections;

    if (backwardMatchType == TextBlockUserData::NoMatch && forwardMatchType == TextBlockUserData::NoMatch) {
        q->setExtraSelections(TextEditorWidget::ParenthesesMatchingSelection, extraSelections); // clear
        return;
    }

    const QTextCharFormat &matchFormat
            = q->textDocument()->fontSettings().toTextCharFormat(C_PARENTHESES);
    const QTextCharFormat &mismatchFormat
            = q->textDocument()->fontSettings().toTextCharFormat(C_PARENTHESES_MISMATCH);
    int animatePosition = -1;
    if (backwardMatch.hasSelection()) {
        QTextEdit::ExtraSelection sel;
        if (backwardMatchType == TextBlockUserData::Mismatch) {
            sel.cursor = backwardMatch;
            sel.format = mismatchFormat;
            extraSelections.append(sel);
        } else {

            sel.cursor = backwardMatch;
            sel.format = matchFormat;

            sel.cursor.setPosition(backwardMatch.selectionStart());
            sel.cursor.setPosition(sel.cursor.position() + 1, QTextCursor::KeepAnchor);
            extraSelections.append(sel);

            if (m_displaySettings.m_animateMatchingParentheses && sel.cursor.block().isVisible())
                animatePosition = backwardMatch.selectionStart();

            sel.cursor.setPosition(backwardMatch.selectionEnd());
            sel.cursor.setPosition(sel.cursor.position() - 1, QTextCursor::KeepAnchor);
            extraSelections.append(sel);
        }
    }

    if (forwardMatch.hasSelection()) {
        QTextEdit::ExtraSelection sel;
        if (forwardMatchType == TextBlockUserData::Mismatch) {
            sel.cursor = forwardMatch;
            sel.format = mismatchFormat;
            extraSelections.append(sel);
        } else {

            sel.cursor = forwardMatch;
            sel.format = matchFormat;

            sel.cursor.setPosition(forwardMatch.selectionStart());
            sel.cursor.setPosition(sel.cursor.position() + 1, QTextCursor::KeepAnchor);
            extraSelections.append(sel);

            sel.cursor.setPosition(forwardMatch.selectionEnd());
            sel.cursor.setPosition(sel.cursor.position() - 1, QTextCursor::KeepAnchor);
            extraSelections.append(sel);

            if (m_displaySettings.m_animateMatchingParentheses && sel.cursor.block().isVisible())
                animatePosition = forwardMatch.selectionEnd() - 1;
        }
    }


    if (animatePosition >= 0) {
        foreach (const QTextEdit::ExtraSelection &sel, q->extraSelections(TextEditorWidget::ParenthesesMatchingSelection)) {
            if (sel.cursor.selectionStart() == animatePosition
                || sel.cursor.selectionEnd() - 1 == animatePosition) {
                animatePosition = -1;
                break;
            }
        }
    }

    if (animatePosition >= 0) {
        if (m_animator)
            m_animator->finish();  // one animation is enough
        m_animator = new TextEditorAnimator(this);
        m_animator->setPosition(animatePosition);
        QPalette pal;
        pal.setBrush(QPalette::Text, matchFormat.foreground());
        pal.setBrush(QPalette::Base, matchFormat.background());
        m_animator->setData(q->font(), pal, q->document()->characterAt(m_animator->position()));
        connect(m_animator.data(), &TextEditorAnimator::updateRequest,
                this, &TextEditorWidgetPrivate::_q_animateUpdate);
    }
    if (m_displaySettings.m_highlightMatchingParentheses)
        q->setExtraSelections(TextEditorWidget::ParenthesesMatchingSelection, extraSelections);
}

void TextEditorWidgetPrivate::_q_highlightBlocks()
{
    TextEditorPrivateHighlightBlocks highlightBlocksInfo;

    QTextBlock block;
    if (extraAreaHighlightFoldedBlockNumber >= 0) {
        block = q->document()->findBlockByNumber(extraAreaHighlightFoldedBlockNumber);
        if (block.isValid()
            && block.next().isValid()
            && TextDocumentLayout::foldingIndent(block.next())
            > TextDocumentLayout::foldingIndent(block))
            block = block.next();
    }

    QTextBlock closeBlock = block;
    while (block.isValid()) {
        int foldingIndent = TextDocumentLayout::foldingIndent(block);

        while (block.previous().isValid() && TextDocumentLayout::foldingIndent(block) >= foldingIndent)
            block = block.previous();
        int nextIndent = TextDocumentLayout::foldingIndent(block);
        if (nextIndent == foldingIndent)
            break;
        highlightBlocksInfo.open.prepend(block.blockNumber());
        while (closeBlock.next().isValid()
            && TextDocumentLayout::foldingIndent(closeBlock.next()) >= foldingIndent )
            closeBlock = closeBlock.next();
        highlightBlocksInfo.close.append(closeBlock.blockNumber());
        int indent = qMin(visualIndent(block), visualIndent(closeBlock));
        highlightBlocksInfo.visualIndent.prepend(indent);
    }

#if 0
    if (block.isValid()) {
        QTextCursor cursor(block);
        if (extraAreaHighlightCollapseColumn >= 0)
            cursor.setPosition(cursor.position() + qMin(extraAreaHighlightCollapseColumn,
                                                        block.length()-1));
        QTextCursor closeCursor;
        bool firstRun = true;
        while (TextBlockUserData::findPreviousBlockOpenParenthesis(&cursor, firstRun)) {
            firstRun = false;
            highlightBlocksInfo.open.prepend(cursor.blockNumber());
            int visualIndent = visualIndent(cursor.block());
            if (closeCursor.isNull())
                closeCursor = cursor;
            if (TextBlockUserData::findNextBlockClosingParenthesis(&closeCursor)) {
                highlightBlocksInfo.close.append(closeCursor.blockNumber());
                visualIndent = qMin(visualIndent, visualIndent(closeCursor.block()));
            }
            highlightBlocksInfo.visualIndent.prepend(visualIndent);
        }
    }
#endif
    if (m_highlightBlocksInfo != highlightBlocksInfo) {
        m_highlightBlocksInfo = highlightBlocksInfo;
        q->viewport()->update();
        m_extraArea->update();
    }
}

void TextEditorWidget::changeEvent(QEvent *e)
{
    QPlainTextEdit::changeEvent(e);
    if (e->type() == QEvent::ApplicationFontChange
        || e->type() == QEvent::FontChange) {
        if (d->m_extraArea) {
            QFont f = d->m_extraArea->font();
            f.setPointSizeF(font().pointSizeF());
            d->m_extraArea->setFont(f);
            d->slotUpdateExtraAreaWidth();
            d->m_extraArea->update();
        }
    }
}

void TextEditorWidget::focusInEvent(QFocusEvent *e)
{
    QPlainTextEdit::focusInEvent(e);
    d->updateHighlights();
}

void TextEditorWidget::focusOutEvent(QFocusEvent *e)
{
    QPlainTextEdit::focusOutEvent(e);
    if (viewport()->cursor().shape() == Qt::BlankCursor)
        viewport()->setCursor(Qt::IBeamCursor);
}


void TextEditorWidgetPrivate::maybeSelectLine()
{
    QTextCursor cursor = q->textCursor();
    if (!cursor.hasSelection()) {
        const QTextBlock &block = cursor.block();
        if (block.next().isValid()) {
            cursor.setPosition(block.position());
            cursor.setPosition(block.next().position(), QTextCursor::KeepAnchor);
        } else {
            cursor.movePosition(QTextCursor::EndOfBlock);
            cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
            cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
        }
        q->setTextCursor(cursor);
    }
}

// shift+del
void TextEditorWidget::cutLine()
{
    d->maybeSelectLine();
    cut();
}

// ctrl+ins
void TextEditorWidget::copyLine()
{
    QTextCursor prevCursor = textCursor();
    d->maybeSelectLine();
    copy();
    if (!prevCursor.hasSelection())
        prevCursor.movePosition(QTextCursor::StartOfBlock);
    doSetTextCursor(prevCursor, d->m_inBlockSelectionMode);
}

void TextEditorWidget::deleteLine()
{
    d->maybeSelectLine();
    textCursor().removeSelectedText();
}

void TextEditorWidget::deleteEndOfWord()
{
    moveCursor(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    textCursor().removeSelectedText();
    setTextCursor(textCursor());
}

void TextEditorWidget::deleteEndOfWordCamelCase()
{
    QTextCursor c = textCursor();
    d->camelCaseRight(c, QTextCursor::KeepAnchor);
    c.removeSelectedText();
    setTextCursor(c);
}

void TextEditorWidget::deleteStartOfWord()
{
    moveCursor(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
    textCursor().removeSelectedText();
    setTextCursor(textCursor());
}

void TextEditorWidget::deleteStartOfWordCamelCase()
{
    QTextCursor c = textCursor();
    d->camelCaseLeft(c, QTextCursor::KeepAnchor);
    c.removeSelectedText();
    setTextCursor(c);
}

void TextEditorWidgetPrivate::setExtraSelections(Id kind, const QList<QTextEdit::ExtraSelection> &selections)
{
    if (selections.isEmpty() && m_extraSelections[kind].isEmpty())
        return;
    m_extraSelections[kind] = selections;

    if (kind == TextEditorWidget::CodeSemanticsSelection) {
        m_overlay->clear();
        foreach (const QTextEdit::ExtraSelection &selection, m_extraSelections[kind]) {
            m_overlay->addOverlaySelection(selection.cursor,
                                              selection.format.background().color(),
                                              selection.format.background().color(),
                                              TextEditorOverlay::LockSize);
        }
        m_overlay->setVisible(!m_overlay->isEmpty());
    } else if (kind == TextEditorWidget::SnippetPlaceholderSelection) {
        m_snippetOverlay->mangle();
        m_snippetOverlay->clear();
        foreach (const QTextEdit::ExtraSelection &selection, m_extraSelections[kind]) {
            m_snippetOverlay->addOverlaySelection(selection.cursor,
                                              selection.format.background().color(),
                                              selection.format.background().color(),
                                              TextEditorOverlay::ExpandBegin);
        }
        m_snippetOverlay->mapEquivalentSelections();
        m_snippetOverlay->setVisible(!m_snippetOverlay->isEmpty());
    } else {
        QList<QTextEdit::ExtraSelection> all;
        for (auto i = m_extraSelections.constBegin(); i != m_extraSelections.constEnd(); ++i) {
            if (i.key() == TextEditorWidget::CodeSemanticsSelection
                || i.key() == TextEditorWidget::SnippetPlaceholderSelection)
                continue;
            all += i.value();
        }
        q->QPlainTextEdit::setExtraSelections(all);
    }
}

void TextEditorWidget::setExtraSelections(Id kind, const QList<QTextEdit::ExtraSelection> &selections)
{
    d->setExtraSelections(kind, selections);
}

QList<QTextEdit::ExtraSelection> TextEditorWidget::extraSelections(Id kind) const
{
    return d->m_extraSelections.value(kind);
}

QString TextEditorWidget::extraSelectionTooltip(int pos) const
{
    foreach (const QList<QTextEdit::ExtraSelection> &sel, d->m_extraSelections) {
        for (int j = 0; j < sel.size(); ++j) {
            const QTextEdit::ExtraSelection &s = sel.at(j);
            if (s.cursor.selectionStart() <= pos
                && s.cursor.selectionEnd() >= pos
                && !s.format.toolTip().isEmpty())
                return s.format.toolTip();
        }
    }
    return QString();
}

// the blocks list must be sorted
void TextEditorWidget::setIfdefedOutBlocks(const QList<BlockRange> &blocks)
{
    QTextDocument *doc = document();
    TextDocumentLayout *documentLayout = qobject_cast<TextDocumentLayout*>(doc->documentLayout());
    QTC_ASSERT(documentLayout, return);

    bool needUpdate = false;

    QTextBlock block = doc->firstBlock();

    int rangeNumber = 0;
    int braceDepthDelta = 0;
    while (block.isValid()) {
        bool cleared = false;
        bool set = false;
        if (rangeNumber < blocks.size()) {
            const BlockRange &range = blocks.at(rangeNumber);
            if (block.position() >= range.first()
                    && ((block.position() + block.length() - 1) <= range.last() || !range.last()))
                set = TextDocumentLayout::setIfdefedOut(block);
            else
                cleared = TextDocumentLayout::clearIfdefedOut(block);
            if (block.contains(range.last()))
                ++rangeNumber;
        } else {
            cleared = TextDocumentLayout::clearIfdefedOut(block);
        }

        if (cleared || set) {
            needUpdate = true;
            int delta = TextDocumentLayout::braceDepthDelta(block);
            if (cleared)
                braceDepthDelta += delta;
            else if (set)
                braceDepthDelta -= delta;
        }

        if (braceDepthDelta) {
            TextDocumentLayout::changeBraceDepth(block,braceDepthDelta);
            TextDocumentLayout::changeFoldingIndent(block, braceDepthDelta); // ### C++ only, refactor!
        }

        block = block.next();
    }

    if (needUpdate)
        documentLayout->requestUpdate();
}

void TextEditorWidget::format()
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();
    d->m_document->autoIndent(cursor);
    cursor.endEditBlock();
}

void TextEditorWidget::rewrapParagraph()
{
    const int paragraphWidth = marginSettings().m_marginColumn;
    const QRegExp anyLettersOrNumbers = QRegExp(QLatin1String("\\w"));
    const int tabSize = d->m_document->tabSettings().m_tabSize;

    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    // Find start of paragraph.

    while (cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::MoveAnchor)) {
        QTextBlock block = cursor.block();
        QString text = block.text();

        // If this block is empty, move marker back to previous and terminate.
        if (!text.contains(anyLettersOrNumbers)) {
            cursor.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor);
            break;
        }
    }

    cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);

    // Find indent level of current block.

    int indentLevel = 0;
    QString text = cursor.block().text();

    for (int i = 0; i < text.length(); i++) {
        const QChar ch = text.at(i);

        if (ch == QLatin1Char(' '))
            indentLevel++;
        else if (ch == QLatin1Char('\t'))
            indentLevel += tabSize - (indentLevel % tabSize);
        else
            break;
    }

    // If there is a common prefix, it should be kept and expanded to all lines.
    // this allows nice reflowing of doxygen style comments.
    QTextCursor nextBlock = cursor;
    QString commonPrefix;

    if (nextBlock.movePosition(QTextCursor::NextBlock))
    {
         QString nText = nextBlock.block().text();
         int maxLength = qMin(text.length(), nText.length());

         for (int i = 0; i < maxLength; ++i) {
             const QChar ch = text.at(i);

             if (ch != nText[i] || ch.isLetterOrNumber())
                 break;
             commonPrefix.append(ch);
         }
    }


    // Find end of paragraph.
    while (cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor)) {
        QString text = cursor.block().text();

        if (!text.contains(anyLettersOrNumbers))
            break;
    }


    QString selectedText = cursor.selectedText();

    // Preserve initial indent level.or common prefix.
    QString spacing;

    if (commonPrefix.isEmpty()) {
        spacing = d->m_document->tabSettings().indentationString(
                    0, indentLevel, textCursor().block());
    } else {
        spacing = commonPrefix;
        indentLevel = commonPrefix.length();
    }

    int currentLength = indentLevel;
    QString result;
    result.append(spacing);

    // Remove existing instances of any common prefix from paragraph to
    // reflow.
    selectedText.remove(0, commonPrefix.length());
    commonPrefix.prepend(QChar::ParagraphSeparator);
    selectedText.replace(commonPrefix, QLatin1String("\n"));

    // remove any repeated spaces, trim lines to PARAGRAPH_WIDTH width and
    // keep the same indentation level as first line in paragraph.
    QString currentWord;

    for (int i = 0; i < selectedText.length(); ++i) {
        QChar ch = selectedText.at(i);
        if (ch.isSpace()) {
            if (!currentWord.isEmpty()) {
                currentLength += currentWord.length() + 1;

                if (currentLength > paragraphWidth) {
                    currentLength = currentWord.length() + 1 + indentLevel;
                    result.chop(1); // remove trailing space
                    result.append(QChar::ParagraphSeparator);
                    result.append(spacing);
                }

                result.append(currentWord);
                result.append(QLatin1Char(' '));
                currentWord.clear();
            }

            continue;
        }

        currentWord.append(ch);
    }
    result.chop(1);
    result.append(QChar::ParagraphSeparator);

    cursor.insertText(result);
    cursor.endEditBlock();
}

void TextEditorWidget::unCommentSelection()
{
    Utils::unCommentSelection(this, d->m_commentDefinition);
}

void TextEditorWidget::encourageApply()
{
    if (!d->m_snippetOverlay->isVisible() || d->m_snippetOverlay->isEmpty())
        return;
    d->m_snippetOverlay->updateEquivalentSelections(textCursor());
}

void TextEditorWidget::showEvent(QShowEvent* e)
{
    triggerPendingUpdates();
    QPlainTextEdit::showEvent(e);
}


void TextEditorWidgetPrivate::applyFontSettingsDelayed()
{
    m_fontSettingsNeedsApply = true;
    if (q->isVisible())
        q->triggerPendingUpdates();
}

void TextEditorWidget::triggerPendingUpdates()
{
    if (d->m_fontSettingsNeedsApply)
        applyFontSettings();
    textDocument()->triggerPendingUpdates();
}

void TextEditorWidget::applyFontSettings()
{
    d->m_fontSettingsNeedsApply = false;
    const FontSettings &fs = textDocument()->fontSettings();
    const QTextCharFormat textFormat = fs.toTextCharFormat(C_TEXT);
    const QTextCharFormat selectionFormat = fs.toTextCharFormat(C_SELECTION);
    const QTextCharFormat lineNumberFormat = fs.toTextCharFormat(C_LINE_NUMBER);
    QFont font(textFormat.font());

    const QColor foreground = textFormat.foreground().color();
    const QColor background = textFormat.background().color();
    QPalette p = palette();
    p.setColor(QPalette::Text, foreground);
    p.setColor(QPalette::Foreground, foreground);
    p.setColor(QPalette::Base, background);
    p.setColor(QPalette::Highlight, (selectionFormat.background().style() != Qt::NoBrush) ?
               selectionFormat.background().color() :
               QApplication::palette().color(QPalette::Highlight));

    p.setBrush(QPalette::HighlightedText, selectionFormat.foreground());

    p.setBrush(QPalette::Inactive, QPalette::Highlight, p.highlight());
    p.setBrush(QPalette::Inactive, QPalette::HighlightedText, p.highlightedText());
    setPalette(p);
    setFont(font);
    d->updateTabStops(); // update tab stops, they depend on the font

    // Line numbers
    QPalette ep;
    ep.setColor(QPalette::Dark, lineNumberFormat.foreground().color());
    ep.setColor(QPalette::Background, lineNumberFormat.background().style() != Qt::NoBrush ?
                lineNumberFormat.background().color() : background);
    d->m_extraArea->setPalette(ep);

    d->slotUpdateExtraAreaWidth();   // Adjust to new font width
    d->updateHighlights();
}

void TextEditorWidget::setDisplaySettings(const DisplaySettings &ds)
{
    setLineWrapMode(ds.m_textWrapping ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
    setLineNumbersVisible(ds.m_displayLineNumbers);
    setHighlightCurrentLine(ds.m_highlightCurrentLine);
    setRevisionsVisible(ds.m_markTextChanges);
    setCenterOnScroll(ds.m_centerCursorOnScroll);
    setParenthesesMatchingEnabled(ds.m_highlightMatchingParentheses);
    d->m_fileEncodingLabelAction->setVisible(ds.m_displayFileEncoding);

    if (d->m_displaySettings.m_visualizeWhitespace != ds.m_visualizeWhitespace) {
        if (SyntaxHighlighter *highlighter = textDocument()->syntaxHighlighter())
            highlighter->rehighlight();
        QTextOption option =  document()->defaultTextOption();
        if (ds.m_visualizeWhitespace)
            option.setFlags(option.flags() | QTextOption::ShowTabsAndSpaces);
        else
            option.setFlags(option.flags() & ~QTextOption::ShowTabsAndSpaces);
        option.setFlags(option.flags() | QTextOption::AddSpaceForLineAndParagraphSeparators);
        document()->setDefaultTextOption(option);
    }

    d->m_displaySettings = ds;
    if (!ds.m_highlightBlocks) {
        d->extraAreaHighlightFoldedBlockNumber = -1;
        d->m_highlightBlocksInfo = TextEditorPrivateHighlightBlocks();
    }

    d->updateCodeFoldingVisible();
    d->updateHighlights();
    d->setupScrollBar();
    viewport()->update();
    extraArea()->update();
}

void TextEditorWidget::setMarginSettings(const MarginSettings &ms)
{
    setVisibleWrapColumn(ms.m_showMargin ? ms.m_marginColumn : 0);
    d->m_marginSettings = ms;

    viewport()->update();
    extraArea()->update();
}

void TextEditorWidget::setBehaviorSettings(const BehaviorSettings &bs)
{
    d->m_behaviorSettings = bs;
}

void TextEditorWidget::setTypingSettings(const TypingSettings &typingSettings)
{
    d->m_document->setTypingSettings(typingSettings);
}

void TextEditorWidget::setStorageSettings(const StorageSettings &storageSettings)
{
    d->m_document->setStorageSettings(storageSettings);
}

void TextEditorWidget::setCompletionSettings(const CompletionSettings &completionSettings)
{
    d->m_autoCompleter->setAutoParenthesesEnabled(completionSettings.m_autoInsertBrackets);
    d->m_autoCompleter->setSurroundWithEnabled(completionSettings.m_autoInsertBrackets
                                               && completionSettings.m_surroundingAutoBrackets);
    d->m_codeAssistant.updateFromCompletionSettings(completionSettings);
}

void TextEditorWidget::setExtraEncodingSettings(const ExtraEncodingSettings &extraEncodingSettings)
{
    d->m_document->setExtraEncodingSettings(extraEncodingSettings);
}

void TextEditorWidget::fold()
{
    QTextDocument *doc = document();
    TextDocumentLayout *documentLayout = qobject_cast<TextDocumentLayout*>(doc->documentLayout());
    QTC_ASSERT(documentLayout, return);
    QTextBlock block = textCursor().block();
    if (!(TextDocumentLayout::canFold(block) && block.next().isVisible())) {
        // find the closest previous block which can fold
        int indent = TextDocumentLayout::foldingIndent(block);
        while (block.isValid() && (TextDocumentLayout::foldingIndent(block) >= indent || !block.isVisible()))
            block = block.previous();
    }
    if (block.isValid()) {
        TextDocumentLayout::doFoldOrUnfold(block, false);
        d->moveCursorVisible();
        documentLayout->requestUpdate();
        documentLayout->emitDocumentSizeChanged();
    }
}

void TextEditorWidget::unfold()
{
    QTextDocument *doc = document();
    TextDocumentLayout *documentLayout = qobject_cast<TextDocumentLayout*>(doc->documentLayout());
    QTC_ASSERT(documentLayout, return);
    QTextBlock block = textCursor().block();
    while (block.isValid() && !block.isVisible())
        block = block.previous();
    TextDocumentLayout::doFoldOrUnfold(block, true);
    d->moveCursorVisible();
    documentLayout->requestUpdate();
    documentLayout->emitDocumentSizeChanged();
}

void TextEditorWidget::unfoldAll()
{
    QTextDocument *doc = document();
    TextDocumentLayout *documentLayout = qobject_cast<TextDocumentLayout*>(doc->documentLayout());
    QTC_ASSERT(documentLayout, return);

    QTextBlock block = doc->firstBlock();
    bool makeVisible = true;
    while (block.isValid()) {
        if (block.isVisible() && TextDocumentLayout::canFold(block) && block.next().isVisible()) {
            makeVisible = false;
            break;
        }
        block = block.next();
    }

    block = doc->firstBlock();

    while (block.isValid()) {
        if (TextDocumentLayout::canFold(block))
            TextDocumentLayout::doFoldOrUnfold(block, makeVisible);
        block = block.next();
    }

    d->moveCursorVisible();
    documentLayout->requestUpdate();
    documentLayout->emitDocumentSizeChanged();
    centerCursor();
}

void TextEditorWidget::setReadOnly(bool b)
{
    QPlainTextEdit::setReadOnly(b);
    emit readOnlyChanged();
    if (b)
        setTextInteractionFlags(textInteractionFlags() | Qt::TextSelectableByKeyboard);
}

void TextEditorWidget::cut()
{
    if (d->m_inBlockSelectionMode) {
        copy();
        d->removeBlockSelection();
        return;
    }
    QPlainTextEdit::cut();
    d->collectToCircularClipboard();
}

void TextEditorWidget::selectAll()
{
    if (d->m_inBlockSelectionMode)
        d->disableBlockSelection();
    QPlainTextEdit::selectAll();
}

void TextEditorWidget::copy()
{
    if (!textCursor().hasSelection() || (d->m_inBlockSelectionMode
            && d->m_blockSelection.anchorColumn == d->m_blockSelection.positionColumn)) {
        return;
    }

    QPlainTextEdit::copy();
    d->collectToCircularClipboard();
}

void TextEditorWidget::paste()
{
    QPlainTextEdit::paste();
    encourageApply();
}

void TextEditorWidgetPrivate::collectToCircularClipboard()
{
    const QMimeData *mimeData = QApplication::clipboard()->mimeData();
    if (!mimeData)
        return;
    CircularClipboard *circularClipBoard = CircularClipboard::instance();
    circularClipBoard->collect(TextEditorWidget::duplicateMimeData(mimeData));
    // We want the latest copied content to be the first one to appear on circular paste.
    circularClipBoard->toLastCollect();
}

void TextEditorWidget::circularPaste()
{
    CircularClipboard *circularClipBoard = CircularClipboard::instance();
    if (const QMimeData *clipboardData = QApplication::clipboard()->mimeData()) {
        circularClipBoard->collect(TextEditorWidget::duplicateMimeData(clipboardData));
        circularClipBoard->toLastCollect();
    }

    if (circularClipBoard->size() > 1)
        return invokeAssist(QuickFix, d->m_clipboardAssistProvider.data());

    if (const QMimeData *mimeData = circularClipBoard->next().data()) {
        QApplication::clipboard()->setMimeData(TextEditorWidget::duplicateMimeData(mimeData));
        paste();
    }
}

void TextEditorWidget::switchUtf8bom()
{
    textDocument()->switchUtf8Bom();
}

QMimeData *TextEditorWidget::createMimeDataFromSelection() const
{
    if (d->m_inBlockSelectionMode) {
        QMimeData *mimeData = new QMimeData;
        mimeData->setText(d->copyBlockSelection());
        return mimeData;
    } else if (textCursor().hasSelection()) {
        QTextCursor cursor = textCursor();
        QMimeData *mimeData = new QMimeData;

        QString text = plainTextFromSelection(cursor);
        mimeData->setText(text);

        // Copy the selected text as HTML
        {
            // Create a new document from the selected text document fragment
            QTextDocument *tempDocument = new QTextDocument;
            QTextCursor tempCursor(tempDocument);
            tempCursor.insertFragment(cursor.selection());

            // Apply the additional formats set by the syntax highlighter
            QTextBlock start = document()->findBlock(cursor.selectionStart());
            QTextBlock last = document()->findBlock(cursor.selectionEnd());
            QTextBlock end = last.next();

            const int selectionStart = cursor.selectionStart();
            const int endOfDocument = tempDocument->characterCount() - 1;
            int removedCount = 0;
            for (QTextBlock current = start; current.isValid() && current != end; current = current.next()) {
                if (selectionVisible(current.blockNumber())) {
                    const QTextLayout *layout = current.layout();
                    foreach (const QTextLayout::FormatRange &range, layout->additionalFormats()) {
                        const int startPosition = current.position() + range.start - selectionStart - removedCount;
                        const int endPosition = startPosition + range.length;
                        if (endPosition <= 0 || startPosition >= endOfDocument - removedCount)
                            continue;
                        tempCursor.setPosition(qMax(startPosition, 0));
                        tempCursor.setPosition(qMin(endPosition, endOfDocument - removedCount), QTextCursor::KeepAnchor);
                        tempCursor.setCharFormat(range.format);
                    }
                } else {
                    const int startPosition = current.position() - start.position() - removedCount;
                    int endPosition = startPosition + current.text().count();
                    if (current != last)
                        endPosition++;
                    removedCount += endPosition - startPosition;
                    tempCursor.setPosition(startPosition);
                    tempCursor.setPosition(endPosition, QTextCursor::KeepAnchor);
                    tempCursor.deleteChar();
                }
            }

            // Reset the user states since they are not interesting
            for (QTextBlock block = tempDocument->begin(); block.isValid(); block = block.next())
                block.setUserState(-1);

            // Make sure the text appears pre-formatted
            tempCursor.setPosition(0);
            tempCursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
            QTextBlockFormat blockFormat = tempCursor.blockFormat();
            blockFormat.setNonBreakableLines(true);
            tempCursor.setBlockFormat(blockFormat);

            mimeData->setHtml(tempCursor.selection().toHtml());
            delete tempDocument;
        }

        /*
          Try to figure out whether we are copying an entire block, and store the complete block
          including indentation in the qtcreator.blocktext mimetype.
        */
        QTextCursor selstart = cursor;
        selstart.setPosition(cursor.selectionStart());
        QTextCursor selend = cursor;
        selend.setPosition(cursor.selectionEnd());

        bool startOk = TabSettings::cursorIsAtBeginningOfLine(selstart);
        bool multipleBlocks = (selend.block() != selstart.block());

        if (startOk && multipleBlocks) {
            selstart.movePosition(QTextCursor::StartOfBlock);
            if (TabSettings::cursorIsAtBeginningOfLine(selend))
                selend.movePosition(QTextCursor::StartOfBlock);
            cursor.setPosition(selstart.position());
            cursor.setPosition(selend.position(), QTextCursor::KeepAnchor);
            text = plainTextFromSelection(cursor);
            mimeData->setData(QLatin1String(kTextBlockMimeType), text.toUtf8());
        }
        return mimeData;
    }
    return 0;
}

bool TextEditorWidget::canInsertFromMimeData(const QMimeData *source) const
{
    return QPlainTextEdit::canInsertFromMimeData(source);
}

void TextEditorWidget::insertFromMimeData(const QMimeData *source)
{
    if (isReadOnly())
        return;

    QString text = source->text();
    if (text.isEmpty())
        return;

    if (d->m_codeAssistant.hasContext())
        d->m_codeAssistant.destroyContext();

    if (d->m_inBlockSelectionMode) {
        d->insertIntoBlockSelection(text);
        return;
    }

    if (d->m_snippetOverlay->isVisible() && (text.contains(QLatin1Char('\n'))
                                             || text.contains(QLatin1Char('\t')))) {
        d->m_snippetOverlay->hide();
        d->m_snippetOverlay->mangle();
        d->m_snippetOverlay->clear();
    }

    const TypingSettings &tps = d->m_document->typingSettings();
    QTextCursor cursor = textCursor();
    if (!tps.m_autoIndent) {
        cursor.beginEditBlock();
        cursor.insertText(text);
        cursor.endEditBlock();
        setTextCursor(cursor);
        return;
    }

    cursor.beginEditBlock();
    cursor.removeSelectedText();

    bool insertAtBeginningOfLine = TabSettings::cursorIsAtBeginningOfLine(cursor);

    if (insertAtBeginningOfLine
        && source->hasFormat(QLatin1String(kTextBlockMimeType))) {
        text = QString::fromUtf8(source->data(QLatin1String(kTextBlockMimeType)));
        if (text.isEmpty())
            return;
    }

    int reindentBlockStart = cursor.blockNumber() + (insertAtBeginningOfLine?0:1);

    bool hasFinalNewline = (text.endsWith(QLatin1Char('\n'))
                            || text.endsWith(QChar::ParagraphSeparator)
                            || text.endsWith(QLatin1Char('\r')));

    if (insertAtBeginningOfLine
        && hasFinalNewline) // since we'll add a final newline, preserve current line's indentation
        cursor.setPosition(cursor.block().position());

    int cursorPosition = cursor.position();
    cursor.insertText(text);

    int reindentBlockEnd = cursor.blockNumber() - (hasFinalNewline?1:0);

    if (reindentBlockStart < reindentBlockEnd
        || (reindentBlockStart == reindentBlockEnd
            && (!insertAtBeginningOfLine || hasFinalNewline))) {
        if (insertAtBeginningOfLine && !hasFinalNewline) {
            QTextCursor unnecessaryWhitespace = cursor;
            unnecessaryWhitespace.setPosition(cursorPosition);
            unnecessaryWhitespace.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
            unnecessaryWhitespace.removeSelectedText();
        }
        QTextCursor c = cursor;
        c.setPosition(cursor.document()->findBlockByNumber(reindentBlockStart).position());
        c.setPosition(cursor.document()->findBlockByNumber(reindentBlockEnd).position(),
                      QTextCursor::KeepAnchor);
        d->m_document->autoReindent(c);
    }

    cursor.endEditBlock();
    setTextCursor(cursor);
}

QMimeData *TextEditorWidget::duplicateMimeData(const QMimeData *source)
{
    Q_ASSERT(source);

    QMimeData *mimeData = new QMimeData;
    mimeData->setText(source->text());
    mimeData->setHtml(source->html());
    if (source->hasFormat(QLatin1String(kTextBlockMimeType))) {
        mimeData->setData(QLatin1String(kTextBlockMimeType),
                          source->data(QLatin1String(kTextBlockMimeType)));
    }

    return mimeData;
}

QString TextEditorWidget::lineNumber(int blockNumber) const
{
    return QString::number(blockNumber + 1);
}

int TextEditorWidget::lineNumberDigits() const
{
    int digits = 2;
    int max = qMax(1, blockCount());
    while (max >= 100) {
        max /= 10;
        ++digits;
    }
    return digits;
}

bool TextEditorWidget::selectionVisible(int blockNumber) const
{
    Q_UNUSED(blockNumber);
    return true;
}

bool TextEditorWidget::replacementVisible(int blockNumber) const
{
    Q_UNUSED(blockNumber)
    return true;
}

QColor TextEditorWidget::replacementPenColor(int blockNumber) const
{
    Q_UNUSED(blockNumber)
    return QColor();
}

void TextEditorWidget::setupFallBackEditor(Id id)
{
    TextDocumentPtr doc(new TextDocument(id));
    doc->setFontSettings(TextEditorSettings::fontSettings());
    setTextDocument(doc);
}

void TextEditorWidget::appendStandardContextMenuActions(QMenu *menu)
{
    menu->addSeparator();

    QAction *a = ActionManager::command(Core::Constants::CUT)->action();
    if (a && a->isEnabled())
        menu->addAction(a);
    a = ActionManager::command(Core::Constants::COPY)->action();
    if (a && a->isEnabled())
        menu->addAction(a);
    a = ActionManager::command(Core::Constants::PASTE)->action();
    if (a && a->isEnabled())
        menu->addAction(a);
    a = ActionManager::command(Constants::CIRCULAR_PASTE)->action();
    if (a && a->isEnabled())
        menu->addAction(a);

    TextDocument *doc = textDocument();
    if (doc->codec()->name() == QByteArray("UTF-8") && doc->supportsUtf8Bom()) {
        a = ActionManager::command(Constants::SWITCH_UTF8BOM)->action();
        if (a && a->isEnabled()) {
            a->setText(doc->format().hasUtf8Bom ? tr("Delete UTF-8 BOM on Save")
                                                : tr("Add UTF-8 BOM on Save"));
            menu->addSeparator();
            menu->addAction(a);
        }
    }
}


BaseTextEditor::BaseTextEditor()
    : d(new BaseTextEditorPrivate)
{
    addContext(Constants::C_TEXTEDITOR);
}

BaseTextEditor::~BaseTextEditor()
{
    delete m_widget;
    delete d;
}

TextDocument *BaseTextEditor::textDocument() const
{
    TextEditorWidget *widget = editorWidget();
    QTC_CHECK(!widget->d->m_document.isNull());
    return widget->d->m_document.data();
}

void BaseTextEditor::addContext(Id id)
{
    m_context.add(id);
}

IDocument *BaseTextEditor::document()
{
    return textDocument();
}

QWidget *BaseTextEditor::toolBar()
{
    return editorWidget()->d->m_toolBar;
}

void TextEditorWidget::insertExtraToolBarWidget(TextEditorWidget::Side side,
                                              QWidget *widget)
{
    if (widget->sizePolicy().horizontalPolicy() & QSizePolicy::ExpandFlag) {
        if (d->m_stretchWidget)
            d->m_stretchWidget->deleteLater();
        d->m_stretchWidget = 0;
    }

    if (side == Right)
        d->m_toolBar->insertWidget(d->m_cursorPositionLabelAction, widget);
    else
        d->m_toolBar->insertWidget(d->m_toolBar->actions().first(), widget);
}

int BaseTextEditor::currentLine() const
{
    return editorWidget()->textCursor().blockNumber() + 1;
}

int BaseTextEditor::currentColumn() const
{
    QTextCursor cursor = editorWidget()->textCursor();
    return cursor.position() - cursor.block().position() + 1;
}

void BaseTextEditor::gotoLine(int line, int column, bool centerLine)
{
    editorWidget()->gotoLine(line, column, centerLine);
}

int BaseTextEditor::columnCount() const
{
    return editorWidget()->columnCount();
}

int BaseTextEditor::rowCount() const
{
    return editorWidget()->rowCount();
}

int BaseTextEditor::position(TextPositionOperation posOp, int at) const
{
    return editorWidget()->position(posOp, at);
}

void BaseTextEditor::convertPosition(int pos, int *line, int *column) const
{
    editorWidget()->convertPosition(pos, line, column);
}

QString BaseTextEditor::selectedText() const
{
    return editorWidget()->selectedText();
}

void BaseTextEditor::remove(int length)
{
    editorWidget()->remove(length);
}

void TextEditorWidget::remove(int length)
{
    QTextCursor tc = textCursor();
    tc.setPosition(tc.position() + length, QTextCursor::KeepAnchor);
    tc.removeSelectedText();
}

void BaseTextEditor::insert(const QString &string)
{
    editorWidget()->insertPlainText(string);
}

void BaseTextEditor::replace(int length, const QString &string)
{
    editorWidget()->replace(length, string);
}

void TextEditorWidget::replace(int length, const QString &string)
{
    QTextCursor tc = textCursor();
    tc.setPosition(tc.position() + length, QTextCursor::KeepAnchor);
    tc.insertText(string);
}

void BaseTextEditor::setCursorPosition(int pos)
{
    editorWidget()->setCursorPosition(pos);
}

void TextEditorWidget::setCursorPosition(int pos)
{
    setBlockSelection(false);
    QTextCursor tc = textCursor();
    tc.setPosition(pos);
    setTextCursor(tc);
}

void BaseTextEditor::select(int toPos)
{
    editorWidget()->setBlockSelection(false);
    QTextCursor tc = editorWidget()->textCursor();
    tc.setPosition(toPos, QTextCursor::KeepAnchor);
    editorWidget()->setTextCursor(tc);
}

void TextEditorWidgetPrivate::updateCursorPosition()
{
    const QTextCursor cursor = q->textCursor();
    const QTextBlock block = cursor.block();
    const int line = block.blockNumber() + 1;
    const int column = cursor.position() - block.position();
    m_cursorPositionLabel->show();
    m_cursorPositionLabel->setText(TextEditorWidget::tr("Line: %1, Col: %2").arg(line)
                                   .arg(q->textDocument()->tabSettings().columnAt(block.text(),
                                                                                   column)+1),
                                   TextEditorWidget::tr("Line: 9999, Col: 999"));
    m_contextHelpId.clear();

    if (!block.isVisible())
        q->ensureCursorVisible();
}

QString BaseTextEditor::contextHelpId() const
{
    return editorWidget()->contextHelpId();
}

void BaseTextEditor::setContextHelpId(const QString &id)
{
    IEditor::setContextHelpId(id);
    editorWidget()->setContextHelpId(id);
}

QString TextEditorWidget::contextHelpId()
{
    if (d->m_contextHelpId.isEmpty() && !d->m_hoverHandlers.isEmpty())
        d->m_contextHelpId = d->m_hoverHandlers.first()->contextHelpId(this, textCursor().position());
    return d->m_contextHelpId;
}

void TextEditorWidget::setContextHelpId(const QString &id)
{
    d->m_contextHelpId = id;
}

RefactorMarkers TextEditorWidget::refactorMarkers() const
{
    return d->m_refactorOverlay->markers();
}

void TextEditorWidget::setRefactorMarkers(const RefactorMarkers &markers)
{
    foreach (const RefactorMarker &marker, d->m_refactorOverlay->markers())
        requestBlockUpdate(marker.cursor.block());
    d->m_refactorOverlay->setMarkers(markers);
    foreach (const RefactorMarker &marker, markers)
        requestBlockUpdate(marker.cursor.block());
}

void TextEditorWidget::doFoo()
{
#ifdef DO_FOO
    qDebug() << Q_FUNC_INFO;
    RefactorMarkers markers = d->m_refactorOverlay->markers();
    RefactorMarker marker;
    marker.tooltip = "Hello World";
    marker.cursor = textCursor();
    markers += marker;
    setRefactorMarkers(markers);
#endif
}

TextBlockSelection::TextBlockSelection(const TextBlockSelection &other)
{
    positionBlock = other.positionBlock;
    positionColumn = other.positionColumn;
    anchorBlock = other.anchorBlock;
    anchorColumn = other.anchorColumn;
}

void TextBlockSelection::clear()
{
    positionColumn = positionBlock = anchorColumn = anchorBlock = 0;
}

// returns a cursor which always has the complete selection
QTextCursor TextBlockSelection::selection(const TextDocument *baseTextDocument) const
{
    return cursor(baseTextDocument, true);
}

// returns a cursor which always has the correct position and anchor
QTextCursor TextBlockSelection::cursor(const TextDocument *baseTextDocument) const
{
    return cursor(baseTextDocument, false);
}

QTextCursor TextBlockSelection::cursor(const TextDocument *baseTextDocument,
                                           bool fullSelection) const
{
    if (!baseTextDocument)
        return QTextCursor();
    QTextDocument *document = baseTextDocument->document();
    const TabSettings &ts = baseTextDocument->tabSettings();

    int selectionAnchorColumn;
    int selectionPositionColumn;
    if (anchorBlock == positionBlock || !fullSelection) {
        selectionAnchorColumn = anchorColumn;
        selectionPositionColumn = positionColumn;
    } else if (anchorBlock == firstBlockNumber()){
        selectionAnchorColumn = firstVisualColumn();
        selectionPositionColumn = lastVisualColumn();
    } else {
        selectionAnchorColumn = lastVisualColumn();
        selectionPositionColumn = firstVisualColumn();
    }

    QTextCursor cursor(document);

    const QTextBlock &anchorTextBlock = document->findBlockByNumber(anchorBlock);
    const int anchorPosition = anchorTextBlock.position()
            + ts.positionAtColumn(anchorTextBlock.text(), selectionAnchorColumn);

    const QTextBlock &positionTextBlock = document->findBlockByNumber(positionBlock);
    const int cursorPosition = positionTextBlock.position()
            + ts.positionAtColumn(positionTextBlock.text(), selectionPositionColumn);

    cursor.setPosition(anchorPosition);
    cursor.setPosition(cursorPosition, QTextCursor::KeepAnchor);
    return cursor;
}

void TextBlockSelection::fromPostition(int positionBlock, int positionColumn,
                                           int anchorBlock, int anchorColumn)
{
    this->positionBlock = positionBlock;
    this->positionColumn = positionColumn;
    this->anchorBlock = anchorBlock;
    this->anchorColumn = anchorColumn;
}

bool TextEditorWidget::inFindScope(const QTextCursor &cursor)
{
    if (cursor.isNull())
        return false;
    return inFindScope(cursor.selectionStart(), cursor.selectionEnd());
}

bool TextEditorWidget::inFindScope(int selectionStart, int selectionEnd)
{
    if (d->m_findScopeStart.isNull())
        return true; // no scope, everything is included
    if (selectionStart < d->m_findScopeStart.position())
        return false;
    if (selectionEnd > d->m_findScopeEnd.position())
        return false;
    if (d->m_findScopeVerticalBlockSelectionFirstColumn < 0)
        return true;
    QTextBlock block = document()->findBlock(selectionStart);
    if (block != document()->findBlock(selectionEnd))
        return false;
    QString text = block.text();
    const TabSettings &ts = d->m_document->tabSettings();
    int startPosition = ts.positionAtColumn(text, d->m_findScopeVerticalBlockSelectionFirstColumn);
    int endPosition = ts.positionAtColumn(text, d->m_findScopeVerticalBlockSelectionLastColumn);
    if (selectionStart - block.position() < startPosition)
        return false;
    if (selectionEnd - block.position() > endPosition)
        return false;
    return true;
}

void TextEditorWidget::setBlockSelection(bool on)
{
    if (d->m_inBlockSelectionMode == on)
        return;

    if (on)
        d->enableBlockSelection(textCursor());
    else
        d->disableBlockSelection(false);
}

void TextEditorWidget::setBlockSelection(int positionBlock, int positionColumn,
                                             int anchhorBlock, int anchorColumn)
{
    d->enableBlockSelection(positionBlock, positionColumn, anchhorBlock, anchorColumn);
}

void TextEditorWidget::setBlockSelection(const QTextCursor &cursor)
{
    d->enableBlockSelection(cursor);
}

QTextCursor TextEditorWidget::blockSelection() const
{
    return d->m_blockSelection.cursor(d->m_document.data());
}

bool TextEditorWidget::hasBlockSelection() const
{
    return d->m_inBlockSelectionMode;
}

void TextEditorWidgetPrivate::updateTabStops()
{
    // Although the tab stop is stored as qreal the API from QPlainTextEdit only allows it
    // to be set as an int. A work around is to access directly the QTextOption.
    qreal charWidth = QFontMetricsF(q->font()).width(QLatin1Char(' '));
    QTextOption option = q->document()->defaultTextOption();
    option.setTabStop(charWidth * m_document->tabSettings().m_tabSize);
    q->document()->setDefaultTextOption(option);
}

int TextEditorWidget::columnCount() const
{
    QFontMetricsF fm(font());
    return viewport()->rect().width() / fm.width(QLatin1Char('x'));
}

int TextEditorWidget::rowCount() const
{
    QFontMetricsF fm(font());
    return viewport()->rect().height() / fm.lineSpacing();
}

/**
  Helper function to transform a selected text. If nothing is selected at the moment
  the word under the cursor is used.
  The type of the transformation is determined by the function pointer given.

  @param method     pointer to the QString function to use for the transformation

  @see uppercaseSelection, lowercaseSelection
*/
void TextEditorWidgetPrivate::transformSelection(TransformationMethod method)
{
    if (q->hasBlockSelection()) {
        transformBlockSelection(method);
        return;
    }

    QTextCursor cursor = q->textCursor();
    int pos    = cursor.position();
    int anchor = cursor.anchor();

    if (!cursor.hasSelection()) {
        // if nothing is selected, select the word over the cursor
        cursor.select(QTextCursor::WordUnderCursor);
    }

    QString text = cursor.selectedText();
    QString transformedText = method(text);

    if (transformedText == text) {
        // if the transformation does not do anything to the selection, do no create an undo step
        return;
    }

    cursor.insertText(transformedText);

    // (re)select the changed text
    // Note: this assumes the transformation did not change the length,
    cursor.setPosition(anchor);
    cursor.setPosition(pos, QTextCursor::KeepAnchor);
    q->setTextCursor(cursor);
}

void TextEditorWidgetPrivate::transformBlockSelection(TransformationMethod method)
{
    QTextCursor cursor = q->textCursor();
    const TabSettings &ts = m_document->tabSettings();

    // saved to restore the blockselection
    const int positionColumn = m_blockSelection.positionColumn;
    const int positionBlock = m_blockSelection.positionBlock;
    const int anchorColumn = m_blockSelection.anchorColumn;
    const int anchorBlock = m_blockSelection.anchorBlock;

    QTextBlock block = m_document->document()->findBlockByNumber(
                m_blockSelection.firstBlockNumber());
    const QTextBlock &lastBlock = m_document->document()->findBlockByNumber(
                m_blockSelection.lastBlockNumber());

    cursor.beginEditBlock();
    for (;;) {
        // get position of the selection
        const QString &blockText = block.text();
        const int startPos = block.position()
                + ts.positionAtColumn(blockText, m_blockSelection.firstVisualColumn());
        const int endPos = block.position()
                + ts.positionAtColumn(blockText, m_blockSelection.lastVisualColumn());

        // check if the selection is inside the text block
        if (startPos < endPos) {
            cursor.setPosition(startPos);
            cursor.setPosition(endPos, QTextCursor::KeepAnchor);
            const QString &transformedText = method(m_document->textAt(startPos, endPos - startPos));
            if (transformedText != cursor.selectedText())
                cursor.insertText(transformedText);
        }
        if (block == lastBlock)
            break;
        block = block.next();
    }
    cursor.endEditBlock();

    // restore former block selection
    enableBlockSelection(positionBlock, anchorColumn, anchorBlock, positionColumn);
}

void TextEditorWidget::inSnippetMode(bool *active)
{
    *active = d->m_snippetOverlay->isVisible();
}

QTextBlock TextEditorWidget::blockForVisibleRow(int row) const
{
    const int count = rowCount();
    if (row < 0 && row >= count)
        return QTextBlock();

    QTextBlock block = firstVisibleBlock();
    for (int i = 0; i < count; ++i) {
        if (!block.isValid() || i == row)
            return block;

        while (block.isValid()) {
            block = block.next();
            if (block.isVisible())
                break;
        }
    }
    return QTextBlock();

}

void TextEditorWidget::invokeAssist(AssistKind kind, IAssistProvider *provider)
{
    if (kind == QuickFix && d->m_snippetOverlay->isVisible()) {
        d->m_snippetOverlay->setVisible(false);
        d->m_snippetOverlay->mangle();
        d->m_snippetOverlay->clear();
    }

    bool previousMode = overwriteMode();
    setOverwriteMode(false);
    ensureCursorVisible();
    d->m_codeAssistant.invoke(kind, provider);
    setOverwriteMode(previousMode);
}

AssistInterface *TextEditorWidget::createAssistInterface(AssistKind kind,
                                                             AssistReason reason) const
{
    Q_UNUSED(kind);
    return new AssistInterface(document(), position(), d->m_document->filePath().toString(), reason);
}

QString TextEditorWidget::foldReplacementText(const QTextBlock &) const
{
    return QLatin1String("...");
}

QByteArray BaseTextEditor::saveState() const
{
    return editorWidget()->saveState();
}

bool BaseTextEditor::restoreState(const QByteArray &state)
{
    return editorWidget()->restoreState(state);
}

BaseTextEditor *BaseTextEditor::currentTextEditor()
{
    return qobject_cast<BaseTextEditor *>(EditorManager::currentEditor());
}

TextEditorWidget *BaseTextEditor::editorWidget() const
{
    QTC_ASSERT(qobject_cast<TextEditorWidget *>(m_widget.data()), return 0);
    return static_cast<TextEditorWidget *>(m_widget.data());
}

void BaseTextEditor::setTextCursor(const QTextCursor &cursor)
{
    editorWidget()->setTextCursor(cursor);
}

QTextCursor BaseTextEditor::textCursor() const
{
    return editorWidget()->textCursor();
}

QChar BaseTextEditor::characterAt(int pos) const
{
    return textDocument()->characterAt(pos);
}

QString BaseTextEditor::textAt(int from, int to) const
{
    return textDocument()->textAt(from, to);
}

QChar TextEditorWidget::characterAt(int pos) const
{
    return textDocument()->characterAt(pos);
}

QString TextEditorWidget::textAt(int from, int to) const
{
    return textDocument()->textAt(from, to);
}

void TextEditorWidget::configureGenericHighlighter()
{
    Highlighter *highlighter = new Highlighter();
    highlighter->setTabSettings(textDocument()->tabSettings());
    textDocument()->setSyntaxHighlighter(highlighter);

    setCodeFoldingSupported(false);

    const QString type = textDocument()->mimeType();
    Utils::MimeDatabase mdb;
    const Utils::MimeType mimeType = mdb.mimeTypeForName(type);
    if (mimeType.isValid()) {
        d->m_isMissingSyntaxDefinition = true;

        QString definitionId;
        setMimeTypeForHighlighter(highlighter, mimeType, textDocument()->filePath().toString(),
                                  &definitionId);

        if (!definitionId.isEmpty()) {
            d->m_isMissingSyntaxDefinition = false;
            const QSharedPointer<HighlightDefinition> &definition =
                Manager::instance()->definition(definitionId);
            if (!definition.isNull() && definition->isValid()) {
                d->m_commentDefinition.isAfterWhiteSpaces = definition->isCommentAfterWhiteSpaces();
                d->m_commentDefinition.singleLine = definition->singleLineComment();
                d->m_commentDefinition.multiLineStart = definition->multiLineCommentStart();
                d->m_commentDefinition.multiLineEnd = definition->multiLineCommentEnd();

                setCodeFoldingSupported(true);
            }
        } else {
            const QString fileName = textDocument()->filePath().toString();
            if (TextEditorSettings::highlighterSettings().isIgnoredFilePattern(fileName))
                d->m_isMissingSyntaxDefinition = false;
        }
    }

    textDocument()->setFontSettings(TextEditorSettings::fontSettings());

    updateEditorInfoBar(this);
}

int TextEditorWidget::lineForVisibleRow(int row) const
{
    QTextBlock block = blockForVisibleRow(row);
    return block.isValid() ? block.blockNumber() : -1;
}

int TextEditorWidget::firstVisibleLine() const
{
    return lineForVisibleRow(0);
}

int TextEditorWidget::lastVisibleLine() const
{
    QTextBlock block = blockForVisibleRow(rowCount() - 1);
    if (!block.isValid())
        block.previous();
    return block.isValid() ? block.blockNumber() : -1;
}

int TextEditorWidget::centerVisibleLine() const
{
    QTextBlock block = blockForVisibleRow(rowCount() / 2);
    if (!block.isValid())
        block.previous();
    return block.isValid() ? block.blockNumber() : -1;
}

bool TextEditorWidget::isMissingSyntaxDefinition() const
{
    return d->m_isMissingSyntaxDefinition;
}

// The remnants of PlainTextEditor.
void TextEditorWidget::setupGenericHighlighter()
{
    setMarksVisible(true);
    setLineSeparatorsAllowed(true);

    connect(textDocument(), &IDocument::filePathChanged,
            d, &TextEditorWidgetPrivate::reconfigure);

    connect(Manager::instance(), &Manager::highlightingFilesRegistered,
            d, &TextEditorWidgetPrivate::reconfigure);

    updateEditorInfoBar(this);
}

//
// TextEditorLinkLabel
//
TextEditorLinkLabel::TextEditorLinkLabel(QWidget *parent)
    : QLabel(parent)
{
}

void TextEditorLinkLabel::setLink(TextEditorWidget::Link link)
{
    m_link = link;
}

TextEditorWidget::Link TextEditorLinkLabel::link() const
{
    return m_link;
}

void TextEditorLinkLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_dragStartPosition = event->pos();
}

void TextEditorLinkLabel::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton))
        return;
    if ((event->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance())
        return;

    auto data = new DropMimeData;
    data->addFile(m_link.targetFileName, m_link.targetLine, m_link.targetColumn);
    auto drag = new QDrag(this);
    drag->setMimeData(data);
    drag->exec(Qt::MoveAction);
}

void TextEditorLinkLabel::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    if (!m_link.hasValidTarget())
        return;

    EditorManager::openEditorAt(m_link.targetFileName, m_link.targetLine, m_link.targetColumn);
}

//
// BaseTextEditorFactory
//

namespace Internal {

class TextEditorFactoryPrivate
{
public:
    TextEditorFactoryPrivate(TextEditorFactory *parent) :
        q(parent),
        m_widgetCreator([]() { return new TextEditorWidget; }),
        m_editorCreator([]() { return new BaseTextEditor; }),
        m_commentStyle(CommentDefinition::NoStyle),
        m_completionAssistProvider(0),
        m_useGenericHighlighter(false),
        m_duplicatedSupported(true),
        m_codeFoldingSupported(false),
        m_paranthesesMatchinEnabled(false),
        m_marksVisible(false)
    {}

    BaseTextEditor *duplicateTextEditor(BaseTextEditor *other)
    {
        BaseTextEditor *editor = createEditorHelper(other->editorWidget()->textDocumentPtr());
        editor->editorWidget()->finalizeInitializationAfterDuplication(other->editorWidget());
        return editor;
    }

    BaseTextEditor *createEditorHelper(const TextDocumentPtr &doc);

    TextEditorFactory *q;
    TextEditorFactory::DocumentCreator m_documentCreator;
    TextEditorFactory::EditorWidgetCreator m_widgetCreator;
    TextEditorFactory::EditorCreator m_editorCreator;
    TextEditorFactory::AutoCompleterCreator m_autoCompleterCreator;
    TextEditorFactory::IndenterCreator m_indenterCreator;
    TextEditorFactory::SyntaxHighLighterCreator m_syntaxHighlighterCreator;
    CommentDefinition::Style m_commentStyle;
    QList<BaseHoverHandler *> m_hoverHandlers; // owned
    CompletionAssistProvider * m_completionAssistProvider; // owned
    bool m_useGenericHighlighter;
    bool m_duplicatedSupported;
    bool m_codeFoldingSupported;
    bool m_paranthesesMatchinEnabled;
    bool m_marksVisible;
};

} /// namespace Internal

TextEditorFactory::TextEditorFactory(QObject *parent)
    : IEditorFactory(parent), d(new TextEditorFactoryPrivate(this))
{}

TextEditorFactory::~TextEditorFactory()
{
    qDeleteAll(d->m_hoverHandlers);
    delete d->m_completionAssistProvider;
    delete d;
}

void TextEditorFactory::setDocumentCreator(DocumentCreator &&creator)
{
    d->m_documentCreator = std::move(creator);
}

void TextEditorFactory::setEditorWidgetCreator(EditorWidgetCreator &&creator)
{
    d->m_widgetCreator = std::move(creator);
}

void TextEditorFactory::setEditorCreator(EditorCreator &&creator)
{
    d->m_editorCreator = std::move(creator);
}

void TextEditorFactory::setIndenterCreator(IndenterCreator &&creator)
{
    d->m_indenterCreator = std::move(creator);
}

void TextEditorFactory::setSyntaxHighlighterCreator(SyntaxHighLighterCreator &&creator)
{
    d->m_syntaxHighlighterCreator = std::move(creator);
}

void TextEditorFactory::setUseGenericHighlighter(bool enabled)
{
    d->m_useGenericHighlighter = enabled;
}

void TextEditorFactory::setAutoCompleterCreator(AutoCompleterCreator &&creator)
{
    d->m_autoCompleterCreator = std::move(creator);
}

void TextEditorFactory::setEditorActionHandlers(Id contextId, uint optionalActions)
{
    new TextEditorActionHandler(this, contextId, optionalActions);
}

void TextEditorFactory::setEditorActionHandlers(uint optionalActions)
{
    new TextEditorActionHandler(this, id(), optionalActions);
}

void TextEditorFactory::addHoverHandler(BaseHoverHandler *handler)
{
    d->m_hoverHandlers.append(handler);
}

void TextEditorFactory::setCompletionAssistProvider(CompletionAssistProvider *provider)
{
    d->m_completionAssistProvider = provider;
}

void TextEditorFactory::setCommentStyle(CommentDefinition::Style style)
{
    d->m_commentStyle = style;
}

void TextEditorFactory::setDuplicatedSupported(bool on)
{
    d->m_duplicatedSupported = on;
}

void TextEditorFactory::setMarksVisible(bool on)
{
    d->m_marksVisible = on;
}

void TextEditorFactory::setCodeFoldingSupported(bool on)
{
    d->m_codeFoldingSupported = on;
}

void TextEditorFactory::setParenthesesMatchingEnabled(bool on)
{
    d->m_paranthesesMatchinEnabled = on;
}

IEditor *TextEditorFactory::createEditor()
{
    TextDocumentPtr doc(d->m_documentCreator());

    if (d->m_indenterCreator)
        doc->setIndenter(d->m_indenterCreator());

    if (d->m_syntaxHighlighterCreator)
        doc->setSyntaxHighlighter(d->m_syntaxHighlighterCreator());

    doc->setCompletionAssistProvider(d->m_completionAssistProvider);

    return d->createEditorHelper(doc);
}

BaseTextEditor *TextEditorFactoryPrivate::createEditorHelper(const TextDocumentPtr &document)
{
    TextEditorWidget *widget = m_widgetCreator();
    widget->setMarksVisible(m_marksVisible);
    widget->setParenthesesMatchingEnabled(m_paranthesesMatchinEnabled);
    widget->setCodeFoldingSupported(m_codeFoldingSupported);

    BaseTextEditor *editor = m_editorCreator();
    editor->setDuplicateSupported(m_duplicatedSupported);
    editor->addContext(q->id());
    editor->d->m_origin = this;

    editor->m_widget = widget;

    // Needs to go before setTextDocument as this copies the current settings.
    if (m_autoCompleterCreator)
        widget->setAutoCompleter(m_autoCompleterCreator());

    widget->setTextDocument(document);
    widget->d->m_hoverHandlers = m_hoverHandlers;

    widget->d->m_codeAssistant.configure(widget);
    widget->d->m_commentDefinition.setStyle(m_commentStyle);

    QObject::connect(widget, &TextEditorWidget::activateEditor,
                     [editor]() { EditorManager::activateEditor(editor); });

    if (m_useGenericHighlighter)
        widget->setupGenericHighlighter();
    widget->finalizeInitialization();
    editor->finalizeInitialization();

    QObject::connect(widget->d->m_cursorPositionLabel, &LineColumnLabel::clicked, [editor] {
        EditorManager::activateEditor(editor, EditorManager::IgnoreNavigationHistory);
        if (Command *cmd = ActionManager::command(Core::Constants::GOTO)) {
            if (QAction *act = cmd->action())
                act->trigger();
        }
    });
    return editor;
}

IEditor *BaseTextEditor::duplicate()
{
    // Use new standard setup if that's available.
    if (d->m_origin)
        return d->m_origin->duplicateTextEditor(this);

    // If neither is sufficient, you need to implement 'YourEditor::duplicate'.
    QTC_CHECK(false);
    return 0;
}


} // namespace TextEditor

#include "texteditor.moc"
