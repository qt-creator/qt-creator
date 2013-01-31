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

#include "editorview.h"
#include "editormanager.h"
#include "icore.h"
#include "minisplitter.h"
#include "openeditorsmodel.h"

#include <coreplugin/editortoolbar.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/infobar.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <coreplugin/findplaceholder.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMimeData>

#include <QApplication>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QStackedWidget>
#include <QStyle>
#include <QStyleOption>
#include <QToolButton>
#include <QMenu>
#include <QClipboard>
#include <QAction>
#include <QSplitter>
#include <QStackedLayout>

using namespace Core;
using namespace Core::Internal;


// ================EditorView====================

EditorView::EditorView(QWidget *parent) :
    QWidget(parent),
    m_toolBar(EditorManager::createToolBar(this)),
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
        connect(m_toolBar, SIGNAL(goBackClicked()), this, SLOT(goBackInNavigationHistory()));
        connect(m_toolBar, SIGNAL(goForwardClicked()), this, SLOT(goForwardInNavigationHistory()));
        connect(m_toolBar, SIGNAL(closeClicked()), this, SLOT(closeView()));
        connect(m_toolBar, SIGNAL(listSelectionActivated(int)), this, SLOT(listSelectionActivated(int)));
        connect(m_toolBar, SIGNAL(horizontalSplitClicked()), this, SLOT(splitHorizontally()));
        connect(m_toolBar, SIGNAL(verticalSplitClicked()), this, SLOT(splitVertically()));
        connect(m_toolBar, SIGNAL(closeSplitClicked()), this, SLOT(closeSplit()));
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

    updateNavigatorActions();
}

EditorView::~EditorView()
{
}


void EditorView::closeView()
{
    EditorManager *em = ICore::editorManager();
    IEditor *editor = currentEditor();
    if (editor)
       em->closeEditor(editor);
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
    if (m_container->count() > 0)
        return m_widgetEditorMap.value(m_container->currentWidget());
    return 0;
}

void EditorView::listSelectionActivated(int index)
{
    QAbstractItemModel *model = EditorManager::instance()->openedEditorsModel();
    EditorManager::instance()->activateEditorForIndex(this, model->index(index, 0), Core::EditorManager::ModeSwitch);
}

void EditorView::splitHorizontally()
{
    EditorManager *editorManager = EditorManager::instance();
    SplitterOrView *splitterOrView = editorManager->topSplitterOrView()->findView(this);
    if (splitterOrView)
        splitterOrView->split(Qt::Vertical);
    editorManager->updateActions();
}

void EditorView::splitVertically()
{
    EditorManager *editorManager = EditorManager::instance();
    SplitterOrView *splitterOrView = editorManager->topSplitterOrView()->findView(this);
    if (splitterOrView)
        splitterOrView->split(Qt::Horizontal);
    editorManager->updateActions();
}

void EditorView::closeSplit()
{
    EditorManager *editorManager = EditorManager::instance();
    editorManager->closeView(this);
    editorManager->updateActions();
}

