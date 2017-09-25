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

#include <coreplugin/minisplitter.h>
#include <coreplugin/id.h>

#include <QHash>

QT_BEGIN_NAMESPACE
class QSettings;
class QAbstractItemModel;
QT_END_NAMESPACE

namespace Core {
class INavigationWidgetFactory;
class Command;
class NavigationWidget;
struct NavigationWidgetPrivate;
namespace Internal { class NavigationSubWidget; }

enum class Side {
    Left,
    Right
};

class CORE_EXPORT NavigationWidgetPlaceHolder : public QWidget
{
    Q_OBJECT
    friend class Core::NavigationWidget;

public:
    explicit NavigationWidgetPlaceHolder(Id mode, Side side, QWidget *parent = 0);
    virtual ~NavigationWidgetPlaceHolder();
    static NavigationWidgetPlaceHolder *current(Side side);
    static void setCurrent(Side side, NavigationWidgetPlaceHolder *navWidget);
    void applyStoredSize();

private:
    void currentModeAboutToChange(Id mode);
    int storedWidth() const;

    Id m_mode;
    Side m_side;
    static NavigationWidgetPlaceHolder *s_currentLeft;
    static NavigationWidgetPlaceHolder *s_currentRight;
};

class CORE_EXPORT NavigationWidget : public MiniSplitter
{
    Q_OBJECT

public:
    enum FactoryModelRoles {
        FactoryObjectRole = Qt::UserRole,
        FactoryIdRole,
        FactoryPriorityRole
    };

    explicit NavigationWidget(QAction *toggleSideBarAction, Side side);
    virtual ~NavigationWidget();

    void setFactories(const QList<INavigationWidgetFactory*> &factories);

    QString settingsGroup() const;
    void saveSettings(QSettings *settings);
    void restoreSettings(QSettings *settings);

    QWidget *activateSubWidget(Id factoryId, int preferredPosition);
    void closeSubWidgets();

    bool isShown() const;
    void setShown(bool b);

    static NavigationWidget *instance(Side side);
    static QWidget *activateSubWidget(Id factoryId, Side fallbackSide);

    int storedWidth();

    // Called from the place holders
    void placeHolderChanged(NavigationWidgetPlaceHolder *holder);

    QHash<Id, Command *> commandMap() const;
    QAbstractItemModel *factoryModel() const;

protected:
    void resizeEvent(QResizeEvent *);

private:
    void splitSubWidget(int factoryIndex);
    void closeSubWidget();
    void updateToggleText();
    Internal::NavigationSubWidget *insertSubItem(int position, int factoryIndex);
    int factoryIndex(Id id);
    QString settingsKey(const QString &key) const;
    void onSubWidgetFactoryIndexChanged(int factoryIndex);

    NavigationWidgetPrivate *d;
};

} // namespace Core
