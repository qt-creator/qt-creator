/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qt4target.h"

#include "makestep.h"
#include "profilereader.h"
#include "qmakestep.h"
#include "qt4project.h"
#include "qt4runconfiguration.h"
#include "qt4projectmanagerconstants.h"
#include "qt-maemo/maemorunconfiguration.h"
#include "qt-s60/s60devicerunconfiguration.h"
#include "qt-s60/s60emulatorrunconfiguration.h"
#include "qt-s60/s60createpackagestep.h"

#include <projectexplorer/toolchain.h>
#include <coreplugin/coreconstants.h>
#include <symbianutils/symbiandevicemanager.h>

#include <QtGui/QApplication>
#include <QtGui/QPixmap>
#include <QtGui/QPainter>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

namespace {

QString displayNameForId(const QString &id) {
    if (id == QLatin1String(Constants::DESKTOP_TARGET_ID))
        return QApplication::translate("Qt4ProjectManager::Internal::Qt4Target", "Desktop", "Qt4 Desktop target display name");
    if (id == QLatin1String(Constants::S60_EMULATOR_TARGET_ID))
        return QApplication::translate("Qt4ProjectManager::Internal::Qt4Target", "Symbian Emulator", "Qt4 Symbian Emulator target display name");
    if (id == QLatin1String(Constants::S60_DEVICE_TARGET_ID))
        return QApplication::translate("Qt4ProjectManager::Internal::Qt4Target", "Symbian Device", "Qt4 Symbian Device target display name");
    if (id == QLatin1String(Constants::MAEMO_DEVICE_TARGET_ID))
        return QApplication::translate("Qt4ProjectManager::Internal::Qt4Target", "Maemo", "Qt4 Maemo target display name");
    return QString();
}

QIcon iconForId(const QString &id) {
    if (id == QLatin1String(Constants::DESKTOP_TARGET_ID))
        return QIcon(qApp->style()->standardIcon(QStyle::SP_ComputerIcon));
    if (id == QLatin1String(Constants::S60_EMULATOR_TARGET_ID))
        return QIcon(":/projectexplorer/images/SymbianEmulator.png");
    if (id == QLatin1String(Constants::S60_DEVICE_TARGET_ID))
        return QIcon(":/projectexplorer/images/SymbianDevice.png");
    if (id == QLatin1String(Constants::MAEMO_DEVICE_TARGET_ID))
        return QIcon(":/projectexplorer/images/MaemoDevice.png");
    return QIcon();
}

} // namespace

// -------------------------------------------------------------------------
// Qt4TargetFactory
// -------------------------------------------------------------------------

Qt4TargetFactory::Qt4TargetFactory(QObject *parent) :
    ITargetFactory(parent)
{
    connect(QtVersionManager::instance(), SIGNAL(qtVersionsChanged(QList<int>)),
            this, SIGNAL(availableCreationIdsChanged()));
}

Qt4TargetFactory::~Qt4TargetFactory()
{
}

QStringList Qt4TargetFactory::availableCreationIds(ProjectExplorer::Project *parent) const
{
    if (!qobject_cast<Qt4Project *>(parent))
        return QStringList();

    return parent->possibleTargetIds().toList();
}

QString Qt4TargetFactory::displayNameForId(const QString &id) const
{
    return ::displayNameForId(id);
}

bool Qt4TargetFactory::canCreate(ProjectExplorer::Project *parent, const QString &id) const
{
    if (!qobject_cast<Qt4Project *>(parent))
        return false;

    return parent->canAddTarget(id);
}

Qt4Target *Qt4TargetFactory::create(ProjectExplorer::Project *parent, const QString &id)
{
    return create(parent, id, QList<QtVersion*>());
}

