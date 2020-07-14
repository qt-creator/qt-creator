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

#include "qbsprofilemanager.h"

#include "defaultpropertyprovider.h"
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"
#include "qbsprojectmanagerplugin.h"
#include "qbssettings.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <qmljstools/qmljstoolsconstants.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/qtcassert.h>

#include <QCryptographicHash>
#include <QJSEngine>
#include <QProcess>
#include <QRegularExpression>
#include <QVariantMap>

namespace QbsProjectManager {

static QList<PropertyProvider *> g_propertyProviders;

PropertyProvider::PropertyProvider()
{
    g_propertyProviders.append(this);
}

PropertyProvider::~PropertyProvider()
{
    g_propertyProviders.removeOne(this);
}

namespace Internal {

static QString toJSLiteral(const bool b)
{
    return QLatin1String(b ? "true" : "false");
}

static QString toJSLiteral(const QString &str)
{
    QString js = str;
    js.replace(QRegularExpression("([\\\\\"])"), "\\\\1");
    js.prepend('"');
    js.append('"');
    return js;
}

QString toJSLiteral(const QVariant &val)
{
    if (!val.isValid())
        return QString("undefined");
    if (val.type() == QVariant::List || val.type() == QVariant::StringList) {
        QString res;
        const auto list = val.toList();
        for (const QVariant &child : list) {
            if (!res.isEmpty() ) res.append(", ");
            res.append(toJSLiteral(child));
        }
        res.prepend('[');
        res.append(']');
        return res;
    }
    if (val.type() == QVariant::Map) {
        const QVariantMap &vm = val.toMap();
        QString str("{");
        for (auto it = vm.begin(); it != vm.end(); ++it) {
            if (it != vm.begin())
                str += ',';
            str += toJSLiteral(it.key()) + ':' + toJSLiteral(it.value());
        }
        str += '}';
        return str;
    }
    if (val.type() == QVariant::Bool)
        return toJSLiteral(val.toBool());
    if (val.canConvert(QVariant::String))
        return toJSLiteral(val.toString());
    return QString::fromLatin1("Unconvertible type %1").arg(QLatin1String(val.typeName()));
}


static QbsProfileManager *m_instance = nullptr;

static QString kitNameKeyInQbsSettings(const ProjectExplorer::Kit *kit)
{
    return "preferences.qtcreator.kit." + kit->id().toString();
}

QbsProfileManager::QbsProfileManager() : m_defaultPropertyProvider(new DefaultPropertyProvider)
{
    m_instance = this;

    setObjectName(QLatin1String("QbsProjectManager"));
    connect(ProjectExplorer::KitManager::instance(), &ProjectExplorer::KitManager::kitsLoaded, this,
            [this]() { m_kitsToBeSetupForQbs = ProjectExplorer::KitManager::kits(); } );
    connect(ProjectExplorer::KitManager::instance(), &ProjectExplorer::KitManager::kitAdded, this,
            &QbsProfileManager::addProfileFromKit);
    connect(ProjectExplorer::KitManager::instance(), &ProjectExplorer::KitManager::kitUpdated, this,
            &QbsProfileManager::handleKitUpdate);
    connect(ProjectExplorer::KitManager::instance(), &ProjectExplorer::KitManager::kitRemoved, this,
            &QbsProfileManager::handleKitRemoval);
    connect(&QbsSettings::instance(), &QbsSettings::settingsChanged,
            this, &QbsProfileManager::updateAllProfiles);
}

QbsProfileManager::~QbsProfileManager()
{
    delete m_defaultPropertyProvider;
    m_instance = nullptr;
}

QbsProfileManager *QbsProfileManager::instance()
{
    return m_instance;
}

QString QbsProfileManager::ensureProfileForKit(const ProjectExplorer::Kit *k)
{
    if (!k)
        return QString();
    m_instance->updateProfileIfNecessary(k);
    return profileNameForKit(k);
}

void QbsProfileManager::setProfileForKit(const QString &name, const ProjectExplorer::Kit *k)
{
    runQbsConfig(QbsConfigOp::Set, kitNameKeyInQbsSettings(k), name);
}

void QbsProfileManager::updateProfileIfNecessary(const ProjectExplorer::Kit *kit)
{
    // kit in list <=> profile update is necessary
    // Note that the const_cast is safe, as we do not call any non-const methods on the object.
    if (m_instance->m_kitsToBeSetupForQbs.removeOne(const_cast<ProjectExplorer::Kit *>(kit)))
        m_instance->addProfileFromKit(kit);
}

void QbsProfileManager::updateAllProfiles()
{
    for (const auto * const kit : ProjectExplorer::KitManager::kits())
        addProfileFromKit(kit);
}

void QbsProfileManager::addProfile(const QString &name, const QVariantMap &data)
{
    const QString keyPrefix = "profiles." + name + ".";
    for (auto it = data.begin(); it != data.end(); ++it)
        runQbsConfig(QbsConfigOp::Set, keyPrefix + it.key(), it.value());
    emit qbsProfilesUpdated();
}

void QbsProfileManager::addQtProfileFromKit(const QString &profileName, const ProjectExplorer::Kit *k)
{
    if (const QtSupport::BaseQtVersion * const qt = QtSupport::QtKitAspect::qtVersion(k)) {
        runQbsConfig(QbsConfigOp::Set,
                    "profiles." + profileName + ".moduleProviders.Qt.qmakeFilePaths",
                    qt->qmakeCommand().toString());
    }
}

void QbsProfileManager::addProfileFromKit(const ProjectExplorer::Kit *k)
{
    const QString name = profileNameForKit(k);
    runQbsConfig(QbsConfigOp::Unset, "profiles." + name);
    setProfileForKit(name, k);
    addQtProfileFromKit(name, k);

    // set up properties:
    QVariantMap data = m_defaultPropertyProvider->properties(k, QVariantMap());
    for (PropertyProvider *provider : g_propertyProviders) {
        if (provider->canHandle(k))
            data = provider->properties(k, data);
    }

    addProfile(name, data);
}

void QbsProfileManager::handleKitUpdate(ProjectExplorer::Kit *kit)
{
    m_kitsToBeSetupForQbs.removeOne(kit);
    addProfileFromKit(kit);
}

void QbsProfileManager::handleKitRemoval(ProjectExplorer::Kit *kit)
{
    m_kitsToBeSetupForQbs.removeOne(kit);
    runQbsConfig(QbsConfigOp::Unset, kitNameKeyInQbsSettings(kit));
    runQbsConfig(QbsConfigOp::Unset, "profiles." + profileNameForKit(kit));
    emit qbsProfilesUpdated();
}

QString QbsProfileManager::profileNameForKit(const ProjectExplorer::Kit *kit)
{
    if (!kit)
        return QString();
    return QString::fromLatin1("qtc_%1_%2").arg(kit->fileSystemFriendlyName().left(8),
            QString::fromLatin1(QCryptographicHash::hash(
                                    kit->id().name(), QCryptographicHash::Sha1).toHex().left(8)));
}

QString QbsProfileManager::runQbsConfig(QbsConfigOp op, const QString &key, const QVariant &value)
{
    QProcess qbsConfig;
    QStringList args("config");
    if (QbsSettings::useCreatorSettingsDirForQbs())
        args << "--settings-dir" << QbsSettings::qbsSettingsBaseDir();
    switch (op) {
    case QbsConfigOp::Get:
        args << key;
        break;
    case QbsConfigOp::Set:
        args << key << toJSLiteral(value);
        break;
    case QbsConfigOp::Unset:
        args << "--unset" << key;
        break;
    }
    const Utils::FilePath qbsExe = QbsSettings::qbsExecutableFilePath();
    if (qbsExe.isEmpty() || !qbsExe.exists())
        return {};
    qbsConfig.start(qbsExe.toString(), args);
    if (!qbsConfig.waitForStarted(3000) || !qbsConfig.waitForFinished(5000)) {
        Core::MessageManager::write(tr("Failed run qbs config: %1").arg(qbsConfig.errorString()));
    } else if (qbsConfig.exitCode() != 0) {
        Core::MessageManager::write(tr("Failed to run qbs config: %1")
                                    .arg(QString::fromLocal8Bit(qbsConfig.readAllStandardError())));
    }
    return QString::fromLocal8Bit(qbsConfig.readAllStandardOutput()).trimmed();
}

QVariant fromJSLiteral(const QString &str)
{
    QJSEngine engine;
    QJSValue sv = engine.evaluate("(function(){return " + str + ";})()");
    return sv.isError() ? str : sv.toVariant();
}

} // namespace Internal
} // namespace QbsProjectManager
