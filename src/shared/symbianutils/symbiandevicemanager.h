/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef SYMBIANDEVICEMANAGER_H
#define SYMBIANDEVICEMANAGER_H

#include "symbianutils_global.h"

#include <QtCore/QObject>
#include <QtCore/QExplicitlySharedDataPointer>
#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE
class QDebug;
class QTextStream;
QT_END_NAMESPACE

namespace trk {
    class TrkDevice;
}

namespace SymbianUtils {

struct SymbianDeviceManagerPrivate;
class SymbianDeviceData;

enum DeviceCommunicationType {
    SerialPortCommunication = 0,
    BlueToothCommunication = 1
};

// SymbianDevice: Explicitly shared device data and a TrkDevice
// instance that can be acquired (exclusively) for use.
// A device removal from the manager will result in the
// device being closed.
class SYMBIANUTILS_EXPORT SymbianDevice {
    explicit SymbianDevice(SymbianDeviceData *data);
    friend class SymbianDeviceManager;
public:
    typedef QSharedPointer<trk::TrkDevice> TrkDevicePtr;

    SymbianDevice();
    SymbianDevice(const SymbianDevice &rhs);
    SymbianDevice &operator=(const SymbianDevice &rhs);
    ~SymbianDevice();
    int compare(const SymbianDevice &rhs) const;

    DeviceCommunicationType type() const;
    bool isNull() const;
    QString portName() const;
    QString friendlyName() const;
    QString additionalInformation() const;
    void setAdditionalInformation(const QString &);

    // Acquire: Mark the device as 'out' and return a shared pointer
    // unless it is already in use by another owner. The result should not
    // be passed on further.
    TrkDevicePtr acquireDevice();
    // Give back a device and mark it as 'free'.
    void releaseDevice(TrkDevicePtr *ptr = 0);

    bool isOpen() const;

    // Windows only.
    QString deviceDesc() const;
    QString manufacturer() const;

    void format(QTextStream &str) const;
    QString toString() const;

private:
    void forcedClose();

    QExplicitlySharedDataPointer<SymbianDeviceData> m_data;
};

SYMBIANUTILS_EXPORT QDebug operator<<(QDebug d, const SymbianDevice &);

inline bool operator==(const SymbianDevice &d1, const SymbianDevice &d2)
    { return d1.compare(d2) == 0; }
inline bool operator!=(const SymbianDevice &d1, const SymbianDevice &d2)
    { return d1.compare(d2) != 0; }
inline bool operator<(const SymbianDevice &d1, const SymbianDevice &d2)
    { return d1.compare(d2) < 0; }

/* SymbianDeviceManager: Singleton that maintains a list of Symbian devices.
 * and emits change signals.
 * On Windows, the update slot must be connected to a [delayed] signal
 * emitted from an event handler listening for WM_DEVICECHANGE.
 * Device removal will result in the device being closed. */
class SYMBIANUTILS_EXPORT SymbianDeviceManager : public QObject
{
    Q_OBJECT
public:
    typedef QList<SymbianDevice> SymbianDeviceList;
    typedef QSharedPointer<trk::TrkDevice> TrkDevicePtr;

    static const char *linuxBlueToothDeviceRootC;

    // Do not use this constructor, it is just public for Q_GLOBAL_STATIC
    explicit SymbianDeviceManager(QObject *parent = 0);
    virtual ~SymbianDeviceManager();

    // Singleton access.
    static SymbianDeviceManager *instance();

    SymbianDeviceList devices() const;
    QString toString() const;

    // Acquire a device for use. See releaseDevice().
    TrkDevicePtr acquireDevice(const QString &port);

    int findByPortName(const QString &p) const;
    QString friendlyNameForPort(const QString &port) const;

public slots:
    void update();
    // Relase a device, make it available for further use.
    void releaseDevice(const QString &port);
    void setAdditionalInformation(const QString &port, const QString &ai);

signals:
    void deviceRemoved(const SymbianUtils::SymbianDevice &d);
    void deviceAdded(const SymbianUtils::SymbianDevice &d);
    void updated();

private:
    void ensureInitialized() const;
    void update(bool emitSignals);
    SymbianDeviceList serialPorts() const;
    SymbianDeviceList blueToothDevices() const;

    SymbianDeviceManagerPrivate *d;
};

SYMBIANUTILS_EXPORT QDebug operator<<(QDebug d, const SymbianDeviceManager &);

} // namespace SymbianUtils

#endif // SYMBIANDEVICEMANAGER_H
