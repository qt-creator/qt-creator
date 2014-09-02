/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "basetexteditor.h"
#include "basetexteditor_p.h"

#include "basetextdocument.h"
#include "basetextdocumentlayout.h"
#include "behaviorsettings.h"
#include "codecselector.h"
#include "completionsettings.h"
#include "snippets/snippet.h"
#include "tabsettings.h"
#include "typingsettings.h"
#include "icodestylepreferences.h"
#include "syntaxhighlighter.h"
#include "indenter.h"
#include "autocompleter.h"
#include "convenience.h"
#include "texteditorsettings.h"
#include "texteditoroverlay.h"
#include "circularclipboard.h"
#include "circularclipboardassist.h"
#include "highlighterutils.h"
#include "basetexteditor.h"

#include <texteditor/codeassist/codeassistant.h>
#include <texteditor/codeassist/defaultassistinterface.h>
#include <texteditor/generichighlighter/context.h>
#include <texteditor/generichighlighter/highlightdefinition.h>
#include <texteditor/generichighlighter/highlighter.h>
#include <texteditor/generichighlighter/highlightersettings.h>
#include <texteditor/generichighlighter/manager.h>

#include <coreplugin/icore.h>
#include <aggregation/aggregate.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/infobar.h>
#include <coreplugin/manhattanstyle.h>
#include <coreplugin/find/basetextfind.h>
#include <utils/linecolumnlabel.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>
#include <utils/tooltip/tooltip.h>
#include <utils/tooltip/tipcontents.h>
#include <utils/uncommentselection.h>

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

typedef QString (TransformationMethod)(const QString &);

static QString QString_toUpper(const QString &str)
{
    return str.toUpper();
}

static QString QString_toLower(const QString &str)
{
    return str.toLower();
}

class BaseTextEditorAnimator : public QObject
{
    Q_OBJECT

public:
    BaseTextEditorAnimator(QObject *parent);

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
    TextEditExtraArea(BaseTextEditorWidget *edit)
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
    BaseTextEditorWidget *textEdit;
};

class BaseTextEditorPrivate
{
public:
    BaseTextEditorPrivate() {}

    std::function<CompletionAssistProvider *()> m_completionAssistProvider;

    QPointer<BaseTextEditorFactory> m_origin;
};

class BaseTextEditorWidgetPrivate : public QObject
{
public:
    BaseTextEditorWidgetPrivate(BaseTextEditorWidget *parent);
    ~BaseTextEditorWidgetPrivate()
    {
        delete m_toolBar;
        if (m_editorIsFallBack) {
            m_editor->m_widget = 0; // That's us.
            delete m_editor;
        }
    }

    void setupDocumentSignals();
    void updateLineSelectionColor();

    void print(QPrinter *printer);

    void maybeSelectLine();
    void updateCannotDecodeInfo();
    void collectToCircularClipboard();

    void ctor(const QSharedPointer<BaseTextDocument> &doc);
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
    QRect foldBox();

    QTextBlock foldedBlockAt(const QPoint &pos, QRect *box = 0) const;

    void updateLink(QMouseEvent *e);
    void showLink(const BaseTextEditorWidget::Link &);
    void clearLink();

    void universalHelper(); // test function for development

    bool cursorMoveKeyEvent(QKeyEvent *e);
    bool camelCaseRight(QTextCursor &cursor, QTextCursor::MoveMode mode);
    bool camelCaseLeft(QTextCursor &cursor, QTextCursor::MoveMode mode);

    void processTooltipRequest(const QTextCursor &c);

    void transformSelection(Internal::TransformationMethod method);
    void transformBlockSelection(Internal::TransformationMethod method);

    bool inFindScope(const QTextCursor &cursor);
    bool inFindScope(int selectionStart, int selectionEnd);

    void slotUpdateExtraAreaWidth();
    void slotUpdateRequest(const QRect &r, int dy);
    void slotUpdateBlockNotify(const QTextBlock &);
    void updateTabStops();
    void applyFontSettingsDelayed();

    void editorContentsChange(int position, int charsRemoved, int charsAdded);
    void documentAboutToBeReloaded();
    void documentReloadFinished(bool success);
    void highlightSearchResultsSlot(const QString &txt, FindFlags findFlags);
    void setFindScope(const QTextCursor &start, const QTextCursor &end, int, int);

    void updateCursorPosition();

    // parentheses matcher
    void _q_matchParentheses();
    void _q_highlightBlocks();
    void slotSelectionChanged();
    void _q_animateUpdate(int position, QPointF lastPos, QRectF rect);
    void updateCodeFoldingVisible();

public:
    BaseTextEditorWidget *q;
    QToolBar *m_toolBar;
    QWidget *m_stretchWidget;
    LineColumnLabel *m_cursorPositionLabel;
    LineColumnLabel *m_fileEncodingLabel;
    QAction *m_cursorPositionLabelAction;
    QAction *m_fileEncodingLabelAction;

    bool m_contentsChanged;
    bool m_lastCursorChangeWasInteresting;

    QSharedPointer<BaseTextDocument> m_document;
    QByteArray m_tempState;
    QByteArray m_tempNavigationState;

    bool m_parenthesesMatchingEnabled;

    // parentheses matcher
    bool m_formatRange;
    QTextCharFormat m_mismatchFormat;
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

    BaseTextEditorWidget::Link m_currentLink;
    bool m_linkPressed;

    QRegExp m_searchExpr;
    FindFlags m_findFlags;
    void highlightSearchResults(const QTextBlock &block, TextEditorOverlay *overlay);
    QTimer m_delayedUpdateTimer;

    BaseTextEditor *m_editor;

    QList<QTextEdit::ExtraSelection> m_extraSelections[BaseTextEditorWidget::NExtraSelectionKinds];

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

    Internal::BaseTextBlockSelection m_blockSelection;

    void moveCursorVisible(bool ensureVisible = true);

    int visualIndent(const QTextBlock &block) const;
    BaseTextEditorPrivateHighlightBlocks m_highlightBlocksInfo;
    QTimer m_highlightBlocksTimer;

    CodeAssistant m_codeAssistant;
    bool m_assistRelevantContentAdded;

    QPointer<BaseTextEditorAnimator> m_animator;
    int m_cursorBlockNumber;
    int m_blockCount;

    QPoint m_markDragStart;
    bool m_markDragging;

    QScopedPointer<Internal::ClipboardAssistProvider> m_clipboardAssistProvider;

    bool m_isMissingSyntaxDefinition;
    bool m_editorIsFallBack;

    QScopedPointer<AutoCompleter> m_autoCompleter;
    CommentDefinition m_commentDefinition;
};

BaseTextEditorWidgetPrivate::BaseTextEditorWidgetPrivate(BaseTextEditorWidget *parent)
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
    m_editor(0),
    m_inBlockSelectionMode(false),
    m_moveLineUndoHack(false),
    m_findScopeVerticalBlockSelectionFirstColumn(-1),
    m_findScopeVerticalBlockSelectionLastColumn(-1),
    m_highlightBlocksTimer(0),
    m_assistRelevantContentAdded(false),
    m_cursorBlockNumber(-1),
    m_blockCount(0),
    m_markDragging(false),
    m_clipboardAssistProvider(new Internal::ClipboardAssistProvider),
    m_isMissingSyntaxDefinition(false),
    m_editorIsFallBack(false),
    m_autoCompleter(new AutoCompleter)
{
    Aggregation::Aggregate *aggregate = new Aggregation::Aggregate;
    BaseTextFind *baseTextFind = new BaseTextFind(q);
    connect(baseTextFind, &BaseTextFind::highlightAllRequested,
            this, &BaseTextEditorWidgetPrivate::highlightSearchResultsSlot);
    connect(baseTextFind, &BaseTextFind::findScopeChanged,
            this, &BaseTextEditorWidgetPrivate::setFindScope);
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
}

} // namespace Internal

using namespace Internal;

/*!
 * Test if syntax highlighter is available (or unneeded) for \a widget.
 * If not found, show a warning with a link to the relevant settings page.
 */
static void updateEditorInfoBar(BaseTextEditorWidget *widget)
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
        info.setCustomButtonInfo(BaseTextEditor::tr("Show Highlighter Options..."),
                                 widget, SLOT(acceptMissingSyntaxDefinitionInfo()));
        infoBar->addInfo(info);
    }
}

QString BaseTextEditorWidget::plainTextFromSelection(const QTextCursor &cursor) const
{
    // Copy the selected text as plain text
    QString text = cursor.selectedText();
    return convertToPlainText(text);
}

QString BaseTextEditorWidget::convertToPlainText(const QString &txt)
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

BaseTextEditorWidget::BaseTextEditorWidget(QWidget *parent)
    : QPlainTextEdit(parent)
{
    // "Needed", as the creation below triggers ChildEvents that are
    // passed to this object's event() which uses 'd'.
    d = 0;
    d = new BaseTextEditorWidgetPrivate(this);
}

void BaseTextEditorWidget::setTextDocument(const QSharedPointer<BaseTextDocument> &doc)
{
    d->ctor(doc);
}

void BaseTextEditorWidgetPrivate::ctor(const QSharedPointer<BaseTextDocument> &doc)
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
                     q, &BaseTextEditorWidget::assistFinished);

    QObject::connect(q, &QPlainTextEdit::blockCountChanged,
                     this, &BaseTextEditorWidgetPrivate::slotUpdateExtraAreaWidth);

    QObject::connect(q, &QPlainTextEdit::modificationChanged, m_extraArea,
                     static_cast<void (QWidget::*)()>(&QWidget::update));

    QObject::connect(q, &QPlainTextEdit::cursorPositionChanged,
                     q, &BaseTextEditorWidget::slotCursorPositionChanged);

    QObject::connect(q, &QPlainTextEdit::cursorPositionChanged,
                     this, &BaseTextEditorWidgetPrivate::updateCursorPosition);

    QObject::connect(q, &QPlainTextEdit::updateRequest,
                     this, &BaseTextEditorWidgetPrivate::slotUpdateRequest);

    QObject::connect(q, &QPlainTextEdit::selectionChanged,
                     this, &BaseTextEditorWidgetPrivate::slotSelectionChanged);

//     (void) new QShortcut(tr("CTRL+L"), this, SLOT(centerCursor()), 0, Qt::WidgetShortcut);
//     (void) new QShortcut(tr("F9"), this, SLOT(slotToggleMark()), 0, Qt::WidgetShortcut);
//     (void) new QShortcut(tr("F11"), this, SLOT(slotToggleBlockVisible()));

#ifdef DO_FOO
    (void) new QShortcut(tr("CTRL+D"), this, SLOT(doFoo()));
#endif

    // parentheses matcher
    m_formatRange = true;
    m_mismatchFormat.setBackground(q->palette().color(QPalette::Base).value() < 128
                                      ? Qt::darkMagenta : Qt::magenta);
    m_parenthesesMatchingTimer.setSingleShot(true);
    QObject::connect(&m_parenthesesMatchingTimer, &QTimer::timeout,
                     this, &BaseTextEditorWidgetPrivate::_q_matchParentheses);

    m_highlightBlocksTimer.setSingleShot(true);
    QObject::connect(&m_highlightBlocksTimer, &QTimer::timeout,
                     this, &BaseTextEditorWidgetPrivate::_q_highlightBlocks);

    m_animator = 0;

    slotUpdateExtraAreaWidth();
    updateHighlights();
    q->setFrameStyle(QFrame::NoFrame);

    m_delayedUpdateTimer.setSingleShot(true);
    QObject::connect(&m_delayedUpdateTimer, &QTimer::timeout, q->viewport(),
                     static_cast<void (QWidget::*)()>(&QWidget::update));

    m_moveLineUndoHack = false;
}

