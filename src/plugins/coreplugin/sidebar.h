/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef SIDEBAR_H
#define SIDEBAR_H

#include "core_global.h"
#include "minisplitter.h"

#include <QtCore/QMap>
#include <QtCore/QList>
#include <QtCore/QScopedPointer>

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
    explicit SideBar(QList< SideBarItem*> widgetList,
            QList< SideBarItem*> defaultVisible);
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

    void setShortcutMap(const QMap<QString, Core::Command*> &shortcutMap);
    QMap<QString, Core::Command*> shortcutMap() const;

signals:
    void availableItemsChanged();

private slots:
    void splitSubWidget();
    void closeSubWidget();
    void updateWidgets();

private:
    Internal::SideBarWidget *insertSideBarWidget(int position,
                                                 const QString &title = QString());
    void removeSideBarWidget(Internal::SideBarWidget *widget);

    QScopedPointer<SideBarPrivate> d;
};

} // namespace Core

#endif // SIDEBAR_H
