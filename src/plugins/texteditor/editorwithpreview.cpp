// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "editorwithpreview.h"

#include "textdocument.h"
#include "texteditortr.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/basetextfind.h>
#include <coreplugin/icore.h>
#include <coreplugin/minisplitter.h>

#include <utils/aggregate.h>
#include <utils/qtcassert.h>
#include <utils/qtcsettings.h>

#include <QElapsedTimer>
#include <QScrollBar>
#include <QTextBrowser>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

#include <optional>

using namespace Core;
using namespace Utils;

namespace TextEditor {

namespace Internal {

const bool kTextEditorRightDefault = false;
const bool kShowEditorDefault = true;
const bool kShowPreviewDefault = true;
const int kMinimumUpdateInterval = 120;
const int kMaximumUpdateInterval = 1000;
const int kStateVersion = 1;

class Ids
{
public:
    explicit Ids(const QByteArray &prefix)
        : editorId(EditorWithPreview::editorId(prefix))
        , textContext(EditorWithPreview::textContextId(prefix))
        , previewContext(editorId.withSuffix(".Preview"))
        , toggleEditorAction(Id::fromName(prefix).withSuffix(".ToggleEditor"))
        , togglePreviewAction(Id::fromName(prefix).withSuffix(".TogglePreview"))
        , swapViewsAction(Id::fromName(prefix).withSuffix(".SwapViews"))
        , showEditorKey(prefix + ".ShowEditor")
        , showPreviewKey(prefix + ".ShowPreview")
        , textEditorRightKey(prefix + ".TextEditorRight")
    {}

    Id editorId;
    Id textContext;
    Id previewContext;
    Id toggleEditorAction;
    Id togglePreviewAction;
    Id swapViewsAction;
    Key showEditorKey;
    Key showPreviewKey;
    Key textEditorRightKey;
};

class EditorWithPreviewPrivate
{
public:
    struct MirroredHighlight
    {
        BaseTextFindBase *find = nullptr;
        QWidget *view = nullptr;
        std::optional<QString> pendingText;
        FindFlags pendingFlags;
    };

    EditorWithPreviewPrivate(
        const TextDocumentPtr &document,
        TextEditorWidget *textEditorWidget,
        QTextBrowser *previewWidget,
        const QByteArray &idPrefix)
        : m_ids(idPrefix)
        , m_document(document)
        , m_textEditorWidget(textEditorWidget)
        , m_previewWidget(previewWidget)
    {}

    bool isTextEditorRight() const { return m_splitter->widget(0) == m_previewWidget; }
    void setWidgetOrder(bool textEditorRight);
    void viewToggled(QWidget *view, bool visible, QWidget *otherView, QToolButton *otherButton);
    void saveViewSettings();
    void mirrorHighlights(MirroredHighlight &target, const QString &txt, FindFlags findFlags);
    void flushHighlights(MirroredHighlight &target);

