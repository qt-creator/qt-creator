/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef HELPPLUGIN_H
#define HELPPLUGIN_H

#include "help_global.h"

#include <extensionsystem/iplugin.h>

#include <QtCore/QMap>

QT_BEGIN_NAMESPACE
class QAction;
class QComboBox;
class QHelpEngineCore;
class QHelpEngine;
class QShortcut;
class QToolBar;
class QUrl;

class IndexWindow;
class ContentWindow;
class BookmarkManager;
class BookmarkWidget;
class CentralWidget;
class HelpViewer;
QT_END_NAMESPACE


namespace Core {
class ICore;
class IMode;
class SideBar;
class SideBarItem;
}

namespace Help {

namespace Constants {
    const char * const HELPVIEWER_KIND = "Qt Help Viewer";
    const char * const C_MODE_HELP     = "Help Mode";
    const int          P_MODE_HELP     = 70;
    const char * const ID_MODE_HELP    = "Help";
}

class HELP_EXPORT HelpManager : public QObject
{
    Q_OBJECT
public:
    HelpManager(QHelpEngine *helpEngine);

    void registerDocumentation(const QStringList &fileNames);

private:
    QHelpEngine *m_helpEngine;
};

namespace Internal {

class HelpMode;
class HelpPluginEditorFactory;
class DocSettingsPage;
class FilterSettingsPage;
class SearchWidget;

class HelpPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    HelpPlugin();
    virtual ~HelpPlugin();

    bool initialize(const QStringList &arguments, QString *error_message);
    void extensionsInitialized();
    void shutdown();

    // Necessary to get the unfiltered list in the help index filter
    void setIndexFilter(const QString &filter);
    QString indexFilter() const;

private slots:
    void modeChanged(Core::IMode *mode);
    void activateContext();
    void activateIndex();
    void activateContents();
    void activateSearch();
    void checkForHelpChanges();
    void updateFilterComboBox();
    void filterDocumentation(const QString &customFilter);
    void addBookmark();
    void addNewBookmark(const QString &title, const QString &url);

    void rightPaneBackward();
    void rightPaneForward();
    void switchToHelpMode();
    void switchToHelpMode(const QUrl &source);
    void switchToHelpMode(const QMap<QString, QUrl> &urls, const QString &keyword);
    void slotHideRightPane();

    void openGettingStarted();

private:
    QToolBar *createToolBar();
    void createRightPaneSideBar();
    void activateHelpMode();

    Core::ICore *m_core;
    QHelpEngine *m_helpEngine;
    QHelpEngineCore *m_contextHelpEngine;
    ContentWindow *m_contentWidget;
    IndexWindow *m_indexWidget;
    BookmarkWidget *m_bookmarkWidget;
    BookmarkManager *m_bookmarkManager;
    SearchWidget *m_searchWidget;
    CentralWidget *m_centralWidget;
    HelpViewer *m_helpViewerForSideBar;
    HelpMode *m_mode;
    bool m_shownLastPages;

    Core::SideBarItem *m_contentItem;
    Core::SideBarItem *m_indexItem;
    Core::SideBarItem *m_searchItem;
    Core::SideBarItem *m_bookmarkItem;

    DocSettingsPage *m_docSettingsPage;
    FilterSettingsPage *m_filterSettingsPage;

    QComboBox *m_filterComboBox;
    Core::SideBar *m_sideBar;
    QWidget *m_rightPaneSideBar;

    QAction *m_rightPaneBackwardAction;
    QAction *m_rightPaneForwardAction;
};

} // namespace Internal
} // namespace Help

#endif // HELPPLUGIN_H
