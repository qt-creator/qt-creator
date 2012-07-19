/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef BOOKMARKSPLUGIN_H
#define BOOKMARKSPLUGIN_H

#include <extensionsystem/iplugin.h>

#include <QObject>
#include <QMultiMap>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
QT_END_NAMESPACE

namespace Core {
class IEditor;
}

namespace TextEditor {
class ITextEditor;
}

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

    static BookmarksPlugin *instance() { return m_instance; }

    bool initialize(const QStringList &arguments, QString *errorMessage);
    void extensionsInitialized();

public slots:
    void updateActions(int stateMask);

private slots:
    void editorOpened(Core::IEditor *editor);
    void editorAboutToClose(Core::IEditor *editor);
    void requestContextMenu(TextEditor::ITextEditor *editor,
        int lineNumber, QMenu *menu);
    void bookmarkMarginActionTriggered();

private:
    static BookmarksPlugin *m_instance;
    BookmarkManager *m_bookmarkManager;

    QAction *m_toggleAction;
    QAction *m_prevAction;
    QAction *m_nextAction;
    QAction *m_docPrevAction;
    QAction *m_docNextAction;

    QAction *m_bookmarkMarginAction;
    int m_bookmarkMarginActionLineNumber;
    QString m_bookmarkMarginActionFileName;
};

} // namespace Internal
} // namespace Bookmarks

#endif // BOOKMARKS_H