BaseTextEditorWidget::~BaseTextEditorWidget()
{
    delete d;
    d = 0;
}

void BaseTextEditorWidget::print(QPrinter *printer)
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

void BaseTextEditorWidgetPrivate::print(QPrinter *printer)
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


int BaseTextEditorWidgetPrivate::visualIndent(const QTextBlock &block) const
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

void BaseTextEditorWidget::selectEncoding()
{
    BaseTextDocument *doc = d->m_document.data();
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

void BaseTextEditorWidget::updateTextCodecLabel()
{
    QString text = QString::fromLatin1(d->m_document->codec()->name());
    d->m_fileEncodingLabel->setText(text, text);
}

QString BaseTextEditorWidget::msgTextTooLarge(quint64 size)
{
    return tr("The text is too large to be displayed (%1 MB).").
           arg(size >> 20);
}

void BaseTextEditorWidget::insertPlainText(const QString &text)
{
    if (d->m_inBlockSelectionMode)
        d->insertIntoBlockSelection(text);
    else
        QPlainTextEdit::insertPlainText(text);
}

QString BaseTextEditorWidget::selectedText() const
{
    if (d->m_inBlockSelectionMode)
        return d->copyBlockSelection();
    else
        return textCursor().selectedText();
}

void BaseTextEditorWidgetPrivate::updateCannotDecodeInfo()
{
    q->setReadOnly(m_document->hasDecodingError());
    InfoBar *infoBar = m_document->infoBar();
    Id selectEncodingId(Constants::SELECT_ENCODING);
    if (m_document->hasDecodingError()) {
        if (!infoBar->canInfoBeAdded(selectEncodingId))
            return;
        InfoBarEntry info(selectEncodingId,
            BaseTextEditorWidget::tr("<b>Error:</b> Could not decode \"%1\" with \"%2\"-encoding. Editing not possible.")
            .arg(m_document->displayName()).arg(QString::fromLatin1(m_document->codec()->name())));
        info.setCustomButtonInfo(BaseTextEditorWidget::tr("Select Encoding"), q, SLOT(selectEncoding()));
        infoBar->addInfo(info);
    } else {
        infoBar->removeInfo(selectEncodingId);
    }
}

bool BaseTextEditorWidget::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    if (d->m_document->open(errorString, fileName, realFileName)) {
        moveCursor(QTextCursor::Start);
        d->updateCannotDecodeInfo();
        if (d->m_fileEncodingLabel) {
            connect(d->m_fileEncodingLabel, &LineColumnLabel::clicked,
                    this, &BaseTextEditorWidget::selectEncoding, Qt::UniqueConnection);
            connect(d->m_document->document(), &QTextDocument::modificationChanged,
                    this, &BaseTextEditorWidget::updateTextCodecLabel, Qt::UniqueConnection);
            updateTextCodecLabel();
        }
        return true;
    }
    return false;
}

/*
  Collapses the first comment in a file, if there is only whitespace above
  */
