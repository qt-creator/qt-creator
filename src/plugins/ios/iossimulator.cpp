/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "iossimulator.h"
#include "iosconstants.h"
#include "iostoolhandler.h"

#include <projectexplorer/kitinformation.h>
#include <utils/port.h>

#include <QCoreApplication>
#include <QMapIterator>
#include <QMutexLocker>
#include <QProcess>

using namespace ProjectExplorer;

namespace Ios {
namespace Internal {

static const QLatin1String iosDeviceTypeDisplayNameKey = QLatin1String("displayName");
static const QLatin1String iosDeviceTypeTypeKey = QLatin1String("type");
static const QLatin1String iosDeviceTypeIdentifierKey = QLatin1String("identifier");

IosSimulator::IosSimulator(Core::Id id)
    : IDevice(Core::Id(Constants::IOS_SIMULATOR_TYPE),
              IDevice::AutoDetected,
              IDevice::Emulator,
              id),
      m_lastPort(Constants::IOS_SIMULATOR_PORT_START)
{
    setDisplayName(QCoreApplication::translate("Ios::Internal::IosSimulator", "iOS Simulator"));
    setDeviceState(DeviceReadyToUse);
}

IosSimulator::IosSimulator()
    : IDevice(Core::Id(Constants::IOS_SIMULATOR_TYPE),
                             IDevice::AutoDetected,
                             IDevice::Emulator,
                             Core::Id(Constants::IOS_SIMULATOR_DEVICE_ID)),
      m_lastPort(Constants::IOS_SIMULATOR_PORT_START)
{
    setDisplayName(QCoreApplication::translate("Ios::Internal::IosSimulator", "iOS Simulator"));
    setDeviceState(DeviceReadyToUse);
}

IosSimulator::IosSimulator(const IosSimulator &other)
    : IDevice(other), m_lastPort(other.m_lastPort)
{
    setDisplayName(QCoreApplication::translate("Ios::Internal::IosSimulator", "iOS Simulator"));
    setDeviceState(DeviceReadyToUse);
}


IDevice::DeviceInfo IosSimulator::deviceInformation() const
{
    return IDevice::DeviceInfo();
}

QString IosSimulator::displayType() const
{
    return QCoreApplication::translate("Ios::Internal::IosSimulator", "iOS Simulator");
}

IDeviceWidget *IosSimulator::createWidget()
{
    return 0;
}

QList<Core::Id> IosSimulator::actionIds() const
{
    return QList<Core::Id>();
}

QString IosSimulator::displayNameForActionId(Core::Id actionId) const
{
    Q_UNUSED(actionId)
    return QString();
}

void IosSimulator::executeAction(Core::Id actionId, QWidget *parent)
{
    Q_UNUSED(actionId)
    Q_UNUSED(parent)
}

DeviceProcessSignalOperation::Ptr IosSimulator::signalOperation() const
{
    return DeviceProcessSignalOperation::Ptr();
}

IDevice::Ptr IosSimulator::clone() const
{
    return IDevice::Ptr(new IosSimulator(*this));
}

void IosSimulator::fromMap(const QVariantMap &map)
{
    IDevice::fromMap(map);
}

QVariantMap IosSimulator::toMap() const
{
    QVariantMap res = IDevice::toMap();
    return res;
}

Utils::Port IosSimulator::nextPort() const
{
    for (int i = 0; i < 100; ++i) {
        // use qrand instead?
        if (++m_lastPort >= Constants::IOS_SIMULATOR_PORT_END)
            m_lastPort = Constants::IOS_SIMULATOR_PORT_START;
        QProcess portVerifier;
        // this is a bit too broad (it does not check just listening sockets, but also connections
        // to that port from this computer)
        portVerifier.start(QLatin1String("lsof"), {"-n", "-P", "-i", QString(":%1").arg(m_lastPort) });
        if (!portVerifier.waitForStarted())
            break;
        portVerifier.closeWriteChannel();
        if (!portVerifier.waitForFinished() && portVerifier.state() == QProcess::Running)
            break;
        if (portVerifier.exitStatus() != QProcess::NormalExit
                || portVerifier.exitCode() != 0)
            break;
    }
    return Utils::Port(m_lastPort);
}

bool IosSimulator::canAutoDetectPorts() const
{
    return true;
}

IosSimulator::ConstPtr IosKitInformation::simulator(Kit *kit)
{
    if (!kit)
        return IosSimulator::ConstPtr();
    IDevice::ConstPtr dev = DeviceKitInformation::device(kit);
    IosSimulator::ConstPtr res = dev.dynamicCast<const IosSimulator>();
    return res;
}

IosDeviceType::IosDeviceType(IosDeviceType::Type type, const QString &identifier, const QString &displayName) :
    type(type), identifier(identifier), displayName(displayName)
{ }

bool IosDeviceType::fromMap(const QVariantMap &map)
{
    bool validType;
    displayName = map.value(iosDeviceTypeDisplayNameKey, QVariant()).toString();
    type = IosDeviceType::Type(map.value(iosDeviceTypeTypeKey, QVariant()).toInt(&validType));
    identifier = map.value(iosDeviceTypeIdentifierKey, QVariant()).toString();
    return validType && !displayName.isEmpty()
            && (type != IosDeviceType::SimulatedDevice || !identifier.isEmpty());
}

QVariantMap IosDeviceType::toMap() const
{
    QVariantMap res;
    res[iosDeviceTypeDisplayNameKey] = displayName;
    res[iosDeviceTypeTypeKey]        = type;
    res[iosDeviceTypeIdentifierKey]  = identifier;
    return res;
}

bool IosDeviceType::operator ==(const IosDeviceType &o) const
{
    return o.type == type && o.identifier == identifier && o.displayName == displayName;
}

// compare strings comparing embedded numbers as numeric values.
// the result is negative if x<y, zero if x==y, positive if x>y
// Prefixed 0 are used to resolve ties, so that this ordering is still a total ordering (equality
// only for identical strings)
// "20" > "3" , "03-4" < "3-10", "3-5" < "03-5"
static int numberCompare(const QString &s1, const QString &s2)
{
    int i1 = 0;
    int i2 = 0;
    int solveTie = 0;
    while (i1 < s1.size() && i2 < s2.size()) {
        QChar c1 = s1.at(i1);
        QChar c2 = s2.at(i2);
        if (c1.isDigit() && c2.isDigit()) {
            // we found a number on both sides, find where the number ends
            int j1 = i1 + 1;
            int j2 = i2 + 1;
            while (j1 < s1.size() && s1.at(j1).isDigit())
                ++j1;
            while (j2 < s2.size() && s2.at(j2).isDigit())
                ++j2;
            // and compare it from the right side, first units, then decimals,....
            int cmp = 0;
            int newI1 = j1;
            int newI2 = j2;
            while (j1 > i1 && j2 > i2) {
                --j1;
                --j2;
                QChar cc1 = s1.at(j1);
                QChar cc2 = s2.at(j2);
                if (cc1 < cc2)
                    cmp = -1;
                else if (cc1 > cc2)
                    cmp = 1;
            }
            int tie = 0;
            // if the left number has more digits, if they are all 0, use this info only to break
            // ties, otherwise the left number is larger
            while (j1-- > i1) {
                tie = 1;
                if (s1.at(j1) != QLatin1Char('0'))
                    cmp = 1;
            }
            // same for the right number
            while (j2-- > i2) {
                tie = -1;
                if (s2.at(j2) != QLatin1Char('0'))
                    cmp = -1;
            }
            // if not equal return
            if (cmp != 0)
                return cmp;
            // otherwise possibly store info to break ties (first nomber with more leading zeros is
            // larger)
            if (solveTie == 0)
                solveTie = tie;
            // continue comparing after the numbers
            i1 = newI1;
            i2 = newI2;
        } else {
            // compare plain characters (non numbers)
            if (c1 < c2)
                return -1;
            if (c1 > c2)
                return 1;
            ++i1; ++i2;
        }
    }
    // if one side has more characters it is the larger one
    if (i1 < s1.size())
        return 1;
    if (i2 < s2.size())
        return -1;
    // if we had differences in prefixed 0, use that choose the largest string, otherwise they are
    // equal
    return solveTie;
}

bool IosDeviceType::operator <(const IosDeviceType &o) const
{
    if (type < o.type)
        return true;
    if (type > o.type)
        return false;
    int cmp = numberCompare(displayName, o.displayName);
    if (cmp < 0)
        return true;
    if (cmp > 0)
        return false;
    cmp = numberCompare(identifier, o.identifier);
    if (cmp < 0)
        return true;
    return false;
}

QDebug operator <<(QDebug debug, const IosDeviceType &deviceType)
{
    if (deviceType.type == IosDeviceType::IosDevice)
        debug << "iOS Device " << deviceType.displayName << deviceType.identifier;
    else
        debug << deviceType.displayName << " (" << deviceType.identifier << ")";
    return debug;
}

} // namespace Internal
} // namespace Ios
