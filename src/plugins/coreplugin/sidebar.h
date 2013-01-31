/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef SIDEBAR_H
#define SIDEBAR_H

#include "core_global.h"
#include "minisplitter.h"

#include <QMap>
#include <QList>

QT_BEGIN_NAMESPACE
class QSettings;
class QToolButton;
QT_END_NAMESPACE

namespace Core {

class Command;
struct SideBarPrivate;

namespace Internal {
class SideBarWidget;
} // namespace Internal

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
    virtual ~SideBarItem();

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
    virtual ~SideBar();

    QStringList availableItemIds() const;
    QStringList availableItemTitles() const;
    QStringList unavailableItemIds() const;
    void makeItemAvailable(SideBarItem *item);
    void setUnavailableItemIds(const QStringList &itemTitles);
    QString idForTitle(const QString &itemId) const;

    SideBarItem *item(const QString &title);

    bool closeWhenEmpty() const;
    void setCloseWhenEmpty(bool value);

    void saveSettings(QSettings *settings, const QString &name);
    void readSettings(QSettings *settings, const QString &name);
    void closeAllWidgets();
    void activateItem(SideBarItem *item);

    void setShortcutMap(const QMap<QString, Core::Command *> &shortcutMap);
    QMap<QString, Core::Command *> shortcutMap() const;

signals:
    void sideBarClosed();
    void availableItemsChanged();

private slots:
    void splitSubWidget();
    void closeSubWidget();
    void updateWidgets();

private:
    Internal::SideBarWidget *insertSideBarWidget(int position,
                                                 const QString &title = QString());
    void removeSideBarWidget(Internal::SideBarWidget *widget);

    SideBarPrivate *d;
};

} // namespace Core

#endif // SIDEBAR_H