Qt4Target *Qt4TargetFactory::create(ProjectExplorer::Project *parent, const QString &id, QList<QtVersion *> versions)
{
    if (!canCreate(parent, id))
        return 0;

    Qt4Project * qt4project(static_cast<Qt4Project *>(parent));
    Qt4Target *t(new Qt4Target(qt4project, id));

    QList<QtVersion *> knownVersions(QtVersionManager::instance()->versionsForTargetId(id));
    if (knownVersions.isEmpty())
        return t;

    if (versions.isEmpty())
        versions.append(knownVersions.at(0));

    foreach (QtVersion *version, versions) {
        if (!knownVersions.contains(version))
            continue;

        bool buildAll(false);
        if (version && version->isValid() && (version->defaultBuildConfig() & QtVersion::BuildAll))
            buildAll = true;

        QString debugName;
        QString releaseName;
        if (versions.count() > 1) {
            debugName = tr("%1 Debug", "debug buildconfiguration name, %1 is Qt version").arg(version->displayName());
            releaseName = tr("%1 Release", "release buildconfiguration name, %1 is Qt version").arg(version->displayName());
        } else {
            debugName = tr("Debug", "debug buildconfiguration name (only one Qt version!)");
            releaseName = tr("Release", "release buildconfiguration name (only one Qt version!)");
        }

        if (buildAll) {
            t->addQt4BuildConfiguration(debugName, version, QtVersion::BuildAll | QtVersion::DebugBuild);
            if (id != QLatin1String(Constants::S60_EMULATOR_TARGET_ID))
                t->addQt4BuildConfiguration(releaseName, version, QtVersion::BuildAll);
        } else {
            t->addQt4BuildConfiguration(debugName, version, QtVersion::DebugBuild);
            if (id != QLatin1String(Constants::S60_EMULATOR_TARGET_ID))
                t->addQt4BuildConfiguration(releaseName, version, QtVersion::QmakeBuildConfig(0));
        }
    }

    QStringList pathes = qt4project->applicationProFilePathes();
    foreach (const QString &path, pathes)
        t->addRunConfigurationForPath(path);

    return t;
}


bool Qt4TargetFactory::canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

Qt4Target *Qt4TargetFactory::restore(ProjectExplorer::Project *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    Qt4Project * qt4project(static_cast<Qt4Project *>(parent));
    Qt4Target *t(new Qt4Target(qt4project, QLatin1String("transient ID")));
    if (t->fromMap(map))
        return t;
    delete t;
    return 0;
}

// -------------------------------------------------------------------------
// Qt4Target
// -------------------------------------------------------------------------

Qt4Target::Qt4Target(Qt4Project *parent, const QString &id) :
    ProjectExplorer::Target(parent, id),
    m_connectedPixmap(QLatin1String(":/projectexplorer/images/ConnectionOn.png")),
    m_disconnectedPixmap(QLatin1String(":/projectexplorer/images/ConnectionOff.png")),
    m_buildConfigurationFactory(new Qt4BuildConfigurationFactory(this))
{
    connect(project(), SIGNAL(supportedTargetIdsChanged()),
            this, SLOT(updateQtVersion()));

    connect(this, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
            this, SLOT(emitProFileEvaluateNeeded()));
    connect(this, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
            this, SIGNAL(environmentChanged()));
    connect(this, SIGNAL(addedRunConfiguration(ProjectExplorer::RunConfiguration*)),
            this, SLOT(onAddedRunConfiguration(ProjectExplorer::RunConfiguration*)));
    connect(this, SIGNAL(addedBuildConfiguration(ProjectExplorer::BuildConfiguration*)),
            this, SLOT(onAddedBuildConfiguration(ProjectExplorer::BuildConfiguration*)));
    connect(this, SIGNAL(activeRunConfigurationChanged(ProjectExplorer::RunConfiguration*)),
            this, SLOT(updateToolTipAndIcon()));

    setDisplayName(displayNameForId(id));
    setIcon(iconForId(id));
}

Qt4Target::~Qt4Target()
{
}

Qt4BuildConfiguration *Qt4Target::activeBuildConfiguration() const
{
    return static_cast<Qt4BuildConfiguration *>(Target::activeBuildConfiguration());
}

Qt4Project *Qt4Target::qt4Project() const
{
    return static_cast<Qt4Project *>(project());
}

