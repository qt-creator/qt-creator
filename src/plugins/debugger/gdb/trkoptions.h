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

#ifndef TRKOPTIONS_H
#define TRKOPTIONS_H

#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

/* Parameter to be used for debugging S60 via TRK.
 * GDB is a Symbian-ARM Gdb.
 * Communication happens either via BlueTooth (Linux only) or
 * serial ports. */

struct TrkOptions
{
    enum Mode { Serial, BlueTooth };

    TrkOptions();
    void fromSettings(const QSettings *s);
    void toSettings(QSettings *s) const;
    bool equals(const  TrkOptions &o) const;

    bool check(QString *errorMessage) const;

    // Lists of choices for the devices
    static QStringList serialPorts();
    static QStringList blueToothDevices();

    int mode;
    QString serialPort;
    QString blueToothDevice;
    QString gdb;
};

inline bool operator==(const TrkOptions &o1, const TrkOptions &o2)
{
    return o1.equals(o2);
}

inline bool operator!=(const TrkOptions &o1, const TrkOptions &o2)
{
    return !o1.equals(o2);
}

} // namespace Internal
} // namespace Debugger

#endif // TRKOPTIONS_H
