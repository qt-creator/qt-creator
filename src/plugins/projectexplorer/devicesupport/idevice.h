/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#ifndef IDEVICE_H
#define IDEVICE_H

#include <projectexplorer/projectexplorer_export.h>

#include <QSharedPointer>
#include <QVariantMap>

namespace ProjectExplorer {
namespace Internal { class IDevicePrivate; }

// See cpp file for documentation.
class PROJECTEXPLORER_EXPORT IDevice
{
    friend class DeviceManager;
public:
    typedef QSharedPointer<IDevice> Ptr;
    typedef QSharedPointer<const IDevice> ConstPtr;

    typedef quint64 Id;

    enum Origin { ManuallyAdded, AutoDetected };

    virtual ~IDevice();

    QString displayName() const;
    void setDisplayName(const QString &name);

    QString type() const;
    bool isAutoDetected() const;
    QString fingerprint() const;
    Id internalId() const;

    virtual void fromMap(const QVariantMap &map);
    virtual Ptr clone() const = 0;

    static Id invalidId();

    static QString typeFromMap(const QVariantMap &map);

protected:
    IDevice();
    IDevice(const QString &type, Origin origin, const QString fingerprint = QString());
    IDevice(const IDevice &other);

    virtual QVariantMap toMap() const;

private:
    void setInternalId(Id id);

    IDevice &operator=(const IDevice &);

    Internal::IDevicePrivate *d;
};

} // namespace ProjectExplorer

#endif // IDEVICE_H
