/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <coreplugin/inavigationwidgetfactory.h>
#include <utils/fileutils.h>

#include <QIcon>
#include <QWidget>

namespace Core {
class IContext;
class IEditor;
}

namespace Utils {
class NavigationTreeView;
class FileCrumbLabel;
}

QT_BEGIN_NAMESPACE
class QAction;
class QComboBox;
class QFileSystemModel;
class QModelIndex;
class QSortFilterProxyModel;
QT_END_NAMESPACE

namespace ProjectExplorer {
namespace Internal {

class DelayedFileCrumbLabel;

class FolderNavigationWidgetFactory : public Core::INavigationWidgetFactory
{
    Q_OBJECT

public:
    struct RootDirectory {
        QString id;
        int sortValue;
        QString displayName;
        Utils::FilePath path;
        QIcon icon;
    };

    FolderNavigationWidgetFactory();

    Core::NavigationView createWidget() override;
    void saveSettings(QSettings *settings, int position, QWidget *widget) override;
    void restoreSettings(QSettings *settings, int position, QWidget *widget) override;

    static void insertRootDirectory(const RootDirectory &directory);
    static void removeRootDirectory(const QString &id);

signals:
    void rootDirectoryAdded(const RootDirectory &directory);
    void rootDirectoryRemoved(const QString &id);

private:
    static int rootIndex(const QString &id);
    void updateProjectsDirectoryRoot();
    void registerActions();

    static QVector<RootDirectory> m_rootDirectories;
};

class FolderNavigationWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool autoSynchronization READ autoSynchronization WRITE setAutoSynchronization)
public:
    explicit FolderNavigationWidget(QWidget *parent = nullptr);

    static QStringList projectFilesInDirectory(const QString &path);

    bool autoSynchronization() const;
    bool hiddenFilesFilter() const;
    bool isShowingBreadCrumbs() const;
    bool isShowingFoldersOnTop() const;

    void setAutoSynchronization(bool sync);
    void toggleAutoSynchronization();
    void setShowBreadCrumbs(bool show);
    void setShowFoldersOnTop(bool onTop);

    void insertRootDirectory(const FolderNavigationWidgetFactory::RootDirectory &directory);
    void removeRootDirectory(const QString &id);

    void addNewItem();
    void editCurrentItem();
    void removeCurrentItem();

protected:
    void contextMenuEvent(QContextMenuEvent *ev) override;

private slots:
    void setCrumblePath(const Utils::FilePath &filePath);

private:
    bool rootAutoSynchronization() const;
    void setRootAutoSynchronization(bool sync);
    void setHiddenFilesFilter(bool filter);
    void selectBestRootForFile(const Utils::FilePath &filePath);
    void handleCurrentEditorChanged(Core::IEditor *editor);
    void selectFile(const Utils::FilePath &filePath);
    void setRootDirectory(const Utils::FilePath &directory);
    int bestRootForFile(const Utils::FilePath &filePath);
    void openItem(const QModelIndex &index);
    QStringList projectsInDirectory(const QModelIndex &index) const;
    void openProjectsInDirectory(const QModelIndex &index);
    void createNewFolder(const QModelIndex &parent);

    Utils::NavigationTreeView *m_listView = nullptr;
    QFileSystemModel *m_fileSystemModel = nullptr;
    QSortFilterProxyModel *m_sortProxyModel = nullptr;
    QAction *m_filterHiddenFilesAction = nullptr;
    QAction *m_showBreadCrumbsAction = nullptr;
    QAction *m_showFoldersOnTopAction = nullptr;
    bool m_autoSync = false;
    bool m_rootAutoSync = true;
    QToolButton *m_toggleSync = nullptr;
    QToolButton *m_toggleRootSync = nullptr;
    QComboBox *m_rootSelector = nullptr;
    QWidget *m_crumbContainer = nullptr;
    DelayedFileCrumbLabel *m_crumbLabel = nullptr;

    // FolderNavigationWidgetFactory needs private members to build a menu
    friend class FolderNavigationWidgetFactory;
};

} // namespace Internal
} // namespace ProjectExplorer
