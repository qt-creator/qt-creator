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

#include "bookmarkfilter.h"
#include "bookmarkmanager.h"
#include "bookmarks_global.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>

#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditorconstants.h>

#include <utils/fileutils.h>
#include <utils/utilsicons.h>

#include <QMenu>

using namespace Core;
using namespace TextEditor;
using namespace Utils;

using namespace Bookmarks::Constants;

namespace Bookmarks {
namespace Internal {

class BookmarksPluginPrivate : public QObject
{
public:
    BookmarksPluginPrivate();

    void updateActions(bool enableToggle, int stateMask);
    void editorOpened(Core::IEditor *editor);
    void editorAboutToClose(Core::IEditor *editor);

    void requestContextMenu(TextEditor::TextEditorWidget *widget,
                            int lineNumber, QMenu *menu);

    BookmarkManager m_bookmarkManager;
    BookmarkFilter m_bookmarkFilter;
    BookmarkViewFactory m_bookmarkViewFactory;

    QAction m_toggleAction{BookmarksPlugin::tr("Toggle Bookmark"), nullptr};
    QAction m_prevAction{BookmarksPlugin::tr("Previous Bookmark"), nullptr};
    QAction m_nextAction{BookmarksPlugin::tr("Next Bookmark"), nullptr};
    QAction m_docPrevAction{BookmarksPlugin::tr("Previous Bookmark in Document"), nullptr};
    QAction m_docNextAction{BookmarksPlugin::tr("Next Bookmark in Document"), nullptr};
    QAction m_editBookmarkAction{BookmarksPlugin::tr("Edit Bookmark"), nullptr};
    QAction m_bookmarkMarginAction{BookmarksPlugin::tr("Toggle Bookmark"), nullptr};

    int m_marginActionLineNumber = 0;
    Utils::FilePath m_marginActionFileName;
};

BookmarksPlugin::~BookmarksPlugin()
{
    delete d;
}

bool BookmarksPlugin::initialize(const QStringList &, QString *)
{
    d = new BookmarksPluginPrivate;
    return true;
}

BookmarksPluginPrivate::BookmarksPluginPrivate()
    : m_bookmarkFilter(&m_bookmarkManager)
    , m_bookmarkViewFactory(&m_bookmarkManager)
{
    ActionContainer *mtools = ActionManager::actionContainer(Core::Constants::M_TOOLS);
    ActionContainer *touchBar = ActionManager::actionContainer(Core::Constants::TOUCH_BAR);
    ActionContainer *mbm = ActionManager::createMenu(Id(BOOKMARKS_MENU));
    mbm->menu()->setTitle(BookmarksPlugin::tr("&Bookmarks"));
    mtools->addMenu(mbm);

    const Context editorManagerContext(Core::Constants::C_EDITORMANAGER);

    // Toggle
    Command *cmd = ActionManager::registerAction(&m_toggleAction, BOOKMARKS_TOGGLE_ACTION,
                                                 editorManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? BookmarksPlugin::tr("Meta+M")
                                                            : BookmarksPlugin::tr("Ctrl+M")));
    cmd->setTouchBarIcon(Utils::Icons::MACOS_TOUCHBAR_BOOKMARK.icon());
    mbm->addAction(cmd);
    touchBar->addAction(cmd, Core::Constants::G_TOUCHBAR_EDITOR);

    mbm->addSeparator();

    // Previous
    m_prevAction.setIcon(Utils::Icons::PREV_TOOLBAR.icon());
    m_prevAction.setIconVisibleInMenu(false);
    cmd = ActionManager::registerAction(&m_prevAction, BOOKMARKS_PREV_ACTION, editorManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? BookmarksPlugin::tr("Meta+,")
                                                            : BookmarksPlugin::tr("Ctrl+,")));
    mbm->addAction(cmd);

