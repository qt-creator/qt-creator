/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef KITINFORMATION_H
#define KITINFORMATION_H

#include "kitmanager.h"
#include "kit.h"

#include "devicesupport/idevice.h"
#include "toolchain.h"

#include <utils/fileutils.h>

#include <QVariant>

namespace ProjectExplorer {

class KitConfigWidget;

// --------------------------------------------------------------------------
// SysRootInformation:
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT SysRootKitInformation : public KitInformation
{
    Q_OBJECT

public:
    SysRootKitInformation();

    Core::Id dataId() const;
    unsigned int priority() const;

    QVariant defaultValue(Kit *k) const;

    QList<Task> validate(const Kit *k) const;

    KitConfigWidget *createConfigWidget(Kit *k) const;

    ItemList toUserOutput(Kit *k) const;

    static bool hasSysRoot(const Kit *k);
    static Utils::FileName sysRoot(const Kit *k);
    static void setSysRoot(Kit *k, const Utils::FileName &v);
};

class PROJECTEXPLORER_EXPORT SysRootMatcher : public KitMatcher
{
public:
    SysRootMatcher(const Utils::FileName &fn) : m_sysroot(fn)
    { }

    bool matches(const Kit *k) const
    {
        return SysRootKitInformation::sysRoot(k) == m_sysroot;
    }

private:
    Utils::FileName m_sysroot;
};

// --------------------------------------------------------------------------
// ToolChainInformation:
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT ToolChainKitInformation : public KitInformation
{
    Q_OBJECT

public:
    ToolChainKitInformation();

    Core::Id dataId() const;
    unsigned int priority() const;

    QVariant defaultValue(Kit *k) const;

    QList<Task> validate(const Kit *k) const;
    void fix(Kit *k);
    void setup(Kit *k);

    KitConfigWidget *createConfigWidget(Kit *k) const;

    QString displayNamePostfix(const Kit *k) const;

    ItemList toUserOutput(Kit *k) const;

    void addToEnvironment(const Kit *k, Utils::Environment &env) const;
    IOutputParser *createOutputParser(const Kit *k) const;

    static ToolChain *toolChain(const Kit *k);
    static void setToolChain(Kit *k, ToolChain *tc);

    static QString msgNoToolChainInTarget();
private slots:
    void toolChainUpdated(ProjectExplorer::ToolChain *tc);
};

class PROJECTEXPLORER_EXPORT ToolChainMatcher : public KitMatcher
{
public:
    ToolChainMatcher(const ToolChain *tc) : m_tc(tc)
    { }

    bool matches(const Kit *k) const
    {
        return ToolChainKitInformation::toolChain(k) == m_tc;
    }

private:
    const ToolChain *m_tc;
};

// --------------------------------------------------------------------------
// DeviceTypeInformation:
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT DeviceTypeKitInformation : public KitInformation
{
    Q_OBJECT

public:
    DeviceTypeKitInformation();

    Core::Id dataId() const;
    unsigned int priority() const;

    QVariant defaultValue(Kit *k) const;

    QList<Task> validate(const Kit *k) const;

    KitConfigWidget *createConfigWidget(Kit *k) const;

    ItemList toUserOutput(Kit *k) const;

    static const Core::Id deviceTypeId(const Kit *k);
    static void setDeviceTypeId(Kit *k, Core::Id type);
};

class PROJECTEXPLORER_EXPORT DeviceTypeMatcher : public KitMatcher
{
public:
    DeviceTypeMatcher(const Core::Id t) : m_type(t)
    { }

    bool matches(const Kit *k) const
    {
        Core::Id deviceType = DeviceTypeKitInformation::deviceTypeId(k);
        if (!deviceType.isValid())
            return false;
        return deviceType == m_type;
    }

private:
    const Core::Id m_type;
};

// --------------------------------------------------------------------------
// DeviceInformation:
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT DeviceKitInformation : public KitInformation
{
    Q_OBJECT

public:
    DeviceKitInformation();

    Core::Id dataId() const;
    unsigned int priority() const;

    QVariant defaultValue(Kit *k) const;

    QList<Task> validate(const Kit *k) const;
    void fix(Kit *k);

    KitConfigWidget *createConfigWidget(Kit *k) const;

    QString displayNamePostfix(const Kit *k) const;

    ItemList toUserOutput(Kit *k) const;

    static IDevice::ConstPtr device(const Kit *k);
    static Core::Id deviceId(const Kit *k);
    static void setDevice(Kit *k, IDevice::ConstPtr dev);
    static void setDeviceId(Kit *k, const Core::Id id);
private slots:
    void deviceUpdated(const Core::Id &id);
};

class PROJECTEXPLORER_EXPORT DeviceMatcher : public KitMatcher
{
public:
    DeviceMatcher(Core::Id id) : m_devId(id)
    { }

    bool matches(const Kit *k) const
    {
        return DeviceKitInformation::deviceId(k) == m_devId;
    }

private:
    Core::Id m_devId;
};

} // namespace ProjectExplorer

#endif // KITINFORMATION_H
