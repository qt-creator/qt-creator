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

#pragma once

#include <extensionsystem/iplugin.h>

#include <QObject>
#include <QMultiMap>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
QT_END_NAMESPACE

namespace Core { class IEditor; }

namespace TextEditor { class TextEditorWidget; }

namespace Bookmarks {
namespace Internal {

class BookmarkManager;

class BookmarksPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Bookmarks.json")

public:
    BookmarksPlugin();
    ~BookmarksPlugin();

    bool initialize(const QStringList &arguments, QString *errorMessage);
    void extensionsInitialized() {}

private:
    void updateActions(bool enableToggle, int stateMask);
    void editorOpened(Core::IEditor *editor);
    void editorAboutToClose(Core::IEditor *editor);

    void requestContextMenu(TextEditor::TextEditorWidget *widget,
                            int lineNumber, QMenu *menu);

    BookmarkManager *m_bookmarkManager;

    QAction *m_toggleAction;
    QAction *m_prevAction;
    QAction *m_nextAction;
    QAction *m_docPrevAction;
    QAction *m_docNextAction;
    QAction *m_editBookmarkAction;

    QAction *m_bookmarkMarginAction;
    int m_bookmarkMarginActionLineNumber;
    QString m_bookmarkMarginActionFileName;
};

} // namespace Internal
} // namespace Bookmarks
