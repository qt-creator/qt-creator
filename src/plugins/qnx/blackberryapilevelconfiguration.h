/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
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

#ifndef BLACKBERRYCONFIGURATIONS_H
#define BLACKBERRYCONFIGURATIONS_H

#include "qnxutils.h"
#include "blackberryversionnumber.h"
#include "qnxconstants.h"

#include <utils/environment.h>
#include <utils/fileutils.h>

#include <projectexplorer/abi.h>
#include <projectexplorer/kit.h>

#include <QObject>
#include <QCoreApplication>

namespace QtSupport { class BaseQtVersion; }
namespace Debugger { class DebuggerItem; }

namespace Qnx {
namespace Internal {

class QnxAbstractQtVersion;
class QnxToolChain;

class BlackBerryApiLevelConfiguration
{
    Q_DECLARE_TR_FUNCTIONS(Qnx::Internal::BlackBerryApiLevelConfiguration)
public:
    BlackBerryApiLevelConfiguration(const NdkInstallInformation &ndkInstallInfo);
    BlackBerryApiLevelConfiguration(const Utils::FileName &ndkEnvFile);
    BlackBerryApiLevelConfiguration(const QVariantMap &data);
    bool activate();
    void deactivate();
    QString ndkPath() const;
    QString displayName() const;
    QString targetName() const;
    QString qnxHost() const;
    BlackBerryVersionNumber version() const;
    bool isAutoDetected() const;
    Utils::FileName autoDetectionSource() const;
    bool isActive() const;
    bool isValid() const;
    Utils::FileName ndkEnvFile() const;
    Utils::FileName qmake4BinaryFile() const;
    Utils::FileName qmake5BinaryFile() const;
    Utils::FileName gccCompiler() const;
    Utils::FileName deviceDebuger() const;
    Utils::FileName simulatorDebuger() const;
    Utils::FileName sysRoot() const;
    QList<Utils::EnvironmentItem> qnxEnv() const;
    QVariantMap toMap() const;

#ifdef WITH_TESTS
    static void setFakeConfig(bool fakeConfig);
    static bool fakeConfig();
#endif

private:
    QString m_displayName;
    QString m_targetName;
    QString m_qnxHost;
    BlackBerryVersionNumber m_version;
    Utils::FileName m_autoDetectionSource;
    Utils::FileName m_ndkEnvFile;
    Utils::FileName m_qmake4BinaryFile;
    Utils::FileName m_qmake5BinaryFile;
    Utils::FileName m_gccCompiler;
    Utils::FileName m_deviceDebugger;
    Utils::FileName m_simulatorDebugger;
    Utils::FileName m_sysRoot;
    QList<Utils::EnvironmentItem> m_qnxEnv;

    void ctor();

    QnxAbstractQtVersion* createQtVersion(
            const Utils::FileName &qmakePath, Qnx::QnxArchitecture arch, const QString &versionName);
    QnxToolChain* createToolChain(
            ProjectExplorer::Abi abi, const QString &versionName);
    QVariant createDebuggerItem(
            QList<ProjectExplorer::Abi> abis, Qnx::QnxArchitecture arch, const QString &versionName);
    ProjectExplorer::Kit* createKit(
            QnxAbstractQtVersion* version, QnxToolChain* toolChain,
            const QVariant &debuggerItemId);

#ifdef WITH_TESTS
    static bool m_fakeConfig;
#endif
};

} // namespace Internal
} // namespace Qnx

#endif // BLACKBERRYCONFIGURATIONS_H