void EditorView::setCurrentEditor(IEditor *editor)
{
    if (!editor || m_container->count() <= 0
        || m_container->indexOf(editor->widget()) == -1) {
        m_toolBar->updateEditorStatus(0);
        m_infoBarDisplay->setInfoBar(0);
        QTC_CHECK(m_container->count() == 0);
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
}

int EditorView::editorCount() const
{
    return m_container->count();
}

QList<IEditor *> EditorView::editors() const
{
    return m_widgetEditorMap.values();
}

void EditorView::updateEditorHistory(IEditor *editor)
{
    if (!editor)
        return;
    IDocument *document = editor->document();

    if (!document)
        return;

    QByteArray state = editor->saveState();

    EditLocation location;
    location.document = document;
    location.fileName = document->fileName();
    location.id = editor->id();
    location.state = QVariant(state);

    for (int i = 0; i < m_editorHistory.size(); ++i) {
        if (m_editorHistory.at(i).document == 0
            || m_editorHistory.at(i).document == document
            ){
            m_editorHistory.removeAt(i--);
            continue;
        }
    }
    m_editorHistory.prepend(location);
}

QRect EditorView::editorArea() const
{
    const QRect cRect = m_container->rect();
    return QRect(m_container->mapToGlobal(cRect.topLeft()), cRect.size());
}

void EditorView::addCurrentPositionToNavigationHistory(IEditor *editor, const QByteArray &saveState)
{
    if (editor && editor != currentEditor())
        return; // we only save editor sate for the current editor, when the user interacts

    if (!editor)
        editor = currentEditor();
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
    location.fileName = document->fileName();
    location.id = editor->id();
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
    location->fileName = document->fileName();
    location->id = editor->id();
    location->state = QVariant(editor->saveState());
}

void EditorView::goBackInNavigationHistory()
{
    EditorManager *em = ICore::editorManager();
    updateCurrentPositionInNavigationHistory();
    while (m_currentNavigationHistoryPosition > 0) {
        --m_currentNavigationHistoryPosition;
        EditLocation location = m_navigationHistory.at(m_currentNavigationHistoryPosition);
        IEditor *editor = 0;
        if (location.document) {
            editor = em->activateEditorForDocument(this, location.document,
                                        EditorManager::IgnoreNavigationHistory | EditorManager::ModeSwitch);
        }
        if (!editor) {
            editor = em->openEditor(this, location.fileName, location.id,
                                    EditorManager::IgnoreNavigationHistory | EditorManager::ModeSwitch);
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
    EditorManager *em = ICore::editorManager();
    updateCurrentPositionInNavigationHistory();
    if (m_currentNavigationHistoryPosition >= m_navigationHistory.size()-1)
        return;
    ++m_currentNavigationHistoryPosition;
    EditLocation location = m_navigationHistory.at(m_currentNavigationHistoryPosition);
    IEditor *editor = 0;
    if (location.document) {
        editor = em->activateEditorForDocument(this, location.document,
                                    EditorManager::IgnoreNavigationHistory | EditorManager::ModeSwitch);
    }
    if (!editor) {
        editor = em->openEditor(this, location.fileName, location.id, EditorManager::IgnoreNavigationHistory);
        if (!editor) {
            //TODO
            qDebug() << Q_FUNC_INFO << "can't open file" << location.fileName;
            return;
        }
    }
    editor->restoreState(location.state.toByteArray());
    updateNavigatorActions();
}


SplitterOrView::SplitterOrView(OpenEditorsModel *model)
{
    Q_ASSERT(model);
    m_isRoot = true;
    m_layout = new QStackedLayout(this);
    m_view = new EditorView();
    m_splitter = 0;
    m_layout->addWidget(m_view);
}

SplitterOrView::SplitterOrView(Core::IEditor *editor)
{
    m_isRoot = false;
    m_layout = new QStackedLayout(this);
    m_view = new EditorView();
    if (editor)
        m_view->addEditor(editor);
    m_splitter = 0;
    m_layout->addWidget(m_view);
}

SplitterOrView::~SplitterOrView()
{
    delete m_layout;
    m_layout = 0;
    delete m_view;
    m_view = 0;
    delete m_splitter;
    m_splitter = 0;
}

void SplitterOrView::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton)
        return;
    setFocus(Qt::MouseFocusReason);
    ICore::editorManager()->setCurrentView(this);
}

void SplitterOrView::paintEvent(QPaintEvent *)
{
    if (ICore::editorManager()->currentSplitterOrView() != this)
        return;

    if (!m_view || hasEditors())
        return;

    // Discreet indication where an editor would be if there is none
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(palette().color(QPalette::Background).darker(107));
    const int r = 3;
    const QRect areaGlobal(view()->editorArea());
    const QRect areaLocal(mapFromGlobal(areaGlobal.topLeft()), areaGlobal.size());
    painter.drawRoundedRect(areaLocal.adjusted(r , r, -r, -r), r * 2, r * 2);
}

SplitterOrView *SplitterOrView::findFirstView()
{
    if (m_splitter) {
        for (int i = 0; i < m_splitter->count(); ++i) {
            if (SplitterOrView *splitterOrView = qobject_cast<SplitterOrView*>(m_splitter->widget(i)))
                if (SplitterOrView *result = splitterOrView->findFirstView())
                    return result;
        }
        return 0;
    }
    return this;
}

SplitterOrView *SplitterOrView::findEmptyView()
{
    if (m_splitter) {
        for (int i = 0; i < m_splitter->count(); ++i) {
            if (SplitterOrView *splitterOrView = qobject_cast<SplitterOrView*>(m_splitter->widget(i)))
                if (SplitterOrView *result = splitterOrView->findEmptyView())
                    return result;
        }
        return 0;
    }
    if (!hasEditors())
        return this;
    return 0;
}

SplitterOrView *SplitterOrView::findView(Core::IEditor *editor)
{
    if (!editor || hasEditor(editor))
        return this;
    if (m_splitter) {
        for (int i = 0; i < m_splitter->count(); ++i) {
            if (SplitterOrView *splitterOrView = qobject_cast<SplitterOrView*>(m_splitter->widget(i)))
                if (SplitterOrView *result = splitterOrView->findView(editor))
                    return result;
        }
    }
    return 0;
}

SplitterOrView *SplitterOrView::findView(EditorView *view)
{
    if (view == m_view)
        return this;
    if (m_splitter) {
        for (int i = 0; i < m_splitter->count(); ++i) {
            if (SplitterOrView *splitterOrView = qobject_cast<SplitterOrView*>(m_splitter->widget(i)))
                if (SplitterOrView *result = splitterOrView->findView(view))
                    return result;
        }
    }
    return 0;
}

SplitterOrView *SplitterOrView::findSplitter(Core::IEditor *editor)
{
    if (m_splitter) {
        for (int i = 0; i < m_splitter->count(); ++i) {
            if (SplitterOrView *splitterOrView = qobject_cast<SplitterOrView*>(m_splitter->widget(i))) {
                if (splitterOrView->hasEditor(editor))
                    return this;
                if (SplitterOrView *result = splitterOrView->findSplitter(editor))
                    return result;
            }
        }
    }
    return 0;
}

SplitterOrView *SplitterOrView::findSplitter(SplitterOrView *child)
{
    if (m_splitter) {
        for (int i = 0; i < m_splitter->count(); ++i) {
            if (SplitterOrView *splitterOrView = qobject_cast<SplitterOrView*>(m_splitter->widget(i))) {
                if (splitterOrView == child)
                    return this;
                if (SplitterOrView *result = splitterOrView->findSplitter(child))
                    return result;
            }
        }
    }
    return 0;
}

SplitterOrView *SplitterOrView::findNextView(SplitterOrView *view)
{
    bool found = false;
    return findNextView_helper(view, &found);
}

SplitterOrView *SplitterOrView::findNextView_helper(SplitterOrView *view, bool *found)
{
    if (*found && m_view)
        return this;

    if (this == view) {
        *found = true;
        return 0;
    }

    if (m_splitter) {
        for (int i = 0; i < m_splitter->count(); ++i) {
            if (SplitterOrView *splitterOrView = qobject_cast<SplitterOrView*>(m_splitter->widget(i))) {
                if (SplitterOrView *result = splitterOrView->findNextView_helper(view, found))
                    return result;
            }
        }
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
    if (m_view)
        m_layout->removeWidget(m_view);
    m_view = 0;
    return oldView;
}

void SplitterOrView::split(Qt::Orientation orientation)
{
    Q_ASSERT(m_view && m_splitter == 0);
    m_splitter = new MiniSplitter(this);
    m_splitter->setOrientation(orientation);
    m_layout->addWidget(m_splitter);
    EditorManager *em = ICore::editorManager();
    Core::IEditor *e = m_view->currentEditor();

    SplitterOrView *view = 0;
    SplitterOrView *otherView = 0;
    if (e) {

        foreach (IEditor *editor, m_view->editors())
            m_view->removeEditor(editor);

        m_splitter->addWidget((view = new SplitterOrView(e)));
        if (e->duplicateSupported()) {
            Core::IEditor *duplicate = em->duplicateEditor(e);
            m_splitter->addWidget((otherView = new SplitterOrView(duplicate)));
        } else {
            m_splitter->addWidget((otherView = new SplitterOrView()));
        }
    } else {
        m_splitter->addWidget((otherView = new SplitterOrView()));
        m_splitter->addWidget((view = new SplitterOrView()));
    }

    m_layout->setCurrentWidget(m_splitter);

    view->view()->copyNavigationHistoryFrom(m_view);
    view->view()->setCurrentEditor(view->view()->currentEditor());
    otherView->view()->copyNavigationHistoryFrom(m_view);
    otherView->view()->setCurrentEditor(otherView->view()->currentEditor());

    if (orientation == Qt::Horizontal) {
        view->view()->setCloseSplitIcon(QIcon(QLatin1String(Constants::ICON_CLOSE_SPLIT_LEFT)));
        otherView->view()->setCloseSplitIcon(QIcon(QLatin1String(Constants::ICON_CLOSE_SPLIT_RIGHT)));
    } else {
        view->view()->setCloseSplitIcon(QIcon(QLatin1String(Constants::ICON_CLOSE_SPLIT_TOP)));
        otherView->view()->setCloseSplitIcon(QIcon(QLatin1String(Constants::ICON_CLOSE_SPLIT_BOTTOM)));
    }

    if (m_view && !m_isRoot) {
        em->emptyView(m_view);
        delete m_view;
        m_view = 0;
    }

    if (e)
        em->activateEditor(view->view(), e);
    else
        em->setCurrentView(view);
}

void SplitterOrView::unsplitAll()
{
    m_splitter->hide();
    m_layout->removeWidget(m_splitter); // workaround Qt bug
    unsplitAll_helper();
    delete m_splitter;
    m_splitter = 0;
}

void SplitterOrView::unsplitAll_helper()
{
    if (!m_isRoot && m_view)
        ICore::editorManager()->emptyView(m_view);
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
    EditorManager *em = ICore::editorManager();
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
            em->emptyView(childView);
        } else {
            m_view = childSplitterOrView->takeView();
            m_layout->addWidget(m_view);
            QSplitter *parentSplitter = qobject_cast<QSplitter *>(parentWidget());
            if (parentSplitter) { // not the toplevel splitterOrView
                if (parentSplitter->orientation() == Qt::Horizontal) {
                    if (parentSplitter->widget(0) == this)
                        m_view->setCloseSplitIcon(QIcon(QLatin1String(Constants::ICON_CLOSE_SPLIT_LEFT)));
                    else
                        m_view->setCloseSplitIcon(QIcon(QLatin1String(Constants::ICON_CLOSE_SPLIT_RIGHT)));
                } else {
                    if (parentSplitter->widget(0) == this)
                        m_view->setCloseSplitIcon(QIcon(QLatin1String(Constants::ICON_CLOSE_SPLIT_TOP)));
                    else
                        m_view->setCloseSplitIcon(QIcon(QLatin1String(Constants::ICON_CLOSE_SPLIT_BOTTOM)));
                }
            }
        }
        m_layout->setCurrentWidget(m_view);
    }
    delete oldSplitter;
    em->setCurrentView(findFirstView());
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
        if (e && (e->isTemporary() || e->document()->fileName().isEmpty())) {
            // look for another editor that is more suited
            e = 0;
            foreach (IEditor *otherEditor, editors()) {
                if (!otherEditor->isTemporary() && !otherEditor->document()->fileName().isEmpty()) {
                    e = otherEditor;
                    break;
                }
            }
        }

        if (!e) {
            stream << QByteArray("empty");
        } else if (e == EditorManager::currentEditor()) {
            stream << QByteArray("currenteditor")
                    << e->document()->fileName() << e->id().toString() << e->saveState();
        } else {
            stream << QByteArray("editor")
                    << e->document()->fileName() << e->id().toString() << e->saveState();
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
        EditorManager *em = ICore::editorManager();
        QString fileName;
        QString id;
        QByteArray editorState;
        stream >> fileName >> id >> editorState;
        if (!QFile::exists(fileName))
            return;
        IEditor *e = em->openEditor(view(), fileName, Id::fromString(id), Core::EditorManager::IgnoreNavigationHistory
                                    | Core::EditorManager::NoActivate);

        if (!e) {
            QModelIndex idx = em->openedEditorsModel()->firstRestoredEditor();
            if (idx.isValid())
                em->activateEditorForIndex(view(), idx, Core::EditorManager::IgnoreNavigationHistory
                                    | Core::EditorManager::NoActivate);
        }

        if (e) {
            e->restoreState(editorState);
            if (mode == "currenteditor")
                em->setCurrentEditor(e);
        }
    }
}
