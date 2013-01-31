/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "deviceorientation.h"
#include <QtDBus>

#include <mce/mode-names.h>
#include <mce/dbus-names.h>

class MaemoOrientation : public DeviceOrientation
{
    Q_OBJECT
public:
    MaemoOrientation()
        : o(UnknownOrientation), sensorEnabled(false)
    {
        resumeListening();
        // connect to the orientation change signal
        QDBusConnection::systemBus().connect(QString(), MCE_SIGNAL_PATH, MCE_SIGNAL_IF,
                MCE_DEVICE_ORIENTATION_SIG,
                this,
                SLOT(deviceOrientationChanged(QString)));
    }

    ~MaemoOrientation()
    {
        // disable the orientation sensor
        QDBusConnection::systemBus().call(
                QDBusMessage::createMethodCall(MCE_SERVICE, MCE_REQUEST_PATH,
                                               MCE_REQUEST_IF, MCE_ACCELEROMETER_DISABLE_REQ));
    }

    inline Orientation orientation() const
    {
        return o;
    }

    void setOrientation(Orientation o)
    {
    }

    void pauseListening() {
        if (sensorEnabled) {
            // disable the orientation sensor
            QDBusConnection::systemBus().call(
                    QDBusMessage::createMethodCall(MCE_SERVICE, MCE_REQUEST_PATH,
                                                   MCE_REQUEST_IF, MCE_ACCELEROMETER_DISABLE_REQ));
            sensorEnabled = false;
        }
    }

    void resumeListening() {
        if (!sensorEnabled) {
            // enable the orientation sensor
            QDBusConnection::systemBus().call(
                    QDBusMessage::createMethodCall(MCE_SERVICE, MCE_REQUEST_PATH,
                                                   MCE_REQUEST_IF, MCE_ACCELEROMETER_ENABLE_REQ));

            QDBusMessage reply = QDBusConnection::systemBus().call(
                    QDBusMessage::createMethodCall(MCE_SERVICE, MCE_REQUEST_PATH,
                                                   MCE_REQUEST_IF, MCE_DEVICE_ORIENTATION_GET));

            if (reply.type() == QDBusMessage::ErrorMessage) {
                qWarning("Unable to retrieve device orientation: %s", qPrintable(reply.errorMessage()));
            } else {
                Orientation orientation = toOrientation(reply.arguments().value(0).toString());
                if (o != orientation) {
                    o = orientation;
                    emit orientationChanged();
                }
                sensorEnabled = true;
            }
        }
    }

private Q_SLOTS:
    void deviceOrientationChanged(const QString &newOrientation)
    {
        o = toOrientation(newOrientation);

        emit orientationChanged();
//        printf("%d\n", o);
    }

private:
    static Orientation toOrientation(const QString &nativeOrientation)
    {
        if (nativeOrientation == MCE_ORIENTATION_LANDSCAPE)
            return Landscape;
        else if (nativeOrientation == MCE_ORIENTATION_LANDSCAPE_INVERTED)
            return LandscapeInverted;
        else if (nativeOrientation == MCE_ORIENTATION_PORTRAIT)
            return Portrait;
        else if (nativeOrientation == MCE_ORIENTATION_PORTRAIT_INVERTED)
            return PortraitInverted;
        return UnknownOrientation;
    }

private:
    Orientation o;
    bool sensorEnabled;
};

DeviceOrientation* DeviceOrientation::instance()
{
    static MaemoOrientation *o = new MaemoOrientation;
    return o;
}

#include "deviceorientation_maemo5.moc"