    Ids m_ids;
    QTimer m_previewTimer;
    bool m_previewOutdated = true;
    MiniSplitter *m_splitter = nullptr;
    TextDocumentPtr m_document;
    TextEditorWidget *m_textEditorWidget = nullptr;
    QTextBrowser *m_previewWidget = nullptr;
    QList<QAction *> m_editorToolBarActions;
    QToolButton *m_toggleEditorVisible = nullptr;
    QToolButton *m_togglePreviewVisible = nullptr;
    QToolButton *m_swapViews = nullptr;
    QAction *m_toggleEditorVisibleAction = nullptr;
    QAction *m_togglePreviewVisibleAction = nullptr;
    QAction *m_swapViewsAction = nullptr;
    std::optional<QPoint> m_previewRestoreScrollPosition;
    QByteArray m_savedNavigationState;
    MirroredHighlight m_editorHighlight;
    MirroredHighlight m_previewHighlight;
    bool m_blockMirrorHighlights = false;
};

void EditorWithPreviewPrivate::setWidgetOrder(bool textEditorRight)
{
    QTC_ASSERT(m_splitter->count() > 1, return);
    QWidget *left = textEditorRight ? static_cast<QWidget *>(m_previewWidget)
                                    : m_textEditorWidget;
    QWidget *right = textEditorRight ? static_cast<QWidget *>(m_textEditorWidget)
                                     : m_previewWidget;
    m_splitter->insertWidget(0, left);
    m_splitter->insertWidget(1, right);
    // buttons
    QAction *leftAction = textEditorRight ? m_togglePreviewVisibleAction
                                          : m_toggleEditorVisibleAction;
    QAction *rightAction = textEditorRight ? m_toggleEditorVisibleAction
                                           : m_togglePreviewVisibleAction;
    m_textEditorWidget->toolBar()->insertAction(m_swapViewsAction, leftAction);
    m_textEditorWidget->toolBar()->insertAction(m_swapViewsAction, rightAction);
}

void EditorWithPreviewPrivate::viewToggled(QWidget *view,
                                           bool visible,
                                           QWidget *otherView,
                                           QToolButton *otherButton)
{
    if (view->isVisible() == visible)
        return;
    view->setVisible(visible);
    if (visible) {
        view->setFocus();
    } else if (otherView->isVisible()) {
        otherView->setFocus();
    } else {
        // make sure at least one view is visible
        otherButton->toggle();
    }
    m_swapViews->setEnabled(view->isVisible() && otherView->isVisible());
}

void EditorWithPreviewPrivate::saveViewSettings()
{
    QtcSettings *s = ICore::settings();
    s->setValueWithDefault(m_ids.showPreviewKey,
                           m_togglePreviewVisible->isChecked(),
                           kShowPreviewDefault);
    s->setValueWithDefault(m_ids.showEditorKey,
                           m_toggleEditorVisible->isChecked(),
                           kShowEditorDefault);
}

void EditorWithPreviewPrivate::mirrorHighlights(MirroredHighlight &target,
                                                const QString &txt,
                                                FindFlags findFlags)
{
    if (m_blockMirrorHighlights)
        return;
    target.pendingText = txt;
    target.pendingFlags = findFlags;
    if (target.view->isVisible())
        flushHighlights(target);
}

void EditorWithPreviewPrivate::flushHighlights(MirroredHighlight &target)
{
    if (!target.pendingText)
        return;
    const QString txt = *std::exchange(target.pendingText, std::nullopt);
    m_blockMirrorHighlights = true;
    target.find->highlightAll(txt, target.pendingFlags);
    m_blockMirrorHighlights = false;
}

} // namespace Internal

/*!
    \class TextEditor::EditorWithPreview
    \inmodule QtCreator

    \brief The EditorWithPreview class is the base class for editors that show
    a text editor and a rendered preview of the document side by side.

    Subclasses implement renderPreview() to set the contents of the preview
    widget. The views can be shown, hidden and swapped by the user, and the
    preview is only rendered while it is visible.

    Use EditorWithPreviewActions in the editor factory to register the actions
    for the ID prefix that is passed to the editor.
*/

/*!
    \fn void TextEditor::EditorWithPreview::renderPreview()

    Sets the contents of previewWidget() from the document. The scroll position
    of the preview is kept by the caller.
*/

/*!
    Creates an editor for \a document with the text editor \a textEditorWidget
    and the preview \a previewWidget side by side.

    The IDs of the editor, its contexts and actions, and its settings keys are
    derived from \a idPrefix. For the prefix \c Foo, the editor ID is
    \c Editors.Foo, the contexts of the text editor and the preview are
    \c Editors.Foo.Text and \c Editors.Foo.Preview, the actions are
    \c Foo.ToggleEditor, \c Foo.TogglePreview and \c Foo.SwapViews, and the
    settings keys are \c Foo.ShowEditor, \c Foo.ShowPreview and
    \c Foo.TextEditorRight.
*/
EditorWithPreview::EditorWithPreview(
    const TextDocumentPtr &document,
    TextEditorWidget *textEditorWidget,
    QTextBrowser *previewWidget,
    const QByteArray &idPrefix)
    : d(new Internal::EditorWithPreviewPrivate(document, textEditorWidget, previewWidget, idPrefix))
{
    using namespace Internal;

    const Ids &ids = d->m_ids;

    setDuplicateSupported(true);

    QtcSettings *s = ICore::settings();
    const bool textEditorRight
        = s->value(ids.textEditorRightKey, kTextEditorRightDefault).toBool();
    const bool showPreview = s->value(ids.showPreviewKey, kShowPreviewDefault).toBool();
    const bool showEditor = s->value(ids.showEditorKey, kShowEditorDefault).toBool()
                            || !showPreview; // ensure at least one is visible

    d->m_splitter = new MiniSplitter;

    // preview
    auto previewFind = new BaseTextFind<QTextBrowser>(previewWidget);
    Aggregation::aggregate({previewWidget, previewFind});
    IContext::attach(previewWidget, Context(ids.previewContext));

    // editor
    textEditorWidget->setTextDocument(document);
    connect(textEditorWidget, &TextEditorWidget::saveCurrentStateForNavigationHistory,
            this, [this] { d->m_savedNavigationState = saveState(); });
    connect(textEditorWidget, &TextEditorWidget::addSavedStateToNavigationHistory,
            this, [this] {
                EditorManager::addCurrentPositionToNavigationHistory(
                    d->m_savedNavigationState);
            });
    connect(textEditorWidget, &TextEditorWidget::addCurrentStateToNavigationHistory,
            this, [] { EditorManager::addCurrentPositionToNavigationHistory(); });
    IContext::attach(textEditorWidget, Context(ids.textContext));

    // synchronize highlighting of search results between the two views
    auto textFind = Aggregation::query<BaseTextFindBase>(textEditorWidget);
    if (QTC_GUARD(textFind)) {
        d->m_editorHighlight.find = textFind;
        d->m_editorHighlight.view = textEditorWidget;
        d->m_previewHighlight.find = previewFind;
        d->m_previewHighlight.view = previewWidget;
        connect(textFind, &BaseTextFindBase::highlightAllRequested,
                this, [this](const QString &txt, FindFlags findFlags) {
                    d->mirrorHighlights(d->m_previewHighlight, txt, findFlags);
                });
        connect(previewFind, &BaseTextFindBase::highlightAllRequested,
                this, [this](const QString &txt, FindFlags findFlags) {
                    d->mirrorHighlights(d->m_editorHighlight, txt, findFlags);
                });
    }

    d->m_splitter->addWidget(textEditorWidget); // sets splitter->focusWidget() on non-Windows
    d->m_splitter->addWidget(previewWidget);

    // Ignore size hints and just distribute equally 50%/50%
    previewWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    textEditorWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    d->m_splitter->setSizes({1, 1});

    setContext(Context(ids.editorId));

    auto widget = new QWidget;
    auto layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(d->m_splitter);
    setWidget(widget);
    m_widget->installEventFilter(this);
    using namespace Aggregation;
    Aggregate *agg = Aggregate::parentAggregate(textEditorWidget);
    if (!agg) {
        agg = new Aggregate;
        agg->add(textEditorWidget);
    }
    agg->add(m_widget.get());

    d->m_togglePreviewVisible = Command::createToolButtonWithShortcutToolTip(
        ids.togglePreviewAction);
    d->m_togglePreviewVisible->defaultAction()->setCheckable(true);
    d->m_togglePreviewVisible->setChecked(showPreview);
    previewWidget->setVisible(showPreview);

    d->m_toggleEditorVisible = Command::createToolButtonWithShortcutToolTip(
        ids.toggleEditorAction);
    d->m_toggleEditorVisible->defaultAction()->setCheckable(true);
    d->m_toggleEditorVisible->setChecked(showEditor);
    textEditorWidget->setVisible(showEditor);

    d->m_swapViews = Command::createToolButtonWithShortcutToolTip(ids.swapViewsAction);
    d->m_swapViews->setEnabled(showEditor && showPreview);

    d->m_swapViewsAction = textEditorWidget->insertExtraToolBarWidget(TextEditorWidget::Right,
                                                                      d->m_swapViews);
    d->m_toggleEditorVisibleAction
        = textEditorWidget->insertExtraToolBarWidget(TextEditorWidget::Right,
                                                     d->m_toggleEditorVisible);
    d->m_togglePreviewVisibleAction
        = textEditorWidget->insertExtraToolBarWidget(TextEditorWidget::Right,
                                                     d->m_togglePreviewVisible);
    d->setWidgetOrder(textEditorRight);

    connect(d->m_toggleEditorVisible, &QToolButton::toggled, this, [this](bool visible) {
        d->viewToggled(d->m_textEditorWidget, visible, d->m_previewWidget,
                       d->m_togglePreviewVisible);
        for (QAction *action : std::as_const(d->m_editorToolBarActions))
            action->setVisible(visible);
        if (visible)
            d->flushHighlights(d->m_editorHighlight);
        d->saveViewSettings();
    });
    connect(d->m_togglePreviewVisible, &QToolButton::toggled, this, [this](bool visible) {
        d->viewToggled(d->m_previewWidget, visible, d->m_textEditorWidget,
                       d->m_toggleEditorVisible);
        if (visible && d->m_previewOutdated)
            updatePreviewNow();
        if (visible)
            d->flushHighlights(d->m_previewHighlight);
        d->saveViewSettings();
    });
    connect(d->m_swapViews, &QToolButton::clicked, textEditorWidget, [this] {
        const bool textEditorRight = !d->isTextEditorRight();
        d->setWidgetOrder(textEditorRight);
        ICore::settings()->setValueWithDefault(d->m_ids.textEditorRightKey,
                                               textEditorRight,
                                               kTextEditorRightDefault);
    });

    d->m_previewTimer.setInterval(kMinimumUpdateInterval);
    d->m_previewTimer.setSingleShot(true);
    connect(&d->m_previewTimer, &QTimer::timeout, this, &EditorWithPreview::updatePreview);
    // Started, not restarted: the preview follows along while typing continues,
    // instead of waiting for a pause in it.
    connect(document->document(), &QTextDocument::contentsChanged, &d->m_previewTimer, [this] {
        if (!d->m_previewTimer.isActive())
            d->m_previewTimer.start();
    });
}

EditorWithPreview::~EditorWithPreview()
{
    delete d;
}

/*!
    Returns the ID of the editor for \a idPrefix, which is also its context.
*/
Id EditorWithPreview::editorId(const QByteArray &idPrefix)
{
    return Id("Editors.").withSuffix(idPrefix.constData());
}

/*!
    Returns the context of the text editor for \a idPrefix.
*/
Id EditorWithPreview::textContextId(const QByteArray &idPrefix)
{
    return editorId(idPrefix).withSuffix(".Text");
}

/*!
    Renders the preview if it is visible, and otherwise when it is shown the
    next time.
*/
void EditorWithPreview::updatePreview()
{
    if (d->m_togglePreviewVisible->isChecked())
        updatePreviewNow();
    else
        d->m_previewOutdated = true;
}

/*!
    Renders the preview, regardless of whether it is visible.
*/
void EditorWithPreview::updatePreviewNow()
{
    d->m_previewOutdated = false;
    QElapsedTimer duration;
    duration.start();

    const QPoint positions = d->m_previewRestoreScrollPosition
                                 ? *d->m_previewRestoreScrollPosition
                                 : QPoint(d->m_previewWidget->horizontalScrollBar()->value(),
                                          d->m_previewWidget->verticalScrollBar()->value());
    d->m_previewRestoreScrollPosition.reset();

    renderPreview();

    d->m_previewWidget->horizontalScrollBar()->setValue(positions.x());
    d->m_previewWidget->verticalScrollBar()->setValue(positions.y());

    // A document that is expensive to render throttles itself, so typing in it
    // stays as responsive as the document allows.
    d->m_previewTimer.setInterval(qBound(Internal::kMinimumUpdateInterval,
                                         int(duration.elapsed()) * 3,
                                         Internal::kMaximumUpdateInterval));
}

void EditorWithPreview::toggleEditor()
{
    d->m_toggleEditorVisible->toggle();
}

void EditorWithPreview::togglePreview()
{
    d->m_togglePreviewVisible->toggle();
}

void EditorWithPreview::swapViews()
{
    d->m_swapViews->click();
}

void EditorWithPreview::increasePreviewZoom()
{
    d->m_previewWidget->zoomIn();
}

void EditorWithPreview::decreasePreviewZoom()
{
    d->m_previewWidget->zoomOut();
}

void EditorWithPreview::resetPreviewZoom()
{
    d->m_previewWidget->setFont({});
}

void EditorWithPreview::gotoLine(int line, int column, bool centerLine)
{
    if (!d->m_toggleEditorVisible->isChecked())
        d->m_toggleEditorVisible->toggle();
    d->m_textEditorWidget->gotoLine(line, column, centerLine);
}

QByteArray EditorWithPreview::saveState() const
{
    QByteArray state;
    QDataStream stream(&state, QIODevice::WriteOnly);
    stream << Internal::kStateVersion;
    stream << d->m_textEditorWidget->saveState();
    stream << d->m_previewWidget->horizontalScrollBar()->value();
    stream << d->m_previewWidget->verticalScrollBar()->value();
    stream << d->isTextEditorRight();
    stream << d->m_togglePreviewVisible->isChecked();
    stream << d->m_toggleEditorVisible->isChecked();
    stream << d->m_splitter->saveState();
    return state;
}

void EditorWithPreview::restoreState(const QByteArray &state)
{
    if (state.isEmpty())
        return;
    int version = 0;
    QByteArray editorState;
    int previewHV = 0;
    int previewVV = 0;
    bool textEditorRight = false;
    bool previewShown = true;
    bool textEditorShown = true;
    QByteArray splitterState;
    QDataStream stream(state);
    stream >> version;
    if (version != Internal::kStateVersion) {
        // Saved by a different editor for this document, for example when the document
        // was opened with a plain text editor before: it is the text editor's own state,
        // so the text editor takes it and the preview keeps its settings.
        d->m_textEditorWidget->restoreState(state);
        return;
    }
    stream >> editorState;
    stream >> previewHV;
    stream >> previewVV;
    stream >> textEditorRight;
    stream >> previewShown;
    stream >> textEditorShown;
    stream >> splitterState;
    if (stream.status() != QDataStream::Ok)
        return;
    d->m_textEditorWidget->restoreState(editorState);
    d->m_previewRestoreScrollPosition.emplace(previewHV, previewVV);
    d->setWidgetOrder(textEditorRight);
    d->m_splitter->restoreState(splitterState);
    d->m_togglePreviewVisible->setChecked(previewShown);
    d->m_previewWidget->setVisible(previewShown);
    // ensure at least one is shown
    d->m_toggleEditorVisible->setChecked(textEditorShown || !previewShown);
    d->m_textEditorWidget->setVisible(d->m_toggleEditorVisible->isChecked());
    // A pending render applies the scroll position, otherwise the preview is up to date
    // and only needs to be scrolled.
    if (d->m_previewTimer.isActive())
        return;
    if (d->m_previewOutdated) {
        updatePreview();
    } else if (d->m_previewRestoreScrollPosition) {
        const QPoint position = *std::exchange(d->m_previewRestoreScrollPosition, std::nullopt);
        d->m_previewWidget->horizontalScrollBar()->setValue(position.x());
        d->m_previewWidget->verticalScrollBar()->setValue(position.y());
    }
}

bool EditorWithPreview::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == m_widget && ev->type() == QEvent::FocusIn) {
        if (d->m_splitter->focusWidget())
            d->m_splitter->focusWidget()->setFocus();
        else if (d->m_textEditorWidget->isVisible())
            d->m_textEditorWidget->setFocus();
        else
            d->m_splitter->widget(0)->setFocus();
        return true;
    }
    return BaseTextEditor::eventFilter(obj, ev);
}

