// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "editorview.h"

#include "../editormanager/ieditor.h"
#include "../editortoolbar.h"
#include "../findplaceholder.h"
#include "../generalsettings.h"
#include "../minisplitter.h"
#include "documentmodel.h"
#include "documentmodel_p.h"
#include "editormanager.h"
#include "editormanager_p.h"

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/infobar.h>
#include <utils/layoutbuilder.h>
#include <utils/link.h>
#include <utils/overlaywidget.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QSplitter>
#include <QStackedLayout>
#include <QStackedWidget>
#include <QStylePainter>
#include <QTabBar>
#include <QToolButton>

using namespace Core;
using namespace Utils;

namespace Core::Internal {

// EditorView

static void updateTabText(QTabBar *tabBar, int index, IDocument *document)
{
    QTC_ASSERT(index >= 0 && index < tabBar->count(), return);
    const auto data = tabBar->tabData(index).value<EditorView::TabData>();
    QString title = document->displayName();
    if (document->isModified())
        title += '*';
    if (qtcEnvironmentVariableIsSet("QTC_DEBUG_DOCUMENTMODEL") && !data.editor)
        title += " (s)";
    tabBar->setTabText(index, title);
    tabBar->setTabToolTip(index, document->toolTip());
}

EditorView::EditorView(SplitterOrView *parentSplitterOrView, QWidget *parent)
    : QWidget(parent)
    , m_parentSplitterOrView(parentSplitterOrView)
    , m_toolBar(new EditorToolBar(this))
    , m_tabBar(new QTabBar(this))
    , m_container(new QStackedWidget(this))
    , m_infoBarDisplay(new InfoBarDisplay(this))
    , m_statusHLine(Layouting::createHr(this))
    , m_statusWidget(new QFrame(this))
    , m_backMenu(new QMenu(this))
    , m_forwardMenu(new QMenu(this))
{
    auto tl = new QVBoxLayout(this);
    tl->setSpacing(0);
    tl->setContentsMargins(0, 0, 0, 0);
    {
        connect(m_toolBar, &EditorToolBar::goBackClicked,
                this, &EditorView::goBackInNavigationHistory);
        connect(m_toolBar, &EditorToolBar::goForwardClicked,
                this, &EditorView::goForwardInNavigationHistory);
        connect(m_toolBar, &EditorToolBar::closeClicked, this, &EditorView::closeCurrentEditor);
        connect(m_toolBar, &EditorToolBar::listSelectionActivated,
                this, &EditorView::listSelectionActivated);
        connect(m_toolBar, &EditorToolBar::currentDocumentMoved,
                this, &EditorView::closeCurrentEditor);
        connect(m_toolBar, &EditorToolBar::horizontalSplitClicked,
                this, &EditorView::splitHorizontally);
        connect(m_toolBar, &EditorToolBar::verticalSplitClicked, this, &EditorView::splitVertically);
        connect(m_toolBar, &EditorToolBar::splitNewWindowClicked, this, &EditorView::splitNewWindow);
        connect(m_toolBar, &EditorToolBar::closeSplitClicked, this, &EditorView::closeSplit);
        m_toolBar->setMenuProvider([this](QMenu *menu) { fillListContextMenu(menu); });
        m_toolBar->setGoBackMenu(m_backMenu);
        m_toolBar->setGoForwardMenu(m_forwardMenu);
        tl->addWidget(m_toolBar);
    }

    m_tabBar->setVisible(false);
    m_tabBar->setDocumentMode(true);
    m_tabBar->setUsesScrollButtons(true);
    m_tabBar->setExpanding(false);
    m_tabBar->setMovable(true);
    m_tabBar->setTabsClosable(true);
    m_tabBar->setElideMode(Qt::ElideNone);
    m_tabBar->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tabBar->setShape(QTabBar::RoundedNorth);
    m_tabBar->installEventFilter(this);
    connect(m_tabBar, &QTabBar::tabBarClicked, this, &EditorView::activateTab);
    connect(
        m_tabBar,
        &QTabBar::tabCloseRequested,
        this,
        [this](int index) { closeTab(index); },
        Qt::QueuedConnection /* do not modify tab bar in tab bar signal */);
    connect(
        m_tabBar,
        &QWidget::customContextMenuRequested,
        m_tabBar,
        [this](const QPoint &pos) {
            const int index = m_tabBar->tabAt(pos);
            if (index < 0 || index >= m_tabBar->count())
                return;
            const auto data = m_tabBar->tabData(index).value<TabData>();
            QMenu menu;
            EditorManagerPrivate::addContextMenuActions(&menu, data.entry, data.editor, this);
            menu.exec(m_tabBar->mapToGlobal(pos));
        },
        Qt::QueuedConnection);
    // We cannot watch for IDocument changes, because the tab might refer
    // to a suspended document. And if a new editor for that is opened in another view,
    // this view will not know about that.
    connect(
        DocumentModel::model(),
        &QAbstractItemModel::dataChanged,
        m_tabBar,
        [this](const QModelIndex &topLeft, const QModelIndex &bottomRight) {
            for (int i = topLeft.row(); i <= bottomRight.row(); ++i) {
                DocumentModel::Entry *e = DocumentModel::entryAtRow(i);
                const int tabIndex = tabForEntry(e);
                if (tabIndex >= 0)
                    updateTabText(m_tabBar, tabIndex, e->document);
            }
        });
    // Watch for items that are removed from the document model, e.g. suspended items
    // when the user closes one in Open Documents view, which we otherwise not get notified for
    connect(
        DocumentModel::model(),
        &QAbstractItemModel::rowsAboutToBeRemoved,
        m_tabBar,
        [this](const QModelIndex & /*parent*/, int first, int last) {
            for (int i = first; i <= last; ++i) {
                DocumentModel::Entry *e = DocumentModel::entryAtRow(i);
                const int tabIndex = tabForEntry(e);
                if (tabIndex >= 0) {
                    const auto data = m_tabBar->tabData(tabIndex).value<TabData>();
                    // for editors we get a call of removeEditor, but not for suspended tabs
                    if (!data.editor)
                        m_tabBar->removeTab(tabIndex);
                }
            }
        });
    tl->addWidget(m_tabBar);

    m_infoBarDisplay->setTarget(tl, 2);

    tl->addWidget(m_container);

    tl->addWidget(new FindToolBarPlaceHolder(this));

    {
        m_statusWidget->setFrameStyle(QFrame::NoFrame);
        m_statusWidget->setLineWidth(0);
        m_statusWidget->setAutoFillBackground(true);

        auto hbox = new QHBoxLayout(m_statusWidget);
        hbox->setContentsMargins(1, 0, 1, 1);
        m_statusWidgetLabel = new QLabel;
        m_statusWidgetLabel->setContentsMargins(3, 0, 3, 0);
        hbox->addWidget(m_statusWidgetLabel);
        hbox->addStretch(1);

        m_statusWidgetButton = new QToolButton;
        m_statusWidgetButton->setContentsMargins(0, 0, 0, 0);
        hbox->addWidget(m_statusWidgetButton);

        m_statusHLine->setVisible(false);
        m_statusWidget->setVisible(false);
        tl->addWidget(m_statusHLine);
        tl->addWidget(m_statusWidget);
    }

    // for the case of no document selected
    auto empty = new QWidget;
    empty->hide();
    auto emptyLayout = new QGridLayout(empty);
    empty->setLayout(emptyLayout);
    m_emptyViewLabel = new QLabel;
    connect(EditorManagerPrivate::instance(), &EditorManagerPrivate::placeholderTextChanged,
            m_emptyViewLabel, &QLabel::setText);
    m_emptyViewLabel->setText(EditorManagerPrivate::placeholderText());
    emptyLayout->addWidget(m_emptyViewLabel);
    m_container->addWidget(empty);
    m_widgetEditorMap.insert(empty, nullptr);

    const auto dropSupport = new DropSupport(this, [this](QDropEvent *event, DropSupport *) {
        // do not accept move events except from other editor views (i.e. their tool bars)
        // otherwise e.g. item views that support moving items within themselves would
        // also "move" the item into the editor view, i.e. the item would be removed from the
        // item view
        if (!qobject_cast<EditorToolBar*>(event->source()))
            event->setDropAction(Qt::CopyAction);
        if (event->type() == QDropEvent::DragEnter && !DropSupport::isFileDrop(event))
            return false; // do not accept drops without files
        return event->source() != m_toolBar; // do not accept drops on ourselves
    });
    connect(dropSupport, &DropSupport::filesDropped,
            this, &EditorView::openDroppedFiles);

    updateNavigatorActions();

    auto currentViewOverlay = new OverlayWidget;
    currentViewOverlay->attachToWidget(this);
    currentViewOverlay->setAttribute(Qt::WA_OpaquePaintEvent);
    currentViewOverlay->setResizeFunction([this](QWidget *w, const QSize &) {
        const QRect toolbarRect = m_toolBar->geometry();
        w->setGeometry(toolbarRect.x(), toolbarRect.bottom() - 1, toolbarRect.width(),
                       StyleHelper::HighlightThickness);
    });
    currentViewOverlay->setPaintFunction([](QWidget *w, QPainter &p, QPaintEvent *) {
        QColor viewHighlight = w->palette().color(QPalette::Highlight);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        // Themes that use the application accent color on the mode bar do it on the view, too.
        const QColor modeHighlight = creatorColor(Theme::FancyToolButtonHighlightColor);
        const QColor paletteAccent = w->palette().color(QPalette::Accent);
        if (modeHighlight == paletteAccent)
            viewHighlight = paletteAccent;
#endif // >= Qt 6.6.0
        p.fillRect(w->rect(), viewHighlight);
    });
    currentViewOverlay->setVisible(false);
    const auto updateCurrentViewOverlay = [this, currentViewOverlay] {
        currentViewOverlay->setVisible(
            EditorManagerPrivate::hasMoreThanOneview()
            && EditorManagerPrivate::currentEditorView() == this);
    };
    connect(
        EditorManagerPrivate::instance(),
        &EditorManagerPrivate::currentViewChanged,
        currentViewOverlay,
        updateCurrentViewOverlay);
    connect(
        EditorManagerPrivate::instance(),
        &EditorManagerPrivate::viewCountChanged,
        currentViewOverlay,
        updateCurrentViewOverlay);
}

bool EditorView::isInSplit() const
{
    SplitterOrView *viewParent = parentSplitterOrView();
    SplitterOrView *parentSplitter = (viewParent ? viewParent->findParentSplitter() : nullptr);
    return parentSplitter && parentSplitter->isSplitter();
}

EditorView *EditorView::split(Qt::Orientation orientation)
{
    return parentSplitterOrView()->split(orientation);
}

EditorArea *EditorView::editorArea() const
{
    QWidget *current = parentSplitterOrView();
    while (current) {
        if (auto area = qobject_cast<EditorArea *>(current))
            return area;
        current = current->parentWidget();
    }
    QTC_CHECK(false);
    return nullptr;
}

EditorView::~EditorView() = default;

SplitterOrView *EditorView::parentSplitterOrView() const
{
    return m_parentSplitterOrView;
}

EditorView *EditorView::findNextView() const
{
    SplitterOrView *current = parentSplitterOrView();
    QTC_ASSERT(current, return nullptr);
    SplitterOrView *parent = current->findParentSplitter();
    while (parent) {
        QSplitter *splitter = parent->splitter();
        QTC_ASSERT(splitter, return nullptr);
        QTC_ASSERT(splitter->count() == 2, return nullptr);
        // is current the first child? then the next view is the first one in current's sibling
        if (splitter->widget(0) == current) {
            auto second = qobject_cast<SplitterOrView *>(splitter->widget(1));
            QTC_ASSERT(second, return nullptr);
            return second->findFirstView();
        }
        // otherwise go up the hierarchy
        current = parent;
        parent = current->findParentSplitter();
    }
    // current has no parent, so we are at the top and there is no "next" view
    return nullptr;
}

EditorView *EditorView::findPreviousView() const
{
    SplitterOrView *current = parentSplitterOrView();
    QTC_ASSERT(current, return nullptr);
    SplitterOrView *parent = current->findParentSplitter();
    while (parent) {
        QSplitter *splitter = parent->splitter();
        QTC_ASSERT(splitter, return nullptr);
        QTC_ASSERT(splitter->count() == 2, return nullptr);
        // is current the last child? then the previous view is the first child in current's sibling
        if (splitter->widget(1) == current) {
            auto first = qobject_cast<SplitterOrView *>(splitter->widget(0));
            QTC_ASSERT(first, return nullptr);
            return first->findFirstView();
        }
        // otherwise go up the hierarchy
        current = parent;
        parent = current->findParentSplitter();
    }
    // current has no parent, so we are at the top and there is no "previous" view
    return nullptr;
}

void EditorView::closeCurrentEditor()
{
    IEditor *editor = currentEditor();
    if (editor)
       EditorManagerPrivate::closeEditorOrDocument(editor);
}

void EditorView::showEditorStatusBar(const QString &id,
                                     const QString &infoText,
                                     const QString &buttonText,
                                     QObject *object, const std::function<void()> &function)
{
    m_statusWidgetId = id;
    m_statusWidgetLabel->setText(infoText);
    m_statusWidgetButton->setText(buttonText);
    m_statusWidgetButton->setToolTip(buttonText);
    m_statusWidgetButton->disconnect();
    if (object && function)
        connect(m_statusWidgetButton, &QToolButton::clicked, object, function);
    m_statusWidget->setVisible(true);
    m_statusHLine->setVisible(true);
    //m_editorForInfoWidget = currentEditor();
}

void EditorView::hideEditorStatusBar(const QString &id)
{
    if (id == m_statusWidgetId) {
        m_statusWidget->setVisible(false);
        m_statusHLine->setVisible(false);
    }
}

void EditorView::setCloseSplitEnabled(bool enable)
{
    m_toolBar->setCloseSplitEnabled(enable);
}

void EditorView::setCloseSplitIcon(const QIcon &icon)
{
    m_toolBar->setCloseSplitIcon(icon);
}

bool EditorView::isShowingTabs() const
{
    return m_isShowingTabs;
}

void EditorView::setTabsVisible(bool visible)
{
    m_isShowingTabs = visible;
    m_tabBar->setVisible(m_isShowingTabs);
    m_toolBar->setDocumentDropdownVisible(!visible);
}

bool EditorView::canGoForward() const
{
    return m_currentNavigationHistoryPosition < m_navigationHistory.size() - 1;
}

bool EditorView::canGoBack() const
{
    return m_currentNavigationHistoryPosition > 0;
}

bool EditorView::canReopen() const
{
    return !m_closedEditorHistory.isEmpty();
}

void EditorView::updateEditorHistory(IEditor *editor, QList<EditLocation> &history)
{
    IDocument *document = editor ? editor->document() : nullptr;
    QTC_ASSERT(document, return);

    const auto location = EditLocation::forEditor(editor);

    for (int i = 0; i < history.size(); ++i) {
        const EditLocation &item = history.at(i);
        // remove items that refer to the same document/file,
        // or that are no longer in the "open documents"
        if (item.document == document || (!item.document && item.filePath == document->filePath())
            || (!item.document && !DocumentModel::indexOfFilePath(item.filePath))) {
            history.removeAt(i--);
        }
    }
    history.prepend(location);
}

void EditorView::saveTabState(QDataStream *stream) const
{
    QStringList tabs;
    for (int i = 0; i < m_tabBar->count(); ++i) {
        const auto data = m_tabBar->tabData(i).value<TabData>();
        IDocument *document = data.entry->document;
        const FilePath path = document->filePath();
        if (!document->isTemporary() && !path.isEmpty())
            tabs << path.toUrlishString();
    }
    *stream << tabs;
}

void EditorView::restoreTabState(QDataStream *stream)
{
    if (stream->atEnd())
        return;
    QStringList tabs;
    *stream >> tabs;
    for (const QString &tab : std::as_const(tabs)) {
        DocumentModel::Entry *entry = DocumentModel::entryForFilePath(FilePath::fromString(tab));
        if (!entry)
            continue;
        m_tabBar->addTab(""); // text set below
        const int tabIndex = m_tabBar->count() - 1;
        m_tabBar->setTabData(tabIndex, QVariant::fromValue(TabData({nullptr, entry})));
        updateTabText(m_tabBar, tabIndex, entry->document);
    }
}

void EditorView::paintEvent(QPaintEvent *)
{
    EditorView *editorView = EditorManagerPrivate::currentEditorView();
    if (editorView != this)
        return;

    if (m_container->currentIndex() != 0) // so a document is selected
        return;

    // Discreet indication where an editor would be if there is none
    QPainter painter(this);

    QRect rect = m_container->geometry();
    if (creatorTheme()->flag(Theme::FlatToolBars)) {
        painter.fillRect(rect, creatorColor(Theme::EditorPlaceholderColor));
    } else {
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Qt::NoPen);
        painter.setBrush(creatorColor(Theme::EditorPlaceholderColor));
        const int r = 3;
        painter.drawRoundedRect(rect.adjusted(r , r, -r, -r), r * 2, r * 2);
    }
}

