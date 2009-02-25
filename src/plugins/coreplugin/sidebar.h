/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef SIDEBAR_H
#define SIDEBAR_H

#include <QtCore/QMap>
#include <QtGui/QWidget>
#include <QtGui/QComboBox>

#include <coreplugin/minisplitter.h>

QT_BEGIN_NAMESPACE
class QSettings;
class QComboBox;
class QToolBar;
class QAction;
class QToolButton;
QT_END_NAMESPACE

namespace Core {

class Command;

namespace Internal {
class SideBarWidget;
class ComboBox;
} // namespace Internal

/*
 * An item in the sidebar. Has a widget that is displayed in the sidebar and
 * optionally a list of tool buttons that are added to the toolbar above it.
 * The window title of the widget is displayed in the combo box.
 *
 * The SideBarItem takes ownership over the widget.
 */
class CORE_EXPORT SideBarItem
{
public:
    SideBarItem(QWidget *widget)
        : m_widget(widget)
    {}

    virtual ~SideBarItem();

    QWidget *widget() { return m_widget; }

    /* Should always return a new set of tool buttons.
     *
     * Workaround since there doesn't seem to be a nice way to remove widgets
     * that have been added to a QToolBar without either not deleting the
     * associated QAction or causing the QToolButton to be deleted.
     */
    virtual QList<QToolButton *> createToolBarWidgets()
    {
        return QList<QToolButton *>();
    }

private:
    QWidget *m_widget;
};

class CORE_EXPORT SideBar : public MiniSplitter
{
    Q_OBJECT
public:
    /*
     * The SideBar takes ownership of the SideBarItems.
     */
    SideBar(QList<SideBarItem*> widgetList,
            QList<SideBarItem*> defaultVisible);
    ~SideBar();

    QStringList availableItems() const;
    void makeItemAvailable(SideBarItem *item);
    SideBarItem *item(const QString &title);

    void saveSettings(QSettings *settings);
    void readSettings(QSettings *settings);

    void activateItem(SideBarItem *item);

    void setShortcutMap(const QMap<QString, Core::Command*> &shortcutMap);
    QMap<QString, Core::Command*> shortcutMap() const;

private slots:
    void split();
    void close();
    void updateWidgets();

private:
    Internal::SideBarWidget *insertSideBarWidget(int position,
                                                 const QString &title = QString());
    QList<Internal::SideBarWidget*> m_widgets;

    QMap<QString, SideBarItem*> m_itemMap;
    QStringList m_availableItems;
    QStringList m_defaultVisible;
    QMap<QString, Core::Command*> m_shortcutMap;
};

namespace Internal {

class SideBarWidget : public QWidget
{
    Q_OBJECT
public:
    SideBarWidget(SideBar *sideBar, const QString &title);
    ~SideBarWidget();

    QString currentItemTitle() const;
    void setCurrentItem(const QString &title);

    void updateAvailableItems();
    void removeCurrentItem();

    Core::Command *command(const QString &title) const;

signals:
    void split();
    void close();
    void currentWidgetChanged();

private slots:
    void setCurrentIndex(int);

private:
    ComboBox *m_comboBox;
    SideBarItem *m_currentItem;
    QToolBar *m_toolbar;
    QAction *m_splitAction;
    QList<QAction *> m_addedToolBarActions;
    SideBar *m_sideBar;
    QToolButton *m_splitButton;
    QToolButton *m_closeButton;
};

class ComboBox : public QComboBox
{
    Q_OBJECT

public:
    ComboBox(SideBarWidget *sideBarWidget);

protected:
    bool event(QEvent *event);

private:
    SideBarWidget *m_sideBarWidget;
};

} // namespace Internal
} // namespace Core

#endif // SIDEBAR_H