Qt4BuildConfiguration *Qt4Target::addQt4BuildConfiguration(QString displayName, QtVersion *qtversion,
                                                           QtVersion::QmakeBuildConfigs qmakeBuildConfiguration,
                                                           QStringList additionalArguments)
{
    Q_ASSERT(qtversion);
    bool debug = qmakeBuildConfiguration & QtVersion::DebugBuild;

    // Add the buildconfiguration
    Qt4BuildConfiguration *bc = new Qt4BuildConfiguration(this);
    bc->setDisplayName(displayName);

    QMakeStep *qmakeStep = new QMakeStep(bc);
    bc->insertStep(ProjectExplorer::Build, 0, qmakeStep);

    MakeStep *makeStep = new MakeStep(bc);
    bc->insertStep(ProjectExplorer::Build, 1, makeStep);

    if (id() == Constants::S60_DEVICE_TARGET_ID) {
        S60CreatePackageStep *packageStep = new S60CreatePackageStep(bc);
        bc->insertStep(ProjectExplorer::Build, 2, packageStep);
    }

    MakeStep* cleanStep = new MakeStep(bc);
    cleanStep->setClean(true);
    cleanStep->setUserArguments(QStringList() << "clean");
    bc->insertStep(ProjectExplorer::Clean, 0, cleanStep);
    if (!additionalArguments.isEmpty())
        qmakeStep->setUserArguments(additionalArguments);

    // set some options for qmake and make
    if (qmakeBuildConfiguration & QtVersion::BuildAll) // debug_and_release => explicit targets
        makeStep->setUserArguments(QStringList() << (debug ? "debug" : "release"));

    bc->setQMakeBuildConfiguration(qmakeBuildConfiguration);

    // Finally set the qt version & ToolChain
    bc->setQtVersion(qtversion);
    ToolChain::ToolChainType defaultTc = preferredToolChainType(filterToolChainTypes(bc->qtVersion()->possibleToolChainTypes()));
    bc->setToolChainType(defaultTc);
    addBuildConfiguration(bc);

    return bc;
}

Qt4BuildConfigurationFactory *Qt4Target::buildConfigurationFactory() const
{
    return m_buildConfigurationFactory;
}

void Qt4Target::addRunConfigurationForPath(const QString &proFilePath)
{
    if (id() == QLatin1String(Constants::DESKTOP_TARGET_ID))
        addRunConfiguration(new Qt4RunConfiguration(this, proFilePath));
    else if (id() == QLatin1String(Constants::S60_EMULATOR_TARGET_ID))
        addRunConfiguration(new S60EmulatorRunConfiguration(this, proFilePath));
    else if (id() == QLatin1String(Constants::S60_DEVICE_TARGET_ID))
        addRunConfiguration(new S60DeviceRunConfiguration(this, proFilePath));
    else if (id() == QLatin1String(Constants::MAEMO_DEVICE_TARGET_ID))
        addRunConfiguration(new MaemoRunConfiguration(this, proFilePath));
}

QList<ToolChain::ToolChainType> Qt4Target::filterToolChainTypes(const QList<ToolChain::ToolChainType> &candidates) const
{
    QList<ToolChain::ToolChainType> tmp(candidates);
    if (id() == QLatin1String(Constants::S60_EMULATOR_TARGET_ID)) {
        if (tmp.contains(ToolChain::WINSCW))
            return QList<ToolChain::ToolChainType>() << ToolChain::WINSCW;
        else
            return QList<ToolChain::ToolChainType>();
    } else if (id() == QLatin1String(Constants::S60_DEVICE_TARGET_ID)) {
        tmp.removeAll(ToolChain::WINSCW);
        return tmp;
    }
    return tmp;
}

ToolChain::ToolChainType Qt4Target::preferredToolChainType(const QList<ToolChain::ToolChainType> &candidates) const
{
    ToolChain::ToolChainType preferredType = ToolChain::INVALID;
    if (id() == QLatin1String(Constants::S60_EMULATOR_TARGET_ID) &&
        candidates.contains(ToolChain::WINSCW))
        preferredType = ToolChain::WINSCW;
    if (!candidates.isEmpty())
        preferredType = candidates.at(0);
    return preferredType;
}

QString Qt4Target::defaultBuildDirectory() const
{
    if (id() == QLatin1String(Constants::S60_DEVICE_TARGET_ID)
        || id() == QLatin1String(Constants::S60_EMULATOR_TARGET_ID)
#if defined(Q_OS_WIN)
        || id() == QLatin1String(Constants::MAEMO_DEVICE_TARGET_ID)
#endif
        )
        return project()->projectDirectory();

    QString shortName = QLatin1String("unknown");
    if (id() == QLatin1String(Constants::DESKTOP_TARGET_ID))
        shortName = QLatin1String("desktop");
    else if (id() == QLatin1String(Constants::S60_EMULATOR_TARGET_ID))
        shortName = QLatin1String("symbian_emulator");
    else if (id() == QLatin1String(Constants::S60_DEVICE_TARGET_ID))
        shortName = QLatin1String("symbian");
    else if (id() == QLatin1String(Constants::MAEMO_DEVICE_TARGET_ID))
        shortName = QLatin1String("maemo");

    return qt4Project()->defaultTopLevelBuildDirectory() + QChar('/') + shortName;
}

