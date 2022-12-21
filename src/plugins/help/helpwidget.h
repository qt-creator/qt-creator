// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/helpmanager.h>
#include <coreplugin/icontext.h>

#include <QAbstractTableModel>
#include <QWidget>
#include <qglobal.h>

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
class HelpWidget;
class OpenPagesManager;

class OpenPagesModel : public QAbstractTableModel
{
public:
    OpenPagesModel(HelpWidget *parent);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    HelpWidget *m_parent;

    friend class HelpWidget;
};

class HelpWidget : public QWidget
{
    Q_OBJECT
public:
    enum WidgetStyle {
        ModeWidget,
        SideBarWidget,
        ExternalWindow
    };

    HelpWidget(const Core::Context &context, WidgetStyle style, QWidget *parent = nullptr);
    ~HelpWidget() override;

    QAbstractItemModel *model();
    WidgetStyle widgetStyle() const;

    HelpViewer *currentViewer() const;
    int currentIndex() const;
    void setCurrentIndex(int index);
    HelpViewer *addViewer(const QUrl &url);
    void removeViewerAt(int index);

    int viewerCount() const;
    HelpViewer *viewerAt(int index) const;

    void open(const QUrl &url, bool newPage = false);
    HelpViewer *openNewPage(const QUrl &url);
    void openFromSearch(const QUrl &url, const QStringList &searchTerms, bool newPage = false);
    void showLinks(const QMultiMap<QString, QUrl> &links, const QString &key,
                          bool newPage = false);
    void activateSideBarItem(const QString &id);

    OpenPagesManager *openPagesManager() const;

public:
    void setSource(const QUrl &url);
    void updateCloseButton();

protected:
    void closeEvent(QCloseEvent *) override;

signals:
    void requestShowHelpUrl(const QUrl &url, Core::HelpManager::HelpViewerLocation location);
    void closeButtonClicked();
    void aboutToClose();
    void filterActivated(const QString &name);
    void currentIndexChanged(int index);

private:
    int indexOf(HelpViewer *viewer) const;
    HelpViewer *insertViewer(int index, const QUrl &url);
    void updateBackMenu();
    void updateForwardMenu();
    void updateWindowTitle();
    void postRequestShowHelpUrl(Core::HelpManager::HelpViewerLocation location);
    void closeCurrentPage();
    void saveState() const;
    bool supportsPages() const;
    void closeWindow();

    void goHome();
    void addBookmark();
    void openOnlineDocumentation();
    void copy();
    void forward();
    void backward();
    void scaleUp();
    void scaleDown();
    void resetScale();
    void print(HelpViewer *viewer);
    void highlightSearchTerms(HelpViewer *viewer);
    void addSideBar();
    QString sideBarSettingsKey() const;

    void setupFilterCombo();
    void filterDocumentation(int filterIndex);
    void currentFilterChanged(const QString &filter);

    OpenPagesModel m_model;
    OpenPagesManager *m_openPagesManager = nullptr;
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
    QAction *m_openOnlineDocumentationAction = nullptr;
    QComboBox *m_filterComboBox = nullptr;
    QAction *m_closeAction = nullptr;
    QAction *m_scaleUp = nullptr;
    QAction *m_scaleDown = nullptr;
    QAction *m_resetScale = nullptr;
    QAction *m_printAction = nullptr;
    QAction *m_copy = nullptr;
    QAction *m_gotoPrevious = nullptr;
    QAction *m_gotoNext = nullptr;

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
