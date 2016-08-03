/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "editorview.h"

#include "editormanager.h"
#include "editormanager_p.h"
#include "documentmodel.h"
#include "documentmodel_p.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editortoolbar.h>
#include <coreplugin/icore.h>
#include <coreplugin/infobar.h>
#include <coreplugin/locator/locatorconstants.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/findplaceholder.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>
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
    m_statusHLine(new QFrame(this)),
    m_statusWidget(new QFrame(this)),
    m_currentNavigationHistoryPosition(0)
{
    QVBoxLayout *tl = new QVBoxLayout(this);
    tl->setSpacing(0);
    tl->setMargin(0);
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
        m_statusHLine->setFrameStyle(QFrame::HLine);

        m_statusWidget->setFrameStyle(QFrame::NoFrame);
        m_statusWidget->setLineWidth(0);
        m_statusWidget->setAutoFillBackground(true);

        QHBoxLayout *hbox = new QHBoxLayout(m_statusWidget);
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
    m_widgetEditorMap.insert(empty, 0);

    auto dropSupport = new DropSupport(this, [this](QDropEvent *event, DropSupport *dropSupport) -> bool {
        // do not accept move events except from other editor views (i.e. their tool bars)
        // otherwise e.g. item views that support moving items within themselves would
        // also "move" the item into the editor view, i.e. the item would be removed from the
        // item view
        if (!qobject_cast<EditorToolBar*>(event->source()))
            event->setDropAction(Qt::CopyAction);
        if (event->type() == QDropEvent::DragEnter && !dropSupport->isFileDrop(event))
            return false; // do not accept drops without files
        return event->source() != m_toolBar; // do not accept drops on ourselves
    });
    connect(dropSupport, &DropSupport::filesDropped,
            this, &EditorView::openDroppedFiles);

    updateNavigatorActions();
}

EditorView::~EditorView()
{
}

SplitterOrView *EditorView::parentSplitterOrView() const
{
    return m_parentSplitterOrView;
}

EditorView *EditorView::findNextView()
{
    SplitterOrView *current = parentSplitterOrView();
    QTC_ASSERT(current, return 0);
    SplitterOrView *parent = current->findParentSplitter();
    while (parent) {
        QSplitter *splitter = parent->splitter();
        QTC_ASSERT(splitter, return 0);
        QTC_ASSERT(splitter->count() == 2, return 0);
        // is current the first child? then the next view is the first one in current's sibling
        if (splitter->widget(0) == current) {
            SplitterOrView *second = qobject_cast<SplitterOrView *>(splitter->widget(1));
            QTC_ASSERT(second, return 0);
            return second->findFirstView();
        }
        // otherwise go up the hierarchy
        current = parent;
        parent = current->findParentSplitter();
    }
    // current has no parent, so we are at the top and there is no "next" view
    return 0;
}

