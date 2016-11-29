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

#include "helpwidget.h"

#include <coreplugin/helpmanager.h>
#include <coreplugin/icontext.h>
#include <extensionsystem/iplugin.h>

#include <QMap>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QToolButton;
class QUrl;
QT_END_NAMESPACE

namespace Core {
class MiniSplitter;
class SideBar;
class SideBarItem;
}   // Core

namespace Utils { class StyledBar; }

namespace Help {
namespace Internal {
class CentralWidget;
class DocSettingsPage;
class FilterSettingsPage;
class GeneralSettingsPage;
class HelpMode;
class HelpViewer;
class LocalHelpManager;
class OpenPagesManager;
class SearchWidget;
class SearchTaskHandler;

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

    static HelpViewer *viewerForHelpViewerLocation(Core::HelpManager::HelpViewerLocation location);

    static HelpViewer *createHelpViewer(qreal zoom);

private:
    void modeChanged(Core::Id mode, Core::Id old);

    void showContextHelp();
    void activateIndex();
    void activateContents();

    void saveExternalWindowSettings();
    void showLinkInHelpMode(const QUrl &source);
    void showLinksInHelpMode(const QMap<QString, QUrl> &links, const QString &key);
    void slotHideRightPane();

    void updateSideBarSource(const QUrl &newUrl);

    void setupHelpEngineIfNeeded();

    void highlightSearchTermsInContextHelp();
    void handleHelpRequest(const QUrl &url, Core::HelpManager::HelpViewerLocation location);

    void slotOpenSupportPage();
    void slotReportBug();
    void slotSystemInformation();

    void resetFilter();
    static void activateHelpMode();
    static bool canShowHelpSideBySide();
    HelpViewer *viewerForContextHelp();
    HelpWidget *createHelpWidget(const Core::Context &context, HelpWidget::WidgetStyle style);
    void createRightPaneContextViewer();
    HelpViewer *externalHelpViewer();

    void doSetupIfNeeded();

    HelpMode *m_mode = nullptr;
    CentralWidget *m_centralWidget = nullptr;
    HelpWidget *m_rightPaneSideBarWidget = nullptr;

    DocSettingsPage *m_docSettingsPage = nullptr;
    FilterSettingsPage *m_filterSettingsPage = nullptr;
    SearchTaskHandler *m_searchTaskHandler = nullptr;

    bool m_setupNeeded = true;
    LocalHelpManager *m_helpManager = nullptr;
    OpenPagesManager *m_openPagesManager = nullptr;

    QString m_contextHelpHighlightId;

    QPointer<HelpWidget> m_externalWindow;
    QRect m_externalWindowState;
};

} // namespace Internal
} // namespace Help
