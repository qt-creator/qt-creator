/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "designmodetoolbar.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/sidebar.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/openeditorsmodel.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/modemanager.h>

#include <utils/parameteraction.h>
#include <utils/qtcassert.h>
#include <utils/styledbar.h>

#include <QtCore/QSettings>
#include <QtCore/QEvent>
#include <QtCore/QDir>
#include <QtGui/QApplication>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QVBoxLayout>
#include <QtGui/QScrollArea>
#include <QtGui/QTabWidget>
#include <QtGui/QToolButton>
#include <QtGui/QMenu>
#include <QtGui/QClipboard>
#include <QtGui/QLabel>
#include <QtGui/QToolBar>

Q_DECLARE_METATYPE(Core::IEditor*)

enum {
    debug = false
};

namespace Core {

/*!
  Mimic the look of the text editor toolbar as defined in e.g. EditorView::EditorView
  */
EditorToolBar::EditorToolBar(QWidget *parent) :
        Utils::StyledBar(parent),
        m_editorList(new QComboBox(this)),
        m_rightToolBar(new QToolBar(this)),
        m_closeButton(new QToolButton),
        m_lockButton(new QToolButton),

        m_goBackAction(new QAction(QIcon(QLatin1String(":/help/images/previous.png")), EditorManager::tr("Go Back"), parent)),
        m_goForwardAction(new QAction(QIcon(QLatin1String(":/help/images/next.png")), EditorManager::tr("Go Forward"), parent)),

