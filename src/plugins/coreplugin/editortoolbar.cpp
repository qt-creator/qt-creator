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

#include "editortoolbar.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/sidebar.h>
#include <coreplugin/id.h>

#include <coreplugin/editormanager/editorview.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/openeditorsmodel.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>

#include <utils/hostosinfo.h>
#include <utils/parameteraction.h>
#include <utils/qtcassert.h>
#include <utils/styledbar.h>

#include <QSettings>
#include <QEvent>
#include <QDir>
#include <QPointer>
#include <QApplication>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QTabWidget>
#include <QToolButton>
#include <QMenu>
#include <QClipboard>
#include <QLabel>
#include <QToolBar>

enum {
    debug = false
};

namespace Core {

struct EditorToolBarPrivate {
    explicit EditorToolBarPrivate(QWidget *parent, EditorToolBar *q);

    Core::OpenEditorsModel *m_editorsListModel;
    QComboBox *m_editorList;
    QToolButton *m_closeEditorButton;
    QToolButton *m_lockButton;
    QAction *m_goBackAction;
    QAction *m_goForwardAction;
    QToolButton *m_backButton;
    QToolButton *m_forwardButton;
    QToolButton *m_splitButton;
    QAction *m_horizontalSplitAction;
    QAction *m_verticalSplitAction;
    QToolButton *m_closeSplitButton;

    QWidget *m_activeToolBar;
    QWidget *m_toolBarPlaceholder;
    QWidget *m_defaultToolBar;

    bool m_isStandalone;
};

EditorToolBarPrivate::EditorToolBarPrivate(QWidget *parent, EditorToolBar *q) :
    m_editorList(new QComboBox(q)),
    m_closeEditorButton(new QToolButton),
    m_lockButton(new QToolButton),
    m_goBackAction(new QAction(QIcon(QLatin1String(Constants::ICON_PREV)), EditorManager::tr("Go Back"), parent)),
    m_goForwardAction(new QAction(QIcon(QLatin1String(Constants::ICON_NEXT)), EditorManager::tr("Go Forward"), parent)),
    m_splitButton(new QToolButton),
    m_horizontalSplitAction(new QAction(QIcon(QLatin1String(Constants::ICON_SPLIT_HORIZONTAL)), EditorManager::tr("Split"), parent)),
    m_verticalSplitAction(new QAction(QIcon(QLatin1String(Constants::ICON_SPLIT_VERTICAL)), EditorManager::tr("Split Side by Side"), parent)),
    m_closeSplitButton(new QToolButton),
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

    d->m_lockButton->setAutoRaise(true);
    d->m_lockButton->setEnabled(false);

    d->m_editorsListModel = EditorManager::instance()->openedEditorsModel();
    connect(d->m_goBackAction, SIGNAL(triggered()), this, SIGNAL(goBackClicked()));
    connect(d->m_goForwardAction, SIGNAL(triggered()), this, SIGNAL(goForwardClicked()));

    d->m_editorList->setProperty("hideicon", true);
    d->m_editorList->setProperty("notelideasterisk", true);
    d->m_editorList->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    d->m_editorList->setMinimumContentsLength(20);
    d->m_editorList->setModel(d->m_editorsListModel);
    d->m_editorList->setMaxVisibleItems(40);
    d->m_editorList->setContextMenuPolicy(Qt::CustomContextMenu);

    d->m_closeEditorButton->setAutoRaise(true);
    d->m_closeEditorButton->setIcon(QIcon(QLatin1String(Constants::ICON_CLOSE_DOCUMENT)));
    d->m_closeEditorButton->setEnabled(false);

    d->m_toolBarPlaceholder->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    d->m_backButton = new QToolButton(this);
    d->m_backButton->setDefaultAction(d->m_goBackAction);

    d->m_forwardButton= new QToolButton(this);
    d->m_forwardButton->setDefaultAction(d->m_goForwardAction);

    if (Utils::HostOsInfo::isMacHost()) {
        d->m_horizontalSplitAction->setIconVisibleInMenu(false);
        d->m_verticalSplitAction->setIconVisibleInMenu(false);
    }

    d->m_splitButton->setIcon(QIcon(QLatin1String(Constants::ICON_SPLIT_HORIZONTAL)));
    d->m_splitButton->setToolTip(tr("Split"));
    d->m_splitButton->setPopupMode(QToolButton::InstantPopup);
    d->m_splitButton->setProperty("noArrow", true);
    QMenu *splitMenu = new QMenu(d->m_splitButton);
    splitMenu->addAction(d->m_horizontalSplitAction);
    splitMenu->addAction(d->m_verticalSplitAction);
    d->m_splitButton->setMenu(splitMenu);

    d->m_closeSplitButton->setAutoRaise(true);
    d->m_closeSplitButton->setIcon(QIcon(QLatin1String(Constants::ICON_CLOSE_SPLIT_BOTTOM)));

    QHBoxLayout *toplayout = new QHBoxLayout(this);
    toplayout->setSpacing(0);
    toplayout->setMargin(0);
    toplayout->addWidget(d->m_backButton);
    toplayout->addWidget(d->m_forwardButton);
    toplayout->addWidget(d->m_lockButton);
    toplayout->addWidget(d->m_editorList);
    toplayout->addWidget(d->m_toolBarPlaceholder, 1); // Custom toolbar stretches
    toplayout->addWidget(d->m_splitButton);
    toplayout->addWidget(d->m_closeSplitButton);
    toplayout->addWidget(d->m_closeEditorButton);

    setLayout(toplayout);

    // this signal is disconnected for standalone toolbars and replaced with
    // a private slot connection
    connect(d->m_editorList, SIGNAL(activated(int)), this, SIGNAL(listSelectionActivated(int)));

