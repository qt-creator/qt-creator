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

#ifndef FOLDERNAVIGATIONWIDGET_H
#define FOLDERNAVIGATIONWIDGET_H

#include <coreplugin/inavigationwidgetfactory.h>

#include <QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
class QListView;
class QSortFilterProxyModel;
class QModelIndex;
class QFileSystemModel;
class QDir;
class QAction;
QT_END_NAMESPACE

namespace ProjectExplorer {

class ProjectExplorerPlugin;
class Project;
class Node;

namespace Internal {

class FolderNavigationWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool autoSynchronization READ autoSynchronization WRITE setAutoSynchronization)
public:
    FolderNavigationWidget(QWidget *parent = 0);

    bool autoSynchronization() const;

    static void findOnFileSystem(const QString &pathIn);
    static QString msgFindOnFileSystem();
    bool hiddenFilesFilter() const;

public slots:
    void setAutoSynchronization(bool sync);
    void toggleAutoSynchronization();

private slots:
    void setCurrentFile(const QString &filePath);
    void slotOpenItem(const QModelIndex &viewIndex);
    void setHiddenFilesFilter(bool filter);

protected:
    virtual void contextMenuEvent(QContextMenuEvent *ev);

private:
    void setCurrentTitle(QString dirName, const QString &fullPath);
    bool setCurrentDirectory(const QString &directory);
    void openItem(const QModelIndex &srcIndex);
    QModelIndex currentItem() const;
    QString currentDirectory() const;

    QListView *m_listView;
    QFileSystemModel *m_fileSystemModel;
    QAction *m_filterHiddenFilesAction;
    QSortFilterProxyModel *m_filterModel;
    QLabel *m_title;
    bool m_autoSync;
    QToolButton *m_toggleSync;
    // FolderNavigationWidgetFactory needs private members to build a menu
    friend class FolderNavigationWidgetFactory;
};

class FolderNavigationWidgetFactory : public Core::INavigationWidgetFactory
{
    Q_OBJECT
public:
    FolderNavigationWidgetFactory();
    ~FolderNavigationWidgetFactory();

    QString displayName() const;
    int priority() const;
    Core::Id id() const;
    QKeySequence activationSequence() const;
    Core::NavigationView createWidget();
    void saveSettings(int position, QWidget *widget);
    void restoreSettings(int position, QWidget *widget);
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // FOLDERNAVIGATIONWIDGET_H
