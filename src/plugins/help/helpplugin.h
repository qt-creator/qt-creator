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

#ifndef HELPPLUGIN_H
#define HELPPLUGIN_H

#include <extensionsystem/iplugin.h>
#include <QMap>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QAction;
class QComboBox;
class QMenu;
class QToolButton;
class QUrl;
QT_END_NAMESPACE

namespace Core {
class IMode;
class MiniSplitter;
class SideBar;
class SideBarItem;
}   // Core

namespace Utils {
class StyledBar;
}   // Utils

namespace Help {
namespace Internal {
class CentralWidget;
class DocSettingsPage;
class ExternalHelpWindow;
class FilterSettingsPage;
class GeneralSettingsPage;
class HelpMode;
class HelpViewer;
class LocalHelpManager;
class OpenPagesManager;
class SearchWidget;

class HelpPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Help.json")

public:
    HelpPlugin();
    virtual ~HelpPlugin();

    bool initialize(const QStringList &arguments, QString *errorMessage);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();

private slots:
    void unregisterOldQtCreatorDocumentation();

    void showExternalWindow();
    void modeChanged(Core::IMode *mode, Core::IMode *old);

    void activateContext();
    void activateIndex();
    void activateContents();
    void activateSearch();
    void activateOpenPages();
    void activateBookmarks();

    void addBookmark();
    void updateFilterComboBox();
    void filterDocumentation(const QString &customFilter);

    void switchToHelpMode();
    void switchToHelpMode(const QUrl &source);
    void slotHideRightPane();
    void showHideSidebar();

    void updateSideBarSource();
    void updateSideBarSource(const QUrl &newUrl);

    void fontChanged();
    void contextHelpOptionChanged();

    void updateCloseButton();
    void setupHelpEngineIfNeeded();

    void highlightSearchTerms();
    void handleHelpRequest(const QUrl &url);

    void slotAboutToShowBackMenu();
    void slotAboutToShowNextMenu();
    void slotOpenActionUrl(QAction *action);
    void slotOpenSupportPage();
    void slotReportBug();

    void openFindToolBar();
    void onSideBarVisibilityChanged();

    void scaleRightPaneUp();
    void scaleRightPaneDown();
    void resetRightPaneScale();

private:
    void setupUi();
    void resetFilter();
    void activateHelpMode();
    Utils::StyledBar *createWidgetToolBar();
    Utils::StyledBar *createIconToolBar(bool external);
    HelpViewer* viewerForContextMode();
    void createRightPaneContextViewer();

    void doSetupIfNeeded();
    int contextHelpOption() const;
    void connectExternalHelpWindow();
    void setupNavigationMenus(QAction *back, QAction *next, QWidget *parent);

private:
    HelpMode *m_mode;
    CentralWidget *m_centralWidget;
    QWidget *m_rightPaneSideBarWidget;
    HelpViewer *m_helpViewerForSideBar;

    Core::SideBarItem *m_contentItem;
    Core::SideBarItem *m_indexItem;
    Core::SideBarItem *m_searchItem;
    Core::SideBarItem *m_bookmarkItem;
    Core::SideBarItem *m_openPagesItem;

    DocSettingsPage *m_docSettingsPage;
    FilterSettingsPage *m_filterSettingsPage;
    GeneralSettingsPage *m_generalSettingsPage;

    QComboBox *m_filterComboBox;
    Core::SideBar *m_sideBar;

    bool m_firstModeChange;
    LocalHelpManager *m_helpManager;
    OpenPagesManager *m_openPagesManager;
    Core::MiniSplitter *m_splitter;

    QToolButton *m_closeButton;

    QString m_oldAttrValue;
    QString m_styleProperty;
    QString m_idFromContext;

    Core::IMode* m_oldMode;
    bool m_connectWindow;
    ExternalHelpWindow *m_externalWindow;

    QMenu *m_backMenu;
    QMenu *m_nextMenu;
    Utils::StyledBar *m_internalHelpBar;
    Utils::StyledBar *m_externalHelpBar;

    bool m_isSidebarVisible;
    QAction *m_toggleSideBarAction;
};

} // namespace Internal
} // namespace Help

#endif // HELPPLUGIN_H
