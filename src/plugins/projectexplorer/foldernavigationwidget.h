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
    explicit FolderNavigationWidget(QWidget *parent = nullptr);

    static QStringList projectFilesInDirectory(const QString &path);

    bool autoSynchronization() const;
    bool hiddenFilesFilter() const;

    void setAutoSynchronization(bool sync);
    void toggleAutoSynchronization();

private:
    void setCurrentFile(Core::IEditor *editor);
    void slotOpenItem(const QModelIndex &viewIndex);
    void setHiddenFilesFilter(bool filter);
    void ensureCurrentIndex();

protected:
    void contextMenuEvent(QContextMenuEvent *ev) override;

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
    bool m_autoSync = false;
    QToolButton *m_toggleSync;

    // FolderNavigationWidgetFactory needs private members to build a menu
    friend class FolderNavigationWidgetFactory;
};

class FolderNavigationWidgetFactory : public Core::INavigationWidgetFactory
{
    Q_OBJECT

public:
    FolderNavigationWidgetFactory();

    Core::NavigationView createWidget() override;
    void saveSettings(int position, QWidget *widget) override;
    void restoreSettings(int position, QWidget *widget) override;
};

} // namespace Internal
} // namespace ProjectExplorer
