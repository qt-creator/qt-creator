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

#include "iosconfigurations.h"
#include "iosconstants.h"
#include "iosdevice.h"
#include "iossimulator.h"
#include "iosprobe.h"

#include <coreplugin/icore.h>
#include <utils/hostosinfo.h>
#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/gcctoolchain.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <qt4projectmanager/qmakekitinformation.h>
#include <debugger/debuggerkitinformation.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qtversionfactory.h>

#include <QDateTime>
#include <QSettings>
#include <QStringList>
#include <QProcess>
#include <QFileInfo>
#include <QDirIterator>
#include <QMetaObject>
#include <QList>

#include <QStringListModel>
#include <QMessageBox>

#if defined(Q_OS_UNIX)
#include <unistd.h>
#endif

using namespace ProjectExplorer;
using namespace Utils;

static const bool debugProbe = false;

namespace Ios {
namespace Internal {

namespace {
    const QLatin1String SettingsGroup("IosConfigurations");
    const QLatin1String ignoreAllDevicesKey("IgnoreAllDevices");

}

void IosConfigurations::updateAutomaticKitList()
{
    QMap<QString, Platform> platforms = IosProbe::detectPlatforms();
    {
        QMapIterator<QString, Platform> iter(platforms);
        while (iter.hasNext()) {
            iter.next();
            const Platform &p = iter.value();
            setDeveloperPath(p.developerPath);
            break;
        }
}
    QMap<QString, ProjectExplorer::GccToolChain *> platformToolchainMap;
    // check existing toolchains (and remove old ones)
    foreach (ProjectExplorer::ToolChain *tc, ProjectExplorer::ToolChainManager::toolChains()) {
        if (!tc->isAutoDetected()) // use also user toolchains?
            continue;
        if (tc->type() != QLatin1String("clang") && tc->type() != QLatin1String("gcc"))
            continue;
        ProjectExplorer::GccToolChain *toolchain = static_cast<ProjectExplorer::GccToolChain *>(tc);
        QMapIterator<QString, Platform> iter(platforms);
        bool found = false;
        while (iter.hasNext()) {
            iter.next();
            const Platform &p = iter.value();
            if (p.compilerPath == toolchain->compilerCommand()
                    && p.backendFlags == toolchain->platformCodeGenFlags()) {
                platformToolchainMap[p.name] = toolchain;
                found = true;
            }
        }
        iter.toFront();
        while (iter.hasNext()) {
            iter.next();
            const Platform &p = iter.value();
            if (p.platformKind)
                continue;
            if (p.compilerPath == toolchain->compilerCommand()
                    && p.backendFlags == toolchain->platformCodeGenFlags()) {
                platformToolchainMap[p.name] = toolchain;
                found = true;
            }
        }
        if (!found && (tc->displayName().startsWith(QLatin1String("iphone"))
                       || tc->displayName().startsWith(QLatin1String("mac")))) {
            qDebug() << "removing toolchain" << tc->displayName();
            ProjectExplorer::ToolChainManager::deregisterToolChain(tc);
        }
    }
    // add missing toolchains
    {
        QMapIterator<QString, Platform> iter(platforms);
        while (iter.hasNext()) {
            iter.next();
            const Platform &p = iter.value();
            if (platformToolchainMap.contains(p.name)
                || (p.platformKind & Platform::BasePlatform) == 0
                || (p.name.startsWith(QLatin1String("iphone"))
                    && (p.platformKind & Platform::Cxx11Support) != 0))
                continue;
            ProjectExplorer::GccToolChain *toolchain;
            if (p.compilerPath.toFileInfo().baseName().startsWith(QLatin1String("clang")))
                toolchain = new ProjectExplorer::ClangToolChain(
                            ProjectExplorer::ToolChain::AutoDetection);
            else
                toolchain = new ProjectExplorer::GccToolChain(
                            QLatin1String(ProjectExplorer::Constants::GCC_TOOLCHAIN_ID),
                            ProjectExplorer::ToolChain::AutoDetection);
            QString baseDisplayName = p.name;
            QString displayName = baseDisplayName;
            for (int iVers = 1; iVers < 100; ++iVers) {
                bool unique = true;
                foreach (ProjectExplorer::ToolChain *existingTC, ProjectExplorer::ToolChainManager::toolChains()) {
                    if (existingTC->displayName() == displayName) {
                        unique = false;
                        break;
                    }
                }
                if (unique) break;
                displayName = baseDisplayName + QLatin1String("-") + QString::number(iVers);
            }
            toolchain->setDisplayName(displayName);
            toolchain->setPlatformCodeGenFlags(p.backendFlags);
            toolchain->setPlatformLinkerFlags(p.backendFlags);
            toolchain->setCompilerCommand(p.compilerPath);
            ProjectExplorer::ToolChainManager::registerToolChain(toolchain);
            platformToolchainMap.insert(p.name, toolchain);
            QMapIterator<QString, Platform> iter2(iter);
            while (iter2.hasNext()) {
                iter2.next();
                const Platform &p2 = iter2.value();
                if (!platformToolchainMap.contains(p2.name)
                        && p2.compilerPath == toolchain->compilerCommand()
                        && p2.backendFlags == toolchain->platformCodeGenFlags()) {
                    platformToolchainMap[p2.name] = toolchain;
                }
            }
        }
    }
    // filter out all non iphone, non base, non clang or cxx11 platforms, as we don't set up kits for those
    {
        QStringList toRemove;
        QMapIterator<QString, Platform> iter(platforms);
        while (iter.hasNext()) {
            iter.next();
            const Platform &p = iter.value();
            if (!p.name.startsWith(QLatin1String("iphone")) || (p.platformKind & Platform::BasePlatform) == 0
                    || (p.platformKind & Platform::Cxx11Support) != 0
                    || !p.compilerPath.toString().contains(QLatin1String("clang")))
                toRemove.append(p.name);
        }
        foreach (const QString &pName, toRemove) {
            if (debugProbe)
                qDebug() << "filtering out " << pName;
            platforms.remove(pName);
        }
    }
    QMap<ProjectExplorer::Abi::Architecture, QList<QtSupport::BaseQtVersion *> > qtVersionsForArch;
    foreach (QtSupport::BaseQtVersion *qtVersion, QtSupport::QtVersionManager::versions()) {
        if (debugProbe)
            qDebug() << "qt type " << qtVersion->type();
        if (qtVersion->type() != QLatin1String(Constants::IOSQT)) {
            if (qtVersion->qmakeProperty("QMAKE_PLATFORM").contains(QLatin1String("ios"))
                    || qtVersion->qmakeProperty("QMAKE_XSPEC").contains(QLatin1String("ios"))) {
                // replace with an ios version
                QtSupport::BaseQtVersion *iosVersion =
                        QtSupport::QtVersionFactory::createQtVersionFromQMakePath(
                            qtVersion->qmakeCommand(),
                            qtVersion->isAutodetected(),
                            qtVersion->autodetectionSource());
                if (iosVersion && iosVersion->type() == QLatin1String(Constants::IOSQT)) {
                    if (debugProbe)
                        qDebug() << "converting QT to iOS QT for " << qtVersion->qmakeCommand().toUserOutput();
                    QtSupport::QtVersionManager::removeVersion(qtVersion);
                    QtSupport::QtVersionManager::addVersion(iosVersion);
                    qtVersion = iosVersion;
                } else {
                    continue;
                }
            } else {
                continue;
            }
        }
        if (!qtVersion->isValid())
            continue;
        QList<ProjectExplorer::Abi> qtAbis = qtVersion->qtAbis();
        if (qtAbis.empty())
            continue;
        if (debugProbe)
            qDebug() << "qt arch " << qtAbis.first().architecture();
        foreach (const ProjectExplorer::Abi &abi, qtAbis)
            qtVersionsForArch[abi.architecture()].append(qtVersion);
    }

    QList<ProjectExplorer::Kit *> existingKits;
    QList<bool> kitMatched;
    foreach (ProjectExplorer::Kit *k, ProjectExplorer::KitManager::kits()) {
        Core::Id deviceKind=ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(k);
        if (deviceKind != Core::Id(Constants::IOS_DEVICE_TYPE)
                && deviceKind != Core::Id(Constants::IOS_SIMULATOR_TYPE)
                && deviceKind != Core::Id(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)){
            if (debugProbe)
                qDebug() << "skipping existing kit with deviceKind " << deviceKind.toString();
            continue;
        }
        if (!k->isAutoDetected()) // use also used set kits?
            continue;
        existingKits << k;
        kitMatched << false;
    }
    // create missing kits
    {
        QMapIterator<QString, Platform> iter(platforms);
        while (iter.hasNext()) {
            iter.next();
            const Platform &p = iter.value();
            ProjectExplorer::GccToolChain *pToolchain = platformToolchainMap.value(p.name, 0);
            if (!pToolchain)
                continue;
            Core::Id pDeviceType;
            if (debugProbe)
                qDebug() << "guaranteeing kit for " << p.name ;
            if (p.name.startsWith(QLatin1String("iphoneos-"))) {
                pDeviceType = Core::Id(Constants::IOS_DEVICE_TYPE);
            } else if (p.name.startsWith(QLatin1String("iphonesimulator-"))) {
                pDeviceType = Core::Id(Constants::IOS_SIMULATOR_TYPE);
                if (debugProbe)
                    qDebug() << "pDeviceType " << pDeviceType.toString();
            } else {
                if (debugProbe)
                    qDebug() << "skipping non ios kit " << p.name;
                // we looked up only the ios qt build above...
                continue;
                //pDeviceType = Core::Id(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
            }
            ProjectExplorer::Abi::Architecture arch = pToolchain->targetAbi().architecture();

            QList<QtSupport::BaseQtVersion *> qtVersions = qtVersionsForArch.value(arch);
            foreach (QtSupport::BaseQtVersion *qt, qtVersions) {
                bool kitExists = false;
                for (int i = 0; i < existingKits.size(); ++i) {
                    Kit *k = existingKits.at(i);
                    if (DeviceTypeKitInformation::deviceTypeId(k) == pDeviceType
                            && ToolChainKitInformation::toolChain(k) == pToolchain
                            && SysRootKitInformation::sysRoot(k) == p.sdkPath
                            && QtSupport::QtKitInformation::qtVersion(k) == qt)
                    {
                        kitExists = true;
                        if (debugProbe)
                            qDebug() << "found existing kit " << k->displayName() << " for " << p.name
                                     << "," << qt->displayName();
                        if (i<kitMatched.size())
                            kitMatched.replace(i, true);
                        break;
                    }
                }
                if (kitExists)
                    continue;
                if (debugProbe)
                    qDebug() << "setting up new kit for " << p.name;
                Kit *newKit = new Kit;
                newKit->setAutoDetected(true);
                QString baseDisplayName = tr("%1 %2").arg(p.name, qt->displayName());
                QString displayName = baseDisplayName;
                for (int iVers = 1; iVers < 100; ++iVers) {
                    bool unique = true;
                    foreach (const ProjectExplorer::Kit *k, existingKits) {
                        if (k->displayName() == displayName) {
                            unique = false;
                            break;
                        }
                    }
                    if (unique) break;
                    displayName = baseDisplayName + QLatin1String("-") + QString::number(iVers);
                }
                newKit->setDisplayName(displayName);
                newKit->setIconPath(Utils::FileName::fromString(
                                        QLatin1String(Constants::IOS_SETTINGS_CATEGORY_ICON)));
                DeviceTypeKitInformation::setDeviceTypeId(newKit, pDeviceType);
                ToolChainKitInformation::setToolChain(newKit, pToolchain);
                QtSupport::QtKitInformation::setQtVersion(newKit, qt);
                //DeviceKitInformation::setDevice(newKit, device);

                Debugger::DebuggerItem debugger;
                debugger.setCommand(pToolchain->suggestedDebugger());
                debugger.setEngineType(Debugger::LldbEngineType);
                debugger.setDisplayName(tr("IOS Debugger"));
                debugger.setAutoDetected(true);
                debugger.setAbi(pToolchain->targetAbi());
                Debugger::DebuggerKitInformation::setDebugger(newKit, debugger);

                SysRootKitInformation::setSysRoot(newKit, p.sdkPath);
                //Qt4ProjectManager::QmakeKitInformation::setMkspec(newKit,
                //    Utils::FileName::fromString(QLatin1String("macx-ios-clang")));
                KitManager::registerKit(newKit);
                existingKits << newKit;
            }
        }
    }
    // deleting extra (old) kits
    for (int i = 0; i < kitMatched.size(); ++i) {
        if (!kitMatched.at(i) && !existingKits.at(i)->isValid()) {
            qDebug() << "deleting kit " << existingKits.at(i)->displayName();
            KitManager::deregisterKit(existingKits.at(i));
        }
    }
}

IosConfigurations *IosConfigurations::instance()
{
    IosConfigurations *m_instance = 0;
    if (m_instance == 0) {
        m_instance = new IosConfigurations(0);
        m_instance->updateSimulators();
        connect(&(m_instance->m_updateAvailableDevices),SIGNAL(timeout()),
                IosDeviceManager::instance(),SLOT(monitorAvailableDevices()));
        m_instance->m_updateAvailableDevices.setSingleShot(true);
        m_instance->m_updateAvailableDevices.start(10000);
    }
    return m_instance;
}

bool IosConfigurations::ignoreAllDevices()
{
    return instance()->m_ignoreAllDevices;
}

void IosConfigurations::setIgnoreAllDevices(bool ignoreDevices)
{
    if (ignoreDevices != instance()->m_ignoreAllDevices) {
        instance()->m_ignoreAllDevices = ignoreDevices;
        instance()->save();
        emit instance()->updated();
    }
}

FileName IosConfigurations::developerPath()
{
    return instance()->m_developerPath;
}

void IosConfigurations::save()
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(SettingsGroup);
    settings->setValue(ignoreAllDevicesKey, m_ignoreAllDevices);
    settings->endGroup();
}

IosConfigurations::IosConfigurations(QObject *parent)
    : QObject(parent)
{
    load();
}

void IosConfigurations::load()
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(SettingsGroup);
    m_ignoreAllDevices = settings->value(ignoreAllDevicesKey, false).toBool();
    settings->endGroup();
}

