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
        raiseError(tr("Could not find packager command '%1' in the build environment.")
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
                raiseError(tr("Could not create build directory '%1'.").arg(buildDir));
                return false;
            }
        }

        const QString preparedFilePath =  buildDir + QLatin1String("/bar-descriptor-") + project()->displayName() + QLatin1String("-qtc-generated.xml");
        if (!prepareAppDescriptorFile(info.appDescriptorPath(), preparedFilePath))
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
        args << QnxUtils::addQuotes(QDir::toNativeSeparators(preparedFilePath));

        addCommand(packageCmd, args);
    }

    return true;
}

ProjectExplorer::BuildStepConfigWidget *BlackBerryCreatePackageStep::createConfigWidget()
{
    return new BlackBerryCreatePackageStepConfigWidget(this);
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

QString BlackBerryCreatePackageStep::fullQtLibraryPath() const
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

bool BlackBerryCreatePackageStep::prepareAppDescriptorFile(const QString &appDescriptorPath, const QString &preparedFilePath)
{
    BlackBerryQtVersion *qtVersion = dynamic_cast<BlackBerryQtVersion *>(QtSupport::QtKitInformation::qtVersion(target()->kit()));
    if (!qtVersion) {
        raiseError(tr("Error preparing BAR application descriptor file."));
        return false;
    }

    BarDescriptorDocument doc;
    QString errorString;
    if (!doc.open(&errorString, appDescriptorPath)) {
        raiseError(tr("Error opening BAR application descriptor file '%1' - %2")
            .arg(QDir::toNativeSeparators(appDescriptorPath))
            .arg(errorString));
        return false;
    }
    // Add Warning text
    const QString warningText = QString::fromLatin1("This file is autogenerated,"
                " any changes will get overwritten if deploying with Qt Creator");
    doc.setBannerComment(warningText);

    //Replace Source path placeholder
    QHash<QString, QString> placeHoldersHash;
    placeHoldersHash[QLatin1String("%SRC_DIR%")] =
        QDir::toNativeSeparators(target()->project()->projectDirectory());
    doc.expandPlaceHolders(placeHoldersHash);

    // Add parameter for QML debugging (if enabled)
    Debugger::DebuggerRunConfigurationAspect *aspect
            = target()->activeRunConfiguration()->extraAspect<Debugger::DebuggerRunConfigurationAspect>();
    if (aspect->useQmlDebugger()) {
        const QString qmlDebuggerArg = QString::fromLatin1("-qmljsdebugger=port:%1")
                .arg(aspect->qmlDebugServerPort());

        QStringList args = doc.value(BarDescriptorDocument::arg).toStringList();
        if (!args.contains(qmlDebuggerArg))
            args.append(qmlDebuggerArg);

        doc.setValue(BarDescriptorDocument::arg, args);
    }

    // Set up correct environment depending on using bundled/pre-installed Qt
    QList<Utils::EnvironmentItem> envItems =
            doc.value(BarDescriptorDocument::env).value<QList<Utils::EnvironmentItem> >();
    Utils::Environment env(Utils::EnvironmentItem::toStringList(envItems), Utils::OsTypeOtherUnix);
    BarDescriptorAssetList assetList = doc.value(BarDescriptorDocument::asset)
            .value<BarDescriptorAssetList>();

    if (m_packageMode == SigningPackageMode
            || (m_packageMode == DevelopmentMode && m_bundleMode == PreInstalledQt)) {
        QtSupport::QtVersionNumber versionNumber = qtVersion->qtVersion();
        env.appendOrSet(QLatin1String("QML_IMPORT_PATH"),
                        QString::fromLatin1("/usr/lib/qt%1/imports").arg(versionNumber.majorVersion),
                        QLatin1String(":"));
        env.appendOrSet(QLatin1String("QT_PLUGIN_PATH"),
                        QString::fromLatin1("/usr/lib/qt%1/plugins").arg(versionNumber.majorVersion),
                        QLatin1String(":"));
        env.prependOrSetLibrarySearchPath(QString::fromLatin1("/usr/lib/qt%1/lib")
                                          .arg(versionNumber.majorVersion));
    } else if (m_packageMode == DevelopmentMode && m_bundleMode == BundleQt) {
        QList<QPair<QString, QString> > qtFolders;
        qtFolders.append(qMakePair(QString::fromLatin1("lib"),
                                   qtVersion->versionInfo().value(QLatin1String("QT_INSTALL_LIBS"))));
        qtFolders.append(qMakePair(QString::fromLatin1("plugins"),
                                   qtVersion->versionInfo().value(QLatin1String("QT_INSTALL_PLUGINS"))));
        qtFolders.append(qMakePair(QString::fromLatin1("imports"),
                                   qtVersion->versionInfo().value(QLatin1String("QT_INSTALL_IMPORTS"))));
        qtFolders.append(qMakePair(QString::fromLatin1("qml"),
                                   qtVersion->versionInfo().value(QLatin1String("QT_INSTALL_QML"))));

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

        env.appendOrSet(QLatin1String("QML2_IMPORT_PATH"),
                        QLatin1String("app/native/imports:app/native/qml"), QLatin1String(":"));
        env.appendOrSet(QLatin1String("QML_IMPORT_PATH"),
                        QLatin1String("app/native/imports:app/native/qml"), QLatin1String(":"));
        env.appendOrSet(QLatin1String("QT_PLUGIN_PATH"),
                        QLatin1String("app/native/plugins"), QLatin1String(":"));
        env.prependOrSetLibrarySearchPath(QLatin1String("app/native/lib"));
    } else if (m_packageMode == DevelopmentMode && m_bundleMode == DeployedQt) {
        env.appendOrSet(QLatin1String("QML2_IMPORT_PATH"),
                        QString::fromLatin1("%1/qml:%1/imports").arg(fullQtLibraryPath()),
                        QLatin1String(":"));
        env.appendOrSet(QLatin1String("QML_IMPORT_PATH"),
                        QString::fromLatin1("%1/qml:%1/imports").arg(fullQtLibraryPath()),
                        QLatin1String(":"));
        env.appendOrSet(QLatin1String("QT_PLUGIN_PATH"),
                        QString::fromLatin1("%1/plugins").arg(fullQtLibraryPath()), QLatin1String(":"));
        env.prependOrSetLibrarySearchPath(QString::fromLatin1("%1/lib").arg(fullQtLibraryPath()));
    }

    doc.setValue(BarDescriptorDocument::asset, QVariant::fromValue(assetList));

    QVariant envVar;
    envVar.setValue(Utils::EnvironmentItem::fromStringList(env.toStringList()));
    doc.setValue(BarDescriptorDocument::env, envVar);

    doc.setFilePath(preparedFilePath);
    if (!doc.save(&errorString)) {
        raiseError(tr("Error saving prepared BAR application descriptor file '%1' - %2")
            .arg(QDir::toNativeSeparators(preparedFilePath))
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