EditorView *EditorView::findPreviousView()
{
    SplitterOrView *current = parentSplitterOrView();
    QTC_ASSERT(current, return 0);
    SplitterOrView *parent = current->findParentSplitter();
    while (parent) {
        QSplitter *splitter = parent->splitter();
        QTC_ASSERT(splitter, return 0);
        QTC_ASSERT(splitter->count() == 2, return 0);
        // is current the last child? then the previous view is the first child in current's sibling
        if (splitter->widget(1) == current) {
            SplitterOrView *first = qobject_cast<SplitterOrView *>(splitter->widget(0));
            QTC_ASSERT(first, return 0);
            return first->findFirstView();
        }
        // otherwise go up the hierarchy
        current = parent;
        parent = current->findParentSplitter();
    }
    // current has no parent, so we are at the top and there is no "previous" view
    return 0;
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
                                     QObject *object, const char *member)
{
    m_statusWidgetId = id;
    m_statusWidgetLabel->setText(infoText);
    m_statusWidgetButton->setText(buttonText);
    m_statusWidgetButton->setToolTip(buttonText);
    m_statusWidgetButton->disconnect();
    if (object && member)
        connect(m_statusWidgetButton, SIGNAL(clicked()), object, member);
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
    location.fileName = document->filePath().toString();
    location.id = document->id();
    location.state = QVariant(state);

    for (int i = 0; i < history.size(); ++i) {
        if (history.at(i).document == 0
            || history.at(i).document == document
            ){
            history.removeAt(i--);
            continue;
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
    editor->widget()->setParent(0);
    m_toolBar->removeToolbarForEditor(editor);

    if (wasCurrent)
        setCurrentEditor(m_editors.count() ? m_editors.last() : 0);
}

IEditor *EditorView::currentEditor() const
{
    if (m_editors.count() > 0)
        return m_widgetEditorMap.value(m_container->currentWidget());
    return 0;
}

void EditorView::listSelectionActivated(int index)
{
    EditorManagerPrivate::activateEditorForEntry(this, DocumentModel::entryAtRow(index));
}

void EditorView::fillListContextMenu(QMenu *menu)
{
    IEditor *editor = currentEditor();
    DocumentModel::Entry *entry = editor ? DocumentModel::entryForDocument(editor->document())
                                         : 0;
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
    const int count = files.size();
    for (int i = 0; i < count; ++i) {
        const DropSupport::FileSpec spec = files.at(i);
        EditorManagerPrivate::openEditorAt(this, spec.filePath, spec.line, spec.column, Id(),
                                  i < count - 1 ? EditorManager::DoNotChangeCurrentEditor
                                                  | EditorManager::DoNotMakeVisible
                                                : EditorManager::NoFlags);
    }
}

void EditorView::setParentSplitterOrView(SplitterOrView *splitterOrView)
{
    m_parentSplitterOrView = splitterOrView;
}

void EditorView::setCurrentEditor(IEditor *editor)
{
    if (!editor || m_container->indexOf(editor->widget()) == -1) {
        QTC_CHECK(!editor);
        m_toolBar->setCurrentEditor(0);
        m_infoBarDisplay->setInfoBar(0);
        m_container->setCurrentIndex(0);
        emit currentEditorChanged(0);
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
    foreach (IEditor *editor, m_editors)
        if (editor->document() == document)
            return editor;
    return 0;
}

void EditorView::updateEditorHistory(IEditor *editor)
{
    updateEditorHistory(editor, m_editorHistory);
}

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
    location.fileName = document->filePath().toString();
    location.id = document->id();
    location.state = QVariant(state);
    m_currentNavigationHistoryPosition = qMin(m_currentNavigationHistoryPosition, m_navigationHistory.size()); // paranoia
    m_navigationHistory.insert(m_currentNavigationHistoryPosition, location);
    ++m_currentNavigationHistoryPosition;

    while (m_navigationHistory.size() >= 30) {
        if (m_currentNavigationHistoryPosition > 15) {
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
    location->fileName = document->filePath().toString();
    location->id = document->id();
    location->state = QVariant(editor->saveState());
}

namespace {
static inline bool fileNameWasRemoved(const QString &fileName)
{
    return !fileName.isEmpty() && !QFileInfo(fileName).exists();
}
} // End of anonymous namespace

void EditorView::goBackInNavigationHistory()
{
    updateCurrentPositionInNavigationHistory();
    while (m_currentNavigationHistoryPosition > 0) {
        --m_currentNavigationHistoryPosition;
        EditLocation location = m_navigationHistory.at(m_currentNavigationHistoryPosition);
        IEditor *editor = 0;
        if (location.document) {
            editor = EditorManagerPrivate::activateEditorForDocument(this, location.document,
                                        EditorManager::IgnoreNavigationHistory);
        }
        if (!editor) {
            if (fileNameWasRemoved(location.fileName)) {
                m_navigationHistory.removeAt(m_currentNavigationHistoryPosition);
                continue;
            }
            editor = EditorManagerPrivate::openEditor(this, location.fileName, location.id,
                                    EditorManager::IgnoreNavigationHistory);
            if (!editor) {
                m_navigationHistory.removeAt(m_currentNavigationHistoryPosition);
                continue;
            }
        }
        editor->restoreState(location.state.toByteArray());
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
        IEditor *editor = 0;
        EditLocation location = m_navigationHistory.at(m_currentNavigationHistoryPosition);
        if (location.document) {
            editor = EditorManagerPrivate::activateEditorForDocument(this, location.document,
                                                                     EditorManager::IgnoreNavigationHistory);
        }
        if (!editor) {
            if (fileNameWasRemoved(location.fileName)) {
                m_navigationHistory.removeAt(m_currentNavigationHistoryPosition);
                continue;
            }
            editor = EditorManagerPrivate::openEditor(this, location.fileName, location.id,
                                                      EditorManager::IgnoreNavigationHistory);
            if (!editor) {
                m_navigationHistory.removeAt(m_currentNavigationHistoryPosition);
                continue;
            }
        }
        editor->restoreState(location.state.toByteArray());
        break;
    }
    if (m_currentNavigationHistoryPosition >= m_navigationHistory.size())
        m_currentNavigationHistoryPosition = qMax<int>(m_navigationHistory.size() - 1, 0);
    updateNavigatorActions();
}


SplitterOrView::SplitterOrView(IEditor *editor)
{
    m_layout = new QStackedLayout(this);
    m_layout->setSizeConstraint(QLayout::SetNoConstraint);
    m_view = new EditorView(this);
    if (editor)
        m_view->addEditor(editor);
    m_splitter = 0;
    m_layout->addWidget(m_view);
}

SplitterOrView::SplitterOrView(EditorView *view)
{
    Q_ASSERT(view);
    m_layout = new QStackedLayout(this);
    m_layout->setSizeConstraint(QLayout::SetNoConstraint);
    m_view = view;
    m_view->setParentSplitterOrView(this);
    m_splitter = 0;
    m_layout->addWidget(m_view);
}

SplitterOrView::~SplitterOrView()
{
    delete m_layout;
    m_layout = 0;
    if (m_view)
        EditorManagerPrivate::emptyView(m_view);
    delete m_view;
    m_view = 0;
    delete m_splitter;
    m_splitter = 0;
}

EditorView *SplitterOrView::findFirstView()
{
    if (m_splitter) {
        for (int i = 0; i < m_splitter->count(); ++i) {
            if (SplitterOrView *splitterOrView = qobject_cast<SplitterOrView*>(m_splitter->widget(i)))
                if (EditorView *result = splitterOrView->findFirstView())
                    return result;
        }
        return 0;
    }
    return m_view;
}

EditorView *SplitterOrView::findLastView()
{
    if (m_splitter) {
        for (int i = m_splitter->count() - 1; 0 < i; --i) {
            if (SplitterOrView *splitterOrView = qobject_cast<SplitterOrView*>(m_splitter->widget(i)))
                if (EditorView *result = splitterOrView->findLastView())
                    return result;
        }
        return 0;
    }
    return m_view;
}

SplitterOrView *SplitterOrView::findParentSplitter() const
{
    QWidget *w = parentWidget();
    while (w) {
        if (SplitterOrView *splitter = qobject_cast<SplitterOrView *>(w)) {
            QTC_CHECK(splitter->splitter());
            return splitter;
        }
        w = w->parentWidget();
    }
    return 0;
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
    m_splitter = 0;
    return oldSplitter;
}

EditorView *SplitterOrView::takeView()
{
    EditorView *oldView = m_view;
    if (m_view) {
        // the focus update that is triggered by removing should already have 0 parent
        // so we do that first
        m_view->setParentSplitterOrView(0);
        m_layout->removeWidget(m_view);
    }
    m_view = 0;
    return oldView;
}

void SplitterOrView::split(Qt::Orientation orientation)
{
    Q_ASSERT(m_view && m_splitter == 0);
    m_splitter = new MiniSplitter(this);
    m_splitter->setOrientation(orientation);
    m_layout->addWidget(m_splitter);
    m_layout->removeWidget(m_view);
    EditorView *editorView = m_view;
    editorView->setCloseSplitEnabled(true); // might have been disabled for root view
    m_view = 0;
    IEditor *e = editorView->currentEditor();

    SplitterOrView *view = 0;
    SplitterOrView *otherView = 0;
    IEditor *duplicate = e && e->duplicateSupported() ? EditorManagerPrivate::duplicateEditor(e) : 0;
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
    unsplitAll_helper();
    m_view = currentView;
    m_layout->addWidget(m_view);
    delete m_splitter;
    m_splitter = 0;

    // restore some focus
    if (hadFocus) {
        if (IEditor *editor = m_view->currentEditor())
            editor->widget()->setFocus();
        else
            m_view->setFocus();
    }
    emit splitStateChanged();
}

void SplitterOrView::unsplitAll_helper()
{
    if (m_view)
        EditorManagerPrivate::emptyView(m_view);
    if (m_splitter) {
        for (int i = 0; i < m_splitter->count(); ++i) {
            if (SplitterOrView *splitterOrView = qobject_cast<SplitterOrView*>(m_splitter->widget(i)))
                splitterOrView->unsplitAll_helper();
        }
    }
}

void SplitterOrView::unsplit()
{
    if (!m_splitter)
        return;

    Q_ASSERT(m_splitter->count() == 1);
    SplitterOrView *childSplitterOrView = qobject_cast<SplitterOrView*>(m_splitter->widget(0));
    QSplitter *oldSplitter = m_splitter;
    m_splitter = 0;

    if (childSplitterOrView->isSplitter()) {
        Q_ASSERT(childSplitterOrView->view() == 0);
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
            EditorManagerPrivate::emptyView(childView);
        } else {
            m_view = childSplitterOrView->takeView();
            m_view->setParentSplitterOrView(this);
            m_layout->addWidget(m_view);
            QSplitter *parentSplitter = qobject_cast<QSplitter *>(parentWidget());
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
        EditorManagerPrivate::setCurrentView(0);
    emit splitStateChanged();
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
            e = 0;
            foreach (IEditor *otherEditor, editors()) {
                if (!otherEditor->document()->isTemporary() && !otherEditor->document()->filePath().isEmpty()) {
                    e = otherEditor;
                    break;
                }
            }
        }

        if (!e) {
            stream << QByteArray("empty");
        } else if (e == EditorManager::currentEditor()) {
            stream << QByteArray("currenteditor")
                   << e->document()->filePath().toString()
                   << e->document()->id().toString()
                   << e->saveState();
        } else {
            stream << QByteArray("editor")
                   << e->document()->filePath().toString()
                   << e->document()->id().toString()
                   << e->saveState();
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
        split((Qt::Orientation)orientation);
        m_splitter->restoreState(splitter);
        static_cast<SplitterOrView*>(m_splitter->widget(0))->restoreState(first);
        static_cast<SplitterOrView*>(m_splitter->widget(1))->restoreState(second);
    } else if (mode == "editor" || mode == "currenteditor") {
        QString fileName;
        QString id;
        QByteArray editorState;
        stream >> fileName >> id >> editorState;
        if (!QFile::exists(fileName))
            return;
        IEditor *e = EditorManagerPrivate::openEditor(view(), fileName, Id::fromString(id),
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
