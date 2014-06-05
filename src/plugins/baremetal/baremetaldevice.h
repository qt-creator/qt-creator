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

#ifndef BAREMETALDEVICE_H
#define BAREMETALDEVICE_H

#include <projectexplorer/devicesupport/idevice.h>

namespace BareMetal {
namespace Internal {

class BareMetalDevice : public ProjectExplorer::IDevice
{
public:
    typedef QSharedPointer<BareMetalDevice> Ptr;
    typedef QSharedPointer<const BareMetalDevice> ConstPtr;

    static Ptr create();
    static Ptr create(const QString &name, Core::Id type, MachineType machineType,
                      Origin origin = ManuallyAdded, Core::Id id = Core::Id());
    static Ptr create(const BareMetalDevice &other);

    QString displayType() const;
    ProjectExplorer::IDeviceWidget *createWidget();
    QList<Core::Id> actionIds() const;
    QString displayNameForActionId(Core::Id actionId) const;
    void executeAction(Core::Id actionId, QWidget *parent);
    ProjectExplorer::IDevice::Ptr clone() const;

    ProjectExplorer::DeviceProcessSignalOperation::Ptr signalOperation() const;

    QString gdbResetCommands() const { return m_gdbResetCommands; }
    void setGdbResetCommands(const QString &gdbResetCommands) { m_gdbResetCommands = gdbResetCommands; }

    QString gdbInitCommands() const { return m_gdbInitCommands; }
    void setGdbInitCommands(const QString &gdbCommands) { m_gdbInitCommands=gdbCommands; }

    virtual void fromMap(const QVariantMap &map);
    virtual QVariantMap toMap() const;

    static QString exampleString();
    static QString hostLineToolTip();
    static QString initCommandToolTip();
    static QString resetCommandToolTip();

protected:
    BareMetalDevice() {}
    BareMetalDevice(const QString &name, Core::Id type,
                    MachineType machineType, Origin origin, Core::Id id);
    BareMetalDevice(const BareMetalDevice &other);

private:
    BareMetalDevice &operator=(const BareMetalDevice &);
    QString m_gdbResetCommands;
    QString m_gdbInitCommands;
};

} //namespace Internal
} //namespace BareMetal

#endif // BAREMETALDEVICE_H
