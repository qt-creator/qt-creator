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

#ifndef STATUSBARMANAGER_H
#define STATUSBARMANAGER_H

#include "minisplitter.h"

#include <QList>
#include <QObject>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace Core {
namespace Internal {

class MainWindow;

class NonResizingSplitter : public MiniSplitter
{
    Q_OBJECT

public:
    explicit NonResizingSplitter(QWidget *parent);

protected:
    void resizeEvent(QResizeEvent *ev);
};

class StatusBarManager : public QObject
{
    Q_OBJECT

public:
    StatusBarManager(MainWindow *mainWnd);
    ~StatusBarManager();

    void init();
    void extensionsInitalized();
    void restoreSettings();

private slots:
    void objectAdded(QObject *obj);
    void aboutToRemoveObject(QObject *obj);
    void saveSettings();

private:
    MainWindow *m_mainWnd;
    QSplitter *m_splitter;
    QList<QWidget *> m_statusBarWidgets;
};

} // namespace Internal
} // namespace Core

#endif // STATUSBARMANAGER_H
