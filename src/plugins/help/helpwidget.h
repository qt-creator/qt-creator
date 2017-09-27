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

#include <coreplugin/icontext.h>

#include <qglobal.h>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QAction;
class QComboBox;
class QMenu;
class QPrinter;
class QStackedWidget;
QT_END_NAMESPACE

namespace Core {
class MiniSplitter;
class SideBar;
}

namespace Help {
namespace Internal {

class HelpViewer;

class HelpWidget : public QWidget
{
    Q_OBJECT
public:
    enum WidgetStyle {
        ModeWidget,
        SideBarWidget,
        ExternalWindow
    };

    HelpWidget(const Core::Context &context, WidgetStyle style, QWidget *parent = 0);
    ~HelpWidget();

    HelpViewer *currentViewer() const;
    void setCurrentViewer(HelpViewer *viewer);
    int currentIndex() const;
    void addViewer(HelpViewer *viewer);
    void removeViewerAt(int index);

    // so central widget can save the state
    int viewerCount() const;
    HelpViewer *viewerAt(int index) const;

    void open(const QUrl &url, bool newPage = false);
    void openFromSearch(const QUrl &url, const QStringList &searchTerms, bool newPage = false);
    void showLinks(const QMap<QString, QUrl> &links, const QString &key,
                          bool newPage = false);
    void activateSideBarItem(const QString &id);

public:
    void setSource(const QUrl &url);
    void updateCloseButton();

protected:
    void closeEvent(QCloseEvent *);

signals:
    void openHelpMode(const QUrl &url);
    void closeButtonClicked();
    void aboutToClose();
    void sourceChanged(const QUrl &url);
    void filterActivated(const QString &name);

private:
    void updateBackMenu();
    void updateForwardMenu();
    void updateWindowTitle();
    void helpModeButtonClicked();

    void goHome();
    void addBookmark();
    void copy();
    void forward();
    void backward();
    void scaleUp();
    void scaleDown();
    void resetScale();
    void print(HelpViewer *viewer);
    void highlightSearchTerms();
    void addSideBar();
    QString sideBarSettingsKey() const;

    Core::IContext *m_context = nullptr;
    WidgetStyle m_style;
    QAction *m_toggleSideBarAction = nullptr;
    QAction *m_switchToHelp = nullptr;
    QAction *m_homeAction = nullptr;
    QMenu *m_backMenu = nullptr;
    QMenu *m_forwardMenu = nullptr;
    QAction *m_backAction = nullptr;
    QAction *m_forwardAction = nullptr;
    QAction *m_addBookmarkAction = nullptr;
    QComboBox *m_filterComboBox = nullptr;
    QAction *m_closeAction = nullptr;
    QAction *m_scaleUp = nullptr;
    QAction *m_scaleDown = nullptr;
    QAction *m_resetScale = nullptr;
    QAction *m_printAction = nullptr;
    QAction *m_copy = nullptr;

    QStackedWidget *m_viewerStack = nullptr;
    QPrinter *m_printer = nullptr;

    Core::MiniSplitter *m_sideBarSplitter = nullptr;
    Core::SideBar *m_sideBar = nullptr;
    QAction *m_contentsAction = nullptr;
    QAction *m_indexAction = nullptr;
    QAction *m_bookmarkAction = nullptr;
    QAction *m_searchAction = nullptr;
    QAction *m_openPagesAction = nullptr;

    QStringList m_searchTerms;
};

} // Internal
} // Help
