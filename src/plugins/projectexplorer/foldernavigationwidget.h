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

#include <QWidget>

namespace Core { class IEditor; }

namespace Utils {
class FileName;
class NavigationTreeView;
}

QT_BEGIN_NAMESPACE
class QAction;
class QComboBox;
class QFileSystemModel;
class QModelIndex;
QT_END_NAMESPACE

namespace ProjectExplorer {
namespace Internal {

class FolderNavigationWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool autoSynchronization READ autoSynchronization WRITE setAutoSynchronization)
public:
    explicit FolderNavigationWidget(QWidget *parent = nullptr);

    static QStringList projectFilesInDirectory(const QString &path);

    bool autoSynchronization() const;
    bool hiddenFilesFilter() const;

    void setAutoSynchronization(bool sync);
    void toggleAutoSynchronization();

    void addRootDirectory(const QString &displayName, const Utils::FileName &directory);
    void removeRootDirectory(const Utils::FileName &directory);

protected:
    void contextMenuEvent(QContextMenuEvent *ev) override;

private:
    void setHiddenFilesFilter(bool filter);
    void setCurrentEditor(Core::IEditor *editor);
    void selectFile(const Utils::FileName &filePath);
    void setRootDirectory(const Utils::FileName &directory);
    int bestRootForFile(const Utils::FileName &filePath);
    void openItem(const QModelIndex &index);

    Utils::NavigationTreeView *m_listView = nullptr;
    QFileSystemModel *m_fileSystemModel = nullptr;
    QAction *m_filterHiddenFilesAction = nullptr;
    bool m_autoSync = false;
    QToolButton *m_toggleSync = nullptr;
    QComboBox *m_rootSelector = nullptr;

    // FolderNavigationWidgetFactory needs private members to build a menu
    friend class FolderNavigationWidgetFactory;
};

class FolderNavigationWidgetFactory : public Core::INavigationWidgetFactory
{
    Q_OBJECT

public:
    FolderNavigationWidgetFactory();

    Core::NavigationView createWidget() override;
    void saveSettings(QSettings *settings, int position, QWidget *widget) override;
    void restoreSettings(QSettings *settings, int position, QWidget *widget) override;

    static void addRootDirectory(const QString &displayName, const Utils::FileName &directory);
    static void removeRootDirectory(const Utils::FileName &directory);

signals:
    void rootDirectoryAdded(const QString &displayName, const Utils::FileName &directory);
    void rootDirectoryRemoved(const Utils::FileName &directory);

private:
    using DirectoryEntry = std::pair<QString, Utils::FileName>;
    static QVector<DirectoryEntry> m_rootDirectories;
};

} // namespace Internal
} // namespace ProjectExplorer