void EditorView::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton)
        return;
    setFocus(Qt::MouseFocusReason);
}

void EditorView::focusInEvent(QFocusEvent *)
{
    EditorManagerPrivate::setCurrentView(this);
}

bool EditorView::event(QEvent *e)
{
    if (e->type() == QEvent::NativeGesture) {
        auto ev = static_cast<QNativeGestureEvent *>(e);
        if (ev->gestureType() == Qt::SwipeNativeGesture) {
            if (ev->value() > 0 && canGoBack()) { // swipe from right to left == go back
                goBackInNavigationHistory();
                return true;
            } else if (ev->value() <= 0 && canGoForward()) {
                goForwardInNavigationHistory();
                return true;
            }
        }
    }
    return QWidget::event(e);
}

bool EditorView::eventFilter(QObject *obj, QEvent *e)
{
    if (obj == m_tabBar && e->type() == QEvent::MouseButtonPress) {
        auto me = static_cast<QMouseEvent *>(e);
        if (me->button() != Qt::LeftButton)
            return true;
    }
    return QWidget::eventFilter(obj, e);
}

void EditorView::addEditor(IEditor *editor)
{
    if (m_editors.contains(editor))
        return;
    QTC_ASSERT(
        !Utils::contains(m_editors, Utils::equal(&IEditor::document, editor->document())), return);

    m_editors.append(editor);

    m_container->addWidget(editor->widget());
    m_widgetEditorMap.insert(editor->widget(), editor);
    m_toolBar->addEditor(editor);
    IDocument *document = editor->document();
    int tabIndex = tabForEditor(editor);
    if (tabIndex < 0)
        tabIndex = m_tabBar->addTab(""); // text set below
    m_tabBar->setTabData(
        tabIndex, QVariant::fromValue(TabData({editor, DocumentModel::entryForDocument(document)})));
    updateTabText(m_tabBar, tabIndex, document);
    m_tabBar->setVisible(false); // something is wrong with QTabBar... this is needed
    m_tabBar->setVisible(m_isShowingTabs);

    if (editor == currentEditor())
        setCurrentEditor(editor);
}

