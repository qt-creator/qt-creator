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

#ifndef FOLDERNAVIGATIONWIDGET_H
#define FOLDERNAVIGATIONWIDGET_H

#include <coreplugin/inavigationwidgetfactory.h>

#include <QWidget>

namespace Utils { class ListView; }
namespace Core { class IEditor; }

QT_BEGIN_NAMESPACE
class QLabel;
class QSortFilterProxyModel;
class QModelIndex;
class QFileSystemModel;
class QAction;
QT_END_NAMESPACE

namespace ProjectExplorer {
namespace Internal {

class FolderNavigationWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool autoSynchronization READ autoSynchronization WRITE setAutoSynchronization)
public:
    FolderNavigationWidget(QWidget *parent = 0);

    bool autoSynchronization() const;
    bool hiddenFilesFilter() const;

public slots:
    void setAutoSynchronization(bool sync);
    void toggleAutoSynchronization();

private slots:
    void setCurrentFile(Core::IEditor *editor);
    void slotOpenItem(const QModelIndex &viewIndex);
    void setHiddenFilesFilter(bool filter);
    void ensureCurrentIndex();

protected:
    virtual void contextMenuEvent(QContextMenuEvent *ev);

private:
    void setCurrentTitle(QString dirName, const QString &fullPath);
    bool setCurrentDirectory(const QString &directory);
    void openItem(const QModelIndex &srcIndex, bool openDirectoryAsProject = false);
    QModelIndex currentItem() const;
    QString currentDirectory() const;

    Utils::ListView *m_listView;
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

    Core::NavigationView createWidget();
    void saveSettings(int position, QWidget *widget);
    void restoreSettings(int position, QWidget *widget);
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // FOLDERNAVIGATIONWIDGET_H