    connect(d->m_editorList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(listContextMenu(QPoint)));
    connect(d->m_lockButton, SIGNAL(clicked()), this, SLOT(makeEditorWritable()));
    connect(d->m_closeEditorButton, SIGNAL(clicked()), this, SLOT(closeEditor()), Qt::QueuedConnection);
    connect(d->m_horizontalSplitAction, SIGNAL(triggered()),
            this, SIGNAL(horizontalSplitClicked()), Qt::QueuedConnection);
    connect(d->m_verticalSplitAction, SIGNAL(triggered()),
            this, SIGNAL(verticalSplitClicked()), Qt::QueuedConnection);
    connect(d->m_closeSplitButton, SIGNAL(clicked()),
            this, SIGNAL(closeSplitClicked()), Qt::QueuedConnection);


    connect(ActionManager::command(Constants::CLOSE), SIGNAL(keySequenceChanged()),
            this, SLOT(updateActionShortcuts()));
    connect(ActionManager::command(Constants::GO_BACK), SIGNAL(keySequenceChanged()),
            this, SLOT(updateActionShortcuts()));
    connect(ActionManager::command(Constants::GO_FORWARD), SIGNAL(keySequenceChanged()),
            this, SLOT(updateActionShortcuts()));

    updateActionShortcuts();
}

EditorToolBar::~EditorToolBar()
{
    delete d;
}

void EditorToolBar::removeToolbarForEditor(IEditor *editor)
{
    QTC_ASSERT(editor, return);
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

void EditorToolBar::setCloseSplitEnabled(bool enable)
{
    d->m_closeSplitButton->setVisible(enable);
}

void EditorToolBar::setCloseSplitIcon(const QIcon &icon)
{
    d->m_closeSplitButton->setIcon(icon);
}

void EditorToolBar::closeEditor()
{
    IEditor *current = EditorManager::currentEditor();
    if (!current)
        return;

    if (d->m_isStandalone)
        EditorManager::instance()->closeEditor(current);
    emit closeClicked();
}

void EditorToolBar::addEditor(IEditor *editor)
{
    QTC_ASSERT(editor, return);
    connect(editor, SIGNAL(changed()), this, SLOT(checkEditorStatus()));
    QWidget *toolBar = editor->toolBar();

    if (toolBar && !d->m_isStandalone)
        addCenterToolBar(toolBar);

    updateEditorStatus(editor);
}

void EditorToolBar::addCenterToolBar(QWidget *toolBar)
{
    QTC_ASSERT(toolBar, return);
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
        d->m_splitButton->setVisible(false);
        d->m_closeSplitButton->setVisible(false);
    }
}

void EditorToolBar::setCurrentEditor(IEditor *editor)
{
    QTC_ASSERT(editor, return);
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
    EditorManager *em = ICore::editorManager();
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
    QAction *copyPath = menu.addAction(tr("Copy Full Path to Clipboard"));
    menu.addSeparator();
    EditorManager::instance()->addSaveAndCloseEditorActions(&menu, index);
    menu.addSeparator();
    EditorManager::instance()->addNativeDirActions(&menu, index);
    QAction *result = menu.exec(d->m_editorList->mapToGlobal(pos));
    if (result == copyPath)
        QApplication::clipboard()->setText(QDir::toNativeSeparators(fileName));
}

void EditorToolBar::makeEditorWritable()
{
    if (IEditor *current = EditorManager::currentEditor())
        EditorManager::instance()->makeFileWritable(current->document());
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
    d->m_closeEditorButton->setToolTip(ActionManager::command(Constants::CLOSE)->stringWithAppendedShortcut(EditorManager::tr("Close Document")));
    d->m_goBackAction->setToolTip(ActionManager::command(Constants::GO_BACK)->action()->toolTip());
    d->m_goForwardAction->setToolTip(ActionManager::command(Constants::GO_FORWARD)->action()->toolTip());
    d->m_closeSplitButton->setToolTip(ActionManager::command(Constants::REMOVE_CURRENT_SPLIT)->stringWithAppendedShortcut(tr("Remove Split")));
}

void EditorToolBar::checkEditorStatus()
{
    IEditor *editor = qobject_cast<IEditor *>(sender());
    IEditor *current = EditorManager::currentEditor();

    if (current == editor)
        updateEditorStatus(editor);
}

void EditorToolBar::updateEditorStatus(IEditor *editor)
{
    d->m_closeEditorButton->setEnabled(editor != 0);

    if (!editor || !editor->document()) {
        d->m_lockButton->setIcon(QIcon());
        d->m_lockButton->setEnabled(false);
        d->m_lockButton->setToolTip(QString());
        d->m_editorList->setToolTip(QString());
        return;
    }

    d->m_editorList->setCurrentIndex(d->m_editorsListModel->indexOf(editor).row());

    if (editor->document()->fileName().isEmpty()) {
        d->m_lockButton->setIcon(QIcon());
        d->m_lockButton->setEnabled(false);
        d->m_lockButton->setToolTip(QString());
    } else if (editor->document()->isFileReadOnly()) {
        d->m_lockButton->setIcon(QIcon(d->m_editorsListModel->lockedIcon()));
        d->m_lockButton->setEnabled(true);
        d->m_lockButton->setToolTip(tr("Make Writable"));
    } else {
        d->m_lockButton->setIcon(QIcon(d->m_editorsListModel->unlockedIcon()));
        d->m_lockButton->setEnabled(false);
        d->m_lockButton->setToolTip(tr("File is writable"));
    }
    IEditor *current = EditorManager::currentEditor();
    if (editor == current)
        d->m_editorList->setToolTip(
                current->document()->fileName().isEmpty()
                ? current->displayName()
                    : QDir::toNativeSeparators(editor->document()->fileName())
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