bool EditorView::hasEditor(IEditor *editor) const
{
    return m_editors.contains(editor);
}

void EditorView::removeEditor(IEditor *editor, RemovalOption option)
{
    QTC_ASSERT(editor, return);
    if (!m_editors.contains(editor))
        return;

    const int index = m_container->indexOf(editor->widget());
    QTC_ASSERT((index != -1), return);
    const bool wasCurrent = (index == m_container->currentIndex());
    m_editors.removeAll(editor);

    m_container->removeWidget(editor->widget());
    m_widgetEditorMap.remove(editor->widget());
    editor->widget()->setParent(nullptr);
    m_toolBar->removeToolbarForEditor(editor);

    const int tabIndex = tabForEditor(editor);
    if (QTC_GUARD(tabIndex >= 0)) {
        if (option == RemoveTab) {
            m_tabBar->removeTab(tabIndex);
        } else {
            const auto data = m_tabBar->tabData(tabIndex).value<TabData>();
            m_tabBar->setTabData(tabIndex, QVariant::fromValue(TabData({nullptr, data.entry})));
            updateTabText(m_tabBar, tabIndex, editor->document());
        }
    }

    if (wasCurrent)
        setCurrentEditor(nullptr);
}

IEditor *EditorView::currentEditor() const
{
    if (!m_editors.isEmpty())
        return m_widgetEditorMap.value(m_container->currentWidget());
    return nullptr;
}

