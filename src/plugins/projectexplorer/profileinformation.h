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

#ifndef PROFILEINFORMATION_H
#define PROFILEINFORMATION_H

#include "profilemanager.h"
#include "profile.h"

#include "devicesupport/idevice.h"
#include "toolchain.h"

#include <utils/fileutils.h>

#include <QVariant>

namespace ProjectExplorer {

class ProfileConfigWidget;

// --------------------------------------------------------------------------
// SysRootInformation:
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT SysRootProfileInformation : public ProfileInformation
{
    Q_OBJECT

public:
    SysRootProfileInformation();

    Core::Id dataId() const;
    unsigned int priority() const;

    QVariant defaultValue(Profile *p) const;

    QList<Task> validate(Profile *p) const;

    ProfileConfigWidget *createConfigWidget(Profile *p) const;

    ItemList toUserOutput(Profile *p) const;

    static bool hasSysRoot(const Profile *p);
    static Utils::FileName sysRoot(const Profile *p);
    static void setSysRoot(Profile *p, const Utils::FileName &v);
};

class PROJECTEXPLORER_EXPORT SysRootMatcher : public ProfileMatcher
{
public:
    SysRootMatcher(const Utils::FileName &fn) : m_sysroot(fn)
    { }

    bool matches(const Profile *p) const
    {
        return SysRootProfileInformation::sysRoot(p) == m_sysroot;
    }

private:
    Utils::FileName m_sysroot;
};

// --------------------------------------------------------------------------
// ToolChainInformation:
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT ToolChainProfileInformation : public ProfileInformation
{
    Q_OBJECT

public:
    ToolChainProfileInformation();

    Core::Id dataId() const;
    unsigned int priority() const;

    QVariant defaultValue(Profile *p) const;

    QList<Task> validate(Profile *p) const;

    ProfileConfigWidget *createConfigWidget(Profile *p) const;

    QString displayNamePostfix(const Profile *p) const;

    ItemList toUserOutput(Profile *p) const;

    void addToEnvironment(const Profile *p, Utils::Environment &env) const;

    static ToolChain *toolChain(const Profile *p);
    static void setToolChain(Profile *p, ToolChain *tc);

    static QString msgNoToolChainInTarget();
};

class PROJECTEXPLORER_EXPORT ToolChainMatcher : public ProfileMatcher
{
public:
    ToolChainMatcher(const ToolChain *tc) : m_tc(tc)
    { }

    bool matches(const Profile *p) const
    {
        return ToolChainProfileInformation::toolChain(p) == m_tc;
    }

private:
    const ToolChain *m_tc;
};

// --------------------------------------------------------------------------
// DeviceTypeInformation:
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT DeviceTypeProfileInformation : public ProfileInformation
{
    Q_OBJECT

public:
    DeviceTypeProfileInformation();

    Core::Id dataId() const;
    unsigned int priority() const;

    QVariant defaultValue(Profile *p) const;

    QList<Task> validate(Profile *p) const;

    ProfileConfigWidget *createConfigWidget(Profile *p) const;

    ItemList toUserOutput(Profile *p) const;

    static const Core::Id deviceTypeId(const Profile *p);
    static void setDeviceTypeId(Profile *p, Core::Id type);
};

class PROJECTEXPLORER_EXPORT DeviceTypeMatcher : public ProfileMatcher
{
public:
    DeviceTypeMatcher(const Core::Id t) : m_type(t)
    { }

    bool matches(const Profile *p) const
    {
        Core::Id deviceType = DeviceTypeProfileInformation::deviceTypeId(p);
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

class PROJECTEXPLORER_EXPORT DeviceProfileInformation : public ProfileInformation
{
    Q_OBJECT

public:
    DeviceProfileInformation();

    Core::Id dataId() const;
    unsigned int priority() const;

    QVariant defaultValue(Profile *p) const;

    QList<Task> validate(Profile *p) const;

    ProfileConfigWidget *createConfigWidget(Profile *p) const;

    QString displayNamePostfix(const Profile *p) const;

    ItemList toUserOutput(Profile *p) const;

    static IDevice::ConstPtr device(const Profile *p);
    static Core::Id deviceId(const Profile *p);
    static void setDevice(Profile *p, IDevice::ConstPtr dev);
    static void setDeviceId(Profile *p, const Core::Id id);
};

class PROJECTEXPLORER_EXPORT DeviceMatcher : public ProfileMatcher
{
public:
    DeviceMatcher(Core::Id id) : m_devId(id)
    { }

    bool matches(const Profile *p) const
    {
        return DeviceProfileInformation::deviceId(p) == m_devId;
    }

private:
    Core::Id m_devId;
};

} // namespace ProjectExplorer

#endif // PROFILEINFORMATION_H
