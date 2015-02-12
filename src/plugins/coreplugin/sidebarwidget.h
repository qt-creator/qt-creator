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

#ifndef SIDEBARWIDGET_H
#define SIDEBARWIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QSettings;
class QToolBar;
class QAction;
class QToolButton;
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

private slots:
    void setCurrentIndex(int);

private:
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

#endif // SIDEBARWIDGET_H
