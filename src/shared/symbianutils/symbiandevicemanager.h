/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef SYMBIANDEVICEMANAGER_H
#define SYMBIANDEVICEMANAGER_H

#include "symbianutils_global.h"

#include <QIODevice>
#include <QExplicitlySharedDataPointer>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QDebug;
class QTextStream;
QT_END_NAMESPACE

namespace Coda {
    class CodaDevice;
}

namespace SymbianUtils {

struct SymbianDeviceManagerPrivate;
class SymbianDeviceData;
class OstChannel;

enum DeviceCommunicationType {
    SerialPortCommunication = 0,
    BlueToothCommunication = 1
};

typedef QSharedPointer<Coda::CodaDevice> CodaDevicePtr;

// SymbianDevice: Explicitly shared device data and a CodaDevice
// instance that can be acquired (exclusively) for use.
// A device removal from the manager will result in the
// device being closed.
class SYMBIANUTILS_EXPORT SymbianDevice {
    explicit SymbianDevice(SymbianDeviceData *data);
    friend class SymbianDeviceManager;

public:
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

    static const char *linuxBlueToothDeviceRootC;

    // Do not use this constructor, it is just public for Q_GLOBAL_STATIC
    explicit SymbianDeviceManager(QObject *parent = 0);
    virtual ~SymbianDeviceManager();

    // Singleton access.
    static SymbianDeviceManager *instance();

    SymbianDeviceList devices() const;
    QString toString() const;

    //// The CODA code prefers to set up the CodaDevice object itself, so we let it and just handle opening the underlying QIODevice and keeping track of the CodaDevice
    //// Returns true if port was opened successfully.

    // Gets the CodaDevice, which may or may not be open depending on what other clients have already acquired it.
    // Therefore once clients have set up any signals and slots they required, they should check CodaDevice::device()->isOpen()
    // and if false, the open failed and they should check device()->errorString() if required.
    // Caller should call releaseCodaDevice if they want the port to auto-close itself
    CodaDevicePtr getCodaDevice(const QString &port);

    // Note this function makes no guarantee that someone else isn't already listening on this channel id, or that there is anything on the other end
    // Returns NULL if the port couldn't be opened
    OstChannel *getOstChannel(const QString &port, uchar channelId);

    // Caller is responsible for disconnecting any signals from aPort - do not assume the CodaDevice will be deleted as a result of this call. On return aPort will be clear()ed.
    void releaseCodaDevice(CodaDevicePtr &aPort);

    int findByPortName(const QString &p) const;
    QString friendlyNameForPort(const QString &port) const;

public slots:
    void update();
    void setAdditionalInformation(const QString &port, const QString &ai);

signals:
    void deviceRemoved(const SymbianUtils::SymbianDevice &d);
    void deviceAdded(const SymbianUtils::SymbianDevice &d);
    void updated();

private slots:
    void delayedClosePort();

private:
    void ensureInitialized() const;
    void update(bool emitSignals);
    SymbianDeviceList serialPorts() const;
    SymbianDeviceList blueToothDevices() const;
    void customEvent(QEvent *event);
    void constructCodaPort(CodaDevicePtr& device, const QString& portName);

    SymbianDeviceManagerPrivate *d;
};

SYMBIANUTILS_EXPORT QDebug operator<<(QDebug d, const SymbianDeviceManager &);

struct OstChannelPrivate;

class SYMBIANUTILS_EXPORT OstChannel : public QIODevice
{
    Q_OBJECT

public:
    void close();
    ~OstChannel();
    void flush();

    qint64 bytesAvailable() const;
    bool isSequential() const;
    bool hasReceivedData() const;

    Coda::CodaDevice &codaDevice() const;

private slots:
    void ostDataReceived(uchar channelId, const QByteArray &aData);
    void deviceAboutToClose();

private:
    OstChannel(const CodaDevicePtr &codaPtr, uchar channelId);
    qint64 readData(char *data, qint64 maxSize);
    qint64 writeData(const char *data, qint64 maxSize);

private:
    OstChannelPrivate *d;
    friend class SymbianDeviceManager;
};

} // namespace SymbianUtils

#endif // SYMBIANDEVICEMANAGER_H
