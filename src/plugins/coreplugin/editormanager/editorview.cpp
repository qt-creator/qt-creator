// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "editorview.h"

#include "editormanager.h"
#include "editormanager_p.h"
#include "documentmodel.h"
#include "documentmodel_p.h"
#include "../actionmanager/actionmanager.h"
#include "../editormanager/ieditor.h"
#include "../editortoolbar.h"
#include "../findplaceholder.h"
#include "../icore.h"
#include "../minisplitter.h"

#include <utils/algorithm.h>
#include <utils/infobar.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>
#include <utils/layoutbuilder.h>
#include <utils/link.h>
#include <utils/utilsicons.h>

#include <QDebug>

#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QStackedWidget>
#include <QToolButton>
#include <QSplitter>
#include <QStackedLayout>

using namespace Core;
using namespace Core::Internal;
using namespace Utils;

// ================EditorView====================

EditorView::EditorView(SplitterOrView *parentSplitterOrView, QWidget *parent) :
    QWidget(parent),
    m_parentSplitterOrView(parentSplitterOrView),
    m_toolBar(new EditorToolBar(this)),
    m_container(new QStackedWidget(this)),
    m_infoBarDisplay(new InfoBarDisplay(this)),
    m_statusHLine(Layouting::createHr(this)),
    m_statusWidget(new QFrame(this))
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
        tl->addWidget(m_toolBar);
    }

    m_infoBarDisplay->setTarget(tl, 1);

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

void EditorView::updateEditorHistory(IEditor *editor, QList<EditLocation> &history)
{
    if (!editor)
        return;
    IDocument *document = editor->document();

    if (!document)
        return;

    QByteArray state = editor->saveState();

    EditLocation location;
    location.document = document;
    location.filePath = document->filePath();
    location.id = document->id();
    location.state = state;

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
        painter.fillRect(rect, creatorTheme()->color(Theme::EditorPlaceholderColor));
    } else {
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Qt::NoPen);
        painter.setBrush(creatorTheme()->color(Theme::EditorPlaceholderColor));
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

void EditorView::addEditor(IEditor *editor)
{
    if (m_editors.contains(editor))
        return;

    m_editors.append(editor);

    m_container->addWidget(editor->widget());
    m_widgetEditorMap.insert(editor->widget(), editor);
    m_toolBar->addEditor(editor);

    if (editor == currentEditor())
        setCurrentEditor(editor);
}

bool EditorView::hasEditor(IEditor *editor) const
{
    return m_editors.contains(editor);
}

