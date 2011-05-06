/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "editortoolbar.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/sidebar.h>
#include <coreplugin/uniqueidmanager.h>

#include <coreplugin/editormanager/editorview.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/openeditorsmodel.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>

#include <utils/parameteraction.h>
#include <utils/qtcassert.h>
#include <utils/styledbar.h>

#include <QtCore/QSettings>
#include <QtCore/QEvent>
#include <QtCore/QDir>
#include <QtCore/QPointer>
#include <QtGui/QApplication>
#include <QtGui/QComboBox>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QVBoxLayout>
#include <QtGui/QScrollArea>
#include <QtGui/QTabWidget>
#include <QtGui/QToolButton>
#include <QtGui/QMenu>
#include <QtGui/QClipboard>
#include <QtGui/QLabel>
#include <QtGui/QToolBar>

enum {
    debug = false
};

namespace Core {

struct EditorToolBarPrivate {
    explicit EditorToolBarPrivate(QWidget *parent, EditorToolBar *q);

    Core::OpenEditorsModel *m_editorsListModel;
    QComboBox *m_editorList;
    QToolButton *m_closeButton;
    QToolButton *m_lockButton;
    QAction *m_goBackAction;
    QAction *m_goForwardAction;
    QToolButton *m_backButton;
    QToolButton *m_forwardButton;

    QWidget *m_activeToolBar;
    QWidget *m_toolBarPlaceholder;
    QWidget *m_defaultToolBar;

