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

#ifndef OPENPAGESMANAGER_H
#define OPENPAGESMANAGER_H

#include <QtCore/QObject>

QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QListView)
QT_FORWARD_DECLARE_CLASS(QModelIndex)
QT_FORWARD_DECLARE_CLASS(QTreeView)
QT_FORWARD_DECLARE_CLASS(QUrl)
QT_FORWARD_DECLARE_CLASS(QWidget)

class HelpViewer;

namespace Help {
    namespace Internal {

class OpenPagesModel;

class OpenPagesManager : public QObject
{
    Q_OBJECT

public:
    OpenPagesManager(QObject *parent = 0);

    static OpenPagesManager &instance();

    QWidget* openPagesWidget() const;
    QComboBox* openPagesComboBox() const;

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

signals:
    void pagesChanged();

private:
    void selectCurrentPage();
    void removePage(int index);

private:
    QComboBox *m_comboBox;
    OpenPagesModel *m_model;
    QTreeView *m_openPagesWidget;

    static OpenPagesManager *m_instance;
};

    } // namespace Internal
} // namespace Help

#endif // OPENPAGESMANAGER_H