void IosConfigurations::updateSimulators() {
    // currently we have just one simulator
    DeviceManager *devManager = DeviceManager::instance();
    Core::Id devId(Constants::IOS_SIMULATOR_DEVICE_ID);
    QMap<QString, Platform> platforms = IosProbe::detectPlatforms();
    QMapIterator<QString, Platform> iter(platforms);
    Utils::FileName simulatorPath;
    while (iter.hasNext()) {
        iter.next();
        const Platform &p = iter.value();
        if (p.name.startsWith(QLatin1String("iphonesimulator-"))) {
            simulatorPath = p.platformPath;
            simulatorPath.appendPath(QLatin1String(
                "/Developer/Applications/iPhone Simulator.app/Contents/MacOS/iPhone Simulator"));
            if (simulatorPath.toFileInfo().exists())
                break;
        }
    }
    IDevice::ConstPtr dev = devManager->find(devId);
    if (!simulatorPath.isEmpty() && simulatorPath.toFileInfo().exists()) {
        if (!dev.isNull()) {
            if (static_cast<const IosSimulator*>(dev.data())->simulatorPath() == simulatorPath)
                return;
            devManager->removeDevice(devId);
        }
        IosSimulator *newDev = new IosSimulator(devId, simulatorPath);
        devManager->addDevice(IDevice::ConstPtr(newDev));
    } else if (!dev.isNull()) {
        devManager->removeDevice(devId);
    }
}

void IosConfigurations::setDeveloperPath(const FileName &devPath)
{
    if (devPath != instance()->m_developerPath) {
        instance()->m_developerPath = devPath;
        instance()->save();
        instance()->updateAutomaticKitList();
        emit instance()->updated();
    }
}

} // namespace Internal
} // namespace Ios
