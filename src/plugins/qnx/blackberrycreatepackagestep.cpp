/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
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
#include "blackberryqtversion.h"
#include "blackberrydeviceconfiguration.h"
#include "blackberrydeployinformation.h"
#include "blackberrysigningpasswordsdialog.h"

#include <debugger/debuggerrunconfigurationaspect.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/runconfiguration.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4nodes.h>
#include <qt4projectmanager/qt4project.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/qtcassert.h>

#include <QFile>

using namespace Qnx;
using namespace Qnx::Internal;

namespace {
const char PACKAGER_CMD[] = "blackberry-nativepackager";

const char QT_INSTALL_LIBS[]        = "QT_INSTALL_LIBS";
const char QT_INSTALL_LIBS_VAR[]    = "%QT_INSTALL_LIBS%";
const char QT_INSTALL_PLUGINS[]     = "QT_INSTALL_PLUGINS";
const char QT_INSTALL_PLUGINS_VAR[] = "%QT_INSTALL_PLUGINS%";
const char QT_INSTALL_IMPORTS[]     = "QT_INSTALL_IMPORTS";
const char QT_INSTALL_IMPORTS_VAR[] = "%QT_INSTALL_IMPORTS%";
const char QT_INSTALL_QML[]         = "QT_INSTALL_QML";
const char QT_INSTALL_QML_VAR[]     = "%QT_INSTALL_QML%";
const char SRC_DIR_VAR[]            = "%SRC_DIR%";

const char PACKAGE_MODE_KEY[]      = "Qt4ProjectManager.BlackBerryCreatePackageStep.PackageMode";
const char CSK_PASSWORD_KEY[]      = "Qt4ProjectManager.BlackBerryCreatePackageStep.CskPassword";
const char KEYSTORE_PASSWORD_KEY[] = "Qt4ProjectManager.BlackBerryCreatePackageStep.KeystorePassword";
const char SAVE_PASSWORDS_KEY[]    = "Qt4ProjectManager.BlackBerryCreatePackageStep.SavePasswords";
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
}

bool BlackBerryCreatePackageStep::init()
{
    if (!BlackBerryAbstractDeployStep::init())
        return false;

    const QString packageCmd = target()->activeBuildConfiguration()->environment().searchInPath(QLatin1String(PACKAGER_CMD));
    if (packageCmd.isEmpty()) {
        raiseError(tr("Could not find packager command '%1' in the build environment")
                   .arg(QLatin1String(PACKAGER_CMD)));
        return false;
    }

    BlackBerryDeployConfiguration *deployConfig = qobject_cast<BlackBerryDeployConfiguration *>(deployConfiguration());
    QTC_ASSERT(deployConfig, return false);

    QList<BarPackageDeployInformation> packagesToDeploy = deployConfig->deploymentInfo()->enabledPackages();
    if (packagesToDeploy.isEmpty()) {
        raiseError(tr("No packages enabled for deployment"));
        return false;
    }

    foreach (const BarPackageDeployInformation &info, packagesToDeploy) {
        if (info.appDescriptorPath().isEmpty()) {
            raiseError(tr("Application descriptor file not specified, please check deployment settings"));
            return false;
        }

        if (info.packagePath().isEmpty()) {
            raiseError(tr("No package specified, please check deployment settings"));
            return false;
        }

        const QString buildDir = QFileInfo(info.packagePath()).absolutePath();
        QDir dir(buildDir);
        if (!dir.exists()) {
            if (!dir.mkpath(buildDir)) {
                raiseError(tr("Could not create build directory '%1'").arg(buildDir));
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
                    raiseError(tr("Missing passwords for signing packages"));
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
    m_packageMode = static_cast<PackageMode>(map.value(QLatin1String(PACKAGE_MODE_KEY), DevelopmentMode).toInt());
    m_savePasswords = map.value(QLatin1String(SAVE_PASSWORDS_KEY), false).toBool();
    if (m_savePasswords) {
        m_cskPassword = map.value(QLatin1String(CSK_PASSWORD_KEY)).toString();
        m_keystorePassword = map.value(QLatin1String(KEYSTORE_PASSWORD_KEY)).toString();
    }
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

bool BlackBerryCreatePackageStep::prepareAppDescriptorFile(const QString &appDescriptorPath, const QString &preparedFilePath)
{
    BlackBerryQtVersion *qtVersion = dynamic_cast<BlackBerryQtVersion *>(QtSupport::QtKitInformation::qtVersion(target()->kit()));
    if (!qtVersion) {
        raiseError(tr("Error preparing application descriptor file"));
        return false;
    }

    QFile file(appDescriptorPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        raiseError(tr("Could not open '%1' for reading").arg(appDescriptorPath));
        return false;
    }

    QFile preparedFile(preparedFilePath);

    QByteArray fileContent = file.readAll();

    // Add Warning text
    const QString warningText = QString::fromLatin1("<!-- This file is autogenerated;"
                                                       " any changes will get overwritten if deploying with Qt Creator -->\n<qnx");
    fileContent.replace("<qnx", warningText.toLatin1());

    // Replace Qt path placeholders
    if (fileContent.contains(QT_INSTALL_LIBS_VAR))
        fileContent.replace(QT_INSTALL_LIBS_VAR, qtVersion->versionInfo().value(QLatin1String(QT_INSTALL_LIBS)).toLatin1());
    if (fileContent.contains(QT_INSTALL_PLUGINS_VAR))
        fileContent.replace(QT_INSTALL_PLUGINS_VAR, qtVersion->versionInfo().value(QLatin1String(QT_INSTALL_PLUGINS)).toLatin1());
    if (fileContent.contains(QT_INSTALL_IMPORTS_VAR))
        fileContent.replace(QT_INSTALL_IMPORTS_VAR, qtVersion->versionInfo().value(QLatin1String(QT_INSTALL_IMPORTS)).toLatin1());
    if (fileContent.contains(QT_INSTALL_QML_VAR))
        fileContent.replace(QT_INSTALL_QML_VAR, qtVersion->versionInfo().value(QLatin1String(QT_INSTALL_QML)).toLatin1());

    //Replace Source path placeholder
    if (fileContent.contains(SRC_DIR_VAR))
        fileContent.replace(SRC_DIR_VAR, QDir::toNativeSeparators(target()->project()->projectDirectory()).toLatin1());

    // Add parameter for QML debugging (if enabled)
    Debugger::DebuggerRunConfigurationAspect *aspect
            = target()->activeRunConfiguration()->extraAspect<Debugger::DebuggerRunConfigurationAspect>();
    if (aspect->useQmlDebugger()) {
        if (!fileContent.contains("-qmljsdebugger")) {
            const QString argString = QString::fromLatin1("<arg>-qmljsdebugger=port:%1</arg>\n</qnx>")
                    .arg(aspect->qmlDebugServerPort());
            fileContent.replace("</qnx>", argString.toLatin1());
        }
    }

    const QString buildDir = target()->activeBuildConfiguration()->buildDirectory();
    if (!preparedFile.open(QIODevice::WriteOnly)) {
        raiseError(tr("Could not create prepared application descriptor file in '%1'").arg(buildDir));
        return false;
    }

    preparedFile.write(fileContent);
    preparedFile.close();

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
