/****************************************************************************
**
** Copyright (C) 2018 Andre Hartmann <aha_1980@gmx.de>
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

#include <QPointer>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QModelIndex;
class QPoint;
class QToolButton;
class QTreeView;
QT_END_NAMESPACE;

namespace Utils {
class ElidingLabel;
class NavigationTreeView;
}

namespace Git {
namespace Internal {

class BranchModel;
class BranchFilterModel;

class BranchView : public QWidget
{
    Q_OBJECT

public:
    explicit BranchView();

    void refreshIfSame(const QString &repository);
    void refresh(const QString &repository, bool force);

    QToolButton *addButton() const;
    QToolButton *refreshButton() const;

    QAction *m_includeOldEntriesAction = nullptr;
    QAction *m_includeTagsAction = nullptr;

private:
    void refreshCurrentRepository();
    void resizeColumns();
    void slotCustomContextMenu(const QPoint &point);
    void expandAndResize();
    void setIncludeOldEntries(bool filter);
    void setIncludeTags(bool includeTags);
    QModelIndex selectedIndex();
    bool add();
    bool checkout();
    bool remove();
    bool rename();
    bool reset();
    bool isFastForwardMerge();
    bool merge(bool allowFastForward);
    void rebase();
    bool cherryPick();
    void log(const QModelIndex &idx);

    QToolButton *m_addButton = nullptr;
    QToolButton *m_refreshButton = nullptr;
    Utils::ElidingLabel *m_repositoryLabel = nullptr;
    Utils::NavigationTreeView *m_branchView = nullptr;
    BranchModel *m_model = nullptr;
    BranchFilterModel *m_filterModel = nullptr;
    QString m_repository;
};

class BranchViewFactory : public Core::INavigationWidgetFactory
{
    Q_OBJECT

public:
    BranchViewFactory();

    BranchView *view() const;

private:
    Core::NavigationView createWidget() override;

    QPointer<BranchView> m_view;
};

} // namespace Internal
} // namespace Git
