// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"
#include "minisplitter.h"

#include <QMap>
#include <QList>

QT_BEGIN_NAMESPACE
class QToolButton;
QT_END_NAMESPACE

namespace Utils { class QtcSettings; }

namespace Core {

class Command;
struct SideBarPrivate;

namespace Internal { class SideBarWidget; }

/*
 * An item in the sidebar. Has a widget that is displayed in the sidebar and
 * optionally a list of tool buttons that are added to the toolbar above it.
 * The window title of the widget is displayed in the combo box.
 *
 * The SideBarItem takes ownership over the widget.
 */
class CORE_EXPORT SideBarItem : public QObject
{
    Q_OBJECT
public:
    // id is non-localized string of the item that's used to store the settings.
    explicit SideBarItem(QWidget *widget, const QString &id);
    ~SideBarItem() override;

    QWidget *widget() const;
    QString id() const;
    QString title() const;

    /* Should always return a new set of tool buttons.
     *
     * Workaround since there doesn't seem to be a nice way to remove widgets
     * that have been added to a QToolBar without either not deleting the
     * associated QAction or causing the QToolButton to be deleted.
     */
    virtual QList<QToolButton *> createToolBarWidgets();

private:
    const QString m_id;
    QWidget *m_widget;
};

class CORE_EXPORT SideBar : public MiniSplitter
{
    Q_OBJECT

public:
    /*
     * The SideBar takes explicit ownership of the SideBarItems
     * if you have one SideBar, or shared ownership in case
     * of multiple SideBars.
     */
    SideBar(QList<SideBarItem *> widgetList, QList<SideBarItem *> defaultVisible);
    ~SideBar() override;

    QStringList availableItemIds() const;
    QStringList availableItemTitles() const;
    QStringList unavailableItemIds() const;
    void makeItemAvailable(SideBarItem *item);
    void setUnavailableItemIds(const QStringList &itemTitles);
    QString idForTitle(const QString &title) const;

    SideBarItem *item(const QString &title);

    bool closeWhenEmpty() const;
    void setCloseWhenEmpty(bool value);

    void saveSettings(Utils::QtcSettings *settings, const QString &name);
    void readSettings(Utils::QtcSettings *settings, const QString &name);
    void closeAllWidgets();
    void activateItem(const QString &id);

    void setShortcutMap(const QMap<QString, Command *> &shortcutMap);
    QMap<QString, Command *> shortcutMap() const;

signals:
    void sideBarClosed();
    void availableItemsChanged();

private:
    void splitSubWidget(Internal::SideBarWidget *widget);
    void closeSubWidget(Internal::SideBarWidget *widget);
    void updateWidgets();

    Internal::SideBarWidget *insertSideBarWidget(int position,
                                                 const QString &title = QString());
    void removeSideBarWidget(Internal::SideBarWidget *widget);

    SideBarPrivate *d;
};

} // namespace Core