bool Qt4Target::fromMap(const QVariantMap &map)
{
    if (!Target::fromMap(map))
        return false;

    setDisplayName(displayNameForId(id()));
    setIcon(iconForId(id()));

    return true;
}

void Qt4Target::updateQtVersion()
{
    setEnabled(project()->supportedTargetIds().contains(id()));
}

void Qt4Target::onAddedRunConfiguration(ProjectExplorer::RunConfiguration *rc)
{
    Q_ASSERT(rc);
    S60DeviceRunConfiguration *deviceRc(qobject_cast<S60DeviceRunConfiguration *>(rc));
    if (!deviceRc)
        return;
    connect(deviceRc, SIGNAL(serialPortNameChanged()),
            this, SLOT(slotUpdateDeviceInformation()));
}

void Qt4Target::onAddedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc)
{
    Q_ASSERT(bc);
    Qt4BuildConfiguration *qt4bc = qobject_cast<Qt4BuildConfiguration *>(bc);
    Q_ASSERT(qt4bc);
    connect(qt4bc, SIGNAL(buildDirectoryInitialized()),
            this, SIGNAL(buildDirectoryInitialized()));
    connect(qt4bc, SIGNAL(proFileEvaluateNeeded(Qt4ProjectManager::Internal::Qt4BuildConfiguration *)),
            this, SLOT(onProFileEvaluateNeeded(Qt4ProjectManager::Internal::Qt4BuildConfiguration *)));
}

void Qt4Target::slotUpdateDeviceInformation()
{
    S60DeviceRunConfiguration *deviceRc(qobject_cast<S60DeviceRunConfiguration *>(sender()));
    if (deviceRc && deviceRc == activeRunConfiguration()) {
        updateToolTipAndIcon();
    }
}

void Qt4Target::onProFileEvaluateNeeded(Qt4ProjectManager::Internal::Qt4BuildConfiguration *bc)
{
    if (bc && bc == activeBuildConfiguration())
        emit proFileEvaluateNeeded(this);
}

void Qt4Target::emitProFileEvaluateNeeded()
{
    emit proFileEvaluateNeeded(this);
}

void Qt4Target::updateToolTipAndIcon()
{
    static const int TARGET_OVERLAY_ORIGINAL_SIZE = 32;
    if (const S60DeviceRunConfiguration *s60DeviceRc = qobject_cast<S60DeviceRunConfiguration *>(activeRunConfiguration()))  {
        const SymbianUtils::SymbianDeviceManager *sdm = SymbianUtils::SymbianDeviceManager::instance();
        const int deviceIndex = sdm->findByPortName(s60DeviceRc->serialPortName());
        QPixmap overlay;
        if (deviceIndex == -1) {
            setToolTip(tr("<b>Device:</b> Not connected"));
            overlay = m_disconnectedPixmap;
        } else {
            // device connected
            const SymbianUtils::SymbianDevice device = sdm->devices().at(deviceIndex);
            const QString tooltip = device.additionalInformation().isEmpty() ?
                                    tr("<b>Device:</b> %1").arg(device.friendlyName()) :
                                    tr("<b>Device:</b> %1, %2").arg(device.friendlyName(), device.additionalInformation());
            setToolTip(tooltip);
            overlay = m_connectedPixmap;
        }
        double factor = Core::Constants::TARGET_ICON_SIZE / (double)TARGET_OVERLAY_ORIGINAL_SIZE;
        QSize overlaySize(overlay.size().width()*factor, overlay.size().height()*factor);
        QPixmap pixmap(Core::Constants::TARGET_ICON_SIZE, Core::Constants::TARGET_ICON_SIZE);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.drawPixmap(Core::Constants::TARGET_ICON_SIZE - overlaySize.width(),
                           Core::Constants::TARGET_ICON_SIZE - overlaySize.height(),
                           overlay.scaled(overlaySize));

        setOverlayIcon(QIcon(pixmap));
    } else {
        setToolTip(QString());
        setOverlayIcon(QIcon());
    }
}
