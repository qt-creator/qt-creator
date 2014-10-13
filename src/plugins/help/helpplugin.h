/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef HELPPLUGIN_H
#define HELPPLUGIN_H

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
class IMode;
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

    static HelpViewer *createHelpViewer(qreal zoom);

private slots:
    void unregisterOldQtCreatorDocumentation();

    void modeChanged(Core::IMode *mode, Core::IMode *old);

    void showContextHelp();
    void activateIndex();
    void activateContents();

    void saveExternalWindowSettings();
    void switchToHelpMode(const QUrl &source);
    void slotHideRightPane();

    void updateSideBarSource();
    void updateSideBarSource(const QUrl &newUrl);

    void fontChanged();

    void setupHelpEngineIfNeeded();

    void highlightSearchTermsInContextHelp();
    void handleHelpRequest(const QUrl &url, Core::HelpManager::HelpViewerLocation location);

    void slotOpenSupportPage();
    void slotReportBug();

private:
    void resetFilter();
    void activateHelpMode();
    bool canShowHelpSideBySide() const;
    HelpViewer *viewerForHelpViewerLocation(Core::HelpManager::HelpViewerLocation location);
    HelpViewer *viewerForContextHelp();
    HelpWidget *createHelpWidget(const Core::Context &context, HelpWidget::WidgetStyle style);
    void createRightPaneContextViewer();
    HelpViewer *externalHelpViewer();

    void doSetupIfNeeded();
    Core::HelpManager::HelpViewerLocation contextHelpOption() const;

private:
    HelpMode *m_mode;
    CentralWidget *m_centralWidget;
    HelpWidget *m_rightPaneSideBarWidget;

    DocSettingsPage *m_docSettingsPage;
    FilterSettingsPage *m_filterSettingsPage;
    GeneralSettingsPage *m_generalSettingsPage;
    SearchTaskHandler *m_searchTaskHandler;

    bool m_setupNeeded;
    LocalHelpManager *m_helpManager;
    OpenPagesManager *m_openPagesManager;

    QString m_contextHelpHighlightId;

    QPointer<HelpWidget> m_externalWindow;
    QRect m_externalWindowState;
};

} // namespace Internal
} // namespace Help

#endif // HELPPLUGIN_H
