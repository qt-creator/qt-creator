/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
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
**
**
**
**
**
**
**
** $QT_END_LICENSE$
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
        : o(UnknownOrientation)
    {
        // enable the orientation sensor
        QDBusConnection::systemBus().call(
                QDBusMessage::createMethodCall(MCE_SERVICE, MCE_REQUEST_PATH,
                                               MCE_REQUEST_IF, MCE_ACCELEROMETER_ENABLE_REQ));

        // query the initial orientation
        QDBusMessage reply = QDBusConnection::systemBus().call(
                QDBusMessage::createMethodCall(MCE_SERVICE, MCE_REQUEST_PATH,
                                               MCE_REQUEST_IF, MCE_DEVICE_ORIENTATION_GET));
        if (reply.type() == QDBusMessage::ErrorMessage) {
            qWarning("Unable to retrieve device orientation: %s", qPrintable(reply.errorMessage()));
        } else {
            o = toOrientation(reply.arguments().value(0).toString());
        }

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
};

DeviceOrientation* DeviceOrientation::instance()
{
    static MaemoOrientation *o = new MaemoOrientation;
    return o;
}

#include "deviceorientation_maemo5.moc"