void EditorView::listSelectionActivated(int index)
{
    EditorManagerPrivate::activateEditorForEntry(this, DocumentModel::entryAtRow(index));
}

void EditorView::fillListContextMenu(QMenu *menu)
{
    IEditor *editor = currentEditor();
    DocumentModel::Entry *entry = editor ? DocumentModel::entryForDocument(editor->document())
                                         : nullptr;
    EditorManagerPrivate::addContextMenuActions(menu, entry, editor, this);
}

void EditorView::splitHorizontally()
{
    if (m_parentSplitterOrView)
        EditorManagerPrivate::activateView(m_parentSplitterOrView->split(Qt::Vertical));
    EditorManagerPrivate::updateActions();
}

void EditorView::splitVertically()
{
    if (m_parentSplitterOrView)
        EditorManagerPrivate::activateView(m_parentSplitterOrView->split(Qt::Horizontal));
    EditorManagerPrivate::updateActions();
}

void EditorView::splitNewWindow()
{
    EditorManagerPrivate::splitNewWindow(this);
}

void EditorView::closeSplit()
{
    EditorManagerPrivate::closeView(this);
    EditorManagerPrivate::updateActions();
}

int EditorView::tabForEditor(IEditor *editor) const
{
    for (int i = 0; i < m_tabBar->count(); ++i) {
        const auto data = m_tabBar->tabData(i).value<TabData>();
        if (data.editor ? data.editor == editor : data.entry->document == editor->document())
            return i;
    }
    return -1;
}

void EditorView::closeTab(DocumentModel::Entry *entry)
{
    closeTab(tabForEntry(entry));
}

void EditorView::removeUnpinnedSuspendedTabs()
{
    for (int i = m_tabBar->count() - 1; i >= 0; --i) {
        const auto data = m_tabBar->tabData(i).value<TabData>();
        if (!data.editor && !data.entry->pinned)
            m_tabBar->removeTab(i);
    }
}

void EditorView::closeTab(int index)
{
    if (index < 0 || index >= m_tabBar->count())
        return;
    const auto data = m_tabBar->tabData(index).value<TabData>();
    if (data.editor)
        EditorManagerPrivate::closeEditorOrDocument(data.editor);
    else {
        m_tabBar->removeTab(index);
        EditorManagerPrivate::tabClosed(data.entry);
    }
}

QList<EditorView::TabData> EditorView::tabs() const
{
    QList<TabData> result;
    for (int i = 0; i < m_tabBar->count(); ++i)
        result.append(m_tabBar->tabData(i).value<TabData>());
    return result;
}

int EditorView::tabForEntry(DocumentModel::Entry *entry) const
{
    for (int i = 0; i < m_tabBar->count(); ++i) {
        const auto data = m_tabBar->tabData(i).value<TabData>();
        if (data.entry == entry)
            return i;
    }
    return -1;
}

void EditorView::activateTab(int index)
{
    if (index < 0) // this happens when clicking in the bar outside of tabs on macOS...
        return;
    const auto data = m_tabBar->tabData(index).value<TabData>();
    if (data.editor)
        EditorManagerPrivate::activateEditor(this, data.editor);
    else
        EditorManagerPrivate::activateEditorForEntry(this, data.entry);
}

