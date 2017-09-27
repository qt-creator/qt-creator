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

#include "qbsprojectmanager.h"

#include "defaultpropertyprovider.h"
#include "qbslogsink.h"
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"
#include "qbsprojectmanagerplugin.h"
#include "qbsprojectmanagersettings.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <qmljstools/qmljstoolsconstants.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>

#include <QCryptographicHash>
#include <QVariantMap>

#include <qbs.h>
#include <qtprofilesetup.h>

const QChar sep = QLatin1Char('.');

static QString qtcProfileGroup() { return QLatin1String("preferences.qtcreator.kit"); }
static QString qtcProfilePrefix() { return qtcProfileGroup() + sep; }

namespace QbsProjectManager {
namespace Internal {

qbs::Settings *QbsManager::m_settings = nullptr;
Internal::QbsLogSink *QbsManager::m_logSink = nullptr;
QbsManager *QbsManager::m_instance = nullptr;

QbsManager::QbsManager() : m_defaultPropertyProvider(new DefaultPropertyProvider)
{
    m_instance = this;

    setObjectName(QLatin1String("QbsProjectManager"));
    connect(ProjectExplorer::KitManager::instance(), &ProjectExplorer::KitManager::kitsLoaded, this,
            [this]() { m_kitsToBeSetupForQbs = ProjectExplorer::KitManager::kits(); } );
    connect(ProjectExplorer::KitManager::instance(), &ProjectExplorer::KitManager::kitAdded, this,
            &QbsManager::addProfileFromKit);
    connect(ProjectExplorer::KitManager::instance(), &ProjectExplorer::KitManager::kitUpdated, this,
            &QbsManager::handleKitUpdate);
    connect(ProjectExplorer::KitManager::instance(), &ProjectExplorer::KitManager::kitRemoved, this,
            &QbsManager::handleKitRemoval);
    connect(&QbsProjectManagerSettings::instance(), &QbsProjectManagerSettings::settingsBaseChanged,
            this, &QbsManager::updateAllProfiles);

    m_logSink = new QbsLogSink(this);
    int level = qbs::LoggerInfo;
    const QString levelEnv = QString::fromLocal8Bit(qgetenv("QBS_LOG_LEVEL"));
    if (!levelEnv.isEmpty()) {
        bool ok = false;
        int tmp = levelEnv.toInt(&ok);
        if (ok) {
            if (tmp < static_cast<int>(qbs::LoggerMinLevel))
                tmp = static_cast<int>(qbs::LoggerMinLevel);
            if (tmp > static_cast<int>(qbs::LoggerMaxLevel))
                tmp = static_cast<int>(qbs::LoggerMaxLevel);
            level = tmp;
        }
    }
    m_logSink->setLogLevel(static_cast<qbs::LoggerLevel>(level));
}

QbsManager::~QbsManager()
{
    delete m_defaultPropertyProvider;
    delete m_settings;
    m_instance = nullptr;
}

QString QbsManager::profileForKit(const ProjectExplorer::Kit *k)
{
    if (!k)
        return QString();
    m_instance->updateProfileIfNecessary(k);
    return settings()->value(qtcProfilePrefix() + k->id().toString()).toString();
}

void QbsManager::setProfileForKit(const QString &name, const ProjectExplorer::Kit *k)
{
    settings()->setValue(qtcProfilePrefix() + k->id().toString(), name);
}

void QbsManager::updateProfileIfNecessary(const ProjectExplorer::Kit *kit)
{
    // kit in list <=> profile update is necessary
    // Note that the const_cast is safe, as we do not call any non-const methods on the object.
    if (m_kitsToBeSetupForQbs.removeOne(const_cast<ProjectExplorer::Kit *>(kit)))
        addProfileFromKit(kit);
}

void QbsManager::updateAllProfiles()
{
    foreach (const auto * const kit, ProjectExplorer::KitManager::kits())
        addProfileFromKit(kit);
}

qbs::Settings *QbsManager::settings()
{
    if (!m_settings
            || m_settings->baseDirectory() != QbsProjectManagerSettings::qbsSettingsBaseDir()) {
        delete m_settings;
        m_settings = new qbs::Settings(QbsProjectManagerSettings::qbsSettingsBaseDir());
    }
    return m_settings;
}

void QbsManager::addProfile(const QString &name, const QVariantMap &data)
{
    qbs::Profile profile(name, settings());
    const QVariantMap::ConstIterator cend = data.constEnd();
    for (QVariantMap::ConstIterator it = data.constBegin(); it != cend; ++it)
        profile.setValue(it.key(), it.value());
}

void QbsManager::addQtProfileFromKit(const QString &profileName, const ProjectExplorer::Kit *k)
{
    const QtSupport::BaseQtVersion * const qt = QtSupport::QtKitInformation::qtVersion(k);
    if (!qt)
        return;

    qbs::QtEnvironment qtEnv;
    const QList<ProjectExplorer::Abi> abi = qt->qtAbis();
    if (!abi.empty()) {
        qtEnv.architecture = ProjectExplorer::Abi::toString(abi.first().architecture());
        if (abi.first().wordWidth() == 64)
            qtEnv.architecture.append(QLatin1String("_64"));
    }
    qtEnv.binaryPath = qt->binPath().toString();
    qtEnv.documentationPath = qt->docsPath().toString();
    qtEnv.includePath = qt->headerPath().toString();
    qtEnv.libraryPath = qt->libraryPath().toString();
    qtEnv.pluginPath = qt->pluginPath().toString();
    qtEnv.mkspecBasePath = qt->mkspecsPath().toString();
    qtEnv.mkspecName = qt->mkspec().toString();
    qtEnv.mkspecPath = qt->mkspecPath().toString();
    qtEnv.qtNameSpace = qt->qtNamespace();
    qtEnv.qtLibInfix = qt->qtLibInfix();
    qtEnv.qtVersion = qt->qtVersionString();
    qtEnv.qtMajorVersion = qt->qtVersion().majorVersion;
    qtEnv.qtMinorVersion = qt->qtVersion().minorVersion;
    qtEnv.qtPatchVersion = qt->qtVersion().patchVersion;
    qtEnv.frameworkBuild = qt->isFrameworkBuild();
    qtEnv.configItems = qt->configValues();
    qtEnv.qtConfigItems = qt->qtConfigValues();
    foreach (const QString &buildVariant,
            QStringList() << QLatin1String("debug") << QLatin1String("release")) {
        if (qtEnv.qtConfigItems.contains(buildVariant))
            qtEnv.buildVariant << buildVariant;
    }
    qtEnv.qmlPath = qt->qmlPath().toString();
    qtEnv.qmlImportPath = qt->qmakeProperty("QT_INSTALL_IMPORTS");
    const qbs::ErrorInfo errorInfo = qbs::setupQtProfile(profileName, settings(), qtEnv);
    if (errorInfo.hasError()) {
        Core::MessageManager::write(tr("Failed to set up kit for Qbs: %1")
                .arg(errorInfo.toString()), Core::MessageManager::ModeSwitch);
    }
}

void QbsManager::addProfileFromKit(const ProjectExplorer::Kit *k)
{
    const QString name = QString::fromLatin1("qtc_%1_%2").arg(k->fileSystemFriendlyName().left(8),
            QString::fromLatin1(QCryptographicHash::hash(k->id().name(),
                                                         QCryptographicHash::Sha1).toHex().left(8)));
    qbs::Profile(name, settings()).removeProfile();
    setProfileForKit(name, k);
    addQtProfileFromKit(name, k);

    // set up properties:
    QVariantMap data = m_defaultPropertyProvider->properties(k, QVariantMap());
    QList<PropertyProvider *> providerList = ExtensionSystem::PluginManager::getObjects<PropertyProvider>();
    foreach (PropertyProvider *provider, providerList) {
        if (provider->canHandle(k))
            data = provider->properties(k, data);
    }

    addProfile(name, data);
}

void QbsManager::handleKitUpdate(ProjectExplorer::Kit *kit)
{
    m_kitsToBeSetupForQbs.removeOne(kit);
    addProfileFromKit(kit);
}

void QbsManager::handleKitRemoval(ProjectExplorer::Kit *kit)
{
    m_kitsToBeSetupForQbs.removeOne(kit);
    const QString key = qtcProfilePrefix() + kit->id().toString();
    const QString profileName = settings()->value(key).toString();
    settings()->remove(key);
    qbs::Profile(profileName, settings()).removeProfile();
}

} // namespace Internal
} // namespace QbsProjectManager
