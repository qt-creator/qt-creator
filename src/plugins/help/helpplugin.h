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
#include <QtCore/QMap>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QAction;
class QComboBox;
class QToolBar;
class QUrl;
QT_END_NAMESPACE

class HelpViewer;

namespace Core {
class ICore;
class IMode;
class MiniSplitter;
class SideBar;
class SideBarItem;
}   // Core

namespace Help {
class HelpManager;

namespace Internal {
class CentralWidget;
class DocSettingsPage;
class FilterSettingsPage;
class HelpMode;
class GeneralSettingsPage;
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

public slots:
    void handleHelpRequest(const QString &url);

private slots:
    void modeChanged(Core::IMode *mode);
    void activateContext();
    void activateIndex();
    void activateContents();
    void activateSearch();
    void updateFilterComboBox();
    void filterDocumentation(const QString &customFilter);
    void addBookmark();
    void addNewBookmark(const QString &title, const QString &url);

    void switchToHelpMode();
    void switchToHelpMode(const QUrl &source);
    void switchToHelpMode(const QMap<QString, QUrl> &urls, const QString &keyword);
    void slotHideRightPane();

    void updateSideBarSource();
    void updateSideBarSource(const QUrl &newUrl);

    void fontChanged();
    void setupHelpEngineIfNeeded();

    void rebuildViewerComboBox();
    void removeViewerFromComboBox(int index);
    void updateViewerComboBoxIndex(int index);

private:
    void setupUi();
    void resetFilter();
    void activateHelpMode();
    QToolBar *createToolBar();
    void connectCentralWidget();
    HelpViewer* viewerForContextMode();
    void createRightPaneContextViewer();

private:
    HelpMode *m_mode;
    Core::ICore *m_core;
    QWidget *m_mainWidget;
    CentralWidget *m_centralWidget;
    HelpViewer *m_helpViewerForSideBar;

    Core::SideBarItem *m_contentItem;
    Core::SideBarItem *m_indexItem;
    Core::SideBarItem *m_searchItem;
    Core::SideBarItem *m_bookmarkItem;

    DocSettingsPage *m_docSettingsPage;
    FilterSettingsPage *m_filterSettingsPage;
    GeneralSettingsPage *m_generalSettingsPage;

    QComboBox *m_documentsCombo;
    QComboBox *m_filterComboBox;
    Core::SideBar *m_sideBar;

    bool m_firstModeChange;
    HelpManager *m_helpManager;
    Core::MiniSplitter *m_splitter;
};

} // namespace Internal
} // namespace Help

#endif // HELPPLUGIN_H