void EditorView::openDroppedFiles(const QList<DropSupport::FileSpec> &files)
{
    bool first = true;
    auto specToLink = [](const DropSupport::FileSpec &spec) {
        return Utils::Link(spec.filePath, spec.line, spec.column);
    };
    auto openEntry = [&](const DropSupport::FileSpec &spec) {
        if (first) {
            first = !EditorManagerPrivate::openEditorAt(this, specToLink(spec));
        } else if (spec.column != -1 || spec.line != -1) {
            EditorManagerPrivate::openEditorAt(this,
                                               specToLink(spec),
                                               Id(),
                                               EditorManager::DoNotChangeCurrentEditor
                                                   | EditorManager::DoNotMakeVisible);
        } else {
            auto factory = IEditorFactory::preferredEditorFactories(spec.filePath).value(0);
            DocumentModelPrivate::addSuspendedDocument(spec.filePath, {},
                                                       factory ? factory->id() : Id());
        }
    };
    Utils::reverseForeach(files, openEntry);
}

void EditorView::setParentSplitterOrView(SplitterOrView *splitterOrView)
{
    m_parentSplitterOrView = splitterOrView;
}

void EditorView::setCurrentEditor(IEditor *editor)
{
    if (!editor || m_container->indexOf(editor->widget()) == -1) {
        QTC_CHECK(!editor);
        m_toolBar->setCurrentEditor(nullptr);
        m_infoBarDisplay->setInfoBar(nullptr);
        m_container->setCurrentIndex(0);
        // QTabBar still shows a tab selected in the UI when setting current index to -1
        // but we should only temporarily end up here when tabs are actually shown
        // anyway.
        m_tabBar->setCurrentIndex(-1);
        emit currentEditorChanged(nullptr);
        return;
    }

    m_editors.removeAll(editor);
    m_editors.append(editor);

    const int idx = m_container->indexOf(editor->widget());
    QTC_ASSERT(idx >= 0, return);
    m_container->setCurrentIndex(idx);
    m_toolBar->setCurrentEditor(editor);
    const int tabIndex = tabForEditor(editor);
    m_tabBar->setCurrentIndex(tabIndex);

    updateEditorHistory(editor);

    m_infoBarDisplay->setInfoBar(editor->document()->infoBar());
    emit currentEditorChanged(editor);
}

int EditorView::editorCount() const
{
    return m_editors.size();
}

QList<IEditor *> EditorView::editors() const
{
    return m_editors;
}

IEditor *EditorView::editorForDocument(const IDocument *document) const
{
    return Utils::findOrDefault(m_editors, Utils::equal(&IEditor::document, document));
}

QList<EditorView::TabData> EditorView::visibleTabs() const
{
    if (isShowingTabs()) {
        return tabs();
    }
    if (IEditor *current = currentEditor())
        return {TabData({current, DocumentModel::entryForDocument(current->document())})};
    return {};
}

void EditorView::updateEditorHistory(IEditor *editor)
{
    updateEditorHistory(editor, m_editorHistory);
}

constexpr int navigationHistorySize = 100;

void EditorView::addCurrentPositionToNavigationHistory(const QByteArray &saveState)
{
    IEditor *editor = currentEditor();

    if (!editor || !editor->document())
        return;

    const auto location = EditLocation::forEditor(editor, saveState);
    m_currentNavigationHistoryPosition = qMin(m_currentNavigationHistoryPosition, m_navigationHistory.size()); // paranoia
    m_navigationHistory.insert(m_currentNavigationHistoryPosition, location);
    ++m_currentNavigationHistoryPosition;

    while (m_navigationHistory.size() >= navigationHistorySize) {
        if (m_currentNavigationHistoryPosition > navigationHistorySize / 2) {
            m_navigationHistory.removeFirst();
            --m_currentNavigationHistoryPosition;
        } else {
            m_navigationHistory.removeLast();
        }
    }
    updateNavigatorActions();
}

void EditorView::addClosedEditorToCloseHistory(IEditor *editor)
{
    static const int MAX_ITEMS = 20;

    if (!editor || !editor->document())
        return;

    EditLocation location = EditLocation::forEditor(editor);

    m_closedEditorHistory.push_back(location);

    if (m_closedEditorHistory.size() > MAX_ITEMS)
        m_closedEditorHistory.removeFirst();
}

void EditorView::cutForwardNavigationHistory()
{
    while (m_currentNavigationHistoryPosition < m_navigationHistory.size() - 1)
        m_navigationHistory.removeLast();
}

void EditorView::updateNavigatorActions()
{
    static const int MAX_ITEMS = 20;
    QString lastDisplay;
    m_backMenu->clear();
    int count = 0;
    for (int i = m_currentNavigationHistoryPosition - 1; i >= 0; i--) {
        const EditLocation &loc = m_navigationHistory.at(i);
        if (!loc.displayName().isEmpty() && loc.displayName() != lastDisplay) {
            lastDisplay = loc.displayName();
            m_backMenu->addAction(lastDisplay, this, [this, i] { goToNavigationHistory(i); });
            ++count;
            if (count >= MAX_ITEMS)
                break;
        }
    }
    lastDisplay.clear();
    m_forwardMenu->clear();
    count = 0;
    for (int i = m_currentNavigationHistoryPosition + 1; i < m_navigationHistory.size(); i++) {
        const EditLocation &loc = m_navigationHistory.at(i);
        if (!loc.displayName().isEmpty() && loc.displayName() != lastDisplay) {
            lastDisplay = loc.displayName();
            m_forwardMenu->addAction(lastDisplay, this, [this, i] { goToNavigationHistory(i); });
            ++count;
            if (count >= MAX_ITEMS)
                break;
        }
    }
    m_toolBar->setCanGoBack(canGoBack());
    m_toolBar->setCanGoForward(canGoForward());
}