void EditorView::removeEditor(IEditor *editor)
{
    QTC_ASSERT(editor, return);
    if (!m_editors.contains(editor))
        return;

    const int index = m_container->indexOf(editor->widget());
    QTC_ASSERT((index != -1), return);
    bool wasCurrent = (index == m_container->currentIndex());
    m_editors.removeAll(editor);

    m_container->removeWidget(editor->widget());
    m_widgetEditorMap.remove(editor->widget());
    editor->widget()->setParent(nullptr);
    m_toolBar->removeToolbarForEditor(editor);

    if (wasCurrent)
        setCurrentEditor(!m_editors.isEmpty() ? m_editors.last() : nullptr);
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

void EditorView::fillListContextMenu(QMenu *menu) const
{
    IEditor *editor = currentEditor();
    DocumentModel::Entry *entry = editor ? DocumentModel::entryForDocument(editor->document())
                                         : nullptr;
    EditorManager::addSaveAndCloseEditorActions(menu, entry, editor);
    menu->addSeparator();
    EditorManager::addNativeDirAndOpenWithActions(menu, entry);
}

void EditorView::splitHorizontally()
{
    if (m_parentSplitterOrView)
        m_parentSplitterOrView->split(Qt::Vertical);
    EditorManagerPrivate::updateActions();
}

void EditorView::splitVertically()
{
    if (m_parentSplitterOrView)
        m_parentSplitterOrView->split(Qt::Horizontal);
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
        emit currentEditorChanged(nullptr);
        return;
    }

    m_editors.removeAll(editor);
    m_editors.append(editor);

    const int idx = m_container->indexOf(editor->widget());
    QTC_ASSERT(idx >= 0, return);
    m_container->setCurrentIndex(idx);
    m_toolBar->setCurrentEditor(editor);

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

void EditorView::updateEditorHistory(IEditor *editor)
{
    updateEditorHistory(editor, m_editorHistory);
}

constexpr int navigationHistorySize = 100;

void EditorView::addCurrentPositionToNavigationHistory(const QByteArray &saveState)
{
    IEditor *editor = currentEditor();
    if (!editor)
        return;
    IDocument *document = editor->document();

    if (!document)
        return;

    QByteArray state;
    if (saveState.isNull())
        state = editor->saveState();
    else
        state = saveState;

    EditLocation location;
    location.document = document;
    location.filePath = document->filePath();
    location.id = document->id();
    location.state = state;
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

void EditorView::cutForwardNavigationHistory()
{
    while (m_currentNavigationHistoryPosition < m_navigationHistory.size() - 1)
        m_navigationHistory.removeLast();
}

void EditorView::updateNavigatorActions()
{
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

    IDocument *document = editor->document();
    EditLocation *location;
    if (m_currentNavigationHistoryPosition < m_navigationHistory.size()) {
        location = &m_navigationHistory[m_currentNavigationHistoryPosition];
    } else {
        m_navigationHistory.append(EditLocation());
        location = &m_navigationHistory[m_navigationHistory.size()-1];
    }
    location->document = document;
    location->filePath = document->filePath();
    location->id = document->id();
    location->state = editor->saveState();
}

static bool fileNameWasRemoved(const FilePath &filePath)
{
    return !filePath.isEmpty() && !filePath.exists();
}

void EditorView::goBackInNavigationHistory()
{
    updateCurrentPositionInNavigationHistory();
    while (m_currentNavigationHistoryPosition > 0) {
        --m_currentNavigationHistoryPosition;
        EditLocation location = m_navigationHistory.at(m_currentNavigationHistoryPosition);
        IEditor *editor = nullptr;
        if (location.document) {
            editor = EditorManagerPrivate::activateEditorForDocument(this, location.document,
                                        EditorManager::IgnoreNavigationHistory);
        }
        if (!editor) {
            if (fileNameWasRemoved(location.filePath)) {
                m_navigationHistory.removeAt(m_currentNavigationHistoryPosition);
                continue;
            }
            editor = EditorManagerPrivate::openEditor(this, location.filePath, location.id,
                                    EditorManager::IgnoreNavigationHistory);
            if (!editor) {
                m_navigationHistory.removeAt(m_currentNavigationHistoryPosition);
                continue;
            }
        }
        editor->restoreState(location.state);
        break;
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
        IEditor *editor = nullptr;
        EditLocation location = m_navigationHistory.at(m_currentNavigationHistoryPosition);
        if (location.document) {
            editor = EditorManagerPrivate::activateEditorForDocument(this, location.document,
                                                                     EditorManager::IgnoreNavigationHistory);
        }
        if (!editor) {
            if (fileNameWasRemoved(location.filePath)) {
                m_navigationHistory.removeAt(m_currentNavigationHistoryPosition);
                continue;
            }
            editor = EditorManagerPrivate::openEditor(this, location.filePath, location.id,
                                                      EditorManager::IgnoreNavigationHistory);
            if (!editor) {
                m_navigationHistory.removeAt(m_currentNavigationHistoryPosition);
                continue;
            }
        }
        editor->restoreState(location.state);
        break;
    }
    if (m_currentNavigationHistoryPosition >= m_navigationHistory.size())
        m_currentNavigationHistoryPosition = qMax<int>(m_navigationHistory.size() - 1, 0);
    updateNavigatorActions();
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


SplitterOrView::SplitterOrView(IEditor *editor)
{
    m_layout = new QStackedLayout(this);
    m_layout->setSizeConstraint(QLayout::SetNoConstraint);
    m_view = new EditorView(this);
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

void SplitterOrView::split(Qt::Orientation orientation, bool activateView)
{
    Q_ASSERT(m_view && m_splitter == nullptr);
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
    m_splitter->addWidget((view = new SplitterOrView(duplicate)));
    m_splitter->addWidget((otherView = new SplitterOrView(editorView)));

    m_layout->setCurrentWidget(m_splitter);

    view->view()->copyNavigationHistoryFrom(editorView);
    view->view()->setCurrentEditor(duplicate);

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

    if (activateView)
        EditorManagerPrivate::activateView(otherView->view());
    emit splitStateChanged();
}

void SplitterOrView::unsplitAll()
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

    EditorView *currentView = EditorManagerPrivate::currentEditorView();
    if (currentView) {
        currentView->parentSplitterOrView()->takeView();
        currentView->setParentSplitterOrView(this);
    } else {
        currentView = new EditorView(this);
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
                childView->removeEditor(e);
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
                stream << QByteArray("currenteditor") << e->document()->filePath().toString()
                       << e->document()->id().toString() << e->saveState();
            } else {
                stream << QByteArray("editor") << e->document()->filePath().toString()
                       << e->document()->id().toString() << e->saveState();
            }

            // save edit history
            stream << saveHistory(view()->editorHistory());
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
        split((Qt::Orientation) orientation, false);
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
