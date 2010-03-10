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
#ifndef HELPPLUGIN_H
#define HELPPLUGIN_H

#include <extensionsystem/iplugin.h>

#include <QtCore/QFutureInterface>
#include <QtCore/QFutureWatcher>
#include <QtCore/QMap>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QAction;
class QComboBox;
class QHelpEngine;
class QHelpEngineCore;
class QToolBar;
class QUrl;
QT_END_NAMESPACE

class IndexWindow;
class ContentWindow;
class BookmarkManager;
class BookmarkWidget;
class HelpViewer;

namespace Core {
class ICore;
class IMode;
class SideBar;
class SideBarItem;
}   // Core

namespace Help {
class HelpManager;

namespace Constants {
const char * const C_MODE_HELP    = "Help Mode";
const char * const C_HELP_SIDEBAR = "Help Sidebar";
const int          P_MODE_HELP    = 70;
const char * const ID_MODE_HELP   = "Help";
}   // Constants

namespace Internal {
class CentralWidget;
class DocSettingsPage;
class FilterSettingsPage;
class HelpMode;
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

    QHelpEngine* helpEngine() const;

    void setFilesToRegister(const QStringList &files);

public slots:
    void pluginUpdateDocumentation();
    void handleHelpRequest(const QString &url);

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
    void copyFromSideBar();

    void updateSideBarSource();
    void updateSideBarSource(const QUrl &newUrl);

    void fontChanged();

    void rebuildViewerComboBox();
    void removeViewerFromComboBox(int index);
    void updateViewerComboBoxIndex(int index);

    void indexingStarted();
    void indexingFinished();

private:
    bool updateDocumentation();

private:
    QToolBar *createToolBar();
    void createRightPaneSideBar();
    void activateHelpMode();
    HelpViewer* viewerForContextMode();

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

    Core::SideBarItem *m_contentItem;
    Core::SideBarItem *m_indexItem;
    Core::SideBarItem *m_searchItem;
    Core::SideBarItem *m_bookmarkItem;

    DocSettingsPage *m_docSettingsPage;
    FilterSettingsPage *m_filterSettingsPage;

    QComboBox *m_documentsCombo;
    QComboBox *m_filterComboBox;
    Core::SideBar *m_sideBar;
    QWidget *m_rightPaneSideBar;

    QAction *m_rightPaneBackwardAction;
    QAction *m_rightPaneForwardAction;

    HelpManager *helpManager;
    QStringList filesToRegister;

    QFutureWatcher<void> m_watcher;
    QFutureInterface<void> *m_progress;

    bool isInitialised;
    bool firstModeChange;
};

} // namespace Internal
} // namespace Help

#endif // HELPPLUGIN_H
