// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qbsprofilemanager.h"

#include "defaultpropertyprovider.h"
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"
#include "qbsprojectmanagerplugin.h"
#include "qbsprojectmanagertr.h"
#include "qbssettings.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <qmljstools/qmljstoolsconstants.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QCryptographicHash>
#include <QJSEngine>
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
    if (val.typeId() == QVariant::List || val.typeId() == QVariant::StringList) {
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
    if (val.typeId() == QVariant::Map) {
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
    if (val.typeId() == QVariant::Bool)
        return toJSLiteral(val.toBool());
    if (val.canConvert(QVariant::String))
        return toJSLiteral(val.toString());
    return QString::fromLatin1("Unconvertible type %1").arg(QLatin1String(val.typeName()));
}

static PropertyProvider &defaultPropertyProvider()
{
    static DefaultPropertyProvider theDefaultPropertyProvider;
    return theDefaultPropertyProvider;
}

static QString kitNameKeyInQbsSettings(const ProjectExplorer::Kit *kit)
{
    return "preferences.qtcreator.kit." + kit->id().toString();
}

QbsProfileManager::QbsProfileManager()
{
    setObjectName(QLatin1String("QbsProjectManager"));

    if (ProjectExplorer::KitManager::instance()->isLoaded()) {
        m_kitsToBeSetupForQbs = ProjectExplorer::KitManager::kits();
    } else {
        connect(ProjectExplorer::KitManager::instance(), &ProjectExplorer::KitManager::kitsLoaded,
                this, [this] { m_kitsToBeSetupForQbs = ProjectExplorer::KitManager::kits(); } );
    }
    connect(ProjectExplorer::KitManager::instance(), &ProjectExplorer::KitManager::kitAdded, this,
            &QbsProfileManager::addProfileFromKit);
    connect(ProjectExplorer::KitManager::instance(), &ProjectExplorer::KitManager::kitUpdated, this,
            &QbsProfileManager::handleKitUpdate);
    connect(ProjectExplorer::KitManager::instance(), &ProjectExplorer::KitManager::kitRemoved, this,
            &QbsProfileManager::handleKitRemoval);
    connect(&QbsSettings::instance(), &QbsSettings::settingsChanged,
            this, &QbsProfileManager::updateAllProfiles);
}

QbsProfileManager::~QbsProfileManager() = default;

QbsProfileManager *QbsProfileManager::instance()
{
    static QbsProfileManager theQbsProfileManager;
    return &theQbsProfileManager;
}

QString QbsProfileManager::ensureProfileForKit(const ProjectExplorer::Kit *k)
{
    if (!k)
        return QString();
    updateProfileIfNecessary(k);
    return profileNameForKit(k);
}

void QbsProfileManager::updateProfileIfNecessary(const ProjectExplorer::Kit *kit)
{
    // kit in list <=> profile update is necessary
    // Note that the const_cast is safe, as we do not call any non-const methods on the object.
    if (instance()->m_kitsToBeSetupForQbs.removeOne(const_cast<ProjectExplorer::Kit *>(kit)))
        instance()->addProfileFromKit(kit);
}

void QbsProfileManager::updateAllProfiles()
{
    for (const auto * const kit : ProjectExplorer::KitManager::kits())
        addProfileFromKit(kit);
}

void QbsProfileManager::addProfileFromKit(const ProjectExplorer::Kit *k)
{
    const QString name = profileNameForKit(k);
    runQbsConfig(QbsConfigOp::Unset, "profiles." + name);
    runQbsConfig(QbsConfigOp::Set, kitNameKeyInQbsSettings(k), name);

    // set up properties:
    QVariantMap data = defaultPropertyProvider().properties(k, QVariantMap());
    for (PropertyProvider *provider : std::as_const(g_propertyProviders)) {
        if (provider->canHandle(k))
            data = provider->properties(k, data);
    }
    if (const QtSupport::QtVersion * const qt = QtSupport::QtKitAspect::qtVersion(k))
        data.insert("moduleProviders.Qt.qmakeFilePaths", qt->qmakeFilePath().toString());

    if (QbsSettings::qbsVersion() < QVersionNumber({1, 20})) {
        const QString keyPrefix = "profiles." + name + ".";
        for (auto it = data.begin(); it != data.end(); ++it)
            runQbsConfig(QbsConfigOp::Set, keyPrefix + it.key(), it.value());
    } else {
        runQbsConfig(QbsConfigOp::AddProfile, name, data);
    }
    emit qbsProfilesUpdated();
}

void QbsProfileManager::handleKitUpdate(ProjectExplorer::Kit *kit)
{
    if (!m_kitsToBeSetupForQbs.contains(kit))
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
    QStringList args;
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
    case QbsConfigOp::AddProfile: {
        args << "--add-profile" << key;
        const QVariantMap props = value.toMap();
        for (auto it = props.begin(); it != props.end(); ++it)
            args << it.key() << toJSLiteral(it.value());
        if (props.isEmpty()) // Make sure we still create a profile for "empty" kits.
            args << "qbs.optimization" << toJSLiteral(QString("none"));
        break;
    }
    }
    const Utils::FilePath qbsConfigExe = QbsSettings::qbsConfigFilePath();
    if (qbsConfigExe.isEmpty() || !qbsConfigExe.exists())
        return {};
    Utils::Process qbsConfig;
    qbsConfig.setCommand({qbsConfigExe, args});
    qbsConfig.start();
    if (!qbsConfig.waitForFinished(5000)) {
        Core::MessageManager::writeFlashing(
            Tr::tr("Failed to run qbs config: %1").arg(qbsConfig.errorString()));
    } else if (qbsConfig.exitCode() != 0) {
        Core::MessageManager::writeFlashing(
            Tr::tr("Failed to run qbs config: %1")
                .arg(QString::fromLocal8Bit(qbsConfig.readAllRawStandardError())));
    }
    return QString::fromLocal8Bit(qbsConfig.readAllRawStandardOutput()).trimmed();
}

QVariant fromJSLiteral(const QString &str)
{
    QJSEngine engine;
    QJSValue sv = engine.evaluate("(function(){return " + str + ";})()");
    return sv.isError() ? str : sv.toVariant();
}

} // namespace Internal
} // namespace QbsProjectManager
