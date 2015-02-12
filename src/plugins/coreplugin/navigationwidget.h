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

#ifndef NAVIGATIONWIDGET_H
#define NAVIGATIONWIDGET_H

#include <coreplugin/minisplitter.h>
#include <coreplugin/id.h>

#include <QHash>

QT_BEGIN_NAMESPACE
class QSettings;
class QAbstractItemModel;
class QStandardItemModel;
QT_END_NAMESPACE

namespace Core {
class INavigationWidgetFactory;
class IMode;
class Command;
class NavigationWidget;
struct NavigationWidgetPrivate;
namespace Internal { class NavigationSubWidget; }

class CORE_EXPORT NavigationWidgetPlaceHolder : public QWidget
{
    Q_OBJECT
    friend class Core::NavigationWidget;

public:
    explicit NavigationWidgetPlaceHolder(IMode *mode, QWidget *parent = 0);
    virtual ~NavigationWidgetPlaceHolder();
    static NavigationWidgetPlaceHolder* current();
    void applyStoredSize(int width);

private:
    void currentModeAboutToChange(IMode *);

    IMode *m_mode;
    static NavigationWidgetPlaceHolder* m_current;
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

    explicit NavigationWidget(QAction *toggleSideBarAction);
    virtual ~NavigationWidget();

    void setFactories(const QList<INavigationWidgetFactory*> &factories);

    void saveSettings(QSettings *settings);
    void restoreSettings(QSettings *settings);

    void activateSubWidget(Id factoryId);
    void closeSubWidgets();

    bool isShown() const;
    void setShown(bool b);

    bool isSuppressed() const;
    void setSuppressed(bool b);

    static NavigationWidget* instance();

    int storedWidth();

    // Called from the place holders
    void placeHolderChanged(NavigationWidgetPlaceHolder *holder);

    QHash<Id, Command *> commandMap() const;
    QAbstractItemModel *factoryModel() const;

protected:
    void resizeEvent(QResizeEvent *);

private slots:
    void activateSubWidget();
    void splitSubWidget(int factoryIndex);
    void closeSubWidget();

private:
    void updateToggleText();
    Internal::NavigationSubWidget *insertSubItem(int position, int index);
    int factoryIndex(Id id);

    NavigationWidgetPrivate *d;
};

} // namespace Core

#endif // NAVIGATIONWIDGET_H