    // Next
    m_nextAction.setIcon(Utils::Icons::NEXT_TOOLBAR.icon());
    m_nextAction.setIconVisibleInMenu(false);
    cmd = ActionManager::registerAction(&m_nextAction, BOOKMARKS_NEXT_ACTION, editorManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? BookmarksPlugin::tr("Meta+.")
                                                            : BookmarksPlugin::tr("Ctrl+.")));
    mbm->addAction(cmd);

    mbm->addSeparator();

    // Previous Doc
    cmd = ActionManager::registerAction(&m_docPrevAction, BOOKMARKS_PREVDOC_ACTION,
                                        editorManagerContext);
    mbm->addAction(cmd);

    // Next Doc
    cmd = ActionManager::registerAction(&m_docNextAction, BOOKMARKS_NEXTDOC_ACTION,
                                        editorManagerContext);
    mbm->addAction(cmd);

    connect(&m_toggleAction, &QAction::triggered, this, [this] {
        BaseTextEditor *editor = BaseTextEditor::currentTextEditor();
        if (editor && !editor->document()->isTemporary())
            m_bookmarkManager.toggleBookmark(editor->document()->filePath(), editor->currentLine());
    });

    connect(&m_prevAction, &QAction::triggered, &m_bookmarkManager, &BookmarkManager::prev);
    connect(&m_nextAction, &QAction::triggered, &m_bookmarkManager, &BookmarkManager::next);
    connect(&m_docPrevAction, &QAction::triggered,
            &m_bookmarkManager, &BookmarkManager::prevInDocument);
    connect(&m_docNextAction, &QAction::triggered,
            &m_bookmarkManager, &BookmarkManager::nextInDocument);

    connect(&m_editBookmarkAction, &QAction::triggered, this, [this] {
            m_bookmarkManager.editByFileAndLine(m_marginActionFileName, m_marginActionLineNumber);
    });

    connect(&m_bookmarkManager, &BookmarkManager::updateActions,
            this, &BookmarksPluginPrivate::updateActions);
    updateActions(false, m_bookmarkManager.state());

    connect(&m_bookmarkMarginAction, &QAction::triggered, this, [this] {
            m_bookmarkManager.toggleBookmark(m_marginActionFileName, m_marginActionLineNumber);
    });

    // EditorManager
    connect(EditorManager::instance(), &EditorManager::editorAboutToClose,
        this, &BookmarksPluginPrivate::editorAboutToClose);
    connect(EditorManager::instance(), &EditorManager::editorOpened,
        this, &BookmarksPluginPrivate::editorOpened);
}

void BookmarksPluginPrivate::updateActions(bool enableToggle, int state)
{
    const bool hasbm    = state >= BookmarkManager::HasBookMarks;
    const bool hasdocbm = state == BookmarkManager::HasBookmarksInDocument;

    m_toggleAction.setEnabled(enableToggle);
    m_prevAction.setEnabled(hasbm);
    m_nextAction.setEnabled(hasbm);
    m_docPrevAction.setEnabled(hasdocbm);
    m_docNextAction.setEnabled(hasdocbm);
}

void BookmarksPluginPrivate::editorOpened(IEditor *editor)
{
    if (auto widget = TextEditorWidget::fromEditor(editor)) {
        connect(widget, &TextEditorWidget::markRequested,
                this, [this, editor](TextEditorWidget *, int line, TextMarkRequestKind kind) {
                    if (kind == BookmarkRequest && !editor->document()->isTemporary())
                        m_bookmarkManager.toggleBookmark(editor->document()->filePath(), line);
                });

        connect(widget, &TextEditorWidget::markContextMenuRequested,
                this, &BookmarksPluginPrivate::requestContextMenu);
    }
}

void BookmarksPluginPrivate::editorAboutToClose(IEditor *editor)
{
    if (auto widget = TextEditorWidget::fromEditor(editor)) {
        disconnect(widget, &TextEditorWidget::markContextMenuRequested,
                   this, &BookmarksPluginPrivate::requestContextMenu);
    }
}

void BookmarksPluginPrivate::requestContextMenu(TextEditorWidget *widget,
    int lineNumber, QMenu *menu)
{
    if (widget->textDocument()->isTemporary())
        return;

    m_marginActionLineNumber = lineNumber;
    m_marginActionFileName = widget->textDocument()->filePath();

    menu->addAction(&m_bookmarkMarginAction);
    if (m_bookmarkManager.hasBookmarkInPosition(m_marginActionFileName, m_marginActionLineNumber))
        menu->addAction(&m_editBookmarkAction);
}

} // namespace Internal
} // namespace Bookmarks
