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

#include <QApplication>

QT_FORWARD_DECLARE_CLASS(QSharedMemory)

namespace SharedTools {

class QtLocalPeer;

class QtSingleApplication : public QApplication
{
    Q_OBJECT

public:
    QtSingleApplication(const QString &id, int &argc, char **argv);
    ~QtSingleApplication();

    bool isRunning(qint64 pid = -1);

    void setActivationWindow(QWidget* aw, bool activateOnMessage = true);
    QWidget* activationWindow() const;
    bool event(QEvent *event);

    QString applicationId() const;
    void setBlock(bool value);

public Q_SLOTS:
    bool sendMessage(const QString &message, int timeout = 5000, qint64 pid = -1);
    void activateWindow();

Q_SIGNALS:
    void messageReceived(const QString &message, QObject *socket);
    void fileOpenRequest(const QString &file);

private:
    QString instancesFileName(const QString &appId);

    qint64 firstPeer;
    QSharedMemory *instances;
    QtLocalPeer *pidPeer;
    QWidget *actWin;
    QString appId;
    bool block;
};

} // namespace SharedTools
