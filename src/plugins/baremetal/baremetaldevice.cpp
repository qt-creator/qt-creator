/****************************************************************************
**
** Copyright (C) 2014 Tim Sander <tim@krieglstein.org>
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

#include "baremetaldevice.h"
#include "baremetaldeviceconfigurationwidget.h"
#include <coreplugin/id.h>
#include <utils/qtcassert.h>
#include <QCoreApplication>

using namespace ProjectExplorer;

namespace BareMetal {
namespace Internal {

const char GdbResetKey[] = "GdbResetCommand";
const char GdbCommandsKey[] = "GdbCommands";

BareMetalDevice::Ptr BareMetalDevice::create()
{
    return Ptr(new BareMetalDevice);
}

BareMetalDevice::Ptr BareMetalDevice::create(const QString &name, Core::Id type, MachineType machineType, Origin origin, Core::Id id)
{
    return Ptr(new BareMetalDevice(name, type, machineType, origin, id));
}

BareMetalDevice::Ptr BareMetalDevice::create(const BareMetalDevice &other)
{
    return Ptr(new BareMetalDevice(other));
}

void BareMetalDevice::fromMap(const QVariantMap &map)
{
    IDevice::fromMap(map);
    setGdbResetCommands(map.value(QLatin1String(GdbResetKey)).toString());
    setGdbInitCommands(map.value(QLatin1String(GdbCommandsKey)).toString());
}

QVariantMap BareMetalDevice::toMap() const
{
    QVariantMap map = IDevice::toMap();
    map.insert(QLatin1String(GdbResetKey), gdbResetCommands());
    map.insert(QLatin1String(GdbCommandsKey), gdbInitCommands());
    return map;
}

BareMetalDevice::IDevice::Ptr BareMetalDevice::clone() const
{
    return Ptr(new BareMetalDevice(*this));
}

DeviceProcessSignalOperation::Ptr BareMetalDevice::signalOperation() const
{
    return ProjectExplorer::DeviceProcessSignalOperation::Ptr();
}

QString BareMetalDevice::displayType() const
{
    return QCoreApplication::translate("BareMetal::Internal::BareMetalDevice", "Bare Metal");
}

ProjectExplorer::IDeviceWidget *BareMetalDevice::createWidget()
{
    return new BareMetalDeviceConfigurationWidget(sharedFromThis());
}

QList<Core::Id> BareMetalDevice::actionIds() const
{
    return QList<Core::Id>(); // no actions
}

QString BareMetalDevice::displayNameForActionId(Core::Id actionId) const
{
    QTC_ASSERT(actionIds().contains(actionId), return QString());
    return QString();
}

void BareMetalDevice::executeAction(Core::Id actionId, QWidget *parent)
{
    QTC_ASSERT(actionIds().contains(actionId), return);
    Q_UNUSED(parent);
}

BareMetalDevice::BareMetalDevice(const QString &name, Core::Id type, MachineType machineType, Origin origin, Core::Id id)
    : IDevice(type, origin, machineType, id)
{
    setDisplayName(name);
}

BareMetalDevice::BareMetalDevice(const BareMetalDevice &other)
    : IDevice(other)
{
    setGdbResetCommands(other.gdbResetCommands());
    setGdbInitCommands(other.gdbInitCommands());
}

QString BareMetalDevice::exampleString()
{
    return QLatin1String("<p><i>")
            + QCoreApplication::translate("BareMetal", "Example:")
            + QLatin1String("</i><p>");
}

QString BareMetalDevice::hostLineToolTip()
{
    return QLatin1String("<html>")
            + QCoreApplication::translate("BareMetal",
              "Enter your hostname like \"localhost\" or \"192.0.2.1\" or "
              "a command which must support GDB pipelining "
              "starting with a pipe symbol.")
            + exampleString() + QLatin1String(
              "&nbsp;&nbsp;|openocd -c \"gdb_port pipe; "
              "log_output openocd.log\" -f boards/myboard.cfg");
}

QString BareMetalDevice::resetCommandToolTip()
{
    return QLatin1String("<html>")
            + QCoreApplication::translate("BareMetal",
              "Enter the hardware reset command here.<br>"
              "The CPU should be halted after this command.")
            + exampleString() + QLatin1String(
              "&nbsp;&nbsp;monitor reset halt");
}

QString BareMetalDevice::initCommandToolTip()
{
    return QLatin1String("<html>")
            + QCoreApplication::translate("BareMetal",
              "Enter commands to reset the board, and write the nonvolatile memory.")
            + exampleString() + QLatin1String(
              "&nbsp;&nbsp;set remote hardware-breakpoint-limit 6<br/>"
              "&nbsp;&nbsp;set remote hardware-watchpoint-limit 4<br/>"
              "&nbsp;&nbsp;monitor reset halt<br/>"
              "&nbsp;&nbsp;load<br/>"
              "&nbsp;&nbsp;monitor reset halt");
}

} //namespace Internal
} //namespace BareMetal
