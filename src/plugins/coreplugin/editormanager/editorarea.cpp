// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "editorarea.h"

#include "editormanager.h"
#include "editormanager_p.h"
#include "editorview.h"
#include "ieditor.h"

#include "../coreconstants.h"
#include "../icontext.h"
#include "../idocument.h"

#include <QApplication>
#include <QVBoxLayout>

namespace Core::Internal {

EditorArea::EditorArea(Utils::Id id)
    : m_id(id)
    , m_splitterOrView(new SplitterOrView)
{
    IContext::attach(this, Context(Constants::C_EDITORMANAGER));

    auto layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);
    layout->addWidget(m_splitterOrView);

    setFocusProxy(m_splitterOrView);

    setCurrentView(m_splitterOrView->view());
    updateCloseSplitButton();

    connect(qApp, &QApplication::focusChanged,
            this, &EditorArea::focusChanged,
            Qt::QueuedConnection);

    connect(
        m_splitterOrView,
        &SplitterOrView::splitStateChanged,
        this,
        &EditorArea::updateCloseSplitButton);
    connect(
        m_splitterOrView, &SplitterOrView::splitStateChanged, this, &EditorArea::splitStateChanged);
}

EditorArea::~EditorArea()
{
    // disconnect
    setCurrentView(nullptr);
    disconnect(qApp, &QApplication::focusChanged,
               this, &EditorArea::focusChanged);
}

Utils::Id EditorArea::id() const
{
    return m_id;
}

IDocument *EditorArea::currentDocument() const
{
    return m_currentDocument;
}

EditorView *EditorArea::currentView() const
{
    return m_currentView;
}

EditorView *EditorArea::findFirstView() const
{
    return m_splitterOrView->findFirstView();
}

EditorView *EditorArea::findLastView() const
{
    return m_splitterOrView->findLastView();
}

SplitterOrView *EditorArea::splitterOrView() const
{
    return m_splitterOrView;
}

bool EditorArea::hasSplits() const
{
    return m_splitterOrView->isSplitter();
}

EditorView *EditorArea::unsplit(EditorView *view)
{
    SplitterOrView *splitterOrView = view->parentSplitterOrView();
    Q_ASSERT(splitterOrView);
    Q_ASSERT(splitterOrView->view() == view);
    SplitterOrView *splitter = splitterOrView->findParentSplitter();
    Q_ASSERT(splitterOrView->hasEditors() == false);
    splitterOrView->hide();
    delete splitterOrView;

    splitter->unsplit();

    // candidate for new current view
    EditorView *newCurrent = splitter->findFirstView();
    // The current view is gone if it was the one that was removed.
    if (!m_currentView)
        setCurrentView(newCurrent);
    return newCurrent;
}

void EditorArea::unsplitAll(EditorView *viewToKeep)
{
    m_splitterOrView->unsplitAll(viewToKeep);
    // The current view is gone if it was not the one that was kept.
    if (!m_currentView)
        setCurrentView(findFirstView());
}

QByteArray EditorArea::saveState() const
{
    return m_splitterOrView->saveState();
}

void EditorArea::restoreState(const QByteArray &s)
{
    m_splitterOrView->restoreState(s);
}

void EditorArea::focusChanged()
{
    QWidget *now = QApplication::focusWidget();
    // only interesting if the focus moved within the editor area
    if (!focusWidget() || focusWidget() != now)
        return;
    // find the view with focus
    EditorView *current = findFirstView();
    while (current) {
        if (current->focusWidget() && current->focusWidget() == now) {
            setCurrentView(current);
            break;
        }
        current = current->findNextView();
    }
}

void EditorArea::setCurrentView(EditorView *view)
{
    if (view == m_currentView)
        return;
    if (m_currentView) {
        disconnect(m_currentView.data(), &EditorView::currentEditorChanged,
                   this, &EditorArea::updateCurrentEditor);
    }
    m_currentView = view;
    if (m_currentView) {
        connect(m_currentView.data(), &EditorView::currentEditorChanged,
                this, &EditorArea::updateCurrentEditor);
    }
    updateCurrentEditor(m_currentView ? m_currentView->currentEditor() : nullptr);
}

void EditorArea::updateCurrentEditor(IEditor *editor)
{
    IDocument *document = editor ? editor->document() : nullptr;
    if (document == m_currentDocument)
        return;
    if (m_currentDocument) {
        disconnect(m_currentDocument.data(), &IDocument::changed,
                   this, &EditorArea::windowTitleNeedsUpdate);
    }
    m_currentDocument = document;
    if (m_currentDocument) {
        connect(m_currentDocument.data(), &IDocument::changed,
                this, &EditorArea::windowTitleNeedsUpdate);
    }
    emit windowTitleNeedsUpdate();
}

void EditorArea::updateCloseSplitButton()
{
    if (EditorView *v = m_splitterOrView->view())
        v->setCloseSplitEnabled(false);
}

void EditorArea::showEvent(QShowEvent *)
{
    // The window title follows what the area on screen shows.
    emit windowTitleNeedsUpdate();

    // Whatever is opened next belongs to the area that is now on screen, not
    // to the one of the mode that was left. An area in another window keeps
    // the current view while that window is the one being worked in: a mode
    // switch in the main window is not a reason to open the next editor there.
    EditorView *current = EditorManagerPrivate::currentEditorView();
    if (current
        && (current->editorArea() == this
            || (current->window() != window() && current->window()->isActiveWindow()))) {
        return;
    }
    if (m_currentView)
        EditorManagerPrivate::setCurrentView(m_currentView);
}

void EditorArea::hideEvent(QHideEvent *)
{
    emit hidden();
}

} // Core::Internal
