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
class CentralWidget;
class HelpViewer;

QT_END_NAMESPACE

namespace Help {
namespace Internal {
class PrintHelper;
} // namespace Internal
} // namespace Help

QT_BEGIN_NAMESPACE

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
    void setSourceInNewTab(const QUrl &url);
    HelpViewer *newEmptyTab();
    void home();
    void forward();
    void backward();
    void showTopicChooser(const QMap<QString, QUrl> &links,
        const QString &keyword);
    void copy();

protected:
    void focusInEvent(QFocusEvent *event);

signals:
    void currentViewerChanged();
    void copyAvailable(bool yes);
    void sourceChanged(const QUrl &url);
    void highlighted(const QString &link);
    void forwardAvailable(bool available);
    void backwardAvailable(bool available);
    void addNewBookmark(const QString &title, const QString &url);

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
    HelpViewer *helpViewerAtIndex(int index) const;
    QString quoteTabTitle(const QString &title) const;

private:
    int lastTabPage;
    QString collectionFile;
    QList<QAction*> globalActionList;

    QWidget *findBar;
    QTabWidget* tabWidget;
    QHelpEngine *helpEngine;
    QPrinter *printer;
};

QT_END_NAMESPACE
//} // namespace Internal
//} // namespace Help

#endif  // CENTRALWIDGET_H
