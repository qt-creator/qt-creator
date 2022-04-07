/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "kitdetector.h"

#include "dockerconstants.h"

#include <cmakeprojectmanager/cmakeprojectconstants.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionfactory.h>
#include <qtsupport/qtversionmanager.h>

#include <utils/filepath.h>
#include <utils/qtcassert.h>
#include <utils/algorithm.h>

#include <QApplication>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;

namespace Docker {
namespace Internal {

class KitDetectorPrivate
{
    Q_DECLARE_TR_FUNCTIONS(ProjectExplorer::KitItemDetector)

public:
    KitDetectorPrivate(KitDetector *parent, const IDevice::ConstPtr &device)
        : q(parent)
        , m_device(device)
    {}

    void autoDetect();
    void undoAutoDetect() const;
    void listAutoDetected() const;

    void setSharedId(const QString &sharedId) { m_sharedId = sharedId; }
    void setSearchPaths(const FilePaths &searchPaths) { m_searchPaths = searchPaths; }

private:
    QtVersions autoDetectQtVersions() const;
    QList<ToolChain *> autoDetectToolChains();
    QList<Id> autoDetectCMake();
    void autoDetectDebugger();

    KitDetector *q;
    IDevice::ConstPtr m_device;
    QString m_sharedId;
    FilePaths m_searchPaths;
};

KitDetector::KitDetector(const IDevice::ConstPtr &device)
    : d(new KitDetectorPrivate(this, device))
{}

KitDetector::~KitDetector()
{
    delete d;
}

void KitDetector::autoDetect(const QString &sharedId, const FilePaths &searchPaths) const
{
    d->setSharedId(sharedId);
    d->setSearchPaths(searchPaths);
    d->autoDetect();
}

void KitDetector::undoAutoDetect(const QString &sharedId) const
{
    d->setSharedId(sharedId);
    d->undoAutoDetect();
}

void KitDetector::listAutoDetected(const QString &sharedId) const
{
    d->setSharedId(sharedId);
    d->listAutoDetected();
}

void KitDetectorPrivate::undoAutoDetect() const
{
    emit q->logOutput(tr("Start removing auto-detected items associated with this docker image."));

    emit q->logOutput('\n' + tr("Removing kits..."));
    for (Kit *kit : KitManager::kits()) {
        if (kit->autoDetectionSource() == m_sharedId) {
            emit q->logOutput(tr("Removed \"%1\"").arg(kit->displayName()));
            KitManager::deregisterKit(kit);
        }
    };

    emit q->logOutput('\n' + tr("Removing Qt version entries..."));
    for (QtVersion *qtVersion : QtVersionManager::versions()) {
        if (qtVersion->detectionSource() == m_sharedId) {
            emit q->logOutput(tr("Removed \"%1\"").arg(qtVersion->displayName()));
            QtVersionManager::removeVersion(qtVersion);
        }
    };

    emit q->logOutput('\n' + tr("Removing toolchain entries..."));
    const Toolchains toolchains = ToolChainManager::toolchains();
    for (ToolChain *toolChain : toolchains) {
        if (toolChain && toolChain->detectionSource() == m_sharedId) {
            emit q->logOutput(tr("Removed \"%1\"").arg(toolChain->displayName()));
            ToolChainManager::deregisterToolChain(toolChain);
        }
    };

    if (auto cmakeManager = ExtensionSystem::PluginManager::getObjectByName("CMakeToolManager")) {
        QString logMessage;
        const bool res = QMetaObject::invokeMethod(cmakeManager,
                                                   "removeDetectedCMake",
                                                   Q_ARG(QString, m_sharedId),
                                                   Q_ARG(QString *, &logMessage));
        QTC_CHECK(res);
        emit q->logOutput('\n' + logMessage);
    }

    if (auto debuggerPlugin = ExtensionSystem::PluginManager::getObjectByName("DebuggerPlugin")) {
        QString logMessage;
        const bool res = QMetaObject::invokeMethod(debuggerPlugin,
                                                   "removeDetectedDebuggers",
                                                   Q_ARG(QString, m_sharedId),
                                                   Q_ARG(QString *, &logMessage));
        QTC_CHECK(res);
        emit q->logOutput('\n' + logMessage);
    }

    emit q->logOutput('\n' + tr("Removal of previously auto-detected kit items finished.") + "\n\n");
}

void KitDetectorPrivate::listAutoDetected() const
{
    emit q->logOutput(tr("Start listing auto-detected items associated with this docker image."));

    emit q->logOutput('\n' + tr("Kits:"));
    for (Kit *kit : KitManager::kits()) {
        if (kit->autoDetectionSource() == m_sharedId)
            emit q->logOutput(kit->displayName());
    }

    emit q->logOutput('\n' + tr("Qt versions:"));
    for (QtVersion *qtVersion : QtVersionManager::versions()) {
        if (qtVersion->detectionSource() == m_sharedId)
            emit q->logOutput(qtVersion->displayName());
    }

    emit q->logOutput('\n' + tr("Toolchains:"));
    for (ToolChain *toolChain : ToolChainManager::toolchains()) {
        if (toolChain->detectionSource() == m_sharedId)
            emit q->logOutput(toolChain->displayName());
    }

    if (QObject *cmakeManager = ExtensionSystem::PluginManager::getObjectByName(
            "CMakeToolManager")) {
        QString logMessage;
        const bool res = QMetaObject::invokeMethod(cmakeManager,
                                                   "listDetectedCMake",
                                                   Q_ARG(QString, m_sharedId),
                                                   Q_ARG(QString *, &logMessage));
        QTC_CHECK(res);
        emit q->logOutput('\n' + logMessage);
    }

    if (QObject *debuggerPlugin = ExtensionSystem::PluginManager::getObjectByName(
            "DebuggerPlugin")) {
        QString logMessage;
        const bool res = QMetaObject::invokeMethod(debuggerPlugin,
                                                   "listDetectedDebuggers",
                                                   Q_ARG(QString, m_sharedId),
                                                   Q_ARG(QString *, &logMessage));
        QTC_CHECK(res);
        emit q->logOutput('\n' + logMessage);
    }

    emit q->logOutput('\n' + tr("Listing of previously auto-detected kit items finished.") + "\n\n");
}

QtVersions KitDetectorPrivate::autoDetectQtVersions() const
{
    QtVersions qtVersions;

    QString error;

    const auto handleQmake = [this, &qtVersions, &error](const FilePath &qmake) {
        if (QtVersion *qtVersion = QtVersionFactory::createQtVersionFromQMakePath(qmake,
                                                                            false,
                                                                            m_sharedId,
                                                                            &error)) {
            if (qtVersion->isValid()) {
                if (!Utils::anyOf(qtVersions,
                                 [qtVersion](QtVersion* other) {
                                     return qtVersion->mkspecPath() == other->mkspecPath();
                                 })) {

                    qtVersions.append(qtVersion);
                    QtVersionManager::addVersion(qtVersion);
                    emit q->logOutput(
                        tr("Found \"%1\"").arg(qtVersion->qmakeFilePath().toUserOutput()));
                }
            }
        }
        return true;
    };

    emit q->logOutput(tr("Searching for qmake executables..."));

    const QStringList candidates = {"qmake-qt6", "qmake-qt5", "qmake"};
    for (const FilePath &searchPath : m_searchPaths) {
        searchPath.iterateDirectory(handleQmake,
                                    {candidates,
                                     QDir::Files | QDir::Executable,
                                     QDirIterator::Subdirectories});
    }

    if (!error.isEmpty())
        emit q->logOutput(tr("Error: %1.").arg(error));
    if (qtVersions.isEmpty())
        emit q->logOutput(tr("No Qt installation found."));
    return qtVersions;
}

Toolchains KitDetectorPrivate::autoDetectToolChains()
{
    const QList<ToolChainFactory *> factories = ToolChainFactory::allToolChainFactories();

    Toolchains alreadyKnown = ToolChainManager::toolchains();
    Toolchains allNewToolChains;
    QApplication::processEvents();
    emit q->logOutput('\n' + tr("Searching toolchains..."));
    for (ToolChainFactory *factory : factories) {
        emit q->logOutput(tr("Searching toolchains of type %1").arg(factory->displayName()));
        const ToolchainDetector detector(alreadyKnown, m_device, m_searchPaths);
        const Toolchains newToolChains = factory->autoDetect(detector);
        for (ToolChain *toolChain : newToolChains) {
            emit q->logOutput(tr("Found \"%1\"").arg(toolChain->compilerCommand().toUserOutput()));
            toolChain->setDetectionSource(m_sharedId);
            ToolChainManager::registerToolChain(toolChain);
            alreadyKnown.append(toolChain);
        }
        allNewToolChains.append(newToolChains);
    }
    emit q->logOutput(tr("%1 new toolchains found.").arg(allNewToolChains.size()));

    return allNewToolChains;
}

QList<Id> KitDetectorPrivate::autoDetectCMake()
{
    QList<Id> result;
    QObject *cmakeManager = ExtensionSystem::PluginManager::getObjectByName("CMakeToolManager");
    if (!cmakeManager)
        return {};

    QString logMessage;
    const bool res = QMetaObject::invokeMethod(cmakeManager,
                                               "autoDetectCMakeForDevice",
                                               Q_RETURN_ARG(QList<Utils::Id>, result),
                                               Q_ARG(Utils::FilePaths, m_searchPaths),
                                               Q_ARG(QString, m_sharedId),
                                               Q_ARG(QString *, &logMessage));
    QTC_CHECK(res);
    emit q->logOutput('\n' + logMessage);

    return result;
}

void KitDetectorPrivate::autoDetectDebugger()
{
    QObject *debuggerPlugin = ExtensionSystem::PluginManager::getObjectByName("DebuggerPlugin");
    if (!debuggerPlugin)
        return;

    QString logMessage;
    const bool res = QMetaObject::invokeMethod(debuggerPlugin,
                                               "autoDetectDebuggersForDevice",
                                               Q_ARG(Utils::FilePaths, m_searchPaths),
                                               Q_ARG(QString, m_sharedId),
                                               Q_ARG(QString *, &logMessage));
    QTC_CHECK(res);
    emit q->logOutput('\n' + logMessage);
}

void KitDetectorPrivate::autoDetect()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);

