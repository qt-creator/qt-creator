// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    ~SideBarWidget() override;

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

    SideBarComboBox *m_comboBox = nullptr;
    SideBarItem *m_currentItem = nullptr;
    QToolBar *m_toolbar = nullptr;
    QAction *m_splitAction = nullptr;
    QAction *m_closeAction = nullptr;
    QList<QAction *> m_addedToolBarActions;
    SideBar *m_sideBar = nullptr;
};

} // namespace Internal
} // namespace Core