void EditorView::copyNavigationHistoryFrom(EditorView* other)
{
    if (!other)
        return;
    m_currentNavigationHistoryPosition = other->m_currentNavigationHistoryPosition;
    m_navigationHistory = other->m_navigationHistory;
    m_editorHistory = other->m_editorHistory;
    updateNavigatorActions();
}

void EditorView::updateCurrentPositionInNavigationHistory()
{
    IEditor *editor = currentEditor();
    if (!editor || !editor->document())
        return;

    EditLocation *location;
    if (m_currentNavigationHistoryPosition < m_navigationHistory.size()) {
        location = &m_navigationHistory[m_currentNavigationHistoryPosition];
    } else {
        m_navigationHistory.append(EditLocation());
        location = &m_navigationHistory[m_navigationHistory.size()-1];
    }
    *location = EditLocation::forEditor(editor);
}

static bool fileNameWasRemoved(const FilePath &filePath)
{
    return !filePath.isEmpty() && !filePath.exists();
}

bool EditorView::openEditorFromNavigationHistory(int index)
{
    EditLocation location = m_navigationHistory.at(index);
    IEditor *editor = nullptr;
    if (location.document) {
        editor = EditorManagerPrivate::activateEditorForDocument(
            this, location.document, EditorManager::IgnoreNavigationHistory);
    }
    if (!editor) {
        if (fileNameWasRemoved(location.filePath))
            return false;
        editor = EditorManagerPrivate::openEditor(
            this, location.filePath, location.id, EditorManager::IgnoreNavigationHistory);
        if (!editor)
            return false;
    }
    editor->restoreState(location.state);
    return true;
}

void EditorView::goToNavigationHistory(int index)
{
    if (index >= m_navigationHistory.size())
        return;
    updateCurrentPositionInNavigationHistory();
    if (!openEditorFromNavigationHistory(index))
        m_navigationHistory.removeAt(index);
    m_currentNavigationHistoryPosition = index;
    updateNavigatorActions();
}

void EditorView::goBackInNavigationHistory()
{
    updateCurrentPositionInNavigationHistory();
    while (m_currentNavigationHistoryPosition > 0) {
        --m_currentNavigationHistoryPosition;
        if (openEditorFromNavigationHistory(m_currentNavigationHistoryPosition))
            break;
        m_navigationHistory.removeAt(m_currentNavigationHistoryPosition);
    }
    updateNavigatorActions();
}

void EditorView::goForwardInNavigationHistory()
{
    updateCurrentPositionInNavigationHistory();
    if (m_currentNavigationHistoryPosition >= m_navigationHistory.size()-1)
        return;
    ++m_currentNavigationHistoryPosition;
    while (m_currentNavigationHistoryPosition < m_navigationHistory.size()) {
        if (openEditorFromNavigationHistory(m_currentNavigationHistoryPosition))
            break;
        m_navigationHistory.removeAt(m_currentNavigationHistoryPosition);
    }
    if (m_currentNavigationHistoryPosition >= m_navigationHistory.size())
        m_currentNavigationHistoryPosition = qMax<int>(m_navigationHistory.size() - 1, 0);
    updateNavigatorActions();
}

void EditorView::reopenLastClosedDocument()
{
    if (m_closedEditorHistory.isEmpty())
        return;
    goToEditLocation(m_closedEditorHistory.takeLast());
}

void EditorView::goToEditLocation(const EditLocation &location)
{
    IEditor *editor = nullptr;

    if (location.document) {
        editor = EditorManagerPrivate::activateEditorForDocument(this, location.document,
                                                                 EditorManager::IgnoreNavigationHistory);
    }

    if (!editor) {
        if (fileNameWasRemoved(location.filePath))
            return;

        editor = EditorManagerPrivate::openEditor(this, location.filePath, location.id,
                                                  EditorManager::IgnoreNavigationHistory);
    }

    if (editor) {
        editor->restoreState(location.state);
    }
}

void EditorView::gotoNextTab()
{
    activateTab((m_tabBar->currentIndex() + 1) % m_tabBar->count());
}

void EditorView::gotoPreviousTab()
{
    activateTab((m_tabBar->currentIndex() + m_tabBar->count() - 1) % m_tabBar->count());
}

SplitterOrView::SplitterOrView(IEditor *editor)
{
    m_layout = new QStackedLayout(this);
    m_layout->setSizeConstraint(QLayout::SetNoConstraint);
    m_view = new EditorView(this);
    m_view->setTabsVisible(generalSettings().useTabsInEditorViews.value());
    if (editor)
        m_view->addEditor(editor);
    m_splitter = nullptr;
    m_layout->addWidget(m_view);
}

SplitterOrView::SplitterOrView(EditorView *view)
{
    Q_ASSERT(view);
    m_layout = new QStackedLayout(this);
    m_layout->setSizeConstraint(QLayout::SetNoConstraint);
    m_view = view;
    m_view->setParentSplitterOrView(this);
    m_splitter = nullptr;
    m_layout->addWidget(m_view);
}

SplitterOrView::~SplitterOrView()
{
    delete m_layout;
    m_layout = nullptr;
    if (m_view)
        EditorManagerPrivate::deleteEditors(EditorManagerPrivate::emptyView(m_view));
    delete m_view;
    m_view = nullptr;
    delete m_splitter;
    m_splitter = nullptr;
}

EditorView *SplitterOrView::findFirstView() const
{
    if (m_splitter) {
        for (int i = 0; i < m_splitter->count(); ++i) {
            if (auto splitterOrView = qobject_cast<SplitterOrView*>(m_splitter->widget(i)))
                if (EditorView *result = splitterOrView->findFirstView())
                    return result;
        }
        return nullptr;
    }
    return m_view;
}

EditorView *SplitterOrView::findLastView() const
{
    if (m_splitter) {
        for (int i = m_splitter->count() - 1; 0 < i; --i) {
            if (auto splitterOrView = qobject_cast<SplitterOrView*>(m_splitter->widget(i)))
                if (EditorView *result = splitterOrView->findLastView())
                    return result;
        }
        return nullptr;
    }
    return m_view;
}

