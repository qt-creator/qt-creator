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

#include <find/ifindsupport.h>

#include <QtGui/QWidget>

QT_FORWARD_DECLARE_CLASS(QEvent)
QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QStackedWidget)
QT_FORWARD_DECLARE_CLASS(QFocusEvent)

namespace Help {
    namespace Internal {

class HelpViewer;
class PrintHelper;

class CentralWidget : public QWidget
{
    Q_OBJECT

public:
    CentralWidget(QWidget *parent = 0);
    ~CentralWidget();

    static CentralWidget *instance();

    bool hasSelection() const;
    bool isForwardAvailable() const;
    bool isBackwardAvailable() const;

    HelpViewer *viewerAt(int index) const;
    HelpViewer *currentHelpViewer() const;

    void addPage(HelpViewer *page, bool fromSearch = false);
    void removePage(int index);

    int currentIndex() const;
    void setCurrentPage(HelpViewer *page);

    bool find(const QString &txt, Find::IFindSupport::FindFlags findFlags,
        bool incremental);

public slots:
    void copy();
    void home();

    void zoomIn();
    void zoomOut();
    void resetZoom();

    void forward();
    void nextPage();

    void backward();
    void previousPage();

    void print();
    void pageSetup();
    void printPreview();

    void setSource(const QUrl &url);
    void setSourceFromSearch(const QUrl &url);
    void showTopicChooser(const QMap<QString, QUrl> &links, const QString &key);

protected:
    void focusInEvent(QFocusEvent *event);

signals:
    void currentViewerChanged();
    void sourceChanged(const QUrl &url);
    void forwardAvailable(bool available);
    void backwardAvailable(bool available);

private slots:
    void highlightSearchTerms();
    void printPreview(QPrinter *printer);
    void handleSourceChanged(const QUrl &url);

private:
    void initPrinter();
    void connectSignals(HelpViewer *page);
    bool eventFilter(QObject *object, QEvent *e);

private:
    QPrinter *printer;
    QStackedWidget *m_stackedWidget;
};

    } // namespace Internal
} // namespace Help

#endif  // CENTRALWIDGET_H
