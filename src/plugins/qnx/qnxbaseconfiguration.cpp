/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
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

#include "qnxbaseconfiguration.h"
#include "qnxutils.h"
#include "qnxtoolchain.h"

#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>

#include <debugger/debuggerkitinformation.h>

#include <coreplugin/icore.h>

#include <QVariantMap>
#include <QFileInfo>
#include <QMessageBox>

namespace Qnx {
namespace Internal {

const QLatin1String QNXEnvFileKey("EnvFile");
const QLatin1String QNXVersionKey("QNXVersion");
// For backward compatibility
const QLatin1String NDKEnvFileKey("NDKEnvFile");


using namespace Utils;
using namespace ProjectExplorer;

QnxBaseConfiguration::QnxBaseConfiguration()
{
}

QnxBaseConfiguration::QnxBaseConfiguration(const FileName &envFile)
{
    ctor(envFile);
}

QnxBaseConfiguration::QnxBaseConfiguration(const QVariantMap &data)
{
    QString envFilePath = data.value(QNXEnvFileKey).toString();
    if (envFilePath.isEmpty())
        envFilePath = data.value(NDKEnvFileKey).toString();

    m_version = QnxVersionNumber(data.value(QNXVersionKey).toString());
    ctor(FileName::fromString(envFilePath));
}

QnxBaseConfiguration::~QnxBaseConfiguration()
{
}

Utils::FileName QnxBaseConfiguration::envFile() const
{
    return m_envFile;
}

Utils::FileName QnxBaseConfiguration::qnxTarget() const
{
    return m_qnxTarget;
}

Utils::FileName QnxBaseConfiguration::qnxHost() const
{
    return m_qnxHost;
}

Utils::FileName QnxBaseConfiguration::qccCompilerPath() const
{
    return m_qccCompiler;
}

Utils::FileName QnxBaseConfiguration::armDebuggerPath() const
{
    return m_armlev7Debugger;
}

Utils::FileName QnxBaseConfiguration::x86DebuggerPath() const
{
    return m_x86Debugger;
}

QList<EnvironmentItem> QnxBaseConfiguration::qnxEnv() const
{
    return m_qnxEnv;
}

QnxVersionNumber QnxBaseConfiguration::version() const
{
    return m_version;
}

QVariantMap QnxBaseConfiguration::toMap() const
{
    QVariantMap data;
    data.insert(QLatin1String(QNXEnvFileKey), m_envFile.toString());
    data.insert(QLatin1String(QNXVersionKey), m_version.toString());
    return data;
}

bool QnxBaseConfiguration::isValid() const
{
    return !m_qccCompiler.isEmpty()
            && !m_armlev7Debugger.isEmpty()
            && !m_x86Debugger.isEmpty();
}

void QnxBaseConfiguration::ctor(const FileName &envScript)
{
    QTC_ASSERT(!envScript.isEmpty() && envScript.toFileInfo().exists(), return);
    m_envFile = envScript;
    m_qnxEnv = QnxUtils::qnxEnvironmentFromEnvFile(m_envFile.toString());
    foreach (const Utils::EnvironmentItem &item, m_qnxEnv) {
        if (item.name == QLatin1String("QNX_TARGET"))
            m_qnxTarget = Utils::FileName::fromString(item.value);

        else if (item.name == QLatin1String("QNX_HOST"))
            m_qnxHost = Utils::FileName::fromString(item.value);
    }

    FileName qccPath = FileName::fromString(Utils::HostOsInfo::withExecutableSuffix(
        m_qnxHost.toString() + QLatin1String("/usr/bin/qcc")));
    FileName armlev7GdbPath = FileName::fromString(Utils::HostOsInfo::withExecutableSuffix(
        m_qnxHost.toString() + QLatin1String("/usr/bin/ntoarm-gdb")));
    if (!armlev7GdbPath.toFileInfo().exists()) {
        armlev7GdbPath = FileName::fromString(Utils::HostOsInfo::withExecutableSuffix(
            m_qnxHost.toString() + QLatin1String("/usr/bin/ntoarmv7-gdb")));
    }

    FileName x86GdbPath = FileName::fromString(Utils::HostOsInfo::withExecutableSuffix(
        m_qnxHost.toString() + QLatin1String("/usr/bin/ntox86-gdb")));

    if (qccPath.toFileInfo().exists())
        m_qccCompiler = qccPath;

    if (armlev7GdbPath.toFileInfo().exists())
        m_armlev7Debugger = armlev7GdbPath;

    if (x86GdbPath.toFileInfo().exists())
        m_x86Debugger = x86GdbPath;
}

QVariant QnxBaseConfiguration::createDebuggerItem(QnxArchitecture arch,
                                              const QString &displayName)
{
    Utils::FileName command = (arch == X86) ? x86DebuggerPath() : armDebuggerPath();
    Debugger::DebuggerItem debugger;
    debugger.setCommand(command);
    debugger.setEngineType(Debugger::GdbEngineType);
    debugger.setAbi(Abi(arch == Qnx::ArmLeV7 ? Abi::ArmArchitecture : Abi::X86Architecture,
                        Abi::LinuxOS, Abi::GenericLinuxFlavor, Abi::ElfFormat, 32));
    debugger.setAutoDetected(true);
    debugger.setDisplayName(displayName);
    return Debugger::DebuggerItemManager::registerDebugger(debugger);
}

QnxToolChain *QnxBaseConfiguration::createToolChain(QnxArchitecture arch,
                                                    const QString &displayName,
                                                    const QString &ndkPath)
{
    QnxToolChain *toolChain = new QnxToolChain(ProjectExplorer::ToolChain::AutoDetection);
    toolChain->setCompilerCommand(m_qccCompiler);
    toolChain->setTargetAbi(Abi((arch == Qnx::ArmLeV7) ? Abi::ArmArchitecture : Abi::X86Architecture,
                                Abi::LinuxOS, Abi::GenericLinuxFlavor, Abi::ElfFormat, 32));
    toolChain->setDisplayName(displayName);
    toolChain->setNdkPath(ndkPath);
    ProjectExplorer::ToolChainManager::registerToolChain(toolChain);
    return toolChain;
}

QStringList QnxBaseConfiguration::validationErrors() const
{
    QStringList errorStrings;
    if (m_qccCompiler.isEmpty())
        errorStrings << tr("- No GCC compiler found.");

    if (m_armlev7Debugger.isEmpty())
        errorStrings << tr("- No GDB debugger found for armvle7.");

    if (m_x86Debugger.isEmpty())
        errorStrings << tr("- No GDB debugger found for x86.");

    return errorStrings;
}

void QnxBaseConfiguration::setVersion(const QnxVersionNumber &version)
{
    m_version = version;
}

}
}
