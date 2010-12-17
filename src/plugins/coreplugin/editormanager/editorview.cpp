/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "editorview.h"
#include "editormanager.h"
#include "coreimpl.h"
#include "minisplitter.h"
#include "openeditorsmodel.h"

#include <coreplugin/editortoolbar.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <coreplugin/findplaceholder.h>
#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QMimeData>

#include <QtGui/QApplication>
#include <QtGui/QComboBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QStackedWidget>
#include <QtGui/QStyle>
#include <QtGui/QStyleOption>
#include <QtGui/QToolButton>
#include <QtGui/QMenu>
#include <QtGui/QClipboard>
#include <QtGui/QAction>
#include <QtGui/QSplitter>
#include <QtGui/QStackedLayout>

#ifdef Q_WS_MAC
#include <qmacstyle_mac.h>
#endif

using namespace Core;
using namespace Core::Internal;


// ================EditorView====================

EditorView::EditorView(QWidget *parent) :
    QWidget(parent),
    m_toolBar(EditorManager::createToolBar(this)),
    m_container(new QStackedWidget(this)),
    m_infoWidget(new QFrame(this)),
    m_editorForInfoWidget(0),
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
        tl->addWidget(m_toolBar);
    }
    {
        QPalette pal = m_infoWidget->palette();
        pal.setColor(QPalette::Window, QColor(255, 255, 225));
        pal.setColor(QPalette::WindowText, Qt::black);

        m_infoWidget->setPalette(pal);
        m_infoWidget->setFrameStyle(QFrame::Panel | QFrame::Raised);
        m_infoWidget->setLineWidth(1);
        m_infoWidget->setAutoFillBackground(true);

        QHBoxLayout *hbox = new QHBoxLayout(m_infoWidget);
        hbox->setMargin(2);
        m_infoWidgetLabel = new QLabel("Placeholder");
        m_infoWidgetLabel->setWordWrap(true);
        hbox->addWidget(m_infoWidgetLabel);

        m_infoWidgetButton = new QToolButton;
        m_infoWidgetButton->setText(tr("Placeholder"));
        hbox->addWidget(m_infoWidgetButton);

        m_infoWidgetCloseButton = new QToolButton;
        m_infoWidgetCloseButton->setAutoRaise(true);
        m_infoWidgetCloseButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_CLEAR)));
        m_infoWidgetCloseButton->setToolTip(tr("Close"));

        hbox->addWidget(m_infoWidgetCloseButton);

        m_infoWidget->setVisible(false);
        tl->addWidget(m_infoWidget);
    }

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
    EditorManager *em = CoreImpl::instance()->editorManager();
    IEditor *editor = currentEditor();
    if (editor)
       em->closeEditor(editor);
}
void EditorView::showEditorInfoBar(const QString &id,
                                   const QString &infoText,
                                   const QString &buttonText,
                                   QObject *object, const char *buttonPressMember,
                                   const char *cancelButtonPressMember)
{
    m_infoWidgetId = id;
    m_infoWidgetLabel->setText(infoText);
    m_infoWidgetButton->setText(buttonText);

    if (object && !buttonText.isEmpty()) {
        m_infoWidgetButton->show();
    } else {
        m_infoWidgetButton->hide();
    }

    m_infoWidgetButton->disconnect();
    if (object && buttonPressMember)
        connect(m_infoWidgetButton, SIGNAL(clicked()), object, buttonPressMember);

    m_infoWidgetCloseButton->disconnect();
    if (object && cancelButtonPressMember)
        connect(m_infoWidgetCloseButton, SIGNAL(clicked()), object, cancelButtonPressMember);
    connect(m_infoWidgetCloseButton, SIGNAL(clicked()), m_infoWidget, SLOT(hide()));

    m_infoWidget->setVisible(true);
    m_editorForInfoWidget = currentEditor();
}

void EditorView::hideEditorInfoBar(const QString &id)
{
    if (id == m_infoWidgetId)
        m_infoWidget->setVisible(false);
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
    EditorManager *em = CoreImpl::instance()->editorManager();
    QAbstractItemModel *model = EditorManager::instance()->openedEditorsModel();
    if (IEditor *editor = model->data(model->index(index, 0), Qt::UserRole).value<IEditor*>()) {
        em->activateEditor(this, editor, Core::EditorManager::ModeSwitch);
    } else {
        em->activateEditor(model->index(index, 0), this, Core::EditorManager::ModeSwitch);
    }
}

