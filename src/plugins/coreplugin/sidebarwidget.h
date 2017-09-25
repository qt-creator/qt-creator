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

#include <QWidget>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
QT_END_NAMESPACE

namespace Core {
class SideBar;
class SideBarItem;
class Command;

namespace Internal {
class SideBarComboBox;

class SideBarWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SideBarWidget(SideBar *sideBar, const QString &title);
    virtual ~SideBarWidget();

    QString currentItemId() const;
    QString currentItemTitle() const;
    void setCurrentItem(const QString &id);

    void updateAvailableItems();
    void removeCurrentItem();

    Command *command(const QString &title) const;

    void setCloseIcon(const QIcon &icon);

signals:
    void splitMe();
    void closeMe();
    void currentWidgetChanged();

private:
    void setCurrentIndex(int);

    SideBarComboBox *m_comboBox;
    SideBarItem *m_currentItem;
    QToolBar *m_toolbar;
    QAction *m_splitAction;
    QAction *m_closeAction;
    QList<QAction *> m_addedToolBarActions;
    SideBar *m_sideBar;
};

} // namespace Internal
} // namespace Core