    bool m_isStandalone;
};

EditorToolBarPrivate::EditorToolBarPrivate(QWidget *parent, EditorToolBar *q) :
    m_editorList(new QComboBox(q)),
    m_closeButton(new QToolButton),
    m_lockButton(new QToolButton),
    m_goBackAction(new QAction(QIcon(QLatin1String(Constants::ICON_PREV)), EditorManager::tr("Go Back"), parent)),
    m_goForwardAction(new QAction(QIcon(QLatin1String(Constants::ICON_NEXT)), EditorManager::tr("Go Forward"), parent)),
    m_activeToolBar(0),
    m_toolBarPlaceholder(new QWidget),
    m_defaultToolBar(new QWidget(q)),
    m_isStandalone(false)
{
}

/*!
  Mimic the look of the text editor toolbar as defined in e.g. EditorView::EditorView
  */
EditorToolBar::EditorToolBar(QWidget *parent) :
        Utils::StyledBar(parent), d(new EditorToolBarPrivate(parent, this))
{
    QHBoxLayout *toolBarLayout = new QHBoxLayout(this);
    toolBarLayout->setMargin(0);
    toolBarLayout->setSpacing(0);
    toolBarLayout->addWidget(d->m_defaultToolBar);
    d->m_toolBarPlaceholder->setLayout(toolBarLayout);
    d->m_toolBarPlaceholder->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    d->m_defaultToolBar->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    d->m_activeToolBar = d->m_defaultToolBar;

    d->m_editorsListModel = EditorManager::instance()->openedEditorsModel();
    connect(d->m_goBackAction, SIGNAL(triggered()), this, SIGNAL(goBackClicked()));
    connect(d->m_goForwardAction, SIGNAL(triggered()), this, SIGNAL(goForwardClicked()));

    d->m_editorList->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    d->m_editorList->setMinimumContentsLength(20);
    d->m_editorList->setModel(d->m_editorsListModel);
    d->m_editorList->setMaxVisibleItems(40);
    d->m_editorList->setContextMenuPolicy(Qt::CustomContextMenu);

    d->m_lockButton->setAutoRaise(true);
    d->m_lockButton->setProperty("type", QLatin1String("dockbutton"));
    d->m_lockButton->setVisible(false);

    d->m_closeButton->setAutoRaise(true);
    d->m_closeButton->setIcon(QIcon(QLatin1String(Constants::ICON_CLOSE)));
    d->m_closeButton->setProperty("type", QLatin1String("dockbutton"));
    d->m_closeButton->setEnabled(false);

    d->m_toolBarPlaceholder->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    d->m_backButton = new QToolButton(this);
    d->m_backButton->setDefaultAction(d->m_goBackAction);

    d->m_forwardButton= new QToolButton(this);
    d->m_forwardButton->setDefaultAction(d->m_goForwardAction);

    QHBoxLayout *toplayout = new QHBoxLayout(this);
    toplayout->setSpacing(0);
    toplayout->setMargin(0);
    toplayout->addWidget(d->m_backButton);
    toplayout->addWidget(d->m_forwardButton);
    toplayout->addWidget(d->m_editorList);
    toplayout->addWidget(d->m_toolBarPlaceholder, 1); // Custom toolbar stretches
    toplayout->addWidget(d->m_lockButton);
    toplayout->addWidget(d->m_closeButton);

    setLayout(toplayout);

    // this signal is disconnected for standalone toolbars and replaced with
    // a private slot connection
    connect(d->m_editorList, SIGNAL(activated(int)), this, SIGNAL(listSelectionActivated(int)));

    connect(d->m_editorList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(listContextMenu(QPoint)));
    connect(d->m_lockButton, SIGNAL(clicked()), this, SLOT(makeEditorWritable()));
    connect(d->m_closeButton, SIGNAL(clicked()), this, SLOT(closeView()), Qt::QueuedConnection);

    ActionManager *am = ICore::instance()->actionManager();
    connect(am->command(Constants::CLOSE), SIGNAL(keySequenceChanged()),
            this, SLOT(updateActionShortcuts()));
    connect(am->command(Constants::GO_BACK), SIGNAL(keySequenceChanged()),
            this, SLOT(updateActionShortcuts()));
    connect(am->command(Constants::GO_FORWARD), SIGNAL(keySequenceChanged()),
            this, SLOT(updateActionShortcuts()));

}

EditorToolBar::~EditorToolBar()
{
}

void EditorToolBar::removeToolbarForEditor(IEditor *editor)
{
    QTC_ASSERT(editor, return)
    disconnect(editor, SIGNAL(changed()), this, SLOT(checkEditorStatus()));

    QWidget *toolBar = editor->toolBar();
    if (toolBar != 0) {
        if (d->m_activeToolBar == toolBar) {
            d->m_activeToolBar = d->m_defaultToolBar;
            d->m_activeToolBar->setVisible(true);
        }
        d->m_toolBarPlaceholder->layout()->removeWidget(toolBar);
        toolBar->setVisible(false);
        toolBar->setParent(0);
    }
}

void EditorToolBar::closeView()
{
    if (!currentEditor())
        return;

    if (d->m_isStandalone) {
        EditorManager *em = ICore::instance()->editorManager();
        if (IEditor *editor = currentEditor()) {
            em->closeEditor(editor);
        }
    }
    emit closeClicked();
}

void EditorToolBar::addEditor(IEditor *editor)
{
    QTC_ASSERT(editor, return)
    connect(editor, SIGNAL(changed()), this, SLOT(checkEditorStatus()));
    QWidget *toolBar = editor->toolBar();

    if (toolBar && !d->m_isStandalone)
        addCenterToolBar(toolBar);

    updateEditorStatus(editor);
}

void EditorToolBar::addCenterToolBar(QWidget *toolBar)
{
    QTC_ASSERT(toolBar, return)
    toolBar->setVisible(false); // will be made visible in setCurrentEditor
    d->m_toolBarPlaceholder->layout()->addWidget(toolBar);

    updateToolBar(toolBar);
}

void EditorToolBar::updateToolBar(QWidget *toolBar)
{
    if (!toolBar)
        toolBar = d->m_defaultToolBar;
    if (d->m_activeToolBar == toolBar)
        return;
    toolBar->setVisible(true);
    d->m_activeToolBar->setVisible(false);
    d->m_activeToolBar = toolBar;
}

void EditorToolBar::setToolbarCreationFlags(ToolbarCreationFlags flags)
{
    d->m_isStandalone = flags & FlagsStandalone;
    if (d->m_isStandalone) {
        EditorManager *em = EditorManager::instance();
        connect(em, SIGNAL(currentEditorChanged(Core::IEditor*)), SLOT(updateEditorListSelection(Core::IEditor*)));

        disconnect(d->m_editorList, SIGNAL(activated(int)), this, SIGNAL(listSelectionActivated(int)));
        connect(d->m_editorList, SIGNAL(activated(int)), this, SLOT(changeActiveEditor(int)));
    }
}

void EditorToolBar::setCurrentEditor(IEditor *editor)
{
    QTC_ASSERT(editor, return)
    d->m_editorList->setCurrentIndex(d->m_editorsListModel->indexOf(editor).row());

    // If we never added the toolbar from the editor,  we will never change
    // the editor, so there's no need to update the toolbar either.
    if (!d->m_isStandalone)
        updateToolBar(editor->toolBar());

    updateEditorStatus(editor);
}

void EditorToolBar::updateEditorListSelection(IEditor *newSelection)
{
    if (newSelection)
        d->m_editorList->setCurrentIndex(d->m_editorsListModel->indexOf(newSelection).row());
}

void EditorToolBar::changeActiveEditor(int row)
{
    EditorManager *em = ICore::instance()->editorManager();
    QAbstractItemModel *model = d->m_editorList->model();
    em->activateEditorForIndex(model->index(row, 0), EditorManager::ModeSwitch);
}

void EditorToolBar::listContextMenu(QPoint pos)
{
    QModelIndex index = d->m_editorsListModel->index(d->m_editorList->currentIndex(), 0);
    QString fileName = d->m_editorsListModel->data(index, Qt::UserRole + 1).toString();
    if (fileName.isEmpty())
        return;
    QMenu menu;
    menu.addAction(tr("Copy Full Path to Clipboard"));
    if (menu.exec(d->m_editorList->mapToGlobal(pos))) {
        QApplication::clipboard()->setText(QDir::toNativeSeparators(fileName));
    }
}

void EditorToolBar::makeEditorWritable()
{
    if (currentEditor())
        ICore::instance()->editorManager()->makeFileWritable(currentEditor()->file());
}

void EditorToolBar::setCanGoBack(bool canGoBack)
{
    d->m_goBackAction->setEnabled(canGoBack);
}

void EditorToolBar::setCanGoForward(bool canGoForward)
{
    d->m_goForwardAction->setEnabled(canGoForward);
}

void EditorToolBar::updateActionShortcuts()
{
    ActionManager *am = ICore::instance()->actionManager();
    d->m_closeButton->setToolTip(am->command(Constants::CLOSE)->stringWithAppendedShortcut(EditorManager::tr("Close")));
    d->m_goBackAction->setToolTip(am->command(Constants::GO_BACK)->action()->toolTip());
    d->m_goForwardAction->setToolTip(am->command(Constants::GO_FORWARD)->action()->toolTip());
}

IEditor *EditorToolBar::currentEditor() const
{
    return ICore::instance()->editorManager()->currentEditor();
}

void EditorToolBar::checkEditorStatus()
{
    IEditor *editor = qobject_cast<IEditor *>(sender());
    IEditor *current = currentEditor();

    if (current == editor)
        updateEditorStatus(editor);
}

void EditorToolBar::updateEditorStatus(IEditor *editor)
{
    d->m_lockButton->setVisible(editor != 0);
    d->m_closeButton->setEnabled(editor != 0);

    if (!editor || !editor->file()) {
        d->m_editorList->setToolTip(QString());
        return;
    }

    d->m_editorList->setCurrentIndex(d->m_editorsListModel->indexOf(editor).row());

    if (editor->file()->isReadOnly()) {
        d->m_lockButton->setIcon(QIcon(d->m_editorsListModel->lockedIcon()));
        d->m_lockButton->setEnabled(!editor->file()->fileName().isEmpty());
        d->m_lockButton->setToolTip(tr("Make writable"));
    } else {
        d->m_lockButton->setIcon(QIcon(d->m_editorsListModel->unlockedIcon()));
        d->m_lockButton->setEnabled(false);
        d->m_lockButton->setToolTip(tr("File is writable"));
    }
    if (editor == currentEditor())
        d->m_editorList->setToolTip(
                currentEditor()->file()->fileName().isEmpty()
                ? currentEditor()->displayName()
                    : QDir::toNativeSeparators(editor->file()->fileName())
                    );

}

void EditorToolBar::setNavigationVisible(bool isVisible)
{
    d->m_goBackAction->setVisible(isVisible);
    d->m_goForwardAction->setVisible(isVisible);
    d->m_backButton->setVisible(isVisible);
    d->m_forwardButton->setVisible(isVisible);
}

} // Core
