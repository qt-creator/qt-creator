/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "bluetoothlistener_gui.h"
#include "bluetoothlistener.h"

#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>

namespace trk {

StartBluetoothGuiResult
    startBluetoothGui(AbstractBluetoothStarter &starter,
                      QWidget *msgBoxParent,
                      QString *errorMessage)
{
    errorMessage->clear();
    switch (starter.start()) {
    case AbstractBluetoothStarter::Started:
        break;
    case AbstractBluetoothStarter::ConnectionSucceeded:
        return BluetoothGuiConnected;
    case AbstractBluetoothStarter::StartError:
        *errorMessage = starter.errorString();
        return BluetoothGuiError;
    }
    // Run the starter with the event loop of a message box, close it
    // with the finished signals.
    const QString title = QCoreApplication::translate("trk::startBluetoothGui", "Waiting for Bluetooth Connection");
    const QString message = QCoreApplication::translate("trk::startBluetoothGui", "Connecting to %1...").arg(starter.device());
    QMessageBox messageBox(QMessageBox::Information, title, message, QMessageBox::Cancel, msgBoxParent);
    QObject::connect(&starter, SIGNAL(connected()), &messageBox, SLOT(close()));
    QObject::connect(&starter, SIGNAL(timeout()), &messageBox, SLOT(close()));
    messageBox.exec();
    // Only starter.state() is reliable here.
    if (starter.state() == AbstractBluetoothStarter::Running) {
        *errorMessage = QCoreApplication::translate("trk::startBluetoothGui", "Connection on %1 canceled.").arg(starter.device());
        return BluetoothGuiCanceled;
    }
    if (starter.state() != AbstractBluetoothStarter::Connected) {
        *errorMessage = starter.errorString();
        return BluetoothGuiError;
    }
    return BluetoothGuiConnected;
}
} // namespace trk
