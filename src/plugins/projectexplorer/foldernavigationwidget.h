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

#ifndef FOLDERNAVIGATIONWIDGET_H
#define FOLDERNAVIGATIONWIDGET_H

#include <coreplugin/inavigationwidgetfactory.h>

#include <QtGui/QDirModel>
#include <QtGui/QLabel>
#include <QtGui/QListView>
#include <QtGui/QSortFilterProxyModel>
#include <QtGui/QWidget>

namespace ProjectExplorer {

class ProjectExplorerPlugin;
class Project;
class Node;

namespace Internal {

class FolderNavigationWidget : public QWidget
{
    Q_OBJECT
public:
    FolderNavigationWidget(QWidget *parent = 0);

    bool autoSynchronization() const;
    void setAutoSynchronization(bool sync);

    QString currentFolder() const;

public slots:
    void toggleAutoSynchronization();

private slots:
    void openItem(const QModelIndex &mainIndex);
    void setCurrentFile(const QString &filePath);

private:
    void setCurrentTitle(const QDir &directory);

    ProjectExplorerPlugin *m_explorer;
    QListView *m_view;
    QDirModel *m_dirModel;
    QSortFilterProxyModel *m_filter;
    QLabel *m_title;
    bool m_autoSync;
};

class FolderNavigationWidgetFactory : public Core::INavigationWidgetFactory
{
public:
    FolderNavigationWidgetFactory();
    virtual ~FolderNavigationWidgetFactory();

    virtual QString displayName();
    virtual QKeySequence activationSequence();
    virtual Core::NavigationView createWidget();
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // FOLDERNAVIGATIONWIDGET_H
