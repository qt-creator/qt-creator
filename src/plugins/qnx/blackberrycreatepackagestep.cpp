/**************************************************************************
**
** Copyright (C) 2012 - 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
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

#include "blackberrycreatepackagestep.h"

#include "qnxconstants.h"
#include "blackberrycreatepackagestepconfigwidget.h"
#include "blackberrydeployconfiguration.h"
#include "qnxutils.h"
#include "bardescriptordocument.h"
#include "blackberryqtversion.h"
#include "blackberrydeviceconfiguration.h"
#include "blackberrydeployinformation.h"
#include "blackberrysigningpasswordsdialog.h"
#include "bardescriptordocument.h"

#include <debugger/debuggerrunconfigurationaspect.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/runconfiguration.h>
#include <qmakeprojectmanager/qmakebuildconfiguration.h>
#include <qmakeprojectmanager/qmakenodes.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/qtcassert.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>

#include <QMessageBox>

using namespace Qnx;
using namespace Qnx::Internal;

namespace {
const char PACKAGER_CMD[] = "blackberry-nativepackager";

const char PACKAGE_MODE_KEY[]      = "Qt4ProjectManager.BlackBerryCreatePackageStep.PackageMode";
const char CSK_PASSWORD_KEY[]      = "Qt4ProjectManager.BlackBerryCreatePackageStep.CskPassword";
const char KEYSTORE_PASSWORD_KEY[] = "Qt4ProjectManager.BlackBerryCreatePackageStep.KeystorePassword";
const char SAVE_PASSWORDS_KEY[]    = "Qt4ProjectManager.BlackBerryCreatePackageStep.SavePasswords";
const char BUNDLE_MODE_KEY[]       = "Qt4ProjectManager.BlackBerryCreatePackageStep.BundleMode";
const char QT_LIBRARY_PATH_KEY[]   = "Qt4ProjectManager.BlackBerryCreatePackageStep.QtLibraryPath";
}

static void prependOrSetQtEnvironment(Utils::Environment &env,
                                     const QString &key,
                                     const QString &value,
                                     bool &updated)
{
    const QString currentValue = env.value(key);
    const QString newValue = value + QLatin1String(":$") + key;
    if (!currentValue.isEmpty()) {
        if (currentValue == newValue)
            return;
        else
            env.unset(key);
    }

    env.prependOrSet(key, newValue);
    updated = true;
}

static void setQtEnvironment(Utils::Environment &env, const QString &qtPath,
                             bool &updated, bool isQt5)
{
    prependOrSetQtEnvironment(env, QLatin1String("LD_LIBRARY_PATH"),
                             QString::fromLatin1("%1/lib").arg(qtPath),
                              updated);
    prependOrSetQtEnvironment(env, QLatin1String("QML_IMPORT_PATH"),
                             QString::fromLatin1("%1/imports").arg(qtPath),
                              updated);
    prependOrSetQtEnvironment(env, QLatin1String("QT_PLUGIN_PATH"),
                             QString::fromLatin1("%1/plugins").arg(qtPath),
                              updated);
    if (isQt5) {
        prependOrSetQtEnvironment(env, QLatin1String("QML2_IMPORT_PATH"),
                                  QString::fromLatin1("%1/qml").arg(qtPath),
                                  updated);
    }
}

static bool removeQtAssets(BarDescriptorAssetList &assetList)
{
    bool assetsRemoved = false;
    foreach (const BarDescriptorAsset &a, assetList) {
        if (a.destination == QLatin1String("runtime/qt/lib") ||
                a.destination == QLatin1String("runtime/qt/plugins") ||
                a.destination == QLatin1String("runtime/qt/imports") ||
                a.destination == QLatin1String("runtime/qt/qml")) {
            assetList.removeOne(a);
            assetsRemoved = true;
        }
    }

    return assetsRemoved;
}

static bool addQtAssets(BarDescriptorAssetList &assetList, BlackBerryQtVersion *qtVersion)
{
    const bool isQt5 = qtVersion->qtVersion().majorVersion == 5;
    bool libAssetExists = false;
    bool pluginAssetExists = false;
    bool importAssetExists = false;
    bool qmlAssetExists = false;
    const QString qtInstallLibsPath =
            qtVersion->versionInfo().value(QLatin1String("QT_INSTALL_LIBS"));
    const QString qtInstallPluginPath =
            qtVersion->versionInfo().value(QLatin1String("QT_INSTALL_PLUGINS"));
    const QString qtInstallImportsPath =
            qtVersion->versionInfo().value(QLatin1String("QT_INSTALL_IMPORTS"));
    const QString qtInstallQmlPath =
            qtVersion->versionInfo().value(QLatin1String("QT_INSTALL_QML"));
    foreach (const BarDescriptorAsset &a, assetList) {
        // TODO: Also check if the asset's source is correct
        if (a.destination == QLatin1String("runtime/qt/lib")) {
            if (a.source == qtInstallLibsPath)
                libAssetExists = true;
            else
                assetList.removeOne(a);
        } else if (a.destination == QLatin1String("runtime/qt/plugins")) {
            if (a.source == qtInstallPluginPath)
                pluginAssetExists = true;
            else
                assetList.removeOne(a);
        } else if (a.destination == QLatin1String("runtime/qt/imports")) {
            if (a.source == qtInstallImportsPath)
                importAssetExists = true;
            else
                assetList.removeOne(a);
        } else if (isQt5 && a.destination == QLatin1String("runtime/qt/qml")) {
            if (a.destination == qtInstallQmlPath)
                qmlAssetExists = true;
            else
                assetList.removeOne(a);
        }
    }

    // return false if all assets already exist
    if (libAssetExists && pluginAssetExists && importAssetExists) {
        if (!isQt5 || qmlAssetExists)
            return false;
    }

    QList<QPair<QString, QString> > qtFolders;
    qtFolders.append(qMakePair(QString::fromLatin1("runtime/qt/lib"),
                               qtInstallLibsPath));
    qtFolders.append(qMakePair(QString::fromLatin1("runtime/qt/plugins"),
                               qtInstallPluginPath));
    qtFolders.append(qMakePair(QString::fromLatin1("runtime/qt/imports"),
                               qtInstallImportsPath));

    if (isQt5) {
        qtFolders.append(qMakePair(QString::fromLatin1("runtime/qt/qml"),
                                   qtInstallQmlPath));
    }

    for (QList<QPair<QString, QString> >::const_iterator it = qtFolders.constBegin();
         it != qtFolders.constEnd(); ++it) {
        const QString target = it->first;
        const QString qtFolder = it->second;
        if (QFileInfo(qtFolder).exists()) {
            BarDescriptorAsset asset;
            asset.source = qtFolder;
            asset.destination = target;
            asset.entry = false;
            assetList << asset;
        }
    }

    return true;
}

BlackBerryCreatePackageStep::BlackBerryCreatePackageStep(ProjectExplorer::BuildStepList *bsl)
    : BlackBerryAbstractDeployStep(bsl, Core::Id(Constants::QNX_CREATE_PACKAGE_BS_ID))
{
    ctor();
}

BlackBerryCreatePackageStep::BlackBerryCreatePackageStep(ProjectExplorer::BuildStepList *bsl,
                                           BlackBerryCreatePackageStep *bs)
    : BlackBerryAbstractDeployStep(bsl, bs)
{
    ctor();
}

void BlackBerryCreatePackageStep::ctor()
{
    setDisplayName(tr("Create packages"));

    m_packageMode = DevelopmentMode;
    m_bundleMode = PreInstalledQt;
    m_qtLibraryPath = QLatin1String("qt");
}

bool BlackBerryCreatePackageStep::init()
{
    if (!BlackBerryAbstractDeployStep::init())
        return false;

    const QString packageCmd = target()->activeBuildConfiguration()->environment().searchInPath(QLatin1String(PACKAGER_CMD));
    if (packageCmd.isEmpty()) {
        raiseError(tr("Could not find packager command \"%1\" in the build environment.")
                   .arg(QLatin1String(PACKAGER_CMD)));
        return false;
    }

    BlackBerryDeployConfiguration *deployConfig = qobject_cast<BlackBerryDeployConfiguration *>(deployConfiguration());
    QTC_ASSERT(deployConfig, return false);

    QList<BarPackageDeployInformation> packagesToDeploy = deployConfig->deploymentInfo()->enabledPackages();
    if (packagesToDeploy.isEmpty()) {
        raiseError(tr("No packages enabled for deployment."));
        return false;
    }

    foreach (const BarPackageDeployInformation &info, packagesToDeploy) {
        if (info.appDescriptorPath().isEmpty()) {
            raiseError(tr("BAR application descriptor file not specified. Check deployment settings."));
            return false;
        }

        if (info.packagePath().isEmpty()) {
            raiseError(tr("No package specified. Check deployment settings."));
            return false;
        }

        const QString buildDir = QFileInfo(info.packagePath()).absolutePath();
        QDir dir(buildDir);
        if (!dir.exists()) {
            if (!dir.mkpath(buildDir)) {
                raiseError(tr("Could not create build directory \"%1\".").arg(buildDir));
                return false;
            }
        }

        const QString appDescriptorPath =  info.appDescriptorPath();
        if (!doUpdateAppDescriptorFile(appDescriptorPath, PlaceHolders))
            // If there is an error, prepareAppDescriptorFile() will raise it
            return false;

        QStringList args;
        if (m_packageMode == DevelopmentMode) {
            args << QLatin1String("-devMode");
            if (!debugToken().isEmpty())
                args << QLatin1String("-debugToken") << QnxUtils::addQuotes(QDir::toNativeSeparators(debugToken()));
        } else if (m_packageMode == SigningPackageMode) {
            if (m_cskPassword.isEmpty() || m_keystorePassword.isEmpty()) {
                BlackBerrySigningPasswordsDialog dlg;
                dlg.setCskPassword(m_cskPassword);
                dlg.setStorePassword(m_keystorePassword);
                if (dlg.exec() == QDialog::Rejected) {
                    raiseError(tr("Missing passwords for signing packages."));
                    return false;
                }

                m_cskPassword = dlg.cskPassword();
                m_keystorePassword = dlg.storePassword();

                emit cskPasswordChanged(m_cskPassword);
                emit keystorePasswordChanged(m_keystorePassword);
            }
            args << QLatin1String("-sign");
            args << QLatin1String("-cskpass");
            args << m_cskPassword;
            args << QLatin1String("-storepass");
            args << m_keystorePassword;
        }
        args << QLatin1String("-package") << QnxUtils::addQuotes(QDir::toNativeSeparators(info.packagePath()));
        args << QnxUtils::addQuotes(QDir::toNativeSeparators(appDescriptorPath));

        addCommand(packageCmd, args);
    }

    return true;
}

ProjectExplorer::BuildStepConfigWidget *BlackBerryCreatePackageStep::createConfigWidget()
{
    BlackBerryCreatePackageStepConfigWidget *config = new BlackBerryCreatePackageStepConfigWidget(this);
    connect(config, SIGNAL(bundleModeChanged()), this, SLOT(updateAppDescriptorFile()));
    return config;
}

QString BlackBerryCreatePackageStep::debugToken() const
{
    BlackBerryDeviceConfiguration::ConstPtr device = BlackBerryDeviceConfiguration::device(target()->kit());
    if (!device)
        return QString();

    return device->debugToken();
}

bool BlackBerryCreatePackageStep::fromMap(const QVariantMap &map)
{
    m_packageMode = static_cast<PackageMode>(map.value(QLatin1String(PACKAGE_MODE_KEY),
                                                       DevelopmentMode).toInt());
    m_savePasswords = map.value(QLatin1String(SAVE_PASSWORDS_KEY), false).toBool();
    if (m_savePasswords) {
        m_cskPassword = map.value(QLatin1String(CSK_PASSWORD_KEY)).toString();
        m_keystorePassword = map.value(QLatin1String(KEYSTORE_PASSWORD_KEY)).toString();
    }
    m_bundleMode = static_cast<BundleMode>(map.value(QLatin1String(BUNDLE_MODE_KEY),
                                                     PreInstalledQt).toInt());
    m_qtLibraryPath = map.value(QLatin1String(QT_LIBRARY_PATH_KEY),
                                QLatin1String("qt")).toString();
    return BlackBerryAbstractDeployStep::fromMap(map);
}

QVariantMap BlackBerryCreatePackageStep::toMap() const
{
    QVariantMap map = BlackBerryAbstractDeployStep::toMap();
    map.insert(QLatin1String(PACKAGE_MODE_KEY), m_packageMode);
    map.insert(QLatin1String(SAVE_PASSWORDS_KEY), m_savePasswords);
    if (m_savePasswords) {
        map.insert(QLatin1String(CSK_PASSWORD_KEY), m_cskPassword);
        map.insert(QLatin1String(KEYSTORE_PASSWORD_KEY), m_keystorePassword);
    }
    map.insert(QLatin1String(BUNDLE_MODE_KEY), m_bundleMode);
    map.insert(QLatin1String(QT_LIBRARY_PATH_KEY), m_qtLibraryPath);
    return map;
}

BlackBerryCreatePackageStep::PackageMode BlackBerryCreatePackageStep::packageMode() const
{
    return m_packageMode;
}

QString BlackBerryCreatePackageStep::cskPassword() const
{
    return m_cskPassword;
}

QString BlackBerryCreatePackageStep::keystorePassword() const
{
    return m_keystorePassword;
}

bool BlackBerryCreatePackageStep::savePasswords() const
{
    return m_savePasswords;
}

BlackBerryCreatePackageStep::BundleMode BlackBerryCreatePackageStep::bundleMode() const
{
    return m_bundleMode;
}

QString BlackBerryCreatePackageStep::qtLibraryPath() const
{
    return m_qtLibraryPath;
}

QString BlackBerryCreatePackageStep::fullDeployedQtLibraryPath() const
{
    return QLatin1String(Constants::QNX_BLACKBERRY_DEFAULT_DEPLOY_QT_BASEPATH) + m_qtLibraryPath;
}

void BlackBerryCreatePackageStep::setPackageMode(BlackBerryCreatePackageStep::PackageMode packageMode)
{
    m_packageMode = packageMode;
}

void BlackBerryCreatePackageStep::setCskPassword(const QString &cskPassword)
{
    m_cskPassword = cskPassword;
}

void BlackBerryCreatePackageStep::setKeystorePassword(const QString &storePassword)
{
    m_keystorePassword = storePassword;
}

void BlackBerryCreatePackageStep::setSavePasswords(bool savePasswords)
{
    m_savePasswords = savePasswords;
}

void BlackBerryCreatePackageStep::setBundleMode(BlackBerryCreatePackageStep::BundleMode bundleMode)
{
    m_bundleMode = bundleMode;
}

void BlackBerryCreatePackageStep::setQtLibraryPath(const QString &qtLibraryPath)
{
    m_qtLibraryPath = qtLibraryPath;
}

void BlackBerryCreatePackageStep::updateAppDescriptorFile()
{
    BlackBerryDeployConfiguration *deployConfig = qobject_cast<BlackBerryDeployConfiguration *>(deployConfiguration());
    QTC_ASSERT(deployConfig, return);

    QList<BarPackageDeployInformation> packagesToDeploy = deployConfig->deploymentInfo()->enabledPackages();
    if (packagesToDeploy.isEmpty())
        return;

    foreach (const BarPackageDeployInformation &info, packagesToDeploy)
        doUpdateAppDescriptorFile(info.appDescriptorPath(), QtEnvironment);
}

bool BlackBerryCreatePackageStep::doUpdateAppDescriptorFile(const QString &appDescriptorPath,
                                                            QFlags<EditMode> types,
                                                            bool skipConfirmation)
{
    Core::FileChangeBlocker fb(appDescriptorPath);
    BarDescriptorDocument doc;
    QString errorString;
    if (!doc.open(&errorString, appDescriptorPath)) {
        raiseError(tr("Error opening BAR application descriptor file \"%1\" - %2")
            .arg(QDir::toNativeSeparators(appDescriptorPath))
            .arg(errorString));
        return false;
    }

    BarDescriptorAssetList assetList = doc.value(BarDescriptorDocument::asset)
            .value<BarDescriptorAssetList>();
    bool updated = false;
    if (types.testFlag(PlaceHolders)) {

        foreach (const BarDescriptorAsset &a, assetList) {
            if (a.source.contains(QLatin1String("%SRC_DIR%"))) {
                // Keep backward compatibility with older templates
                QHash<QString, QString> placeHoldersHash;
                placeHoldersHash[QLatin1String("%SRC_DIR%")] = QString();
                doc.expandPlaceHolders(placeHoldersHash);
                updated = true;
            }

            // Update the entry point source path to make use of the BUILD_DIR variable
            // if not set
            if (a.entry) {
                BarDescriptorAsset asset = a;
                if (asset.source.contains(QLatin1String("${BUILD_DIR}/")))
                    break;
                asset.source = QLatin1String("${BUILD_DIR}/") + asset.destination;
                assetList.removeOne(a);
                assetList << asset;
                updated = true;
                break;
            }
        }
    }

    if (types.testFlag(QtEnvironment)) {
        bool environmentUpdated = false;
        bool assetsUpdated = false;
        // Set up correct environment depending on using bundled/pre-installed Qt
        QList<Utils::EnvironmentItem> envItems =
                doc.value(BarDescriptorDocument::env).value<QList<Utils::EnvironmentItem> >();
        Utils::Environment env(Utils::EnvironmentItem::toStringList(envItems), Utils::OsTypeOtherUnix);
        BlackBerryQtVersion *qtVersion = dynamic_cast<BlackBerryQtVersion *>(QtSupport::QtKitInformation::qtVersion(target()->kit()));
        const bool isQt5 = qtVersion->qtVersion().majorVersion == 5;
        if (!qtVersion) {
            raiseError(tr("Error preparing BAR application descriptor file."));
            return false;
        }

        if (m_packageMode == SigningPackageMode
                || (m_packageMode == DevelopmentMode && m_bundleMode == PreInstalledQt)) {
            QtSupport::QtVersionNumber versionNumber = qtVersion->qtVersion();
            setQtEnvironment(env, QString::fromLatin1("/usr/lib/qt%1").arg(versionNumber.majorVersion),
                             environmentUpdated, isQt5);
            // remove qt assets if existing since not needed
            assetsUpdated = removeQtAssets(assetList);
        } else if (m_packageMode == DevelopmentMode && m_bundleMode == BundleQt) {
            assetsUpdated = addQtAssets(assetList, qtVersion);
            // TODO: Check for every Qt environment if the corresponding
            // assets exist for broken/internal builds(?)
            setQtEnvironment(env, QLatin1String("app/native/runtime/qt"),
                             environmentUpdated, isQt5);
        } else if (m_packageMode == DevelopmentMode && m_bundleMode == DeployedQt) {
            setQtEnvironment(env, fullDeployedQtLibraryPath(),
                             environmentUpdated, isQt5);
            // remove qt assets if existing since not needed
            assetsUpdated = removeQtAssets(assetList);
        }

        if (environmentUpdated) {
            QMessageBox::StandardButton answer = QMessageBox::Yes;
            if (!skipConfirmation) {
                QString confirmationText = tr("In order to link to the correct Qt library specified in the deployment settings "
                                              "Qt Creator needs to update the Qt environment variables "
                                              "in the BAR application file as follows:\n\n"
                                              "<env var=\"LD_LIBRARY_PATH\" value=\"%1\"/>\n"
                                              "<env var=\"QT_PLUGIN_PATH\" value=\"%2\"/>\n"
                                              "<env var=\"QML_IMPORT_PATH\" value=\"%3\"/>\n")
                        .arg(env.value(QLatin1String("LD_LIBRARY_PATH")),
                             env.value(QLatin1String("QT_PLUGIN_PATH")),
                             env.value(QLatin1String("QML_IMPORT_PATH")));

                if (isQt5)
                    confirmationText.append(QString::fromLatin1("<env var=\"QML2_IMPORT_PATH\" value=\"%1\"/>\n")
                                            .arg(env.value(QLatin1String("QML2_IMPORT_PATH"))));

                confirmationText.append(tr("\nDo you want to update it?"));
                answer = QMessageBox::question(Core::ICore::mainWindow(), tr("Confirmation"),
                                               confirmationText,
                                               QMessageBox::Yes | QMessageBox::No);
            }

            if (answer == QMessageBox::Yes) {
                QVariant envVar;
                envVar.setValue(Utils::EnvironmentItem::fromStringList(env.toStringList()));
                doc.setValue(BarDescriptorDocument::env, envVar);
                updated = true;
            }
        }

        if (assetsUpdated) {
            doc.setValue(BarDescriptorDocument::asset, QVariant::fromValue(assetList));
            updated = true;
        }
    }

    // Skip unnecessary saving
    if (!updated)
        return true;

    if (!doc.save(&errorString)) {
        raiseError(tr("Error saving BAR application descriptor file \"%1\" - %2")
                   .arg(QDir::toNativeSeparators(appDescriptorPath))
                   .arg(errorString));
        return false;
    }

    return true;
}

void BlackBerryCreatePackageStep::processStarted(const ProjectExplorer::ProcessParameters &params)
{
    if (m_packageMode == SigningPackageMode) {
        QString arguments = params.prettyArguments();

        const QString cskPasswordLine = QLatin1String(" -cskpass ") + m_cskPassword;
        const QString hiddenCskPasswordLine = QLatin1String(" -cskpass <hidden>");
        arguments.replace(cskPasswordLine, hiddenCskPasswordLine);

        const QString storePasswordLine = QLatin1String(" -storepass ") + m_keystorePassword;
        const QString hiddenStorePasswordLine = QLatin1String(" -storepass <hidden>");
        arguments.replace(storePasswordLine, hiddenStorePasswordLine);

        emitOutputInfo(params, arguments);
    } else {
        BlackBerryAbstractDeployStep::processStarted(params);
    }
}