SplitterOrView *SplitterOrView::findParentSplitter() const
{
    QWidget *w = parentWidget();
    while (w) {
        if (auto splitter = qobject_cast<SplitterOrView *>(w)) {
            QTC_CHECK(splitter->splitter());
            return splitter;
        }
        w = w->parentWidget();
    }
    return nullptr;
}

QSize SplitterOrView::minimumSizeHint() const
{
    if (m_splitter)
        return m_splitter->minimumSizeHint();
    return QSize(64, 64);
}

QSplitter *SplitterOrView::takeSplitter()
{
    QSplitter *oldSplitter = m_splitter;
    if (m_splitter)
        m_layout->removeWidget(m_splitter);
    m_splitter = nullptr;
    return oldSplitter;
}

EditorView *SplitterOrView::takeView()
{
    EditorView *oldView = m_view;
    if (m_view) {
        // the focus update that is triggered by removing should already have 0 parent
        // so we do that first
        m_view->setParentSplitterOrView(nullptr);
        m_layout->removeWidget(m_view);
    }
    m_view = nullptr;
    return oldView;
}

EditorView *SplitterOrView::split(Qt::Orientation orientation)
{
    QTC_ASSERT(m_view && m_splitter == nullptr, return nullptr);
    m_splitter = new MiniSplitter(this);
    m_splitter->setOrientation(orientation);
    m_layout->addWidget(m_splitter);
    m_layout->removeWidget(m_view);
    EditorView *editorView = m_view;
    editorView->setCloseSplitEnabled(true); // might have been disabled for root view
    m_view = nullptr;
    IEditor *e = editorView->currentEditor();
    const QByteArray state = e ? e->saveState() : QByteArray();

    SplitterOrView *view = nullptr;
    SplitterOrView *otherView = nullptr;
    IEditor *duplicate = e && e->duplicateSupported() ? EditorManagerPrivate::duplicateEditor(e) : nullptr;
    m_splitter->addWidget((view = new SplitterOrView(editorView)));
    m_splitter->addWidget((otherView = new SplitterOrView(duplicate)));

    m_layout->setCurrentWidget(m_splitter);

    otherView->view()->copyNavigationHistoryFrom(editorView);
    otherView->view()->setCurrentEditor(duplicate);

    if (orientation == Qt::Horizontal) {
        view->view()->setCloseSplitIcon(Utils::Icons::CLOSE_SPLIT_LEFT.icon());
        otherView->view()->setCloseSplitIcon(Utils::Icons::CLOSE_SPLIT_RIGHT.icon());
    } else {
        view->view()->setCloseSplitIcon(Utils::Icons::CLOSE_SPLIT_TOP.icon());
        otherView->view()->setCloseSplitIcon(Utils::Icons::CLOSE_SPLIT_BOTTOM.icon());
    }

    // restore old state, possibly adapted to the new layout (the editors can e.g. make sure that
    // a previously visible text cursor stays visible)
    if (duplicate)
        duplicate->restoreState(state);
    if (e)
        e->restoreState(state);

    emit splitStateChanged();
    return otherView->view();
}

void SplitterOrView::unsplitAll(EditorView *currentView)
{
    QTC_ASSERT(m_splitter, return);
    // avoid focus changes while unsplitting is in progress
    bool hadFocus = false;
    if (QWidget *w = focusWidget()) {
        if (w->hasFocus()) {
            w->clearFocus();
            hadFocus = true;
        }
    }

    if (currentView) {
        currentView->parentSplitterOrView()->takeView();
        currentView->setParentSplitterOrView(this);
    } else {
        currentView = new EditorView(this);
        currentView->setTabsVisible(generalSettings().useTabsInEditorViews.value());
    }
    m_splitter->hide();
    m_layout->removeWidget(m_splitter); // workaround Qt bug
    const QList<IEditor *> editorsToDelete = unsplitAll_helper();
    m_view = currentView;
    m_layout->addWidget(m_view);
    delete m_splitter;
    m_splitter = nullptr;

    // restore some focus
    if (hadFocus) {
        if (IEditor *editor = m_view->currentEditor())
            editor->widget()->setFocus();
        else
            m_view->setFocus();
    }
    EditorManagerPrivate::deleteEditors(editorsToDelete);
    emit splitStateChanged();
}

/*!
    Recursively empties all views.
    Returns the editors to delete with EditorManagerPrivate::deleteEditors.
    \internal
*/
const QList<IEditor *> SplitterOrView::unsplitAll_helper()
{
    if (m_view)
        return EditorManagerPrivate::emptyView(m_view);
    QList<IEditor *> editorsToDelete;
    if (m_splitter) {
        for (int i = 0; i < m_splitter->count(); ++i) {
            if (auto splitterOrView = qobject_cast<SplitterOrView*>(m_splitter->widget(i)))
                editorsToDelete.append(splitterOrView->unsplitAll_helper());
        }
    }
    return editorsToDelete;
}

