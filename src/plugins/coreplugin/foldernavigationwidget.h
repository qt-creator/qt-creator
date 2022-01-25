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

#include "core_global.h"
#include "inavigationwidgetfactory.h"

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
class QtcSettings;
}

QT_BEGIN_NAMESPACE
class QAction;
class QComboBox;
class QFileSystemModel;
class QMenu;
class QModelIndex;
class QSortFilterProxyModel;
QT_END_NAMESPACE

namespace Core {

namespace Internal {
class DelayedFileCrumbLabel;
} // namespace Internal

class CORE_EXPORT FolderNavigationWidgetFactory : public Core::INavigationWidgetFactory
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

    static FolderNavigationWidgetFactory *instance();

    FolderNavigationWidgetFactory();

    Core::NavigationView createWidget() override;
    void saveSettings(Utils::QtcSettings *settings, int position, QWidget *widget) override;
    void restoreSettings(QSettings *settings, int position, QWidget *widget) override;

    static void insertRootDirectory(const RootDirectory &directory);
    static void removeRootDirectory(const QString &id);

    static void setFallbackSyncFilePath(const Utils::FilePath &filePath);

    static const Utils::FilePath &fallbackSyncFilePath();

signals:
    void rootDirectoryAdded(const RootDirectory &directory);
    void rootDirectoryRemoved(const QString &id);

    void aboutToShowContextMenu(QMenu *menu, const Utils::FilePath &filePath, bool isDir);
    void fileRenamed(const Utils::FilePath &before, const Utils::FilePath &after);
    void aboutToRemoveFile(const Utils::FilePath &filePath);

private:
    static int rootIndex(const QString &id);
    void updateProjectsDirectoryRoot();
    void registerActions();

    static QVector<RootDirectory> m_rootDirectories;
    static Utils::FilePath m_fallbackSyncFilePath;
};

class CORE_EXPORT FolderNavigationWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool autoSynchronization READ autoSynchronization WRITE setAutoSynchronization)
public:
    explicit FolderNavigationWidget(QWidget *parent = nullptr);

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

    void syncWithFilePath(const Utils::FilePath &filePath);

    bool eventFilter(QObject *obj, QEvent *event) override;

protected:
    void contextMenuEvent(QContextMenuEvent *ev) override;

private slots:
    void setCrumblePath(const Utils::FilePath &filePath);

private:
    bool rootAutoSynchronization() const;
    void setRootAutoSynchronization(bool sync);
    void setHiddenFilesFilter(bool filter);
    void selectBestRootForFile(const Utils::FilePath &filePath);
    void handleCurrentEditorChanged(Core::IEditor *editor);    void selectFile(const Utils::FilePath &filePath);
    void setRootDirectory(const Utils::FilePath &directory);
    int bestRootForFile(const Utils::FilePath &filePath);
    void openItem(const QModelIndex &index);
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
    Internal::DelayedFileCrumbLabel *m_crumbLabel = nullptr;

    // FolderNavigationWidgetFactory needs private members to build a menu
    friend class FolderNavigationWidgetFactory;
};

} // namespace Core