void EditorView::setCurrentEditor(IEditor *editor)
{
    // FIXME: this keeps the editor hidden if switching from A to B and back
    if (editor != m_editorForInfoWidget) {
        m_infoWidget->hide();
        m_editorForInfoWidget = 0;
    }

    if (!editor || m_container->count() <= 0
        || m_container->indexOf(editor->widget()) == -1) {
        m_toolBar->updateEditorStatus(0);
        // ### TODO the combo box m_editorList should show an empty item
        return;
    }

    m_editors.removeAll(editor);
    m_editors.append(editor);

    const int idx = m_container->indexOf(editor->widget());
    QTC_ASSERT(idx >= 0, return);
    m_container->setCurrentIndex(idx);
    m_toolBar->setCurrentEditor(editor);

    updateEditorHistory(editor);
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
    IFile *file = editor->file();

    if (!file)
        return;

    QByteArray state = editor->saveState();

    EditLocation location;
    location.file = file;
    location.fileName = file->fileName();
    location.id = editor->id();
    location.state = QVariant(state);

    for(int i = 0; i < m_editorHistory.size(); ++i) {
        if (m_editorHistory.at(i).file == 0
            || m_editorHistory.at(i).file == file
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
    if (editor && editor != currentEditor()) {
        return; // we only save editor sate for the current editor, when the user interacts
    }

    if (!editor)
        editor = currentEditor();
    if (!editor)
        return;
    IFile *file = editor->file();

    if (!file)
        return;

    QByteArray state;
    if (saveState.isNull()) {
        state = editor->saveState();
    } else {
        state = saveState;
    }

    EditLocation location;
    location.file = file;
    location.fileName = file->fileName();
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
    if (!editor || !editor->file())
        return;

    IFile *file = editor->file();
    EditLocation *location;
    if (m_currentNavigationHistoryPosition < m_navigationHistory.size()) {
        location = &m_navigationHistory[m_currentNavigationHistoryPosition];
    } else {
        m_navigationHistory.append(EditLocation());
        location = &m_navigationHistory[m_navigationHistory.size()-1];
    }
    location->file = file;
    location->fileName = file->fileName();
    location->id = editor->id();
    location->state = QVariant(editor->saveState());
}

void EditorView::goBackInNavigationHistory()
{
    EditorManager *em = CoreImpl::instance()->editorManager();
    updateCurrentPositionInNavigationHistory();
    while (m_currentNavigationHistoryPosition > 0) {
        --m_currentNavigationHistoryPosition;
        EditLocation location = m_navigationHistory.at(m_currentNavigationHistoryPosition);
        IEditor *editor;
        if (location.file) {
            editor = em->activateEditor(this, location.file,
                                        EditorManager::IgnoreNavigationHistory | EditorManager::ModeSwitch);
        } else {
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
    EditorManager *em = CoreImpl::instance()->editorManager();
    updateCurrentPositionInNavigationHistory();
    if (m_currentNavigationHistoryPosition >= m_navigationHistory.size()-1)
        return;
    ++m_currentNavigationHistoryPosition;
    EditLocation location = m_navigationHistory.at(m_currentNavigationHistoryPosition);
    IEditor *editor;
    if (location.file) {
        editor = em->activateEditor(this, location.file,
                                    EditorManager::IgnoreNavigationHistory | EditorManager::ModeSwitch);
    } else {
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
    CoreImpl::instance()->editorManager()->setCurrentView(this);
}

void SplitterOrView::paintEvent(QPaintEvent *)
{
    if (CoreImpl::instance()->editorManager()->currentSplitterOrView() != this)
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
    if (*found && m_view) {
        return this;
    }

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
    EditorManager *em = CoreImpl::instance()->editorManager();
    Core::IEditor *e = m_view->currentEditor();

    SplitterOrView *view = 0;
    SplitterOrView *otherView = 0;
    if (e) {

        foreach(IEditor *editor, m_view->editors())
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
        CoreImpl::instance()->editorManager()->emptyView(m_view);
    if (m_splitter) {
        for (int i = 0; i < m_splitter->count(); ++i) {
            if (SplitterOrView *splitterOrView = qobject_cast<SplitterOrView*>(m_splitter->widget(i))) {
                splitterOrView->unsplitAll_helper();
            }
        }
    }
}

void SplitterOrView::unsplit()
{
    if (!m_splitter)
        return;

    Q_ASSERT(m_splitter->count() == 1);
    EditorManager *em = CoreImpl::instance()->editorManager();
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
        EditorManager *em = CoreImpl::instance()->editorManager();

        // don't save state of temporary or ad-hoc editors
        if (e && (e->isTemporary() || e->file()->fileName().isEmpty())) {
            // look for another editor that is more suited
            e = 0;
            foreach (IEditor *otherEditor, editors()) {
                if (!otherEditor->isTemporary() && !otherEditor->file()->fileName().isEmpty()) {
                    e = otherEditor;
                    break;
                }
            }
        }

        if (!e) {
            stream << QByteArray("empty");
        } else if (e == em->currentEditor()) {
            stream << QByteArray("currenteditor")
                    << e->file()->fileName() << e->id() << e->saveState();
        } else {
            stream << QByteArray("editor")
                    << e->file()->fileName() << e->id() << e->saveState();
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
        EditorManager *em = CoreImpl::instance()->editorManager();
        QString fileName;
        QByteArray id;
        QByteArray editorState;
        stream >> fileName >> id >> editorState;
        IEditor *e = em->openEditor(view(), fileName, id, Core::EditorManager::IgnoreNavigationHistory
                                    | Core::EditorManager::NoActivate);

        if (!e) {
            QModelIndex idx = em->openedEditorsModel()->firstRestoredEditor();
            if (idx.isValid())
                em->activateEditor(idx, view(), Core::EditorManager::IgnoreNavigationHistory
                                    | Core::EditorManager::NoActivate);
        }

        if (e) {
            e->restoreState(editorState);
            if (mode == "currenteditor")
                em->setCurrentEditor(e);
        }
    }
}
