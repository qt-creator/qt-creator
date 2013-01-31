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

#ifndef OPENPAGESMANAGER_H
#define OPENPAGESMANAGER_H

#include <QObject>

QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QListView)
QT_FORWARD_DECLARE_CLASS(QModelIndex)
QT_FORWARD_DECLARE_CLASS(QPoint)
QT_FORWARD_DECLARE_CLASS(QTreeView)
QT_FORWARD_DECLARE_CLASS(QUrl)
QT_FORWARD_DECLARE_CLASS(QWidget)

namespace Help {
    namespace Internal {

class HelpViewer;
class OpenPagesModel;
class OpenPagesSwitcher;
class OpenPagesWidget;

class OpenPagesManager : public QObject
{
    Q_OBJECT

public:
    OpenPagesManager(QObject *parent = 0);
    ~OpenPagesManager();

    static OpenPagesManager &instance();

    QWidget *openPagesWidget() const;
    QComboBox *openPagesComboBox() const;

    int pageCount() const;
    void setupInitialPages();

public slots:
    HelpViewer *createPage();
    HelpViewer *createPageFromSearch(const QUrl &url);
    HelpViewer *createPage(const QUrl &url, bool fromSearch = false);

    void setCurrentPage(int index);
    void setCurrentPage(const QModelIndex &index);

    void closeCurrentPage();
    void closePage(const QModelIndex &index);
    void closePagesExcept(const QModelIndex &index);

    void gotoNextPage();
    void gotoPreviousPage();

signals:
    void pagesChanged();

private:
    void removePage(int index);
    void showTwicherOrSelectPage() const;

private slots:
    void openPagesContextMenu(const QPoint &point);

private:
    QComboBox *m_comboBox;
    OpenPagesModel *m_model;
    mutable OpenPagesWidget *m_openPagesWidget;
    OpenPagesSwitcher *m_openPagesSwitcher;

    static OpenPagesManager *m_instance;
};

    } // namespace Internal
} // namespace Help

#endif // OPENPAGESMANAGER_H
