/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef KITINFORMATION_H
#define KITINFORMATION_H

#include "kitmanager.h"
#include "kit.h"

#include "devicesupport/idevice.h"
#include <utils/environment.h>

#include <QVariant>

namespace ProjectExplorer {

class ToolChain;
class KitConfigWidget;

// --------------------------------------------------------------------------
// SysRootInformation:
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT SysRootKitInformation : public KitInformation
{
    Q_OBJECT

public:
    SysRootKitInformation();

    QVariant defaultValue(Kit *k) const;

    QList<Task> validate(const Kit *k) const;

    KitConfigWidget *createConfigWidget(Kit *k) const;

    ItemList toUserOutput(const Kit *k) const;

    static Core::Id id();
    static bool hasSysRoot(const Kit *k);
    static Utils::FileName sysRoot(const Kit *k);
    static void setSysRoot(Kit *k, const Utils::FileName &v);
};

// --------------------------------------------------------------------------
// ToolChainInformation:
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT ToolChainKitInformation : public KitInformation
{
    Q_OBJECT

public:
    ToolChainKitInformation();

    QVariant defaultValue(Kit *k) const;

    QList<Task> validate(const Kit *k) const;
    void fix(Kit *k);
    void setup(Kit *k);

    KitConfigWidget *createConfigWidget(Kit *k) const;

    QString displayNamePostfix(const Kit *k) const;

    ItemList toUserOutput(const Kit *k) const;

    void addToEnvironment(const Kit *k, Utils::Environment &env) const;
    void addToMacroExpander(Kit *kit, Utils::MacroExpander *expander) const;
    IOutputParser *createOutputParser(const Kit *k) const;

    static Core::Id id();
    static ToolChain *toolChain(const Kit *k);
    static void setToolChain(Kit *k, ToolChain *tc);

    static QString msgNoToolChainInTarget();

private slots:
    void kitsWereLoaded();
    void toolChainUpdated(ProjectExplorer::ToolChain *tc);
    void toolChainRemoved(ProjectExplorer::ToolChain *tc);
};

// --------------------------------------------------------------------------
// DeviceTypeInformation:
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT DeviceTypeKitInformation : public KitInformation
{
    Q_OBJECT

public:
    DeviceTypeKitInformation();

    QVariant defaultValue(Kit *k) const;

    QList<Task> validate(const Kit *k) const;

    KitConfigWidget *createConfigWidget(Kit *k) const;

    ItemList toUserOutput(const Kit *k) const;

    static const Core::Id id();
    static const Core::Id deviceTypeId(const Kit *k);
    static void setDeviceTypeId(Kit *k, Core::Id type);

    static KitMatcher deviceTypeMatcher(Core::Id type);

    Core::FeatureSet availableFeatures(const Kit *k) const;
};

// --------------------------------------------------------------------------
// DeviceInformation:
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT DeviceKitInformation : public KitInformation
{
    Q_OBJECT

public:
    DeviceKitInformation();

    QVariant defaultValue(Kit *k) const;

    QList<Task> validate(const Kit *k) const;
    void fix(Kit *k);
    void setup(Kit *k);

    KitConfigWidget *createConfigWidget(Kit *k) const;

    QString displayNamePostfix(const Kit *k) const;

    ItemList toUserOutput(const Kit *k) const;

    virtual void addToMacroExpander(ProjectExplorer::Kit *kit, Utils::MacroExpander *expander) const;

    static Core::Id id();
    static IDevice::ConstPtr device(const Kit *k);
    static Core::Id deviceId(const Kit *k);
    static void setDevice(Kit *k, IDevice::ConstPtr dev);
    static void setDeviceId(Kit *k, Core::Id dataId);

private slots:
    void kitsWereLoaded();
    void deviceUpdated(Core::Id dataId);
    void devicesChanged();
    void kitUpdated(ProjectExplorer::Kit *k);
};

// --------------------------------------------------------------------------
// EnvironmentKitInformation:
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT EnvironmentKitInformation : public KitInformation
{
    Q_OBJECT

public:
    EnvironmentKitInformation();

    QVariant defaultValue(Kit *k) const;

    QList<Task> validate(const Kit *k) const;
    void fix(Kit *k);

    void addToEnvironment(const Kit *k, Utils::Environment &env) const;
    KitConfigWidget *createConfigWidget(Kit *k) const;

    ItemList toUserOutput(const Kit *k) const;

    static Core::Id id();
    static QList<Utils::EnvironmentItem> environmentChanges(const Kit *k);
    static void setEnvironmentChanges(Kit *k, const QList<Utils::EnvironmentItem> &changes);
};

} // namespace ProjectExplorer

#endif // KITINFORMATION_H
