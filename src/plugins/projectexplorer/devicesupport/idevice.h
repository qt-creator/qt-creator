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

#include "../projectexplorer_export.h"

#include <coreplugin/id.h>

#include <QList>
#include <QSharedPointer>
#include <QVariantMap>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace ProjectExplorer {
namespace Internal { class IDevicePrivate; }
class IDeviceWidget;

// See cpp file for documentation.
class PROJECTEXPLORER_EXPORT IDevice
{
public:
    typedef QSharedPointer<IDevice> Ptr;
    typedef QSharedPointer<const IDevice> ConstPtr;

    enum Origin { ManuallyAdded, AutoDetected };

    virtual ~IDevice();

    QString displayName() const;
    void setDisplayName(const QString &name);

    // Provide some information on the device suitable for formated
    // output, e.g. in tool tips. Get a list of name value pairs.
    class DeviceInfoItem {
    public:
        DeviceInfoItem(const QString &k, const QString &v) : key(k), value(v) { }

        QString key;
        QString value;
    };
    typedef QList<DeviceInfoItem> DeviceInfo;
    virtual DeviceInfo deviceInformation() const;

    Core::Id type() const;
    bool isAutoDetected() const;
    Core::Id id() const;

    virtual QString displayType() const = 0;
    virtual IDeviceWidget *createWidget() = 0;
    virtual QList<Core::Id> actionIds() const = 0;
    virtual QString displayNameForActionId(Core::Id actionId) const = 0;
    virtual void executeAction(Core::Id actionId, QWidget *parent = 0) const = 0;

    enum AvailabilityState { DeviceAvailable, DeviceUnavailable, DeviceAvailabilityUnknown };
    AvailabilityState availability() const;
    void setAvailability(const AvailabilityState as);

    virtual void fromMap(const QVariantMap &map);
    virtual QVariantMap toMap() const;
    virtual Ptr clone() const = 0;

    static Core::Id invalidId();

    static Core::Id typeFromMap(const QVariantMap &map);
    static Core::Id idFromMap(const QVariantMap &map);

protected:
    IDevice();
    IDevice(Core::Id type, Origin origin, Core::Id id = Core::Id());
    IDevice(const IDevice &other);

    Ptr sharedFromThis();
    ConstPtr sharedFromThis() const;

private:
    IDevice &operator=(const IDevice &);

    Internal::IDevicePrivate *d;
};

} // namespace ProjectExplorer

#endif // IDEVICE_H
