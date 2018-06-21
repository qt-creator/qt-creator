/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: BlackBerry (qt@blackberry.com), KDAB (info@kdab.com)
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

#include "qnxtoolchain.h"
#include "qnxconfiguration.h"
#include "qnxconfigurationmanager.h"
#include "qnxconstants.h"
#include "qnxutils.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <utils/algorithm.h>
#include <utils/pathchooser.h>

#include <QFormLayout>

using namespace ProjectExplorer;
using namespace Utils;

namespace Qnx {
namespace Internal {

static const char CompilerSdpPath[] = "Qnx.QnxToolChain.NDKPath";
static const char CpuDirKey[] = "Qnx.QnxToolChain.CpuDir";

static QList<Abi> detectTargetAbis(const FileName &sdpPath)
{
    QList<Abi> result;
    FileName qnxTarget;

    if (!sdpPath.fileName().isEmpty()) {
        QList<Utils::EnvironmentItem> environment = QnxUtils::qnxEnvironment(sdpPath.toString());
        foreach (const Utils::EnvironmentItem &item, environment) {
            if (item.name == QLatin1Literal("QNX_TARGET"))
                qnxTarget = FileName::fromString(item.value);
        }
    }

    if (qnxTarget.isEmpty())
        return result;

    QList<QnxTarget> targets = QnxUtils::findTargets(qnxTarget);
    for (const auto &target : targets) {
        if (!result.contains(target.m_abi))
            result.append(target.m_abi);
    }

    Utils::sort(result,
              [](const Abi &arg1, const Abi &arg2) { return arg1.toString() < arg2.toString(); });

    return result;
}

static void setQnxEnvironment(Environment &env, const QList<EnvironmentItem> &qnxEnv)
{
    // We only need to set QNX_HOST and QNX_TARGET needed when running qcc
    foreach (const EnvironmentItem &item, qnxEnv) {
        if (item.name == QLatin1String("QNX_HOST") ||
                item.name == QLatin1String("QNX_TARGET") )
            env.set(item.name, item.value);
    }
}

// Qcc is a multi-compiler driver, and most of the gcc options can be accomplished by using the -Wp, and -Wc
// options to pass the options directly down to the compiler
static QStringList reinterpretOptions(const QStringList &args)
{
    QStringList arguments;
    foreach (const QString &str, args) {
        if (str.startsWith(QLatin1String("--sysroot=")))
            continue;
        QString arg = str;
        if (arg == QLatin1String("-v")
            || arg == QLatin1String("-dM"))
                arg.prepend(QLatin1String("-Wp,"));
        arguments << arg;
    }

    return arguments;
}

QnxToolChain::QnxToolChain(ToolChain::Detection d)
    : GccToolChain(Constants::QNX_TOOLCHAIN_ID, d)
{
    setOptionsReinterpreter(&reinterpretOptions);
}

QnxToolChain::QnxToolChain(Core::Id l, ToolChain::Detection d)
    : QnxToolChain(d)
{
    setLanguage(l);
}


QString QnxToolChain::typeDisplayName() const
{
    return QnxToolChainFactory::tr("QCC");
}

ToolChainConfigWidget *QnxToolChain::configurationWidget()
{
    return new QnxToolChainConfigWidget(this);
}

void QnxToolChain::addToEnvironment(Environment &env) const
{
    if (env.value("QNX_HOST").isEmpty() || env.value("QNX_TARGET").isEmpty())
        setQnxEnvironment(env, QnxUtils::qnxEnvironment(m_sdpPath));

    GccToolChain::addToEnvironment(env);
}

FileNameList QnxToolChain::suggestedMkspecList() const
{
    FileNameList mkspecList;
    mkspecList << FileName::fromLatin1("qnx-armle-v7-qcc");
    mkspecList << FileName::fromLatin1("qnx-x86-qcc");
    mkspecList << FileName::fromLatin1("qnx-aarch64le-qcc");
    mkspecList << FileName::fromLatin1("qnx-x86-64-qcc");

    return mkspecList;
}

QVariantMap QnxToolChain::toMap() const
{
    QVariantMap data = GccToolChain::toMap();
    data.insert(QLatin1String(CompilerSdpPath), m_sdpPath);
    data.insert(QLatin1String(CpuDirKey), m_cpuDir);
    return data;
}

bool QnxToolChain::fromMap(const QVariantMap &data)
{
    if (!GccToolChain::fromMap(data))
        return false;

    m_sdpPath = data.value(QLatin1String(CompilerSdpPath)).toString();
    m_cpuDir = data.value(QLatin1String(CpuDirKey)).toString();

    // Make the ABIs QNX specific (if they aren't already).
    setSupportedAbis(QnxUtils::convertAbis(supportedAbis()));
    setTargetAbi(QnxUtils::convertAbi(targetAbi()));

    return true;
}

QString QnxToolChain::sdpPath() const
{
    return m_sdpPath;
}

void QnxToolChain::setSdpPath(const QString &sdpPath)
{
    if (m_sdpPath == sdpPath)
        return;
    m_sdpPath = sdpPath;
    toolChainUpdated();
}

QString QnxToolChain::cpuDir() const
{
    return m_cpuDir;
}

void QnxToolChain::setCpuDir(const QString &cpuDir)
{
    if (m_cpuDir == cpuDir)
        return;
    m_cpuDir = cpuDir;
    toolChainUpdated();
}

GccToolChain::DetectedAbisResult QnxToolChain::detectSupportedAbis() const
{
    return detectTargetAbis(FileName::fromString(m_sdpPath));
}

bool QnxToolChain::operator ==(const ToolChain &other) const
{
    if (!GccToolChain::operator ==(other))
        return false;

    auto qnxTc = static_cast<const QnxToolChain *>(&other);

    return m_sdpPath == qnxTc->m_sdpPath && m_cpuDir == qnxTc->m_cpuDir;
}

// --------------------------------------------------------------------------
// QnxToolChainFactory
// --------------------------------------------------------------------------

QnxToolChainFactory::QnxToolChainFactory()
{
    setDisplayName(tr("QCC"));
}

QList<ProjectExplorer::ToolChain *> QnxToolChainFactory::autoDetect(
       const QList<ProjectExplorer::ToolChain *> &alreadyKnown)
{
    QList<ToolChain *> tcs;
    QList<QnxConfiguration *> configurations =
            QnxConfigurationManager::instance()->configurations();
    foreach (QnxConfiguration *configuration, configurations)
        tcs += configuration->autoDetect(alreadyKnown);
    return tcs;
}

QSet<Core::Id> QnxToolChainFactory::supportedLanguages() const
{
    return {ProjectExplorer::Constants::CXX_LANGUAGE_ID};
}

bool QnxToolChainFactory::canRestore(const QVariantMap &data)
{
    return typeIdFromMap(data) == Constants::QNX_TOOLCHAIN_ID;
}

ToolChain *QnxToolChainFactory::restore(const QVariantMap &data)
{
    QnxToolChain *tc = new QnxToolChain(ToolChain::ManualDetection);
    if (tc->fromMap(data))
        return tc;

    delete tc;
    return 0;
}

bool QnxToolChainFactory::canCreate()
{
    return true;
}

ToolChain *QnxToolChainFactory::create(Core::Id l)
{
    return new QnxToolChain(l, ToolChain::ManualDetection);
}

//---------------------------------------------------------------------------------
// QnxToolChainConfigWidget
//---------------------------------------------------------------------------------

QnxToolChainConfigWidget::QnxToolChainConfigWidget(QnxToolChain *tc)
    : ToolChainConfigWidget(tc)
    , m_compilerCommand(new PathChooser)
    , m_sdpPath(new PathChooser)
    , m_abiWidget(new AbiWidget)
{
    m_compilerCommand->setExpectedKind(PathChooser::ExistingCommand);
    m_compilerCommand->setHistoryCompleter(QLatin1String("Qnx.ToolChain.History"));
    m_compilerCommand->setFileName(tc->compilerCommand());
    m_compilerCommand->setEnabled(!tc->isAutoDetected());

    m_sdpPath->setExpectedKind(PathChooser::ExistingDirectory);
    m_sdpPath->setHistoryCompleter(QLatin1String("Qnx.Sdp.History"));
    m_sdpPath->setPath(tc->sdpPath());
    m_sdpPath->setEnabled(!tc->isAutoDetected());

    QList<Abi> abiList = detectTargetAbis(m_sdpPath->fileName());
    m_abiWidget->setAbis(abiList, tc->targetAbi());
    m_abiWidget->setEnabled(!tc->isAutoDetected() && !abiList.isEmpty());

    m_mainLayout->addRow(tr("&Compiler path:"), m_compilerCommand);
    //: SDP refers to 'Software Development Platform'.
    m_mainLayout->addRow(tr("SDP path:"), m_sdpPath);
    m_mainLayout->addRow(tr("&ABI:"), m_abiWidget);

    connect(m_compilerCommand, &PathChooser::rawPathChanged, this, &ToolChainConfigWidget::dirty);
    connect(m_sdpPath, &PathChooser::rawPathChanged,
            this, &QnxToolChainConfigWidget::handleSdpPathChange);
    connect(m_abiWidget, &AbiWidget::abiChanged, this, &ToolChainConfigWidget::dirty);
}

void QnxToolChainConfigWidget::applyImpl()
{
    if (toolChain()->isAutoDetected())
        return;

    QnxToolChain *tc = static_cast<QnxToolChain *>(toolChain());
    Q_ASSERT(tc);
    QString displayName = tc->displayName();
    tc->setDisplayName(displayName); // reset display name
    tc->setSdpPath(m_sdpPath->fileName().toString());
    tc->setTargetAbi(m_abiWidget->currentAbi());
    tc->resetToolChain(m_compilerCommand->fileName());
}

void QnxToolChainConfigWidget::discardImpl()
{
    // subwidgets are not yet connected!
    QSignalBlocker blocker(this);
    QnxToolChain *tc = static_cast<QnxToolChain *>(toolChain());
    m_compilerCommand->setFileName(tc->compilerCommand());
    m_sdpPath->setPath(tc->sdpPath());
    m_abiWidget->setAbis(tc->supportedAbis(), tc->targetAbi());
    if (!m_compilerCommand->path().isEmpty())
        m_abiWidget->setEnabled(true);
}

bool QnxToolChainConfigWidget::isDirtyImpl() const
{
    QnxToolChain *tc = static_cast<QnxToolChain *>(toolChain());
    Q_ASSERT(tc);
    return m_compilerCommand->fileName() != tc->compilerCommand()
            || m_sdpPath->path() != tc->sdpPath()
            || m_abiWidget->currentAbi() != tc->targetAbi();
}

void QnxToolChainConfigWidget::handleSdpPathChange()
{
    Abi currentAbi = m_abiWidget->currentAbi();
    bool customAbi = m_abiWidget->isCustomAbi();
    QList<Abi> abiList = detectTargetAbis(m_sdpPath->fileName());

    m_abiWidget->setEnabled(!abiList.isEmpty());

    // Find a good ABI for the new compiler:
    Abi newAbi;
    if (customAbi)
        newAbi = currentAbi;
    else if (abiList.contains(currentAbi))
        newAbi = currentAbi;

    m_abiWidget->setAbis(abiList, newAbi);
    emit dirty();
}

} // namespace Internal
} // namespace Qnx
