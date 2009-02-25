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

#ifndef VIEWMANAGER_H
#define VIEWMANAGER_H

#include "viewmanagerinterface.h"

#include <QtCore/QMap>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QAction;
class QSettings;
class QMainWindow;
class QComboBox;
class QStackedWidget;
QT_END_NAMESPACE

namespace Core {

class UniqueIDManager;

namespace Internal {

class MainWindow;
class NavigationWidget;

class ViewManager
  : public Core::ViewManagerInterface
{
    Q_OBJECT

public:
    ViewManager(MainWindow *mainWnd);
    ~ViewManager();

    void init();
    void extensionsInitalized();
    void saveSettings(QSettings *settings);

    IView * view(const QString & id);
private slots:
    void objectAdded(QObject *obj);
    void aboutToRemoveObject(QObject *obj);

private:
    QMap<Core::IView *, QWidget *> m_viewMap;

    MainWindow *m_mainWnd;
    QList<QWidget *> m_statusBarWidgets;
};

} // namespace Internal
} // namespace Core

#endif // VIEWMANAGER_H