        m_activeToolBar(0),
        m_toolBarPlaceholder(new QWidget),
        m_defaultToolBar(new QWidget(this)),
        m_ignoreEditorToolbar(false)
{
    QHBoxLayout *toolBarLayout = new QHBoxLayout(this);
    toolBarLayout->setMargin(0);
    toolBarLayout->setSpacing(0);
    toolBarLayout->addWidget(m_defaultToolBar);
    m_toolBarPlaceholder->setLayout(toolBarLayout);
    m_toolBarPlaceholder->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    m_defaultToolBar->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    m_activeToolBar = m_defaultToolBar;

    m_editorsListModel = EditorManager::instance()->openedEditorsModel();
    connect(m_goBackAction, SIGNAL(triggered()), this, SIGNAL(goBackClicked()));
    connect(m_goForwardAction, SIGNAL(triggered()), this, SIGNAL(goForwardClicked()));

    m_editorList->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_editorList->setMinimumContentsLength(20);
    m_editorList->setModel(m_editorsListModel);
    m_editorList->setMaxVisibleItems(40);
    m_editorList->setContextMenuPolicy(Qt::CustomContextMenu);

    m_lockButton->setAutoRaise(true);
    m_lockButton->setProperty("type", QLatin1String("dockbutton"));
    m_lockButton->setVisible(false);

    m_closeButton->setAutoRaise(true);
    m_closeButton->setIcon(QIcon(":/core/images/closebutton.png"));
    m_closeButton->setProperty("type", QLatin1String("dockbutton"));
    m_closeButton->setEnabled(false);

    m_toolBarPlaceholder->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    m_backButton = new QToolButton(this);
    m_backButton->setDefaultAction(m_goBackAction);

    m_forwardButton= new QToolButton(this);
    m_forwardButton->setDefaultAction(m_goForwardAction);

    m_rightToolBar->setLayoutDirection(Qt::RightToLeft);
    m_rightToolBar->addWidget(m_closeButton);
    m_rightToolBar->addWidget(m_lockButton);

    QHBoxLayout *toplayout = new QHBoxLayout(this);
    toplayout->setSpacing(0);
    toplayout->setMargin(0);
    toplayout->addWidget(m_backButton);
    toplayout->addWidget(m_forwardButton);
    toplayout->addWidget(m_editorList);
    toplayout->addWidget(m_toolBarPlaceholder, 1); // Custom toolbar stretches
    toplayout->addWidget(m_rightToolBar);

    setLayout(toplayout);

    connect(m_editorList, SIGNAL(activated(int)), this, SLOT(listSelectionActivated(int)));
    connect(m_editorList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(listContextMenu(QPoint)));
    connect(m_lockButton, SIGNAL(clicked()), this, SLOT(makeEditorWritable()));
    connect(m_closeButton, SIGNAL(clicked()), this, SLOT(closeView()), Qt::QueuedConnection);

    EditorManager *em = EditorManager::instance();
    connect(em, SIGNAL(currentEditorChanged(Core::IEditor*)), SLOT(updateEditorListSelection(Core::IEditor*)));

    ActionManager *am = ICore::instance()->actionManager();
    connect(am->command(Constants::CLOSE), SIGNAL(keySequenceChanged()),
            this, SLOT(updateActionShortcuts()));
    connect(am->command(Constants::GO_BACK), SIGNAL(keySequenceChanged()),
            this, SLOT(updateActionShortcuts()));
    connect(am->command(Constants::GO_FORWARD), SIGNAL(keySequenceChanged()),
            this, SLOT(updateActionShortcuts()));

}

void EditorToolBar::removeToolbarForEditor(IEditor *editor)
{
    disconnect(editor, SIGNAL(changed()), this, SLOT(checkEditorStatus()));

    QWidget *toolBar = editor->toolBar();
    if (toolBar != 0) {
        if (m_activeToolBar == toolBar) {
            m_activeToolBar = m_defaultToolBar;
            m_activeToolBar->setVisible(true);
        }
        m_toolBarPlaceholder->layout()->removeWidget(toolBar);
        toolBar->setVisible(false);
        toolBar->setParent(0);
    }
}

void EditorToolBar::closeView()
{
    if (!currentEditor())
        return;

    EditorManager *em = ICore::instance()->editorManager();
    if (IEditor *editor = currentEditor()) {
            em->closeDuplicate(editor);
    }
    emit closeClicked();
}

void EditorToolBar::addEditor(IEditor *editor)
{
    connect(editor, SIGNAL(changed()), this, SLOT(checkEditorStatus()));
    QWidget *toolBar = editor->toolBar();

    if (toolBar && !m_ignoreEditorToolbar)
        addCenterToolBar(toolBar);

    updateEditorStatus(editor);
}

void EditorToolBar::addCenterToolBar(QWidget *toolBar)
{
    toolBar->setVisible(false); // will be made visible in setCurrentEditor
    m_toolBarPlaceholder->layout()->addWidget(toolBar);

    updateToolBar(toolBar);
}

void EditorToolBar::updateToolBar(QWidget *toolBar)
{
    if (!toolBar)
        toolBar = m_defaultToolBar;
    if (m_activeToolBar == toolBar)
        return;
    toolBar->setVisible(true);
    m_activeToolBar->setVisible(false);
    m_activeToolBar = toolBar;
}

void EditorToolBar::setToolbarCreationFlags(ToolbarCreationFlags flags)
{
    m_ignoreEditorToolbar = flags & FlagsIgnoreIEditorToolBar;
}

void EditorToolBar::setCurrentEditor(IEditor *editor)
{
    m_editorList->setCurrentIndex(m_editorsListModel->indexOf(editor).row());

    // If we never added the toolbar from the editor,  we will never change
    // the editor, so there's no need to update the toolbar either.
    if (!m_ignoreEditorToolbar)
        updateToolBar(editor->toolBar());

    updateEditorStatus(editor);
}

void EditorToolBar::updateEditorListSelection(IEditor *newSelection)
{
    if (newSelection) {
        m_editorList->setCurrentIndex(m_editorsListModel->indexOf(newSelection).row());
    }
}

void EditorToolBar::listSelectionActivated(int row)
{
    EditorManager *em = ICore::instance()->editorManager();
    QAbstractItemModel *model = m_editorList->model();
    const QModelIndex modelIndex = model->index(row, 0);
    IEditor *editor = model->data(modelIndex, Qt::UserRole).value<IEditor*>();
    if (editor) {
        if (editor != em->currentEditor())
            em->activateEditor(editor, EditorManager::NoModeSwitch);
    } else {
        //em->activateEditor(model->index(index, 0), this);
        QString fileName = model->data(modelIndex, Qt::UserRole + 1).toString();
        QByteArray kind = model->data(modelIndex, Qt::UserRole + 2).toByteArray();
        editor = em->openEditor(fileName, kind, EditorManager::NoModeSwitch);
    }
    if (editor) {
        m_editorList->setCurrentIndex(m_editorsListModel->indexOf(editor).row());
    }
}

void EditorToolBar::listContextMenu(QPoint pos)
{
    QModelIndex index = m_editorsListModel->index(m_editorList->currentIndex(), 0);
    QString fileName = m_editorsListModel->data(index, Qt::UserRole + 1).toString();
    if (fileName.isEmpty())
        return;
    QMenu menu;
    menu.addAction(tr("Copy full path to clipboard"));
    if (menu.exec(m_editorList->mapToGlobal(pos))) {
        QApplication::clipboard()->setText(fileName);
    }
}

void EditorToolBar::makeEditorWritable()
{
    if (currentEditor())
        ICore::instance()->editorManager()->makeEditorWritable(currentEditor());
}

void EditorToolBar::setCanGoBack(bool canGoBack)
{
    m_goBackAction->setEnabled(canGoBack);
}

void EditorToolBar::setCanGoForward(bool canGoForward)
{
    m_goForwardAction->setEnabled(canGoForward);
}

void EditorToolBar::updateActionShortcuts()
{
    ActionManager *am = ICore::instance()->actionManager();
    m_closeButton->setToolTip(am->command(Constants::CLOSE)->stringWithAppendedShortcut(EditorManager::tr("Close")));
    m_goBackAction->setToolTip(am->command(Constants::GO_BACK)->action()->toolTip());
    m_goForwardAction->setToolTip(am->command(Constants::GO_FORWARD)->action()->toolTip());
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
    m_lockButton->setVisible(editor != 0);
    m_closeButton->setEnabled(editor != 0);

    if (!editor || !editor->file()) {
        m_editorList->setToolTip(QString());
        return;
    }

    m_editorList->setCurrentIndex(m_editorsListModel->indexOf(editor).row());

    if (editor->file()->isReadOnly()) {
        m_lockButton->setIcon(QIcon(m_editorsListModel->lockedIcon()));
        m_lockButton->setEnabled(!editor->file()->fileName().isEmpty());
        m_lockButton->setToolTip(tr("Make writable"));
    } else {
        m_lockButton->setIcon(QIcon(m_editorsListModel->unlockedIcon()));
        m_lockButton->setEnabled(false);
        m_lockButton->setToolTip(tr("File is writable"));
    }
    if (editor == currentEditor())
        m_editorList->setToolTip(
                currentEditor()->file()->fileName().isEmpty()
                ? currentEditor()->displayName()
                    : QDir::toNativeSeparators(editor->file()->fileName())
                    );

}

void EditorToolBar::setNavigationVisible(bool isVisible)
{
    m_goBackAction->setVisible(isVisible);
    m_goForwardAction->setVisible(isVisible);
    m_backButton->setVisible(isVisible);
    m_forwardButton->setVisible(isVisible);
}

} // Core
