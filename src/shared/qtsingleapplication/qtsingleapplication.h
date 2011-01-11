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

#include <QtGui/QApplication>

namespace SharedTools {

class QtLocalPeer;

class QtSingleApplication : public QApplication
{
    Q_OBJECT

public:
    QtSingleApplication(int &argc, char **argv, bool GUIenabled = true);
    QtSingleApplication(const QString &id, int &argc, char **argv);
    QtSingleApplication(int &argc, char **argv, Type type);
#if defined(Q_WS_X11)
    explicit QtSingleApplication(Display *dpy, Qt::HANDLE visual = 0, Qt::HANDLE colormap = 0);
    QtSingleApplication(Display *dpy, int &argc, char **argv, Qt::HANDLE visual = 0, Qt::HANDLE cmap = 0);
#endif

    bool isRunning();
    QString id() const;

    void setActivationWindow(QWidget* aw, bool activateOnMessage = true);
    QWidget* activationWindow() const;
    bool event(QEvent *event);


public Q_SLOTS:
    bool sendMessage(const QString &message, int timeout = 5000);
    void activateWindow();

//Obsolete methods:
public:
    void initialize(bool = true)
        { isRunning(); }

#if defined(Q_WS_X11)
    QtSingleApplication(Display* dpy, const QString &id, int argc, char **argv, Qt::HANDLE visual = 0, Qt::HANDLE colormap = 0);
#endif
// end obsolete methods

Q_SIGNALS:
    void messageReceived(const QString &message);
    void fileOpenRequest(const QString &file);

private:
    void sysInit(const QString &appId = QString());
    QtLocalPeer *peer;
    QWidget *actWin;
};

} // namespace SharedTools
