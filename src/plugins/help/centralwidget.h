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

#ifndef CENTRALWIDGET_H
#define CENTRALWIDGET_H

#include <QtCore/QUrl>
#include <QtCore/QPoint>
#include <QtCore/QObject>

#include <QtGui/QWidget>
#include <QtGui/QTextDocument>

QT_BEGIN_NAMESPACE

class QEvent;
class QLabel;
class QAction;
class QCheckBox;
class QLineEdit;
class QToolButton;
class QTabWidget;
class QHelpEngine;
class QFocusEvent;
QT_END_NAMESPACE

class HelpViewer;

namespace Help {
namespace Internal {
class PrintHelper;

class CentralWidget : public QWidget
{
    Q_OBJECT

public:
    CentralWidget(QHelpEngine *engine, QWidget *parent = 0);
    ~CentralWidget();

    bool hasSelection() const;
    QUrl currentSource() const;
    QString currentTitle() const;
    bool isHomeAvailable() const;
    bool isForwardAvailable() const;
    bool isBackwardAvailable() const;
    QList<QAction*> globalActions() const;
    void setGlobalActions(const QList<QAction*> &actions);
    HelpViewer *currentHelpViewer() const;
    void activateTab(bool onlyHelpViewer = false);
    bool find(const QString &txt, QTextDocument::FindFlags findFlags, bool incremental);
    void setLastShownPages();
    HelpViewer *helpViewerAtIndex(int index) const;
    int indexOf(HelpViewer *viewer) const;

    static CentralWidget *instance();

public slots:
    void zoomIn();
    void zoomOut();
    void nextPage();
    void resetZoom();
    void previousPage();
    void copySelection();
    void print();
    void pageSetup();
    void printPreview();
    void setSource(const QUrl &url);
    void setSourceInNewTab(const QUrl &url, int zoom = 0);
    HelpViewer *newEmptyTab();
    void home();
    void forward();
    void backward();
    void showTopicChooser(const QMap<QString, QUrl> &links,
        const QString &keyword);
    void copy();
    void activateTab(int index);

protected:
    void focusInEvent(QFocusEvent *event);

signals:
    void currentViewerChanged(int index);
    void copyAvailable(bool yes);
    void sourceChanged(const QUrl &url);
    void highlighted(const QString &link);
    void forwardAvailable(bool available);
    void backwardAvailable(bool available);
    void addNewBookmark(const QString &title, const QString &url);

    void viewerAboutToBeRemoved(int index);
    void viewerRemoved(int index);

private slots:
    void newTab();
    void closeTab();
    void closeTab(int index);
    void setTabTitle(const QUrl& url);
    void currentPageChanged(int index);
    void showTabBarContextMenu(const QPoint &point);
    void printPreview(QPrinter *printer);

private:
    void connectSignals();
    bool eventFilter(QObject *object, QEvent *e);
    void initPrinter();
    QString quoteTabTitle(const QString &title) const;

private:
    int lastTabPage;
    QList<QAction*> globalActionList;

    QWidget *findBar;
    QTabWidget* tabWidget;
    QPrinter *printer;
};

} // namespace Internal
} // namespace Help

#endif  // CENTRALWIDGET_H
