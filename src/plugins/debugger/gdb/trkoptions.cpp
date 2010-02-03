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

#include "trkoptions.h"
#include <utils/synchronousprocess.h>

#include <QtCore/QSettings>
#include <QtCore/QFileInfo>
#include <QtCore/QCoreApplication>

#ifdef Q_OS_WIN
#    define SERIALPORT_ROOT "COM"
enum { firstSerialPort = 1, lastSerialPort = 12 };
enum { modeDefault = Debugger::Internal::TrkOptions::Serial };
static const char *serialPortDefaultC = SERIALPORT_ROOT"1";
static const char *gdbDefaultC = "gdb-arm-none-symbianelf.exe";
#else
#    define SERIALPORT_ROOT "/dev/ttyS"
enum { firstSerialPort = 0, lastSerialPort = 3 };
enum { modeDefault = Debugger::Internal::TrkOptions::BlueTooth };
static const char *serialPortDefaultC = SERIALPORT_ROOT"0";
static const char *gdbDefaultC = "gdb-arm-none-symbianelf";
#endif

static const char *settingsGroupC = "S60Debugger";
static const char *serialPortKeyC = "Port";
static const char *modeKeyC = "Mode";
static const char *blueToothDeviceKeyC = "BlueToothDevice";
static const char *blueToothDeviceDefaultC = "/dev/rfcomm0";
static const char *gdbKeyC = "gdb";

namespace Debugger {
namespace Internal {

TrkOptions::TrkOptions() :
        mode(modeDefault),
        serialPort(QLatin1String(serialPortDefaultC)),
        blueToothDevice(QLatin1String(blueToothDeviceDefaultC)),
        gdb(QLatin1String(gdbDefaultC))
{
}

void TrkOptions::fromSettings(const QSettings *s)
{
    const QString keyRoot = QLatin1String(settingsGroupC) + QLatin1Char('/');
    mode = s->value(keyRoot + QLatin1String(modeKeyC), modeDefault).toInt();
    serialPort = s->value(keyRoot + QLatin1String(serialPortKeyC), QLatin1String(serialPortDefaultC)).toString();
    gdb =  s->value(keyRoot + QLatin1String(gdbKeyC),QLatin1String(gdbDefaultC)).toString();
    blueToothDevice = s->value(keyRoot + QLatin1String(blueToothDeviceKeyC), QLatin1String(blueToothDeviceDefaultC)).toString();
}

void TrkOptions::toSettings(QSettings *s) const
{
    s->beginGroup(QLatin1String(settingsGroupC));
    s->setValue(QLatin1String(modeKeyC), mode);
    s->setValue(QLatin1String(serialPortKeyC), serialPort);
    s->setValue(QLatin1String(blueToothDeviceKeyC), blueToothDevice);
    s->setValue(QLatin1String(gdbKeyC), gdb);
    s->endGroup();
}

bool TrkOptions::check(QString *errorMessage) const
{
    if (gdb.isEmpty()) {
        *errorMessage = QCoreApplication::translate("TrkOptions", "No Symbian gdb executable specified.");
        return false;
    }
    const QString expanded = Utils::SynchronousProcess::locateBinary(gdb);
    if (expanded.isEmpty()) {
        *errorMessage = QCoreApplication::translate("TrkOptions", "The Symbian gdb executable '%1' could not be found in the search path.").arg(gdb);
        return false;
    }
    return true;
}

bool TrkOptions::equals(const  TrkOptions &o) const
{
    return mode == o.mode
            && serialPort == o.serialPort
            && blueToothDevice == o.blueToothDevice
            && gdb == o.gdb;
}

QStringList TrkOptions::serialPorts()
{
    QStringList rc;
    const QString root = QLatin1String(SERIALPORT_ROOT);
    for (int p = firstSerialPort; p != lastSerialPort; p++)
        rc.push_back(root + QString::number(p));
    return rc;
}

QStringList TrkOptions::blueToothDevices()
{
    QStringList rc;
    rc.push_back(QLatin1String(blueToothDeviceDefaultC));
    return rc;
}

} // namespace Internal
} // namespace Debugger