void BaseTextEditorWidgetPrivate::foldLicenseHeader()
{
    QTextDocument *doc = q->document();
    BaseTextDocumentLayout *documentLayout = qobject_cast<BaseTextDocumentLayout*>(doc->documentLayout());
    QTC_ASSERT(documentLayout, return);
    QTextBlock block = doc->firstBlock();
    while (block.isValid() && block.isVisible()) {
        QString text = block.text();
        if (BaseTextDocumentLayout::canFold(block) && block.next().isVisible()) {
            if (text.trimmed().startsWith(QLatin1String("/*"))) {
                BaseTextDocumentLayout::doFoldOrUnfold(block, false);
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

BaseTextDocument *BaseTextEditorWidget::textDocument() const
{
    return d->m_document.data();
}

BaseTextDocumentPtr BaseTextEditorWidget::textDocumentPtr() const
{
    return d->m_document;
}

void BaseTextEditorWidgetPrivate::editorContentsChange(int position, int charsRemoved, int charsAdded)
{
    if (m_animator)
        m_animator->finish();

    m_contentsChanged = true;
    QTextDocument *doc = q->document();
    BaseTextDocumentLayout *documentLayout = static_cast<BaseTextDocumentLayout*>(doc->documentLayout());
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
}

void BaseTextEditorWidgetPrivate::slotSelectionChanged()
{
    if (!q->textCursor().hasSelection() && !m_selectBlockAnchor.isNull())
        m_selectBlockAnchor = QTextCursor();
    // Clear any link which might be showing when the selection changes
    clearLink();
}

void BaseTextEditorWidget::gotoBlockStart()
{
    QTextCursor cursor = textCursor();
    if (TextBlockUserData::findPreviousOpenParenthesis(&cursor, false)) {
        setTextCursor(cursor);
        d->_q_matchParentheses();
    }
}

void BaseTextEditorWidget::gotoBlockEnd()
{
    QTextCursor cursor = textCursor();
    if (TextBlockUserData::findNextClosingParenthesis(&cursor, false)) {
        setTextCursor(cursor);
        d->_q_matchParentheses();
    }
}

void BaseTextEditorWidget::gotoBlockStartWithSelection()
{
    QTextCursor cursor = textCursor();
    if (TextBlockUserData::findPreviousOpenParenthesis(&cursor, true)) {
        setTextCursor(cursor);
        d->_q_matchParentheses();
    }
}

void BaseTextEditorWidget::gotoBlockEndWithSelection()
{
    QTextCursor cursor = textCursor();
    if (TextBlockUserData::findNextClosingParenthesis(&cursor, true)) {
        setTextCursor(cursor);
        d->_q_matchParentheses();
    }
}

void BaseTextEditorWidget::gotoLineStart()
{
    d->handleHomeKey(false);
}

void BaseTextEditorWidget::gotoLineStartWithSelection()
{
    d->handleHomeKey(true);
}

void BaseTextEditorWidget::gotoLineEnd()
{
    moveCursor(QTextCursor::EndOfLine);
}

void BaseTextEditorWidget::gotoLineEndWithSelection()
{
    moveCursor(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
}

void BaseTextEditorWidget::gotoNextLine()
{
    moveCursor(QTextCursor::Down);
}

void BaseTextEditorWidget::gotoNextLineWithSelection()
{
    moveCursor(QTextCursor::Down, QTextCursor::KeepAnchor);
}

void BaseTextEditorWidget::gotoPreviousLine()
{
    moveCursor(QTextCursor::Up);
}

void BaseTextEditorWidget::gotoPreviousLineWithSelection()
{
    moveCursor(QTextCursor::Up, QTextCursor::KeepAnchor);
}

void BaseTextEditorWidget::gotoPreviousCharacter()
{
    moveCursor(QTextCursor::PreviousCharacter);
}

void BaseTextEditorWidget::gotoPreviousCharacterWithSelection()
{
    moveCursor(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
}

void BaseTextEditorWidget::gotoNextCharacter()
{
    moveCursor(QTextCursor::NextCharacter);
}

void BaseTextEditorWidget::gotoNextCharacterWithSelection()
{
    moveCursor(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
}

void BaseTextEditorWidget::gotoPreviousWord()
{
    moveCursor(QTextCursor::PreviousWord);
    setTextCursor(textCursor());
}

void BaseTextEditorWidget::gotoPreviousWordWithSelection()
{
    moveCursor(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
    setTextCursor(textCursor());
}

void BaseTextEditorWidget::gotoNextWord()
{
    moveCursor(QTextCursor::NextWord);
    setTextCursor(textCursor());
}

void BaseTextEditorWidget::gotoNextWordWithSelection()
{
    moveCursor(QTextCursor::NextWord, QTextCursor::KeepAnchor);
    setTextCursor(textCursor());
}

void BaseTextEditorWidget::gotoPreviousWordCamelCase()
{
    QTextCursor c = textCursor();
    d->camelCaseLeft(c, QTextCursor::MoveAnchor);
    setTextCursor(c);
}

void BaseTextEditorWidget::gotoPreviousWordCamelCaseWithSelection()
{
    QTextCursor c = textCursor();
    d->camelCaseLeft(c, QTextCursor::KeepAnchor);
    setTextCursor(c);
}

void BaseTextEditorWidget::gotoNextWordCamelCase()
{
    QTextCursor c = textCursor();
    d->camelCaseRight(c, QTextCursor::MoveAnchor);
    setTextCursor(c);
}

void BaseTextEditorWidget::gotoNextWordCamelCaseWithSelection()
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

bool BaseTextEditorWidget::selectBlockUp()
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

bool BaseTextEditorWidget::selectBlockDown()
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

void BaseTextEditorWidget::copyLineUp()
{
    d->copyLineUpDown(true);
}

void BaseTextEditorWidget::copyLineDown()
{
    d->copyLineUpDown(false);
}

// @todo: Potential reuse of some code around the following functions...
void BaseTextEditorWidgetPrivate::copyLineUpDown(bool up)
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

void BaseTextEditorWidget::joinLines()
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

void BaseTextEditorWidget::insertLineAbove()
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

void BaseTextEditorWidget::insertLineBelow()
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

void BaseTextEditorWidget::moveLineUp()
{
    d->moveLineUpDown(true);
}

void BaseTextEditorWidget::moveLineDown()
{
    d->moveLineUpDown(false);
}

void BaseTextEditorWidget::uppercaseSelection()
{
    d->transformSelection(&QString_toUpper);
}

void BaseTextEditorWidget::lowercaseSelection()
{
    d->transformSelection(&QString_toLower);
}

void BaseTextEditorWidget::indent()
{
    setTextCursor(textDocument()->indent(textCursor()));
}

void BaseTextEditorWidget::unindent()
{
    setTextCursor(textDocument()->unindent(textCursor()));
}

void BaseTextEditorWidget::undo()
{
    if (d->m_inBlockSelectionMode)
        d->disableBlockSelection(false);
    QPlainTextEdit::undo();
}

void BaseTextEditorWidget::redo()
{
    if (d->m_inBlockSelectionMode)
        d->disableBlockSelection(false);
    QPlainTextEdit::redo();
}

void BaseTextEditorWidget::openLinkUnderCursor()
{
    const bool openInNextSplit = alwaysOpenLinksInNextSplit();
    Link symbolLink = findLinkAt(textCursor(), true, openInNextSplit);
    openLink(symbolLink, openInNextSplit);
}

void BaseTextEditorWidget::openLinkUnderCursorInNextSplit()
{
    const bool openInNextSplit = !alwaysOpenLinksInNextSplit();
    Link symbolLink = findLinkAt(textCursor(), true, openInNextSplit);
    openLink(symbolLink, openInNextSplit);
}

void BaseTextEditorWidget::abortAssist()
{
    d->m_codeAssistant.destroyContext();
}

void BaseTextEditorWidgetPrivate::moveLineUpDown(bool up)
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

void BaseTextEditorWidget::cleanWhitespace()
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

bool BaseTextEditorWidgetPrivate::camelCaseLeft(QTextCursor &cursor, QTextCursor::MoveMode mode)
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

bool BaseTextEditorWidgetPrivate::camelCaseRight(QTextCursor &cursor, QTextCursor::MoveMode mode)
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

bool BaseTextEditorWidgetPrivate::cursorMoveKeyEvent(QKeyEvent *e)
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

void BaseTextEditorWidget::viewPageUp()
{
    verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepSub);
}

void BaseTextEditorWidget::viewPageDown()
{
    verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepAdd);
}

void BaseTextEditorWidget::viewLineUp()
{
    verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
}

void BaseTextEditorWidget::viewLineDown()
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

void BaseTextEditorWidget::keyPressEvent(QKeyEvent *e)
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
        setTextCursor(d->m_blockSelection.selection(d->m_document.data()), true);
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
            if (e->key() == Qt::Key_Tab)
                indent();
            else
                unindent();
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
            setTextCursor(d->m_blockSelection.selection(d->m_document.data()), true);
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

void BaseTextEditorWidget::insertCodeSnippet(const QTextCursor &cursor_arg, const QString &snippet)
{
    Snippet::ParsedSnippet data = Snippet::parse(snippet);

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

    setExtraSelections(BaseTextEditorWidget::SnippetPlaceholderSelection, selections);
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

void BaseTextEditorWidgetPrivate::universalHelper()
{
    // Test function for development. Place your new fangled experiment here to
    // give it proper scrutiny before pushing it onto others.
}

void BaseTextEditorWidget::setTextCursor(const QTextCursor &cursor, bool keepBlockSelection)
{
    // workaround for QTextControl bug
    bool selectionChange = cursor.hasSelection() || textCursor().hasSelection();
    if (!keepBlockSelection && d->m_inBlockSelectionMode)
        d->disableBlockSelection(false);
    QTextCursor c = cursor;
    c.setVisualNavigation(true);
    QPlainTextEdit::setTextCursor(c);
    if (selectionChange)
        d->slotSelectionChanged();
}

void BaseTextEditorWidget::setTextCursor(const QTextCursor &cursor)
{
    setTextCursor(cursor, false);
}

void BaseTextEditorWidget::gotoLine(int line, int column, bool centerLine)
{
    d->m_lastCursorChangeWasInteresting = false; // avoid adding the previous position to history
    const int blockNumber = line - 1;
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

int BaseTextEditorWidget::position(BaseTextEditor::PositionOperation posOp, int at) const
{
    QTextCursor tc = textCursor();

    if (at != -1)
        tc.setPosition(at);

    if (posOp == BaseTextEditor::Current)
        return tc.position();

    switch (posOp) {
    case BaseTextEditor::EndOfLine:
        tc.movePosition(QTextCursor::EndOfLine);
        return tc.position();
    case BaseTextEditor::StartOfLine:
        tc.movePosition(QTextCursor::StartOfLine);
        return tc.position();
    case BaseTextEditor::Anchor:
        if (tc.hasSelection())
            return tc.anchor();
        break;
    case BaseTextEditor::EndOfDoc:
        tc.movePosition(QTextCursor::End);
        return tc.position();
    default:
        break;
    }

    return -1;
}

void BaseTextEditorWidget::convertPosition(int pos, int *line, int *column) const
{
    Convenience::convertPosition(document(), pos, line, column);
}

bool BaseTextEditorWidget::event(QEvent *e)
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
        break;
    default:
        break;
    }

    return QPlainTextEdit::event(e);
}

void BaseTextEditorWidget::inputMethodEvent(QInputMethodEvent *e)
{
    if (d->m_inBlockSelectionMode) {
        if (!e->commitString().isEmpty())
            d->insertIntoBlockSelection(e->commitString());
        return;
    }
    QPlainTextEdit::inputMethodEvent(e);
}

void BaseTextEditorWidgetPrivate::documentAboutToBeReloaded()
{
    //memorize cursor position
    m_tempState = q->saveState();

    // remove extra selections (loads of QTextCursor objects)

    for (int i = 0; i < BaseTextEditorWidget::NExtraSelectionKinds; ++i)
        m_extraSelections[i].clear();
    q->QPlainTextEdit::setExtraSelections(QList<QTextEdit::ExtraSelection>());

    // clear all overlays
    m_overlay->clear();
    m_snippetOverlay->clear();
    m_searchResultOverlay->clear();
    m_refactorOverlay->clear();
}

void BaseTextEditorWidgetPrivate::documentReloadFinished(bool success)
{
    if (!success)
        return;

    // restore cursor position
    q->restoreState(m_tempState);
    updateCannotDecodeInfo();
}

QByteArray BaseTextEditorWidget::saveState() const
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

bool BaseTextEditorWidget::restoreState(const QByteArray &state)
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
                BaseTextDocumentLayout::doFoldOrUnfold(block, false);
                layoutChanged = true;
            }
        }
        if (layoutChanged) {
            BaseTextDocumentLayout *documentLayout =
                    qobject_cast<BaseTextDocumentLayout*>(doc->documentLayout());
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

void BaseTextEditorWidget::setParenthesesMatchingEnabled(bool b)
{
    d->m_parenthesesMatchingEnabled = b;
}

bool BaseTextEditorWidget::isParenthesesMatchingEnabled() const
{
    return d->m_parenthesesMatchingEnabled;
}

void BaseTextEditorWidget::setHighlightCurrentLine(bool b)
{
    d->m_highlightCurrentLine = b;
    d->updateCurrentLineHighlight();
}

bool BaseTextEditorWidget::highlightCurrentLine() const
{
    return d->m_highlightCurrentLine;
}

void BaseTextEditorWidget::setLineNumbersVisible(bool b)
{
    d->m_lineNumbersVisible = b;
    d->slotUpdateExtraAreaWidth();
}

bool BaseTextEditorWidget::lineNumbersVisible() const
{
    return d->m_lineNumbersVisible;
}

void BaseTextEditorWidget::setAlwaysOpenLinksInNextSplit(bool b)
{
    d->m_displaySettings.m_openLinksInNextSplit = b;
}

bool BaseTextEditorWidget::alwaysOpenLinksInNextSplit() const
{
    return d->m_displaySettings.m_openLinksInNextSplit;
}

void BaseTextEditorWidget::setMarksVisible(bool b)
{
    d->m_marksVisible = b;
    d->slotUpdateExtraAreaWidth();
}

bool BaseTextEditorWidget::marksVisible() const
{
    return d->m_marksVisible;
}

void BaseTextEditorWidget::setRequestMarkEnabled(bool b)
{
    d->m_requestMarkEnabled = b;
}

bool BaseTextEditorWidget::requestMarkEnabled() const
{
    return d->m_requestMarkEnabled;
}

void BaseTextEditorWidget::setLineSeparatorsAllowed(bool b)
{
    d->m_lineSeparatorsAllowed = b;
}

bool BaseTextEditorWidget::lineSeparatorsAllowed() const
{
    return d->m_lineSeparatorsAllowed;
}

void BaseTextEditorWidgetPrivate::updateCodeFoldingVisible()
{
    const bool visible = m_codeFoldingSupported && m_displaySettings.m_displayFoldingMarkers;
    if (m_codeFoldingVisible != visible) {
        m_codeFoldingVisible = visible;
        slotUpdateExtraAreaWidth();
    }
}

bool BaseTextEditorWidget::codeFoldingVisible() const
{
    return d->m_codeFoldingVisible;
}

/**
 * Sets whether code folding is supported by the syntax highlighter. When not
 * supported (the default), this makes sure the code folding is not shown.
 *
 * Needs to be called before calling setCodeFoldingVisible.
 */
void BaseTextEditorWidget::setCodeFoldingSupported(bool b)
{
    d->m_codeFoldingSupported = b;
    d->updateCodeFoldingVisible();
}

bool BaseTextEditorWidget::codeFoldingSupported() const
{
    return d->m_codeFoldingSupported;
}

void BaseTextEditorWidget::setMouseNavigationEnabled(bool b)
{
    d->m_behaviorSettings.m_mouseNavigation = b;
}

bool BaseTextEditorWidget::mouseNavigationEnabled() const
{
    return d->m_behaviorSettings.m_mouseNavigation;
}

void BaseTextEditorWidget::setMouseHidingEnabled(bool b)
{
    d->m_behaviorSettings.m_mouseHiding = b;
}

bool BaseTextEditorWidget::mouseHidingEnabled() const
{
    return d->m_behaviorSettings.m_mouseHiding;
}

void BaseTextEditorWidget::setScrollWheelZoomingEnabled(bool b)
{
    d->m_behaviorSettings.m_scrollWheelZooming = b;
}

bool BaseTextEditorWidget::scrollWheelZoomingEnabled() const
{
    return d->m_behaviorSettings.m_scrollWheelZooming;
}

void BaseTextEditorWidget::setConstrainTooltips(bool b)
{
    d->m_behaviorSettings.m_constrainHoverTooltips = b;
}

bool BaseTextEditorWidget::constrainTooltips() const
{
    return d->m_behaviorSettings.m_constrainHoverTooltips;
}

void BaseTextEditorWidget::setCamelCaseNavigationEnabled(bool b)
{
    d->m_behaviorSettings.m_camelCaseNavigation = b;
}

bool BaseTextEditorWidget::camelCaseNavigationEnabled() const
{
    return d->m_behaviorSettings.m_camelCaseNavigation;
}

void BaseTextEditorWidget::setRevisionsVisible(bool b)
{
    d->m_revisionsVisible = b;
    d->slotUpdateExtraAreaWidth();
}

bool BaseTextEditorWidget::revisionsVisible() const
{
    return d->m_revisionsVisible;
}

void BaseTextEditorWidget::setVisibleWrapColumn(int column)
{
    d->m_visibleWrapColumn = column;
    viewport()->update();
}

int BaseTextEditorWidget::visibleWrapColumn() const
{
    return d->m_visibleWrapColumn;
}

void BaseTextEditorWidget::setAutoCompleter(AutoCompleter *autoCompleter)
{
    d->m_autoCompleter.reset(autoCompleter);
}

AutoCompleter *BaseTextEditorWidget::autoCompleter() const
{
    return d->m_autoCompleter.data();
}

//--------- BaseTextEditorPrivate -----------


void BaseTextEditorWidgetPrivate::setupDocumentSignals()
{
    QTextDocument *doc = m_document->document();
    q->QPlainTextEdit::setDocument(doc);
    q->setCursorWidth(2); // Applies to the document layout

    BaseTextDocumentLayout *documentLayout = qobject_cast<BaseTextDocumentLayout*>(doc->documentLayout());
    QTC_CHECK(documentLayout);

    QObject::connect(documentLayout, &QPlainTextDocumentLayout::updateBlock,
                     this, &BaseTextEditorWidgetPrivate::slotUpdateBlockNotify);

    QObject::connect(documentLayout, &BaseTextDocumentLayout::updateExtraArea,
                     m_extraArea, static_cast<void (QWidget::*)()>(&QWidget::update));

    QObject::connect(q, &BaseTextEditorWidget::requestBlockUpdate,
                     documentLayout, &QPlainTextDocumentLayout::updateBlock);

    QObject::connect(doc, &QTextDocument::contentsChange,
                     this, &BaseTextEditorWidgetPrivate::editorContentsChange);

    QObject::connect(m_document.data(), &BaseTextDocument::aboutToReload,
                     this, &BaseTextEditorWidgetPrivate::documentAboutToBeReloaded);

    QObject::connect(m_document.data(), &BaseTextDocument::reloadFinished,
                     this, &BaseTextEditorWidgetPrivate::documentReloadFinished);

    QObject::connect(m_document.data(), &BaseTextDocument::tabSettingsChanged,
                     this, &BaseTextEditorWidgetPrivate::updateTabStops);

    QObject::connect(m_document.data(), &BaseTextDocument::fontSettingsChanged,
                     this, &BaseTextEditorWidgetPrivate::applyFontSettingsDelayed);

    slotUpdateExtraAreaWidth();

    TextEditorSettings *settings = TextEditorSettings::instance();

    // Connect to settings change signals
    connect(settings, &TextEditorSettings::fontSettingsChanged,
            m_document.data(), &BaseTextDocument::setFontSettings);
    connect(settings, &TextEditorSettings::typingSettingsChanged,
            q, &BaseTextEditorWidget::setTypingSettings);
    connect(settings, &TextEditorSettings::storageSettingsChanged,
            q, &BaseTextEditorWidget::setStorageSettings);
    connect(settings, &TextEditorSettings::behaviorSettingsChanged,
            q, &BaseTextEditorWidget::setBehaviorSettings);
    connect(settings, &TextEditorSettings::marginSettingsChanged,
            q, &BaseTextEditorWidget::setMarginSettings);
    connect(settings, &TextEditorSettings::displaySettingsChanged,
            q, &BaseTextEditorWidget::setDisplaySettings);
    connect(settings, &TextEditorSettings::completionSettingsChanged,
            q, &BaseTextEditorWidget::setCompletionSettings);
    connect(settings, &TextEditorSettings::extraEncodingSettingsChanged,
            q, &BaseTextEditorWidget::setExtraEncodingSettings);

    connect(q, &BaseTextEditorWidget::requestFontZoom,
            settings, &TextEditorSettings::fontZoomRequested);
    connect(q, &BaseTextEditorWidget::requestZoomReset,
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
    q->setCodeStyle(settings->codeStyle(q->languageSettingsId()));
}

bool BaseTextEditorWidgetPrivate::snippetCheckCursor(const QTextCursor &cursor)
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

void BaseTextEditorWidgetPrivate::snippetTabOrBacktab(bool forward)
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
QPoint BaseTextEditorWidget::toolTipPosition(const QTextCursor &c) const
{
    const QPoint cursorPos = mapToGlobal(cursorRect(c).bottomRight() + QPoint(1,1));
    return cursorPos + QPoint(d->m_extraArea->width(), HostOsInfo::isWindowsHost() ? -24 : -16);
}

void BaseTextEditorWidgetPrivate::processTooltipRequest(const QTextCursor &c)
{
    const QPoint toolTipPoint = q->toolTipPosition(c);
    bool handled = false;
    if (!handled)
        emit q->tooltipRequested(toolTipPoint, c.position());
}

bool BaseTextEditorWidget::viewportEvent(QEvent *event)
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
            ToolTip::show(he->globalPos(), TextContent(refactorMarker.tooltip),
                                      viewport(),
                                      refactorMarker.rect);
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


void BaseTextEditorWidget::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);
    QRect cr = rect();
    d->m_extraArea->setGeometry(
        QStyle::visualRect(layoutDirection(), cr,
                           QRect(cr.left(), cr.top(), extraAreaWidth(), cr.height())));
}

QRect BaseTextEditorWidgetPrivate::foldBox()
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

QTextBlock BaseTextEditorWidgetPrivate::foldedBlockAt(const QPoint &pos, QRect *box) const
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

void BaseTextEditorWidgetPrivate::highlightSearchResults(const QTextBlock &block,
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

        if (!inFindScope(blockPosition + idx, blockPosition + idx + l))
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

QString BaseTextEditorWidgetPrivate::copyBlockSelection()
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

void BaseTextEditorWidgetPrivate::setCursorToColumn(QTextCursor &cursor, int column, QTextCursor::MoveMode moveMode)
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

void BaseTextEditorWidgetPrivate::insertIntoBlockSelection(const QString &text)
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
    for (QStringList::iterator textLine = textLines.begin(); textLine != endLine; ++textLine)
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
    q->setTextCursor(m_blockSelection.selection(m_document.data()), true);
}

void BaseTextEditorWidgetPrivate::removeBlockSelection()
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
    q->setTextCursor(cursor, m_blockSelection.hasSelection());
}

void BaseTextEditorWidgetPrivate::enableBlockSelection(const QTextCursor &cursor)
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

void BaseTextEditorWidgetPrivate::enableBlockSelection(int positionBlock, int positionColumn,
                                                       int anchorBlock, int anchorColumn)
{
    m_blockSelection.fromPostition(positionBlock, anchorColumn, anchorBlock, positionColumn);
    resetCursorFlashTimer();
    m_inBlockSelectionMode = true;
    q->setTextCursor(m_blockSelection.selection(m_document.data()), true);
    q->viewport()->update();
}

void BaseTextEditorWidgetPrivate::disableBlockSelection(bool keepSelection)
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

void BaseTextEditorWidgetPrivate::resetCursorFlashTimer()
{
    m_cursorVisible = true;
    const int flashTime = qApp->cursorFlashTime();
    if (flashTime > 0) {
        m_cursorFlashTimer.stop();
        m_cursorFlashTimer.start(flashTime / 2, q);
    }
}

void BaseTextEditorWidgetPrivate::moveCursorVisible(bool ensureVisible)
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

void BaseTextEditorWidget::paintEvent(QPaintEvent *e)
{
    /*
      Here comes an almost verbatim copy of
      QPlainTextEdit::paintEvent() so we can adjust the extra
      selections dynamically to indicate all search results.
    */
    //begin QPlainTextEdit::paintEvent()

    QPainter painter(viewport());
    QTextDocument *doc = document();
    BaseTextDocumentLayout *documentLayout = qobject_cast<BaseTextDocumentLayout*>(doc->documentLayout());
    QTC_ASSERT(documentLayout, return);
    const FontSettings &fs = textDocument()->fontSettings();
    const QTextCharFormat &searchScopeFormat = fs.toTextCharFormat(C_SEARCH_SCOPE);
    const QTextCharFormat &ifdefedOutFormat = fs.toTextCharFormat(C_DISABLED_CODE);

    QPointF offset(contentOffset());
    QTextBlock textCursorBlock = textCursor().block();

    bool hasMainSelection = textCursor().hasSelection();
    bool suppressSyntaxInIfdefedOutBlock = (ifdefedOutFormat.foreground()
                                           != palette().foreground());

    QRect er = e->rect();
    QRect viewportRect = viewport()->rect();

    qreal lineX = 0;

    if (d->m_visibleWrapColumn > 0) {
        // Don't use QFontMetricsF::averageCharWidth here, due to it returning
        // a fractional size even when this is not supported by the platform.
        lineX = QFontMetricsF(font()).width(QLatin1Char('x')) * d->m_visibleWrapColumn + offset.x() + 4;

        if (lineX < viewportRect.width()) {
            const QBrush background = ifdefedOutFormat.background();
            painter.fillRect(QRectF(lineX, er.top(), viewportRect.width() - lineX, er.height()),
                             background);

            const QColor col = (palette().base().color().value() > 128) ? Qt::black : Qt::white;
            const QPen pen = painter.pen();
            painter.setPen(blendColors(background.isOpaque() ? background.color() : palette().base().color(),
                                       col, 32));
            painter.drawLine(QPointF(lineX, er.top()), QPointF(lineX, er.bottom()));
            painter.setPen(pen);
        }
    }

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
                if (BaseTextDocumentLayout::ifdefedOut(blockIDO)) {
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
            if (suppressSyntaxInIfdefedOutBlock && BaseTextDocumentLayout::ifdefedOut(block)) {
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



            layout->draw(&painter, offset, selections, er);

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

                if (TextBlockUserData *nextBlockUserData = BaseTextDocumentLayout::testUserData(nextBlock)) {
                    if (nextBlockUserData->foldingStartIncluded())
                        replacement.prepend(nextBlock.text().trimmed().left(1));
                }

                block = nextVisibleBlock.previous();
                if (!block.isValid())
                    block = doc->lastBlock();

                if (TextBlockUserData *blockUserData = BaseTextDocumentLayout::testUserData(block)) {
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

int BaseTextEditorWidget::visibleFoldedBlockNumber() const
{
    return d->visibleFoldedBlockNumber;
}

void BaseTextEditorWidget::drawCollapsedBlockPopup(QPainter &painter,
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

QWidget *BaseTextEditorWidget::extraArea() const
{
    return d->m_extraArea;
}

int BaseTextEditorWidget::extraAreaWidth(int *markWidthPtr) const
{
    BaseTextDocumentLayout *documentLayout = qobject_cast<BaseTextDocumentLayout*>(document()->documentLayout());
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

void BaseTextEditorWidgetPrivate::slotUpdateExtraAreaWidth()
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

void BaseTextEditorWidget::extraAreaPaintEvent(QPaintEvent *e)
{
    QTextDocument *doc = document();
    BaseTextDocumentLayout *documentLayout = qobject_cast<BaseTextDocumentLayout*>(doc->documentLayout());
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

                TextBlockUserData *nextBlockUserData = BaseTextDocumentLayout::testUserData(nextBlock);

                bool drawBox = nextBlockUserData
                               && BaseTextDocumentLayout::foldingIndent(block) < nextBlockUserData->foldingIndent();



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

        block = nextVisibleBlock;
        blockNumber = nextVisibleBlockNumber;
    }
}

void BaseTextEditorWidgetPrivate::drawFoldingMarker(QPainter *painter, const QPalette &pal,
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

void BaseTextEditorWidgetPrivate::slotUpdateRequest(const QRect &r, int dy)
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

void BaseTextEditorWidgetPrivate::saveCurrentCursorPositionForNavigation()
{
    m_lastCursorChangeWasInteresting = true;
    m_tempNavigationState = q->saveState();
}

void BaseTextEditorWidgetPrivate::updateCurrentLineHighlight()
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

    q->setExtraSelections(BaseTextEditorWidget::CurrentLineSelection, extraSelections);

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

void BaseTextEditorWidget::slotCursorPositionChanged()
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

void BaseTextEditorWidgetPrivate::updateHighlights()
{
    if (m_parenthesesMatchingEnabled && q->hasFocus()) {
        // Delay update when no matching is displayed yet, to avoid flicker
        if (q->extraSelections(BaseTextEditorWidget::ParenthesesMatchingSelection).isEmpty()
            && m_animator == 0) {
            m_parenthesesMatchingTimer.start(50);
        } else {
            // when we uncheck "highlight matching parentheses"
            // we need clear current selection before viewport update
            // otherwise we get sticky highlighted parentheses
            if (!m_displaySettings.m_highlightMatchingParentheses)
                q->setExtraSelections(BaseTextEditorWidget::ParenthesesMatchingSelection, QList<QTextEdit::ExtraSelection>());

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

void BaseTextEditorWidgetPrivate::slotUpdateBlockNotify(const QTextBlock &block)
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

void BaseTextEditorWidget::timerEvent(QTimerEvent *e)
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


void BaseTextEditorWidgetPrivate::clearVisibleFoldedBlock()
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

void BaseTextEditorWidget::mouseMoveEvent(QMouseEvent *e)
{
    d->updateLink(e);

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

                setTextCursor(d->m_blockSelection.selection(d->m_document.data()), true);
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

void BaseTextEditorWidget::mousePressEvent(QMouseEvent *e)
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

                setTextCursor(d->m_blockSelection.selection(d->m_document.data()), true);
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
                d->updateLink(e);

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

void BaseTextEditorWidget::mouseReleaseEvent(QMouseEvent *e)
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

void BaseTextEditorWidget::mouseDoubleClickEvent(QMouseEvent *e)
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

void BaseTextEditorWidget::leaveEvent(QEvent *e)
{
    // Clear link emulation when the mouse leaves the editor
    d->clearLink();
    QPlainTextEdit::leaveEvent(e);
}

void BaseTextEditorWidget::keyReleaseEvent(QKeyEvent *e)
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

void BaseTextEditorWidget::dragEnterEvent(QDragEnterEvent *e)
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

void BaseTextEditorWidget::showDefaultContextMenu(QContextMenuEvent *e, Id menuContextId)
{
    QMenu menu;
    appendMenuActionsFromContext(&menu, menuContextId);
    appendStandardContextMenuActions(&menu);
    menu.exec(e->globalPos());
}

void BaseTextEditorWidget::extraAreaLeaveEvent(QEvent *)
{
    // fake missing mouse move event from Qt
    QMouseEvent me(QEvent::MouseMove, QPoint(-1, -1), Qt::NoButton, 0, 0);
    extraAreaMouseEvent(&me);
}

void BaseTextEditorWidget::extraAreaContextMenuEvent(QContextMenuEvent *e)
{
    if (d->m_marksVisible) {
        QTextCursor cursor = cursorForPosition(QPoint(0, e->pos().y()));
        QMenu * contextMenu = new QMenu(this);
        emit markContextMenuRequested(cursor.blockNumber() + 1, contextMenu);
        if (!contextMenu->isEmpty())
            contextMenu->exec(e->globalPos());
        delete contextMenu;
        e->accept();
    }
}

void BaseTextEditorWidget::updateFoldingHighlight(const QPoint &pos)
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

void BaseTextEditorWidget::extraAreaMouseEvent(QMouseEvent *e)
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
            emit markTooltipRequested(mapToGlobal(e->pos()), line);
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
            BaseTextEditor::MarkRequestKind kind;
            if (QApplication::keyboardModifiers() & Qt::ShiftModifier)
                kind = BaseTextEditor::BookmarkRequest;
            else
                kind = BaseTextEditor::BreakpointRequest;

            emit markRequested(line, kind);
        }
    }
}

void BaseTextEditorWidget::ensureCursorVisible()
{
    QTextBlock block = textCursor().block();
    if (!block.isVisible()) {
        BaseTextDocumentLayout *documentLayout = qobject_cast<BaseTextDocumentLayout*>(document()->documentLayout());
        QTC_ASSERT(documentLayout, return);

        // Open all parent folds of current line.
        int indent = BaseTextDocumentLayout::foldingIndent(block);
        block = block.previous();
        while (block.isValid()) {
            const int indent2 = BaseTextDocumentLayout::foldingIndent(block);
            if (BaseTextDocumentLayout::canFold(block) && indent2 < indent) {
                BaseTextDocumentLayout::doFoldOrUnfold(block, /* unfold = */ true);
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

void BaseTextEditorWidgetPrivate::toggleBlockVisible(const QTextBlock &block)
{
    auto documentLayout = qobject_cast<BaseTextDocumentLayout*>(q->document()->documentLayout());
    QTC_ASSERT(documentLayout, return);

    BaseTextDocumentLayout::doFoldOrUnfold(block, BaseTextDocumentLayout::isFolded(block));
    documentLayout->requestUpdate();
    documentLayout->emitDocumentSizeChanged();
}

void BaseTextEditorWidget::setLanguageSettingsId(Id settingsId)
{
    d->m_tabSettingsId = settingsId;
}

Id BaseTextEditorWidget::languageSettingsId() const
{
    return d->m_tabSettingsId;
}

void BaseTextEditorWidget::setCodeStyle(ICodeStylePreferences *preferences)
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

void BaseTextEditorWidget::slotCodeStyleSettingsChanged(const QVariant &)
{

}

const DisplaySettings &BaseTextEditorWidget::displaySettings() const
{
    return d->m_displaySettings;
}

const MarginSettings &BaseTextEditorWidget::marginSettings() const
{
    return d->m_marginSettings;
}

void BaseTextEditorWidgetPrivate::handleHomeKey(bool anchor)
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

void BaseTextEditorWidgetPrivate::handleBackspaceKey()
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

    const TextEditor::TabSettings &tabSettings = m_document->tabSettings();
    const TextEditor::TypingSettings &typingSettings = m_document->typingSettings();

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

void BaseTextEditorWidget::wheelEvent(QWheelEvent *e)
{
    d->clearVisibleFoldedBlock();
    if (scrollWheelZoomingEnabled() && e->modifiers() & Qt::ControlModifier) {
        const int delta = e->delta();
        if (delta < 0)
            zoomOut();
        else if (delta > 0)
            zoomIn();
        return;
    }
    QPlainTextEdit::wheelEvent(e);
}

void BaseTextEditorWidget::zoomIn()
{
    d->clearVisibleFoldedBlock();
    emit requestFontZoom(10);
}

void BaseTextEditorWidget::zoomOut()
{
    d->clearVisibleFoldedBlock();
    emit requestFontZoom(-10);
}

void BaseTextEditorWidget::zoomReset()
{
    emit requestZoomReset();
}

BaseTextEditorWidget::Link BaseTextEditorWidget::findLinkAt(const QTextCursor &, bool, bool)
{
    return Link();
}

bool BaseTextEditorWidget::openLink(const Link &link, bool inNextSplit)
{
    if (!link.hasValidTarget())
        return false;

    if (inNextSplit) {
        EditorManager::gotoOtherSplit();
    } else if (textDocument()->filePath() == link.targetFileName) {
        EditorManager::addCurrentPositionToNavigationHistory();
        gotoLine(link.targetLine, link.targetColumn);
        setFocus();
        return true;
    }

    return EditorManager::openEditorAt(link.targetFileName, link.targetLine, link.targetColumn);
}

void BaseTextEditorWidgetPrivate::updateLink(QMouseEvent *e)
{
    bool linkFound = false;

    if (q->mouseNavigationEnabled() && e->modifiers() & Qt::ControlModifier) {
        // Link emulation behaviour for 'go to definition'
        const QTextCursor cursor = q->cursorForPosition(e->pos());

        // Check that the mouse was actually on the text somewhere
        bool onText = q->cursorRect(cursor).right() >= e->x();
        if (!onText) {
            QTextCursor nextPos = cursor;
            nextPos.movePosition(QTextCursor::Right);
            onText = q->cursorRect(nextPos).right() >= e->x();
        }

        const BaseTextEditorWidget::Link link = q->findLinkAt(cursor, false);

        if (onText && link.hasValidLinkText()) {
            showLink(link);
            linkFound = true;
        }
    }

    if (!linkFound)
        clearLink();
}

void BaseTextEditorWidgetPrivate::showLink(const BaseTextEditorWidget::Link &link)
{
    if (m_currentLink == link)
        return;

    QTextEdit::ExtraSelection sel;
    sel.cursor = q->textCursor();
    sel.cursor.setPosition(link.linkTextStart);
    sel.cursor.setPosition(link.linkTextEnd, QTextCursor::KeepAnchor);
    sel.format = q->textDocument()->fontSettings().toTextCharFormat(C_LINK);
    sel.format.setFontUnderline(true);
    q->setExtraSelections(BaseTextEditorWidget::OtherSelection, QList<QTextEdit::ExtraSelection>() << sel);
    q->viewport()->setCursor(Qt::PointingHandCursor);
    m_currentLink = link;
    m_linkPressed = false;
}

void BaseTextEditorWidgetPrivate::clearLink()
{
    if (!m_currentLink.hasValidLinkText())
        return;

    q->setExtraSelections(BaseTextEditorWidget::OtherSelection, QList<QTextEdit::ExtraSelection>());
    q->viewport()->setCursor(Qt::IBeamCursor);
    m_currentLink = BaseTextEditorWidget::Link();
    m_linkPressed = false;
}

void BaseTextEditorWidgetPrivate::highlightSearchResultsSlot(const QString &txt, FindFlags findFlags)
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
}

int BaseTextEditorWidget::verticalBlockSelectionFirstColumn() const
{
    return d->m_inBlockSelectionMode ? d->m_blockSelection.firstVisualColumn() : -1;
}

int BaseTextEditorWidget::verticalBlockSelectionLastColumn() const
{
    return d->m_inBlockSelectionMode ? d->m_blockSelection.lastVisualColumn() : -1;
}

QRegion BaseTextEditorWidget::translatedLineRegion(int lineStart, int lineEnd) const
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

void BaseTextEditorWidgetPrivate::setFindScope(const QTextCursor &start, const QTextCursor &end,
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
    }
}

void BaseTextEditorWidgetPrivate::_q_animateUpdate(int position, QPointF lastPos, QRectF rect)
{
    QTextCursor cursor = q->textCursor();
    cursor.setPosition(position);
    q->viewport()->update(QRectF(q->cursorRect(cursor).topLeft() + rect.topLeft(), rect.size()).toAlignedRect());
    if (!lastPos.isNull())
        q->viewport()->update(QRectF(lastPos + rect.topLeft(), rect.size()).toAlignedRect());
}


BaseTextEditorAnimator::BaseTextEditorAnimator(QObject *parent)
    : QObject(parent), m_timeline(256)
{
    m_value = 0;
    m_timeline.setCurveShape(QTimeLine::SineCurve);
    connect(&m_timeline, &QTimeLine::valueChanged, this, &BaseTextEditorAnimator::step);
    connect(&m_timeline, &QTimeLine::finished, this, &QObject::deleteLater);
    m_timeline.start();
}

void BaseTextEditorAnimator::setData(const QFont &f, const QPalette &pal, const QString &text)
{
    m_font = f;
    m_palette = pal;
    m_text = text;
    QFontMetrics fm(m_font);
    m_size = QSizeF(fm.width(m_text), fm.height());
}

void BaseTextEditorAnimator::draw(QPainter *p, const QPointF &pos)
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

bool BaseTextEditorAnimator::isRunning() const
{
    return m_timeline.state() == QTimeLine::Running;
}

QRectF BaseTextEditorAnimator::rect() const
{
    QFont f = m_font;
    f.setPointSizeF(f.pointSizeF() * (1.0 + m_value/2));
    QFontMetrics fm(f);
    int width = fm.width(m_text);
    return QRectF((m_size.width()-width)/2, (m_size.height() - fm.height())/2, width, fm.height());
}

void BaseTextEditorAnimator::step(qreal v)
{
    QRectF before = rect();
    m_value = v;
    QRectF after = rect();
    emit updateRequest(m_position, m_lastDrawPos, before.united(after));
}

void BaseTextEditorAnimator::finish()
{
    m_timeline.stop();
    step(0);
    deleteLater();
}

void BaseTextEditorWidgetPrivate::_q_matchParentheses()
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
        q->setExtraSelections(BaseTextEditorWidget::ParenthesesMatchingSelection, extraSelections); // clear
        return;
    }

    const QTextCharFormat &matchFormat
            = q->textDocument()->fontSettings().toTextCharFormat(C_PARENTHESES);
    int animatePosition = -1;
    if (backwardMatch.hasSelection()) {
        QTextEdit::ExtraSelection sel;
        if (backwardMatchType == TextBlockUserData::Mismatch) {
            sel.cursor = backwardMatch;
            sel.format = m_mismatchFormat;
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
            sel.format = m_mismatchFormat;
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
        foreach (const QTextEdit::ExtraSelection &sel, q->extraSelections(BaseTextEditorWidget::ParenthesesMatchingSelection)) {
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
        m_animator = new BaseTextEditorAnimator(this);
        m_animator->setPosition(animatePosition);
        QPalette pal;
        pal.setBrush(QPalette::Text, matchFormat.foreground());
        pal.setBrush(QPalette::Base, matchFormat.background());
        m_animator->setData(q->font(), pal, q->document()->characterAt(m_animator->position()));
        connect(m_animator.data(), &BaseTextEditorAnimator::updateRequest,
                this, &BaseTextEditorWidgetPrivate::_q_animateUpdate);
    }
    if (m_displaySettings.m_highlightMatchingParentheses)
        q->setExtraSelections(BaseTextEditorWidget::ParenthesesMatchingSelection, extraSelections);
}

void BaseTextEditorWidgetPrivate::_q_highlightBlocks()
{
    BaseTextEditorPrivateHighlightBlocks highlightBlocksInfo;

    QTextBlock block;
    if (extraAreaHighlightFoldedBlockNumber >= 0) {
        block = q->document()->findBlockByNumber(extraAreaHighlightFoldedBlockNumber);
        if (block.isValid()
            && block.next().isValid()
            && BaseTextDocumentLayout::foldingIndent(block.next())
            > BaseTextDocumentLayout::foldingIndent(block))
            block = block.next();
    }

    QTextBlock closeBlock = block;
    while (block.isValid()) {
        int foldingIndent = BaseTextDocumentLayout::foldingIndent(block);

        while (block.previous().isValid() && BaseTextDocumentLayout::foldingIndent(block) >= foldingIndent)
            block = block.previous();
        int nextIndent = BaseTextDocumentLayout::foldingIndent(block);
        if (nextIndent == foldingIndent)
            break;
        highlightBlocksInfo.open.prepend(block.blockNumber());
        while (closeBlock.next().isValid()
            && BaseTextDocumentLayout::foldingIndent(closeBlock.next()) >= foldingIndent )
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

void BaseTextEditorWidget::changeEvent(QEvent *e)
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

void BaseTextEditorWidget::focusInEvent(QFocusEvent *e)
{
    QPlainTextEdit::focusInEvent(e);
    d->updateHighlights();
}

void BaseTextEditorWidget::focusOutEvent(QFocusEvent *e)
{
    QPlainTextEdit::focusOutEvent(e);
    if (viewport()->cursor().shape() == Qt::BlankCursor)
        viewport()->setCursor(Qt::IBeamCursor);
}


void BaseTextEditorWidgetPrivate::maybeSelectLine()
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
void BaseTextEditorWidget::cutLine()
{
    d->maybeSelectLine();
    cut();
}

// ctrl+ins
void BaseTextEditorWidget::copyLine()
{
    QTextCursor prevCursor = textCursor();
    d->maybeSelectLine();
    copy();
    if (!prevCursor.hasSelection())
        prevCursor.movePosition(QTextCursor::StartOfBlock);
    setTextCursor(prevCursor, d->m_inBlockSelectionMode);
}

void BaseTextEditorWidget::deleteLine()
{
    d->maybeSelectLine();
    textCursor().removeSelectedText();
}

void BaseTextEditorWidget::deleteEndOfWord()
{
    moveCursor(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    textCursor().removeSelectedText();
    setTextCursor(textCursor());
}

void BaseTextEditorWidget::deleteEndOfWordCamelCase()
{
    QTextCursor c = textCursor();
    d->camelCaseRight(c, QTextCursor::KeepAnchor);
    c.removeSelectedText();
    setTextCursor(c);
}

void BaseTextEditorWidget::deleteStartOfWord()
{
    moveCursor(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
    textCursor().removeSelectedText();
    setTextCursor(textCursor());
}

void BaseTextEditorWidget::deleteStartOfWordCamelCase()
{
    QTextCursor c = textCursor();
    d->camelCaseLeft(c, QTextCursor::KeepAnchor);
    c.removeSelectedText();
    setTextCursor(c);
}

void BaseTextEditorWidget::setExtraSelections(ExtraSelectionKind kind, const QList<QTextEdit::ExtraSelection> &selections)
{
    if (selections.isEmpty() && d->m_extraSelections[kind].isEmpty())
        return;
    d->m_extraSelections[kind] = selections;

    if (kind == CodeSemanticsSelection) {
        d->m_overlay->clear();
        foreach (const QTextEdit::ExtraSelection &selection, d->m_extraSelections[kind]) {
            d->m_overlay->addOverlaySelection(selection.cursor,
                                              selection.format.background().color(),
                                              selection.format.background().color(),
                                              TextEditorOverlay::LockSize);
        }
        d->m_overlay->setVisible(!d->m_overlay->isEmpty());
    } else if (kind == SnippetPlaceholderSelection) {
        d->m_snippetOverlay->mangle();
        d->m_snippetOverlay->clear();
        foreach (const QTextEdit::ExtraSelection &selection, d->m_extraSelections[kind]) {
            d->m_snippetOverlay->addOverlaySelection(selection.cursor,
                                              selection.format.background().color(),
                                              selection.format.background().color(),
                                              TextEditorOverlay::ExpandBegin);
        }
        d->m_snippetOverlay->mapEquivalentSelections();
        d->m_snippetOverlay->setVisible(!d->m_snippetOverlay->isEmpty());
    } else {
        QList<QTextEdit::ExtraSelection> all;
        for (int i = 0; i < NExtraSelectionKinds; ++i) {
            if (i == CodeSemanticsSelection || i == SnippetPlaceholderSelection)
                continue;
            all += d->m_extraSelections[i];
        }
        QPlainTextEdit::setExtraSelections(all);
    }
}

QList<QTextEdit::ExtraSelection> BaseTextEditorWidget::extraSelections(ExtraSelectionKind kind) const
{
    return d->m_extraSelections[kind];
}

QString BaseTextEditorWidget::extraSelectionTooltip(int pos) const
{
    QList<QTextEdit::ExtraSelection> all;
    for (int i = 0; i < NExtraSelectionKinds; ++i) {
        const QList<QTextEdit::ExtraSelection> &sel = d->m_extraSelections[i];
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
void BaseTextEditorWidget::setIfdefedOutBlocks(const QList<BlockRange> &blocks)
{
    QTextDocument *doc = document();
    BaseTextDocumentLayout *documentLayout = qobject_cast<BaseTextDocumentLayout*>(doc->documentLayout());
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
                set = BaseTextDocumentLayout::setIfdefedOut(block);
            else
                cleared = BaseTextDocumentLayout::clearIfdefedOut(block);
            if (block.contains(range.last()))
                ++rangeNumber;
        } else {
            cleared = BaseTextDocumentLayout::clearIfdefedOut(block);
        }

        if (cleared || set) {
            needUpdate = true;
            int delta = BaseTextDocumentLayout::braceDepthDelta(block);
            if (cleared)
                braceDepthDelta += delta;
            else if (set)
                braceDepthDelta -= delta;
        }

        if (braceDepthDelta) {
            BaseTextDocumentLayout::changeBraceDepth(block,braceDepthDelta);
            BaseTextDocumentLayout::changeFoldingIndent(block, braceDepthDelta); // ### C++ only, refactor!
        }

        block = block.next();
    }

    if (needUpdate)
        documentLayout->requestUpdate();
}

void BaseTextEditorWidget::format()
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();
    d->m_document->autoIndent(cursor);
    cursor.endEditBlock();
}

void BaseTextEditorWidget::rewrapParagraph()
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

void BaseTextEditorWidget::unCommentSelection()
{
    Utils::unCommentSelection(this, d->m_commentDefinition);
}

void BaseTextEditorWidget::showEvent(QShowEvent* e)
{
    triggerPendingUpdates();
    QPlainTextEdit::showEvent(e);
}


void BaseTextEditorWidgetPrivate::applyFontSettingsDelayed()
{
    m_fontSettingsNeedsApply = true;
    if (q->isVisible())
        q->triggerPendingUpdates();
}

void BaseTextEditorWidget::triggerPendingUpdates()
{
    if (d->m_fontSettingsNeedsApply)
        applyFontSettings();
    textDocument()->triggerPendingUpdates();
}

void BaseTextEditorWidget::applyFontSettings()
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

void BaseTextEditorWidget::setDisplaySettings(const DisplaySettings &ds)
{
    setLineWrapMode(ds.m_textWrapping ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
    setLineNumbersVisible(ds.m_displayLineNumbers);
    setHighlightCurrentLine(ds.m_highlightCurrentLine);
    setRevisionsVisible(ds.m_markTextChanges);
    setCenterOnScroll(ds.m_centerCursorOnScroll);
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
        d->m_highlightBlocksInfo = BaseTextEditorPrivateHighlightBlocks();
    }

    d->updateCodeFoldingVisible();
    d->updateHighlights();
    viewport()->update();
    extraArea()->update();
}

void BaseTextEditorWidget::setMarginSettings(const MarginSettings &ms)
{
    setVisibleWrapColumn(ms.m_showMargin ? ms.m_marginColumn : 0);
    d->m_marginSettings = ms;

    viewport()->update();
    extraArea()->update();
}

void BaseTextEditorWidget::setBehaviorSettings(const TextEditor::BehaviorSettings &bs)
{
    d->m_behaviorSettings = bs;
}

void BaseTextEditorWidget::setTypingSettings(const TypingSettings &typingSettings)
{
    d->m_document->setTypingSettings(typingSettings);
}

void BaseTextEditorWidget::setStorageSettings(const StorageSettings &storageSettings)
{
    d->m_document->setStorageSettings(storageSettings);
}

void BaseTextEditorWidget::setCompletionSettings(const TextEditor::CompletionSettings &completionSettings)
{
    d->m_autoCompleter->setAutoParenthesesEnabled(completionSettings.m_autoInsertBrackets);
    d->m_autoCompleter->setSurroundWithEnabled(completionSettings.m_autoInsertBrackets
                                               && completionSettings.m_surroundingAutoBrackets);
}

void BaseTextEditorWidget::setExtraEncodingSettings(const ExtraEncodingSettings &extraEncodingSettings)
{
    d->m_document->setExtraEncodingSettings(extraEncodingSettings);
}

void BaseTextEditorWidget::fold()
{
    QTextDocument *doc = document();
    BaseTextDocumentLayout *documentLayout = qobject_cast<BaseTextDocumentLayout*>(doc->documentLayout());
    QTC_ASSERT(documentLayout, return);
    QTextBlock block = textCursor().block();
    if (!(BaseTextDocumentLayout::canFold(block) && block.next().isVisible())) {
        // find the closest previous block which can fold
        int indent = BaseTextDocumentLayout::foldingIndent(block);
        while (block.isValid() && (BaseTextDocumentLayout::foldingIndent(block) >= indent || !block.isVisible()))
            block = block.previous();
    }
    if (block.isValid()) {
        BaseTextDocumentLayout::doFoldOrUnfold(block, false);
        d->moveCursorVisible();
        documentLayout->requestUpdate();
        documentLayout->emitDocumentSizeChanged();
    }
}

void BaseTextEditorWidget::unfold()
{
    QTextDocument *doc = document();
    BaseTextDocumentLayout *documentLayout = qobject_cast<BaseTextDocumentLayout*>(doc->documentLayout());
    QTC_ASSERT(documentLayout, return);
    QTextBlock block = textCursor().block();
    while (block.isValid() && !block.isVisible())
        block = block.previous();
    BaseTextDocumentLayout::doFoldOrUnfold(block, true);
    d->moveCursorVisible();
    documentLayout->requestUpdate();
    documentLayout->emitDocumentSizeChanged();
}

void BaseTextEditorWidget::unfoldAll()
{
    QTextDocument *doc = document();
    BaseTextDocumentLayout *documentLayout = qobject_cast<BaseTextDocumentLayout*>(doc->documentLayout());
    QTC_ASSERT(documentLayout, return);

    QTextBlock block = doc->firstBlock();
    bool makeVisible = true;
    while (block.isValid()) {
        if (block.isVisible() && BaseTextDocumentLayout::canFold(block) && block.next().isVisible()) {
            makeVisible = false;
            break;
        }
        block = block.next();
    }

    block = doc->firstBlock();

    while (block.isValid()) {
        if (BaseTextDocumentLayout::canFold(block))
            BaseTextDocumentLayout::doFoldOrUnfold(block, makeVisible);
        block = block.next();
    }

    d->moveCursorVisible();
    documentLayout->requestUpdate();
    documentLayout->emitDocumentSizeChanged();
    centerCursor();
}

void BaseTextEditorWidget::setReadOnly(bool b)
{
    QPlainTextEdit::setReadOnly(b);
    emit readOnlyChanged();
    if (b)
        setTextInteractionFlags(textInteractionFlags() | Qt::TextSelectableByKeyboard);
}

void BaseTextEditorWidget::cut()
{
    if (d->m_inBlockSelectionMode) {
        copy();
        d->removeBlockSelection();
        return;
    }
    QPlainTextEdit::cut();
    d->collectToCircularClipboard();
}

void BaseTextEditorWidget::selectAll()
{
    d->disableBlockSelection();
    QPlainTextEdit::selectAll();
}

void BaseTextEditorWidget::copy()
{
    if (!textCursor().hasSelection() || (d->m_inBlockSelectionMode
            && d->m_blockSelection.anchorColumn == d->m_blockSelection.positionColumn)) {
        return;
    }

    QPlainTextEdit::copy();
    d->collectToCircularClipboard();
}

void BaseTextEditorWidget::paste()
{
    QPlainTextEdit::paste();
}

void BaseTextEditorWidgetPrivate::collectToCircularClipboard()
{
    const QMimeData *mimeData = QApplication::clipboard()->mimeData();
    if (!mimeData)
        return;
    CircularClipboard *circularClipBoard = CircularClipboard::instance();
    circularClipBoard->collect(BaseTextEditorWidget::duplicateMimeData(mimeData));
    // We want the latest copied content to be the first one to appear on circular paste.
    circularClipBoard->toLastCollect();
}

void BaseTextEditorWidget::circularPaste()
{
    CircularClipboard *circularClipBoard = CircularClipboard::instance();
    if (const QMimeData *clipboardData = QApplication::clipboard()->mimeData()) {
        circularClipBoard->collect(BaseTextEditorWidget::duplicateMimeData(clipboardData));
        circularClipBoard->toLastCollect();
    }

    if (circularClipBoard->size() > 1)
        return invokeAssist(QuickFix, d->m_clipboardAssistProvider.data());

    if (const QMimeData *mimeData = circularClipBoard->next().data()) {
        QApplication::clipboard()->setMimeData(BaseTextEditorWidget::duplicateMimeData(mimeData));
        paste();
    }
}

void BaseTextEditorWidget::switchUtf8bom()
{
    textDocument()->switchUtf8Bom();
}

QMimeData *BaseTextEditorWidget::createMimeDataFromSelection() const
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

bool BaseTextEditorWidget::canInsertFromMimeData(const QMimeData *source) const
{
    return QPlainTextEdit::canInsertFromMimeData(source);
}

void BaseTextEditorWidget::insertFromMimeData(const QMimeData *source)
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

QMimeData *BaseTextEditorWidget::duplicateMimeData(const QMimeData *source)
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

QString BaseTextEditorWidget::lineNumber(int blockNumber) const
{
    return QString::number(blockNumber + 1);
}

int BaseTextEditorWidget::lineNumberDigits() const
{
    int digits = 2;
    int max = qMax(1, blockCount());
    while (max >= 100) {
        max /= 10;
        ++digits;
    }
    return digits;
}

bool BaseTextEditorWidget::selectionVisible(int blockNumber) const
{
    Q_UNUSED(blockNumber);
    return true;
}

bool BaseTextEditorWidget::replacementVisible(int blockNumber) const
{
    Q_UNUSED(blockNumber)
    return true;
}

QColor BaseTextEditorWidget::replacementPenColor(int blockNumber) const
{
    Q_UNUSED(blockNumber)
    return QColor();
}

void BaseTextEditorWidget::setupFallBackEditor(Id id)
{
    QTC_CHECK(!d->m_editor);
    QTC_CHECK(!d->m_editorIsFallBack);
    BaseTextDocumentPtr doc(new BaseTextDocument(id));
    doc->setFontSettings(TextEditorSettings::fontSettings());
    setTextDocument(doc);
    d->m_editor = new BaseTextEditor;
    d->m_editor->setEditorWidget(this);
    d->m_editorIsFallBack = true;
}

void BaseTextEditorWidget::appendStandardContextMenuActions(QMenu *menu)
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

    BaseTextDocument *doc = textDocument();
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
    d->m_completionAssistProvider = [] () -> CompletionAssistProvider * { return 0; };
    addContext(TextEditor::Constants::C_TEXTEDITOR);
    setDuplicateSupported(true);
}

void BaseTextEditor::setEditorWidget(BaseTextEditorWidget *widget)
{
    widget->d->m_editor = this;
    setWidget(widget);
}

void BaseTextEditor::configureCodeAssistant()
{
    editorWidget()->d->m_codeAssistant.configure(this);
}

BaseTextEditor::~BaseTextEditor()
{
    delete m_widget;
    delete d;
}

BaseTextDocument *BaseTextEditor::textDocument() const
{
    ensureDocument();
    return editorWidget()->textDocument();
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

void BaseTextEditorWidget::insertExtraToolBarWidget(BaseTextEditorWidget::Side side,
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

int BaseTextEditor::position(BaseTextEditor::PositionOperation posOp, int at) const
{
    return editorWidget()->position(posOp, at);
}

void BaseTextEditor::convertPosition(int pos, int *line, int *column) const
{
    editorWidget()->convertPosition(pos, line, column);
}

QRect BaseTextEditor::cursorRect(int pos) const
{
    QTextCursor tc = editorWidget()->textCursor();
    if (pos >= 0)
        tc.setPosition(pos);
    QRect result = editorWidget()->cursorRect(tc);
    result.moveTo(editorWidget()->viewport()->mapToGlobal(result.topLeft()));
    return result;
}

QString BaseTextEditor::selectedText() const
{
    return editorWidget()->selectedText();
}

void BaseTextEditor::remove(int length)
{
    QTextCursor tc = editorWidget()->textCursor();
    tc.setPosition(tc.position() + length, QTextCursor::KeepAnchor);
    tc.removeSelectedText();
}

void BaseTextEditor::insert(const QString &string)
{
    editorWidget()->insertPlainText(string);
}

void BaseTextEditor::replace(int length, const QString &string)
{
    QTextCursor tc = editorWidget()->textCursor();
    tc.setPosition(tc.position() + length, QTextCursor::KeepAnchor);
    tc.insertText(string);
}

void BaseTextEditor::setCursorPosition(int pos)
{
    editorWidget()->setBlockSelection(false);
    QTextCursor tc = editorWidget()->textCursor();
    tc.setPosition(pos);
    editorWidget()->setTextCursor(tc);
}

void BaseTextEditor::select(int toPos)
{
    editorWidget()->setBlockSelection(false);
    QTextCursor tc = editorWidget()->textCursor();
    tc.setPosition(toPos, QTextCursor::KeepAnchor);
    editorWidget()->setTextCursor(tc);
}

CompletionAssistProvider *BaseTextEditor::completionAssistProvider()
{
    return d->m_completionAssistProvider();
}

void BaseTextEditor::setCompletionAssistProvider(CompletionAssistProvider *provider)
{
    d->m_completionAssistProvider = [provider] () -> CompletionAssistProvider * { return provider; };
}

void BaseTextEditor::setCompletionAssistProvider(const std::function<CompletionAssistProvider *()> &provider)
{
    d->m_completionAssistProvider = provider;
}

void BaseTextEditorWidgetPrivate::updateCursorPosition()
{
    const QTextCursor cursor = q->textCursor();
    const QTextBlock block = cursor.block();
    const int line = block.blockNumber() + 1;
    const int column = cursor.position() - block.position();
    m_cursorPositionLabel->show();
    m_cursorPositionLabel->setText(tr("Line: %1, Col: %2").arg(line)
                                   .arg(q->textDocument()->tabSettings().columnAt(block.text(),
                                                                                   column)+1),
                                   tr("Line: 9999, Col: 999"));
    if (m_editor)
        m_editor->m_contextHelpId.clear();

    if (!block.isVisible())
        q->ensureCursorVisible();
}

QString BaseTextEditor::contextHelpId() const
{
    if (m_contextHelpId.isEmpty())
        emit const_cast<BaseTextEditor*>(this)->contextHelpIdRequested(const_cast<BaseTextEditor*>(this),
                                                                       editorWidget()->textCursor().position());
    return m_contextHelpId;
}

RefactorMarkers BaseTextEditorWidget::refactorMarkers() const
{
    return d->m_refactorOverlay->markers();
}

void BaseTextEditorWidget::setRefactorMarkers(const RefactorMarkers &markers)
{
    foreach (const RefactorMarker &marker, d->m_refactorOverlay->markers())
        requestBlockUpdate(marker.cursor.block());
    d->m_refactorOverlay->setMarkers(markers);
    foreach (const RefactorMarker &marker, markers)
        requestBlockUpdate(marker.cursor.block());
}

void BaseTextEditorWidget::doFoo()
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

BaseTextBlockSelection::BaseTextBlockSelection(const BaseTextBlockSelection &other)
{
    positionBlock = other.positionBlock;
    positionColumn = other.positionColumn;
    anchorBlock = other.anchorBlock;
    anchorColumn = other.anchorColumn;
}

void BaseTextBlockSelection::clear()
{
    positionColumn = positionBlock = anchorColumn = anchorBlock = 0;
}

// returns a cursor which always has the complete selection
QTextCursor BaseTextBlockSelection::selection(const BaseTextDocument *baseTextDocument) const
{
    return cursor(baseTextDocument, true);
}

// returns a cursor which always has the correct position and anchor
QTextCursor BaseTextBlockSelection::cursor(const BaseTextDocument *baseTextDocument) const
{
    return cursor(baseTextDocument, false);
}

QTextCursor BaseTextBlockSelection::cursor(const BaseTextDocument *baseTextDocument,
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

void BaseTextBlockSelection::fromPostition(int positionBlock, int positionColumn,
                                           int anchorBlock, int anchorColumn)
{
    this->positionBlock = positionBlock;
    this->positionColumn = positionColumn;
    this->anchorBlock = anchorBlock;
    this->anchorColumn = anchorColumn;
}

bool BaseTextEditorWidgetPrivate::inFindScope(const QTextCursor &cursor)
{
    if (cursor.isNull())
        return false;
    return inFindScope(cursor.selectionStart(), cursor.selectionEnd());
}

bool BaseTextEditorWidgetPrivate::inFindScope(int selectionStart, int selectionEnd)
{
    if (m_findScopeStart.isNull())
        return true; // no scope, everything is included
    if (selectionStart < m_findScopeStart.position())
        return false;
    if (selectionEnd > m_findScopeEnd.position())
        return false;
    if (m_findScopeVerticalBlockSelectionFirstColumn < 0)
        return true;
    QTextBlock block = q->document()->findBlock(selectionStart);
    if (block != q->document()->findBlock(selectionEnd))
        return false;
    QString text = block.text();
    const TabSettings &ts = m_document->tabSettings();
    int startPosition = ts.positionAtColumn(text, m_findScopeVerticalBlockSelectionFirstColumn);
    int endPosition = ts.positionAtColumn(text, m_findScopeVerticalBlockSelectionLastColumn);
    if (selectionStart - block.position() < startPosition)
        return false;
    if (selectionEnd - block.position() > endPosition)
        return false;
    return true;
}

void BaseTextEditorWidget::setBlockSelection(bool on)
{
    if (d->m_inBlockSelectionMode == on)
        return;

    if (on)
        d->enableBlockSelection(textCursor());
    else
        d->disableBlockSelection(false);
}

void BaseTextEditorWidget::setBlockSelection(int positionBlock, int positionColumn,
                                             int anchhorBlock, int anchorColumn)
{
    d->enableBlockSelection(positionBlock, positionColumn, anchhorBlock, anchorColumn);
}

void BaseTextEditorWidget::setBlockSelection(const QTextCursor &cursor)
{
    d->enableBlockSelection(cursor);
}

QTextCursor BaseTextEditorWidget::blockSelection() const
{
    return d->m_blockSelection.cursor(d->m_document.data());
}

bool BaseTextEditorWidget::hasBlockSelection() const
{
    return d->m_inBlockSelectionMode;
}

void BaseTextEditorWidgetPrivate::updateTabStops()
{
    // Although the tab stop is stored as qreal the API from QPlainTextEdit only allows it
    // to be set as an int. A work around is to access directly the QTextOption.
    qreal charWidth = QFontMetricsF(q->font()).width(QLatin1Char(' '));
    QTextOption option = q->document()->defaultTextOption();
    option.setTabStop(charWidth * m_document->tabSettings().m_tabSize);
    q->document()->setDefaultTextOption(option);
}

int BaseTextEditorWidget::columnCount() const
{
    QFontMetricsF fm(font());
    return viewport()->rect().width() / fm.width(QLatin1Char('x'));
}

int BaseTextEditorWidget::rowCount() const
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
void BaseTextEditorWidgetPrivate::transformSelection(TransformationMethod method)
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

void BaseTextEditorWidgetPrivate::transformBlockSelection(TransformationMethod method)
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

void BaseTextEditorWidget::inSnippetMode(bool *active)
{
    *active = d->m_snippetOverlay->isVisible();
}

void BaseTextEditorWidget::invokeAssist(AssistKind kind, IAssistProvider *provider)
{
    bool previousMode = overwriteMode();
    setOverwriteMode(false);
    ensureCursorVisible();
    d->m_codeAssistant.invoke(kind, provider);
    setOverwriteMode(previousMode);
}

IAssistInterface *BaseTextEditorWidget::createAssistInterface(AssistKind kind,
                                                              AssistReason reason) const
{
    Q_UNUSED(kind);
    return new DefaultAssistInterface(document(), position(), d->m_document->filePath(), reason);
}

QString BaseTextEditorWidget::foldReplacementText(const QTextBlock &) const
{
    return QLatin1String("...");
}

bool BaseTextEditor::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    return editorWidget()->open(errorString, fileName, realFileName);
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

BaseTextEditorWidget *BaseTextEditor::editorWidget() const
{
    QTC_ASSERT(qobject_cast<BaseTextEditorWidget *>(m_widget.data()), return 0);
    return static_cast<BaseTextEditorWidget *>(m_widget.data());
}

QTextDocument *BaseTextEditor::qdocument() const
{
    return textDocument()->document();
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

void BaseTextEditorWidget::configureMimeType(const QString &mimeType)
{
    configureMimeType(MimeDatabase::findByType(mimeType));
}

void BaseTextEditorWidget::configureMimeType(const MimeType &mimeType)
{
    Highlighter *highlighter = new Highlighter();
    highlighter->setTabSettings(textDocument()->tabSettings());
    textDocument()->setSyntaxHighlighter(highlighter);

    setCodeFoldingSupported(false);

    if (!mimeType.isNull()) {
        d->m_isMissingSyntaxDefinition = true;

        setMimeTypeForHighlighter(highlighter, mimeType);
        const QString &type = mimeType.type();
        textDocument()->setMimeType(type);

        QString definitionId = Manager::instance()->definitionIdByMimeType(type);
        if (definitionId.isEmpty())
            definitionId = findDefinitionId(mimeType, true);

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
            const QString &fileName = textDocument()->filePath();
            if (TextEditorSettings::highlighterSettings().isIgnoredFilePattern(fileName))
                d->m_isMissingSyntaxDefinition = false;
        }
    }

    textDocument()->setFontSettings(TextEditorSettings::fontSettings());

    updateEditorInfoBar(this);
}

bool BaseTextEditorWidget::isMissingSyntaxDefinition() const
{
    return d->m_isMissingSyntaxDefinition;
}

void BaseTextEditorWidget::acceptMissingSyntaxDefinitionInfo()
{
    ICore::showOptionsDialog(Constants::TEXT_EDITOR_SETTINGS_CATEGORY,
                             Constants::TEXT_EDITOR_HIGHLIGHTER_SETTINGS,
                             this);
}

// The remnants of PlainTextEditor.
void BaseTextEditorWidget::setupAsPlainEditor()
{
    setRevisionsVisible(true);
    setMarksVisible(true);
    setLineSeparatorsAllowed(true);
    setLineSeparatorsAllowed(true);

    textDocument()->setMimeType(QLatin1String(TextEditor::Constants::C_TEXTEDITOR_MIMETYPE_TEXT));

    auto reconf = [this]() {
        MimeType mimeType;
        if (textDocument())
            mimeType = MimeDatabase::findByFile(textDocument()->filePath());
        configureMimeType(mimeType);
    };

    connect(textDocument(), &IDocument::filePathChanged, reconf);
    connect(Manager::instance(), &Manager::mimeTypesRegistered, reconf);

    updateEditorInfoBar(this);
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

QWidget *BaseTextEditor::widget() const
{
    return ensureWidget();
}

BaseTextEditorWidget *BaseTextEditor::ensureWidget() const
{
    return editorWidget();
}

BaseTextDocumentPtr BaseTextEditor::ensureDocument() const
{
    BaseTextEditorWidget *widget = ensureWidget();
    if (widget->d->m_document.isNull()) {
        QTC_ASSERT(!d->m_origin, return BaseTextDocumentPtr()); // New style always sets it.
    }
    return widget->textDocumentPtr();
}


//
// BaseTextEditorFactory
//

BaseTextEditorFactory::BaseTextEditorFactory(QObject *parent)
    : IEditorFactory(parent)
{
    m_editorCreator = []() { return new BaseTextEditor; };
    m_widgetCreator = []() { return new BaseTextEditorWidget; };
    m_commentStyle = CommentDefinition::NoStyle;
}

void BaseTextEditorFactory::setDocumentCreator(const DocumentCreator &creator)
{
    m_documentCreator = creator;
}

void BaseTextEditorFactory::setEditorWidgetCreator(const EditorWidgetCreator &creator)
{
    m_widgetCreator = creator;
}

void BaseTextEditorFactory::setEditorCreator(const EditorCreator &creator)
{
    m_editorCreator = creator;
}

void BaseTextEditorFactory::setIndenterCreator(const IndenterCreator &creator)
{
    m_indenterCreator = creator;
}

void BaseTextEditorFactory::setSyntaxHighlighterCreator(const SyntaxHighLighterCreator &creator)
{
    m_syntaxHighlighterCreator = creator;
}

void BaseTextEditorFactory::setGenericSyntaxHighlighter(const QString &mimeType)
{
    m_syntaxHighlighterCreator = [this, mimeType]() -> SyntaxHighlighter * {
        Highlighter *highlighter = new TextEditor::Highlighter();
        setMimeTypeForHighlighter(highlighter, Core::MimeDatabase::findByType(mimeType));
        return highlighter;
    };
}

void BaseTextEditorFactory::setAutoCompleterCreator(const AutoCompleterCreator &creator)
{
    m_autoCompleterCreator = creator;
}

void BaseTextEditorFactory::setEditorActionHandlers(Id contextId, uint optionalActions)
{
    new TextEditorActionHandler(this, contextId, optionalActions);
}

void BaseTextEditorFactory::setEditorActionHandlers(uint optionalActions)
{
    new TextEditorActionHandler(this, id(), optionalActions);
}

void BaseTextEditorFactory::setCommentStyle(CommentDefinition::Style style)
{
    m_commentStyle = style;
}

BaseTextEditor *BaseTextEditorFactory::duplicateTextEditor(BaseTextEditor *other)
{
    BaseTextEditor *editor = createEditorHelper(other->editorWidget()->textDocumentPtr());
    editor->editorWidget()->finalizeInitializationAfterDuplication(other->editorWidget());
    return editor;
}

IEditor *BaseTextEditorFactory::createEditor()
{
    BaseTextDocumentPtr doc(m_documentCreator());

    if (m_indenterCreator)
        doc->setIndenter(m_indenterCreator());

    if (m_syntaxHighlighterCreator) {
        SyntaxHighlighter *highlighter = m_syntaxHighlighterCreator();
        highlighter->setParent(doc.data());
        doc->setSyntaxHighlighter(highlighter);
    }

    return createEditorHelper(doc);
}

BaseTextEditor *BaseTextEditorFactory::createEditorHelper(const BaseTextDocumentPtr &document)
{
    BaseTextEditorWidget *widget = m_widgetCreator();
    BaseTextEditor *editor = m_editorCreator();
    editor->d->m_origin = this;

    widget->d->m_editor = editor;
    editor->m_widget = widget;
    widget->setTextDocument(document);

    widget->d->m_codeAssistant.configure(editor);
    widget->d->m_commentDefinition.setStyle(m_commentStyle);

    if (m_autoCompleterCreator)
        widget->setAutoCompleter(m_autoCompleterCreator());

    connect(widget, &BaseTextEditorWidget::markRequested, editor,
            [editor](int line, BaseTextEditor::MarkRequestKind kind) {
                editor->markRequested(editor, line, kind);
            });

    connect(widget, &BaseTextEditorWidget::markContextMenuRequested, editor,
            [editor](int line, QMenu *menu) {
                editor->markContextMenuRequested(editor, line, menu);
            });

    connect(widget, &BaseTextEditorWidget::tooltipOverrideRequested, editor,
            [editor](const QPoint &globalPos, int position, bool *handled) {
                editor->tooltipOverrideRequested(editor, globalPos, position, handled);
            });

    connect(widget, &BaseTextEditorWidget::tooltipRequested, editor,
            [editor](const QPoint &globalPos, int position) {
                editor->tooltipRequested(editor, globalPos, position);
            });

    connect(widget, &BaseTextEditorWidget::markTooltipRequested, editor,
            [editor](const QPoint &globalPos, int line) {
                editor->markTooltipRequested(editor, globalPos, line);
            });

    connect(widget, &BaseTextEditorWidget::activateEditor,
            [editor]() { Core::EditorManager::activateEditor(editor); });

    widget->finalizeInitialization();
    editor->finalizeInitialization();

    connect(widget->d->m_cursorPositionLabel, &LineColumnLabel::clicked, [editor] {
        EditorManager::activateEditor(editor, EditorManager::IgnoreNavigationHistory);
        if (Core::Command *cmd = ActionManager::command(Core::Constants::GOTO)) {
            if (QAction *act = cmd->action())
                act->trigger();
        }
    });
    return editor;
}

} // namespace TextEditor

#include "basetexteditor.moc"