void SplitterOrView::unsplit()
{
    if (!m_splitter)
        return;

    Q_ASSERT(m_splitter->count() == 1);
    auto childSplitterOrView = qobject_cast<SplitterOrView*>(m_splitter->widget(0));
    QSplitter *oldSplitter = m_splitter;
    m_splitter = nullptr;
    QList<IEditor *> editorsToDelete;
    if (childSplitterOrView->isSplitter()) {
        Q_ASSERT(childSplitterOrView->view() == nullptr);
        m_splitter = childSplitterOrView->takeSplitter();
        m_layout->addWidget(m_splitter);
        m_layout->setCurrentWidget(m_splitter);
    } else {
        EditorView *childView = childSplitterOrView->view();
        Q_ASSERT(childView);
        if (m_view) {
            m_view->copyNavigationHistoryFrom(childView);
            if (IEditor *e = childView->currentEditor()) {
                childView->removeEditor(e, EditorView::RemoveTab);
                m_view->addEditor(e);
                m_view->setCurrentEditor(e);
            }
            editorsToDelete = EditorManagerPrivate::emptyView(childView);
        } else {
            m_view = childSplitterOrView->takeView();
            m_view->setParentSplitterOrView(this);
            m_layout->addWidget(m_view);
            auto parentSplitter = qobject_cast<QSplitter *>(parentWidget());
            if (parentSplitter) { // not the toplevel splitterOrView
                if (parentSplitter->orientation() == Qt::Horizontal)
                    m_view->setCloseSplitIcon(parentSplitter->widget(0) == this ?
                                                  Utils::Icons::CLOSE_SPLIT_LEFT.icon()
                                                : Utils::Icons::CLOSE_SPLIT_RIGHT.icon());
                else
                    m_view->setCloseSplitIcon(parentSplitter->widget(0) == this ?
                                                  Utils::Icons::CLOSE_SPLIT_TOP.icon()
                                                : Utils::Icons::CLOSE_SPLIT_BOTTOM.icon());
            }
        }
        m_layout->setCurrentWidget(m_view);
    }
    delete oldSplitter;
    if (EditorView *newCurrent = findFirstView())
        EditorManagerPrivate::activateView(newCurrent);
    else
        EditorManagerPrivate::setCurrentView(nullptr);
    EditorManagerPrivate::deleteEditors(editorsToDelete);
    emit splitStateChanged();
}

static QByteArrayList saveHistory(const QList<EditLocation> &history)
{
    const QList<EditLocation> nonTempHistory = Utils::filtered(history, [](const EditLocation &loc) {
        const bool isTemp = loc.filePath.isEmpty() || (loc.document && loc.document->isTemporary());
        return !isTemp;
    });
    return Utils::transform(nonTempHistory, [](const EditLocation &loc) { return loc.save(); });
}

static QList<EditLocation> loadHistory(const QByteArrayList &data)
{
    return Utils::transform(data,
                            [](const QByteArray &locData) { return EditLocation::load(locData); });
}

QByteArray SplitterOrView::saveState() const
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);

    if (m_splitter) {
        stream << QByteArray("splitter")
                << (qint32)m_splitter->orientation()
                << m_splitter->saveState()
                << static_cast<SplitterOrView*>(m_splitter->widget(0))->saveState()
                << static_cast<SplitterOrView*>(m_splitter->widget(1))->saveState();
    } else {
        IEditor* e = editor();

        // don't save state of temporary or ad-hoc editors
        if (e && (e->document()->isTemporary() || e->document()->filePath().isEmpty())) {
            // look for another editor that is more suited
            e = Utils::findOrDefault(editors(), [](IEditor *otherEditor) {
                return !otherEditor->document()->isTemporary()
                       && !otherEditor->document()->filePath().isEmpty();
            });
        }

        if (!e) {
            stream << QByteArray("empty");
        } else {
            if (e == EditorManager::currentEditor()) {
                stream << QByteArray("currenteditor") << e->document()->filePath().toUrlishString()
                       << e->document()->id().toString() << e->saveState();
            } else {
                stream << QByteArray("editor") << e->document()->filePath().toUrlishString()
                       << e->document()->id().toString() << e->saveState();
            }

            // save edit history
            stream << saveHistory(view()->editorHistory());
            view()->saveTabState(&stream);
        }
    }
    return bytes;
}

void SplitterOrView::restoreState(const QByteArray &state)
{
    QDataStream stream(state);
    QByteArray mode;
    stream >> mode;
    if (mode == "splitter") {
        qint32 orientation;
        QByteArray splitter, first, second;
        stream >> orientation >> splitter >> first >> second;
        split((Qt::Orientation) orientation);
        m_splitter->restoreState(splitter);
        static_cast<SplitterOrView*>(m_splitter->widget(0))->restoreState(first);
        static_cast<SplitterOrView*>(m_splitter->widget(1))->restoreState(second);
    } else if (mode == "editor" || mode == "currenteditor") {
        QString fileName;
        QString id;
        QByteArray editorState;
        QByteArrayList historyData;
        stream >> fileName >> id >> editorState;
        if (!stream.atEnd())
            stream >> historyData;
        view()->restoreTabState(&stream);
        view()->m_editorHistory = loadHistory(historyData);

        if (!QFileInfo::exists(fileName))
            return;
        IEditor *e = EditorManagerPrivate::openEditor(view(), FilePath::fromString(fileName), Id::fromString(id),
                                                      EditorManager::IgnoreNavigationHistory
                                                      | EditorManager::DoNotChangeCurrentEditor);

        if (!e) {
            DocumentModel::Entry *entry = DocumentModelPrivate::firstSuspendedEntry();
            if (entry) {
                EditorManagerPrivate::activateEditorForEntry(view(), entry,
                    EditorManager::IgnoreNavigationHistory | EditorManager::DoNotChangeCurrentEditor);
            }
        }

        if (e) {
            e->restoreState(editorState);
            if (mode == "currenteditor")
                EditorManagerPrivate::setCurrentEditor(e);
        }
    }
}

QByteArray EditLocation::save() const
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << filePath.toFSPathString() << id << state;
    return data;
}

EditLocation EditLocation::load(const QByteArray &data)
{
    EditLocation loc;
    QDataStream stream(data);
    QString fp;
    stream >> fp;
    loc.filePath = FilePath::fromString(fp);
    stream >> loc.id;
    stream >> loc.state;
    return loc;
}

EditLocation EditLocation::forEditor(const IEditor *editor, const QByteArray &saveState)
{
    QTC_ASSERT(editor, return EditLocation());

    IDocument *document = editor->document();
    QTC_ASSERT(document, return EditLocation());

    const QByteArray &state = saveState.isEmpty() ? editor->saveState() : saveState;

    EditLocation location;
    location.document = document;
    location.filePath = document->filePath();
    location.id = document->id();
    location.state = state;

    return location;
}

QString EditLocation::displayName() const
{
    if (document)
        return document->displayName();
    return filePath.fileName();
}

} // Core::Internal
