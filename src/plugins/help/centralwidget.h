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

#ifndef CENTRALWIDGET_H
#define CENTRALWIDGET_H

#include <coreplugin/find/ifindsupport.h>

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QEvent)
QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QStackedWidget)
QT_FORWARD_DECLARE_CLASS(QFocusEvent)
QT_FORWARD_DECLARE_CLASS(QPrinter)

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

    bool isForwardAvailable() const;
    bool isBackwardAvailable() const;

    HelpViewer *viewerAt(int index) const;
    HelpViewer *currentHelpViewer() const;

    void addPage(HelpViewer *page, bool fromSearch = false);
    void removePage(int index);

    int currentIndex() const;
    void setCurrentPage(HelpViewer *page);

    bool find(const QString &txt, Core::FindFlags findFlags,
        bool incremental, bool *wrapped = 0);

public slots:
    void copy();
    void home();

    void zoomIn();
    void zoomOut();
    void resetZoom();

    void forward();
    void backward();

    void print();
    void pageSetup();
    void printPreview();

    void setSource(const QUrl &url);
    void setSourceFromSearch(const QUrl &url);
    void showTopicChooser(const QMap<QString, QUrl> &links, const QString &key);

protected:
    void focusInEvent(QFocusEvent *event);

signals:
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
