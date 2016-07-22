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

#include "bookmarksplugin.h"
#include "bookmarkmanager.h"
#include "bookmarks_global.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/id.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditorconstants.h>

#include <QtPlugin>

#include <QMenu>

using namespace Bookmarks::Constants;
using namespace Core;
using namespace TextEditor;

namespace Bookmarks {
namespace Internal {

BookmarksPlugin::BookmarksPlugin() :
    m_bookmarkManager(0),
    m_toggleAction(0),
    m_prevAction(0),
    m_nextAction(0),
    m_docPrevAction(0),
    m_docNextAction(0),
    m_editBookmarkAction(0),
    m_bookmarkMarginAction(0),
    m_bookmarkMarginActionLineNumber(0)
{
}

bool BookmarksPlugin::initialize(const QStringList & /*arguments*/, QString *)
{
    ActionContainer *mtools = ActionManager::actionContainer(Core::Constants::M_TOOLS);
    ActionContainer *mbm = ActionManager::createMenu(Id(BOOKMARKS_MENU));
    mbm->menu()->setTitle(tr("&Bookmarks"));
    mtools->addMenu(mbm);

    const Context editorManagerContext(Core::Constants::C_EDITORMANAGER);

    //Toggle
    m_toggleAction = new QAction(tr("Toggle Bookmark"), this);
    Command *cmd = ActionManager::registerAction(m_toggleAction, BOOKMARKS_TOGGLE_ACTION, editorManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+M") : tr("Ctrl+M")));
    mbm->addAction(cmd);

    mbm->addSeparator();

    //Previous
    m_prevAction = new QAction(tr("Previous Bookmark"), this);
    cmd = ActionManager::registerAction(m_prevAction, BOOKMARKS_PREV_ACTION, editorManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+,") : tr("Ctrl+,")));
    mbm->addAction(cmd);

    //Next
    m_nextAction = new QAction(tr("Next Bookmark"), this);
    cmd = ActionManager::registerAction(m_nextAction, BOOKMARKS_NEXT_ACTION, editorManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+.") : tr("Ctrl+.")));
    mbm->addAction(cmd);

    mbm->addSeparator();

    //Previous Doc
    m_docPrevAction = new QAction(tr("Previous Bookmark in Document"), this);
    cmd = ActionManager::registerAction(m_docPrevAction, BOOKMARKS_PREVDOC_ACTION, editorManagerContext);
    mbm->addAction(cmd);

    //Next Doc
    m_docNextAction = new QAction(tr("Next Bookmark in Document"), this);
    cmd = ActionManager::registerAction(m_docNextAction, BOOKMARKS_NEXTDOC_ACTION, editorManagerContext);
    mbm->addAction(cmd);

    m_editBookmarkAction = new QAction(tr("Edit Bookmark"), this);

    m_bookmarkManager = new BookmarkManager;

    connect(m_toggleAction, &QAction::triggered, [this]() {
        BaseTextEditor *editor = BaseTextEditor::currentTextEditor();
        if (editor && !editor->document()->isTemporary())
            m_bookmarkManager->toggleBookmark(editor->document()->filePath().toString(), editor->currentLine());
    });

    connect(m_prevAction, &QAction::triggered, m_bookmarkManager, &BookmarkManager::prev);
    connect(m_nextAction, &QAction::triggered, m_bookmarkManager, &BookmarkManager::next);
    connect(m_docPrevAction, &QAction::triggered, m_bookmarkManager, &BookmarkManager::prevInDocument);
    connect(m_docNextAction, &QAction::triggered, m_bookmarkManager, &BookmarkManager::nextInDocument);

    connect(m_editBookmarkAction, &QAction::triggered, [this]() {
            m_bookmarkManager->editByFileAndLine(m_bookmarkMarginActionFileName, m_bookmarkMarginActionLineNumber);
    });

    connect(m_bookmarkManager, &BookmarkManager::updateActions, this, &BookmarksPlugin::updateActions);
    updateActions(false, m_bookmarkManager->state());
    addAutoReleasedObject(new BookmarkViewFactory(m_bookmarkManager));

    m_bookmarkMarginAction = new QAction(this);
    m_bookmarkMarginAction->setText(tr("Toggle Bookmark"));
    connect(m_bookmarkMarginAction, &QAction::triggered, [this]() {
            m_bookmarkManager->toggleBookmark(m_bookmarkMarginActionFileName,
                                              m_bookmarkMarginActionLineNumber);
    });

    // EditorManager
    connect(EditorManager::instance(), &EditorManager::editorAboutToClose,
        this, &BookmarksPlugin::editorAboutToClose);
    connect(EditorManager::instance(), &EditorManager::editorOpened,
        this, &BookmarksPlugin::editorOpened);

    return true;
}

BookmarksPlugin::~BookmarksPlugin()
{
    delete m_bookmarkManager;
}

void BookmarksPlugin::updateActions(bool enableToggle, int state)
{
    const bool hasbm    = state >= BookmarkManager::HasBookMarks;
    const bool hasdocbm = state == BookmarkManager::HasBookmarksInDocument;

    m_toggleAction->setEnabled(enableToggle);
    m_prevAction->setEnabled(hasbm);
    m_nextAction->setEnabled(hasbm);
    m_docPrevAction->setEnabled(hasdocbm);
    m_docNextAction->setEnabled(hasdocbm);
}

void BookmarksPlugin::editorOpened(IEditor *editor)
{
    if (auto widget = qobject_cast<TextEditorWidget *>(editor->widget())) {
        connect(widget, &TextEditorWidget::markRequested, m_bookmarkManager,
                [this, editor](TextEditorWidget *, int line, TextMarkRequestKind kind) {
                    if (kind == BookmarkRequest && !editor->document()->isTemporary())
                        m_bookmarkManager->toggleBookmark(editor->document()->filePath().toString(), line);
                });

        connect(widget, &TextEditorWidget::markContextMenuRequested,
                this, &BookmarksPlugin::requestContextMenu);
    }
}

void BookmarksPlugin::editorAboutToClose(IEditor *editor)
{
    if (auto widget = qobject_cast<TextEditorWidget *>(editor->widget())) {
        disconnect(widget, &TextEditorWidget::markContextMenuRequested,
                   this, &BookmarksPlugin::requestContextMenu);
    }
}

void BookmarksPlugin::requestContextMenu(TextEditorWidget *widget,
    int lineNumber, QMenu *menu)
{
    if (widget->textDocument()->isTemporary())
        return;

    m_bookmarkMarginActionLineNumber = lineNumber;
    m_bookmarkMarginActionFileName = widget->textDocument()->filePath().toString();

    menu->addAction(m_bookmarkMarginAction);
    if (m_bookmarkManager->hasBookmarkInPosition(m_bookmarkMarginActionFileName, m_bookmarkMarginActionLineNumber))
        menu->addAction(m_editBookmarkAction);
}

} // namespace Internal
} // namespace Bookmarks
