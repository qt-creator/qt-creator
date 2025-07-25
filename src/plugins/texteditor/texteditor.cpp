// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "texteditor.h"

#include "autocompleter.h"
#include "basehoverhandler.h"
#include "behaviorsettings.h"
#include "bookmarkmanager.h"
#include "circularclipboard.h"
#include "circularclipboardassist.h"
#include "codeassist/assistinterface.h"
#include "codeassist/codeassistant.h"
#include "codeassist/completionassistprovider.h"
#include "codeassist/documentcontentcompletion.h"
#include "completionsettings.h"
#include "displaysettings.h"
#include "extraencodingsettings.h"
#include "fontsettings.h"
#include "highlighter.h"
#include "highlighterhelper.h"
#include "highlightersettings.h"
#include "icodestylepreferences.h"
#include "linenumberfilter.h"
#include "marginsettings.h"
#include "refactoroverlay.h"
#include "snippets/snippetoverlay.h"
#include "storagesettings.h"
#include "tabsettings.h"
#include "textdocument.h"
#include "textdocumentlayout.h"
#include "texteditorconstants.h"
#include "texteditoroverlay.h"
#include "texteditorsettings.h"
#include "texteditortr.h"
#include "typehierarchy.h"
#include "typingsettings.h"

#include <aggregation/aggregate.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/codecselector.h>
#include <coreplugin/find/basetextfind.h>
#include <coreplugin/find/highlightscrollbarcontroller.h>
#include <coreplugin/icore.h>
#include <coreplugin/locator/locatormanager.h>
#include <coreplugin/manhattanstyle.h>
#include <coreplugin/navigationwidget.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/camelcasecursor.h>
#include <utils/dropsupport.h>
#include <utils/fadingindicator.h>
#include <utils/filesearch.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/infobar.h>
#include <utils/mimeutils.h>
#include <utils/minimizableinfobars.h>
#include <utils/multitextcursor.h>
#include <utils/qtcassert.h>
#include <utils/searchresultitem.h>
#include <utils/styledbar.h>
#include <utils/stylehelper.h>
#include <utils/textutils.h>
#include <utils/theme/theme.h>
#include <utils/tooltip/tooltip.h>
#include <utils/uncommentselection.h>

#include <QAbstractTextDocumentLayout>
#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QCoreApplication>
#include <QDebug>
#include <QDesktopServices>
#include <QDrag>
#include <QFutureWatcher>
#include <QGridLayout>
#include <QKeyEvent>
#include <QLoggingCategory>
#include <QMap>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QPainterPath>
#include <QPrintDialog>
#include <QPrinter>
#include <QPropertyAnimation>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QScreen>
#include <QScrollBar>
#include <QSequentialAnimationGroup>
#include <QShortcut>
#include <QStyle>
#include <QStyleFactory>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocumentFragment>
#include <QTextLayout>
#include <QTime>
#include <QTimeLine>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>

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
    \brief The BaseTextEditor class is base implementation for PlainTextEdit-based
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

using namespace Internal;

namespace Internal {

class TextEditorWidgetFind;

enum { NExtraSelectionKinds = 12 };

using TransformationMethod = QString(const QString &);
using ListTransformationMethod = void(QStringList &);

static constexpr char dropProperty[] = "dropProp";

class LineColumnButtonPrivate
{
public:
    QSize m_maxSize;
    TextEditorWidget *m_editor;
};

} // namespace Internal

LineColumnButton::LineColumnButton(TextEditorWidget *parent)
    : QToolButton(parent)
    , m_d(new LineColumnButtonPrivate)
{
    m_d->m_editor = parent;
    connect(m_d->m_editor, &PlainTextEdit::cursorPositionChanged, this, &LineColumnButton::update);
    connect(this, &QToolButton::clicked, ActionManager::instance(), [this] {
        m_d->m_editor->setFocus();
        QMetaObject::invokeMethod(
            ActionManager::instance(),
            [] {
                if (Command *cmd = ActionManager::command(Core::Constants::GOTO)) {
                    if (QAction *act = cmd->action())
                        act->trigger();
                }
            },
            Qt::QueuedConnection);
    });
}

LineColumnButton::~LineColumnButton() = default;

void LineColumnButton::update()
{
    const Utils::MultiTextCursor &cursors = m_d->m_editor->multiTextCursor();
    QString text;
    if (cursors.hasMultipleCursors()) {
        text = Tr::tr("Cursors: %1").arg(cursors.cursorCount());
    } else {
        const QTextCursor cursor = cursors.mainCursor();
        const QTextBlock block = cursor.block();
        const int line = block.blockNumber() + 1;
        const TabSettings &tabSettings = m_d->m_editor->textDocument()->tabSettings();
        const int column = tabSettings.columnAt(block.text(), cursor.positionInBlock()) + 1;
        text = Tr::tr("Line: %1, Col: %2").arg(line).arg(column);
        const QString toolTipText = Tr::tr("Cursor position: %1");
        setToolTip(toolTipText.arg(cursor.position()));
    }
    int selection = 0;
    for (const QTextCursor &cursor : cursors)
        selection += cursor.selectionEnd() - cursor.selectionStart();
    if (selection > 0)
        text += " " + Tr::tr("(Sel: %1)").arg(selection);
    setText(text);
}

bool LineColumnButton::event(QEvent *event)
{
    if (event->type() == QEvent::Leave)
        ToolTip::hideImmediately();

    if (event->type() != QEvent::ToolTip)
        return QToolButton::event(event);

    QString tooltipText = "<table cellpadding='2'>\n";

    const MultiTextCursor multiCursor = m_d->m_editor->multiTextCursor();
    const QList<QTextCursor> cursors = multiCursor.cursors().mid(0, 15);

    tooltipText += "<tr>";
    tooltipText += QString("<th align='left'>%1</th>").arg(Tr::tr("Cursors:"));
    tooltipText += QString("<td>%1</td>").arg(multiCursor.cursorCount());
    tooltipText += "</tr>\n";

    auto addRow = [&](const QString header, auto cellText) {
        tooltipText += "<tr>";
        tooltipText += QString("<th align='left'>%1</th>").arg(header);
        for (const QTextCursor &c : cursors)
            tooltipText += QString("<td>%1</td>").arg(cellText(c));
        if (multiCursor.cursorCount() > cursors.count())
            tooltipText += QString("<td>...</td>");
        tooltipText += "</tr>\n";
    };

    addRow(Tr::tr("Line:"), [](const QTextCursor &c) { return c.blockNumber() + 1; });

    const TabSettings &tabSettings = m_d->m_editor->textDocument()->tabSettings();
    addRow(Tr::tr("Column:"), [&](const QTextCursor &c) {
        return tabSettings.columnAt(c.block().text(), c.positionInBlock()) + 1;
    });

    addRow(Tr::tr("Selection length:"),
           [](const QTextCursor &c) { return c.selectionEnd() - c.selectionStart(); });

    addRow(Tr::tr("Selected lines:"), [](const QTextCursor &c) {
        if (!c.hasSelection())
            return 1;

        QTextCursor startCursor = c;
        startCursor.setPosition(c.selectionStart());
        QTextCursor endCursor = c;
        endCursor.setPosition(c.selectionEnd());

        const int startBlock = startCursor.blockNumber();
        const int endBlock = endCursor.blockNumber();
        int selectedLines = endBlock - startBlock + 1;

        // When selection ends at the start of a line, don't count that line
        if (endBlock > startBlock && endCursor.positionInBlock() == 0)
            --selectedLines;

        return selectedLines;
    });

    addRow(Tr::tr("Position in document:"), [](const QTextCursor &c) { return c.position(); });

    addRow(Tr::tr("Anchor:"), [](const QTextCursor &c) { return c.anchor(); });

    tooltipText += "</table>\n";

    ToolTip::show(static_cast<const QHelpEvent *>(event)->globalPos(), tooltipText, Qt::RichText);
    event->accept();
    return true;
}

QSize LineColumnButton::sizeHint() const
{
    const QSize size = QToolButton::sizeHint();
    auto wider = [](const QSize &left, const QSize &right) { return left.width() < right.width(); };
    if (m_d->m_editor->multiTextCursor().hasSelection())
        return std::max(m_d->m_maxSize, size, wider); // do not save the size if we have a selection
    m_d->m_maxSize = std::max(m_d->m_maxSize, size, wider);
    return m_d->m_maxSize;
}

namespace Internal {

class TabSettingsButton : public QToolButton
{
public:
    TabSettingsButton(TextEditorWidget *parent)
        : QToolButton(parent)
    {
        connect(this, &QToolButton::clicked, this, &TabSettingsButton::showMenu);
    }

    void setDocument(TextDocument *doc)
    {
        if (m_doc)
            disconnect(m_doc, &TextDocument::tabSettingsChanged, this, &TabSettingsButton::update);
        m_doc = doc;
        if (QTC_GUARD(m_doc)) {
            connect(m_doc, &TextDocument::tabSettingsChanged, this, &TabSettingsButton::update);
            update();
        }
    }

private:
    void update()
    {
        QTC_ASSERT(m_doc, return);
        const TabSettings ts = m_doc->tabSettings();
        QString policy;
        switch (ts.m_tabPolicy) {
        case TabSettings::SpacesOnlyTabPolicy:
            policy = Tr::tr("Spaces");
            break;
        case TabSettings::TabsOnlyTabPolicy:
            policy = Tr::tr("Tabs");
            break;
        }
        setText(QString("%1: %2").arg(policy).arg(ts.m_indentSize));
    }

    void showMenu()
    {
        QTC_ASSERT(m_doc, return);
        auto menu = new QMenu(this);
        menu->addAction(ActionManager::command(Constants::AUTO_INDENT_SELECTION)->action());
        menu->setAttribute(Qt::WA_DeleteOnClose);
        if (auto indenter = m_doc->indenter(); indenter && indenter->respectsTabSettings()) {
            auto documentSettings = menu->addMenu(Tr::tr("Document Settings"));

            auto modifyTabSettings =
                [this](std::function<void(TabSettings & tabSettings)> modifier) {
                    return [this, modifier]() {
                        auto ts = m_doc->tabSettings();
                        ts.m_autoDetect = false;
                        modifier(ts);
                        m_doc->setTabSettings(ts);
                    };
                };
            documentSettings->addAction(
                Tr::tr("Auto detect"),
                modifyTabSettings([doc = m_doc->document()](TabSettings &tabSettings) {
                    tabSettings.m_autoDetect = true;
                }));
            auto tabSettings = documentSettings->addMenu(Tr::tr("Tab Settings"));
            tabSettings->addAction(Tr::tr("Spaces"), modifyTabSettings([](TabSettings &tabSettings) {
                                       tabSettings.m_tabPolicy = TabSettings::SpacesOnlyTabPolicy;
                                   }));
            tabSettings->addAction(Tr::tr("Tabs"), modifyTabSettings([](TabSettings &tabSettings) {
                                       tabSettings.m_tabPolicy = TabSettings::TabsOnlyTabPolicy;
                                   }));
            auto indentSize = documentSettings->addMenu(Tr::tr("Indent Size"));
            auto indentSizeGroup = new QActionGroup(indentSize);
            indentSizeGroup->setExclusive(true);
            for (int i = 1; i <= 8; ++i) {
                auto action = indentSizeGroup->addAction(QString::number(i));
                action->setCheckable(true);
                action->setChecked(i == m_doc->tabSettings().m_indentSize);
                connect(action, &QAction::triggered, modifyTabSettings([i](TabSettings &tabSettings) {
                            tabSettings.m_indentSize = i;
                        }));
            }
            indentSize->addActions(indentSizeGroup->actions());
            auto tabSize = documentSettings->addMenu(Tr::tr("Tab Size"));
            auto tabSizeGroup = new QActionGroup(tabSize);
            tabSizeGroup->setExclusive(true);
            for (int i = 1; i <= 8; ++i) {
                auto action = tabSizeGroup->addAction(QString::number(i));
                action->setCheckable(true);
                action->setChecked(i == m_doc->tabSettings().m_tabSize);
                connect(action, &QAction::triggered, modifyTabSettings([i](TabSettings &tabSettings) {
                            tabSettings.m_tabSize = i;
                        }));
                }
                tabSize->addActions(tabSizeGroup->actions());
        }

        Id globalSettingsCategory;
        if (auto codeStyle = m_doc->codeStyle())
            globalSettingsCategory = codeStyle->globalSettingsCategory();
        if (!globalSettingsCategory.isValid())
            globalSettingsCategory = Constants::TEXT_EDITOR_BEHAVIOR_SETTINGS;
        menu->addAction(Tr::tr("Global Settings..."), [globalSettingsCategory] {
            Core::ICore::showOptionsDialog(globalSettingsCategory);
        });

        menu->popup(QCursor::pos());
    }

    TextDocument *m_doc = nullptr;
};

class TextEditorAnimator : public QObject
{
    Q_OBJECT

public:
    TextEditorAnimator(QObject *parent);

    void init(const QTextCursor &cursor, const QFont &f, const QPalette &pal);
    inline QTextCursor cursor() const { return m_cursor; }

    void draw(QPainter *p, const QPointF &pos);
    QRectF rect() const;

    inline qreal value() const { return m_value; }
    inline QPointF lastDrawPos() const { return m_lastDrawPos; }

    void finish();

    bool isRunning() const;

signals:
    void updateRequest(const QTextCursor &cursor, QPointF lastPos, QRectF rect);

private:
    void step(qreal v);

    QTimeLine m_timeline;
    qreal m_value;
    QTextCursor m_cursor;
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
    QSize sizeHint() const override {
        return {textEdit->extraAreaWidth(), 0};
    }
    void paintEvent(QPaintEvent *event) override {
        textEdit->extraAreaPaintEvent(event);
    }
    void mousePressEvent(QMouseEvent *event) override {
        textEdit->extraAreaMouseEvent(event);
    }
    void mouseMoveEvent(QMouseEvent *event) override {
        textEdit->extraAreaMouseEvent(event);
    }
    void mouseReleaseEvent(QMouseEvent *event) override {
        textEdit->extraAreaMouseEvent(event);
    }
    void leaveEvent(QEvent *event) override {
        textEdit->extraAreaLeaveEvent(event);
    }
    void contextMenuEvent(QContextMenuEvent *event) override {
        textEdit->extraAreaContextMenuEvent(event);
    }
    void changeEvent(QEvent *event) override {
        if (event->type() == QEvent::PaletteChange)
            QCoreApplication::sendEvent(textEdit, event);
    }
    void wheelEvent(QWheelEvent *event) override {
        QCoreApplication::sendEvent(textEdit->viewport(), event);
    }
    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::ToolTip) {
            textEdit->extraAreaToolTipEvent(static_cast<QHelpEvent *>(event));
            return true;
        }
        return QWidget::event(event);
    }

private:
    TextEditorWidget *textEdit;
};

class BaseTextEditorPrivate
{
public:
    BaseTextEditorPrivate() = default;

    TextEditorFactoryPrivate *m_origin = nullptr;
    QByteArray m_savedNavigationState;
};

class HoverHandlerRunner
{
public:
    using Callback = std::function<void(TextEditorWidget *, BaseHoverHandler *, int)>;
    using FallbackCallback = std::function<void(TextEditorWidget *)>;

    HoverHandlerRunner(TextEditorWidget *widget, QList<BaseHoverHandler *> &handlers)
        : m_widget(widget)
        , m_handlers(handlers)
    {
    }

    ~HoverHandlerRunner() { abortHandlers(); }

    void startChecking(const QTextCursor &textCursor, const Callback &callback, const FallbackCallback &fallbackCallback)
    {
        if (m_handlers.empty()) {
            fallbackCallback(m_widget);
            return;
        }

        // Does the last handler still applies?
        const int documentRevision = textCursor.document()->revision();
        const int position = Text::wordStartCursor(textCursor).position();
        if (m_lastHandlerInfo.applies(documentRevision, position, m_widget)) {
            callback(m_widget, m_lastHandlerInfo.handler, position);
            return;
        }

        if (isCheckRunning(documentRevision, position))
            return;

        // Update invocation data
        m_documentRevision = documentRevision;
        m_position = position;
        m_callback = callback;
        m_fallbackCallback = fallbackCallback;

        restart();
    }

    bool isCheckRunning(int documentRevision, int position) const
    {
        return m_currentHandlerIndex >= 0
            && m_documentRevision == documentRevision
            && m_position == position;
    }

    void checkNext()
    {
        QTC_ASSERT(m_currentHandlerIndex >= 0, return);
        QTC_ASSERT(m_currentHandlerIndex < m_handlers.size(), return);
        BaseHoverHandler *currentHandler = m_handlers[m_currentHandlerIndex];

        currentHandler->checkPriority(m_widget, m_position, [this](int priority) {
            onHandlerFinished(m_documentRevision, m_position, priority);
        });
    }

    void onHandlerFinished(int documentRevision, int position, int priority)
    {
        QTC_ASSERT(m_currentHandlerIndex >= 0, return);
        QTC_ASSERT(m_currentHandlerIndex < m_handlers.size(), return);
        QTC_ASSERT(documentRevision == m_documentRevision, return);
        QTC_ASSERT(position == m_position, return);

        BaseHoverHandler *currentHandler = m_handlers[m_currentHandlerIndex];
        if (priority > m_highestHandlerPriority) {
            m_highestHandlerPriority = priority;
            m_bestHandler = currentHandler;
        }

        // There are more, check next
        ++m_currentHandlerIndex;
        if (m_currentHandlerIndex < m_handlers.size()) {
            checkNext();
            return;
        }
        m_currentHandlerIndex = -1;

        // All were queried, run the best
        if (m_bestHandler) {
            m_lastHandlerInfo = LastHandlerInfo(m_bestHandler, m_documentRevision, m_position);
            m_callback(m_widget, m_bestHandler, m_position);
        } else {
            m_fallbackCallback(m_widget);
        }
    }

    void handlerRemoved(BaseHoverHandler *handler)
    {
        if (m_lastHandlerInfo.handler == handler)
            m_lastHandlerInfo = LastHandlerInfo();
        if (m_currentHandlerIndex >= 0)
            restart();
    }

    void abortHandlers()
    {
        for (BaseHoverHandler *handler : m_handlers)
            handler->abort();
        m_currentHandlerIndex = -1;
    }

private:
    void restart()
    {
        abortHandlers();

        if (m_handlers.empty())
            return;

        // Re-initialize process data
        m_currentHandlerIndex = 0;
        m_bestHandler = nullptr;
        m_highestHandlerPriority = BaseHoverHandler::Priority_None;

        // Start checking
        checkNext();
    }

    TextEditorWidget *m_widget;
    const QList<BaseHoverHandler *> &m_handlers;

    struct LastHandlerInfo {
        LastHandlerInfo() = default;
        LastHandlerInfo(BaseHoverHandler *handler, int documentRevision, int cursorPosition)
            : handler(handler)
            , documentRevision(documentRevision)
            , cursorPosition(cursorPosition)
        {}

        bool applies(int documentRevision, int cursorPosition, TextEditorWidget *widget) const
        {
            return handler
                && handler->lastHelpItemAppliesTo(widget)
                && documentRevision == this->documentRevision
                && cursorPosition == this->cursorPosition;
        }

        BaseHoverHandler *handler = nullptr;
        int documentRevision = -1;
        int cursorPosition = -1;
    } m_lastHandlerInfo;

    // invocation data
    Callback m_callback;
    FallbackCallback m_fallbackCallback;
    int m_position = -1;
    int m_documentRevision = -1;

    // processing data
    int m_currentHandlerIndex = -1;
    int m_highestHandlerPriority = BaseHoverHandler::Priority_None;
    BaseHoverHandler *m_bestHandler = nullptr;
};

struct CursorData
{
    QTextLayout *layout = nullptr;
    QPointF offset;
    int pos = 0;
    QPen pen;
};

struct PaintEventData
{
    PaintEventData(TextEditorWidget *editor, QPaintEvent *event, QPointF offset)
        : offset(offset)
        , viewportRect(editor->viewport()->rect())
        , eventRect(event->rect())
        , doc(editor->document())
        , documentLayout(qobject_cast<TextDocumentLayout *>(doc->documentLayout()))
        , documentWidth(int(doc->size().width()))
        , textCursor(editor->textCursor())
        , isEditable(!editor->isReadOnly())
        , fontSettings(editor->textDocument()->fontSettings())
        , lineSpacing(fontSettings.lineSpacing())
        , searchScopeFormat(fontSettings.toTextCharFormat(C_SEARCH_SCOPE))
        , searchResultFormat(fontSettings.toTextCharFormat(C_SEARCH_RESULT))
        , visualWhitespaceFormat(fontSettings.toTextCharFormat(C_VISUAL_WHITESPACE))
        , ifdefedOutFormat(fontSettings.toTextCharFormat(C_DISABLED_CODE))
        , suppressSyntaxInIfdefedOutBlock(ifdefedOutFormat.foreground()
                                          != fontSettings.toTextCharFormat(C_TEXT).foreground())
        , tabSettings(editor->textDocument()->tabSettings())

    { }
    QPointF offset;
    const QRect viewportRect;
    const QRect eventRect;
    qreal rightMargin = -1;
    const QTextDocument *doc;
    TextDocumentLayout *documentLayout;
    const int documentWidth;
    const QTextCursor textCursor;
    const bool isEditable;
    const FontSettings fontSettings;
    const int lineSpacing;
    const QTextCharFormat searchScopeFormat;
    const QTextCharFormat searchResultFormat;
    const QTextCharFormat visualWhitespaceFormat;
    const QTextCharFormat ifdefedOutFormat;
    const bool suppressSyntaxInIfdefedOutBlock;
    QAbstractTextDocumentLayout::PaintContext context;
    QTextBlock visibleCollapsedBlock;
    QPointF visibleCollapsedBlockOffset;
    QTextBlock block;
    QList<CursorData> cursors;
    const TabSettings tabSettings;
};

struct PaintEventBlockData
{
    QRectF boundingRect;
    QList<QTextLayout::FormatRange> selections;
    QTextLayout *layout = nullptr;
    int position = 0;
    int length = 0;
};

struct ExtraAreaPaintEventData;

struct TextEditorPrivateHighlightBlocks
{
    QList<int> open;
    QList<int> close;
    QList<int> visualIndent;
    inline int count() const { return visualIndent.size(); }
    inline bool isEmpty() const { return open.isEmpty() || close.isEmpty() || visualIndent.isEmpty(); }
    inline bool operator==(const TextEditorPrivateHighlightBlocks &o) const {
        return (open == o.open && close == o.close && visualIndent == o.visualIndent);
    }
    inline bool operator!=(const TextEditorPrivateHighlightBlocks &o) const { return !(*this == o); }
};

class TextEditorWidgetPrivate : public QObject
{
public:
    TextEditorWidgetPrivate(TextEditorWidget *parent);
    ~TextEditorWidgetPrivate() override;

    void updateLineSelectionColor();

    void print(QPrinter *printer);

    void maybeSelectLine();
    void duplicateSelection(bool comment);
    void updateCannotDecodeInfo();
    void collectToCircularClipboard();
    void setClipboardSelection();

    void setDocument(const QSharedPointer<TextDocument> &doc);
    void handleHomeKey(bool anchor, bool block);
    void handleBackspaceKey();
    void moveLineUpDown(bool up);
    void copyLineUpDown(bool up);
    void addSelectionNextFindMatch();
    void addCursorsToLineEnds();
    void saveCurrentCursorPositionForNavigation();
    void updateHighlights();
    void updateCurrentLineInScrollbar();
    void updateCurrentLineHighlight();
    int indentDepthForBlock(const QTextBlock &block, const PaintEventData &data);

    void drawFoldingMarker(QPainter *painter, const QPalette &pal,
                           const QRect &rect,
                           bool expanded,
                           bool active,
                           bool hovered) const;
    bool updateAnnotationBounds(const QTextBlock &block, TextDocumentLayout *layout,
                                bool annotationsVisible);
    void updateLineAnnotation(const PaintEventData &data, const PaintEventBlockData &blockData,
                              QPainter &painter);
    void paintRightMarginArea(PaintEventData &data, QPainter &painter) const;
    void paintRightMarginLine(const PaintEventData &data, QPainter &painter) const;
    void paintBlockHighlight(const PaintEventData &data, QPainter &painter) const;
    void paintSearchResultOverlay(const PaintEventData &data, QPainter &painter) const;
    void paintSelectionOverlay(const PaintEventData &data, QPainter &painter) const;
    void paintIfDefedOutBlocks(const PaintEventData &data, QPainter &painter) const;
    void paintFindScope(const PaintEventData &data, QPainter &painter) const;
    void paintCurrentLineHighlight(const PaintEventData &data, QPainter &painter) const;
    QRectF cursorBlockRect(const QTextDocument *doc,
                           const QTextBlock &block,
                           int cursorPosition,
                           QRectF blockBoundingRect = {},
                           bool *doSelection = nullptr) const;
    void paintCursorAsBlock(const PaintEventData &data,
                            QPainter &painter,
                            PaintEventBlockData &blockData,
                            int cursorPosition) const;
    void paintAdditionalVisualWhitespaces(PaintEventData &data, QPainter &painter, qreal top) const;
    void paintIndentDepth(PaintEventData &data, QPainter &painter, const PaintEventBlockData &blockData);
    void paintReplacement(PaintEventData &data, QPainter &painter, qreal top) const;
    void paintWidgetBackground(const PaintEventData &data, QPainter &painter) const;
    void paintOverlays(const PaintEventData &data, QPainter &painter) const;
    void paintCursor(const PaintEventData &data, QPainter &painter) const;

    void setupBlockLayout(const PaintEventData &data, QPainter &painter,
                          PaintEventBlockData &blockData) const;
    void setupSelections(const PaintEventData &data, PaintEventBlockData &blockData) const;
    void addCursorsPosition(PaintEventData &data,
                           QPainter &painter,
                           const PaintEventBlockData &blockData) const;
    static QTextBlock nextVisibleBlock(const QTextBlock &block);
    void scheduleCleanupAnnotationCache();
    void cleanupAnnotationCache();

    // extra area paint methods
    void paintLineNumbers(QPainter &painter, const ExtraAreaPaintEventData &data,
                          const QRectF &blockBoundingRect) const;
    void paintTextMarks(QPainter &painter, const ExtraAreaPaintEventData &data,
                        const QRectF &blockBoundingRect) const;
    void paintCodeFolding(QPainter &painter, const ExtraAreaPaintEventData &data,
                          const QRectF &blockBoundingRect) const;
    void paintRevisionMarker(QPainter &painter, const ExtraAreaPaintEventData &data,
                             const QRectF &blockBoundingRect) const;

    void toggleBlockVisible(const QTextBlock &block);
    QRect foldBox();

    QTextBlock foldedBlockAt(const QPoint &pos, QRect *box = nullptr) const;

    bool isMouseNavigationEvent(QMouseEvent *e) const;
    void requestUpdateLink(QMouseEvent *e);
    void updateLink();
    void showLink(const Utils::Link &);
    void clearLink();

    void universalHelper(); // test function for development

    bool cursorMoveKeyEvent(QKeyEvent *e);

    void processTooltipRequest(const QTextCursor &c);
    bool processAnnotaionTooltipRequest(const QTextBlock &block, const QPoint &pos) const;
    void showTextMarksToolTip(const QPoint &pos,
                              const TextMarks &marks,
                              const TextMark *mainTextMark = nullptr) const;

    void transformSelection(TransformationMethod method);

    void slotUpdateExtraAreaWidth(std::optional<int> width = {});
    void slotUpdateBlockCount();
    void slotUpdateRequest(const QRect &r, int dy);
    void slotUpdateBlockNotify(const QTextBlock &);
    void updateTabStops();
    void applyTabSettings();
    void applyFontSettingsDelayed();
    void markRemoved(TextMark *mark);

    void editorContentsChange(int position, int charsRemoved, int charsAdded);
    void documentAboutToBeReloaded();
    void documentReloadFinished(bool success);
    void highlightSearchResultsSlot(const QString &txt, FindFlags findFlags);
    void setupScrollBar();
    void highlightSearchResultsInScrollBar();
    void scheduleUpdateHighlightScrollBar();
    void slotFoldChanged(const int blockNumber, bool folded);
    void updateHighlightScrollBarNow();
    struct SearchResult {
        int start;
        int length;
    };
    void addSearchResultsToScrollBar(
        const Id &category,
        const QList<SearchResult> &results,
        Theme::Color color,
        Highlight::Priority prio);
    void addSearchResultsToScrollBar(const QList<SearchResult> &results);
    void addSelectionHighlightToScrollBar(const QList<SearchResult> &selections);
    void adjustScrollBarRanges();

    void setFindScope(const MultiTextCursor &scope);

    void updateCursorPosition();

    // parentheses matcher
    void _q_matchParentheses();
    void _q_highlightBlocks();
    void autocompleterHighlight(const QTextCursor &cursor = QTextCursor());
    void updateAnimator(QPointer<TextEditorAnimator> animator, QPainter &painter);
    void cancelCurrentAnimations();
    void slotSelectionChanged();
    void _q_animateUpdate(const QTextCursor &cursor, QPointF lastPos, QRectF rect);
    void updateCodeFoldingVisible();
    void updateFileLineEndingVisible();
    void updateTabSettingsButtonVisible();

    void reconfigure();
    void updateSyntaxInfoBar(const HighlighterHelper::Definitions &definitions, const QString &fileName);
    void removeSyntaxInfoBar();
    void configureGenericHighlighter(const KSyntaxHighlighting::Definition &definition);
    void setupFromDefinition(const KSyntaxHighlighting::Definition &definition);
    KSyntaxHighlighting::Definition currentDefinition();
    void rememberCurrentSyntaxDefinition();
    void openLinkUnderCursor(bool openInNextSplit);
    void openTypeUnderCursor(bool openInNextSplit);
    qreal charWidth() const;

    std::unique_ptr<EmbeddedWidgetInterface> insertWidget(QWidget *widget, int pos);
    void forceUpdateScrollbarSize();

    // actions
    void registerActions();
    void updateActions();
    void updateOptionalActions();
    void updateRedoAction();
    void updateUndoAction();
    void updateCopyAction(bool on);
    void updatePasteAction();

public:
    TextEditorWidget *q;
    QWidget *m_toolBarWidget = nullptr;
    QToolBar *m_toolBar = nullptr;
    QWidget *m_stretchWidget = nullptr;
    QAction *m_stretchAction = nullptr;
    QAction *m_toolbarOutlineAction = nullptr;
    LineColumnButton *m_cursorPositionButton = nullptr;
    TabSettingsButton *m_tabSettingsButton = nullptr;
    QToolButton *m_fileEncodingButton = nullptr;
    QAction *m_fileEncodingLabelAction = nullptr;
    TextEditorWidgetFind *m_find = nullptr;

    QToolButton *m_fileLineEnding = nullptr;
    QAction *m_fileLineEndingAction = nullptr;

    uint m_optionalActionMask = OptionalActions::None;
    bool m_contentsChanged = false;
    bool m_lastCursorChangeWasInteresting = false;
    std::shared_ptr<void> m_suggestionBlocker;

    QSharedPointer<TextDocument> m_document;
    QList<QMetaObject::Connection> m_documentConnections;
    QByteArray m_tempState;

    bool m_parenthesesMatchingEnabled = false;
    QTimer m_parenthesesMatchingTimer;

    QWidget *m_extraArea = nullptr;

    Id m_tabSettingsId;
    DisplaySettings m_displaySettings;
    bool m_annotationsrRight = true;
    MarginSettings m_marginSettings;
    // apply when making visible the first time, for the split case
    bool m_fontSettingsNeedsApply = true;
    bool m_wasNotYetShown = true;
    BehaviorSettings m_behaviorSettings;

    int extraAreaSelectionAnchorBlockNumber = -1;
    int extraAreaToggleMarkBlockNumber = -1;
    int extraAreaHighlightFoldedBlockNumber = -1;
    int extraAreaPreviousMarkTooltipRequestedLine = -1;

    TextEditorOverlay *m_overlay = nullptr;
    SnippetOverlay *m_snippetOverlay = nullptr;
    TextEditorOverlay *m_searchResultOverlay = nullptr;
    TextEditorOverlay *m_selectionHighlightOverlay = nullptr;
    bool snippetCheckCursor(const QTextCursor &cursor);
    void snippetTabOrBacktab(bool forward);
    QSet<int> m_foldedBlockCache;

    struct AnnotationRect
    {
        QRectF rect;
        const TextMark *mark;
        friend bool operator==(const AnnotationRect &a, const AnnotationRect &b)
        { return a.mark == b.mark && a.rect == b.rect; }
    };
    bool cleanupAnnotationRectsScheduled = false;
    QMap<int, QList<AnnotationRect>> m_annotationRects;
    QRectF getLastLineLineRect(const QTextBlock &block);

    RefactorOverlay *m_refactorOverlay = nullptr;
    HelpItem m_contextHelpItem;

    QBasicTimer foldedBlockTimer;
    int visibleFoldedBlockNumber = -1;
    int suggestedVisibleFoldedBlockNumber = -1;
    void clearVisibleFoldedBlock();
    bool m_mouseOnFoldedMarker = false;
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
    uint m_maybeFakeTooltipEvent : 1;
    int m_visibleWrapColumn = 0;

    Utils::Link m_currentLink;
    bool m_linkPressed = false;
    QTextCursor m_pendingLinkUpdate;
    QTextCursor m_lastLinkUpdate;

    QRegularExpression m_searchExpr;
    QString m_findText;
    FindFlags m_findFlags;
    void highlightSearchResults(const QTextBlock &block, const PaintEventData &data) const;
    void highlightSelection(const QTextBlock &block, const PaintEventData &data) const;
    QTimer m_delayedUpdateTimer;

    void setExtraSelections(Utils::Id kind, const QList<QTextEdit::ExtraSelection> &selections);
    QHash<Utils::Id, QList<QTextEdit::ExtraSelection>> m_extraSelections;

    void startCursorFlashTimer();
    void resetCursorFlashTimer();
    QBasicTimer m_cursorFlashTimer;
    bool m_cursorVisible = false;
    bool m_moveLineUndoHack = false;
    void updateCursorSelections();
    void moveCursor(QTextCursor::MoveOperation operation,
                    QTextCursor::MoveMode mode = QTextCursor::MoveAnchor);
    QRect cursorUpdateRect(const MultiTextCursor &cursor);

    Utils::MultiTextCursor m_findScope;

    QTextCursor m_selectBlockAnchor;

    void moveCursorVisible(bool ensureVisible = true);

    int visualIndent(const QTextBlock &block) const;
    TextEditorPrivateHighlightBlocks m_highlightBlocksInfo;
    QTimer m_highlightBlocksTimer;

    CodeAssistant m_codeAssistant;

    QList<BaseHoverHandler *> m_hoverHandlers; // Not owned
    HoverHandlerRunner m_hoverHandlerRunner;

    QPointer<QSequentialAnimationGroup> m_navigationAnimation;

    QPointer<TextEditorAnimator> m_bracketsAnimator;

    // Animation and highlighting of auto completed text
    QPointer<TextEditorAnimator> m_autocompleteAnimator;
    bool m_animateAutoComplete = true;
    bool m_highlightAutoComplete = true;
    bool m_skipAutoCompletedText = true;
    bool m_skipFormatOnPaste = false;
    bool m_removeAutoCompletedText = true;
    bool m_keepAutoCompletionHighlight = false;
    QTextCursor m_autoCompleteHighlightPos;
    void updateAutoCompleteHighlight();

    QSet<int> m_cursorBlockNumbers;
    int m_blockCount = 0;

    QPoint m_markDragStart;
    bool m_markDragging = false;
    QCursor m_markDragCursor;
    TextMark* m_dragMark = nullptr;
    QTextCursor m_dndCursor;

    QScopedPointer<AutoCompleter> m_autoCompleter;
    CommentDefinition m_commentDefinition;

    QFuture<SearchResultItems> m_searchFuture;
    QFuture<SearchResultItems> m_selectionHighlightFuture;
    QList<SearchResult> m_searchResults;
    QList<SearchResult> m_selectionResults;
    QTimer m_scrollBarUpdateTimer;
    HighlightScrollBarController *m_highlightScrollBarController = nullptr;
    bool m_scrollBarUpdateScheduled = false;

    const MultiTextCursor m_cursors;
    struct BlockSelection
    {
        int blockNumber = -1;
        int column = -1;
        int anchorBlockNumber = -1;
        int anchorColumn = -1;
    };
    QList<BlockSelection> m_blockSelections;
    QList<QTextCursor> generateCursorsForBlockSelection(const BlockSelection &blockSelection);
    void initBlockSelection();
    void clearBlockSelection();
    void handleMoveBlockSelection(QTextCursor::MoveOperation op);

    class UndoCursor
    {
    public:
        int position = 0;
        int anchor = 0;
    };
    using UndoMultiCursor = QList<UndoCursor>;
    QStack<UndoMultiCursor> m_undoCursorStack;
    QList<int> m_visualIndentCache;
    int m_visualIndentOffset = 0;

    void insertSuggestion(std::unique_ptr<TextSuggestion> &&suggestion);
    void updateSuggestion();
    void clearCurrentSuggestion();
    QTextBlock m_suggestionBlock;
    int m_numEmbeddedWidgets = 0;

    Context m_editorContext;
    QAction *m_undoAction = nullptr;
    QAction *m_redoAction = nullptr;
    QAction *m_copyAction = nullptr;
    QAction *m_copyHtmlAction = nullptr;
    QAction *m_cutAction = nullptr;
    QAction *m_pasteAction = nullptr;
    QAction *m_autoIndentAction = nullptr;
    QAction *m_autoFormatAction = nullptr;
    QAction *m_visualizeWhitespaceAction = nullptr;
    QAction *m_textWrappingAction = nullptr;
    QAction *m_unCommentSelectionAction = nullptr;
    QAction *m_unfoldAllAction = nullptr;
    QAction *m_followSymbolAction = nullptr;
    QAction *m_followSymbolInNextSplitAction = nullptr;
    QAction *m_followToTypeAction = nullptr;
    QAction *m_followToTypeInNextSplitAction = nullptr;
    QAction *m_findUsageAction = nullptr;
    QAction *m_openCallHierarchyAction = nullptr;
    QAction *m_openTypeHierarchyAction = nullptr;
    QAction *m_renameSymbolAction = nullptr;
    QAction *m_jumpToFileAction = nullptr;
    QAction *m_jumpToFileInNextSplitAction = nullptr;
    QList<QAction *> m_modifyingActions;
    bool m_updatePasteActionScheduled = false;
};

class TextEditorWidgetFind : public BaseTextFind<TextEditorWidget>
{
public:
    TextEditorWidgetFind(TextEditorWidget *editor)
        : BaseTextFind(editor)
        , m_editor(editor)
    {
        setMultiTextCursorProvider([editor]() { return editor->multiTextCursor(); });
    }

    ~TextEditorWidgetFind() override { cancelCurrentSelectAll(); }

    bool supportsSelectAll() const override { return true; }
    void selectAll(const QString &txt, FindFlags findFlags) override;

    static void cancelCurrentSelectAll();

private:
    TextEditorWidget * const m_editor;
    static QFutureWatcher<SearchResultItems> *m_selectWatcher;
};

static QTextCursor selectRange(QTextDocument *textDocument, const Text::Range &range,
                               TextEditorWidgetPrivate::SearchResult *searchResult = nullptr)
{
    const int startLine = qMax(range.begin.line - 1, 0);
    const int startColumn = qMax(range.begin.column, 0);
    const int endLine = qMax(range.end.line - 1, 0);
    const int endColumn = qMax(range.end.column, 0);
    const int startPosition = textDocument->findBlockByNumber(startLine).position() + startColumn;
    const int endPosition = textDocument->findBlockByNumber(endLine).position() + endColumn;
    QTextCursor textCursor(textDocument);
    textCursor.setPosition(startPosition);
    textCursor.setPosition(endPosition, QTextCursor::KeepAnchor);
    if (searchResult)
        *searchResult = {startPosition + 1, endPosition + 1};
    return textCursor;
}

QFutureWatcher<SearchResultItems> *TextEditorWidgetFind::m_selectWatcher = nullptr;

void TextEditorWidgetFind::selectAll(const QString &txt, FindFlags findFlags)
{
    if (txt.isEmpty())
        return;

    cancelCurrentSelectAll();

    m_selectWatcher = new QFutureWatcher<SearchResultItems>();
    connect(m_selectWatcher, &QFutureWatcher<SearchResultItems>::finished, this, [this] {
        const QFuture<SearchResultItems> future = m_selectWatcher->future();
        m_selectWatcher->deleteLater();
        m_selectWatcher = nullptr;
        if (future.resultCount() <= 0)
            return;
        const SearchResultItems &results = future.result();
        if (results.isEmpty())
            return;
        const auto cursorForResult = [this](const SearchResultItem &item) {
            return selectRange(m_editor->document(), item.mainRange());
        };
        QList<QTextCursor> cursors = Utils::transform(results, cursorForResult);
        cursors = Utils::filtered(cursors, [this](const QTextCursor &c) {
            return m_editor->inFindScope(c);
        });
        m_editor->setMultiTextCursor(MultiTextCursor(cursors));
        m_editor->setFocus();
    });

    m_selectWatcher->setFuture(Utils::asyncRun(Utils::searchInContents, txt, findFlags,
                                               m_editor->textDocument()->filePath(),
                                               m_editor->textDocument()->plainText()));
}

void TextEditorWidgetFind::cancelCurrentSelectAll()
{
    if (m_selectWatcher) {
        m_selectWatcher->disconnect();
        m_selectWatcher->cancel();
        m_selectWatcher->deleteLater();
        m_selectWatcher = nullptr;
    }
}

TextEditorWidgetPrivate::TextEditorWidgetPrivate(TextEditorWidget *parent)
    : q(parent)
    , m_suggestionBlocker((void *) this, [](void *) {})
    , m_overlay(new TextEditorOverlay(q))
    , m_snippetOverlay(new SnippetOverlay(q))
    , m_searchResultOverlay(new TextEditorOverlay(q))
    , m_selectionHighlightOverlay(new TextEditorOverlay(q))
    , m_refactorOverlay(new RefactorOverlay(q))
    , m_marksVisible(false)
    , m_codeFoldingVisible(false)
    , m_codeFoldingSupported(false)
    , m_revisionsVisible(false)
    , m_lineNumbersVisible(true)
    , m_highlightCurrentLine(true)
    , m_requestMarkEnabled(true)
    , m_lineSeparatorsAllowed(false)
    , m_maybeFakeTooltipEvent(false)
    , m_codeAssistant(parent)
    , m_hoverHandlerRunner(parent, m_hoverHandlers)
    , m_autoCompleter(new AutoCompleter)
    , m_editorContext(Id::generate())
{
    m_selectionHighlightOverlay->show();
    m_find = new TextEditorWidgetFind(q);
    connect(
        m_find,
        &BaseTextFindBase::highlightAllRequested,
        this,
        &TextEditorWidgetPrivate::highlightSearchResultsSlot);
    connect(m_find, &BaseTextFindBase::findScopeChanged, this, &TextEditorWidgetPrivate::setFindScope);
    Aggregation::aggregate({q, m_find});

    m_extraArea = new TextEditExtraArea(q);
    m_extraArea->setMouseTracking(true);

    auto toolBarLayout = new QHBoxLayout;
    toolBarLayout->setContentsMargins(0, 0, 0, 0);
    toolBarLayout->setSpacing(0);
    m_toolBarWidget = new Utils::StyledBar;
    m_toolBarWidget->setLayout(toolBarLayout);
    m_stretchWidget = new QWidget;
    m_stretchWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_toolBar = new QToolBar;
    m_toolBar->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    m_stretchAction = m_toolBar->addWidget(m_stretchWidget);
    m_toolBarWidget->layout()->addWidget(m_toolBar);

    const int spacing = q->style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing) / 2;
    m_cursorPositionButton = new LineColumnButton(q);
    m_cursorPositionButton->setContentsMargins(spacing, 0, spacing, 0);
    m_toolBarWidget->layout()->addWidget(m_cursorPositionButton);

    m_tabSettingsButton = new TabSettingsButton(q);
    m_tabSettingsButton->setContentsMargins(spacing, 0, spacing, 0);
    m_toolBarWidget->layout()->addWidget(m_tabSettingsButton);
    updateTabSettingsButtonVisible();

    m_fileLineEnding = new QToolButton(q);
    m_fileLineEnding->setContentsMargins(spacing, 0, spacing, 0);
    m_fileLineEndingAction = m_toolBar->addWidget(m_fileLineEnding);
    updateFileLineEndingVisible();

    m_fileEncodingButton = new QToolButton;
    m_fileEncodingButton->setContentsMargins(spacing, 0, spacing, 0);
    m_fileEncodingLabelAction = m_toolBar->addWidget(m_fileEncodingButton);

    m_extraSelections.reserve(NExtraSelectionKinds);

    connect(&m_codeAssistant, &CodeAssistant::finished,
            q, &TextEditorWidget::assistFinished);

    connect(q, &PlainTextEdit::blockCountChanged,
            this, &TextEditorWidgetPrivate::slotUpdateBlockCount);

    connect(q, &PlainTextEdit::modificationChanged,
            m_extraArea, QOverload<>::of(&QWidget::update));

    connect(q, &PlainTextEdit::cursorPositionChanged,
            q, &TextEditorWidget::slotCursorPositionChanged);

    connect(q, &PlainTextEdit::cursorPositionChanged,
            this, &TextEditorWidgetPrivate::updateCursorPosition);

    connect(q, &PlainTextEdit::updateRequest,
            this, &TextEditorWidgetPrivate::slotUpdateRequest);

    connect(q, &PlainTextEdit::selectionChanged,
            this, &TextEditorWidgetPrivate::slotSelectionChanged);

    connect(q, &PlainTextEdit::undoAvailable,
            this, &TextEditorWidgetPrivate::updateUndoAction);

    connect(q, &PlainTextEdit::redoAvailable,
            this, &TextEditorWidgetPrivate::updateRedoAction);

    connect(q, &PlainTextEdit::copyAvailable,
            this, &TextEditorWidgetPrivate::updateCopyAction);

    connect(qApp->clipboard(), &QClipboard::dataChanged, this, [this] {
        // selecting text with the mouse can cause the clipboard to change quite often and the
        // check whether the clipboard text is empty in update paste action is potentially
        // expensive. So we only schedule the update if it is not already scheduled.

        if (m_updatePasteActionScheduled)
            return;
        m_updatePasteActionScheduled = true;
        QTimer::singleShot(100, this, &TextEditorWidgetPrivate::updatePasteAction);
    });

    m_parenthesesMatchingTimer.setSingleShot(true);
    m_parenthesesMatchingTimer.setInterval(50);
    connect(&m_parenthesesMatchingTimer, &QTimer::timeout,
            this, &TextEditorWidgetPrivate::_q_matchParentheses);

    m_highlightBlocksTimer.setSingleShot(true);
    connect(&m_highlightBlocksTimer, &QTimer::timeout,
            this, &TextEditorWidgetPrivate::_q_highlightBlocks);

    m_scrollBarUpdateTimer.setSingleShot(true);
    connect(&m_scrollBarUpdateTimer, &QTimer::timeout,
            this, &TextEditorWidgetPrivate::highlightSearchResultsInScrollBar);

    m_delayedUpdateTimer.setSingleShot(true);
    connect(&m_delayedUpdateTimer, &QTimer::timeout,
            q->viewport(), QOverload<>::of(&QWidget::update));

    connect(m_fileEncodingButton, &QToolButton::clicked,
            q, &TextEditorWidget::selectEncoding);

    connect(m_fileLineEnding, &QToolButton::clicked, ActionManager::instance(), [this] {
        QMenu *menu = new QMenu(q);
        menu->setAttribute(Qt::WA_DeleteOnClose);
        menu->addAction(Tr::tr("Unix Line Endings (LF)"),
                        [this] { q->selectLineEnding(TextFileFormat::LFLineTerminator); });
        menu->addAction(Tr::tr("Windows Line Endings (CRLF)"),
                        [this] { q->selectLineEnding(TextFileFormat::CRLFLineTerminator); });
        menu->popup(QCursor::pos());
    });

    TextEditorSettings *settings = TextEditorSettings::instance();

    // Connect to settings change signals
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

    auto context = new Core::IContext(this);
    context->setWidget(q);
    context->setContext(m_editorContext);
    Core::ICore::addContextObject(context);

    registerActions();
    updateActions();
}

TextEditorWidgetPrivate::~TextEditorWidgetPrivate()
{
    QTextDocument *doc = m_document->document();
    QTC_CHECK(doc);
    auto documentLayout = qobject_cast<TextDocumentLayout*>(doc->documentLayout());
    QTC_CHECK(documentLayout);
    QTC_CHECK(m_document.data());
    documentLayout->disconnect(this);
    documentLayout->disconnect(m_extraArea);
    doc->disconnect(this);
    m_document.data()->disconnect(this);
    q->disconnect(documentLayout);
    q->disconnect(this);
    delete m_toolBarWidget;
    delete m_highlightScrollBarController;
    if (m_searchFuture.isRunning())
        m_searchFuture.cancel();
    if (m_selectionHighlightFuture.isRunning())
        m_selectionHighlightFuture.cancel();
}

static QFrame *createSeparator(const QString &styleSheet)
{
    QFrame* separator = new QFrame();
    separator->setStyleSheet(styleSheet);
    separator->setFrameShape(QFrame::HLine);
    QSizePolicy sizePolicy = separator->sizePolicy();
    sizePolicy.setHorizontalPolicy(QSizePolicy::MinimumExpanding);
    separator->setSizePolicy(sizePolicy);

    return separator;
}

static QLayout *createSeparatorLayout()
{
    QString styleSheet = "color: gray";

    QFrame* separator1 = createSeparator(styleSheet);
    QFrame* separator2 = createSeparator(styleSheet);
    auto label = new QLabel(Tr::tr("Other annotations"));
    label->setStyleSheet(styleSheet);

    auto layout = new QHBoxLayout;
    layout->addWidget(separator1);
    layout->addWidget(label);
    layout->addWidget(separator2);

    return layout;
}

void TextEditorWidgetPrivate::showTextMarksToolTip(const QPoint &pos,
                                                   const TextMarks &marks,
                                                   const TextMark *mainTextMark) const
{
    if (!mainTextMark && marks.isEmpty())
        return; // Nothing to show

    TextMarks allMarks = marks;

    auto layout = new QGridLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    if (mainTextMark) {
        mainTextMark->addToToolTipLayout(layout);
        if (allMarks.size() > 1)
            layout->addLayout(createSeparatorLayout(), layout->rowCount(), 0, 1, -1);
    }

    Utils::sort(allMarks, [](const TextMark *mark1, const TextMark *mark2) {
        return mark1->priority() > mark2->priority();
    });

    for (const TextMark *mark : std::as_const(allMarks)) {
        if (mark != mainTextMark)
            mark->addToToolTipLayout(layout);
    }

    layout->addWidget(DisplaySettings::createAnnotationSettingsLink(),
                      layout->rowCount(), 0, 1, -1, Qt::AlignRight);
    ToolTip::show(pos, layout, q);
}

} // namespace Internal

QString TextEditorWidget::plainTextFromSelection(const QTextCursor &cursor) const
{
    // Copy the selected text as plain text
    QString text = cursor.selectedText();
    return TextDocument::convertToPlainText(text);
}

QString TextEditorWidget::plainTextFromSelection(const Utils::MultiTextCursor &cursor) const
{
    return TextDocument::convertToPlainText(cursor.selectedText());
}

static const char kTextBlockMimeType[] = "application/vnd.qtcreator.blocktext";

Id TextEditorWidget::SnippetPlaceholderSelection("TextEdit.SnippetPlaceHolderSelection");
Id TextEditorWidget::CurrentLineSelection("TextEdit.CurrentLineSelection");
Id TextEditorWidget::ParenthesesMatchingSelection("TextEdit.ParenthesesMatchingSelection");
Id TextEditorWidget::AutoCompleteSelection("TextEdit.AutoCompleteSelection");
Id TextEditorWidget::CodeWarningsSelection("TextEdit.CodeWarningsSelection");
Id TextEditorWidget::CodeSemanticsSelection("TextEdit.CodeSemanticsSelection");
Id TextEditorWidget::CursorSelection("TextEdit.CursorSelection");
Id TextEditorWidget::UndefinedSymbolSelection("TextEdit.UndefinedSymbolSelection");
Id TextEditorWidget::UnusedSymbolSelection("TextEdit.UnusedSymbolSelection");
Id TextEditorWidget::OtherSelection("TextEdit.OtherSelection");
Id TextEditorWidget::ObjCSelection("TextEdit.ObjCSelection");
Id TextEditorWidget::DebuggerExceptionSelection("TextEdit.DebuggerExceptionSelection");
Id TextEditorWidget::FakeVimSelection("TextEdit.FakeVimSelection");

TextEditorWidget::TextEditorWidget(QWidget *parent)
    : PlainTextEdit(parent)
{
    // "Needed", as the creation below triggers ChildEvents that are
    // passed to this object's event() which uses 'd'.
    d = std::make_unique<Internal::TextEditorWidgetPrivate>(this);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setLayoutDirection(Qt::LeftToRight);
    viewport()->setMouseTracking(true);
    setFrameStyle(QFrame::NoFrame);
}

TextEditorWidget::~TextEditorWidget()
{
    abortAssist();
}

void TextEditorWidget::setTextDocument(const QSharedPointer<TextDocument> &doc)
{
    d->setDocument(doc);
}

void TextEditorWidgetPrivate::setupScrollBar()
{
    if (m_displaySettings.m_scrollBarHighlights) {
        if (!m_highlightScrollBarController)
            m_highlightScrollBarController = new HighlightScrollBarController();

        m_highlightScrollBarController->setScrollArea(q);
        highlightSearchResultsInScrollBar();
        scheduleUpdateHighlightScrollBar();
    } else if (m_highlightScrollBarController) {
        delete m_highlightScrollBarController;
        m_highlightScrollBarController = nullptr;
    }
}

void TextEditorWidgetPrivate::setDocument(const QSharedPointer<TextDocument> &doc)
{
    QSharedPointer<TextDocument> previousDocument = m_document;
    for (const QMetaObject::Connection &connection : std::as_const(m_documentConnections))
        disconnect(connection);
    m_documentConnections.clear();

    m_document = doc;
    q->PlainTextEdit::setDocument(doc->document());
    m_tabSettingsButton->setDocument(q->textDocument());
    previousDocument.clear();
    q->setCursorWidth(2); // Applies to the document layout

    auto documentLayout = qobject_cast<TextDocumentLayout *>(
        m_document->document()->documentLayout());
    QTC_CHECK(documentLayout);

    m_documentConnections << connect(documentLayout,
                                     &TextDocumentLayout::updateBlock,
                                     this,
                                     &TextEditorWidgetPrivate::slotUpdateBlockNotify);

    m_documentConnections << connect(documentLayout,
                                     &TextDocumentLayout::updateExtraArea,
                                     m_extraArea,
                                     QOverload<>::of(&QWidget::update));

    m_documentConnections << connect(documentLayout,
                                     &TextDocumentLayout::foldChanged,
                                     this,
                                     &TextEditorWidgetPrivate::slotFoldChanged);

    m_documentConnections << connect(q,
                                     &TextEditorWidget::requestBlockUpdate,
                                     documentLayout,
                                     &TextDocumentLayout::updateBlock);

    m_documentConnections << connect(documentLayout,
                                     &TextDocumentLayout::updateExtraArea,
                                     this,
                                     &TextEditorWidgetPrivate::scheduleUpdateHighlightScrollBar);

    m_documentConnections << connect(documentLayout,
                                     &TextDocumentLayout::parenthesesChanged,
                                     &m_parenthesesMatchingTimer,
                                     QOverload<>::of(&QTimer::start));

    m_documentConnections << connect(documentLayout,
                                     &QAbstractTextDocumentLayout::documentSizeChanged,
                                     this,
                                     &TextEditorWidgetPrivate::scheduleUpdateHighlightScrollBar);

    m_documentConnections << connect(documentLayout,
                                     &QAbstractTextDocumentLayout::update,
                                     this,
                                     &TextEditorWidgetPrivate::scheduleUpdateHighlightScrollBar);

    m_documentConnections << connect(m_document->document(),
                                     &QTextDocument::contentsChange,
                                     this,
                                     &TextEditorWidgetPrivate::editorContentsChange);

    m_documentConnections << connect(m_document->document(),
                                     &QTextDocument::modificationChanged,
                                     q,
                                     &TextEditorWidget::updateTextCodecLabel);

    m_documentConnections << connect(m_document->document(),
                                     &QTextDocument::modificationChanged,
                                     q,
                                     &TextEditorWidget::updateTextLineEndingLabel);

    m_documentConnections << connect(m_document.data(),
                                     &TextDocument::aboutToReload,
                                     this,
                                     &TextEditorWidgetPrivate::documentAboutToBeReloaded);

    m_documentConnections << connect(m_document.data(),
                                     &TextDocument::reloadFinished,
                                     this,
                                     &TextEditorWidgetPrivate::documentReloadFinished);

    m_documentConnections << connect(m_document.data(),
                                     &TextDocument::tabSettingsChanged,
                                     this,
                                     &TextEditorWidgetPrivate::applyTabSettings);

    m_documentConnections << connect(m_document.data(),
                                     &TextDocument::fontSettingsChanged,
                                     this,
                                     &TextEditorWidgetPrivate::applyFontSettingsDelayed);

    m_documentConnections << connect(m_document.data(),
                                     &TextDocument::markRemoved,
                                     this,
                                     &TextEditorWidgetPrivate::markRemoved);

    m_documentConnections << connect(m_document.data(),
                                     &TextDocument::aboutToOpen,
                                     q,
                                     &TextEditorWidget::aboutToOpen);

    m_documentConnections << connect(m_document.data(),
                                     &TextDocument::openFinishedSuccessfully,
                                     q,
                                     &TextEditorWidget::openFinishedSuccessfully);

    m_documentConnections << connect(TextEditorSettings::instance(),
                                     &TextEditorSettings::fontSettingsChanged,
                                     m_document.data(),
                                     &TextDocument::setFontSettings);

    slotUpdateExtraAreaWidth();

    // Apply current settings
    // the document might already have the same settings as we set here in which case we do not
    // get an update, so we have to trigger updates manually here
    const FontSettings fontSettings = TextEditorSettings::fontSettings();
    if (m_document->fontSettings() == fontSettings)
        applyFontSettingsDelayed();
    else
        m_document->setFontSettings(fontSettings);
    const TabSettings tabSettings = TextEditorSettings::codeStyle()->tabSettings();
    if (m_document->tabSettings() == tabSettings)
        applyTabSettings();
    else
        m_document->setTabSettings(tabSettings); // also set through code style ???

    q->setTypingSettings(globalTypingSettings());
    q->setStorageSettings(globalStorageSettings());
    q->setBehaviorSettings(globalBehaviorSettings());
    q->setMarginSettings(TextEditorSettings::marginSettings());
    q->setDisplaySettings(TextEditorSettings::displaySettings());
    q->setCompletionSettings(TextEditorSettings::completionSettings());
    q->setExtraEncodingSettings(globalExtraEncodingSettings());
    q->textDocument()->setCodeStyle(TextEditorSettings::codeStyle(m_tabSettingsId));

    m_blockCount = doc->document()->blockCount();

    // from RESEARCH

    extraAreaSelectionAnchorBlockNumber = -1;
    extraAreaToggleMarkBlockNumber = -1;
    extraAreaHighlightFoldedBlockNumber = -1;
    visibleFoldedBlockNumber = -1;
    suggestedVisibleFoldedBlockNumber = -1;

    if (m_bracketsAnimator)
        m_bracketsAnimator->finish();
    if (m_autocompleteAnimator)
        m_autocompleteAnimator->finish();

    slotUpdateExtraAreaWidth();
    updateHighlights();

    m_moveLineUndoHack = false;

    updateCannotDecodeInfo();

    q->updateTextCodecLabel();
    q->updateTextLineEndingLabel();
    setupFromDefinition(currentDefinition());
}

void TextEditorWidget::print(QPrinter *printer)
{
    const bool oldFullPage = printer->fullPage();
    printer->setFullPage(true);
    auto dlg = new QPrintDialog(printer, this);
    dlg->setWindowTitle(Tr::tr("Print Document"));
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

static int foldBoxWidth()
{
    const int lineSpacing = TextEditorSettings::fontSettings().lineSpacing();
    return lineSpacing + lineSpacing % 2 + 1;
}

static void printPage(int index, QPainter *painter, const QTextDocument *doc,
                      const QRectF &body, const QRectF &titleBox,
                      const QString &title)
{
    painter->save();

    painter->translate(body.left(), body.top() - (index - 1) * body.height());
    const QRectF view(0, (index - 1) * body.height(), body.width(), body.height());

    QAbstractTextDocumentLayout *layout = doc->documentLayout();
    QAbstractTextDocumentLayout::PaintContext ctx;

    painter->setFont(QFont(doc->defaultFont()));
    const QRectF box = titleBox.translated(0, view.top());
    const int dpix = painter->device()->logicalDpiX();
    const int dpiy = painter->device()->logicalDpiY();
    const int mx = int(5 * dpix / 72.0);
    const int my = int(2 * dpiy / 72.0);
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

Q_LOGGING_CATEGORY(printLog, "qtc.editor.print", QtWarningMsg)

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

    QRectF pageRect(printer->pageLayout().paintRectPixels(printer->resolution()));
    if (pageRect.isEmpty())
        return;

    doc = doc->clone(doc);
    const QScopeGuard cleanup([doc] { delete doc; });

    QTextOption opt = doc->defaultTextOption();
    opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    doc->setDefaultTextOption(opt);

    (void)doc->documentLayout(); // make sure that there is a layout


    QColor background = m_document->fontSettings().toTextCharFormat(C_TEXT).background().color();
    bool backgroundIsDark = background.value() < 128;

    for (QTextBlock srcBlock = q->document()->firstBlock(), dstBlock = doc->firstBlock();
         srcBlock.isValid() && dstBlock.isValid();
         srcBlock = srcBlock.next(), dstBlock = dstBlock.next()) {
        QList<QTextLayout::FormatRange> formatList
            = q->editorLayout()->blockLayout(srcBlock)->formats();
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

        q->editorLayout()->blockLayout(dstBlock)->setFormats(formatList);
    }

    QAbstractTextDocumentLayout *layout = doc->documentLayout();
    layout->setPaintDevice(p.device());

    int dpiy = qRound(QGuiApplication::primaryScreen()->logicalDotsPerInchY());
    int margin = int((2/2.54)*dpiy); // 2 cm margins

    QTextFrameFormat fmt = doc->rootFrame()->frameFormat();
    fmt.setMargin(margin);
    doc->rootFrame()->setFrameFormat(fmt);

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
        pageCopies = printer->copyCount();
    } else {
        docCopies = printer->copyCount();
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

    qCDebug(printLog) << "Printing " << m_document->filePath() << ":\n"
                      << "  number of copies:" << printer->copyCount() << '\n'
                      << "  from page" << fromPage << "to" << toPage << '\n'
                      << "  document page count:" << doc->pageCount() << '\n'
                      << "  page rectangle:" << pageRect << '\n'
                      << "  title box:" << titleBox << '\n';

    for (int i = 0; i < docCopies; ++i) {

        int page = fromPage;
        while (true) {
            for (int j = 0; j < pageCopies; ++j) {
                if (printer->printerState() == QPrinter::Aborted
                    || printer->printerState() == QPrinter::Error)
                    return;
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

void TextEditorWidgetPrivate::updateAutoCompleteHighlight()
{
    const QTextCharFormat matchFormat = m_document->fontSettings().toTextCharFormat(C_AUTOCOMPLETE);
    const QTextEdit::ExtraSelection sel = {m_autoCompleteHighlightPos, matchFormat};
    q->setExtraSelections(TextEditorWidget::AutoCompleteSelection, {sel});
}

QList<QTextCursor> TextEditorWidgetPrivate::generateCursorsForBlockSelection(
    const BlockSelection &blockSelection)
{
    const TabSettings tabSettings = m_document->tabSettings();

    QList<QTextCursor> result;
    QTextBlock block = m_document->document()->findBlockByNumber(blockSelection.anchorBlockNumber);
    QTextCursor cursor(block);
    cursor.setPosition(block.position()
                       + tabSettings.positionAtColumn(block.text(), blockSelection.anchorColumn));

    const bool forward = blockSelection.blockNumber > blockSelection.anchorBlockNumber
                         || (blockSelection.blockNumber == blockSelection.anchorBlockNumber
                             && blockSelection.column == blockSelection.anchorColumn);

    while (block.isValid()) {
        const QString &blockText = block.text();
        const int columnCount = tabSettings.columnCountForText(blockText);
        if (blockSelection.anchorColumn <= columnCount || blockSelection.column <= columnCount) {
            const int anchor = tabSettings.positionAtColumn(blockText, blockSelection.anchorColumn);
            const int position = tabSettings.positionAtColumn(blockText, blockSelection.column);
            cursor.setPosition(block.position() + anchor);
            cursor.setPosition(block.position() + position, QTextCursor::KeepAnchor);
            result.append(cursor);
        }
        if (block.blockNumber() == blockSelection.blockNumber)
            break;
        block = forward ? block.next() : block.previous();
    }
    return result;
}

void TextEditorWidgetPrivate::initBlockSelection()
{
    const TabSettings tabSettings = m_document->tabSettings();
    for (const QTextCursor &cursor : m_cursors) {
        const int column = tabSettings.columnAtCursorPosition(cursor);
        QTextCursor anchor = cursor;
        anchor.setPosition(anchor.anchor());
        const int anchorColumn = tabSettings.columnAtCursorPosition(anchor);
        m_blockSelections.append({cursor.blockNumber(), column, anchor.blockNumber(), anchorColumn});
    }
}

void TextEditorWidgetPrivate::clearBlockSelection()
{
    m_blockSelections.clear();
}

void TextEditorWidgetPrivate::handleMoveBlockSelection(QTextCursor::MoveOperation op)
{
    if (m_blockSelections.isEmpty())
        initBlockSelection();
    QList<QTextCursor> cursors;
    for (BlockSelection &blockSelection : m_blockSelections) {
        switch (op) {
        case QTextCursor::Up:
            blockSelection.blockNumber = qMax(0, blockSelection.blockNumber - 1);
            break;
        case QTextCursor::Down:
            blockSelection.blockNumber = qMin(m_document->document()->blockCount() - 1,
                                              blockSelection.blockNumber + 1);
            break;
        case QTextCursor::NextCharacter:
            ++blockSelection.column;
            break;
        case QTextCursor::PreviousCharacter:
            blockSelection.column = qMax(0, blockSelection.column - 1);
            break;
        default:
            return;
        }
        cursors.append(generateCursorsForBlockSelection(blockSelection));
    }
    q->setMultiTextCursor(MultiTextCursor(cursors));
}

void TextEditorWidgetPrivate::insertSuggestion(std::unique_ptr<TextSuggestion> &&suggestion)
{
    clearCurrentSuggestion();

    if (m_suggestionBlocker.use_count() > 1)
        return;

    auto cursor = q->textCursor();
    cursor.setPosition(suggestion->currentPosition());
    QTextOption option = suggestion->replacementDocument()->defaultTextOption();
    option.setTabStopDistance(charWidth() * m_document->tabSettings().m_tabSize);
    suggestion->replacementDocument()->setDefaultTextOption(option);
    auto options = suggestion->replacementDocument()->defaultTextOption();
    m_suggestionBlock = cursor.block();
    TextBlockUserData::insertSuggestion(m_suggestionBlock, std::move(suggestion));
    TextBlockUserData::updateSuggestionFormats(m_suggestionBlock, m_document->fontSettings());
    q->editorLayout()->clearBlockLayout(m_suggestionBlock);
    m_document->updateLayout();

    forceUpdateScrollbarSize();
}

void TextEditorWidgetPrivate::updateSuggestion()
{
    if (!m_suggestionBlock.isValid())
        return;
    const QTextCursor cursor = m_cursors.mainCursor();
    if (cursor.block() == m_suggestionBlock) {
        TextSuggestion *suggestion = TextBlockUserData::suggestion(m_suggestionBlock);
        if (QTC_GUARD(suggestion)) {
            const int pos = cursor.position();
            if (pos >= suggestion->currentPosition()) {
                suggestion->setCurrentPosition(pos);
                if (suggestion->filterSuggestions(q)) {
                    TextBlockUserData::updateSuggestionFormats(
                        m_suggestionBlock, m_document->fontSettings());
                    return;
                }
            }
        }
    }
    clearCurrentSuggestion();
}

void TextEditorWidgetPrivate::clearCurrentSuggestion()
{
    if (!m_suggestionBlock.isValid())
        return;
    TextBlockUserData::clearSuggestion(m_suggestionBlock);
    m_document->updateLayout();
    m_suggestionBlock = QTextBlock();
}

void TextEditorWidget::selectEncoding()
{
    TextDocument *doc = d->m_document.data();
    const CodecSelectorResult result = Core::askForCodec(doc);
    switch (result.action) {
    case Core::CodecSelectorResult::Reload: {
        if (Result<> res = doc->reload(result.encoding); !res) {
            QMessageBox::critical(this, Tr::tr("File Error"), res.error());
            break;
        }
        break;
    }
    case Core::CodecSelectorResult::Save:
        doc->setEncoding(result.encoding);
        EditorManager::saveDocument(textDocument());
        updateTextCodecLabel();
        break;
    case Core::CodecSelectorResult::Cancel:
        break;
    }
}

void TextEditorWidget::selectLineEnding(TextFileFormat::LineTerminationMode lineEnding)
{
    if (d->m_document->lineTerminationMode() != lineEnding) {
        d->m_document->setLineTerminationMode(lineEnding);
        d->q->document()->setModified(true);
        updateTextLineEndingLabel();
    }
}

void TextEditorWidget::updateTextLineEndingLabel()
{
    const TextFileFormat::LineTerminationMode lineEnding = d->m_document->lineTerminationMode();
    if (lineEnding == TextFileFormat::LFLineTerminator)
        d->m_fileLineEnding->setText(Tr::tr("LF"));
    else if (lineEnding == TextFileFormat::CRLFLineTerminator)
        d->m_fileLineEnding->setText(Tr::tr("CRLF"));
    else
        QTC_ASSERT_STRING("Unsupported line ending mode.");
}

void TextEditorWidget::updateTextCodecLabel()
{
    d->m_fileEncodingButton->setText(d->m_document->encoding().displayName());
}

QString TextEditorWidget::msgTextTooLarge(quint64 size)
{
    return Tr::tr("The text is too large to be displayed (%1 MB).").
           arg(size >> 20);
}

void TextEditorWidget::insertPlainText(const QString &text)
{
    MultiTextCursor cursor = d->m_cursors;
    cursor.insertText(text);
    setMultiTextCursor(cursor);
}

QString TextEditorWidget::selectedText() const
{
    return d->m_cursors.selectedText();
}

void TextEditorWidget::setVisualIndentOffset(int offset)
{
    d->m_visualIndentOffset = qMax(0, offset);
}

void TextEditorWidget::updateUndoRedoActions()
{
    d->updateUndoAction();
    d->updateRedoAction();
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
            Tr::tr("<b>Error:</b> Could not decode \"%1\" with \"%2\"-encoding. Editing not possible.")
                .arg(m_document->displayName(), m_document->encoding().displayName()));
        info.addCustomButton(Tr::tr("Select Encoding"), [this] { q->selectEncoding(); });
        infoBar->addInfo(info);
    } else {
        infoBar->removeInfo(selectEncodingId);
    }
}

// Skip over shebang to license header (Python, Perl, sh)
// '#!/bin/sh'
// ''
// '###############'

static QTextBlock skipShebang(const QTextBlock &block)
{
    if (!block.isValid() || !block.text().startsWith("#!"))
        return block;
    const QTextBlock nextBlock1 = block.next();
    if (!nextBlock1.isValid() || !nextBlock1.text().isEmpty())
        return block;
    const QTextBlock nextBlock2 = nextBlock1.next();
    return nextBlock2.isValid() && nextBlock2.text().startsWith('#') ? nextBlock2 : block;
}

/*
  Collapses the first comment in a file, if there is only whitespace/shebang line
  above
  */
void TextEditorWidgetPrivate::foldLicenseHeader()
{
    QTextDocument *doc = q->document();
    auto documentLayout = qobject_cast<TextDocumentLayout*>(doc->documentLayout());
    QTC_ASSERT(documentLayout, return);
    QTextBlock block = skipShebang(doc->firstBlock());
    while (block.isValid() && block.isVisible()) {
        QString text = block.text();
        if (TextBlockUserData::canFold(block) && block.next().isVisible()) {
            const QString trimmedText = text.trimmed();
            QStringList commentMarker;
            QStringList docMarker;
            HighlighterHelper::Definition def;
            if (auto highlighter = qobject_cast<Highlighter *>(q->textDocument()->syntaxHighlighter()))
                def = highlighter->definition();

            if (def.isValid()) {
                for (const QString &marker :
                     {def.singleLineCommentMarker(), def.multiLineCommentMarker().first}) {
                    if (!marker.isEmpty())
                        commentMarker << marker;
                }
            } else {
                commentMarker = QStringList({"/*", "#"});
                docMarker = QStringList({"/*!", "/**"});
            }

            if (Utils::anyOf(commentMarker, [&](const QString &marker) {
                    return trimmedText.startsWith(marker);
                })) {
                if (Utils::anyOf(docMarker, [&](const QString &marker) {
                        return trimmedText.startsWith(marker)
                               && (trimmedText.size() == marker.size()
                                   || trimmedText.at(marker.size()).isSpace());
                    })) {
                    break;
                }
                TextBlockUserData::doFoldOrUnfold(block, false);
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

void TextEditorWidget::aboutToOpen(const Utils::FilePath &filePath, const Utils::FilePath &realFilePath)
{
    Q_UNUSED(filePath)
    Q_UNUSED(realFilePath)
}

void TextEditorWidget::openFinishedSuccessfully()
{
    d->moveCursor(QTextCursor::Start);
    d->updateCannotDecodeInfo();
    updateTextCodecLabel();
    updateVisualWrapColumn();
}

TextDocumentPtr TextEditorWidget::textDocumentPtr() const
{
    return d->m_document;
}

TextEditorWidget *TextEditorWidget::currentTextEditorWidget()
{
    return fromEditor(EditorManager::currentEditor());
}

TextEditorWidget *TextEditorWidget::fromEditor(const IEditor *editor)
{
    if (editor)
        return Aggregation::query<TextEditorWidget>(editor->widget());
    return nullptr;
}

void TextEditorWidgetPrivate::editorContentsChange(int position, int charsRemoved, int charsAdded)
{
    updateSuggestion();

    if (m_bracketsAnimator)
        m_bracketsAnimator->finish();

    m_contentsChanged = true;
    QTextDocument *doc = q->document();
    auto documentLayout = static_cast<TextDocumentLayout*>(doc->documentLayout());
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

    if ((charsAdded != 0 && q->document()->characterAt(position + charsAdded - 1).isPrint()) || charsRemoved != 0)
        m_codeAssistant.notifyChange();

    int newBlockCount = doc->blockCount();
    if (!q->hasFocus() && newBlockCount != m_blockCount) {
        // lines were inserted or removed from outside, keep viewport on same part of text
        if (q->firstVisibleBlock().blockNumber() > posBlock.blockNumber())
            q->verticalScrollBar()->setValue(q->verticalScrollBar()->value() + newBlockCount - m_blockCount);
    }
    m_blockCount = newBlockCount;
    m_scrollBarUpdateTimer.start(500);
    m_visualIndentCache.clear();
}

void TextEditorWidgetPrivate::slotSelectionChanged()
{
    if (!q->textCursor().hasSelection() && !m_selectBlockAnchor.isNull())
        m_selectBlockAnchor = QTextCursor();
    // Clear any link which might be showing when the selection changes
    clearLink();
    setClipboardSelection();
}

void TextEditorWidget::gotoBlockStart()
{
    if (multiTextCursor().hasMultipleCursors())
        return;

    QTextCursor cursor = textCursor();
    if (TextBlockUserData::findPreviousOpenParenthesis(&cursor, false)) {
        setTextCursor(cursor);
        d->_q_matchParentheses();
    }
}

void TextEditorWidget::gotoBlockEnd()
{
    if (multiTextCursor().hasMultipleCursors())
        return;

    QTextCursor cursor = textCursor();
    if (TextBlockUserData::findNextClosingParenthesis(&cursor, false)) {
        setTextCursor(cursor);
        d->_q_matchParentheses();
    }
}

void TextEditorWidget::gotoBlockStartWithSelection()
{
    if (multiTextCursor().hasMultipleCursors())
        return;

    QTextCursor cursor = textCursor();
    if (TextBlockUserData::findPreviousOpenParenthesis(&cursor, true)) {
        setTextCursor(cursor);
        d->_q_matchParentheses();
    }
}

void TextEditorWidget::gotoBlockEndWithSelection()
{
    if (multiTextCursor().hasMultipleCursors())
        return;

    QTextCursor cursor = textCursor();
    if (TextBlockUserData::findNextClosingParenthesis(&cursor, true)) {
        setTextCursor(cursor);
        d->_q_matchParentheses();
    }
}

void TextEditorWidget::gotoDocumentStart()
{
    d->moveCursor(QTextCursor::Start);
}

void TextEditorWidget::gotoDocumentEnd()
{
    d->moveCursor(QTextCursor::End);
}

void TextEditorWidget::gotoLineStart()
{
    d->handleHomeKey(false, true);
}

void TextEditorWidget::gotoLineStartWithSelection()
{
    d->handleHomeKey(true, true);
}

void TextEditorWidget::gotoLineEnd()
{
    d->moveCursor(QTextCursor::EndOfLine);
}

void TextEditorWidget::gotoLineEndWithSelection()
{
    d->moveCursor(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
}

void TextEditorWidget::gotoNextLine()
{
    d->moveCursor(QTextCursor::Down);
}

void TextEditorWidget::gotoNextLineWithSelection()
{
    d->moveCursor(QTextCursor::Down, QTextCursor::KeepAnchor);
}

void TextEditorWidget::gotoPreviousLine()
{
    d->moveCursor(QTextCursor::Up);
}

void TextEditorWidget::gotoPreviousLineWithSelection()
{
    d->moveCursor(QTextCursor::Up, QTextCursor::KeepAnchor);
}

void TextEditorWidget::gotoPreviousCharacter()
{
    d->moveCursor(QTextCursor::PreviousCharacter);
}

void TextEditorWidget::gotoPreviousCharacterWithSelection()
{
    d->moveCursor(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
}

void TextEditorWidget::gotoNextCharacter()
{
    d->moveCursor(QTextCursor::NextCharacter);
}

void TextEditorWidget::gotoNextCharacterWithSelection()
{
    d->moveCursor(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
}

void TextEditorWidget::gotoPreviousWord()
{
    d->moveCursor(QTextCursor::PreviousWord);
}

void TextEditorWidget::gotoPreviousWordWithSelection()
{
    d->moveCursor(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
}

void TextEditorWidget::gotoNextWord()
{
    d->moveCursor(QTextCursor::NextWord);
}

void TextEditorWidget::gotoNextWordWithSelection()
{
    d->moveCursor(QTextCursor::NextWord, QTextCursor::KeepAnchor);
}

void TextEditorWidget::gotoPreviousWordCamelCase()
{
    MultiTextCursor cursor = multiTextCursor();
    CamelCaseCursor::left(&cursor, QTextCursor::MoveAnchor);
    setMultiTextCursor(cursor);
}

void TextEditorWidget::gotoPreviousWordCamelCaseWithSelection()
{
    MultiTextCursor cursor = multiTextCursor();
    CamelCaseCursor::left(&cursor, QTextCursor::KeepAnchor);
    setMultiTextCursor(cursor);
}

void TextEditorWidget::gotoNextWordCamelCase()
{
    MultiTextCursor cursor = multiTextCursor();
    CamelCaseCursor::right(&cursor, QTextCursor::MoveAnchor);
    setMultiTextCursor(cursor);
}

void TextEditorWidget::gotoNextWordCamelCaseWithSelection()
{
    MultiTextCursor cursor = multiTextCursor();
    CamelCaseCursor::right(&cursor, QTextCursor::KeepAnchor);
    setMultiTextCursor(cursor);
}

bool TextEditorWidget::selectBlockUp()
{
    if (multiTextCursor().hasMultipleCursors())
        return false;

    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection())
        d->m_selectBlockAnchor = cursor;
    else
        cursor.setPosition(cursor.selectionStart());

    if (!TextBlockUserData::findPreviousOpenParenthesis(&cursor, false))
        return false;
    if (!TextBlockUserData::findNextClosingParenthesis(&cursor, true))
        return false;

    setTextCursor(Text::flippedCursor(cursor));
    d->_q_matchParentheses();
    return true;
}

bool TextEditorWidget::selectBlockDown()
{
    if (multiTextCursor().hasMultipleCursors())
        return false;

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

    setTextCursor(Text::flippedCursor(cursor));
    d->_q_matchParentheses();
    return true;
}

void TextEditorWidget::selectWordUnderCursor()
{
    MultiTextCursor cursor = multiTextCursor();
    for (QTextCursor &c : cursor) {
        if (!c.hasSelection())
            c.select(QTextCursor::WordUnderCursor);
    }
    setMultiTextCursor(cursor);
}

void TextEditorWidget::clearSelection()
{
    MultiTextCursor cursor = multiTextCursor();
    cursor.clearSelection();
    setMultiTextCursor(cursor);
}

void TextEditorWidget::showContextMenu()
{
    QTextCursor tc = textCursor();
    const QPoint cursorPos = mapToGlobal(cursorRect(tc).bottomRight() + QPoint(1,1));
    qGuiApp->postEvent(
        this, new QContextMenuEvent(QContextMenuEvent::Keyboard, cursorPos, QCursor::pos()));
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
    if (q->multiTextCursor().hasMultipleCursors())
        return;
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
    MultiTextCursor cursor = multiTextCursor();
    cursor.beginEditBlock();
    for (QTextCursor &c : cursor) {
        QTextCursor start = c;
        QTextCursor end = c;

        start.setPosition(c.selectionStart());
        end.setPosition(c.selectionEnd() - 1);

        int lineCount = qMax(1, end.blockNumber() - start.blockNumber());

        c.setPosition(c.selectionStart());
        while (lineCount--) {
            c.movePosition(QTextCursor::NextBlock);
            c.movePosition(QTextCursor::StartOfBlock);
            c.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            QString cutLine = c.selectedText();

            // Collapse leading whitespaces to one or insert whitespace
            static const QRegularExpression regexp("^\\s*");
            cutLine.replace(regexp, QLatin1String(" "));
            c.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
            c.removeSelectedText();

            c.movePosition(QTextCursor::PreviousBlock);
            c.movePosition(QTextCursor::EndOfBlock);

            c.insertText(cutLine);
        }
    }
    cursor.endEditBlock();
    cursor.mergeCursors();
    setMultiTextCursor(cursor);
}

void TextEditorWidget::insertLineAbove()
{
    MultiTextCursor cursor = multiTextCursor();
    cursor.beginEditBlock();
    for (QTextCursor &c : cursor) {
        // If the cursor is at the beginning of the document,
        // it should still insert a line above the current line.
        c.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
        c.insertBlock();
        c.movePosition(QTextCursor::PreviousBlock, QTextCursor::MoveAnchor);
        d->m_document->autoIndent(c);
    }
    cursor.endEditBlock();
    setMultiTextCursor(cursor);
}

void TextEditorWidget::insertLineBelow()
{
    MultiTextCursor cursor = multiTextCursor();
    cursor.beginEditBlock();
    for (QTextCursor &c : cursor) {
        c.movePosition(QTextCursor::EndOfBlock, QTextCursor::MoveAnchor);
        c.insertBlock();
        d->m_document->autoIndent(c);
    }
    cursor.endEditBlock();
    setMultiTextCursor(cursor);
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
    d->transformSelection([](const QString &str) { return str.toUpper(); });
}

void TextEditorWidget::lowercaseSelection()
{
    d->transformSelection([](const QString &str) { return str.toLower(); });
}

void TextEditorWidget::sortLines()
{
    if (d->m_cursors.hasMultipleCursors())
        return;

    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection()) {
        // try to get a sensible scope for the sort
        const QTextBlock currentBlock = cursor.block();
        QString text = currentBlock.text();
        if (text.simplified().isEmpty())
            return;
        const TabSettings ts = textDocument()->tabSettings();
        const int currentIndent = ts.columnAt(text, TabSettings::firstNonSpace(text));

        int anchor = currentBlock.position();
        for (auto block = currentBlock.previous(); block.isValid(); block = block.previous()) {
            text = block.text();
            if (text.simplified().isEmpty()
                || ts.columnAt(text, TabSettings::firstNonSpace(text)) != currentIndent) {
                break;
            }
            anchor = block.position();
        }

        int pos = currentBlock.position();
        for (auto block = currentBlock.next(); block.isValid(); block = block.next()) {
            text = block.text();
            if (text.simplified().isEmpty()
                || ts.columnAt(text, TabSettings::firstNonSpace(text)) != currentIndent) {
                break;
            }
            pos = block.position();
        }
        if (anchor == pos)
            return;

        cursor.setPosition(anchor);
        cursor.setPosition(pos, QTextCursor::KeepAnchor);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    }

    const bool downwardDirection = cursor.anchor() < cursor.position();
    int startPosition = cursor.selectionStart();
    int endPosition = cursor.selectionEnd();

    cursor.setPosition(startPosition);
    cursor.movePosition(QTextCursor::StartOfBlock);
    startPosition = cursor.position();

    cursor.setPosition(endPosition, QTextCursor::KeepAnchor);
    if (cursor.positionInBlock() == 0)
        cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::KeepAnchor);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    endPosition = qMax(cursor.position(), endPosition);

    const QString text = cursor.selectedText();
    QStringList lines = text.split(QChar::ParagraphSeparator);
    lines.sort();
    cursor.insertText(lines.join(QChar::ParagraphSeparator));

    // (re)select the changed lines
    // Note: this assumes the transformation did not change the length
    cursor.setPosition(downwardDirection ? startPosition : endPosition);
    cursor.setPosition(downwardDirection ? endPosition : startPosition, QTextCursor::KeepAnchor);
    setTextCursor(cursor);
}

void TextEditorWidget::indent()
{
    setMultiTextCursor(textDocument()->indent(multiTextCursor()));
}

void TextEditorWidget::unindent()
{
    setMultiTextCursor(textDocument()->unindent(multiTextCursor()));
}

void TextEditorWidget::undo()
{
    doSetTextCursor(multiTextCursor().mainCursor());
    PlainTextEdit::undo();
}

void TextEditorWidget::redo()
{
    doSetTextCursor(multiTextCursor().mainCursor());
    PlainTextEdit::redo();
}

bool TextEditorWidget::isUndoAvailable() const
{
    return document()->isUndoAvailable();
}

bool TextEditorWidget::isRedoAvailable() const
{
    return document()->isRedoAvailable();
}

void TextEditorWidget::openLinkUnderCursor()
{
    d->openLinkUnderCursor(alwaysOpenLinksInNextSplit());
}

void TextEditorWidget::openLinkUnderCursorInNextSplit()
{
    d->openLinkUnderCursor(!alwaysOpenLinksInNextSplit());
}

void TextEditorWidget::openTypeUnderCursor()
{
    d->openTypeUnderCursor(alwaysOpenLinksInNextSplit());
}

void TextEditorWidget::openTypeUnderCursorInNextSplit()
{
    d->openTypeUnderCursor(!alwaysOpenLinksInNextSplit());
}

void TextEditorWidget::findUsages()
{
    emit requestUsages(textCursor());
}

void TextEditorWidget::renameSymbolUnderCursor()
{
    emit requestRename(textCursor());
}

void TextEditorWidget::openCallHierarchy()
{
    emit requestCallHierarchy(textCursor());
}

void TextEditorWidget::abortAssist()
{
    d->m_codeAssistant.destroyContext();
}

void TextEditorWidgetPrivate::moveLineUpDown(bool up)
{
    if (m_cursors.hasMultipleCursors())
        return;
    QTextCursor cursor = q->textCursor();
    QTextCursor move = cursor;

    move.setVisualNavigation(false); // this opens folded items instead of destroying them

    if (m_moveLineUndoHack)
        move.joinPreviousEditBlock();
    else
        move.beginEditBlock();

    bool hasSelection = cursor.hasSelection();

    if (hasSelection) {
        move.setPosition(cursor.selectionStart());
        move.movePosition(QTextCursor::StartOfBlock);
        move.setPosition(cursor.selectionEnd(), QTextCursor::KeepAnchor);
        move.movePosition(move.atBlockStart() ? QTextCursor::PreviousCharacter: QTextCursor::EndOfBlock,
                          QTextCursor::KeepAnchor);
    } else {
        move.movePosition(QTextCursor::StartOfBlock);
        move.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    }
    QString text = move.selectedText();

    RefactorMarkers affectedMarkers;
    RefactorMarkers nonAffectedMarkers;
    QList<int> markerOffsets;

    const QList<RefactorMarker> markers = m_refactorOverlay->markers();
    for (const RefactorMarker &marker : markers) {
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

    move.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
    move.removeSelectedText();

    if (up) {
        move.movePosition(QTextCursor::PreviousBlock);
        move.insertBlock();
        move.movePosition(QTextCursor::PreviousCharacter);
    } else {
        move.movePosition(QTextCursor::EndOfBlock);
        if (move.atBlockStart()) { // empty block
            move.movePosition(QTextCursor::NextBlock);
            move.insertBlock();
            move.movePosition(QTextCursor::PreviousCharacter);
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
        if (m_commentDefinition.hasMultiLineStyle()) {
            // Don't have any single line comments; try multi line.
            if (text.startsWith(m_commentDefinition.multiLineStart)
                && text.endsWith(m_commentDefinition.multiLineEnd)) {
                shouldReindent = false;
            }
        }
        if (shouldReindent && m_commentDefinition.hasSingleLineStyle()) {
            shouldReindent = false;
            QTextBlock block = move.block();
            while (block.isValid() && block.position() < end) {
                if (!block.text().startsWith(m_commentDefinition.singleLine))
                    shouldReindent = true;
                block = block.next();
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

bool TextEditorWidgetPrivate::cursorMoveKeyEvent(QKeyEvent *e)
{
    MultiTextCursor cursor = m_cursors;
    if (cursor.handleMoveKeyEvent(e, q->camelCaseNavigationEnabled(), q->editorLayout())) {
        resetCursorFlashTimer();
        q->setMultiTextCursor(cursor);
        q->ensureCursorVisible();
        updateCurrentLineHighlight();
        return true;
    }
    return false;
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
    ICore::restartTrimmer();

    QScopeGuard cleanup([&] { d->clearBlockSelection(); });

    if (!isModifier(e) && mouseHidingEnabled())
        viewport()->setCursor(Qt::BlankCursor);
    ToolTip::hide();

    d->m_moveLineUndoHack = false;
    d->clearVisibleFoldedBlock();

    MultiTextCursor cursor = multiTextCursor();

    if (e->key() == Qt::Key_Alt
            && d->m_behaviorSettings.m_keyboardTooltips) {
        d->m_maybeFakeTooltipEvent = true;
    } else {
        d->m_maybeFakeTooltipEvent = false;
        if (e->key() == Qt::Key_Escape ) {
            TextEditorWidgetFind::cancelCurrentSelectAll();
            if (d->m_suggestionBlock.isValid()) {
                d->clearCurrentSuggestion();
                e->accept();
                return;
            }
            if (d->m_snippetOverlay->isVisible()) {
                e->accept();
                d->m_snippetOverlay->accept();
                QTextCursor cursor = textCursor();
                cursor.clearSelection();
                setTextCursor(cursor);
                return;
            }
            if (cursor.hasMultipleCursors()) {
                QTextCursor c = cursor.mainCursor();
                c.setPosition(c.position(), QTextCursor::MoveAnchor);
                doSetTextCursor(c);
                return;
            }
        }
    }

    const bool ro = isReadOnly();
    const bool inOverwriteMode = overwriteMode();
    const bool hasMultipleCursors = cursor.hasMultipleCursors();

    if (TextSuggestion *suggestion = TextBlockUserData::suggestion(d->m_suggestionBlock)) {
        if (e->matches(QKeySequence::MoveToNextWord)) {
            e->accept();
            if (suggestion->applyWord(this))
                d->clearCurrentSuggestion();
            return;
        } else if (e->modifiers() == Qt::NoModifier
                   && (e->key() == Qt::Key_Tab || e->key() == Qt::Key_Backtab)) {
            e->accept();
            if (suggestion->apply())
                d->clearCurrentSuggestion();
            return;
        }
    }

    if (!ro
        && (e == QKeySequence::InsertParagraphSeparator
            || (!d->m_lineSeparatorsAllowed && e == QKeySequence::InsertLineSeparator))) {
        if (d->m_snippetOverlay->isVisible()) {
            e->accept();
            d->m_snippetOverlay->accept();
            QTextCursor cursor = textCursor();
            cursor.movePosition(QTextCursor::EndOfBlock);
            setTextCursor(cursor);
            return;
        }

        e->accept();
        cursor.beginEditBlock();
        for (QTextCursor &cursor : cursor) {
            const TabSettings ts = d->m_document->tabSettings();
            const TypingSettings &tps = d->m_document->typingSettings();

            int extraBlocks = d->m_autoCompleter->paragraphSeparatorAboutToBeInserted(cursor);

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

            if (extraBlocks > 0) {
                const int cursorPosition = cursor.position();
                QTextCursor ensureVisible = cursor;
                while (extraBlocks > 0) {
                    --extraBlocks;
                    ensureVisible.movePosition(QTextCursor::NextBlock);
                    if (tps.m_autoIndent)
                        d->m_document->autoIndent(ensureVisible, QChar::Null, cursorPosition);
                    else if (!previousIndentationString.isEmpty())
                        ensureVisible.insertText(previousIndentationString);
                    if (d->m_animateAutoComplete || d->m_highlightAutoComplete) {
                        QTextCursor tc = ensureVisible;
                        tc.movePosition(QTextCursor::EndOfBlock);
                        tc.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
                        tc.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor);
                        d->autocompleterHighlight(tc);
                    }
                }
                cursor.setPosition(cursorPosition);
            }
        }
        cursor.endEditBlock();
        setMultiTextCursor(cursor);
        ensureCursorVisible();
        return;
    } else if (!ro
               && (e == QKeySequence::MoveToStartOfBlock || e == QKeySequence::SelectStartOfBlock
                   || e == QKeySequence::MoveToStartOfLine
                   || e == QKeySequence::SelectStartOfLine)) {
        const bool blockOp = e == QKeySequence::MoveToStartOfBlock || e == QKeySequence::SelectStartOfBlock;
        const bool select = e == QKeySequence::SelectStartOfLine || e == QKeySequence::SelectStartOfBlock;
        d->handleHomeKey(select, blockOp);
        e->accept();
        return;
    } else if (!ro && e == QKeySequence::DeleteStartOfWord) {
        e->accept();
        if (!cursor.hasSelection()) {
            if (camelCaseNavigationEnabled())
                CamelCaseCursor::left(&cursor, QTextCursor::KeepAnchor);
            else
                cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
        }
        cursor.removeSelectedText();
        setMultiTextCursor(cursor);
        return;
    } else if (!ro && e == QKeySequence::DeleteEndOfWord) {
        e->accept();
        if (!cursor.hasSelection()) {
            if (camelCaseNavigationEnabled())
                CamelCaseCursor::right(&cursor, QTextCursor::KeepAnchor);
            else
                cursor.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor);
        }
        cursor.removeSelectedText();
        setMultiTextCursor(cursor);
        return;
    } else if (!ro && e == QKeySequence::DeleteCompleteLine) {
        e->accept();
        for (QTextCursor &c : cursor)
            c.select(QTextCursor::BlockUnderCursor);
        cursor.mergeCursors();
        cursor.removeSelectedText();
        setMultiTextCursor(cursor);
        return;
    } else
        switch (e->key()) {
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
        if (d->m_skipAutoCompletedText && e->key() == Qt::Key_Tab) {
            bool skippedAutoCompletedText = false;
            while (!d->m_autoCompleteHighlightPos.isNull()
                   && d->m_autoCompleteHighlightPos.selectionStart() == cursor.position()) {
                skippedAutoCompletedText = true;
                cursor.setPosition(d->m_autoCompleteHighlightPos.selectionEnd());
                d->m_autoCompleteHighlightPos = QTextCursor();
            }
            if (skippedAutoCompletedText) {
                setTextCursor(cursor);
                e->accept();
                d->updateAutoCompleteHighlight();
                return;
            }
        }
        int newPosition;
        if (!hasMultipleCursors
            && d->m_document->typingSettings().tabShouldIndent(document(), cursor, &newPosition)) {
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
                               | Qt::AltModifier
                               | Qt::MetaModifier)) == Qt::NoModifier) {
            e->accept();
            d->clearCurrentSuggestion();
            if (cursor.hasSelection()) {
                cursor.removeSelectedText();
                setMultiTextCursor(cursor);
            } else {
                d->handleBackspaceKey();
            }
            ensureCursorVisible();
            return;
        }
        break;
    case Qt::Key_Insert:
        if (ro) break;
        if (e->modifiers() == Qt::NoModifier || e->modifiers() == Qt::KeypadModifier) {
            setOverwriteMode(!inOverwriteMode);
            if (inOverwriteMode) {
                for (const QTextCursor &cursor : multiTextCursor()) {
                    const QRectF oldBlockRect = d->cursorBlockRect(document(),
                                                                   cursor.block(),
                                                                   cursor.position());
                    viewport()->update(oldBlockRect.toAlignedRect());
                }
            }
            e->accept();
            return;
        }
        break;
    case Qt::Key_Delete:
        if (hasMultipleCursors && !ro
            && (e->modifiers() == Qt::NoModifier || e->modifiers() == Qt::KeypadModifier)) {
            if (cursor.hasSelection()) {
                cursor.removeSelectedText();
            } else {
                cursor.beginEditBlock();
                for (QTextCursor c : cursor)
                    c.deleteChar();
                cursor.mergeCursors();
                cursor.endEditBlock();
            }
            e->accept();
            return;
        }
        break;
    default:
        break;
    }

    const QString eventText = e->text();

    if (e->key() == Qt::Key_H
            && e->modifiers() == Qt::KeyboardModifiers(HostOsInfo::controlModifier())) {
        d->universalHelper();
        e->accept();
        return;
    }

    if (ro || !isPrintableText(eventText)) {
        QTextCursor::MoveOperation blockSelectionOperation = QTextCursor::NoMove;
        if (e->modifiers() == (Qt::AltModifier | Qt::ShiftModifier)
            && !Utils::HostOsInfo::isMacHost()) {
            if (MultiTextCursor::multiCursorEvent(
                           e, QKeySequence::MoveToNextLine, Qt::ShiftModifier)) {
                blockSelectionOperation = QTextCursor::Down;
            } else if (MultiTextCursor::multiCursorEvent(
                           e, QKeySequence::MoveToPreviousLine, Qt::ShiftModifier)) {
                blockSelectionOperation = QTextCursor::Up;
            } else if (MultiTextCursor::multiCursorEvent(
                           e, QKeySequence::MoveToNextChar, Qt::ShiftModifier)) {
                blockSelectionOperation = QTextCursor::NextCharacter;
            } else if (MultiTextCursor::multiCursorEvent(
                           e, QKeySequence::MoveToPreviousChar, Qt::ShiftModifier)) {
                blockSelectionOperation = QTextCursor::PreviousCharacter;
            }
        }

        if (blockSelectionOperation != QTextCursor::NoMove) {
            cleanup.dismiss();
            d->handleMoveBlockSelection(blockSelectionOperation);
        } else if (!d->cursorMoveKeyEvent(e)) {
            QTextCursor cursor = textCursor();
            bool cursorWithinSnippet = false;
            if (d->m_snippetOverlay->isVisible()
                && (e->key() == Qt::Key_Delete || e->key() == Qt::Key_Backspace)) {
                cursorWithinSnippet = d->snippetCheckCursor(cursor);
            }
            if (cursorWithinSnippet)
                cursor.beginEditBlock();

            PlainTextEdit::keyPressEvent(e);

            if (cursorWithinSnippet) {
                cursor.endEditBlock();
                d->m_snippetOverlay->updateEquivalentSelections(textCursor());
            }
        }
    } else if (hasMultipleCursors) {
        if (inOverwriteMode) {
            cursor.beginEditBlock();
            for (QTextCursor &c : cursor) {
                QTextBlock block = c.block();
                int eolPos = block.position() + block.length() - 1;
                int selEndPos = qMin(c.position() + eventText.length(), eolPos);
                c.setPosition(selEndPos, QTextCursor::KeepAnchor);
                c.insertText(eventText);
            }
            cursor.endEditBlock();
        } else {
            cursor.insertText(eventText);
        }
        setMultiTextCursor(cursor);
    } else if ((e->modifiers() & (Qt::ControlModifier|Qt::AltModifier)) != Qt::ControlModifier){
        // only go here if control is not pressed, except if also alt is pressed
        // because AltGr maps to Alt + Ctrl
        QTextCursor cursor = textCursor();
        QString autoText;
        if (!inOverwriteMode) {
            int skippedChars = 0;
            if (d->m_skipAutoCompletedText && !d->m_autoCompleteHighlightPos.isNull()) {
                const QString autoCompletedText = d->m_autoCompleteHighlightPos.selectedText();
                while (skippedChars < autoCompletedText.size() && skippedChars < eventText.size()) {
                    if (autoCompletedText.at(skippedChars) != eventText.at(skippedChars))
                        break;
                    ++skippedChars;
                }
                auto moveCursor = [&](QTextCursor &c) {
                    c.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, skippedChars);
                };
                moveCursor(d->m_autoCompleteHighlightPos);
                moveCursor(cursor);
            }
            if (skippedChars < eventText.size())
                autoText = autoCompleter()->autoComplete(cursor, eventText.mid(skippedChars), false);
        }
        const bool cursorWithinSnippet = d->snippetCheckCursor(cursor);

        QChar electricChar;
        if (d->m_document->typingSettings().m_autoIndent) {
            for (const QChar c : eventText) {
                if (d->m_document->indenter()->isElectricCharacter(c)) {
                    electricChar = c;
                    break;
                }
            }
        }

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
            d->autocompleterHighlight(cursor);
        }
        if (!electricChar.isNull() && d->m_autoCompleter->contextAllowsElectricCharacters(cursor))
            d->m_document->autoIndent(cursor, electricChar, cursor.position());
        if (!autoText.isEmpty())
            cursor.setPosition(cursor.position());

        if (doEditBlock)
            cursor.endEditBlock();

        setTextCursor(cursor);

        if (doEditBlock && cursorWithinSnippet)
            d->m_snippetOverlay->updateEquivalentSelections(textCursor());
    }

    if (!ro && e->key() == Qt::Key_Delete && d->m_parenthesesMatchingEnabled)
        d->m_parenthesesMatchingTimer.start();

    if (!ro && d->m_contentsChanged && isPrintableText(eventText) && !inOverwriteMode)
        d->m_codeAssistant.process();
}

class PositionedPart : public ParsedSnippet::Part
{
public:
    explicit PositionedPart(const ParsedSnippet::Part &part) : ParsedSnippet::Part(part) {}
    int start;
    int end;
};

class CursorPart : public ParsedSnippet::Part
{
public:
    CursorPart(const PositionedPart &part, QTextDocument *doc)
        : ParsedSnippet::Part(part)
        , cursor(doc)
    {
        cursor.setPosition(part.start);
        cursor.setPosition(part.end, QTextCursor::KeepAnchor);
    }
    QTextCursor cursor;
};

void TextEditorWidget::insertCodeSnippet(int basePosition,
                                         const QString &snippet,
                                         const SnippetParser &parse)
{
    SnippetParseResult result = parse(snippet);
    if (std::holds_alternative<SnippetParseError>(result)) {
        const auto &error = std::get<SnippetParseError>(result);
        QMessageBox::warning(this, Tr::tr("Snippet Parse Error"), error.htmlMessage());
        return;
    }
    QTC_ASSERT(std::holds_alternative<ParsedSnippet>(result), return);
    ParsedSnippet data = std::get<ParsedSnippet>(result);

    QTextCursor cursor = textCursor();
    cursor.setPosition(basePosition, QTextCursor::KeepAnchor);
    cursor.beginEditBlock();
    cursor.removeSelectedText();
    const int startCursorPosition = cursor.position();

    d->m_snippetOverlay->accept();

    QList<PositionedPart> positionedParts;
    for (const ParsedSnippet::Part &part : std::as_const(data.parts)) {
        if (part.variableIndex >= 0) {
            PositionedPart posPart(part);
            posPart.start = cursor.position();
            cursor.insertText(part.text);
            posPart.end = cursor.position();
            positionedParts << posPart;
        } else {
            cursor.insertText(part.text);
        }
    }

    const QList<CursorPart> cursorParts = Utils::transform(positionedParts,
                                                     [doc = document()](const PositionedPart &part) {
                                                         return CursorPart(part, doc);
                                                     });

    cursor.setPosition(startCursorPosition, QTextCursor::KeepAnchor);
    d->m_document->autoIndent(cursor);
    cursor.endEditBlock();

    const QColor occurrencesColor
        = textDocument()->fontSettings().toTextCharFormat(C_OCCURRENCES).background().color();
    const QColor renameColor
        = textDocument()->fontSettings().toTextCharFormat(C_OCCURRENCES_RENAME).background().color();

    for (const CursorPart &part : cursorParts) {
        const QColor &color = part.cursor.hasSelection() ? occurrencesColor : renameColor;
        if (part.finalPart) {
            d->m_snippetOverlay->setFinalSelection(part.cursor, color);
        } else {
            d->m_snippetOverlay->addSnippetSelection(part.cursor,
                                                     color,
                                                     part.mangler,
                                                     part.variableIndex);
        }
    }

    cursor = d->m_snippetOverlay->firstSelectionCursor();
    if (!cursor.isNull()) {
        setTextCursor(cursor);
        if (d->m_snippetOverlay->isFinalSelection(cursor))
            d->m_snippetOverlay->accept();
        else
            d->m_snippetOverlay->setVisible(true);
    }
}

void TextEditorWidgetPrivate::universalHelper()
{
    // Test function for development. Place your new fangled experiment here to
    // give it proper scrutiny before pushing it onto others.
}

void TextEditorWidget::doSetTextCursor(const QTextCursor &cursor, bool keepMultiSelection)
{
    // workaround for QTextControl bug
    bool selectionChange = cursor.hasSelection() || textCursor().hasSelection();
    QTextCursor c = cursor;
    c.setVisualNavigation(true);
    const MultiTextCursor oldCursor = d->m_cursors;
    if (!keepMultiSelection)
        const_cast<MultiTextCursor &>(d->m_cursors).setCursors({c});
    else
        const_cast<MultiTextCursor &>(d->m_cursors).replaceMainCursor(c);
    d->updateCursorSelections();
    d->resetCursorFlashTimer();
    PlainTextEdit::doSetTextCursor(c);
    if (oldCursor != d->m_cursors) {
        QRect updateRect = d->cursorUpdateRect(oldCursor);
        if (d->m_highlightCurrentLine)
            updateRect = QRect(0, updateRect.y(), viewport()->rect().width(), updateRect.height());
        updateRect |= d->cursorUpdateRect(d->m_cursors);
        viewport()->update(updateRect);
        emit cursorPositionChanged();
    }
    if (selectionChange)
        d->slotSelectionChanged();
}

void TextEditorWidget::doSetTextCursor(const QTextCursor &cursor)
{
    doSetTextCursor(cursor, false);
}

void TextEditorWidget::gotoLine(int line, int column, bool centerLine, bool animate)
{
    d->m_lastCursorChangeWasInteresting = false; // avoid adding the previous position to history
    const int blockNumber = qMin(line, document()->blockCount()) - 1;
    const QTextBlock &block = document()->findBlockByNumber(blockNumber);
    if (block.isValid()) {
        QTextCursor cursor(block);
        if (column >= block.length()) {
            cursor.movePosition(QTextCursor::EndOfBlock);
        } else if (column > 0) {
            cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, column);
        } else {
            int pos = cursor.position();
            while (document()->characterAt(pos).category() == QChar::Separator_Space) {
                ++pos;
            }
            cursor.setPosition(pos);
        }

        const DisplaySettings &ds = d->m_displaySettings;
        if (animate && ds.m_animateNavigationWithinFile) {
            QScrollBar *scrollBar = verticalScrollBar();
            const int start = scrollBar->value();

            ensureBlockIsUnfolded(block);
            setUpdatesEnabled(false);
            setTextCursor(cursor);
            if (centerLine)
                centerCursor();
            else
                ensureCursorVisible();
            const int end = scrollBar->value();
            scrollBar->setValue(start);
            setUpdatesEnabled(true);

            const int delta = end - start;
            // limit the number of steps for the animation otherwise you wont be able to tell
            // the direction of the animantion for large delta values
            const int steps = qMax(-ds.m_animateWithinFileTimeMax,
                                   qMin(ds.m_animateWithinFileTimeMax, delta));
            // limit the duration of the animation to at least 4 pictures on a 60Hz Monitor and
            // at most to the number of absolute steps
            const int durationMinimum = int (4 // number of pictures
                                             * float(1) / 60 // on a 60 Hz Monitor
                                             * 1000); // milliseconds
            const int duration = qMax(durationMinimum, qAbs(steps));

            d->m_navigationAnimation = new QSequentialAnimationGroup(this);
            auto startAnimation = new QPropertyAnimation(verticalScrollBar(), "value");
            startAnimation->setEasingCurve(QEasingCurve::InExpo);
            startAnimation->setStartValue(start);
            startAnimation->setEndValue(start + steps / 2);
            startAnimation->setDuration(duration / 2);
            d->m_navigationAnimation->addAnimation(startAnimation);
            auto endAnimation = new QPropertyAnimation(verticalScrollBar(), "value");
            endAnimation->setEasingCurve(QEasingCurve::OutExpo);
            endAnimation->setStartValue(end - steps / 2);
            endAnimation->setEndValue(end);
            endAnimation->setDuration(duration / 2);
            d->m_navigationAnimation->addAnimation(endAnimation);
            d->m_navigationAnimation->start(QAbstractAnimation::DeleteWhenStopped);
        } else {
            setTextCursor(cursor);
            if (centerLine)
                centerCursor();
            else
                ensureCursorVisible();
        }
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
        editorLayout()->moveCursor(tc, QTextCursor::EndOfLine);
        return tc.position();
    case StartOfLinePosition:
        editorLayout()->moveCursor(tc, QTextCursor::StartOfLine);
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

QTextCursor TextEditorWidget::textCursorAt(int position) const
{
    QTextCursor c = textCursor();
    c.setPosition(position);
    return c;
}

Text::Position TextEditorWidget::lineColumn() const
{
    return Utils::Text::Position::fromCursor(textCursor());
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
    Text::convertPosition(document(), pos, line, column);
}

bool TextEditorWidget::event(QEvent *e)
{
    if (!d)
        return PlainTextEdit::event(e);

    // FIXME: That's far too heavy, and triggers e.g for ChildEvent
    if (e->type() != QEvent::InputMethodQuery)
        d->m_contentsChanged = false;
    switch (e->type()) {
    case QEvent::ShortcutOverride: {
        auto ke = static_cast<QKeyEvent *>(e);
        if (ke->key() == Qt::Key_Escape
            && (d->m_snippetOverlay->isVisible()
                || multiTextCursor().hasMultipleCursors()
                || d->m_suggestionBlock.isValid()
                || d->m_numEmbeddedWidgets > 0)) {
            if (d->m_numEmbeddedWidgets > 0)
                emit embeddedWidgetsShouldClose();
            e->accept();
        } else {
            // hack copied from QInputControl::isCommonTextEditShortcut
            // Fixes: QTCREATORBUG-22854
            e->setAccepted((ke->modifiers() == Qt::NoModifier || ke->modifiers() == Qt::ShiftModifier
                            || ke->modifiers() == Qt::KeypadModifier)
                           && (ke->key() < Qt::Key_Escape));
            d->m_maybeFakeTooltipEvent = false;
        }
        return true;
    }
    case QEvent::ApplicationPaletteChange: {
        // slight hack: ignore palette changes
        // at this point the palette has changed already,
        // so undo it by re-setting the palette:
        applyFontSettings();
        return true;
    }
    case QEvent::ReadOnlyChange:
        d->updateFileLineEndingVisible();
        d->updateTabSettingsButtonVisible();
        if (isReadOnly())
            setTextInteractionFlags(textInteractionFlags() | Qt::TextSelectableByKeyboard);
        d->updateActions();
        break;
    default:
        break;
    }

    return PlainTextEdit::event(e);
}

void TextEditorWidget::contextMenuEvent(QContextMenuEvent *e)
{
    showDefaultContextMenu(e, Id());
}

void TextEditorWidgetPrivate::documentAboutToBeReloaded()
{
    //memorize cursor position
    m_tempState = q->saveState();

    // remove extra selections (loads of QTextCursor objects)

    m_extraSelections.clear();
    m_extraSelections.reserve(NExtraSelectionKinds);
    q->PlainTextEdit::setExtraSelections(QList<QTextEdit::ExtraSelection>());

    // clear all overlays
    m_overlay->clear();
    m_snippetOverlay->clear();
    m_searchResultOverlay->clear();
    m_selectionHighlightOverlay->clear();
    m_refactorOverlay->clear();

    // clear search results
    m_searchResults.clear();
    m_selectionResults.clear();
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
    stream << 2; // version number
    stream << verticalScrollBar()->value();
    stream << horizontalScrollBar()->value();
    int line, column;
    convertPosition(textCursor().position(), &line, &column);
    stream << line;
    stream << column;
    stream << d->m_foldedBlockCache;
    stream << firstVisibleBlockNumber();
    stream << lastVisibleBlockNumber();

    return state;
}

bool TextEditorWidget::singleShotAfterHighlightingDone(std::function<void()> &&f)
{
    if (d->m_document->syntaxHighlighter()
        && !d->m_document->syntaxHighlighter()->syntaxHighlighterUpToDate()) {
        connect(d->m_document->syntaxHighlighter(),
                &SyntaxHighlighter::finished,
                this,
                [f = std::move(f)] { f(); }, Qt::SingleShotConnection);
        return true;
    }
    return false;
}

void TextEditorWidget::restoreState(const QByteArray &state)
{
    const auto callFoldLicenseHeader = [this] {
        auto callFold = [this] {
            if (d->m_displaySettings.m_autoFoldFirstComment)
                d->foldLicenseHeader();
        };

        if (!singleShotAfterHighlightingDone(callFold))
            callFold();
    };

    if (state.isEmpty()) {
        callFoldLicenseHeader();
        return;
    }

    int version;
    int vval;
    int hval;
    int lineVal;
    int columnVal;
    QDataStream stream(state);
    stream >> version;
    stream >> vval;
    stream >> hval;
    stream >> lineVal;
    stream >> columnVal;

    if (version >= 1) {
        QSet<int> collapsedBlocks;
        stream >> collapsedBlocks;
        auto foldingRestore = [this, collapsedBlocks] {
            QTextDocument *doc = document();
            bool layoutChanged = false;
            for (const int blockNumber : std::as_const(collapsedBlocks)) {
                QTextBlock block = doc->findBlockByNumber(qMax(0, blockNumber));
                if (block.isValid()) {
                    TextBlockUserData::doFoldOrUnfold(block, false);
                    layoutChanged = true;
                }
            }
            if (layoutChanged) {
                auto documentLayout = qobject_cast<TextDocumentLayout *>(doc->documentLayout());
                QTC_ASSERT(documentLayout, return);
                documentLayout->requestUpdate();
                documentLayout->emitDocumentSizeChanged();
                d->updateCursorPosition();
            }
        };
        if (!singleShotAfterHighlightingDone(foldingRestore))
            foldingRestore();
    } else {
        callFoldLicenseHeader();
    }

    d->m_lastCursorChangeWasInteresting = false; // avoid adding last position to history
    // line is 1-based, column is 0-based
    gotoLine(lineVal, columnVal);
    verticalScrollBar()->setValue(vval);
    horizontalScrollBar()->setValue(hval);

    if (version >= 2) {
        int originalFirstBlock, originalLastBlock;
        stream >> originalFirstBlock;
        stream >> originalLastBlock;
        // If current line was visible in the old state, make sure it is visible in the new state.
        // This can happen if the height of the editor changed in the meantime
        const int lineBlock = lineVal - 1; // line is 1-based, blocks are 0-based
        const bool originalCursorVisible = (originalFirstBlock <= lineBlock
                                            && lineBlock <= originalLastBlock);
        const int firstBlock = firstVisibleBlockNumber();
        const int lastBlock = lastVisibleBlockNumber();
        const bool cursorVisible = (firstBlock <= lineBlock && lineBlock <= lastBlock);
        if (originalCursorVisible && !cursorVisible)
            centerCursor();
    }

    d->saveCurrentCursorPositionForNavigation();
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

void TextEditorWidgetPrivate::updateFileLineEndingVisible()
{
    m_fileLineEndingAction->setVisible(m_displaySettings.m_displayFileLineEnding && !q->isReadOnly());
}

void TextEditorWidgetPrivate::updateTabSettingsButtonVisible()
{
    m_tabSettingsButton->setVisible(m_displaySettings.m_displayTabSettings && !q->isReadOnly());
}

void TextEditorWidgetPrivate::reconfigure()
{
    m_document->setMimeType(
        Utils::mimeTypeForFile(m_document->filePath(),
                               MimeMatchMode::MatchDefaultAndRemote).name());
    q->configureGenericHighlighter();
}

void TextEditorWidgetPrivate::updateSyntaxInfoBar(const HighlighterHelper::Definitions &definitions,
                                                  const QString &fileName)
{
    Id missing(Constants::INFO_MISSING_SYNTAX_DEFINITION);
    Id multiple(Constants::INFO_MULTIPLE_SYNTAX_DEFINITIONS);
    InfoBar *infoBar = m_document->infoBar();

    if (definitions.isEmpty() && infoBar->canInfoBeAdded(missing)
        && !TextEditorSettings::highlighterSettings().isIgnoredFilePattern(fileName)) {
        InfoBarEntry info(missing,
                          Tr::tr("A highlight definition was not found for this file. "
                                 "Would you like to download additional highlight definition files?"),
                          InfoBarEntry::GlobalSuppression::Enabled);
        info.addCustomButton(
            Tr::tr("Download Definitions"),
            []() { HighlighterHelper::downloadDefinitions(); },
            {},
            InfoBarEntry::ButtonAction::Hide);

        infoBar->removeInfo(multiple);
        infoBar->addInfo(info);
        return;
    }

    infoBar->removeInfo(multiple);
    infoBar->removeInfo(missing);

    if (definitions.size() > 1) {
        InfoBarEntry info(multiple,
                          Tr::tr("More than one highlight definition was found for this file. "
                                 "Which one should be used to highlight this file?"));
        info.setComboInfo(Utils::transform(definitions, &HighlighterHelper::Definition::name),
                          [this](const InfoBarEntry::ComboInfo &info) {
            this->configureGenericHighlighter(HighlighterHelper::definitionForName(info.displayText));
        });

        info.addCustomButton(
            Tr::tr("Remember My Choice"),
            [this]() { rememberCurrentSyntaxDefinition(); },
            {},
            InfoBarEntry::ButtonAction::Hide);

        infoBar->addInfo(info);
    }
}

void TextEditorWidgetPrivate::removeSyntaxInfoBar()
{
    InfoBar *infoBar = m_document->infoBar();
    infoBar->removeInfo(Constants::INFO_MISSING_SYNTAX_DEFINITION);
    infoBar->removeInfo(Constants::INFO_MULTIPLE_SYNTAX_DEFINITIONS);
}

void TextEditorWidgetPrivate::configureGenericHighlighter(
    const KSyntaxHighlighting::Definition &definition)
{
    if (definition.isValid()) {
        setupFromDefinition(definition);
    } else {
        q->setCodeFoldingSupported(false);
    }

    m_document->resetSyntaxHighlighter([definition] {
        auto highlighter = new Highlighter;
        highlighter->setDefinition(definition);
        return highlighter;
    });

    m_document->setFontSettings(TextEditorSettings::fontSettings());
}

void TextEditorWidgetPrivate::setupFromDefinition(const KSyntaxHighlighting::Definition &definition)
{
    const TypingSettings::CommentPosition commentPosition
        = m_document->typingSettings().m_commentPosition;
    m_commentDefinition.isAfterWhitespace = commentPosition != TypingSettings::StartOfLine;
    if (!definition.isValid())
        return;
    m_commentDefinition.singleLine = definition.singleLineCommentMarker();
    m_commentDefinition.multiLineStart = definition.multiLineCommentMarker().first;
    m_commentDefinition.multiLineEnd = definition.multiLineCommentMarker().second;
    if (commentPosition == TypingSettings::Automatic) {
        m_commentDefinition.isAfterWhitespace
            = definition.singleLineCommentPosition()
              == KSyntaxHighlighting::CommentPosition::AfterWhitespace;
    }
    q->setCodeFoldingSupported(true);
}

KSyntaxHighlighting::Definition TextEditorWidgetPrivate::currentDefinition()
{
    if (auto *highlighter = qobject_cast<Highlighter *>(m_document->syntaxHighlighter()))
        return highlighter->definition();
    return {};
}

void TextEditorWidgetPrivate::rememberCurrentSyntaxDefinition()
{
    const HighlighterHelper::Definition &definition = currentDefinition();
    if (definition.isValid())
        HighlighterHelper::rememberDefinitionForDocument(definition, m_document.data());
}

void TextEditorWidgetPrivate::openLinkUnderCursor(bool openInNextSplit)
{
    q->findLinkAt(
        q->textCursor(),
        [openInNextSplit, self = QPointer<TextEditorWidget>(q)](const Link &symbolLink) {
            if (self)
                self->openLink(symbolLink, openInNextSplit);
        },
        true,
        openInNextSplit);
}

void TextEditorWidgetPrivate::openTypeUnderCursor(bool openInNextSplit)
{
    q->findTypeAt(
        q->textCursor(),
        [openInNextSplit, self = QPointer<TextEditorWidget>(q)](const Link &symbolLink) {
            if (self)
                self->openLink(symbolLink, openInNextSplit);
        },
        true,
        openInNextSplit);
}

qreal TextEditorWidgetPrivate::charWidth() const
{
    return QFontMetricsF(q->font()).horizontalAdvance(QLatin1Char('x'));
}
class CarrierWidget : public QWidget
{
public:
    CarrierWidget(TextEditorWidget *textEditorWidget, QWidget *embed)
        : QWidget(textEditorWidget->viewport())
        , m_embed(embed)
        , m_textEditorWidget(textEditorWidget)
    {
        m_layout = new QVBoxLayout(this);
        updateContentMargins();
        m_layout->addWidget(m_embed);

        setFixedWidth(m_textEditorWidget->width() - m_textEditorWidget->extraAreaWidth());
        setFixedHeight(m_embed->minimumSizeHint().height());

        connect(m_textEditorWidget, &TextEditorWidget::resized, this, [this] {
            setFixedWidth(m_textEditorWidget->width() - m_textEditorWidget->extraAreaWidth());
        });

        m_textEditorWidget->viewport()->installEventFilter(this);
    }

    int embedHeight() { return m_embed->sizeHint().height(); }

    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (event->type() == QEvent::Resize)
            updateContentMargins();
        return QObject::eventFilter(obj, event);
    }

private:
    void updateContentMargins() {
        bool verticalScrollBarVisible = m_textEditorWidget->verticalScrollBar()->isVisible();
        int verticalScrollBarWidth = m_textEditorWidget->verticalScrollBar()->width();

        // Value 4 here is the liitle space between extraArea (space with line numbers) and code.
        m_layout->setContentsMargins(0, 0, 4 + (verticalScrollBarVisible ? verticalScrollBarWidth : 0), 0);
    }

    QWidget *m_embed;
    TextEditorWidget *m_textEditorWidget;
    QVBoxLayout *m_layout;
};

EmbeddedWidgetInterface::~EmbeddedWidgetInterface()
{
    close();
}

void EmbeddedWidgetInterface::resize()
{
    emit resized();
}

void EmbeddedWidgetInterface::close()
{
    emit closed();
}

void TextEditorWidgetPrivate::forceUpdateScrollbarSize()
{
    // We use resizeEvent here as a workaround as we can't get access to the
    // scrollarea which is a private part of the PlainTextEdit.
    // During the resizeEvent the plain text edit will resize its scrollbars.
    // The TextEditorWidget will also update its scrollbar overlays.
    QResizeEvent event(q->size(), q->size());
    q->resizeEvent(&event);
}

std::unique_ptr<EmbeddedWidgetInterface> TextEditorWidgetPrivate::insertWidget(
    QWidget *widget, int pos)
{
    QPointer<CarrierWidget> carrier = new CarrierWidget(q, widget);
    std::unique_ptr<EmbeddedWidgetInterface> result(new EmbeddedWidgetInterface());

    connect(
        q,
        &TextEditorWidget::embeddedWidgetsShouldClose,
        result.get(),
        &EmbeddedWidgetInterface::shouldClose);

    struct State
    {
        int height = 0;
        QTextCursor cursor;
        QTextBlock block;
    };

    std::shared_ptr<State> pState = std::make_shared<State>();
    pState->cursor = QTextCursor(q->document());
    pState->cursor.setPosition(pos);
    pState->cursor.movePosition(QTextCursor::StartOfBlock);

    auto position = [this, pState, carrier] {
        QTextBlock block = pState->cursor.block();
        QTC_ASSERT(block.isValid(), return);
        auto documentLayout = qobject_cast<TextDocumentLayout *>(q->document()->documentLayout());
        QTC_ASSERT(documentLayout, return);

        TextBlockUserData *userData = TextBlockUserData::userData(block);
        if (block != pState->block) {
            TextBlockUserData *previousUserData = TextBlockUserData::userData(pState->block);
            if (previousUserData && userData != previousUserData) {
                // We have swapped into a different block, remove it from the previous block
                TextBlockUserData::removeEmbeddedWidget(pState->block, carrier);
            }
            TextBlockUserData::addEmbeddedWidget(block, carrier);
            pState->block = block;
            pState->height = 0;
        }

        const QPoint pos
            = q->blockBoundingGeometry(block).translated(q->contentOffset()).topLeft().toPoint()
              + QPoint(0, documentLayout->embeddedWidgetOffset(block, carrier));

        int h = carrier->embedHeight();
        if (h == pState->height && pos == carrier->pos())
            return;

        carrier->move(pos);
        carrier->setFixedHeight(h);

        pState->height = h;

        documentLayout->scheduleUpdate();
    };

    position();

    connect(widget, &QWidget::destroyed, this, [pState, carrier, this] {
        if (carrier)
            carrier->deleteLater();
        if (!q->document())
            return;
        QTextBlock block = pState->cursor.block();
        TextBlockUserData::removeEmbeddedWidget(block, carrier);
        m_numEmbeddedWidgets--;
        forceUpdateScrollbarSize();
    });
    connect(q->document()->documentLayout(), &QAbstractTextDocumentLayout::update, carrier, position);
    connect(q->verticalScrollBar(), &QScrollBar::valueChanged, carrier, position);
    connect(result.get(), &EmbeddedWidgetInterface::resized, carrier, [position, this]() {
        position();
        forceUpdateScrollbarSize();
    });
    connect(result.get(), &EmbeddedWidgetInterface::closed, this, [this, carrier] {
        if (carrier)
            carrier->deleteLater();
        QAbstractTextDocumentLayout *layout = q->document()->documentLayout();
        QTimer::singleShot(0, layout, [layout] { layout->update(); });
    });

    m_numEmbeddedWidgets++;

    carrier->show();

    forceUpdateScrollbarSize();
    return result;
}

void TextEditorWidgetPrivate::registerActions()
{
    using namespace Core::Constants;
    using namespace TextEditor::Constants;

    ActionBuilder(this, Constants::COMPLETE_THIS)
        .setContext(m_editorContext)
        .addOnTriggered(this, [this] { q->invokeAssist(Completion); });

    ActionBuilder(this, Constants::FUNCTION_HINT)
        .setContext(m_editorContext)
        .addOnTriggered(this, [this] { q->invokeAssist(FunctionHint); });

    ActionBuilder(this, Constants::QUICKFIX_THIS)
        .setContext(m_editorContext)
        .addOnTriggered(this, [this] { q->invokeAssist(QuickFix); });

    ActionBuilder(this, Constants::SHOWCONTEXTMENU)
        .setContext(m_editorContext)
        .addOnTriggered(this, [this] { q->showContextMenu(); });

    m_undoAction = ActionBuilder(this, UNDO)
                       .setContext(m_editorContext)
                       .addOnTriggered([this] { q->undo(); })
                       .setScriptable(true)
                       .contextAction();
    m_redoAction = ActionBuilder(this, REDO)
                       .setContext(m_editorContext)
                       .addOnTriggered([this] { q->redo(); })
                       .setScriptable(true)
                       .contextAction();
    m_copyAction = ActionBuilder(this, COPY)
                       .setContext(m_editorContext)
                       .addOnTriggered([this] { q->copy(); })
                       .setScriptable(true)
                       .contextAction();
    m_cutAction = ActionBuilder(this, CUT)
                      .setContext(m_editorContext)
                      .addOnTriggered([this] { q->cut(); })
                      .setScriptable(true)
                      .contextAction();
    m_pasteAction = ActionBuilder(this, PASTE)
                                  .setContext(m_editorContext)
                                  .addOnTriggered([this] { q->paste(); })
                                  .setScriptable(true)
                                  .contextAction();
    m_modifyingActions << m_pasteAction;

    ActionBuilder(this, SELECTALL)
        .setContext(m_editorContext)
        .setScriptable(true)
        .addOnTriggered([this] { q->selectAll(); });
    ActionBuilder(this, GOTO).setContext(m_editorContext).addOnTriggered([] {
        LocatorManager::showFilter(lineNumberFilter());
    });
    ActionBuilder(this, PRINT)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->print(ICore::printer()); })
        .contextAction();
    m_modifyingActions << ActionBuilder(this, DELETE_LINE)
                              .setContext(m_editorContext)
                              .addOnTriggered([this] { q->deleteLine(); })
                              .setScriptable(true)
                              .contextAction();
    m_modifyingActions << ActionBuilder(this, DELETE_END_OF_LINE)
                              .setContext(m_editorContext)
                              .addOnTriggered([this] { q->deleteEndOfLine(); })
                              .setScriptable(true)
                              .contextAction();
    m_modifyingActions << ActionBuilder(this, DELETE_END_OF_WORD)
                              .setContext(m_editorContext)
                              .addOnTriggered([this] { q->deleteEndOfWord(); })
                              .setScriptable(true)
                              .contextAction();
    m_modifyingActions << ActionBuilder(this, DELETE_END_OF_WORD_CAMEL_CASE)
                              .setContext(m_editorContext)
                              .addOnTriggered([this] { q->deleteEndOfWordCamelCase(); })
                              .setScriptable(true)
                              .contextAction();
    m_modifyingActions << ActionBuilder(this, DELETE_START_OF_LINE)
                              .setContext(m_editorContext)
                              .addOnTriggered([this] { q->deleteStartOfLine(); })
                              .setScriptable(true)
                              .contextAction();
    m_modifyingActions << ActionBuilder(this, DELETE_START_OF_WORD)
                              .setContext(m_editorContext)
                              .addOnTriggered([this] { q->deleteStartOfWord(); })
                              .setScriptable(true)
                              .contextAction();
    m_modifyingActions << ActionBuilder(this, DELETE_START_OF_WORD_CAMEL_CASE)
                              .setContext(m_editorContext)
                              .addOnTriggered([this] { q->deleteStartOfWordCamelCase(); })
                              .setScriptable(true)
                              .contextAction();
    ActionBuilder(this, GOTO_BLOCK_START_WITH_SELECTION)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->gotoBlockStartWithSelection(); })
        .setScriptable(true);
    ActionBuilder(this, GOTO_BLOCK_END_WITH_SELECTION)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->gotoBlockEndWithSelection(); })
        .setScriptable(true);
    m_modifyingActions << ActionBuilder(this, MOVE_LINE_UP)
                              .setContext(m_editorContext)
                              .addOnTriggered([this] { q->moveLineUp(); })
                              .setScriptable(true)
                              .contextAction();
    m_modifyingActions << ActionBuilder(this, MOVE_LINE_DOWN)
                              .setContext(m_editorContext)
                              .addOnTriggered([this] { q->moveLineDown(); })
                              .setScriptable(true)
                              .contextAction();
    m_modifyingActions << ActionBuilder(this, COPY_LINE_UP)
                              .setContext(m_editorContext)
                              .addOnTriggered([this] { q->copyLineUp(); })
                              .setScriptable(true)
                              .contextAction();
    m_modifyingActions << ActionBuilder(this, COPY_LINE_DOWN)
                              .setContext(m_editorContext)
                              .addOnTriggered([this] { q->copyLineDown(); })
                              .setScriptable(true)
                              .contextAction();
    m_modifyingActions << ActionBuilder(this, JOIN_LINES)
                              .setContext(m_editorContext)
                              .addOnTriggered([this] { q->joinLines(); })
                              .setScriptable(true)
                              .contextAction();
    m_modifyingActions << ActionBuilder(this, INSERT_LINE_ABOVE)
                              .setContext(m_editorContext)
                              .addOnTriggered([this] { q->insertLineAbove(); })
                              .setScriptable(true)
                              .contextAction();
    m_modifyingActions << ActionBuilder(this, INSERT_LINE_BELOW)
                              .setContext(m_editorContext)
                              .addOnTriggered([this] { q->insertLineBelow(); })
                              .setScriptable(true)
                              .contextAction();
    m_modifyingActions << ActionBuilder(this, SWITCH_UTF8BOM)
                              .setContext(m_editorContext)
                              .addOnTriggered([this] { q->switchUtf8bom(); })
                              .contextAction();
    m_modifyingActions << ActionBuilder(this, INDENT)
                              .setContext(m_editorContext)
                              .addOnTriggered([this] { q->indent(); })
                              .setScriptable(true)
                              .contextAction();
    m_modifyingActions << ActionBuilder(this, UNINDENT)
                              .setContext(m_editorContext)
                              .addOnTriggered([this] { q->unindent(); })
                              .setScriptable(true)
                              .contextAction();
    m_followSymbolAction = ActionBuilder(this, FOLLOW_SYMBOL_UNDER_CURSOR)
                               .setContext(m_editorContext)
                               .addOnTriggered([this] { q->openLinkUnderCursor(); })
                               .contextAction();
    m_followSymbolInNextSplitAction = ActionBuilder(this, FOLLOW_SYMBOL_UNDER_CURSOR_IN_NEXT_SPLIT)
                                          .setContext(m_editorContext)
                                          .addOnTriggered(
                                              [this] { q->openLinkUnderCursorInNextSplit(); })
                                          .contextAction();
    m_followToTypeAction = ActionBuilder(this, FOLLOW_SYMBOL_TO_TYPE)
                               .setContext(m_editorContext)
                               .addOnTriggered([this] { q->openTypeUnderCursor(); })
                               .contextAction();
    m_followToTypeInNextSplitAction = ActionBuilder(this, FOLLOW_SYMBOL_TO_TYPE_IN_NEXT_SPLIT)
                                          .setContext(m_editorContext)
                                          .addOnTriggered(
                                              [this] { q->openTypeUnderCursorInNextSplit(); })
                                          .contextAction();
    m_findUsageAction = ActionBuilder(this, FIND_USAGES)
                            .setContext(m_editorContext)
                            .addOnTriggered([this] { q->findUsages(); })
                            .contextAction();
    m_renameSymbolAction = ActionBuilder(this, RENAME_SYMBOL)
                               .setContext(m_editorContext)
                               .addOnTriggered([this] { q->renameSymbolUnderCursor(); })
                               .contextAction();
    m_jumpToFileAction = ActionBuilder(this, JUMP_TO_FILE_UNDER_CURSOR)
                             .setContext(m_editorContext)
                             .addOnTriggered([this] { q->openLinkUnderCursor(); })
                             .contextAction();
    m_jumpToFileInNextSplitAction = ActionBuilder(this, JUMP_TO_FILE_UNDER_CURSOR_IN_NEXT_SPLIT)
                                        .setContext(m_editorContext)
                                        .addOnTriggered(
                                            [this] { q->openLinkUnderCursorInNextSplit(); })
                                        .contextAction();
    m_openCallHierarchyAction = ActionBuilder(this, OPEN_CALL_HIERARCHY)
                                    .setContext(m_editorContext)
                                    .addOnTriggered([this] { q->openCallHierarchy(); })
                                    .setScriptable(true)
                                    .contextAction();
    m_openTypeHierarchyAction = ActionBuilder(this, OPEN_TYPE_HIERARCHY)
                                    .setContext(m_editorContext)
                                    .addOnTriggered([] {
                                        updateTypeHierarchy(NavigationWidget::activateSubWidget(
                                            Constants::TYPE_HIERARCHY_FACTORY_ID, Side::Left));
                                    })
                                    .setScriptable(true)
                                    .contextAction();
    ActionBuilder(this, VIEW_PAGE_UP)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->viewPageUp(); })
        .setScriptable(true);
    ActionBuilder(this, VIEW_PAGE_DOWN)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->viewPageDown(); })
        .setScriptable(true);
    ActionBuilder(this, VIEW_LINE_UP)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->viewLineUp(); })
        .setScriptable(true);
    ActionBuilder(this, VIEW_LINE_DOWN)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->viewLineDown(); })
        .setScriptable(true);

    ActionBuilder(this, SELECT_ENCODING).setContext(m_editorContext).addOnTriggered([this] {
        q->selectEncoding();
    });
    m_modifyingActions << ActionBuilder(this, CIRCULAR_PASTE)
                              .setContext(m_editorContext)
                              .addOnTriggered([this] { q->circularPaste(); })
                              .contextAction();
    m_modifyingActions << ActionBuilder(this, NO_FORMAT_PASTE)
                              .setContext(m_editorContext)
                              .addOnTriggered([this] { q->pasteWithoutFormat(); })
                              .setScriptable(true)
                              .contextAction();

    m_autoIndentAction = ActionBuilder(this, AUTO_INDENT_SELECTION)
                             .setContext(m_editorContext)
                             .addOnTriggered([this] { q->autoIndent(); })
                             .setScriptable(true)
                             .contextAction();
    m_autoFormatAction = ActionBuilder(this, AUTO_FORMAT_SELECTION)
                             .setContext(m_editorContext)
                             .addOnTriggered([this] { q->autoFormat(); })
                             .setScriptable(true)
                             .contextAction();
    m_modifyingActions << ActionBuilder(this, REWRAP_PARAGRAPH)
                              .setContext(m_editorContext)
                              .addOnTriggered([this] { q->rewrapParagraph(); })
                              .setScriptable(true)
                              .contextAction();
    m_visualizeWhitespaceAction = ActionBuilder(this, VISUALIZE_WHITESPACE)
                                      .setContext(m_editorContext)
                                      .setCheckable(true)
                                      .addOnToggled(
                                          this,
                                          [this](bool checked) {
                                              DisplaySettings ds = q->displaySettings();
                                              ds.m_visualizeWhitespace = checked;
                                              q->setDisplaySettings(ds);
                                          })
                                      .contextAction();
    m_modifyingActions << ActionBuilder(this, CLEAN_WHITESPACE)
                              .setContext(m_editorContext)
                              .addOnTriggered([this] { q->cleanWhitespace(); })
                              .setScriptable(true)
                              .contextAction();
    m_textWrappingAction = ActionBuilder(this, TEXT_WRAPPING)
                               .setContext(m_editorContext)
                               .setCheckable(true)
                               .addOnToggled(
                                   this,
                                   [this](bool checked) {
                                       DisplaySettings ds = q->displaySettings();
                                       ds.m_textWrapping = checked;
                                       q->setDisplaySettings(ds);
                                   })
                               .contextAction();
    m_unCommentSelectionAction = ActionBuilder(this, UN_COMMENT_SELECTION)
                                     .setContext(m_editorContext)
                                     .addOnTriggered([this] { q->unCommentSelection(); })
                                     .setScriptable(true)
                                     .contextAction();
    m_modifyingActions << ActionBuilder(this, CUT_LINE)
                              .setContext(m_editorContext)
                              .addOnTriggered([this] { q->cutLine(); })
                              .setScriptable(true)
                              .contextAction();
    ActionBuilder(this, COPY_LINE)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->copyLine(); })
        .setScriptable(true);
    m_copyHtmlAction = ActionBuilder(this, COPY_WITH_HTML)
                           .setContext(m_editorContext)
                           .addOnTriggered([this] { q->copyWithHtml(); })
                           .setScriptable(true)
                           .contextAction();
    ActionBuilder(this, ADD_CURSORS_TO_LINE_ENDS)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->addCursorsToLineEnds(); })
        .setScriptable(true);
    ActionBuilder(this, ADD_SELECT_NEXT_FIND_MATCH)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->addSelectionNextFindMatch(); })
        .setScriptable(true);
    m_modifyingActions << ActionBuilder(this, DUPLICATE_SELECTION)
                              .setContext(m_editorContext)
                              .addOnTriggered([this] { q->duplicateSelection(); })
                              .setScriptable(true)
                              .contextAction();
    m_modifyingActions << ActionBuilder(this, DUPLICATE_SELECTION_AND_COMMENT)
                              .setContext(m_editorContext)
                              .addOnTriggered([this] { q->duplicateSelectionAndComment(); })
                              .setScriptable(true)
                              .contextAction();
    m_modifyingActions << ActionBuilder(this, UPPERCASE_SELECTION)
                              .setContext(m_editorContext)
                              .addOnTriggered([this] { q->uppercaseSelection(); })
                              .setScriptable(true)
                              .contextAction();
    m_modifyingActions << ActionBuilder(this, LOWERCASE_SELECTION)
                              .setContext(m_editorContext)
                              .addOnTriggered([this] { q->lowercaseSelection(); })
                              .setScriptable(true)
                              .contextAction();
    m_modifyingActions << ActionBuilder(this, SORT_LINES)
                              .setContext(m_editorContext)
                              .addOnTriggered([this] { q->sortLines(); })
                              .setScriptable(true)
                              .contextAction();
    ActionBuilder(this, FOLD)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->foldCurrentBlock(); })
        .setScriptable(true);
    ActionBuilder(this, UNFOLD)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->unfoldCurrentBlock(); })
        .setScriptable(true);
    ActionBuilder(this, FOLD_RECURSIVELY)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->fold(q->textCursor().block(), true); })
        .setScriptable(true);
    ActionBuilder(this, UNFOLD_RECURSIVELY)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->unfold(q->textCursor().block(), true); })
        .setScriptable(true);
    m_unfoldAllAction = ActionBuilder(this, UNFOLD_ALL)
                            .setContext(m_editorContext)
                            .addOnTriggered([this] { q->toggleFoldAll(); })
                            .setScriptable(true)
                            .contextAction();
    ActionBuilder(this, INCREASE_FONT_SIZE).setContext(m_editorContext).addOnTriggered([this] {
        q->increaseFontZoom();
    });
    ActionBuilder(this, DECREASE_FONT_SIZE).setContext(m_editorContext).addOnTriggered([this] {
        q->decreaseFontZoom();
    });
    ActionBuilder(this, RESET_FONT_SIZE).setContext(m_editorContext).addOnTriggered([this] {
        q->zoomReset();
    });
    ActionBuilder(this, GOTO_BLOCK_START)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->gotoBlockStart(); })
        .setScriptable(true);
    ActionBuilder(this, GOTO_BLOCK_END)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->gotoBlockEnd(); })
        .setScriptable(true);
    ActionBuilder(this, SELECT_BLOCK_UP)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->selectBlockUp(); })
        .setScriptable(true);
    ActionBuilder(this, SELECT_BLOCK_DOWN)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->selectBlockDown(); })
        .setScriptable(true);
    ActionBuilder(this, SELECT_WORD_UNDER_CURSOR)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->selectWordUnderCursor(); })
        .setScriptable(true);
    ActionBuilder(this, CLEAR_SELECTION)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->clearSelection(); })
        .setScriptable(true);

    ActionBuilder(this, GOTO_DOCUMENT_START)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->gotoDocumentStart(); })
        .setScriptable(true);
    ActionBuilder(this, GOTO_DOCUMENT_END)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->gotoDocumentEnd(); })
        .setScriptable(true);
    ActionBuilder(this, GOTO_LINE_START)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->gotoLineStart(); })
        .setScriptable(true);
    ActionBuilder(this, GOTO_LINE_END)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->gotoLineEnd(); })
        .setScriptable(true);
    ActionBuilder(this, GOTO_NEXT_LINE)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->gotoNextLine(); })
        .setScriptable(true);
    ActionBuilder(this, GOTO_PREVIOUS_LINE)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->gotoPreviousLine(); })
        .setScriptable(true);
    ActionBuilder(this, GOTO_PREVIOUS_CHARACTER)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->gotoPreviousCharacter(); })
        .setScriptable(true);
    ActionBuilder(this, GOTO_NEXT_CHARACTER)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->gotoNextCharacter(); })
        .setScriptable(true);
    ActionBuilder(this, GOTO_PREVIOUS_WORD)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->gotoPreviousWord(); })
        .setScriptable(true);
    ActionBuilder(this, GOTO_NEXT_WORD)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->gotoNextWord(); })
        .setScriptable(true);
    ActionBuilder(this, GOTO_PREVIOUS_WORD_CAMEL_CASE)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->gotoPreviousWordCamelCase(); })
        .setScriptable(true);
    ActionBuilder(this, GOTO_NEXT_WORD_CAMEL_CASE)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->gotoNextWordCamelCase(); })
        .setScriptable(true);

    ActionBuilder(this, GOTO_LINE_START_WITH_SELECTION)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->gotoLineStartWithSelection(); })
        .setScriptable(true);
    ActionBuilder(this, GOTO_LINE_END_WITH_SELECTION)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->gotoLineEndWithSelection(); })
        .setScriptable(true);
    ActionBuilder(this, GOTO_NEXT_LINE_WITH_SELECTION)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->gotoNextLineWithSelection(); })
        .setScriptable(true);
    ActionBuilder(this, GOTO_PREVIOUS_LINE_WITH_SELECTION)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->gotoPreviousLineWithSelection(); })
        .setScriptable(true);
    ActionBuilder(this, GOTO_PREVIOUS_CHARACTER_WITH_SELECTION)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->gotoPreviousCharacterWithSelection(); })
        .setScriptable(true);
    ActionBuilder(this, GOTO_NEXT_CHARACTER_WITH_SELECTION)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->gotoNextCharacterWithSelection(); })
        .setScriptable(true);
    ActionBuilder(this, GOTO_PREVIOUS_WORD_WITH_SELECTION)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->gotoPreviousWordWithSelection(); })
        .setScriptable(true);
    ActionBuilder(this, GOTO_NEXT_WORD_WITH_SELECTION)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->gotoNextWordWithSelection(); })
        .setScriptable(true);
    ActionBuilder(this, GOTO_PREVIOUS_WORD_CAMEL_CASE_WITH_SELECTION)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->gotoPreviousWordCamelCaseWithSelection(); })
        .setScriptable(true);
    ActionBuilder(this, GOTO_NEXT_WORD_CAMEL_CASE_WITH_SELECTION)
        .setContext(m_editorContext)
        .addOnTriggered([this] { q->gotoNextWordCamelCaseWithSelection(); })
        .setScriptable(true);

    // Collect additional modifying actions so we can check for them inside a readonly file
    // and disable them
    m_modifyingActions << m_autoIndentAction;
    m_modifyingActions << m_autoFormatAction;
    m_modifyingActions << m_unCommentSelectionAction;

    updateOptionalActions();
}

void TextEditorWidgetPrivate::updateActions()
{
    bool isWritable = !q->isReadOnly();
    for (QAction *a : std::as_const(m_modifyingActions))
        a->setEnabled(isWritable);
    m_unCommentSelectionAction->setEnabled((m_optionalActionMask & OptionalActions::UnCommentSelection) && isWritable);
    m_visualizeWhitespaceAction->setEnabled(q);
    if (TextEditorSettings::fontSettings().relativeLineSpacing() == 100) {
        m_textWrappingAction->setEnabled(q);
    } else {
        m_textWrappingAction->setEnabled(false);
        m_textWrappingAction->setChecked(false);
    }
    m_visualizeWhitespaceAction->setChecked(m_displaySettings.m_visualizeWhitespace);
    m_textWrappingAction->setChecked(m_displaySettings.m_textWrapping);

    updateRedoAction();
    updateUndoAction();
    updateCopyAction(q->textCursor().hasSelection());
    updatePasteAction();
    updateOptionalActions();
}

void TextEditorWidgetPrivate::updateOptionalActions()
{
    using namespace OptionalActions;
    m_followSymbolAction->setEnabled(m_optionalActionMask & FollowSymbolUnderCursor);
    m_followSymbolInNextSplitAction->setEnabled(m_optionalActionMask & FollowSymbolUnderCursor);
    m_followToTypeAction->setEnabled(m_optionalActionMask & FollowTypeUnderCursor);
    m_followToTypeInNextSplitAction->setEnabled(m_optionalActionMask & FollowTypeUnderCursor);
    m_findUsageAction->setEnabled(m_optionalActionMask & FindUsage);
    m_jumpToFileAction->setEnabled(m_optionalActionMask & JumpToFileUnderCursor);
    m_jumpToFileInNextSplitAction->setEnabled(m_optionalActionMask & JumpToFileUnderCursor);
    m_unfoldAllAction->setEnabled(m_optionalActionMask & UnCollapseAll);
    m_renameSymbolAction->setEnabled(m_optionalActionMask & RenameSymbol);
    m_openCallHierarchyAction->setEnabled(m_optionalActionMask & CallHierarchy);
    m_openTypeHierarchyAction->setEnabled(m_optionalActionMask & TypeHierarchy);

    bool formatEnabled = (m_optionalActionMask & OptionalActions::Format)
                         && !q->isReadOnly();
    m_autoIndentAction->setEnabled(formatEnabled);
    m_autoFormatAction->setEnabled(formatEnabled);
}

void TextEditorWidgetPrivate::updateRedoAction()
{
    m_redoAction->setEnabled(q->isRedoAvailable());
}

void TextEditorWidgetPrivate::updateUndoAction()
{
    m_undoAction->setEnabled(q->isUndoAvailable());
}

void TextEditorWidgetPrivate::updateCopyAction(bool hasCopyableText)
{
    if (m_cutAction)
        m_cutAction->setEnabled(hasCopyableText && !q->isReadOnly());
    if (m_copyAction)
        m_copyAction->setEnabled(hasCopyableText);
    if (m_copyHtmlAction)
        m_copyHtmlAction->setEnabled(hasCopyableText);
}

void TextEditorWidgetPrivate::updatePasteAction()
{
    m_updatePasteActionScheduled = false;
    if (m_pasteAction)
        m_pasteAction->setEnabled(!q->isReadOnly() && !qApp->clipboard()->text(QClipboard::Mode::Clipboard).isEmpty());
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
    return Utils::HostOsInfo::isMacHost() ? false : d->m_behaviorSettings.m_mouseHiding;
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
        m_snippetOverlay->accept();
        return false;
    }
    return true;
}

void TextEditorWidgetPrivate::snippetTabOrBacktab(bool forward)
{
    if (!m_snippetOverlay->isVisible() || m_snippetOverlay->isEmpty())
        return;
    QTextCursor cursor = forward ? m_snippetOverlay->nextSelectionCursor(q->textCursor())
                                 : m_snippetOverlay->previousSelectionCursor(q->textCursor());
    q->setTextCursor(cursor);
    if (m_snippetOverlay->isFinalSelection(cursor))
        m_snippetOverlay->accept();
}

// Calculate global position for a tooltip considering the left extra area.
QPoint TextEditorWidget::toolTipPosition(const QTextCursor &c) const
{
    const QPoint cursorPos = mapToGlobal(cursorRect(c).bottomRight() + QPoint(1,1));
    return cursorPos + QPoint(d->m_extraArea->width(), HostOsInfo::isWindowsHost() ? -24 : -16);
}

void TextEditorWidget::showTextMarksToolTip(const QPoint &pos,
                                            const TextMarks &marks,
                                            const TextMark *mainTextMark) const
{
    d->showTextMarksToolTip(pos, marks, mainTextMark);
}

void TextEditorWidgetPrivate::processTooltipRequest(const QTextCursor &c)
{
    const QPoint toolTipPoint = q->toolTipPosition(c);
    bool handled = false;
    emit q->tooltipOverrideRequested(q, toolTipPoint, c.position(), &handled);
    if (handled)
        return;

    const auto callback = [toolTipPoint](TextEditorWidget *widget, BaseHoverHandler *handler, int) {
        handler->showToolTip(widget, toolTipPoint);
    };
    const auto fallback = [toolTipPoint, position = c.position()](TextEditorWidget *widget) {
        emit widget->tooltipRequested(toolTipPoint, position);
    };
    m_hoverHandlerRunner.startChecking(c, callback, fallback);
}

bool TextEditorWidgetPrivate::processAnnotaionTooltipRequest(const QTextBlock &block,
                                                             const QPoint &pos) const
{
    TextBlockUserData *blockUserData = TextBlockUserData::textUserData(block);
    if (!blockUserData)
        return false;

    for (const AnnotationRect &annotationRect : m_annotationRects[block.blockNumber()]) {
        if (!annotationRect.rect.contains(pos))
            continue;
        showTextMarksToolTip(q->mapToGlobal(pos), blockUserData->marks(), annotationRect.mark);
        return true;
    }
    return false;
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
                          viewport(), {}, refactorMarker.rect);
            return true;
        }

        QTextCursor tc = cursorForPosition(pos);
        QTextBlock block = tc.block();
        const QTextLine line = editorLayout()->blockLayout(block)->lineForTextPosition(
            tc.positionInBlock());
        QTC_CHECK(line.isValid());
        // Only handle tool tip for text cursor if mouse is within the block for the text cursor,
        // and not if the mouse is e.g. in the empty space behind a short line.
        if (line.isValid()) {
            const QRectF blockGeometry = blockBoundingGeometry(block).translated(contentOffset());
            const int width = block == d->m_suggestionBlock ? blockGeometry.width()
                                                            : line.naturalTextRect().right();
            if (pos.x() <= blockGeometry.left() + width) {
                d->processTooltipRequest(tc);
                return true;
            } else if (d->processAnnotaionTooltipRequest(block, pos)) {
                return true;
            }
            ToolTip::hide();
        }
    }
    return PlainTextEdit::viewportEvent(event);
}


void TextEditorWidget::resizeEvent(QResizeEvent *e)
{
    PlainTextEdit::resizeEvent(e);
    QRect cr = rect();
    d->m_extraArea->setGeometry(
        QStyle::visualRect(layoutDirection(), cr,
                           QRect(cr.left() + frameWidth(), cr.top() + frameWidth(),
                                 extraAreaWidth(), cr.height() - 2 * frameWidth())));
    d->adjustScrollBarRanges();
    d->updateCurrentLineInScrollbar();
    emit resized();
}

QRect TextEditorWidgetPrivate::foldBox()
{
    if (m_highlightBlocksInfo.isEmpty() || extraAreaHighlightFoldedBlockNumber < 0)
        return {};

    QTextBlock begin = q->document()->findBlockByNumber(m_highlightBlocksInfo.open.last());

    QTextBlock end = q->document()->findBlockByNumber(m_highlightBlocksInfo.close.first());
    if (!begin.isValid() || !end.isValid())
        return {};
    QRectF br = q->blockBoundingGeometry(begin).translated(q->contentOffset());
    QRectF er = q->blockBoundingGeometry(end).translated(q->contentOffset());


    if (TextEditorSettings::fontSettings().relativeLineSpacing() == 100) {
        return QRect(m_extraArea->width() - foldBoxWidth(q->fontMetrics()),
                     int(br.top()),
                     foldBoxWidth(q->fontMetrics()),
                     int(er.bottom() - br.top()));
    }

    return QRect(m_extraArea->width() - foldBoxWidth(),
                 int(br.top()),
                 foldBoxWidth(),
                 int(er.bottom() - br.top()));
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
                QTextLayout *layout = q->editorLayout()->blockLayout(block);
                QTextLine line = layout->lineAt(layout->lineCount()-1);
                QRectF lineRect = line.naturalTextRect().translated(offset.x(), top);
                lineRect.adjust(0, 0, -1, -1);

                QString replacement = QLatin1String(" {") + q->foldReplacementText(block)
                        + QLatin1String("}; ");

                QRectF collapseRect(lineRect.right() + 12,
                                    lineRect.top(),
                                    q->fontMetrics().horizontalAdvance(replacement),
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

void TextEditorWidgetPrivate::highlightSearchResults(const QTextBlock &block, const PaintEventData &data) const
{
    if (m_searchExpr.pattern().isEmpty())
        return;

    int blockPosition = block.position();

    QTextCursor cursor = q->textCursor();
    QString text = block.text();
    text.replace(QChar::Nbsp, QLatin1Char(' '));
    int idx = -1;
    int l = 0;

    const int left = data.viewportRect.left() - int(data.offset.x());
    const int right = data.viewportRect.right() - int(data.offset.x());
    const int top = data.viewportRect.top() - int(data.offset.y());
    const int bottom = data.viewportRect.bottom() - int(data.offset.y());
    const QColor &searchResultColor = m_document->fontSettings()
            .toTextCharFormat(C_SEARCH_RESULT).background().color().darker(120);

    while (idx < text.length()) {
        const QRegularExpressionMatch match = m_searchExpr.match(text, idx + l + 1);
        if (!match.hasMatch())
            break;
        idx = match.capturedStart();
        l = match.capturedLength();
        if (l == 0)
            break;
        if (m_findFlags & FindWholeWords) {
            auto posAtWordSeparator = [](const QString &text, int idx) {
                if (idx < 0)
                    return QTC_GUARD(idx == -1);
                int textLength = text.length();
                if (idx >= textLength)
                    return QTC_GUARD(idx == textLength);
                const QChar c = text.at(idx);
                return !c.isLetterOrNumber() && c != QLatin1Char('_');
            };
            if (!posAtWordSeparator(text, idx - 1) || !posAtWordSeparator(text, idx + l))
                continue;
        }


        const int start = blockPosition + idx;
        const int end = start + l;
        if (!m_find->inScope(start, end))
            continue;

        // check if the result is inside the visible area for long blocks
        const QTextLine &startLine = q->editorLayout()->blockLayout(block)->lineForTextPosition(idx);
        const QTextLine &endLine = q->editorLayout()->blockLayout(block)->lineForTextPosition(idx + l);

        if (startLine.isValid() && endLine.isValid()
                && startLine.lineNumber() == endLine.lineNumber()) {
            const int lineY = int(endLine.y() + q->blockBoundingGeometry(block).y());
            if (startLine.cursorToX(idx) > right) { // result is behind the visible area
                if (endLine.lineNumber() >= q->editorLayout()->blockLineCount(block) - 1)
                    break; // this is the last line in the block, nothing more to add

                // skip to the start of the next line
                idx = q->editorLayout()->blockLayout(block)->lineAt(endLine.lineNumber() + 1).textStart();
                l = 0;
                continue;
            } else if (endLine.cursorToX(idx + l, QTextLine::Trailing) < left) { // result is in front of the visible area skip it
                continue;
            } else if (lineY + endLine.height() < top) {
                if (endLine.lineNumber() >= q->editorLayout()->blockLineCount(block) - 1)
                    break; // this is the last line in the block, nothing more to add
                // before visible area, skip to the start of the next line
                idx = q->editorLayout()->blockLayout(block)->lineAt(endLine.lineNumber() + 1).textStart();
                l = 0;
                continue;
            } else if (lineY > bottom) {
                break; // under the visible area, nothing more to add
            }
        }

        const uint flag = (idx == cursor.selectionStart() - blockPosition
                           && idx + l == cursor.selectionEnd() - blockPosition) ?
                    TextEditorOverlay::DropShadow : 0;
        m_searchResultOverlay->addOverlaySelection(start, end, searchResultColor, QColor(), flag);
    }
}

void TextEditorWidgetPrivate::highlightSelection(const QTextBlock &block, const PaintEventData &data) const
{
    if (!m_displaySettings.m_highlightSelection || m_cursors.isNull())
        return;
    const QString selection = m_cursors.mainCursor().selectedText();
    if (selection.trimmed().isEmpty())
        return;
    for (auto cursor : m_cursors) {
        if (cursor.selectedText() != selection)
            return;
    }

    const int blockPosition = block.position();

    QString text = block.text();
    text.replace(QChar::Nbsp, QLatin1Char(' '));
    const int l = selection.length();

    const int left = data.viewportRect.left() - int(data.offset.x());
    const int right = data.viewportRect.right() - int(data.offset.x());
    const int top = data.viewportRect.top() - int(data.offset.y());
    const int bottom = data.viewportRect.bottom() - int(data.offset.y());

    for (int idx = text.indexOf(selection, 0, Qt::CaseInsensitive);
         idx >= 0;
         idx = text.indexOf(selection, idx + 1, Qt::CaseInsensitive)) {
        const int start = blockPosition + idx;
        const int end = start + l;

        // check if the result is inside the visible area for long blocks
        const QTextLine &startLine = block.layout()->lineForTextPosition(idx);
        const QTextLine &endLine = block.layout()->lineForTextPosition(idx + l);

        if (startLine.isValid() && endLine.isValid()
            && startLine.lineNumber() == endLine.lineNumber()) {
            const int lineY = int(endLine.y() + q->blockBoundingGeometry(block).y());
            if (startLine.cursorToX(idx) > right) { // result is behind the visible area
                if (endLine.lineNumber() >= block.lineCount() - 1)
                    break; // this is the last line in the block, nothing more to add

                // skip to the start of the next line
                idx = block.layout()->lineAt(endLine.lineNumber() + 1).textStart();
                continue;
            } else if (endLine.cursorToX(idx + l, QTextLine::Trailing) < left) { // result is in front of the visible area skip it
                continue;
            } else if (lineY + endLine.height() < top) {
                if (endLine.lineNumber() >= block.lineCount() - 1)
                    break; // this is the last line in the block, nothing more to add
                // before visible area, skip to the start of the next line
                idx = block.layout()->lineAt(endLine.lineNumber() + 1).textStart();
                continue;
            } else if (lineY > bottom) {
                break; // under the visible area, nothing more to add
            }
        }

        if (!Utils::contains(m_selectionHighlightOverlay->selections(),
                             [&](const OverlaySelection &selection) {
                                 return selection.m_cursor_begin.position() == start
                                        && selection.m_cursor_end.position() == end;
                             })) {
            m_selectionHighlightOverlay->addOverlaySelection(start, end, {}, {});
        }
    }
}

void TextEditorWidgetPrivate::startCursorFlashTimer()
{
    const int flashTime = QApplication::cursorFlashTime();
    if (flashTime > 0) {
        m_cursorFlashTimer.stop();
        m_cursorFlashTimer.start(flashTime / 2, q);
    }
    if (!m_cursorVisible) {
        m_cursorVisible = true;
        q->viewport()->update(cursorUpdateRect(m_cursors));
    }
}

void TextEditorWidgetPrivate::resetCursorFlashTimer()
{
    if (!m_cursorFlashTimer.isActive())
        return;
    const int flashTime = QApplication::cursorFlashTime();
    if (flashTime > 0) {
        m_cursorFlashTimer.stop();
        m_cursorFlashTimer.start(flashTime / 2, q);
    }
    if (!m_cursorVisible) {
        m_cursorVisible = true;
        q->viewport()->update(cursorUpdateRect(m_cursors));
    }
}

void TextEditorWidgetPrivate::updateCursorSelections()
{
    const QTextCharFormat selectionFormat = TextEditorSettings::fontSettings().toTextCharFormat(
        C_SELECTION);
    QList<QTextEdit::ExtraSelection> selections;
    for (const QTextCursor &cursor : m_cursors) {
        if (cursor.hasSelection())
            selections << QTextEdit::ExtraSelection{cursor, selectionFormat};
    }
    q->setExtraSelections(TextEditorWidget::CursorSelection, selections);

    m_selectionHighlightOverlay->clear();

    if (m_selectionHighlightFuture.isRunning())
        m_selectionHighlightFuture.cancel();

    m_selectionResults.clear();
    if (!m_highlightScrollBarController)
        return;
    m_highlightScrollBarController->removeHighlights(Constants::SCROLL_BAR_SELECTION);

    if (!m_displaySettings.m_highlightSelection || m_cursors.isNull())
        return;
    const QString txt = m_cursors.mainCursor().selectedText();
    if (txt.trimmed().isEmpty())
        return;
    for (auto cursor : m_cursors) {
        if (cursor.selectedText() != txt)
            return;
    }

    m_selectionHighlightFuture = Utils::asyncRun(Utils::searchInContents,
                                                 txt,
                                                 FindFlags{},
                                                 m_document->filePath(),
                                                 m_document->plainText());

    Utils::onResultReady(m_selectionHighlightFuture,
                         this,
                         [this](const SearchResultItems &resultList) {
                             QList<SearchResult> results;
                             for (const SearchResultItem &item : resultList) {
                                 int start = item.mainRange().begin.toPositionInDocument(
                                     m_document->document());
                                 int end = item.mainRange().end.toPositionInDocument(
                                     m_document->document());
                                 results << SearchResult{start, end - start};
                             }
                             m_selectionResults = results;
                             addSelectionHighlightToScrollBar(results);
                         });
}

void TextEditorWidgetPrivate::moveCursor(QTextCursor::MoveOperation operation,
                                         QTextCursor::MoveMode mode)
{
    MultiTextCursor cursor = m_cursors;
    cursor.movePosition(operation, mode, 1, q->editorLayout());
    q->setMultiTextCursor(cursor);
}

QRect TextEditorWidgetPrivate::cursorUpdateRect(const MultiTextCursor &cursor)
{
    QRect result(0, 0, 0, 0);
    for (const QTextCursor &c : cursor)
        result |= q->cursorRect(c);
    return result;
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

    const int blendFactor = level * (256 / (count - 1));

    return blendColors(color80, color90, blendFactor);
}

static QTextLayout::FormatRange createBlockCursorCharFormatRange(int pos,
                                                                 const QColor &textColor,
                                                                 const QColor &baseColor)
{
    QTextLayout::FormatRange o;
    o.start = pos;
    o.length = 1;
    o.format.setForeground(baseColor);
    o.format.setBackground(textColor);
    return o;
}

static TextMarks availableMarks(const TextMarks &marks,
                                QRectF &boundingRect,
                                const QFontMetrics &fm,
                                const qreal itemOffset)
{
    TextMarks ret;
    bool first = true;
    for (TextMark *mark : marks) {
        const TextMark::AnnotationRects &rects = mark->annotationRects(
                    boundingRect, fm, first ? 0 : itemOffset, 0);
        if (rects.annotationRect.isEmpty())
            break;
        boundingRect.setLeft(rects.fadeOutRect.right());
        ret.append(mark);
        if (boundingRect.isEmpty())
            break;
        first = false;
    }
    return ret;
}

QRectF TextEditorWidgetPrivate::getLastLineLineRect(const QTextBlock &block)
{
    QTextLayout *layout = nullptr;
    if (TextSuggestion *suggestion = TextBlockUserData::suggestion(block)) {
        layout = suggestion->replacementDocument()->firstBlock().layout();
        QTC_ASSERT(layout, layout = q->editorLayout()->blockLayout(block));
    } else {
        layout = q->editorLayout()->blockLayout(block);
    }

    const int lineCount = layout->lineCount();
    if (lineCount < 1)
        return {};
    const QTextLine line = layout->lineAt(lineCount - 1);
    const QPointF contentOffset = q->contentOffset();
    const qreal top = q->blockBoundingGeometry(block).translated(contentOffset).top();
    return line.naturalTextRect().translated(contentOffset.x(), top).adjusted(0, 0, -1, -1);
}

bool TextEditorWidgetPrivate::updateAnnotationBounds(const QTextBlock &block,
                                                     TextDocumentLayout *layout,
                                                     bool annotationsVisible)
{
    const bool additionalHeightNeeded = annotationsVisible
            && m_displaySettings.m_annotationAlignment == AnnotationAlignment::BetweenLines;

    int additionalHeight = 0;
    if (additionalHeightNeeded) {
        if (TextEditorSettings::fontSettings().relativeLineSpacing() == 100)
            additionalHeight = q->fontMetrics().lineSpacing();
        else
            additionalHeight = TextEditorSettings::fontSettings().lineSpacing();
    }

    if (TextBlockUserData::additionalAnnotationHeight(block) == additionalHeight)
        return false;
    TextBlockUserData::setAdditionalAnnotationHeight(block, additionalHeight);
    q->viewport()->update();
    layout->emitDocumentSizeChanged();
    return true;
}

void TextEditorWidgetPrivate::updateLineAnnotation(const PaintEventData &data,
                                                   const PaintEventBlockData &blockData,
                                                   QPainter &painter)
{
    const QList<AnnotationRect> previousRects = m_annotationRects.take(data.block.blockNumber());

    if (!m_displaySettings.m_displayAnnotations)
        return;

    TextBlockUserData *blockUserData = TextBlockUserData::textUserData(data.block);
    if (!blockUserData)
        return;

    TextMarks marks = Utils::filtered(blockUserData->marks(), [](const TextMark *mark) {
        return !mark->lineAnnotation().isEmpty() && mark->isVisible()
               && !TextDocument::marksAnnotationHidden(mark->category().id);
    });

    const bool annotationsVisible = !marks.isEmpty();

    if (updateAnnotationBounds(data.block, data.documentLayout, annotationsVisible)
            || !annotationsVisible) {
        return;
    }

    const QRectF lineRect = getLastLineLineRect(data.block);
    if (lineRect.isNull())
        return;

    Utils::sort(marks, [](const TextMark* mark1, const TextMark* mark2){
        return mark1->priority() > mark2->priority();
    });

    qreal itemOffset = 0.0;
    if (TextEditorSettings::fontSettings().relativeLineSpacing() == 100)
        itemOffset = q->fontMetrics().lineSpacing();
    else
        itemOffset = blockData.boundingRect.height();

    const qreal initialOffset = m_displaySettings.m_annotationAlignment == AnnotationAlignment::BetweenLines ? itemOffset / 2 : itemOffset * 2;
    const qreal minimalContentWidth = charWidth() * m_displaySettings.m_minimalAnnotationContent;
    qreal offset = initialOffset;
    qreal x = 0;
    if (marks.isEmpty())
        return;
    QRectF boundingRect;
    if (m_displaySettings.m_annotationAlignment == AnnotationAlignment::BetweenLines) {
        boundingRect = QRectF(lineRect.bottomLeft(), blockData.boundingRect.bottomRight());
    } else {
        boundingRect = QRectF(lineRect.topLeft().x(), lineRect.topLeft().y(),
                              q->viewport()->width() - lineRect.right(), lineRect.height());
        x = lineRect.right();
        if (m_displaySettings.m_annotationAlignment == AnnotationAlignment::NextToMargin
                && data.rightMargin > lineRect.right() + offset
                && q->viewport()->width() > data.rightMargin + minimalContentWidth) {
            offset = data.rightMargin - lineRect.right();
        } else if (m_displaySettings.m_annotationAlignment != AnnotationAlignment::NextToContent) {
            marks = availableMarks(marks, boundingRect, q->fontMetrics(), itemOffset);
            if (boundingRect.width() > 0)
                offset = qMax(boundingRect.width(), initialOffset);
        }
    }

    QList<AnnotationRect> newRects;
    for (const TextMark *mark : std::as_const(marks)) {
        boundingRect = QRectF(x, boundingRect.top(), q->viewport()->width() - x, boundingRect.height());
        if (boundingRect.isEmpty())
            break;

        mark->paintAnnotation(painter,
                              data.eventRect,
                              &boundingRect,
                              offset,
                              itemOffset / 2,
                              q->contentOffset());

        x = boundingRect.right();
        offset = itemOffset / 2;
        newRects.append({boundingRect, mark});
    }

    if (previousRects != newRects) {
        for (const AnnotationRect &annotationRect : std::as_const(newRects))
            q->viewport()->update(annotationRect.rect.toAlignedRect());
        for (const AnnotationRect &annotationRect : previousRects)
            q->viewport()->update(annotationRect.rect.toAlignedRect());
    }
    m_annotationRects[data.block.blockNumber()] = newRects;
    QTC_ASSERT(data.lineSpacing != 0, return);
    const int maxVisibleLines = data.viewportRect.height() / data.lineSpacing;
    if (m_annotationRects.size() >= maxVisibleLines * 2)
        scheduleCleanupAnnotationCache();
}

QColor blendRightMarginColor(const FontSettings &settings, bool areaColor)
{
    const QColor baseColor = settings.toTextCharFormat(C_TEXT).background().color();
    const QColor col = (baseColor.value() > 128) ? Qt::black : Qt::white;
    return blendColors(baseColor, col, areaColor ? 16 : 32);
}

void TextEditorWidgetPrivate::paintRightMarginArea(PaintEventData &data, QPainter &painter) const
{
    if (m_visibleWrapColumn <= 0)
        return;
    // Don't use QFontMetricsF::averageCharWidth here, due to it returning
    // a fractional size even when this is not supported by the platform.
    data.rightMargin = charWidth() * (m_visibleWrapColumn + m_visualIndentOffset)
            + data.offset.x() + 4;
    if (m_marginSettings.m_tintMarginArea && data.rightMargin < data.viewportRect.width()) {
        const QRectF behindMargin(data.rightMargin,
                                  data.eventRect.top(),
                                  data.viewportRect.width() - data.rightMargin,
                                  data.eventRect.height());
        painter.fillRect(behindMargin, blendRightMarginColor(m_document->fontSettings(), true));
    }
}

void TextEditorWidgetPrivate::paintRightMarginLine(const PaintEventData &data,
                                                   QPainter &painter) const
{
    if (m_visibleWrapColumn <= 0 || data.rightMargin >= data.viewportRect.width())
        return;

    const QPen pen = painter.pen();
    painter.setPen(blendRightMarginColor(m_document->fontSettings(), false));
    painter.drawLine(QPointF(data.rightMargin, data.eventRect.top()),
                     QPointF(data.rightMargin, data.eventRect.bottom()));
    painter.setPen(pen);
}

void TextEditorWidgetPrivate::paintBlockHighlight(const PaintEventData &data,
                                                  QPainter &painter) const
{
    if (m_highlightBlocksInfo.isEmpty())
        return;

    const QColor baseColor = m_document->fontSettings().toTextCharFormat(C_TEXT).background().color();

    // extra pass for the block highlight

    const int margin = 5;
    QTextBlock block = data.block;
    QPointF offset = data.offset;
    while (block.isValid()) {
        QRectF blockBoundingRect = q->blockBoundingRect(block).translated(offset);

        int n = block.blockNumber();
        int depth = 0;
        const QList<int> open = m_highlightBlocksInfo.open;
        for (const int i : open)
            if (n >= i)
                ++depth;
        const QList<int> close = m_highlightBlocksInfo.close;
        for (const int i : close)
            if (n > i)
                --depth;

        int count = m_highlightBlocksInfo.count();
        if (count) {
            for (int i = 0; i <= depth; ++i) {
                const QColor &blendedColor = calcBlendColor(baseColor, i, count);
                int vi = i > 0 ? m_highlightBlocksInfo.visualIndent.at(i - 1) : 0;
                QRectF oneRect = blockBoundingRect;
                oneRect.setWidth(qMax(data.viewportRect.width(), data.documentWidth));
                oneRect.adjust(vi, 0, 0, 0);
                if (oneRect.left() >= oneRect.right())
                    continue;
                if (data.rightMargin > 0 && oneRect.left() < data.rightMargin
                        && oneRect.right() > data.rightMargin) {
                    QRectF otherRect = blockBoundingRect;
                    otherRect.setLeft(data.rightMargin + 1);
                    otherRect.setRight(oneRect.right());
                    oneRect.setRight(data.rightMargin - 1);
                    painter.fillRect(otherRect, blendedColor);
                }
                painter.fillRect(oneRect, blendedColor);
            }
        }
        offset.ry() += blockBoundingRect.height();

        if (offset.y() > data.viewportRect.height() + margin)
            break;

        block = nextVisibleBlock(block);
    }

}

void TextEditorWidgetPrivate::paintSearchResultOverlay(const PaintEventData &data,
                                                       QPainter &painter) const
{
    m_searchResultOverlay->clear();
    if (m_searchExpr.pattern().isEmpty() || !m_searchExpr.isValid())
        return;

    const int margin = 5;
    QTextBlock block = data.block;
    QPointF offset = data.offset;
    while (block.isValid()) {
        QRectF blockBoundingRect = q->blockBoundingRect(block).translated(offset);

        if (blockBoundingRect.bottom() >= data.eventRect.top() - margin
                && blockBoundingRect.top() <= data.eventRect.bottom() + margin) {
            highlightSearchResults(block, data);
        }
        offset.ry() += blockBoundingRect.height();

        if (offset.y() > data.viewportRect.height() + margin)
            break;

        block = nextVisibleBlock(block);
    }

    m_searchResultOverlay->fill(&painter,
                                data.searchResultFormat.background().color(),
                                data.eventRect);
}

void TextEditorWidgetPrivate::paintSelectionOverlay(const PaintEventData &data,
                                                    QPainter &painter) const
{
    const int margin = 5;
    QTextBlock block = data.block;
    QPointF offset = data.offset;
    while (block.isValid()) {
        QRectF blockBoundingRect = q->blockBoundingRect(block).translated(offset);

        if (blockBoundingRect.bottom() >= data.eventRect.top() - margin
            && blockBoundingRect.top() <= data.eventRect.bottom() + margin) {
            highlightSelection(block, data);
        }
        offset.ry() += blockBoundingRect.height();

        if (offset.y() > data.viewportRect.height() + margin)
            break;

        block = nextVisibleBlock(block);
    }

    QColor selection = m_document->fontSettings().toTextCharFormat(C_SELECTION).background().color();
    const QColor text = m_document->fontSettings().toTextCharFormat(C_TEXT).background().color();
    selection.setAlphaF(StyleHelper::luminance(text) > 0.5 ? 0.25 : 0.5);

    m_selectionHighlightOverlay->fill(&painter, selection, data.eventRect);
}

void TextEditorWidgetPrivate::paintIfDefedOutBlocks(const PaintEventData &data,
                                                    QPainter &painter) const
{
    QTextBlock block = data.block;
    QPointF offset = data.offset;
    while (block.isValid()) {

        QRectF r = q->blockBoundingRect(block).translated(offset);

        if (r.bottom() >= data.eventRect.top() && r.top() <= data.eventRect.bottom()) {
            if (TextBlockUserData::ifdefedOut(block)) {
                QRectF rr = r;
                rr.setRight(data.viewportRect.width() - offset.x());
                if (data.rightMargin > 0)
                    rr.setRight(qMin(data.rightMargin, rr.right()));
                painter.fillRect(rr, data.ifdefedOutFormat.background());
            }
        }
        offset.ry() += r.height();

        if (offset.y() > data.viewportRect.height())
            break;

        block = nextVisibleBlock(block);
    }
}

void TextEditorWidgetPrivate::paintFindScope(const PaintEventData &data, QPainter &painter) const
{
    if (m_findScope.isNull())
        return;
    auto overlay = new TextEditorOverlay(q);
    for (const QTextCursor &c : m_findScope) {
        overlay->addOverlaySelection(c.selectionStart(),
                                     c.selectionEnd(),
                                     data.searchScopeFormat.foreground().color(),
                                     data.searchScopeFormat.background().color(),
                                     TextEditorOverlay::ExpandBegin);
    }
    overlay->setAlpha(false);
    overlay->paint(&painter, data.eventRect);
    delete overlay;
}

void TextEditorWidgetPrivate::paintCurrentLineHighlight(const PaintEventData &data,
                                                        QPainter &painter) const
{
    if (!m_highlightCurrentLine)
        return;

    QList<QTextCursor> cursorsForBlock;
    for (const QTextCursor &c : m_cursors) {
        if (c.block() == data.block)
            cursorsForBlock << c;
    }
    if (cursorsForBlock.isEmpty())
        return;

    const QRectF blockRect = q->blockBoundingRect(data.block).translated(data.offset);
    QColor color = m_document->fontSettings().toTextCharFormat(C_CURRENT_LINE).background().color();
    color.setAlpha(128);
    QSet<int> seenLines;
    for (const QTextCursor &cursor : std::as_const(cursorsForBlock)) {
        QTextLayout *layout = q->editorLayout()->blockLayout(data.block);
        const QTextLine line = layout->lineForTextPosition(cursor.positionInBlock());
        QTC_ASSERT(line.isValid(), continue);
        if (!Utils::insert(seenLines, line.lineNumber()))
            continue;
        QRectF lineRect = line.rect();
        lineRect.moveTop(lineRect.top() + blockRect.top());
        lineRect.setLeft(0);
        lineRect.setRight(data.viewportRect.width());
        painter.fillRect(lineRect, color);
    }
}

QRectF TextEditorWidgetPrivate::cursorBlockRect(const QTextDocument *doc,
                                                const QTextBlock &block,
                                                int cursorPosition,
                                                QRectF blockBoundingRect,
                                                bool *doSelection) const
{
    const qreal space = charWidth();
    int relativePos = cursorPosition - block.position();
    qobject_cast<TextDocumentLayout *>(m_document->document()->documentLayout())
        ->ensureBlockLayout(block);
    QTextLine line = q->editorLayout()->blockLayout(block)->lineForTextPosition(relativePos);
    QTC_ASSERT(line.isValid(), return {});
    qreal x = line.cursorToX(relativePos);
    qreal w = 0;
    if (relativePos < line.textLength() - line.textStart()) {
        w = line.cursorToX(relativePos + 1) - x;
        if (doc->characterAt(cursorPosition) == QLatin1Char('\t')) {
            if (doSelection)
                *doSelection = false;
            if (w > space) {
                x += w - space;
                w = space;
            }
        }
    } else {
        w = space; // in sync with QTextLine::draw()
    }

    if (blockBoundingRect.isEmpty())
        blockBoundingRect = q->blockBoundingGeometry(block).translated(q->contentOffset());

    QRectF cursorRect = line.rect();
    cursorRect.moveTop(cursorRect.top() + blockBoundingRect.top());
    cursorRect.moveLeft(blockBoundingRect.left() + x);
    cursorRect.setWidth(w);
    return cursorRect;
}

void TextEditorWidgetPrivate::paintCursorAsBlock(const PaintEventData &data,
                                                 QPainter &painter,
                                                 PaintEventBlockData &blockData,
                                                 int cursorPosition) const
{
    bool doSelection = true;
    const QRectF cursorRect = cursorBlockRect(data.doc,
                                              data.block,
                                              cursorPosition,
                                              blockData.boundingRect,
                                              &doSelection);
    const QTextCharFormat textFormat = data.fontSettings.toTextCharFormat(C_TEXT);
    painter.fillRect(cursorRect, textFormat.foreground());
    int relativePos = cursorPosition - blockData.position;
    if (doSelection) {
        blockData.selections.append(
            createBlockCursorCharFormatRange(relativePos,
                                             textFormat.foreground().color(),
                                             textFormat.background().color()));
    }
}

void TextEditorWidgetPrivate::paintAdditionalVisualWhitespaces(PaintEventData &data,
                                                               QPainter &painter,
                                                               qreal top) const
{
    if (!m_displaySettings.m_visualizeWhitespace)
        return;

    QTextLayout *layout = q->editorLayout()->blockLayout(data.block);
    const bool nextBlockIsValid = data.block.next().isValid();
    int lineCount = layout->lineCount();
    if (lineCount >= 2 || !nextBlockIsValid) {
        painter.save();
        painter.setPen(data.visualWhitespaceFormat.foreground().color());
        for (int i = 0; i < lineCount-1; ++i) { // paint line wrap indicator
            QTextLine line = layout->lineAt(i);
            QRectF lineRect = line.naturalTextRect().translated(data.offset.x(), top);
            QChar visualArrow(ushort(0x21b5));
            painter.drawText(QPointF(lineRect.right(), lineRect.top() + line.ascent()),
                             visualArrow);
        }
        if (!nextBlockIsValid) { // paint EOF symbol
            if (TextSuggestion *suggestion = TextBlockUserData::suggestion(data.block)) {
                const QTextBlock lastReplacementBlock
                    = suggestion->replacementDocument()->lastBlock();
                for (QTextBlock block = suggestion->replacementDocument()->firstBlock();
                     block != lastReplacementBlock && block.isValid();
                     block = block.next()) {
                    top += suggestion->replacementDocument()
                               ->documentLayout()
                               ->blockBoundingRect(block)
                               .height();
                }
                layout = q->editorLayout()->blockLayout(lastReplacementBlock);
                lineCount = layout->lineCount();
            }
            QTextLine line = layout->lineAt(lineCount - 1);
            QRectF lineRect = line.naturalTextRect().translated(data.offset.x(), top);
            int h = 4;
            lineRect.adjust(0, 0, -1, -1);
            QPainterPath path;
            QPointF pos(lineRect.topRight() + QPointF(h + 4, line.ascent()));
            path.moveTo(pos);
            path.lineTo(pos + QPointF(-h, -h));
            path.lineTo(pos + QPointF(0, -2 * h));
            path.lineTo(pos + QPointF(h, -h));
            path.closeSubpath();
            painter.setBrush(painter.pen().color());
            painter.drawPath(path);
        }
        painter.restore();
    }
}

int TextEditorWidgetPrivate::indentDepthForBlock(const QTextBlock &block, const PaintEventData &data)
{
    const auto blockDepth = [&](const QTextBlock &block) {
        int depth = m_visualIndentCache.value(block.blockNumber(), -1);
        if (depth < 0) {
            const QString text = block.text().mid(m_visualIndentOffset);
            depth = text.simplified().isEmpty() ? -1 : data.tabSettings.indentationColumn(text);
        }
        return depth;
    };
    const auto ensureCacheSize = [&](const int size) {
        if (m_visualIndentCache.size() < size)
            m_visualIndentCache.resize(size, -1);
    };
    int depth = blockDepth(block);
    if (depth < 0) {
        // find previous non empty block and get the indent depth of this block
        QTextBlock it = block.previous();
        int prevDepth = -1;
        while (it.isValid()) {
            prevDepth = blockDepth(it);
            if (prevDepth >= 0)
                break;
            it = it.previous();
        }
        const int startBlockNumber = it.isValid() ? it.blockNumber() + 1 : 0;

        // find next non empty block and get the indent depth of this block
        it = block.next();
        int nextDepth = -1;
        while (it.isValid()) {
            nextDepth = blockDepth(it);
            if (nextDepth >= 0)
                break;
            it = it.next();
        }
        const int endBlockNumber = it.isValid() ? it.blockNumber() : m_blockCount;

        // get the depth for the whole range of empty blocks and fill the cache so we do not need to
        // redo this for every paint event
        depth = prevDepth > 0 && nextDepth > 0 ? qMin(prevDepth, nextDepth) : 0;
        ensureCacheSize(endBlockNumber);
        for (int i = startBlockNumber; i < endBlockNumber; ++i)
            m_visualIndentCache[i] = depth;
    }
    return depth;
}

void TextEditorWidgetPrivate::paintIndentDepth(PaintEventData &data,
                                               QPainter &painter,
                                               const PaintEventBlockData &blockData)
{
    if (!m_displaySettings.m_visualizeIndent)
        return;

    const int depth = indentDepthForBlock(data.block, data);
    if (depth <= 0 || blockData.layout->lineCount() < 1)
        return;

    const qreal singleAdvance = charWidth();
    const qreal indentAdvance = singleAdvance * data.tabSettings.m_indentSize;

    painter.save();

    const QTextLine textLine = blockData.layout->lineAt(0);
    const QRectF rect = textLine.naturalTextRect();
    qreal x = textLine.x() + data.offset.x() + qMax(0, q->cursorWidth() - 1)
              + singleAdvance * m_visualIndentOffset;
    int paintColumn = 0;

    QList<int> cursorPositions;
    for (const QTextCursor & cursor : m_cursors) {
        if (cursor.block() == data.block)
            cursorPositions << cursor.positionInBlock();
    }

    const QColor normalColor = data.visualWhitespaceFormat.foreground().color();
    QColor cursorColor = normalColor;
    cursorColor.setAlpha(cursorColor.alpha() / 2);

    const QString text = data.block.text().mid(m_visualIndentOffset);
    while (paintColumn < depth) {
        if (x >= 0) {
            int paintPosition = data.tabSettings.positionAtColumn(text, paintColumn);
            if (q->lineWrapMode() == PlainTextEdit::WidgetWidth
                && blockData.layout->lineForTextPosition(paintPosition).lineNumber() != 0) {
                break;
            }
            if (cursorPositions.contains(paintPosition))
                painter.setPen(cursorColor);
            else
                painter.setPen(normalColor);
            const QPointF top(x, blockData.boundingRect.top());
            const QPointF bottom(x, blockData.boundingRect.top() + rect.height());
            const QLineF line(top, bottom);
            painter.drawLine(line);
        }
        x += indentAdvance;
        paintColumn += data.tabSettings.m_indentSize;
    }
    painter.restore();
}

void TextEditorWidgetPrivate::paintReplacement(PaintEventData &data, QPainter &painter,
                                               qreal top) const
{
    QTextBlock nextBlock = data.block.next();

    if (nextBlock.isValid() && !nextBlock.isVisible() && q->replacementVisible(data.block.blockNumber())) {
        const bool selectThis = (data.textCursor.hasSelection()
                                 && nextBlock.position() >= data.textCursor.selectionStart()
                                 && nextBlock.position() < data.textCursor.selectionEnd());


        const QTextCharFormat selectionFormat = data.fontSettings.toTextCharFormat(C_SELECTION);

        painter.save();
        if (selectThis) {
            painter.setBrush(selectionFormat.background().style() != Qt::NoBrush
                                 ? selectionFormat.background()
                                 : QApplication::palette().brush(QPalette::Highlight));
        } else {
            QColor rc = q->replacementPenColor(data.block.blockNumber());
            if (rc.isValid())
                painter.setPen(rc);
        }

        QTextLayout *layout = q->editorLayout()->blockLayout(data.block);
        QTextLine line = layout->lineAt(layout->lineCount()-1);
        QRectF lineRect = line.naturalTextRect().translated(data.offset.x(), top);
        lineRect.adjust(0, 0, -1, -1);

        QString replacement = q->foldReplacementText(data.block);
        QString rectReplacement = QLatin1String(" {") + replacement + QLatin1String("}; ");

        QRectF collapseRect(lineRect.right() + 12,
                            lineRect.top(),
                            q->fontMetrics().horizontalAdvance(rectReplacement),
                            lineRect.height());
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.translate(.5, .5);
        painter.drawRoundedRect(collapseRect.adjusted(0, 0, 0, -1), 3, 3);
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.translate(-.5, -.5);

        if (TextBlockUserData::foldingStartIncluded(nextBlock))
            replacement.prepend(nextBlock.text().trimmed().at(0));

        QTextBlock lastInvisibleBlock = nextVisibleBlock(data.block).previous();
        if (!lastInvisibleBlock.isValid())
            lastInvisibleBlock = data.doc->lastBlock();

        if (TextBlockUserData::foldingEndIncluded(lastInvisibleBlock)) {
            QString right = lastInvisibleBlock.text().trimmed();
            if (right.endsWith(QLatin1Char(';'))) {
                right.chop(1);
                right = right.trimmed();
                replacement.append(right.right(right.endsWith('/') ? 2 : 1));
                replacement.append(QLatin1Char(';'));
            } else {
                replacement.append(right.right(right.endsWith('/') ? 2 : 1));
            }
        }

        if (selectThis)
            painter.setPen(selectionFormat.foreground().color());
        painter.drawText(collapseRect, Qt::AlignCenter, replacement);
        painter.restore();
    }
}

void TextEditorWidgetPrivate::paintWidgetBackground(const PaintEventData &data,
                                                    QPainter &painter) const
{
    painter.fillRect(data.eventRect, data.fontSettings.toTextCharFormat(C_TEXT).background());
}

void TextEditorWidgetPrivate::paintOverlays(const PaintEventData &data, QPainter &painter) const
{
    // draw the overlays, but only if we do not have a find scope, otherwise the
    // view becomes too noisy.
    if (m_findScope.isNull()) {
        if (m_overlay->isVisible())
            m_overlay->paint(&painter, data.eventRect);

        if (m_snippetOverlay->isVisible())
            m_snippetOverlay->paint(&painter, data.eventRect);

        if (!m_refactorOverlay->isEmpty())
            m_refactorOverlay->paint(&painter, data.eventRect);
    }

    if (!m_searchResultOverlay->isEmpty()) {
        m_searchResultOverlay->paint(&painter, data.eventRect);
        m_searchResultOverlay->clear();
    }
}

void TextEditorWidgetPrivate::paintCursor(const PaintEventData &data, QPainter &painter) const
{
    for (const CursorData &cursor : data.cursors) {
        painter.setPen(cursor.pen);
        cursor.layout->drawCursor(&painter, cursor.offset, cursor.pos, q->cursorWidth());
    }
}

void TextEditorWidgetPrivate::setupBlockLayout(const PaintEventData &data,
                                               QPainter &painter,
                                               PaintEventBlockData &blockData) const
{
    blockData.layout = q->editorLayout()->blockLayout(data.block);

    QTextOption option = blockData.layout->textOption();
    if (data.suppressSyntaxInIfdefedOutBlock
            && TextBlockUserData::ifdefedOut(data.block)) {
        option.setFlags(option.flags() | QTextOption::SuppressColors);
        painter.setPen(data.ifdefedOutFormat.foreground().color());
    } else {
        option.setFlags(option.flags() & ~QTextOption::SuppressColors);
        painter.setPen(data.context.palette.text().color());
    }
    blockData.layout->setTextOption(option);
    blockData.layout->setFont(data.doc->defaultFont());
}

void TextEditorWidgetPrivate::setupSelections(const PaintEventData &data,
                                              PaintEventBlockData &blockData) const
{
    QList<QTextLayout::FormatRange> prioritySelections;

    int deltaPos = -1;
    int delta = 0;

    if (TextSuggestion *suggestion = TextBlockUserData::suggestion(data.block)) {
        deltaPos = suggestion->currentPosition() - data.block.position();
        const QString trailingText = data.block.text().mid(deltaPos);
        if (!trailingText.isEmpty()) {
            const int trailingIndex = suggestion->replacementDocument()
                                          ->firstBlock()
                                          .text()
                                          .indexOf(trailingText, deltaPos);
            if (trailingIndex >= 0)
                delta = std::max(trailingIndex - deltaPos, 0);
        }
    }

    for (int i = 0; i < data.context.selections.size(); ++i) {
        const QAbstractTextDocumentLayout::Selection &range = data.context.selections.at(i);
        const int selStart = range.cursor.selectionStart() - blockData.position;
        const int selEnd = range.cursor.selectionEnd() - blockData.position;
        if (selStart < blockData.length && selEnd >= 0
            && selEnd >= selStart) {
            QTextLayout::FormatRange o;
            o.start = selStart;
            o.length = selEnd - selStart;
            o.format = range.format;
            QTextLayout::FormatRange rest;
            rest.start = -1;
            if (deltaPos >= 0 && delta != 0) {
                if (o.start >= deltaPos) {
                    o.start += delta;
                } else if (o.start + o.length > deltaPos) {
                    // the format range starts before and ends after the position so we need to
                    // split the format into before and after the suggestion format ranges
                    rest.start = deltaPos + delta;
                    rest.length = o.length - (deltaPos - o.start);
                    rest.format = o.format;
                    o.length = deltaPos - o.start;
                }
            }

            o.format = range.format;
            if (data.textCursor.hasSelection() && data.textCursor == range.cursor
                && data.textCursor.anchor() == range.cursor.anchor()) {
                const QTextCharFormat selectionFormat = data.fontSettings.toTextCharFormat(C_SELECTION);
                if (selectionFormat.background().style() != Qt::NoBrush)
                    o.format.setBackground(selectionFormat.background());
                o.format.setForeground(selectionFormat.foreground());
            }
            if ((data.textCursor.hasSelection() && i == data.context.selections.size() - 1)
                || (o.format.foreground().style() == Qt::NoBrush
                && o.format.underlineStyle() != QTextCharFormat::NoUnderline
                && o.format.background() == Qt::NoBrush)) {
                if (q->selectionVisible(data.block.blockNumber())) {
                    prioritySelections.append(o);
                    if (rest.start >= 0)
                        prioritySelections.append(rest);
                }
            } else {
                blockData.selections.append(o);
                if (rest.start >= 0)
                    blockData.selections.append(rest);
            }
        }
    }
    blockData.selections.append(prioritySelections);
}

static CursorData generateCursorData(const int cursorPos,
                                     const PaintEventData &data,
                                     const PaintEventBlockData &blockData,
                                     QPainter &painter)
{
    CursorData cursorData;
    cursorData.layout = blockData.layout;
    cursorData.offset = data.offset;
    cursorData.pos = cursorPos;
    cursorData.pen = painter.pen();
    return cursorData;
}

static bool blockContainsCursor(const PaintEventBlockData &blockData, const QTextCursor &cursor)
{
    const int pos = cursor.position();
    return pos >= blockData.position && pos < blockData.position + blockData.length;
}

void TextEditorWidgetPrivate::addCursorsPosition(PaintEventData &data,
                                                 QPainter &painter,
                                                 const PaintEventBlockData &blockData) const
{
    if (!m_dndCursor.isNull()) {
        if (blockContainsCursor(blockData, m_dndCursor)) {
            data.cursors.append(
                generateCursorData(m_dndCursor.positionInBlock(), data, blockData, painter));
        }
    } else {
        for (const QTextCursor &cursor : m_cursors) {
            if (blockContainsCursor(blockData, cursor)) {
                data.cursors.append(
                    generateCursorData(cursor.positionInBlock(), data, blockData, painter));
            }
        }
    }
}

QTextBlock TextEditorWidgetPrivate::nextVisibleBlock(const QTextBlock &block)
{
    QTextBlock nextVisibleBlock = block.next();
    while (nextVisibleBlock.isValid() && !nextVisibleBlock.isVisible())
        nextVisibleBlock = nextVisibleBlock.next();
    return nextVisibleBlock;
}

void TextEditorWidgetPrivate::scheduleCleanupAnnotationCache()
{
    if (cleanupAnnotationRectsScheduled)
        return;
    QMetaObject::invokeMethod(this,
                              &TextEditorWidgetPrivate::cleanupAnnotationCache,
                              Qt::QueuedConnection);
    cleanupAnnotationRectsScheduled = true;
}

void TextEditorWidgetPrivate::cleanupAnnotationCache()
{
    cleanupAnnotationRectsScheduled = false;
    const int firstVisibleBlock = q->firstVisibleBlockNumber();
    const int lastVisibleBlock = q->lastVisibleBlockNumber();
    auto lineIsVisble = [&](int blockNumber){
        auto behindFirstVisibleBlock = [&](){
            return firstVisibleBlock >= 0 && blockNumber >= firstVisibleBlock;
        };
        auto beforeLastVisibleBlock = [&](){
            return lastVisibleBlock < 0 ||  (lastVisibleBlock >= 0 && blockNumber <= lastVisibleBlock);
        };
        return behindFirstVisibleBlock() && beforeLastVisibleBlock();
    };
    auto it = m_annotationRects.begin();
    auto end = m_annotationRects.end();
    while (it != end) {
        if (!lineIsVisble(it.key()))
            it = m_annotationRects.erase(it);
        else
            ++it;
    }
}

void TextEditorWidget::paintEvent(QPaintEvent *e)
{
    PaintEventData data(this, e, contentOffset());
    QTC_ASSERT(data.documentLayout, return);

    QPainter painter(viewport());
    // Set a brush origin so that the WaveUnderline knows where the wave started
    painter.setBrushOrigin(data.offset);

    data.block = firstVisibleBlock();
    data.context = getPaintContext();
    const QTextCharFormat textFormat = textDocument()->fontSettings().toTextCharFormat(C_TEXT);
    data.context.palette.setBrush(QPalette::Text, textFormat.foreground());
    data.context.palette.setBrush(QPalette::Base, textFormat.background());

    { // paint background
        d->paintWidgetBackground(data, painter);
        // draw backgrond to the right of the wrap column before everything else
        d->paintRightMarginArea(data, painter);
        // paint a blended background color depending on scope depth
        d->paintBlockHighlight(data, painter);
        // paint background of if defed out blocks in bigger chunks
        d->paintIfDefedOutBlocks(data, painter);
        d->paintRightMarginLine(data, painter);
        // paint find scope on top of ifdefed out blocks and right margin
        d->paintFindScope(data, painter);
        // paint search results on top of the find scope
        d->paintSearchResultOverlay(data, painter);
        // paint selection highlights
        d->paintSelectionOverlay(data, painter);
    }

    while (data.block.isValid()) {

        PaintEventBlockData blockData;
        blockData.boundingRect = blockBoundingRect(data.block).translated(data.offset);

        if (blockData.boundingRect.bottom() >= data.eventRect.top()
                && blockData.boundingRect.top() <= data.eventRect.bottom()) {

            data.documentLayout->ensureBlockLayout(data.block);
            d->setupBlockLayout(data, painter, blockData);
            blockData.position = data.block.position();
            blockData.length = data.block.length();
            d->setupSelections(data, blockData);

            d->paintCurrentLineHighlight(data, painter);

            bool drawCursor = false;
            bool drawCursorAsBlock = false;
            if (d->m_dndCursor.isNull()) {
                drawCursor = d->m_cursorVisible
                             && Utils::anyOf(d->m_cursors, [&](const QTextCursor &cursor) {
                                    return blockContainsCursor(blockData, cursor);
                                });
                drawCursorAsBlock = drawCursor && overwriteMode();
            } else {
                drawCursor = blockContainsCursor(blockData, d->m_dndCursor);
            }

            if (drawCursorAsBlock) {
                for (const QTextCursor &cursor : multiTextCursor()) {
                    if (blockContainsCursor(blockData, cursor))
                        d->paintCursorAsBlock(data, painter, blockData, cursor.position());
                }
            }

            paintBlock(&painter, data.block, data.offset, blockData.selections, data.eventRect);

            if (data.isEditable && !blockData.layout->preeditAreaText().isEmpty()) {
                if (data.context.cursorPosition < -1) {
                    const int cursorPos = blockData.layout->preeditAreaPosition()
                                          - (data.context.cursorPosition + 2);
                    data.cursors = {generateCursorData(cursorPos, data, blockData, painter)};
                }
            } else if (drawCursor && !drawCursorAsBlock) {
                d->addCursorsPosition(data, painter, blockData);
            }
            d->paintIndentDepth(data, painter, blockData);
            d->paintAdditionalVisualWhitespaces(data, painter, blockData.boundingRect.top());
            d->paintReplacement(data, painter, blockData.boundingRect.top());
            d->updateLineAnnotation(data, blockData, painter);
        }

        data.offset.ry() += blockData.boundingRect.height();

        if (data.offset.y() > data.viewportRect.height())
            break;

        data.block = data.block.next();

        if (!data.block.isVisible()) {
            if (data.block.blockNumber() == d->visibleFoldedBlockNumber) {
                data.visibleCollapsedBlock = data.block;
                data.visibleCollapsedBlockOffset = data.offset;
            }

            data.block = TextEditorWidgetPrivate::nextVisibleBlock(data.block);
        }
    }

    painter.setPen(data.context.palette.text().color());

    d->updateAnimator(d->m_bracketsAnimator, painter);
    d->updateAnimator(d->m_autocompleteAnimator, painter);

    d->paintOverlays(data, painter);

    // draw the cursor last, on top of everything
    d->paintCursor(data, painter);

    // paint a popup with the content of the collapsed block
    drawCollapsedBlockPopup(painter, data.visibleCollapsedBlock,
                            data.visibleCollapsedBlockOffset, data.eventRect);
}

void TextEditorWidget::paintBlock(QPainter *painter,
                                  const QTextBlock &block,
                                  const QPointF &offset,
                                  const QList<QTextLayout::FormatRange> &selections,
                                  const QRect &clipRect) const
{
    if (TextSuggestion *suggestion = TextBlockUserData::suggestion(block)) {
        QTextBlock suggestionBlock = suggestion->replacementDocument()->firstBlock();
        QPointF suggestionOffset = offset;
        suggestionOffset.rx() += document()->documentMargin();
        while (suggestionBlock.isValid()) {
            const QList<QTextLayout::FormatRange> blockSelections
                = suggestionBlock.blockNumber() == 0 ? selections
                                                      : QList<QTextLayout::FormatRange>{};
            suggestionBlock.layout()->draw(painter,
                                            suggestionOffset,
                                            blockSelections,
                                            clipRect);
            suggestionOffset.ry() += suggestion->replacementDocument()
                                         ->documentLayout()
                                         ->blockBoundingRect(suggestionBlock)
                                         .height();
            suggestionBlock = suggestionBlock.next();
        }
        return;
    }

    editorLayout()->blockLayout(block)->draw(painter, offset, selections, clipRect);
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
    if (!block.isValid())
        return;

    int margin = int(block.document()->documentMargin());
    qreal maxWidth = 0;
    qreal blockHeight = 0;
    QTextBlock b = block;

    while (!b.isVisible()) {
        b.setVisible(true); // make sure block bounding rect works
        QRectF r = blockBoundingRect(b).translated(offset);

        QTextLayout *layout = editorLayout()->blockLayout(b);
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
    QBrush brush = textDocument()->fontSettings().toTextCharFormat(C_TEXT).background();
    const QTextCharFormat ifdefedOutFormat = textDocument()->fontSettings().toTextCharFormat(
        C_DISABLED_CODE);
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
        QTextLayout *layout = editorLayout()->blockLayout(b);
        QList<QTextLayout::FormatRange> selections;
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
    auto documentLayout = qobject_cast<TextDocumentLayout*>(document()->documentLayout());
    if (!documentLayout)
        return 0;

    if (!d->m_marksVisible && documentLayout->hasMarks)
        d->m_marksVisible = true;

    if (!d->m_marksVisible && !d->m_lineNumbersVisible && !d->m_codeFoldingVisible)
        return 0;

    int space = 0;
    const QFontMetrics fm(d->m_extraArea->fontMetrics());

    if (d->m_lineNumbersVisible) {
        QFont fnt = d->m_extraArea->font();
        // this works under the assumption that bold or italic
        // can only make a font wider
        const QTextCharFormat currentLineNumberFormat
            = textDocument()->fontSettings().toTextCharFormat(C_CURRENT_LINE_NUMBER);
        fnt.setBold(currentLineNumberFormat.font().bold());
        fnt.setItalic(currentLineNumberFormat.font().italic());
        const QFontMetrics linefm(fnt);

        space += linefm.horizontalAdvance(QLatin1Char('9')) * lineNumberDigits();
    }
    int markWidth = 0;

    if (d->m_marksVisible) {
        if (TextEditorSettings::fontSettings().relativeLineSpacing() == 100)
            markWidth += fm.lineSpacing() + 2;
        else
            markWidth += TextEditorSettings::fontSettings().lineSpacing() + 2;

//     if (documentLayout->doubleMarkCount)
//         markWidth += fm.lineSpacing() / 3;
        space += markWidth;
    } else {
        space += 2;
    }

    if (markWidthPtr)
        *markWidthPtr = markWidth;

    space += 4;

    if (d->m_codeFoldingVisible) {
        if (TextEditorSettings::fontSettings().relativeLineSpacing() == 100)
            space += foldBoxWidth(fm);
        else
            space += foldBoxWidth();
    }

    if (viewportMargins() != QMargins{isLeftToRight() ? space : 0, 0, isLeftToRight() ? 0 : space, 0})
        d->slotUpdateExtraAreaWidth(space);

    return space;
}

void TextEditorWidgetPrivate::slotUpdateExtraAreaWidth(std::optional<int> width)
{
    if (!width.has_value())
        width = q->extraAreaWidth();
    QMargins margins;
    if (q->isLeftToRight())
        margins = QMargins(*width, 0, 0, 0);
    else
        margins = QMargins(0, 0, *width, 0);
    if (margins != q->viewportMargins())
        q->setViewportMargins(margins);
}

void TextEditorWidgetPrivate::slotUpdateBlockCount()
{
    slotUpdateExtraAreaWidth();
    m_foldedBlockCache.clear();
    QTextBlock block = q->document()->firstBlock();
    while (block.isValid()) {
        if (TextBlockUserData::isFolded(block))
            m_foldedBlockCache += block.blockNumber();
        block = block.next();
    }
}

struct Internal::ExtraAreaPaintEventData
{
    ExtraAreaPaintEventData(const TextEditorWidget *editor, TextEditorWidgetPrivate *d)
        : doc(editor->document())
        , documentLayout(qobject_cast<TextDocumentLayout*>(doc->documentLayout()))
        , selectionStart(editor->textCursor().selectionStart())
        , selectionEnd(editor->textCursor().selectionEnd())
        , fontMetrics(d->m_extraArea->font())
        , lineSpacing(fontMetrics.lineSpacing())
        , markWidth(d->m_marksVisible ? lineSpacing : 0)
        , collapseColumnWidth(d->m_codeFoldingVisible ? foldBoxWidth(fontMetrics) : 0)
        , extraAreaWidth(d->m_extraArea->width() - collapseColumnWidth)
        , currentLineNumberFormat(
              editor->textDocument()->fontSettings().toTextCharFormat(C_CURRENT_LINE_NUMBER))
        , palette(d->m_extraArea->palette())
    {
        if (TextEditorSettings::fontSettings().relativeLineSpacing() != 100) {
            lineSpacing = TextEditorSettings::fontSettings().lineSpacing();
            collapseColumnWidth = d->m_codeFoldingVisible ? foldBoxWidth() : 0;
            markWidth = d->m_marksVisible ? lineSpacing : 0;
        }
        palette.setCurrentColorGroup(QPalette::Active);
    }
    QTextBlock block;
    const QTextDocument *doc;
    const TextDocumentLayout *documentLayout;
    const int selectionStart;
    const int selectionEnd;
    const QFontMetrics fontMetrics;
    int lineSpacing;
    int markWidth;
    int collapseColumnWidth;
    const int extraAreaWidth;
    const QTextCharFormat currentLineNumberFormat;
    QPalette palette;
};

void TextEditorWidgetPrivate::paintLineNumbers(QPainter &painter,
                                               const ExtraAreaPaintEventData &data,
                                               const QRectF &blockBoundingRect) const
{
    if (!m_lineNumbersVisible)
        return;

    const QString &number = q->lineNumber(data.block.blockNumber());
    const bool selected = (
                (data.selectionStart < data.block.position() + data.block.length()
                 && data.selectionEnd > data.block.position())
                || (data.selectionStart == data.selectionEnd && data.selectionEnd == data.block.position())
                );
    if (selected) {
        painter.save();
        QFont f = painter.font();
        f.setBold(data.currentLineNumberFormat.font().bold());
        f.setItalic(data.currentLineNumberFormat.font().italic());
        painter.setFont(f);
        painter.setPen(data.currentLineNumberFormat.foreground().color());
        if (data.currentLineNumberFormat.background() != Qt::NoBrush) {
            painter.fillRect(QRectF(0, blockBoundingRect.top(),
                                   data.extraAreaWidth, blockBoundingRect.height()),
                             data.currentLineNumberFormat.background().color());
        }
    }
    painter.drawText(QRectF(data.markWidth, blockBoundingRect.top(),
                            data.extraAreaWidth - data.markWidth - 4, blockBoundingRect.height()),
                     Qt::AlignRight,
                     number);
    if (selected)
        painter.restore();
}

void TextEditorWidgetPrivate::paintTextMarks(QPainter &painter, const ExtraAreaPaintEventData &data,
                                             const QRectF &blockBoundingRect) const
{
    auto userData = static_cast<TextBlockUserData*>(data.block.userData());
    if (!userData || !m_marksVisible)
        return;
    TextMarks marks = userData->marks();
    QList<QIcon> icons;
    auto end = marks.crend();
    int marksWithIconCount = 0;
    QIcon overrideIcon;
    for (auto it = marks.crbegin(); it != end; ++it) {
        if ((*it)->isVisible()) {
            const QIcon icon = (*it)->icon();
            if (!icon.isNull()) {
                if ((*it)->isLocationMarker()) {
                    overrideIcon = icon;
                } else {
                    if (icons.size() < 3
                            && !Utils::contains(icons, Utils::equal(&QIcon::cacheKey, icon.cacheKey()))) {
                        icons << icon;
                    }
                    ++marksWithIconCount;
                }
            }
        }
    }


    int size = data.lineSpacing - 1;
    int xoffset = 0;
    int yoffset = blockBoundingRect.top();

    painter.save();
    const QScopeGuard cleanup([&painter, size, yoffset, xoffset, overrideIcon] {
        if (!overrideIcon.isNull()) {
            const QRect r(xoffset, yoffset, size, size);
            overrideIcon.paint(&painter, r, Qt::AlignCenter);
        }
        painter.restore();
    });

    if (icons.isEmpty())
        return;

    if (icons.size() == 1) {
        const QRect r(xoffset, yoffset, size, size);
        icons.first().paint(&painter, r, Qt::AlignCenter);
        return;
    }
    size = size / 2;
    for (const QIcon &icon : std::as_const(icons)) {
        const QRect r(xoffset, yoffset, size, size);
        icon.paint(&painter, r, Qt::AlignCenter);
        if (xoffset != 0) {
            yoffset += size;
            xoffset = 0;
        } else {
            xoffset = size;
        }
    }
    QFont font = painter.font();
    font.setPixelSize(size);
    painter.setFont(font);

    const QColor color = data.currentLineNumberFormat.foreground().color();
    if (color.isValid())
        painter.setPen(color);

    const QRect r(size, blockBoundingRect.top() + size, size, size);
    const QString detail = marksWithIconCount > 9 ? QString("+")
                                                  : QString::number(marksWithIconCount);
    painter.drawText(r, Qt::AlignRight, detail);
}

static void drawRectBox(QPainter *painter, const QRect &rect, const QPalette &pal)
{
    painter->save();
    painter->setOpacity(0.5);
    painter->fillRect(rect, pal.brush(QPalette::Highlight));
    painter->restore();
}

void TextEditorWidgetPrivate::paintCodeFolding(QPainter &painter,
                                               const ExtraAreaPaintEventData &data,
                                               const QRectF &blockBoundingRect) const
{
    if (!m_codeFoldingVisible)
        return;

    int extraAreaHighlightFoldBlockNumber = -1;
    int extraAreaHighlightFoldEndBlockNumber = -1;
    if (!m_highlightBlocksInfo.isEmpty()) {
        extraAreaHighlightFoldBlockNumber = m_highlightBlocksInfo.open.last();
        extraAreaHighlightFoldEndBlockNumber = m_highlightBlocksInfo.close.first();
    }

    const QTextBlock &nextBlock = data.block.next();

    bool drawBox = TextBlockUserData::foldingIndent(data.block)
                   < TextBlockUserData::foldingIndent(nextBlock);
    if (drawBox) {
        qCDebug(foldingLog) << "need to paint folding marker";
        qCDebug(foldingLog) << "folding indent for line" << (data.block.blockNumber() + 1) << "is"
                            << TextBlockUserData::foldingIndent(data.block);
        qCDebug(foldingLog) << "folding indent for line" << (nextBlock.blockNumber() + 1) << "is"
                            << TextBlockUserData::foldingIndent(nextBlock);
    }

    const int blockNumber = data.block.blockNumber();
    bool active = blockNumber == extraAreaHighlightFoldBlockNumber;
    bool hovered = blockNumber >= extraAreaHighlightFoldBlockNumber
            && blockNumber <= extraAreaHighlightFoldEndBlockNumber;

    int boxWidth = 0;
    if (TextEditorSettings::fontSettings().relativeLineSpacing() == 100)
        boxWidth = foldBoxWidth(data.fontMetrics);
    else
        boxWidth = foldBoxWidth();

    if (hovered) {
        int itop = qRound(blockBoundingRect.top());
        int ibottom = qRound(blockBoundingRect.bottom());
        QRect box = QRect(data.extraAreaWidth + 1, itop, boxWidth - 2, ibottom - itop);
        drawRectBox(&painter, box, data.palette);
    }

    if (drawBox) {
        bool expanded = nextBlock.isVisible();
        int size = boxWidth/4;
        QRect box(data.extraAreaWidth + size, int(blockBoundingRect.top()) + size,
                  2 * (size) + 1, 2 * (size) + 1);
        drawFoldingMarker(&painter, data.palette, box, expanded, active, hovered);
    }

}

void TextEditorWidgetPrivate::paintRevisionMarker(QPainter &painter,
                                                  const ExtraAreaPaintEventData &data,
                                                  const QRectF &blockBoundingRect) const
{
    if (m_revisionsVisible && data.block.revision() != data.documentLayout->lastSaveRevision) {
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, false);
        if (data.block.revision() < 0)
            painter.setPen(QPen(Qt::darkGreen, 2));
        else
            painter.setPen(QPen(Qt::red, 2));
        painter.drawLine(data.extraAreaWidth - 1, int(blockBoundingRect.top()),
                         data.extraAreaWidth - 1, int(blockBoundingRect.bottom()) - 1);
        painter.restore();
    }
}

void TextEditorWidget::extraAreaPaintEvent(QPaintEvent *e)
{
    ExtraAreaPaintEventData data(this, d.get());
    QTC_ASSERT(data.documentLayout, return);

    QPainter painter(d->m_extraArea);

    painter.fillRect(e->rect(), data.palette.color(QPalette::Window));

    data.block = firstVisibleBlock();
    QPointF offset = contentOffset();
    QRectF boundingRect = blockBoundingRect(data.block).translated(offset);

    while (data.block.isValid() && boundingRect.top() <= e->rect().bottom()) {
        if (boundingRect.bottom() >= e->rect().top()) {

            painter.setPen(data.palette.color(QPalette::Dark));

            d->paintLineNumbers(painter, data, boundingRect);

            if (d->m_codeFoldingVisible || d->m_marksVisible) {
                painter.save();
                painter.setRenderHint(QPainter::Antialiasing, false);

                d->paintTextMarks(painter, data, boundingRect);
                d->paintCodeFolding(painter, data, boundingRect);

                painter.restore();
            }

            d->paintRevisionMarker(painter, data, boundingRect);
        }

        offset.ry() += boundingRect.height();
        data.block = d->nextVisibleBlock(data.block);
        boundingRect = blockBoundingRect(data.block).translated(offset);
    }
}

void TextEditorWidgetPrivate::drawFoldingMarker(QPainter *painter, const QPalette &pal,
                                       const QRect &rect,
                                       bool expanded,
                                       bool active,
                                       bool hovered) const
{
    QStyle *s = q->style();
    if (auto ms = qobject_cast<ManhattanStyle*>(s))
        s = ms->baseStyle();

    QStyleOptionViewItem opt;
    opt.rect = rect;
    opt.state = QStyle::State_Active | QStyle::State_Item | QStyle::State_Children;
    if (expanded)
        opt.state |= QStyle::State_Open;
    if (active)
        opt.state |= QStyle::State_MouseOver | QStyle::State_Enabled | QStyle::State_Selected;
    if (hovered)
        opt.palette.setBrush(QPalette::Window, pal.highlight());

    const char *className = s->metaObject()->className();

    // Do not use the windows folding marker since we cannot style them and the default hover color
    // is a blue which does not guarantee an high contrast on all themes.
    static QPointer<QStyle> fusionStyleOverwrite = nullptr;
    if (!qstrcmp(className, "QWindowsVistaStyle")) {
        if (fusionStyleOverwrite.isNull())
            fusionStyleOverwrite = QStyleFactory::create("fusion");
        if (!fusionStyleOverwrite.isNull()) {
            s = fusionStyleOverwrite.data();
            className = s->metaObject()->className();
        }
    }

    if (!qstrcmp(className, "OxygenStyle")) {
        const QStyle::PrimitiveElement direction = expanded ? QStyle::PE_IndicatorArrowDown
                                                            : QStyle::PE_IndicatorArrowRight;
        StyleHelper::drawArrow(direction, painter, &opt);
    } else {
         // QGtkStyle needs a small correction to draw the marker in the right place
        if (!qstrcmp(className, "QGtkStyle"))
           opt.rect.translate(-2, 0);
        else if (!qstrcmp(className, "QMacStyle"))
            opt.rect.translate(-2, 0);
        else if (!qstrcmp(className, "QFusionStyle"))
            opt.rect.translate(0, -1);

        s->drawPrimitive(QStyle::PE_IndicatorBranch, &opt, painter, q);
    }
}

void TextEditorWidgetPrivate::slotUpdateRequest(const QRect &r, int /*dy*/)
{
    if (r.width() > 4) { // wider than cursor width, not just cursor blinking
        m_extraArea->update(0, r.y(), m_extraArea->width(), r.height());
        if (!m_searchExpr.pattern().isEmpty()) {
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
    emit q->saveCurrentStateForNavigationHistory();
}

void TextEditorWidgetPrivate::updateCurrentLineHighlight()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (m_highlightCurrentLine) {
        for (const QTextCursor &c : m_cursors) {
            QTextEdit::ExtraSelection sel;
            sel.format.setBackground(
                        m_document->fontSettings().toTextCharFormat(C_CURRENT_LINE).background());
            sel.format.setProperty(QTextFormat::FullWidthSelection, true);
            sel.cursor = c;
            sel.cursor.clearSelection();
            extraSelections.append(sel);
        }
    }
    updateCurrentLineInScrollbar();

    q->setExtraSelections(TextEditorWidget::CurrentLineSelection, extraSelections);

    // the extra area shows information for the entire current block, not just the currentline.
    // This is why we must force a bigger update region.
    const QPointF offset = q->contentOffset();
    auto updateBlock = [&](const QTextBlock &block) {
        if (block.isValid() && block.isVisible()) {
            QRect updateRect = q->blockBoundingGeometry(block).translated(offset).toAlignedRect();
            m_extraArea->update(updateRect);
            updateRect.setLeft(0);
            updateRect.setRight(q->viewport()->width());
            q->viewport()->update(updateRect);
        }
    };

    QSet<int> cursorBlockNumbers;
    for (const QTextCursor &c : m_cursors)
        cursorBlockNumbers.insert(c.blockNumber());

    const QSet<int> updateBlockNumbers = (cursorBlockNumbers - m_cursorBlockNumbers)
                                         + (m_cursorBlockNumbers - cursorBlockNumbers);

    for (const int blockNumber : updateBlockNumbers)
        updateBlock(m_document->document()->findBlockByNumber(blockNumber));

    m_cursorBlockNumbers = cursorBlockNumbers;
}

void TextEditorWidget::slotCursorPositionChanged()
{
#if 0
    qDebug() << "block" << textCursor().blockNumber()+1
            << "brace depth:" << BaseTextDocumentLayout::braceDepth(textCursor().block())
            << "indent:" << BaseTextDocumentLayout::userData(textCursor().block())->foldingIndent();
#endif
    if (!d->m_contentsChanged && d->m_lastCursorChangeWasInteresting) {
        emit addSavedStateToNavigationHistory();
        d->m_lastCursorChangeWasInteresting = false;
    } else if (d->m_contentsChanged) {
        d->saveCurrentCursorPositionForNavigation();
        if (EditorManager::currentEditor() && EditorManager::currentEditor()->widget() == this)
            EditorManager::setLastEditLocation(EditorManager::currentEditor());
    }
    MultiTextCursor cursor = multiTextCursor();
    cursor.replaceMainCursor(textCursor());
    setMultiTextCursor(cursor);
    d->updateCursorSelections();
    d->updateHighlights();
    d->updateSuggestion();
}

void TextEditorWidgetPrivate::updateHighlights()
{
    if (m_parenthesesMatchingEnabled && q->hasFocus()) {
        // Delay update when no matching is displayed yet, to avoid flicker
        if (q->extraSelections(TextEditorWidget::ParenthesesMatchingSelection).isEmpty()
            && m_bracketsAnimator == nullptr) {
            m_parenthesesMatchingTimer.start();
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

    if (m_highlightAutoComplete && !m_autoCompleteHighlightPos.isNull()) {
        QMetaObject::invokeMethod(this, [this] {
            const QTextCursor &cursor = q->textCursor();
            auto popAutoCompletion = [&]() {
                return !m_autoCompleteHighlightPos.isNull()
                        && m_autoCompleteHighlightPos != cursor;
            };
            if ((!m_keepAutoCompletionHighlight && !q->hasFocus()) || popAutoCompletion()) {
                while (popAutoCompletion())
                    m_autoCompleteHighlightPos = QTextCursor();
                updateAutoCompleteHighlight();
            }
        }, Qt::QueuedConnection);
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
    if (m_highlightCurrentLine && m_highlightScrollBarController) {
        m_highlightScrollBarController->removeHighlights(Constants::SCROLL_BAR_CURRENT_LINE);
        for (const QTextCursor &tc : m_cursors) {
            if (QTextLayout *layout = q->editorLayout()->blockLayout(tc.block())) {
                const int pos = q->editorLayout()->firstLineNumberOf(tc.block()) +
                        layout->lineForTextPosition(tc.positionInBlock()).lineNumber();
                m_highlightScrollBarController->addHighlight({Constants::SCROLL_BAR_CURRENT_LINE, pos,
                                                              Theme::TextEditor_CurrentLine_ScrollBarColor,
                                                              Highlight::HighestPriority});
            }
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

        for (const QTextCursor &scope : m_findScope) {
            QSet<int> updatedBlocks;
            const bool blockContainsFindScope = block.position() < scope.selectionEnd()
                                                && block.position() + block.length()
                                                       >= scope.selectionStart();
            if (blockContainsFindScope) {
                QTextBlock b = block.document()->findBlock(scope.selectionStart());
                do {
                    if (Utils::insert(updatedBlocks, b.blockNumber()))
                        emit q->requestBlockUpdate(b);
                    b = b.next();
                } while (b.isValid() && b.position() < scope.selectionEnd());
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
        viewport()->update(d->cursorUpdateRect(d->m_cursors));
    }
    PlainTextEdit::timerEvent(e);
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

    bool onLink = false;
    if (d->m_linkPressed && d->m_currentLink.hasValidTarget()) {
        const int eventCursorPosition = cursorForPosition(e->pos()).position();
        if (eventCursorPosition < d->m_currentLink.linkTextStart
            || eventCursorPosition > d->m_currentLink.linkTextEnd) {
            d->m_linkPressed = false;
        } else {
            onLink = true;
        }
    }

    static std::optional<MultiTextCursor> startMouseMoveCursor;
    if (e->buttons() == Qt::LeftButton && e->modifiers() & Qt::AltModifier) {
        if (!startMouseMoveCursor.has_value()) {
            startMouseMoveCursor = multiTextCursor();
            QTextCursor c = startMouseMoveCursor->takeMainCursor();
            if (!startMouseMoveCursor->hasMultipleCursors()
                && !startMouseMoveCursor->hasSelection()) {
                startMouseMoveCursor.emplace(MultiTextCursor());
            }
            c.setPosition(c.anchor());
            startMouseMoveCursor->addCursor(c);
        }
        MultiTextCursor cursor = *startMouseMoveCursor;
        const QTextCursor anchorCursor = cursor.takeMainCursor();
        const QTextCursor eventCursor = cursorForPosition(e->pos());

        const TabSettings tabSettings = d->m_document->tabSettings();
        int eventColumn = tabSettings.columnAt(eventCursor.block().text(),
                                               eventCursor.positionInBlock());
        if (eventCursor.positionInBlock() == eventCursor.block().length() - 1) {
            eventColumn += int((e->pos().x() - cursorRect(eventCursor).center().x())
                               / d->charWidth());
        }

        int anchorColumn = tabSettings.columnAt(anchorCursor.block().text(),
                                                anchorCursor.positionInBlock());
        const TextEditorWidgetPrivate::BlockSelection blockSelection = {eventCursor.blockNumber(),
                                                                        eventColumn,
                                                                        anchorCursor.blockNumber(),
                                                                        anchorColumn};

        cursor.addCursors(d->generateCursorsForBlockSelection(blockSelection));
        if (!cursor.isNull())
            setMultiTextCursor(cursor);
    } else {
        if (startMouseMoveCursor.has_value())
            startMouseMoveCursor.reset();
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
            if ((collapsedBlock.isValid() || refactorMarker.isValid())
                && !d->m_mouseOnFoldedMarker) {
                d->m_mouseOnFoldedMarker = true;
                viewport()->setCursor(Qt::PointingHandCursor);
            } else if (!collapsedBlock.isValid() && !refactorMarker.isValid()
                       && d->m_mouseOnFoldedMarker) {
                d->m_mouseOnFoldedMarker = false;
                viewport()->setCursor(Qt::IBeamCursor);
            }
        } else if (!onLink || e->buttons() != Qt::LeftButton
                   || e->modifiers() != Qt::ControlModifier) {
            PlainTextEdit::mouseMoveEvent(e);
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
    ICore::restartTrimmer();

    if (e->button() == Qt::LeftButton) {
        MultiTextCursor multiCursor = multiTextCursor();
        const QTextCursor &cursor = cursorForPosition(e->pos());
        if (e->modifiers() & Qt::AltModifier && !(e->modifiers() & Qt::ControlModifier)) {
            if (e->modifiers() & Qt::ShiftModifier) {
                const QTextCursor anchor = multiCursor.takeMainCursor();

                const TabSettings tabSettings = d->m_document->tabSettings();
                int eventColumn
                    = tabSettings.columnAt(cursor.block().text(), cursor.positionInBlock());
                if (cursor.positionInBlock() == cursor.block().length() - 1) {
                    eventColumn += int(
                        (e->pos().x() - cursorRect(cursor).center().x()) / d->charWidth());
                }

                const int anchorColumn
                    = tabSettings.columnAt(anchor.block().text(), anchor.positionInBlock());
                const TextEditorWidgetPrivate::BlockSelection blockSelection
                    = {cursor.blockNumber(), eventColumn, anchor.blockNumber(), anchorColumn};

                multiCursor.addCursors(d->generateCursorsForBlockSelection(blockSelection));
                setMultiTextCursor(multiCursor);
            } else {
                multiCursor.addCursor(cursor);
            }
            setMultiTextCursor(multiCursor);
            return;
        } else {
            if (multiCursor.hasMultipleCursors())
                setMultiTextCursor(MultiTextCursor({cursor}));

            QTextBlock foldedBlock = d->foldedBlockAt(e->pos());
            if (foldedBlock.isValid()) {
                d->toggleBlockVisible(foldedBlock);
                viewport()->setCursor(Qt::IBeamCursor);
            }

            RefactorMarker refactorMarker = d->m_refactorOverlay->markerAt(e->pos());
            if (refactorMarker.isValid()) {
                if (refactorMarker.callback) {
                    refactorMarker.callback(this);
                    return;
                }
            } else {
                d->m_linkPressed = d->isMouseNavigationEvent(e);
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

    PlainTextEdit::mousePressEvent(e);
}

void TextEditorWidget::mouseReleaseEvent(QMouseEvent *e)
{
    const Qt::MouseButton button = e->button();
    if (d->m_linkPressed && d->isMouseNavigationEvent(e) && button == Qt::LeftButton) {
        bool inNextSplit = ((e->modifiers() & Qt::AltModifier) && !alwaysOpenLinksInNextSplit())
                || (alwaysOpenLinksInNextSplit() && !(e->modifiers() & Qt::AltModifier));

        findLinkAt(textCursor(),
                   [inNextSplit, self = QPointer<TextEditorWidget>(this)](const Link &symbolLink) {
            if (self && self->openLink(symbolLink, inNextSplit))
                self->d->clearLink();
        }, true, inNextSplit);
    } else if (button == Qt::MiddleButton && !isReadOnly()
               && QGuiApplication::clipboard()->supportsSelection()) {
        if (!(e->modifiers() & Qt::AltModifier))
            doSetTextCursor(cursorForPosition(e->pos()));
        if (const QMimeData *md = QGuiApplication::clipboard()->mimeData(QClipboard::Selection))
            insertFromMimeData(md);
        e->accept();
        return;
    }

    if (!HostOsInfo::isLinuxHost() && handleForwardBackwardMouseButtons(e))
        return;

    // If the refactor marker was pressed then don't propagate release event to editor
    RefactorMarker refactorMarker = d->m_refactorOverlay->markerAt(e->pos());
    if (refactorMarker.isValid()) {
        if (refactorMarker.callback) {
            e->accept();
            return;
        }
    }

    PlainTextEdit::mouseReleaseEvent(e);

    d->setClipboardSelection();
    const QTextCursor plainTextEditCursor = textCursor();
    const QTextCursor multiMainCursor = multiTextCursor().mainCursor();
    if (multiMainCursor.position() != plainTextEditCursor.position()
        || multiMainCursor.anchor() != plainTextEditCursor.anchor()) {
        doSetTextCursor(plainTextEditCursor, true);
    }
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

    QTextCursor eventCursor = cursorForPosition(QPoint(e->pos().x(), e->pos().y()));
    const int eventDocumentPosition = eventCursor.position();

    PlainTextEdit::mouseDoubleClickEvent(e);

    // PlainTextEdit::mouseDoubleClickEvent just selects the word under the text cursor. If the
    // event is triggered on a position that is inbetween two whitespaces this event selects the
    // previous word or nothing if the whitespaces are at the block start. Replace this behavior
    // with selecting the whitespaces starting from the previous word end to the next word.
    const QChar character = characterAt(eventDocumentPosition);
    const QChar prevCharacter = characterAt(eventDocumentPosition - 1);

    if (character.isSpace() && prevCharacter.isSpace()) {
        if (prevCharacter != QChar::ParagraphSeparator) {
            eventCursor.movePosition(QTextCursor::PreviousWord);
            eventCursor.movePosition(QTextCursor::EndOfWord);
        } else if (character == QChar::ParagraphSeparator) {
            return; // no special handling for empty lines
        }
        eventCursor.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor);
        MultiTextCursor cursor = multiTextCursor();
        cursor.replaceMainCursor(eventCursor);
        setMultiTextCursor(cursor);
    }
}

void TextEditorWidgetPrivate::setClipboardSelection()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (m_cursors.hasSelection() && clipboard->supportsSelection())
        clipboard->setMimeData(q->createMimeDataFromSelection(), QClipboard::Selection);
}

void TextEditorWidget::leaveEvent(QEvent *e)
{
    // Clear link emulation when the mouse leaves the editor
    d->clearLink();
    PlainTextEdit::leaveEvent(e);
}

void TextEditorWidget::keyReleaseEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Control) {
        d->clearLink();
    } else if (e->key() == Qt::Key_Shift && d->m_behaviorSettings.m_constrainHoverTooltips
               && ToolTip::isVisible()) {
        ToolTip::hide();
    } else if (e->key() == Qt::Key_Alt && d->m_maybeFakeTooltipEvent) {
        d->m_maybeFakeTooltipEvent = false;
        d->processTooltipRequest(textCursor());
    }

    PlainTextEdit::keyReleaseEvent(e);
}

void TextEditorWidget::dragEnterEvent(QDragEnterEvent *e)
{
    // If the drag event contains URLs, we don't want to insert them as text
    if (e->mimeData()->hasUrls()) {
        e->ignore();
        return;
    }

    PlainTextEdit::dragEnterEvent(e);
}

static void appendMenuActionsFromContext(QMenu *menu, Id menuContextId)
{
    ActionContainer *mcontext = ActionManager::actionContainer(menuContextId);
    QMenu *contextMenu = mcontext->menu();

    const QList<QAction *> actions = contextMenu->actions();
    for (QAction *action : actions)
        menu->addAction(action);
}

void TextEditorWidget::showDefaultContextMenu(QContextMenuEvent *e, Id menuContextId)
{
    QMenu menu;
    if (menuContextId.isValid())
        appendMenuActionsFromContext(&menu, menuContextId);
    appendStandardContextMenuActions(&menu);
    menu.exec(e->globalPos());
}

void TextEditorWidget::addHoverHandler(BaseHoverHandler *handler)
{
    if (!d->m_hoverHandlers.contains(handler))
        d->m_hoverHandlers.append(handler);
}

void TextEditorWidget::removeHoverHandler(BaseHoverHandler *handler)
{
    if (d->m_hoverHandlers.removeAll(handler) > 0)
        d->m_hoverHandlerRunner.handlerRemoved(handler);
}

void TextEditorWidget::insertSuggestion(std::unique_ptr<TextSuggestion> &&suggestion)
{
    d->insertSuggestion(std::move(suggestion));
}

void TextEditorWidget::clearSuggestion()
{
    d->clearCurrentSuggestion();
}

TextSuggestion *TextEditorWidget::currentSuggestion() const
{
    if (d->m_suggestionBlock.isValid())
        return TextBlockUserData::suggestion(d->m_suggestionBlock);
    return nullptr;
}

bool TextEditorWidget::suggestionVisible() const
{
    return currentSuggestion();
}

bool TextEditorWidget::suggestionsBlocked() const
{
    return d->m_suggestionBlocker.use_count() > 1;
}

TextEditorWidget::SuggestionBlocker TextEditorWidget::blockSuggestions()
{
    if (!suggestionsBlocked())
        clearSuggestion();
    return d->m_suggestionBlocker;
}

std::unique_ptr<EmbeddedWidgetInterface> TextEditorWidget::insertWidget(QWidget *widget, int pos)
{
    return d->insertWidget(widget, pos);
}

QTextCursor TextEditorWidget::autoCompleteHighlightPosition() const
{
    return d->m_autoCompleteHighlightPos;
}

#ifdef WITH_TESTS
void TextEditorWidget::processTooltipRequest(const QTextCursor &c)
{
    d->processTooltipRequest(c);
}
#endif

void TextEditorWidget::extraAreaLeaveEvent(QEvent *)
{
    d->extraAreaPreviousMarkTooltipRequestedLine = -1;
    ToolTip::hide();

    // fake missing mouse move event from Qt
    QMouseEvent me(QEvent::MouseMove, QPoint(-1, -1), QCursor::pos(), Qt::NoButton, {}, {});
    extraAreaMouseEvent(&me);
}

static bool xIsInsideFoldingRegion(int x, int extraAreaWidth, const QFontMetrics &fm)
{
    int boxWidth = 0;
    if (TextEditorSettings::fontSettings().relativeLineSpacing() == 100)
        boxWidth = foldBoxWidth(fm);
    else
        boxWidth = foldBoxWidth();

    return x > extraAreaWidth - boxWidth && x <= extraAreaWidth;
}

void TextEditorWidget::extraAreaContextMenuEvent(QContextMenuEvent *e)
{
    if (d->m_codeFoldingVisible
        && xIsInsideFoldingRegion(e->pos().x(), extraArea()->width(), fontMetrics())) {
        const QTextCursor cursor = cursorForPosition(QPoint(0, e->pos().y()));
        const QTextBlock block = cursor.block();
        auto menu = new QMenu(this);

        menu->addAction(Tr::tr("Fold"), this, [&] { fold(block); });
        menu->addAction(Tr::tr("Fold Recursively"), this, [&] { fold(block, true); });
        menu->addAction(Tr::tr("Fold All"), this, [this] { unfoldAll(/* unfold  = */ false); });
        menu->addAction(Tr::tr("Unfold"), this, [&] { unfold(block); });
        menu->addAction(Tr::tr("Unfold Recursively"), this, [&] { unfold(block, true); });
        menu->addAction(Tr::tr("Unfold All"), this, [this] { unfoldAll(/* fold  = */ true); });
        menu->exec(e->globalPos());

        delete menu;
        e->accept();
        return;
    }

    if (d->m_marksVisible) {
        QTextCursor cursor = cursorForPosition(QPoint(0, e->pos().y()));
        auto contextMenu = new QMenu(this);
        bookmarkManager().requestContextMenu(textDocument()->filePath(),
                                             cursor.blockNumber() + 1,
                                             contextMenu);
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

    // Update which folder marker is highlighted
    QTextCursor cursor;
    if (xIsInsideFoldingRegion(pos.x(), extraArea()->width(), fontMetrics()))
        cursor = cursorForPosition(QPoint(0, pos.y()));
    else if (d->m_displaySettings.m_highlightBlocks)
        cursor = textCursor();

    updateFoldingHighlight(cursor);
}

void TextEditorWidget::updateFoldingHighlight(const QTextCursor &cursor)
{
    const int highlightBlockNumber = d->extraAreaHighlightFoldedBlockNumber;
    const bool curserIsNull = !cursor.isNull();
    if (curserIsNull)
        d->extraAreaHighlightFoldedBlockNumber = cursor.blockNumber();
    else
        d->extraAreaHighlightFoldedBlockNumber = -1;

    if (curserIsNull || (highlightBlockNumber != d->extraAreaHighlightFoldedBlockNumber))
        d->m_highlightBlocksTimer.start(d->m_highlightBlocksInfo.isEmpty() ? 120 : 0);
}

void TextEditorWidget::extraAreaToolTipEvent(QHelpEvent *e)
{
    QTextCursor cursor = cursorForPosition(QPoint(0, e->pos().y()));

    int markWidth = 0;
    extraAreaWidth(&markWidth);
    const bool inMarkArea = e->pos().x() <= markWidth && e->pos().x() >= 0;
    if (!inMarkArea)
        return;
    int line = cursor.blockNumber() + 1;
    if (d->extraAreaPreviousMarkTooltipRequestedLine != line) {
        if (auto data = static_cast<TextBlockUserData *>(cursor.block().userData())) {
            if (!data->marks().isEmpty())
                d->showTextMarksToolTip(mapToGlobal(e->pos()), data->marks());
        }
    }
    d->extraAreaPreviousMarkTooltipRequestedLine = line;
}

void TextEditorWidget::extraAreaMouseEvent(QMouseEvent *e)
{
    QTextCursor cursor = cursorForPosition(QPoint(0, e->pos().y()));

    int markWidth = 0;
    extraAreaWidth(&markWidth);
    const bool inMarkArea = e->pos().x() <= markWidth && e->pos().x() >= 0;

    if (d->m_codeFoldingVisible
            && e->type() == QEvent::MouseMove && e->buttons() == 0) { // mouse tracking
        updateFoldingHighlight(e->pos());
    }

    // Set whether the mouse cursor is a hand or normal arrow
    if (e->type() == QEvent::MouseMove) {
        if (inMarkArea) {
            // tool tips are shown in extraAreaToolTipEvent
            int line = cursor.blockNumber() + 1;
            if (d->extraAreaPreviousMarkTooltipRequestedLine != line) {
                if (auto data = static_cast<TextBlockUserData *>(cursor.block().userData())) {
                    if (data->marks().isEmpty())
                        ToolTip::hide();
                }
            }
        }

        if (!d->m_markDragging && e->buttons() & Qt::LeftButton && !d->m_markDragStart.isNull()) {
            int dist = (e->pos() - d->m_markDragStart).manhattanLength();
            if (dist > QApplication::startDragDistance()) {
                d->m_markDragging = true;
                const int height = fontMetrics().lineSpacing() - 1;
                d->m_markDragCursor = QCursor(d->m_dragMark->icon().pixmap({height, height}));
                d->m_dragMark->setVisible(false);
                QGuiApplication::setOverrideCursor(d->m_markDragCursor);
            }
        }

        if (d->m_markDragging) {
            QGuiApplication::changeOverrideCursor(inMarkArea ? d->m_markDragCursor
                                                             : QCursor(Qt::ForbiddenCursor));
        } else if (inMarkArea != (d->m_extraArea->cursor().shape() == Qt::PointingHandCursor)) {
            d->m_extraArea->setCursor(inMarkArea ? Qt::PointingHandCursor : Qt::ArrowCursor);
        }
    }

    if (e->type() == QEvent::MouseButtonPress || e->type() == QEvent::MouseButtonDblClick) {
        if (e->button() == Qt::LeftButton) {
            int boxWidth = 0;
            if (TextEditorSettings::fontSettings().relativeLineSpacing() == 100)
                boxWidth = foldBoxWidth(fontMetrics());
            else
                boxWidth = foldBoxWidth();
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
                if (auto data = static_cast<TextBlockUserData *>(block.userData())) {
                    TextMarks marks = data->marks();
                    for (int i = marks.size(); --i >= 0; ) {
                        TextMark *mark = marks.at(i);
                        if (mark->isDraggable()) {
                            d->m_markDragStart = e->pos();
                            d->m_dragMark = mark;
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
            TextMark *dragMark = d->m_dragMark;
            d->m_dragMark = nullptr;
            d->m_markDragging = false;
            d->m_markDragStart = QPoint();
            if (dragMark)
                dragMark->setVisible(true);
            QGuiApplication::restoreOverrideCursor();
            if (wasDragging && dragMark) {
                dragMark->dragToLine(cursor.blockNumber() + 1);
                return;
            } else if (sameLine) {
                QTextBlock block = cursor.document()->findBlockByNumber(n);
                if (auto data = static_cast<TextBlockUserData *>(block.userData())) {
                    TextMarks marks = data->marks();
                    for (int i = marks.size(); --i >= 0; ) {
                        TextMark *mark = marks.at(i);
                        if (mark->isClickable()) {
                            mark->clicked();
                            return;
                        }
                    }
                }
            }
            int line = n + 1;
            if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
                if (!textDocument()->isTemporary())
                    bookmarkManager().toggleBookmark(textDocument()->filePath(), line);
            } else {
                emit markRequested(this, line, BreakpointRequest);
            }
        }
    }
}

void TextEditorWidget::ensureCursorVisible()
{
    ensureBlockIsUnfolded(textCursor().block());
    PlainTextEdit::ensureCursorVisible();
}

void TextEditorWidget::ensureBlockIsUnfolded(QTextBlock block)
{
    if (singleShotAfterHighlightingDone([this, block] { ensureBlockIsUnfolded(block); }))
        return;

    if (!block.isVisible()) {
        auto documentLayout = qobject_cast<TextDocumentLayout*>(document()->documentLayout());
        QTC_ASSERT(documentLayout, return);

        // Open all parent folds of current line.
        int indent = TextBlockUserData::foldingIndent(block);
        block = block.previous();
        while (block.isValid()) {
            const int indent2 = TextBlockUserData::foldingIndent(block);
            if (TextBlockUserData::canFold(block) && indent2 < indent) {
                TextBlockUserData::doFoldOrUnfold(block, /* unfold = */ true);
                if (block.isVisible())
                    break;
                indent = indent2;
            }
            block = block.previous();
        }

        documentLayout->requestUpdate();
        documentLayout->emitDocumentSizeChanged();
    }
}

void TextEditorWidgetPrivate::toggleBlockVisible(const QTextBlock &block)
{
    if (q->singleShotAfterHighlightingDone([this, block] { toggleBlockVisible(block); }))
        return;

    auto documentLayout = qobject_cast<TextDocumentLayout*>(q->document()->documentLayout());
    QTC_ASSERT(documentLayout, return);

    TextBlockUserData::doFoldOrUnfold(block, TextBlockUserData::isFolded(block));
    documentLayout->requestUpdate();
    documentLayout->emitDocumentSizeChanged();
}

void TextEditorWidget::setLanguageSettingsId(Id settingsId)
{
    d->m_tabSettingsId = settingsId;
    if (auto doc = textDocument())
        doc->setCodeStyle(TextEditorSettings::codeStyle(settingsId));
}

Id TextEditorWidget::languageSettingsId() const
{
    return d->m_tabSettingsId;
}

const DisplaySettings &TextEditorWidget::displaySettings() const
{
    return d->m_displaySettings;
}

const MarginSettings &TextEditorWidget::marginSettings() const
{
    return d->m_marginSettings;
}

const BehaviorSettings &TextEditorWidget::behaviorSettings() const
{
    return d->m_behaviorSettings;
}

void TextEditorWidgetPrivate::handleHomeKey(bool anchor, bool block)
{
    const QTextCursor::MoveMode mode = anchor ? QTextCursor::KeepAnchor
                                              : QTextCursor::MoveAnchor;

    MultiTextCursor cursor = q->multiTextCursor();
    for (QTextCursor &c : cursor) {
        const int initpos = c.position();
        int pos = c.block().position();

        if (!block) {
            // only go to the first non space if we are in the first line of the layout
            if (QTextLayout *layout = q->editorLayout()->blockLayout(c.block());
                layout->lineForTextPosition(initpos - pos).lineNumber() != 0) {
                q->editorLayout()->moveCursor(c, QTextCursor::StartOfLine, mode);
                continue;
            }
        }

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
            pos = c.block().position();

        c.setPosition(pos, mode);
    }
    q->setMultiTextCursor(cursor);
}

void TextEditorWidgetPrivate::handleBackspaceKey()
{
    QTC_ASSERT(!q->multiTextCursor().hasSelection(), return);
    MultiTextCursor cursor = m_cursors;
    cursor.beginEditBlock();

    const TabSettings tabSettings = m_document->tabSettings();
    const TypingSettings &typingSettings = m_document->typingSettings();

    auto behavior = typingSettings.m_smartBackspaceBehavior;
    if (cursor.hasMultipleCursors()) {
        if (behavior == TypingSettings::BackspaceFollowsPreviousIndents) {
            behavior = TypingSettings::BackspaceNeverIndents;
        } else if (behavior == TypingSettings::BackspaceUnindents) {
            for (QTextCursor &c : cursor) {
                if (c.positionInBlock() == 0
                    || c.positionInBlock() > TabSettings::firstNonSpace(c.block().text())) {
                    behavior = TypingSettings::BackspaceNeverIndents;
                    break;
                }
            }
        }
    }

    for (QTextCursor &c : cursor) {
        const int pos = c.position();
        if (!pos)
            continue;

        bool cursorWithinSnippet = false;
        if (m_snippetOverlay->isVisible()) {
            QTextCursor snippetCursor = c;
            snippetCursor.movePosition(QTextCursor::Left);
            cursorWithinSnippet = snippetCheckCursor(snippetCursor);
        }

        if (typingSettings.m_autoIndent && !m_autoCompleteHighlightPos.isNull()
            && (m_autoCompleteHighlightPos == c) && m_removeAutoCompletedText
            && m_autoCompleter->autoBackspace(c)) {
            continue;
        }

        bool handled = false;
        if (behavior == TypingSettings::BackspaceNeverIndents) {
            if (cursorWithinSnippet)
                c.beginEditBlock();
            c.deletePreviousChar();
            handled = true;
        } else if (behavior
                   == TypingSettings::BackspaceFollowsPreviousIndents) {
            QTextBlock currentBlock = c.block();
            int positionInBlock = pos - currentBlock.position();
            const QString blockText = currentBlock.text();
            if (c.atBlockStart() || TabSettings::firstNonSpace(blockText) < positionInBlock) {
                if (cursorWithinSnippet)
                    c.beginEditBlock();
                c.deletePreviousChar();
                handled = true;
            } else {
                if (cursorWithinSnippet)
                    m_snippetOverlay->accept();
                cursorWithinSnippet = false;
                int previousIndent = 0;
                const int indent = tabSettings.columnAt(blockText, positionInBlock);
                for (QTextBlock previousNonEmptyBlock = currentBlock.previous();
                     previousNonEmptyBlock.isValid();
                     previousNonEmptyBlock = previousNonEmptyBlock.previous()) {
                    QString previousNonEmptyBlockText = previousNonEmptyBlock.text();
                    if (previousNonEmptyBlockText.trimmed().isEmpty())
                        continue;
                    previousIndent = tabSettings.columnAt(previousNonEmptyBlockText,
                                                          TabSettings::firstNonSpace(
                                                              previousNonEmptyBlockText));
                    if (previousIndent < indent) {
                        c.beginEditBlock();
                        c.setPosition(currentBlock.position(), QTextCursor::KeepAnchor);
                        c.insertText(tabSettings.indentationString(previousNonEmptyBlockText));
                        c.endEditBlock();
                        handled = true;
                        break;
                    }
                }
            }
        } else if (behavior == TypingSettings::BackspaceUnindents) {
            if (c.positionInBlock() == 0
                || c.positionInBlock() > TabSettings::firstNonSpace(c.block().text())) {
                if (cursorWithinSnippet)
                    c.beginEditBlock();
                c.deletePreviousChar();
            } else {
                if (cursorWithinSnippet)
                    m_snippetOverlay->accept();
                cursorWithinSnippet = false;
                c = m_document->unindent(MultiTextCursor({c})).mainCursor();
            }
            handled = true;
        }

        if (!handled) {
            if (cursorWithinSnippet)
                c.beginEditBlock();
            c.deletePreviousChar();
        }

        if (cursorWithinSnippet) {
            c.endEditBlock();
            m_snippetOverlay->updateEquivalentSelections(c);
        }
    }
    cursor.endEditBlock();
    q->setMultiTextCursor(cursor);
}

void TextEditorWidget::wheelEvent(QWheelEvent *e)
{
    d->clearVisibleFoldedBlock();
    if (e->modifiers() & Qt::ControlModifier) {
        if (!scrollWheelZoomingEnabled()) {
            // When the setting is disabled globally,
            // we have to skip calling PlainTextEdit::wheelEvent()
            // that changes zoom in it.
            return;
        }

        const int deltaY = e->angleDelta().y();
        if (deltaY != 0)
            zoomF(deltaY / 120.f);
        return;
    }
    PlainTextEdit::wheelEvent(e);
}

static void showZoomIndicator(QWidget *editor, const int newZoom)
{
    Utils::FadingIndicator::showText(editor,
                                     Tr::tr("Zoom: %1%").arg(newZoom),
                                     Utils::FadingIndicator::SmallText);
}

void TextEditorWidget::increaseFontZoom()
{
    d->clearVisibleFoldedBlock();
    showZoomIndicator(this, TextEditorSettings::increaseFontZoom());
}

void TextEditorWidget::decreaseFontZoom()
{
    d->clearVisibleFoldedBlock();
    showZoomIndicator(this, TextEditorSettings::decreaseFontZoom());
}

void TextEditorWidget::zoomF(float delta)
{
    d->clearVisibleFoldedBlock();
    float step = 10.f * delta;
    // Ensure we always zoom a minimal step in-case the resolution is more than 16x
    if (step > 0 && step < 1)
        step = 1;
    else if (step < 0 && step > -1)
        step = -1;

    const int newZoom = TextEditorSettings::increaseFontZoom(int(step));
    showZoomIndicator(this, newZoom);
}

void TextEditorWidget::zoomReset()
{
    TextEditorSettings::resetFontZoom();
    showZoomIndicator(this, 100);
}

void TextEditorWidget::findLinkAt(const QTextCursor &cursor,
                                  const Utils::LinkHandler &callback,
                                  bool resolveTarget,
                                  bool inNextSplit)
{
    emit requestLinkAt(cursor, callback, resolveTarget, inNextSplit);
}

void TextEditorWidget::findTypeAt(const QTextCursor &cursor,
                                  const Utils::LinkHandler &callback,
                                  bool resolveTarget,
                                  bool inNextSplit)
{
    emit requestTypeAt(cursor, callback, resolveTarget, inNextSplit);
}

bool TextEditorWidget::openLink(const Utils::Link &link, bool inNextSplit)
{
#ifdef WITH_TESTS
    struct Signaller {
        ~Signaller() { emit EditorManager::instance()->linkOpened(); }
    } s;
#endif

    QString url = link.targetFilePath.toUrlishString();
    if (url.startsWith(u"https://") || url.startsWith(u"http://")) {
        QDesktopServices::openUrl(url);
        return true;
    }

    if (!inNextSplit && textDocument()->filePath() == link.targetFilePath) {
        emit addCurrentStateToNavigationHistory();
        gotoLine(link.target.line, link.target.column, true, true);
        setFocus();
        return true;
    }
    if (!link.hasValidTarget())
        return false;
    EditorManager::OpenEditorFlags flags;
    if (inNextSplit)
        flags |= EditorManager::OpenInOtherSplit;

    return EditorManager::openEditorAt(link, Id(), flags);
}

bool TextEditorWidgetPrivate::isMouseNavigationEvent(QMouseEvent *e) const
{
    return q->mouseNavigationEnabled() && e->modifiers() & Qt::ControlModifier
           && !(e->modifiers() & Qt::ShiftModifier);
}

void TextEditorWidgetPrivate::requestUpdateLink(QMouseEvent *e)
{
    if (!isMouseNavigationEvent(e))
        return;
    // Link emulation behaviour for 'go to definition'
    const QTextCursor cursor = q->cursorForPosition(e->pos());

    // Avoid updating the link we already found
    if (cursor.position() >= m_currentLink.linkTextStart
        && cursor.position() <= m_currentLink.linkTextEnd)
        return;

    // Check that the mouse was actually on the text somewhere
    const int posX = e->position().x();
    bool onText = q->cursorRect(cursor).right() >= posX;
    if (!onText) {
        QTextCursor nextPos = cursor;
        nextPos.movePosition(QTextCursor::Right);
        onText = q->cursorRect(nextPos).right() >= posX;
    }

    if (onText) {
        m_pendingLinkUpdate = cursor;
        QMetaObject::invokeMethod(this, &TextEditorWidgetPrivate::updateLink, Qt::QueuedConnection);
        return;
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
    q->findLinkAt(m_pendingLinkUpdate,
                  [parent = QPointer<TextEditorWidget>(q), this](const Link &link) {
        if (!parent)
            return;

        if (link.hasValidLinkText())
            showLink(link);
        else
            clearLink();
    }, false);
}

void TextEditorWidgetPrivate::showLink(const Utils::Link &link)
{
    if (m_currentLink == link)
        return;

    QTextEdit::ExtraSelection sel;
    sel.cursor = q->textCursor();
    sel.cursor.setPosition(link.linkTextStart);
    sel.cursor.setPosition(link.linkTextEnd, QTextCursor::KeepAnchor);
    sel.format = m_document->fontSettings().toTextCharFormat(C_LINK);
    sel.format.setFontUnderline(true);
    q->setExtraSelections(TextEditorWidget::OtherSelection, QList<QTextEdit::ExtraSelection>() << sel);
    q->viewport()->setCursor(Qt::PointingHandCursor);
    m_currentLink = link;
}

void TextEditorWidgetPrivate::clearLink()
{
    m_pendingLinkUpdate = QTextCursor();
    m_lastLinkUpdate = QTextCursor();
    if (!m_currentLink.hasValidLinkText())
        return;

    q->setExtraSelections(TextEditorWidget::OtherSelection, QList<QTextEdit::ExtraSelection>());
    q->viewport()->setCursor(Qt::IBeamCursor);
    m_currentLink = Utils::Link();
}

void TextEditorWidgetPrivate::highlightSearchResultsSlot(const QString &txt, FindFlags findFlags)
{
    const QString pattern = (findFlags & FindRegularExpression) ? txt
                                                                : QRegularExpression::escape(txt);
    const QRegularExpression::PatternOptions options
        = (findFlags & FindCaseSensitively) ? QRegularExpression::NoPatternOption
                                            : QRegularExpression::CaseInsensitiveOption;
    if (m_searchExpr.pattern() == pattern && m_searchExpr.patternOptions() == options)
        return;
    m_searchExpr.setPattern(pattern);
    m_searchExpr.setPatternOptions(options);
    m_findText = txt;
    m_findFlags = findFlags;

    m_delayedUpdateTimer.start(50);

    if (m_highlightScrollBarController)
        m_scrollBarUpdateTimer.start(50);
}

void TextEditorWidgetPrivate::adjustScrollBarRanges()
{
    if (!m_highlightScrollBarController)
        return;
    const double lineSpacing = TextEditorSettings::fontSettings().lineSpacing();
    if (lineSpacing == 0)
        return;

    m_highlightScrollBarController->setLineHeight(lineSpacing);
    m_highlightScrollBarController->setVisibleRange(q->viewport()->rect().height());
    m_highlightScrollBarController->setMargin(q->textDocument()->document()->documentMargin());
}

void TextEditorWidgetPrivate::highlightSearchResultsInScrollBar()
{
    if (!m_highlightScrollBarController)
        return;
    m_highlightScrollBarController->removeHighlights(Constants::SCROLL_BAR_SEARCH_RESULT);
    m_searchResults.clear();

    if (m_searchFuture.isRunning())
        m_searchFuture.cancel();

    const QString &txt = m_findText;
    if (txt.isEmpty())
        return;

    adjustScrollBarRanges();

    m_searchFuture = Utils::asyncRun(Utils::searchInContents,
                                     txt,
                                     m_findFlags,
                                     m_document->filePath(),
                                     m_document->plainText());
    Utils::onResultReady(m_searchFuture, this, [this](const SearchResultItems &resultList) {
        QList<SearchResult> results;
        for (const SearchResultItem &result : resultList) {
            int start = result.mainRange().begin.toPositionInDocument(m_document->document());
            if (start < 0)
                continue;
            int end = result.mainRange().end.toPositionInDocument(m_document->document());
            if (end < 0)
                continue;
            if (start > end)
                std::swap(start, end);
            if (m_find->inScope(start, end))
                results << SearchResult{start, start - end};
        }
        m_searchResults << results;
        addSearchResultsToScrollBar(results);
    });
}

void TextEditorWidgetPrivate::scheduleUpdateHighlightScrollBar()
{
    if (m_scrollBarUpdateScheduled)
        return;

    m_scrollBarUpdateScheduled = true;
    QMetaObject::invokeMethod(this, &TextEditorWidgetPrivate::updateHighlightScrollBarNow,
                              Qt::QueuedConnection);
}

void TextEditorWidgetPrivate::slotFoldChanged(const int blockNumber, bool folded)
{
    if (folded)
        m_foldedBlockCache.insert(blockNumber);
    else
        m_foldedBlockCache.remove(blockNumber);
}

Highlight::Priority textMarkPrioToScrollBarPrio(const TextMark::Priority &prio)
{
    switch (prio) {
    case TextMark::LowPriority:
        return Highlight::LowPriority;
    case TextMark::NormalPriority:
        return Highlight::NormalPriority;
    case TextMark::HighPriority:
        return Highlight::HighPriority;
    default:
        return Highlight::NormalPriority;
    }
}

void TextEditorWidgetPrivate::addSearchResultsToScrollBar(
    const Id &category,
    const QList<SearchResult> &results,
    Theme::Color color,
    Highlight::Priority prio)
{
    if (!m_highlightScrollBarController)
        return;
    for (const SearchResult &result : results) {
        const QTextBlock &block = q->document()->findBlock(result.start);
        if (block.isValid() && block.isVisible()) {
            if (q->lineWrapMode() == PlainTextEdit::WidgetWidth) {
                auto blockLayout = q->editorLayout()->blockLayout(block);
                const int firstLine = blockLayout->lineForTextPosition(result.start - block.position()).lineNumber();
                const int lastLine = blockLayout->lineForTextPosition(result.start - block.position() + result.length).lineNumber();
                for (int line = firstLine; line <= lastLine; ++line) {
                    m_highlightScrollBarController->addHighlight(
                        {category, q->editorLayout()->firstLineNumberOf(block) + line, color, prio});
                }
            } else {
                m_highlightScrollBarController->addHighlight(
                    {category, q->editorLayout()->firstLineNumberOf(block), color, prio});
            }
        }
    }
}

void TextEditorWidgetPrivate::addSearchResultsToScrollBar(const QList<SearchResult> &results)
{
    addSearchResultsToScrollBar(
        Constants::SCROLL_BAR_SEARCH_RESULT,
        results,
        Theme::TextEditor_SearchResult_ScrollBarColor,
        Highlight::HighPriority);
}

void TextEditorWidgetPrivate::addSelectionHighlightToScrollBar(const QList<SearchResult> &selections)
{
    addSearchResultsToScrollBar(
        Constants::SCROLL_BAR_SELECTION,
        selections,
        Theme::TextEditor_Selection_ScrollBarColor,
        Highlight::NormalPriority);
}

Highlight markToHighlight(TextMark *mark, int lineNumber)
{
    return Highlight(mark->category().id,
                     lineNumber,
                     mark->color().value_or(Utils::Theme::TextColorNormal),
                     textMarkPrioToScrollBarPrio(mark->priority()));
}

void TextEditorWidgetPrivate::updateHighlightScrollBarNow()
{
    m_scrollBarUpdateScheduled = false;
    if (!m_highlightScrollBarController)
        return;

    m_highlightScrollBarController->removeAllHighlights();

    updateCurrentLineInScrollbar();

    // update search results
    addSearchResultsToScrollBar(m_searchResults);

    // update search selection
    addSelectionHighlightToScrollBar(m_selectionResults);

    // update text marks
    const TextMarks marks = m_document->marks();
    for (TextMark *mark : marks) {
        if (!mark->isVisible() || !mark->color().has_value())
            continue;
        const QTextBlock &block = q->document()->findBlockByNumber(mark->lineNumber() - 1);
        if (block.isVisible())
            m_highlightScrollBarController->addHighlight(markToHighlight(mark, q->editorLayout()->firstLineNumberOf(block)));
    }
}

MultiTextCursor TextEditorWidget::multiTextCursor() const
{
    return d->m_cursors;
}

void TextEditorWidget::setMultiTextCursor(const Utils::MultiTextCursor &cursor)
{
    if (cursor == d->m_cursors)
        return;

    const MultiTextCursor oldCursor = d->m_cursors;
    const_cast<MultiTextCursor &>(d->m_cursors) = cursor;
    doSetTextCursor(d->m_cursors.mainCursor(), /*keepMultiSelection*/ true);
    QRect updateRect = d->cursorUpdateRect(oldCursor);
    if (d->m_highlightCurrentLine)
        updateRect = QRect(0, updateRect.y(), viewport()->rect().width(), updateRect.height());
    updateRect |= d->cursorUpdateRect(d->m_cursors);
    viewport()->update(updateRect);
    emit cursorPositionChanged();
}

QRegion TextEditorWidget::translatedLineRegion(int lineStart, int lineEnd) const
{
    QRegion region;
    for (int i = lineStart ; i <= lineEnd; i++) {
        QTextBlock block = document()->findBlockByNumber(i);
        QPoint topLeft = blockBoundingGeometry(block).translated(contentOffset()).topLeft().toPoint();

        if (block.isValid()) {
            QTextLayout *layout = editorLayout()->blockLayout(block);
            for (int i = 0; i < layout->lineCount();i++) {
                QTextLine line = layout->lineAt(i);
                region += line.naturalTextRect().translated(topLeft).toRect();
            }
        }
    }
    return region;
}

void TextEditorWidgetPrivate::setFindScope(const Utils::MultiTextCursor &scope)
{
    if (m_findScope != scope) {
        m_findScope = scope;
        q->viewport()->update();
        highlightSearchResultsInScrollBar();
    }
}

void TextEditorWidgetPrivate::_q_animateUpdate(const QTextCursor &cursor,
                                               QPointF lastPos, QRectF rect)
{
    q->viewport()->update(QRectF(q->cursorRect(cursor).topLeft() + rect.topLeft(), rect.size()).toAlignedRect());
    if (!lastPos.isNull())
        q->viewport()->update(QRectF(lastPos + rect.topLeft(), rect.size()).toAlignedRect());
}


TextEditorAnimator::TextEditorAnimator(QObject *parent)
    : QObject(parent), m_timeline(256)
{
    m_value = 0;
    m_timeline.setEasingCurve(QEasingCurve::SineCurve);
    connect(&m_timeline, &QTimeLine::valueChanged, this, &TextEditorAnimator::step);
    connect(&m_timeline, &QTimeLine::finished, this, &QObject::deleteLater);
    m_timeline.start();
}

void TextEditorAnimator::init(const QTextCursor &cursor, const QFont &f, const QPalette &pal)
{
    m_cursor = cursor;
    m_font = f;
    m_palette = pal;
    m_text = cursor.selectedText();
    QFontMetrics fm(m_font);
    m_size = QSizeF(fm.horizontalAdvance(m_text), fm.height());
}

void TextEditorAnimator::draw(QPainter *p, const QPointF &pos)
{
    m_lastDrawPos = pos;
    p->setPen(m_palette.text().color());
    QFont f = m_font;
    f.setPointSizeF(f.pointSizeF() * (1.0 + m_value/2));
    QFontMetrics fm(f);
    int width = fm.horizontalAdvance(m_text);
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
    int width = fm.horizontalAdvance(m_text);
    return QRectF((m_size.width()-width)/2, (m_size.height() - fm.height())/2, width, fm.height());
}

void TextEditorAnimator::step(qreal v)
{
    QRectF before = rect();
    m_value = v;
    QRectF after = rect();
    emit updateRequest(m_cursor, m_lastDrawPos, before.united(after));
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

    const QTextCharFormat matchFormat = m_document->fontSettings().toTextCharFormat(C_PARENTHESES);
    const QTextCharFormat mismatchFormat = m_document->fontSettings().toTextCharFormat(
        C_PARENTHESES_MISMATCH);
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
        const QList<QTextEdit::ExtraSelection> selections = q->extraSelections(
            TextEditorWidget::ParenthesesMatchingSelection);
        for (const QTextEdit::ExtraSelection &sel : selections) {
            if (sel.cursor.selectionStart() == animatePosition
                || sel.cursor.selectionEnd() - 1 == animatePosition) {
                animatePosition = -1;
                break;
            }
        }
    }

    if (animatePosition >= 0) {
        cancelCurrentAnimations();// one animation is enough
        QPalette pal;
        pal.setBrush(QPalette::Text, matchFormat.foreground());
        pal.setBrush(QPalette::Base, matchFormat.background());
        QTextCursor cursor = q->textCursor();
        cursor.setPosition(animatePosition + 1);
        cursor.setPosition(animatePosition, QTextCursor::KeepAnchor);
        m_bracketsAnimator = new TextEditorAnimator(this);
        m_bracketsAnimator->init(cursor, q->font(), pal);
        connect(m_bracketsAnimator.data(), &TextEditorAnimator::updateRequest,
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
            && TextBlockUserData::foldingIndent(block.next())
            > TextBlockUserData::foldingIndent(block))
            block = block.next();
    }

    QTextBlock closeBlock = block;
    while (block.isValid()) {
        int foldingIndent = TextBlockUserData::foldingIndent(block);

        while (block.previous().isValid() && TextBlockUserData::foldingIndent(block) >= foldingIndent)
            block = block.previous();
        int nextIndent = TextBlockUserData::foldingIndent(block);
        if (nextIndent == foldingIndent)
            break;
        highlightBlocksInfo.open.prepend(block.blockNumber());
        while (closeBlock.next().isValid()
            && TextBlockUserData::foldingIndent(closeBlock.next()) >= foldingIndent )
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

void TextEditorWidgetPrivate::autocompleterHighlight(const QTextCursor &cursor)
{
    if ((!m_animateAutoComplete && !m_highlightAutoComplete)
            || q->isReadOnly() || !cursor.hasSelection()) {
        m_autoCompleteHighlightPos = QTextCursor();
    } else if (m_highlightAutoComplete) {
        if (m_autoCompleteHighlightPos.isNull())
            m_autoCompleteHighlightPos = cursor;
        else
            m_autoCompleteHighlightPos.setPosition(cursor.position(), QTextCursor::KeepAnchor);
    }
    if (m_animateAutoComplete) {
        const QTextCharFormat matchFormat = m_document->fontSettings().toTextCharFormat(
            C_AUTOCOMPLETE);
        cancelCurrentAnimations();// one animation is enough
        QPalette pal;
        pal.setBrush(QPalette::Text, matchFormat.foreground());
        pal.setBrush(QPalette::Base, matchFormat.background());
        m_autocompleteAnimator = new TextEditorAnimator(this);
        m_autocompleteAnimator->init(cursor, q->font(), pal);
        connect(m_autocompleteAnimator.data(), &TextEditorAnimator::updateRequest,
                this, &TextEditorWidgetPrivate::_q_animateUpdate);
    }
    updateAutoCompleteHighlight();
}

void TextEditorWidgetPrivate::updateAnimator(QPointer<TextEditorAnimator> animator,
                                             QPainter &painter)
{
    if (animator && animator->isRunning())
        animator->draw(&painter, q->cursorRect(animator->cursor()).topLeft());
}

void TextEditorWidgetPrivate::cancelCurrentAnimations()
{
    if (m_autocompleteAnimator)
        m_autocompleteAnimator->finish();
    if (m_bracketsAnimator)
        m_bracketsAnimator->finish();
}

void TextEditorWidget::changeEvent(QEvent *e)
{
    PlainTextEdit::changeEvent(e);
    if (e->type() == QEvent::ApplicationFontChange
        || e->type() == QEvent::FontChange) {
        if (d->m_extraArea) {
            QFont f = d->m_extraArea->font();
            f.setPointSizeF(font().pointSizeF());
            d->m_extraArea->setFont(f);
            d->slotUpdateExtraAreaWidth();
            d->m_extraArea->update();
        }
    } else if (e->type() == QEvent::PaletteChange) {
        applyFontSettings();
    }
}

void TextEditorWidget::focusInEvent(QFocusEvent *e)
{
    PlainTextEdit::focusInEvent(e);
    d->startCursorFlashTimer();
    d->updateHighlights();
}

void TextEditorWidget::focusOutEvent(QFocusEvent *e)
{
    PlainTextEdit::focusOutEvent(e);
    d->m_hoverHandlerRunner.abortHandlers();
    if (viewport()->cursor().shape() == Qt::BlankCursor)
        viewport()->setCursor(Qt::IBeamCursor);
    d->m_cursorFlashTimer.stop();
    if (d->m_cursorVisible) {
        d->m_cursorVisible = false;
        viewport()->update(d->cursorUpdateRect(d->m_cursors));
    }
    d->updateHighlights();
    if (!Utils::ToolTip::isVisible())
        d->clearCurrentSuggestion();
}

void TextEditorWidgetPrivate::maybeSelectLine()
{
    MultiTextCursor cursor = m_cursors;
    if (cursor.hasSelection())
        return;
    for (QTextCursor &c : cursor) {
        const QTextBlock &block = m_document->document()->findBlock(c.selectionStart());
        const QTextBlock &end = m_document->document()->findBlock(c.selectionEnd()).next();
        c.setPosition(block.position());
        if (!end.isValid()) {
            c.movePosition(QTextCursor::PreviousCharacter);
            c.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        } else {
            c.setPosition(end.position(), QTextCursor::KeepAnchor);
        }
    }
    cursor.mergeCursors();
    q->setMultiTextCursor(cursor);
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
    d->maybeSelectLine();
    copy();
}

void TextEditorWidget::copyWithHtml()
{
    if (!multiTextCursor().hasSelection())
        return;
    QGuiApplication::clipboard()->setMimeData(createMimeDataFromSelection(true));
}

void TextEditorWidgetPrivate::addCursorsToLineEnds()
{
    MultiTextCursor multiCursor = q->multiTextCursor();
    MultiTextCursor newMultiCursor;
    const QList<QTextCursor> cursors = multiCursor.cursors();

    if (multiCursor.cursorCount() == 0)
        return;

    QTextDocument *document = q->document();

    for (const QTextCursor &cursor : cursors) {
        if (!cursor.hasSelection())
            continue;

        QTextBlock block = document->findBlock(cursor.selectionStart());

        while (block.isValid()) {
            int blockEnd = block.position() + block.length() - 1;
            if (blockEnd >= cursor.selectionEnd()) {
                break;
            }

            QTextCursor newCursor(document);
            newCursor.setPosition(blockEnd);
            newMultiCursor.addCursor(newCursor);

            block = block.next();
        }
    }

    if (!newMultiCursor.isNull()) {
        q->setMultiTextCursor(newMultiCursor);
    }
}

void TextEditorWidgetPrivate::addSelectionNextFindMatch()
{
    MultiTextCursor cursor = q->multiTextCursor();
    const QList<QTextCursor> cursors = cursor.cursors();

    if (cursor.cursorCount() == 0 || !cursors.first().hasSelection())
        return;

    const QTextCursor &firstCursor = cursors.first();
    const QString selection = firstCursor.selectedText();
    if (selection.contains(QChar::ParagraphSeparator))
        return;
    QTextDocument *document = firstCursor.document();

    if (Utils::anyOf(cursors, [selection = selection.toCaseFolded()](const QTextCursor &c) {
            return c.selectedText().toCaseFolded() != selection;
        })) {
        return;
    }

    const QTextDocument::FindFlags findFlags = Utils::textDocumentFlagsForFindFlags(m_findFlags);

    int searchFrom = cursors.last().selectionEnd();
    while (true) {
        QTextCursor next = document->find(selection, searchFrom, findFlags);
        if (next.isNull()) {
            QTC_ASSERT(searchFrom != 0, return);
            searchFrom = 0;
            continue;
        }
        if (next.selectionStart() == firstCursor.selectionStart())
            break;
        cursor.addCursor(next);
        q->setMultiTextCursor(cursor);
        break;
    }
}

void TextEditorWidgetPrivate::duplicateSelection(bool comment)
{
    if (comment && !m_commentDefinition.hasMultiLineStyle())
        return;

    MultiTextCursor cursor = q->multiTextCursor();
    cursor.beginEditBlock();
    for (QTextCursor &c : cursor) {
        if (c.hasSelection()) {
            // Cannot "duplicate and comment" files without multi-line comment

            QString dupText = c.selectedText().replace(QChar::ParagraphSeparator,
                                                            QLatin1Char('\n'));
            if (comment) {
                dupText = (m_commentDefinition.multiLineStart + dupText
                           + m_commentDefinition.multiLineEnd);
            }
            const int selStart = c.selectionStart();
            const int selEnd = c.selectionEnd();
            const bool cursorAtStart = (c.position() == selStart);
            c.setPosition(selEnd);
            c.insertText(dupText);
            c.setPosition(cursorAtStart ? selEnd : selStart);
            c.setPosition(cursorAtStart ? selStart : selEnd, QTextCursor::KeepAnchor);
        } else if (!m_cursors.hasMultipleCursors()) {
            const int curPos = c.position();
            const QTextBlock &block = c.block();
            QString dupText = block.text() + QLatin1Char('\n');
            if (comment && m_commentDefinition.hasSingleLineStyle())
                dupText.append(m_commentDefinition.singleLine);
            c.setPosition(block.position());
            c.insertText(dupText);
            c.setPosition(curPos);
        }
    }
    cursor.endEditBlock();
    q->setMultiTextCursor(cursor);
}

void TextEditorWidget::duplicateSelection()
{
    d->duplicateSelection(false);
}

void TextEditorWidget::addCursorsToLineEnds()
{
    d->addCursorsToLineEnds();
}

void TextEditorWidget::addSelectionNextFindMatch()
{
    d->addSelectionNextFindMatch();
}

void TextEditorWidget::duplicateSelectionAndComment()
{
    d->duplicateSelection(true);
}

void TextEditorWidget::deleteLine()
{
    d->maybeSelectLine();
    textCursor().removeSelectedText();
}

void TextEditorWidget::deleteEndOfLine()
{
    d->moveCursor(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
    MultiTextCursor cursor = multiTextCursor();
    cursor.removeSelectedText();
    setMultiTextCursor(cursor);
}

void TextEditorWidget::deleteEndOfWord()
{
    d->moveCursor(QTextCursor::NextWord, QTextCursor::KeepAnchor);
    MultiTextCursor cursor = multiTextCursor();
    cursor.removeSelectedText();
    setMultiTextCursor(cursor);
}

void TextEditorWidget::deleteEndOfWordCamelCase()
{
    MultiTextCursor cursor = multiTextCursor();
    CamelCaseCursor::right(&cursor, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    setMultiTextCursor(cursor);
}

void TextEditorWidget::deleteStartOfLine()
{
    d->moveCursor(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
    MultiTextCursor cursor = multiTextCursor();
    cursor.removeSelectedText();
    setMultiTextCursor(cursor);
}

void TextEditorWidget::deleteStartOfWord()
{
    d->moveCursor(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
    MultiTextCursor cursor = multiTextCursor();
    cursor.removeSelectedText();
    setMultiTextCursor(cursor);
}

void TextEditorWidget::deleteStartOfWordCamelCase()
{
    MultiTextCursor cursor = multiTextCursor();
    CamelCaseCursor::left(&cursor, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    setMultiTextCursor(cursor);
}

void TextEditorWidgetPrivate::setExtraSelections(Id kind, const QList<QTextEdit::ExtraSelection> &selections)
{
    if (selections.isEmpty() && m_extraSelections[kind].isEmpty())
        return;
    m_extraSelections[kind] = selections;

    if (kind == TextEditorWidget::CodeSemanticsSelection) {
        m_overlay->clear();
        for (const QTextEdit::ExtraSelection &selection : selections) {
            m_overlay->addOverlaySelection(selection.cursor,
                                              selection.format.background().color(),
                                              selection.format.background().color(),
                                              TextEditorOverlay::LockSize);
        }
        m_overlay->setVisible(!m_overlay->isEmpty());
    } else {
        QList<QTextEdit::ExtraSelection> all = m_extraSelections.value(
            TextEditorWidget::OtherSelection);

        for (auto i = m_extraSelections.constBegin(); i != m_extraSelections.constEnd(); ++i) {
            if (i.key() == TextEditorWidget::CodeSemanticsSelection
                || i.key() == TextEditorWidget::SnippetPlaceholderSelection
                || i.key() == TextEditorWidget::OtherSelection)
                continue;
            all += i.value();
        }
        q->PlainTextEdit::setExtraSelections(all);
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
    for (const QList<QTextEdit::ExtraSelection> &sel : std::as_const(d->m_extraSelections)) {
        for (const QTextEdit::ExtraSelection &s : sel) {
            if (s.cursor.selectionStart() <= pos
                && s.cursor.selectionEnd() >= pos
                && !s.format.toolTip().isEmpty())
                return s.format.toolTip();
        }
    }
    return QString();
}

void TextEditorWidget::autoIndent()
{
    MultiTextCursor cursor = multiTextCursor();
    cursor.beginEditBlock();
    // The order is important, since some indenter refer to previous indent positions.
    const QList<QTextCursor> cursors = Utils::sorted(cursor.cursors(),
            [](const QTextCursor &lhs, const QTextCursor &rhs) {
        return lhs.selectionStart() < rhs.selectionStart();
    });
    for (const QTextCursor &c : cursors)
        d->m_document->autoFormatOrIndent(c);
    cursor.mergeCursors();
    cursor.endEditBlock();
    setMultiTextCursor(cursor);
}

void TextEditorWidget::rewrapParagraph()
{
    const int paragraphWidth = marginSettings().m_marginColumn;
    static const QRegularExpression anyLettersOrNumbers("\\w");
    const TabSettings ts = d->m_document->tabSettings();

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
    const QString text = cursor.block().text();
    int indentLevel = ts.indentationColumn(text);

    // If there is a common prefix, it should be kept and expanded to all lines.
    // this allows nice reflowing of doxygen style comments.
    QTextCursor nextBlock = cursor;
    QString commonPrefix;

    const QString doxygenPrefix("^\\s*(?:///|/\\*\\*|/\\*\\!|\\*)?[ *]+");
    if (nextBlock.movePosition(QTextCursor::NextBlock))
    {
         QString nText = nextBlock.block().text();
         int maxLength = qMin(text.length(), nText.length());

         const auto hasDoxygenPrefix = [&] {
             static const QRegularExpression pattern(doxygenPrefix);
             return pattern.match(commonPrefix).hasMatch();
         };

         for (int i = 0; i < maxLength; ++i) {
             const QChar ch = text.at(i);

             if (ch != nText[i] || ch.isLetterOrNumber()
                     || ((ch == '@' || ch == '\\' ) && hasDoxygenPrefix())) {
                 break;
             }
             commonPrefix.append(ch);
         }
    }

    // Find end of paragraph.
    static const QRegularExpression immovableDoxygenCommand(doxygenPrefix + "[@\\\\][a-zA-Z]{2,}");
    QTC_CHECK(immovableDoxygenCommand.isValid());
    while (cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor)) {
        QString text = cursor.block().text();

        if (!text.contains(anyLettersOrNumbers) || immovableDoxygenCommand.match(text).hasMatch())
            break;
    }


    QString selectedText = cursor.selectedText();

    // Preserve initial indent level.or common prefix.
    QString spacing;

    if (commonPrefix.isEmpty()) {
        spacing = ts.indentationString(0, indentLevel, 0);
    } else {
        spacing = commonPrefix;
        indentLevel = ts.columnCountForText(spacing);
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

    for (const QChar &ch : std::as_const(selectedText)) {
        if (ch.isSpace() && ch != QChar::Nbsp) {
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
    const bool singleLine = d->m_document->typingSettings().m_preferSingleLineComments;
    const MultiTextCursor cursor = Utils::unCommentSelection(multiTextCursor(),
                                                             d->m_commentDefinition,
                                                             singleLine);
    setMultiTextCursor(cursor);
}

void TextEditorWidget::autoFormat()
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();
    d->m_document->autoFormat(cursor);
    cursor.endEditBlock();
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
    // PlainTextEdit::showEvent scrolls to make the cursor visible on first show
    // which we don't want, since we restore previous states when
    // opening editors, and when splitting/duplicating.
    // So restore the previous state after that.
    QByteArray state;
    if (d->m_wasNotYetShown)
        state = saveState();
    PlainTextEdit::showEvent(e);
    if (d->m_wasNotYetShown) {
        restoreState(state);
        d->m_wasNotYetShown = false;
    }
}

void TextEditorWidgetPrivate::applyFontSettingsDelayed()
{
    m_fontSettingsNeedsApply = true;
    if (q->isVisible())
        q->triggerPendingUpdates();
    updateActions();
}

void TextEditorWidgetPrivate::markRemoved(TextMark *mark)
{
    if (m_dragMark == mark) {
        m_dragMark = nullptr;
        m_markDragging = false;
        m_markDragStart = QPoint();
        QGuiApplication::restoreOverrideCursor();
    }

    auto it = m_annotationRects.find(mark->lineNumber() - 1);
    if (it == m_annotationRects.end())
        return;

    Utils::erase(it.value(), [mark](AnnotationRect rect) {
        return rect.mark == mark;
    });
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
    const QTextCharFormat lineNumberFormat = fs.toTextCharFormat(C_LINE_NUMBER);
    QFont font(textFormat.font());

    if (font != this->font()) {
        setFont(font);
        d->updateTabStops(); // update tab stops, they depend on the font
    } else if (font != document()->defaultFont()) {
        // When the editor already have the correct font configured it wont generate a font change
        // signal. In turn the default font of the document wont get updated so we need to do that
        // manually here
        document()->setDefaultFont(font);
        if (auto documentLayout = qobject_cast<TextDocumentLayout*>(document()->documentLayout()))
            documentLayout->emitDocumentSizeChanged();
    }

    // Line numbers
    QPalette ep;
    ep.setColor(QPalette::Dark, lineNumberFormat.foreground().color());
    ep.setColor(QPalette::Window, lineNumberFormat.background().style() != Qt::NoBrush ?
                lineNumberFormat.background().color() : textFormat.background().color());
    if (ep != d->m_extraArea->palette()) {
        d->m_extraArea->setPalette(ep);
        d->slotUpdateExtraAreaWidth();   // Adjust to new font width
    }

    d->updateHighlights();
}

void TextEditorWidget::setDisplaySettings(const DisplaySettings &ds)
{
    const TextEditor::FontSettings &fs = TextEditorSettings::fontSettings();
    if (fs.relativeLineSpacing() == 100)
        setLineWrapMode(ds.m_textWrapping ? PlainTextEdit::WidgetWidth : PlainTextEdit::NoWrap);
    else
        setLineWrapMode(PlainTextEdit::NoWrap);

    QTC_ASSERT((fs.relativeLineSpacing() == 100) || (fs.relativeLineSpacing() != 100
        && lineWrapMode() == PlainTextEdit::NoWrap), setLineWrapMode(PlainTextEdit::NoWrap));

    setLineNumbersVisible(ds.m_displayLineNumbers);
    setHighlightCurrentLine(ds.m_highlightCurrentLine);
    setRevisionsVisible(ds.m_markTextChanges);
    setCenterOnScroll(ds.m_centerCursorOnScroll);
    setParenthesesMatchingEnabled(ds.m_highlightMatchingParentheses);
    d->m_fileEncodingLabelAction->setVisible(ds.m_displayFileEncoding);

    const QTextOption::Flags currentOptionFlags = document()->defaultTextOption().flags();
    QTextOption::Flags optionFlags = currentOptionFlags;
    optionFlags.setFlag(QTextOption::AddSpaceForLineAndParagraphSeparators);
    optionFlags.setFlag(QTextOption::ShowTabsAndSpaces, ds.m_visualizeWhitespace);
    if (optionFlags != currentOptionFlags) {
        if (SyntaxHighlighter *highlighter = textDocument()->syntaxHighlighter())
            highlighter->rehighlight();
        QTextOption option = document()->defaultTextOption();
        option.setFlags(optionFlags);
        document()->setDefaultTextOption(option);
    }

    d->m_displaySettings = ds;
    if (!ds.m_highlightBlocks) {
        d->extraAreaHighlightFoldedBlockNumber = -1;
        d->m_highlightBlocksInfo = TextEditorPrivateHighlightBlocks();
    }

    d->updateCodeFoldingVisible();
    d->updateFileLineEndingVisible();
    d->updateTabSettingsButtonVisible();
    d->updateHighlights();
    d->setupScrollBar();
    d->updateCursorSelections();
    viewport()->update();
    extraArea()->update();
}

void TextEditorWidget::setMarginSettings(const MarginSettings &ms)
{
    d->m_marginSettings = ms;
    updateVisualWrapColumn();

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
    d->setupFromDefinition(d->currentDefinition());
}

void TextEditorWidget::setStorageSettings(const StorageSettings &storageSettings)
{
    d->m_document->setStorageSettings(storageSettings);
}

void TextEditorWidget::setCompletionSettings(const CompletionSettings &completionSettings)
{
    d->m_autoCompleter->setAutoInsertBracketsEnabled(completionSettings.m_autoInsertBrackets);
    d->m_autoCompleter->setSurroundWithBracketsEnabled(completionSettings.m_surroundingAutoBrackets);
    d->m_autoCompleter->setAutoInsertQuotesEnabled(completionSettings.m_autoInsertQuotes);
    d->m_autoCompleter->setSurroundWithQuotesEnabled(completionSettings.m_surroundingAutoQuotes);
    d->m_autoCompleter->setOverwriteClosingCharsEnabled(completionSettings.m_overwriteClosingChars);
    d->m_animateAutoComplete = completionSettings.m_animateAutoComplete;
    d->m_highlightAutoComplete = completionSettings.m_highlightAutoComplete;
    d->m_skipAutoCompletedText = completionSettings.m_skipAutoCompletedText;
    d->m_removeAutoCompletedText = completionSettings.m_autoRemove;
}

void TextEditorWidget::setExtraEncodingSettings(const ExtraEncodingSettings &extraEncodingSettings)
{
    d->m_document->setExtraEncodingSettings(extraEncodingSettings);
}

void TextEditorWidget::foldCurrentBlock()
{
    fold(textCursor().block());
}

void TextEditorWidget::fold(const QTextBlock &block, bool recursive)
{
    if (singleShotAfterHighlightingDone([this, block] { fold(block); }))
        return;

    QTextDocument *doc = document();
    auto documentLayout = qobject_cast<TextDocumentLayout*>(doc->documentLayout());
    QTC_ASSERT(documentLayout, return);
    QTextBlock b = block;
    if (!(TextBlockUserData::canFold(b) && b.next().isVisible())) {
        // find the closest previous block which can fold
        int indent = TextBlockUserData::foldingIndent(b);
        while (b.isValid() && (TextBlockUserData::foldingIndent(b) >= indent || !b.isVisible()))
            b = b.previous();
    }
    if (b.isValid()) {
        TextBlockUserData::doFoldOrUnfold(b, false, recursive);
        d->moveCursorVisible();
        documentLayout->requestUpdate();
        documentLayout->emitDocumentSizeChanged();
    }
}

void TextEditorWidget::unfold(const QTextBlock &block, bool recursive)
{
    if (singleShotAfterHighlightingDone([this, block] { unfold(block); }))
        return;

    QTextDocument *doc = document();
    auto documentLayout = qobject_cast<TextDocumentLayout*>(doc->documentLayout());
    QTC_ASSERT(documentLayout, return);
    QTextBlock b = block;
    while (b.isValid() && !b.isVisible())
        b = b.previous();
    TextBlockUserData::doFoldOrUnfold(b, true, recursive);
    d->moveCursorVisible();
    documentLayout->requestUpdate();
    documentLayout->emitDocumentSizeChanged();
}

void TextEditorWidget::unfoldCurrentBlock()
{
    unfold(textCursor().block());
}

void TextEditorWidget::toggleFoldAll()
{
    if (singleShotAfterHighlightingDone([this] { toggleFoldAll(); }))
        return;

    QTextDocument *doc = document();
    QTextBlock block = doc->firstBlock();

    bool makeVisible = true;
    while (block.isValid()) {
        if (block.isVisible() && TextBlockUserData::canFold(block) && block.next().isVisible()) {
            makeVisible = false;
            break;
        }
        block = block.next();
    }

    unfoldAll(makeVisible);
}

void TextEditorWidget::unfoldAll(bool unfold)
{
    if (singleShotAfterHighlightingDone([this, unfold] { unfoldAll(unfold); }))
        return;

    QTextDocument *doc = document();
    auto documentLayout = qobject_cast<TextDocumentLayout*>(doc->documentLayout());
    QTC_ASSERT(documentLayout, return);

    QTextBlock block = doc->firstBlock();

    while (block.isValid()) {
        if (TextBlockUserData::canFold(block))
            TextBlockUserData::doFoldOrUnfold(block, unfold);
        block = block.next();
    }

    d->moveCursorVisible();
    documentLayout->requestUpdate();
    documentLayout->emitDocumentSizeChanged();
    centerCursor();
}

void TextEditorWidget::cut()
{
    copy();
    MultiTextCursor cursor = multiTextCursor();
    cursor.removeSelectedText();
    setMultiTextCursor(cursor);
    d->collectToCircularClipboard();
}

void TextEditorWidget::selectAll()
{
    // Directly update the internal multi text cursor here to prevent calling setTextCursor.
    // This would indirectly make sure the cursor is visible which is not desired for select all.
    QTextCursor c = PlainTextEdit::textCursor();
    c.select(QTextCursor::Document);
    const_cast<MultiTextCursor &>(d->m_cursors).setCursors({c});
    PlainTextEdit::selectAll();
}

void TextEditorWidget::copy()
{
    PlainTextEdit::copy();
    d->collectToCircularClipboard();
}

void TextEditorWidget::paste()
{
    PlainTextEdit::paste();
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

    if (circularClipBoard->size() > 1) {
        invokeAssist(QuickFix, &clipboardAssistProvider());
        return;
    }

    if (const QMimeData *mimeData = circularClipBoard->next().get()) {
        QApplication::clipboard()->setMimeData(TextEditorWidget::duplicateMimeData(mimeData));
        paste();
    }
}

void TextEditorWidget::pasteWithoutFormat()
{
    d->m_skipFormatOnPaste = true;
    paste();
    d->m_skipFormatOnPaste = false;
}

void TextEditorWidget::switchUtf8bom()
{
    textDocument()->switchUtf8Bom();
}

QMimeData *TextEditorWidget::createMimeDataFromSelection() const
{
    return createMimeDataFromSelection(false);
}

QMimeData *TextEditorWidget::createMimeDataFromSelection(bool withHtml) const
{
    if (multiTextCursor().hasSelection()) {
        auto mimeData = new QMimeData;

        QString text = plainTextFromSelection(multiTextCursor());
        mimeData->setText(text);

        // Copy the selected text as HTML
        if (withHtml) {
            // Create a new document from the selected text document fragment
            auto tempDocument = new QTextDocument;
            QTextCursor tempCursor(tempDocument);
            for (const QTextCursor &cursor : multiTextCursor()) {
                if (!cursor.hasSelection())
                    continue;
                tempCursor.insertFragment(cursor.selection());

                // Apply the additional formats set by the syntax highlighter
                QTextBlock start = document()->findBlock(cursor.selectionStart());
                QTextBlock last = document()->findBlock(cursor.selectionEnd());
                QTextBlock end = last.next();

                const int selectionStart = cursor.selectionStart();
                const int endOfDocument = tempDocument->characterCount() - 1;
                int removedCount = 0;
                for (QTextBlock current = start; current.isValid() && current != end;
                     current = current.next()) {
                    if (selectionVisible(current.blockNumber())) {
                        const QTextLayout *layout = editorLayout()->blockLayout(current);
                        const QList<QTextLayout::FormatRange> ranges = layout->formats();
                        for (const QTextLayout::FormatRange &range : ranges) {
                            const int startPosition = current.position() + range.start
                                                      - selectionStart - removedCount;
                            const int endPosition = startPosition + range.length;
                            if (endPosition <= 0 || startPosition >= endOfDocument - removedCount)
                                continue;
                            tempCursor.setPosition(qMax(startPosition, 0));
                            tempCursor.setPosition(qMin(endPosition, endOfDocument - removedCount),
                                                   QTextCursor::KeepAnchor);
                            tempCursor.setCharFormat(range.format);
                        }
                    } else {
                        const int startPosition = current.position() - selectionStart
                                                  - removedCount;
                        int endPosition = startPosition + current.text().size();
                        if (current != last)
                            endPosition++;
                        removedCount += endPosition - startPosition;
                        tempCursor.setPosition(startPosition);
                        tempCursor.setPosition(endPosition, QTextCursor::KeepAnchor);
                        tempCursor.deleteChar();
                    }
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

        if (!multiTextCursor().hasMultipleCursors()) {
            /*
                Try to figure out whether we are copying an entire block, and store the
                complete block including indentation in the qtcreator.blocktext mimetype.
            */
            QTextCursor cursor = multiTextCursor().mainCursor();
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
        }
        return mimeData;
    }
    return nullptr;
}

bool TextEditorWidget::canInsertFromMimeData(const QMimeData *source) const
{
    return PlainTextEdit::canInsertFromMimeData(source);
}

struct MappedText
{
    MappedText(const QString text, MultiTextCursor &cursor)
        : text(text)
    {
        if (cursor.hasMultipleCursors()) {
            texts = text.split('\n');
            if (texts.last().isEmpty())
                texts.removeLast();
            if (texts.count() != cursor.cursorCount())
                texts.clear();
        }
    }

    QString textAt(int i) const
    {
        return texts.value(i, text);
    }

    QStringList texts;
    const QString text;
};

void TextEditorWidget::insertFromMimeData(const QMimeData *source)
{
    if (!source || isReadOnly())
        return;

    QString text = source->text();
    if (text.isEmpty())
        return;

    if (d->m_codeAssistant.hasContext())
        d->m_codeAssistant.destroyContext();

    if (d->m_snippetOverlay->isVisible() && (text.contains('\n') || text.contains('\t')))
        d->m_snippetOverlay->accept();

    const bool selectInsertedText = source->property(dropProperty).toBool();
    const TypingSettings &tps = d->m_document->typingSettings();
    MultiTextCursor cursor = multiTextCursor();
    if (!tps.m_autoIndent) {
        cursor.insertText(text, selectInsertedText);
        setMultiTextCursor(cursor);
        return;
    }

    if (source->hasFormat(QLatin1String(kTextBlockMimeType))) {
        text = QString::fromUtf8(source->data(QLatin1String(kTextBlockMimeType)));
        if (text.isEmpty())
            return;
    }

    MappedText mappedText(text, cursor);

    int index = 0;
    cursor.beginEditBlock();
    for (QTextCursor &cursor : cursor) {
        const QString textForCursor = mappedText.textAt(index++);

        cursor.removeSelectedText();

        bool insertAtBeginningOfLine = TabSettings::cursorIsAtBeginningOfLine(cursor);
        int reindentBlockStart = cursor.blockNumber() + (insertAtBeginningOfLine ? 0 : 1);

        bool hasFinalNewline = (textForCursor.endsWith(QLatin1Char('\n'))
                                || textForCursor.endsWith(QChar::ParagraphSeparator)
                                || textForCursor.endsWith(QLatin1Char('\r')));

        if (insertAtBeginningOfLine
            && hasFinalNewline) // since we'll add a final newline, preserve current line's indentation
            cursor.setPosition(cursor.block().position());

        int cursorPosition = cursor.position();
        cursor.insertText(textForCursor);
        const QTextCursor endCursor = cursor;
        QTextCursor startCursor = endCursor;
        startCursor.setPosition(cursorPosition);

        int reindentBlockEnd = cursor.blockNumber() - (hasFinalNewline ? 1 : 0);

        if (!d->m_skipFormatOnPaste
            && (reindentBlockStart < reindentBlockEnd
                || (reindentBlockStart == reindentBlockEnd
                    && (!insertAtBeginningOfLine || hasFinalNewline)))) {
            if (insertAtBeginningOfLine && !hasFinalNewline) {
                QTextCursor unnecessaryWhitespace = cursor;
                unnecessaryWhitespace.setPosition(cursorPosition);
                unnecessaryWhitespace.movePosition(QTextCursor::StartOfBlock,
                                                   QTextCursor::KeepAnchor);
                unnecessaryWhitespace.removeSelectedText();
            }
            QTextCursor c = cursor;
            c.setPosition(cursor.document()->findBlockByNumber(reindentBlockStart).position());
            c.setPosition(cursor.document()->findBlockByNumber(reindentBlockEnd).position(),
                          QTextCursor::KeepAnchor);
            d->m_document->autoReindent(c);
        }

        if (selectInsertedText) {
            cursor.setPosition(startCursor.position());
            cursor.setPosition(endCursor.position(), QTextCursor::KeepAnchor);
        }
    }
    cursor.endEditBlock();
    setMultiTextCursor(cursor);
}

void TextEditorWidget::dragLeaveEvent(QDragLeaveEvent *)
{
    const QRect rect = cursorRect(d->m_dndCursor);
    d->m_dndCursor = QTextCursor();
    if (!rect.isNull())
        viewport()->update(rect);
}

void TextEditorWidget::dragMoveEvent(QDragMoveEvent *e)
{
    const QRect rect = cursorRect(d->m_dndCursor);
    d->m_dndCursor = cursorForPosition(e->position().toPoint());
    if (!rect.isNull())
        viewport()->update(rect);
    viewport()->update(cursorRect(d->m_dndCursor));
}

void TextEditorWidget::dropEvent(QDropEvent *e)
{
    const QRect rect = cursorRect(d->m_dndCursor);
    d->m_dndCursor = QTextCursor();
    if (!rect.isNull())
        viewport()->update(rect);
    const QMimeData *mime = e->mimeData();
    if (!canInsertFromMimeData(mime))
        return;
    // Update multi text cursor before inserting data
    MultiTextCursor cursor = multiTextCursor();
    cursor.beginEditBlock();
    const QTextCursor eventCursor = cursorForPosition(e->position().toPoint());
    if (e->dropAction() == Qt::MoveAction && e->source() == viewport())
        cursor.removeSelectedText();
    cursor.setCursors({eventCursor});
    setMultiTextCursor(cursor);
    QMimeData *mimeOverwrite = nullptr;
    if (mime && (mime->hasText() || mime->hasHtml())) {
        mimeOverwrite = duplicateMimeData(mime);
        mimeOverwrite->setProperty(dropProperty, true);
        mime = mimeOverwrite;
    }
    insertFromMimeData(mime);
    delete mimeOverwrite;
    cursor.endEditBlock();
    e->acceptProposedAction();
}

QMimeData *TextEditorWidget::duplicateMimeData(const QMimeData *source)
{
    Q_ASSERT(source);

    auto mimeData = new QMimeData;
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
    Q_UNUSED(blockNumber)
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
    return {};
}

void TextEditorWidget::setupFallBackEditor(Id id)
{
    TextDocumentPtr doc(new TextDocument(id));
    doc->setFontSettings(TextEditorSettings::fontSettings());
    setTextDocument(doc);
}

void TextEditorWidget::appendStandardContextMenuActions(QMenu *menu)
{
    if (optionalActions() & OptionalActions::FollowSymbolUnderCursor) {
        const auto action = ActionManager::command(Constants::FOLLOW_SYMBOL_UNDER_CURSOR)->action();
        if (!menu->actions().contains(action))
            menu->addAction(action);
    }
    if (optionalActions() & OptionalActions::FollowTypeUnderCursor) {
        const auto action = ActionManager::command(Constants::FOLLOW_SYMBOL_TO_TYPE)->action();
        if (!menu->actions().contains(action))
            menu->addAction(action);
    }
    if (optionalActions() & OptionalActions::FindUsage) {
        const auto action = ActionManager::command(Constants::FIND_USAGES)->action();
        if (!menu->actions().contains(action))
            menu->addAction(action);
    }
    if (optionalActions() & OptionalActions::RenameSymbol) {
        const auto action = ActionManager::command(Constants::RENAME_SYMBOL)->action();
        if (!menu->actions().contains(action))
            menu->addAction(action);
    }
    if (optionalActions() & OptionalActions::CallHierarchy) {
        const auto action = ActionManager::command(Constants::OPEN_CALL_HIERARCHY)->action();
        if (!menu->actions().contains(action))
            menu->addAction(action);
    }
    if (optionalActions() & OptionalActions::TypeHierarchy) {
        const auto action = ActionManager::command(Constants::OPEN_TYPE_HIERARCHY)->action();
        if (!menu->actions().contains(action))
            menu->addAction(action);
    }

    menu->addSeparator();
    appendMenuActionsFromContext(menu, Constants::M_STANDARDCONTEXTMENU);

    if (Command *bomCmd = ActionManager::command(Constants::SWITCH_UTF8BOM)) {
        QAction *a = bomCmd->action();
        TextDocument *doc = textDocument();
        if (doc->encoding().isUtf8() && doc->supportsUtf8Bom()) {
            a->setVisible(true);
            a->setText(doc->format().hasUtf8Bom ? Tr::tr("Delete UTF-8 BOM on Save")
                                                : Tr::tr("Add UTF-8 BOM on Save"));
        } else {
            a->setVisible(false);
        }
    }
}

uint TextEditorWidget::optionalActions()
{
    return d->m_optionalActionMask;
}

void TextEditorWidget::setOptionalActions(uint optionalActionMask)
{
    if (d->m_optionalActionMask == optionalActionMask)
        return;
    d->m_optionalActionMask = optionalActionMask;
    d->updateOptionalActions();
}

void TextEditorWidget::addOptionalActions( uint optionalActionMask)
{
    setOptionalActions(d->m_optionalActionMask | optionalActionMask);
}

BaseTextEditor::BaseTextEditor()
    : d(new BaseTextEditorPrivate)
{
    addContext(Constants::C_TEXTEDITOR);
    setContextHelpProvider([this](const HelpCallback &callback) {
        editorWidget()->contextHelpItem(callback);
    });
}

BaseTextEditor::~BaseTextEditor()
{
    delete m_widget;
    delete d;
}

TextDocument *BaseTextEditor::textDocument() const
{
    TextDocument *doc = editorWidget()->textDocument();
    QTC_CHECK(doc);
    return doc;
}

void BaseTextEditor::addContext(Id id)
{
    m_context.add(id);
}

IDocument *BaseTextEditor::document() const
{
    return textDocument();
}

QWidget *BaseTextEditor::toolBar()
{
    return editorWidget()->toolBarWidget();
}

QAction * TextEditorWidget::insertExtraToolBarWidget(TextEditorWidget::Side side,
                                                     QWidget *widget)
{
    if (widget->sizePolicy().horizontalPolicy() & QSizePolicy::ExpandFlag)
        d->m_stretchAction->setVisible(false);

    if (side == Left) {
        auto findLeftMostAction = [this](QAction *action) {
            if (d->m_toolbarOutlineAction && action == d->m_toolbarOutlineAction)
                return false;
            return d->m_toolBar->widgetForAction(action) != nullptr;
        };
        QAction *before = Utils::findOr(d->m_toolBar->actions(),
                                        d->m_fileEncodingLabelAction,
                                        findLeftMostAction);
        return d->m_toolBar->insertWidget(before, widget);
    } else {
        return d->m_toolBar->insertWidget(d->m_fileLineEndingAction, widget);
    }
}

void TextEditorWidget::insertExtraToolBarAction(TextEditorWidget::Side side, QAction *action)
{
    if (side == Left) {
        auto findLeftMostAction = [this](QAction *action) {
            if (d->m_toolbarOutlineAction && action == d->m_toolbarOutlineAction)
                return false;
            return d->m_toolBar->widgetForAction(action) != nullptr;
        };
        QAction *before = Utils::findOr(d->m_toolBar->actions(),
                                        d->m_fileEncodingLabelAction,
                                        findLeftMostAction);
        d->m_toolBar->insertAction(before, action);
    } else {
        d->m_toolBar->insertAction(d->m_fileLineEndingAction, action);
    }
}

void TextEditorWidget::setToolbarOutline(QWidget *widget)
{
    if (d->m_toolbarOutlineAction) {
        if (d->m_toolBar->widgetForAction(d->m_toolbarOutlineAction) == widget)
            return;
        d->m_toolBar->removeAction(d->m_toolbarOutlineAction);
        delete d->m_toolbarOutlineAction;
        d->m_toolbarOutlineAction = nullptr;
    } else if (!widget) {
        return;
    }

    if (widget) {
        if (widget->sizePolicy().horizontalPolicy() & QSizePolicy::ExpandFlag)
            d->m_stretchAction->setVisible(false);

        d->m_toolbarOutlineAction = insertExtraToolBarWidget(Left, widget);
    } else {
        // check for a widget with an expanding size policy otherwise re-enable the stretcher
        for (auto action : d->m_toolBar->actions()) {
            if (QWidget *toolbarWidget = d->m_toolBar->widgetForAction(action)) {
                if (toolbarWidget->isVisible()
                    && toolbarWidget->sizePolicy().horizontalPolicy() & QSizePolicy::ExpandFlag) {
                    d->m_stretchAction->setVisible(false);
                    return;
                }
            }
        }
        d->m_stretchAction->setVisible(true);
    }

    emit toolbarOutlineChanged(widget);
}

const QWidget *TextEditorWidget::toolbarOutlineWidget()
{
    return d->m_toolbarOutlineAction ? d->m_toolBar->widgetForAction(d->m_toolbarOutlineAction)
                                     : nullptr;
}

void TextEditorWidget::keepAutoCompletionHighlight(bool keepHighlight)
{
    d->m_keepAutoCompletionHighlight = keepHighlight;
}

void TextEditorWidget::setAutoCompleteSkipPosition(const QTextCursor &cursor)
{
    QTextCursor tc = cursor;
    // Create a selection of the next character but keep the current position, otherwise
    // the cursor would be removed from the list of automatically inserted text positions
    tc.movePosition(QTextCursor::NextCharacter);
    tc.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
    d->autocompleterHighlight(tc);
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

void TextEditorWidget::replace(int pos, int length, const QString &string)
{
    if (length == string.length()) {
        bool different = false;
        for (int i = 0; !different && i < length; ++i)
            different = document()->characterAt(pos + i) != string.at(i);
        if (!different)
            return;
    }

    QTextCursor tc = textCursor();
    tc.setPosition(pos);
    tc.setPosition(pos + length, QTextCursor::KeepAnchor);
    tc.insertText(string);
}

void BaseTextEditor::setCursorPosition(int pos)
{
    editorWidget()->setCursorPosition(pos);
}

void TextEditorWidget::setCursorPosition(int pos)
{
    QTextCursor tc = textCursor();
    tc.setPosition(pos);
    setTextCursor(tc);
}

QWidget *TextEditorWidget::toolBarWidget() const
{
    return d->m_toolBarWidget;
}

QToolBar *TextEditorWidget::toolBar() const
{
    return d->m_toolBar;
}

void BaseTextEditor::select(int toPos)
{
    QTextCursor tc = editorWidget()->textCursor();
    tc.setPosition(toPos, QTextCursor::KeepAnchor);
    editorWidget()->setTextCursor(tc);
}

void BaseTextEditor::saveCurrentStateForNavigationHistory()
{
    d->m_savedNavigationState = saveState();
}

void BaseTextEditor::addSavedStateToNavigationHistory()
{
    if (EditorManager::currentEditor() == this)
        EditorManager::addCurrentPositionToNavigationHistory(d->m_savedNavigationState);
}

void BaseTextEditor::addCurrentStateToNavigationHistory()
{
    if (EditorManager::currentEditor() == this)
        EditorManager::addCurrentPositionToNavigationHistory();
}

void TextEditorWidgetPrivate::updateCursorPosition()
{
    m_contextHelpItem = HelpItem();
    if (!q->textCursor().block().isVisible())
        q->ensureCursorVisible();
}

void TextEditorWidget::contextHelpItem(const IContext::HelpCallback &callback)
{
    if (!d->m_contextHelpItem.isEmpty()) {
        callback(d->m_contextHelpItem);
        return;
    }
    const QString fallbackWordUnderCursor = Text::wordUnderCursor(textCursor());
    const auto hoverHandlerCallback = [fallbackWordUnderCursor, callback](
            TextEditorWidget *widget, BaseHoverHandler *handler, int position) {
        handler->contextHelpId(widget, position,
                               [fallbackWordUnderCursor, callback](const HelpItem &item) {
            if (item.isEmpty())
                callback(fallbackWordUnderCursor);
            else
                callback(item);
        });

    };
    const auto fallback = [callback, fallbackWordUnderCursor](TextEditorWidget *) {
        callback(fallbackWordUnderCursor);
    };

    d->m_hoverHandlerRunner.startChecking(textCursor(), hoverHandlerCallback, fallback);
}

void TextEditorWidget::setContextHelpItem(const HelpItem &item)
{
    d->m_contextHelpItem = item;
}

RefactorMarkers TextEditorWidget::refactorMarkers() const
{
    return d->m_refactorOverlay->markers();
}

void TextEditorWidget::setRefactorMarkers(const RefactorMarkers &markers)
{
    const QList<RefactorMarker> oldMarkers = d->m_refactorOverlay->markers();
    for (const RefactorMarker &marker : oldMarkers)
        emit requestBlockUpdate(marker.cursor.block());
    d->m_refactorOverlay->setMarkers(markers);
    for (const RefactorMarker &marker : markers)
        emit requestBlockUpdate(marker.cursor.block());
}

void TextEditorWidget::setRefactorMarkers(const RefactorMarkers &newMarkers, const Utils::Id &type)
{
    RefactorMarkers markers = d->m_refactorOverlay->markers();
    auto first = std::partition(markers.begin(),
                                markers.end(),
                                [type](const RefactorMarker &marker) {
                                    return marker.type == type;
                                });

    for (auto it = markers.begin(); it != first; ++it)
        emit requestBlockUpdate(it->cursor.block());
    markers.erase(markers.begin(), first);
    markers.append(newMarkers);
    d->m_refactorOverlay->setMarkers(markers);
    for (const RefactorMarker &marker : newMarkers)
        emit requestBlockUpdate(marker.cursor.block());
}

void TextEditorWidget::clearRefactorMarkers(const Utils::Id &type)
{
    RefactorMarkers markers = d->m_refactorOverlay->markers();
    for (auto it = markers.begin(); it != markers.end();) {
        if (it->type == type) {
            emit requestBlockUpdate(it->cursor.block());
            it = markers.erase(it);
        } else {
            ++it;
        }
    }
    d->m_refactorOverlay->setMarkers(markers);
}

bool TextEditorWidget::inFindScope(const QTextCursor &cursor) const
{
    return d->m_find->inScope(cursor);
}

void TextEditorWidget::updateVisualWrapColumn()
{
    auto calcMargin = [this] {
        const auto &ms = d->m_marginSettings;

        if (!ms.m_showMargin) {
            return 0;
        }
        if (ms.m_useIndenter) {
            if (auto margin = d->m_document->indenter()->margin()) {
                return *margin;
            }
        }
        return ms.m_marginColumn;
    };
    setVisibleWrapColumn(calcMargin());
}

void TextEditorWidgetPrivate::updateTabStops()
{
    // Although the tab stop is stored as qreal the API from PlainTextEdit only allows it
    // to be set as an int. A work around is to access directly the QTextOption.
    QTextOption option = q->document()->defaultTextOption();
    option.setTabStopDistance(charWidth() * m_document->tabSettings().m_tabSize);
    q->document()->setDefaultTextOption(option);
    if (TextSuggestion *suggestion = TextBlockUserData::suggestion(m_suggestionBlock)) {
        QTextOption option = suggestion->replacementDocument()->defaultTextOption();
        option.setTabStopDistance(option.tabStopDistance());
        suggestion->replacementDocument()->setDefaultTextOption(option);
    }
}

void TextEditorWidgetPrivate::applyTabSettings()
{
    updateTabStops();
    m_autoCompleter->setTabSettings(m_document->tabSettings());
    emit q->tabSettingsChanged();
}

int TextEditorWidget::columnCount() const
{
    return int(viewport()->rect().width() / d->charWidth());
}

int TextEditorWidget::rowCount() const
{
    int height = viewport()->rect().height();
    int lineCount = 0;
    QTextBlock block = firstVisibleBlock();
    while (block.isValid()) {
        height -= blockBoundingRect(block).height();
        QTextLayout *blockLayout = editorLayout()->blockLayout(block);
        if (height < 0) {
            const int blockLineCount = blockLayout->lineCount();
            for (int i = 0; i < blockLineCount; ++i) {
                ++lineCount;
                const QTextLine line = blockLayout->lineAt(i);
                height += line.rect().height();
                if (height >= 0)
                    break;
            }
            return lineCount;
        }
        lineCount += blockLayout->lineCount();
        block = block.next();
    }
    return lineCount;
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
    MultiTextCursor cursor = m_cursors;
    cursor.beginEditBlock();
    for (QTextCursor &c : cursor) {
        int pos = c.position();
        int anchor = c.anchor();

        if (!c.hasSelection() && !m_cursors.hasMultipleCursors()) {
            // if nothing is selected, select the word under the cursor
            c.select(QTextCursor::WordUnderCursor);
        }

        QString text = c.selectedText();
        QString transformedText = method(text);

        if (transformedText == text)
            continue;

        c.insertText(transformedText);

        // (re)select the changed text
        // Note: this assumes the transformation did not change the length,
        c.setPosition(anchor);
        c.setPosition(pos, QTextCursor::KeepAnchor);
    }
    cursor.endEditBlock();
    q->setMultiTextCursor(cursor);
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
    for (int i = 0; i < count;) {
        if (!block.isValid() || i >= row)
            return block;

        i += editorLayout()->blockLineCount(block);
        block = d->nextVisibleBlock(block);
    }
    return QTextBlock();

}

QTextBlock TextEditorWidget::blockForVerticalOffset(int offset) const
{
    QTextBlock block = firstVisibleBlock();
    while (block.isValid()) {
        offset -= blockBoundingRect(block).height();
        if (offset < 0)
            return block;
        block = block.next();
    }
    return block;
}

void TextEditorWidget::invokeAssist(AssistKind kind, IAssistProvider *provider)
{
    if (multiTextCursor().hasMultipleCursors())
        return;

    if (kind == QuickFix && d->m_snippetOverlay->isVisible())
        d->m_snippetOverlay->accept();

    bool previousMode = overwriteMode();
    setOverwriteMode(false);
    ensureCursorVisible();
    d->m_codeAssistant.invoke(kind, provider);
    setOverwriteMode(previousMode);
}

std::unique_ptr<AssistInterface> TextEditorWidget::createAssistInterface(AssistKind kind,
                                                                         AssistReason reason) const
{
    Q_UNUSED(kind)
    return std::make_unique<AssistInterface>(textCursor(), d->m_document->filePath(), reason);
}

QString TextEditorWidget::foldReplacementText(const QTextBlock &) const
{
    return QLatin1String("...");
}

QByteArray BaseTextEditor::saveState() const
{
    return editorWidget()->saveState();
}

void BaseTextEditor::restoreState(const QByteArray &state)
{
    editorWidget()->restoreState(state);
}

BaseTextEditor *BaseTextEditor::currentTextEditor()
{
    return qobject_cast<BaseTextEditor *>(EditorManager::currentEditor());
}

QList<BaseTextEditor *> BaseTextEditor::openedTextEditors()
{
    return qobject_container_cast<BaseTextEditor *>(DocumentModel::editorsForOpenedDocuments());
}

QList<BaseTextEditor *> BaseTextEditor::textEditorsForDocument(TextDocument *doc)
{
    return qobject_container_cast<BaseTextEditor *>(DocumentModel::editorsForDocument(doc));
}

QList<BaseTextEditor *> BaseTextEditor::textEditorsForFilePath(const FilePath &path)
{
    return qobject_container_cast<BaseTextEditor *>(DocumentModel::editorsForFilePath(path));
}

TextEditorWidget *BaseTextEditor::editorWidget() const
{
    auto textEditorWidget = TextEditorWidget::fromEditor(this);
    QTC_CHECK(textEditorWidget);
    return textEditorWidget;
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
    const HighlighterHelper::Definitions definitions = HighlighterHelper::definitionsForDocument(textDocument());
    d->configureGenericHighlighter(definitions.isEmpty() ? HighlighterHelper::Definition()
                                                         : definitions.first());
    d->updateSyntaxInfoBar(definitions, textDocument()->filePath().fileName());
}

void TextEditorWidget::configureGenericHighlighter(const Utils::MimeType &mimeType)
{
    const HighlighterHelper::Definitions definitions = HighlighterHelper::definitionsForMimeType(mimeType.name());
    d->configureGenericHighlighter(definitions.isEmpty() ? HighlighterHelper::Definition()
                                                         : definitions.first());
    d->removeSyntaxInfoBar();
}

Result<> TextEditorWidget::configureGenericHighlighter(const QString &definitionName)
{
    const HighlighterHelper::Definition definition = TextEditor::HighlighterHelper::definitionForName(definitionName);
    if (!definition.isValid())
        return make_unexpected(Tr::tr("Could not find definition."));

    d->configureGenericHighlighter(definition);
    d->removeSyntaxInfoBar();
    return {};
}

int TextEditorWidget::blockNumberForVisibleRow(int row) const
{
    QTextBlock block = blockForVisibleRow(row);
    return block.isValid() ? block.blockNumber() : -1;
}

int TextEditorWidget::firstVisibleBlockNumber() const
{
    return blockNumberForVisibleRow(0);
}

int TextEditorWidget::lastVisibleBlockNumber() const
{
    QTextBlock block = blockForVerticalOffset(viewport()->height() - 1);
    if (!block.isValid()) {
        block = document()->lastBlock();
        while (block.isValid() && !block.isVisible())
            block = block.previous();
    }
    return block.isValid() ? block.blockNumber() : -1;
}

int TextEditorWidget::centerVisibleBlockNumber() const
{
    QTextBlock block = blockForVerticalOffset(viewport()->height() / 2);
    if (!block.isValid())
        block.previous();
    return block.isValid() ? block.blockNumber() : -1;
}

HighlightScrollBarController *TextEditorWidget::highlightScrollBarController() const
{
    return d->m_highlightScrollBarController;
}

// The remnants of PlainTextEditor.
void TextEditorWidget::setupGenericHighlighter()
{
    setLineSeparatorsAllowed(true);

    connect(textDocument(), &IDocument::filePathChanged,
            d.get(), &TextEditorWidgetPrivate::reconfigure);
}

//
// TextEditorLinkLabel
//
TextEditorLinkLabel::TextEditorLinkLabel(QWidget *parent)
    : Utils::ElidingLabel(parent)
{
    setElideMode(Qt::ElideMiddle);
}

void TextEditorLinkLabel::setLink(Utils::Link link)
{
    m_link = link;
}

Utils::Link TextEditorLinkLabel::link() const
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
    data->addFile(m_link.targetFilePath, m_link.target.line, m_link.target.column);
    auto drag = new QDrag(this);
    drag->setMimeData(data);
    drag->exec(Qt::CopyAction);
}

void TextEditorLinkLabel::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    if (!m_link.hasValidTarget())
        return;

    EditorManager::openEditorAt(m_link);
}

//
// BaseTextEditorFactory
//

namespace Internal {

class TextEditorFactoryPrivate
{
public:
    TextEditorFactoryPrivate(TextEditorFactory *parent)
        : q(parent)
        , m_widgetCreator([]() { return new TextEditorWidget; })
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
    CommentDefinition m_commentDefinition;
    QList<BaseHoverHandler *> m_hoverHandlers; // owned
    std::unique_ptr<CompletionAssistProvider> m_completionAssistProvider; // owned
    int m_optionalActionMask = 0;
    bool m_useGenericHighlighter = false;
    bool m_duplicatedSupported = true;
    bool m_codeFoldingSupported = false;
    bool m_paranthesesMatchinEnabled = false;
    bool m_marksVisible = true;
};

} /// namespace Internal

TextEditorFactory::TextEditorFactory()
    : d(new TextEditorFactoryPrivate(this))
{
    setEditorCreator([]() { return new BaseTextEditor; });
    addHoverHandler(new SuggestionHoverHandler);
}

TextEditorFactory::~TextEditorFactory()
{
    qDeleteAll(d->m_hoverHandlers);
    delete d;
}

void TextEditorFactory::setDocumentCreator(const DocumentCreator &creator)
{
    d->m_documentCreator = creator;
}

void TextEditorFactory::setEditorWidgetCreator(const EditorWidgetCreator &creator)
{
    d->m_widgetCreator = creator;
}

void TextEditorFactory::setEditorCreator(const EditorCreator &creator)
{
    d->m_editorCreator = creator;
    IEditorFactory::setEditorCreator([this] {
        static DocumentContentCompletionProvider basicSnippetProvider;
        TextDocumentPtr doc(d->m_documentCreator());

        if (d->m_indenterCreator)
            doc->setIndenter(d->m_indenterCreator(doc->document()));

        if (d->m_syntaxHighlighterCreator)
            doc->resetSyntaxHighlighter(d->m_syntaxHighlighterCreator);

        doc->setCompletionAssistProvider(d->m_completionAssistProvider
                                             ? d->m_completionAssistProvider.get()
                                             : &basicSnippetProvider);

        return d->createEditorHelper(doc);
    });
}

void TextEditorFactory::setIndenterCreator(const IndenterCreator &creator)
{
    d->m_indenterCreator = creator;
}

void TextEditorFactory::setSyntaxHighlighterCreator(const SyntaxHighLighterCreator &creator)
{
    d->m_syntaxHighlighterCreator = creator;
}

void TextEditorFactory::setUseGenericHighlighter(bool enabled)
{
    d->m_useGenericHighlighter = enabled;
}

void TextEditorFactory::setAutoCompleterCreator(const AutoCompleterCreator &creator)
{
    d->m_autoCompleterCreator = creator;
}

void TextEditorFactory::setOptionalActionMask(int optionalActions)
{
    d->m_optionalActionMask = optionalActions;
}

void TextEditorFactory::addHoverHandler(BaseHoverHandler *handler)
{
    d->m_hoverHandlers.append(handler);
}

void TextEditorFactory::setCompletionAssistProvider(CompletionAssistProvider *provider)
{
    d->m_completionAssistProvider.reset(provider);
}

void TextEditorFactory::setCommentDefinition(CommentDefinition definition)
{
    d->m_commentDefinition = definition;
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

BaseTextEditor *TextEditorFactoryPrivate::createEditorHelper(const TextDocumentPtr &document)
{
    QWidget *widget = m_widgetCreator();
    TextEditorWidget *textEditorWidget = Aggregation::query<TextEditorWidget>(widget);
    QTC_ASSERT(textEditorWidget, return nullptr);
    textEditorWidget->setMarksVisible(m_marksVisible);
    textEditorWidget->setParenthesesMatchingEnabled(m_paranthesesMatchinEnabled);
    textEditorWidget->setCodeFoldingSupported(m_codeFoldingSupported);
    textEditorWidget->setOptionalActions(m_optionalActionMask);

    BaseTextEditor *editor = m_editorCreator();
    editor->setDuplicateSupported(m_duplicatedSupported);
    editor->addContext(q->id());
    editor->d->m_origin = this;

    editor->m_widget = widget;

    // Needs to go before setTextDocument as this copies the current settings.
    if (m_autoCompleterCreator)
        textEditorWidget->setAutoCompleter(m_autoCompleterCreator());

    textEditorWidget->setTextDocument(document);
    textEditorWidget->autoCompleter()->setTabSettings(document->tabSettings());
    textEditorWidget->d->m_hoverHandlers = m_hoverHandlers;

    textEditorWidget->d->m_commentDefinition = m_commentDefinition;
    textEditorWidget->d->m_commentDefinition.isAfterWhitespace
        = document->typingSettings().m_commentPosition != TypingSettings::StartOfLine;

    QObject::connect(textEditorWidget,
                     &TextEditorWidget::activateEditor,
                     textEditorWidget,
                     [editor](EditorManager::OpenEditorFlags flags) {
                         EditorManager::activateEditor(editor, flags);
                     });
    QObject::connect(
        textEditorWidget,
        &TextEditorWidget::saveCurrentStateForNavigationHistory,
        editor,
        &BaseTextEditor::saveCurrentStateForNavigationHistory);
    QObject::connect(
        textEditorWidget,
        &TextEditorWidget::addSavedStateToNavigationHistory,
        editor,
        &BaseTextEditor::addSavedStateToNavigationHistory);
    QObject::connect(
        textEditorWidget,
        &TextEditorWidget::addCurrentStateToNavigationHistory,
        editor,
        &BaseTextEditor::addCurrentStateToNavigationHistory);

    if (m_useGenericHighlighter)
        textEditorWidget->setupGenericHighlighter();
    textEditorWidget->finalizeInitialization();

    // Toolbar: Actions to show minimized info bars
    document->minimizableInfoBars()->createShowInfoBarActions([textEditorWidget](QWidget *w) {
        return textEditorWidget->insertExtraToolBarWidget(TextEditorWidget::Left, w);
    });

    editor->finalizeInitialization();
    return editor;
}

BaseTextEditor *BaseTextEditor::duplicate()
{
    // Use new standard setup if that's available.
    if (d->m_origin) {
        BaseTextEditor *dup = d->m_origin->duplicateTextEditor(this);
        emit editorDuplicated(dup);
        return dup;
    }

    // If neither is sufficient, you need to implement 'YourEditor::duplicate'.
    QTC_CHECK(false);
    return nullptr;
}

} // namespace TextEditor

QT_BEGIN_NAMESPACE

size_t qHash(const QColor &color)
{
    return color.rgba();
}

QT_END_NAMESPACE

#include "texteditor.moc"
