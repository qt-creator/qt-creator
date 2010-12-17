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

#ifndef BOOKMARKSPLUGIN_H
#define BOOKMARKSPLUGIN_H

#include <extensionsystem/iplugin.h>

#include <QtCore/QObject>
#include <QtCore/QMultiMap>

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

public:
    BookmarksPlugin();
    ~BookmarksPlugin();

    static BookmarksPlugin *instance() { return m_instance; }

    bool initialize(const QStringList &arguments, QString *error_message);
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
