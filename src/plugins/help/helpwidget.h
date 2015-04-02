/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef HELPWIDGET_H
#define HELPWIDGET_H

#include <coreplugin/icontext.h>

#include <qglobal.h>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QAction;
class QComboBox;
class QFont;
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
    void showTopicChooser(const QMap<QString, QUrl> &links, const QString &key,
                          bool newPage = false);
    void activateSideBarItem(const QString &id);

public slots:
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

private slots:
    void updateBackMenu();
    void updateForwardMenu();
    void updateWindowTitle();
    void helpModeButtonClicked();

private:
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

    Core::IContext *m_context;
    WidgetStyle m_style;
    QAction *m_toggleSideBarAction;
    QAction *m_switchToHelp;
    QAction *m_homeAction;
    QMenu *m_backMenu;
    QMenu *m_forwardMenu;
    QAction *m_backAction;
    QAction *m_forwardAction;
    QAction *m_addBookmarkAction;
    QComboBox *m_filterComboBox;
    QAction *m_closeAction;
    QAction *m_scaleUp;
    QAction *m_scaleDown;
    QAction *m_resetScale;
    QAction *m_printAction;
    QAction *m_copy;

    QStackedWidget *m_viewerStack;
    QPrinter *m_printer;

    Core::MiniSplitter *m_sideBarSplitter;
    Core::SideBar *m_sideBar;
    QAction *m_contentsAction;
    QAction *m_indexAction;
    QAction *m_bookmarkAction;
    QAction *m_searchAction;
    QAction *m_openPagesAction;

    QStringList m_searchTerms;
};

} // Internal
} // Help

#endif // HELPWIDGET_H
