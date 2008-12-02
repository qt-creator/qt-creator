/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
***************************************************************************/
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

#endif //VIEWMANAGER_H
