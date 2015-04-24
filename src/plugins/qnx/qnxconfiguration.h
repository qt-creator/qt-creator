/**************************************************************************
**
** Copyright (C) 2015 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QNXCONFIGURATION_H
#define QNXCONFIGURATION_H

#include "qnxconstants.h"
#include "qnxutils.h"
#include "qnxversionnumber.h"

#include <utils/fileutils.h>
#include <utils/environment.h>

#include <projectexplorer/abi.h>

#include <debugger/debuggeritemmanager.h>

#include <QVariant>

namespace ProjectExplorer { class Kit; }

namespace Qnx {
namespace Internal {

class QnxToolChain;
class QnxQtVersion;

class QnxConfiguration
{
public:
    QnxConfiguration();
    QnxConfiguration(const Utils::FileName &sdpEnvFile);
    QnxConfiguration(const QVariantMap &data);

    Utils::FileName envFile() const;
    Utils::FileName qnxTarget() const;
    Utils::FileName qnxHost() const;
    Utils::FileName qccCompilerPath() const;
    Utils::FileName armDebuggerPath() const;
    Utils::FileName x86DebuggerPath() const;
    QList<Utils::EnvironmentItem> qnxEnv() const;
    QnxVersionNumber version() const;
    QVariantMap toMap() const;

    bool isValid() const;

    QString displayName() const;
    bool activate();
    void deactivate();
    bool isActive() const;
    bool canCreateKits() const;
    Utils::FileName sdpPath() const;
    QnxQtVersion* qnxQtVersion(QnxArchitecture arch) const;

private:
    QVariant createDebuggerItem(Qnx::QnxArchitecture arch,
                                             const QString &displayName);
    QnxToolChain* createToolChain(Qnx::QnxArchitecture arch,
                                 const QString &displayName,
                                 const QString &ndkPath);

    ProjectExplorer::Kit *createKit(QnxArchitecture arch,
                                    QnxToolChain *toolChain,
                                    const QVariant &debuggerItemId,
                                    const QString &displayName);

    QStringList validationErrors() const;

    void setVersion(const QnxVersionNumber& version);

    void readInformation();

    void setDefaultConfiguration(const Utils::FileName &envScript);

    QString m_configName;

    Utils::FileName m_envFile;
    Utils::FileName m_qnxTarget;
    Utils::FileName m_qnxHost;
    Utils::FileName m_qccCompiler;
    Utils::FileName m_armlev7Debugger;
    Utils::FileName m_x86Debugger;
    QList<Utils::EnvironmentItem> m_qnxEnv;
    QnxVersionNumber m_version;
};

} // Internal
} // Qnx

#endif // QNXCONFIGURATION_H