    undoAutoDetect();

    emit q->logOutput(tr("Starting auto-detection. This will take a while..."));

    const Toolchains toolchains = autoDetectToolChains();
    const QtVersions qtVersions = autoDetectQtVersions();

    const QList<Id> cmakeIds = autoDetectCMake();
    const Id cmakeId = cmakeIds.empty() ? Id() : cmakeIds.first();
    autoDetectDebugger();

    const auto initializeKit = [this, toolchains, qtVersions, cmakeId](Kit *k) {
        k->setAutoDetected(false);
        k->setAutoDetectionSource(m_sharedId);
        k->setUnexpandedDisplayName("%{Device:Name}");

        if (cmakeId.isValid())
            k->setValue(CMakeProjectManager::Constants::TOOL_ID, cmakeId.toSetting());

        DeviceTypeKitAspect::setDeviceTypeId(k, Constants::DOCKER_DEVICE_TYPE);
        DeviceKitAspect::setDevice(k, m_device);
        BuildDeviceKitAspect::setDevice(k, m_device);

        QtVersion *qt = nullptr;
        if (!qtVersions.isEmpty()) {
            qt = qtVersions.at(0);
            QtSupport::QtKitAspect::setQtVersion(k, qt);
        }
        Toolchains toolchainsToSet;
        toolchainsToSet = ToolChainManager::toolchains([qt, this](const ToolChain *tc) {
            return tc->detectionSource() == m_sharedId
                   && (!qt || qt->qtAbis().contains(tc->targetAbi()));
        });
        for (ToolChain *toolChain : toolchainsToSet)
            ToolChainKitAspect::setToolChain(k, toolChain);

        if (cmakeId.isValid())
            k->setSticky(CMakeProjectManager::Constants::TOOL_ID, true);

        k->setSticky(ToolChainKitAspect::id(), true);
        k->setSticky(QtSupport::QtKitAspect::id(), true);
        k->setSticky(DeviceKitAspect::id(), true);
        k->setSticky(DeviceTypeKitAspect::id(), true);
        k->setSticky(BuildDeviceKitAspect::id(), true);
    };

    Kit *kit = KitManager::registerKit(initializeKit);
    emit q->logOutput('\n' + tr("Registered kit %1").arg(kit->displayName()));

    QApplication::restoreOverrideCursor();
}

} // namespace Internal
} // namespace Docker