/*!
    Inserts \a widget into the tool bar of the text editor at \a side. The
    widget is only shown while the text editor is visible.
*/
QAction *EditorWithPreview::insertEditorToolBarWidget(TextEditorWidget::Side side,
                                                      QWidget *widget)
{
    QAction *action = d->m_textEditorWidget->insertExtraToolBarWidget(side, widget);
    action->setVisible(d->m_toggleEditorVisible->isChecked());
    d->m_editorToolBarActions.append(action);
    return action;
}

TextDocumentPtr EditorWithPreview::textDocumentPtr() const
{
    return d->m_document;
}

TextEditorWidget *EditorWithPreview::textEditorWidget() const
{
    return d->m_textEditorWidget;
}

QTextBrowser *EditorWithPreview::previewWidget() const
{
    return d->m_previewWidget;
}

/*!
    \class TextEditor::EditorWithPreviewActions
    \inmodule QtCreator

    \brief The EditorWithPreviewActions class registers the actions for showing,
    hiding and swapping the views of an EditorWithPreview, and for zooming its
    preview.

    Create one instance in the editor factory, with the same ID prefix that is
    passed to the editors.
*/

EditorWithPreviewActions::EditorWithPreviewActions(const QByteArray &idPrefix)
{
    const Internal::Ids ids(idPrefix);
    const auto onCurrentEditor = [](void (EditorWithPreview::*function)()) {
        return [function] {
            if (auto editor = qobject_cast<EditorWithPreview *>(EditorManager::currentEditor()))
                (editor->*function)();
        };
    };
    const Context context(ids.editorId);
    const Context previewContext(ids.previewContext);

    ActionBuilder(nullptr, ids.toggleEditorAction)
        .adopt(&m_toggleEditorAction)
        .setText(Tr::tr("Show Editor"))
        .setContext(context)
        .addOnTriggered(EditorManager::instance(),
                        onCurrentEditor(&EditorWithPreview::toggleEditor));

    ActionBuilder(nullptr, ids.togglePreviewAction)
        .adopt(&m_togglePreviewAction)
        .setText(Tr::tr("Show Preview"))
        .setContext(context)
        .addOnTriggered(EditorManager::instance(),
                        onCurrentEditor(&EditorWithPreview::togglePreview));

    ActionBuilder(nullptr, ids.swapViewsAction)
        .adopt(&m_swapViewsAction)
        .setText(Tr::tr("Swap Views"))
        .setContext(context)
        .addOnTriggered(EditorManager::instance(),
                        onCurrentEditor(&EditorWithPreview::swapViews));

    ActionBuilder(nullptr, Core::Constants::ZOOM_IN)
        .adopt(&m_previewZoomInAction)
        .setContext(previewContext)
        .addOnTriggered(EditorManager::instance(),
                        onCurrentEditor(&EditorWithPreview::increasePreviewZoom));

    ActionBuilder(nullptr, Core::Constants::ZOOM_OUT)
        .adopt(&m_previewZoomOutAction)
        .setContext(previewContext)
        .addOnTriggered(EditorManager::instance(),
                        onCurrentEditor(&EditorWithPreview::decreasePreviewZoom));

    ActionBuilder(nullptr, Core::Constants::ZOOM_RESET)
        .adopt(&m_previewZoomResetAction)
        .setContext(previewContext)
        .addOnTriggered(EditorManager::instance(),
                        onCurrentEditor(&EditorWithPreview::resetPreviewZoom));
}

} // namespace TextEditor
