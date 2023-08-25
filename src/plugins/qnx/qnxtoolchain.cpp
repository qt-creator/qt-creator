// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qnxtoolchain.h"

#include "qnxconstants.h"
#include "qnxsettingspage.h"
#include "qnxtr.h"
#include "qnxutils.h"

#include <projectexplorer/abiwidget.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchainconfigwidget.h>

#include <utils/algorithm.h>
#include <utils/pathchooser.h>

#include <QFormLayout>

using namespace ProjectExplorer;
using namespace Utils;

namespace Qnx::Internal {

// QnxToolChainConfigWidget

class QnxToolChainConfigWidget : public ToolChainConfigWidget
{
public:
    QnxToolChainConfigWidget(QnxToolChain *tc);

private:
    void applyImpl() override;
    void discardImpl() override;
    bool isDirtyImpl() const override;
    void makeReadOnlyImpl() override { }

    void handleSdpPathChange();

    PathChooser *m_compilerCommand;
    PathChooser *m_sdpPath;
    ProjectExplorer::AbiWidget *m_abiWidget;
};

static Abis detectTargetAbis(const FilePath &sdpPath)
{
    Abis result;
    FilePath qnxTarget;

    if (!sdpPath.fileName().isEmpty()) {
        const EnvironmentItems environment = QnxUtils::qnxEnvironment(sdpPath);
        for (const EnvironmentItem &item : environment) {
            if (item.name == QLatin1String("QNX_TARGET"))
                qnxTarget = sdpPath.withNewPath(item.value);
        }
    }

    if (qnxTarget.isEmpty())
        return result;

    QList<QnxTarget> targets = QnxUtils::findTargets(qnxTarget);
    for (const auto &target : targets) {
        if (!result.contains(target.m_abi))
            result.append(target.m_abi);
    }

    return Utils::sorted(std::move(result),
              [](const Abi &arg1, const Abi &arg2) { return arg1.toString() < arg2.toString(); });
}

static void setQnxEnvironment(Environment &env, const EnvironmentItems &qnxEnv)
{
    // We only need to set QNX_HOST, QNX_TARGET, and QNX_CONFIGURATION_EXCLUSIVE
    // needed when running qcc
    for (const EnvironmentItem &item : qnxEnv) {
        if (item.name == QLatin1String("QNX_HOST") ||
            item.name == QLatin1String("QNX_TARGET") ||
            item.name == QLatin1String("QNX_CONFIGURATION_EXCLUSIVE"))
            env.set(item.name, item.value);
    }
}

// Qcc is a multi-compiler driver, and most of the gcc options can be accomplished by using the -Wp, and -Wc
// options to pass the options directly down to the compiler
static QStringList reinterpretOptions(const QStringList &args)
{
    QStringList arguments;
    for (const QString &str : args) {
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

QnxToolChain::QnxToolChain()
    : GccToolChain(Constants::QNX_TOOLCHAIN_ID)
{
    setOptionsReinterpreter(&reinterpretOptions);
    setTypeDisplayName(Tr::tr("QCC"));

    sdpPath.setSettingsKey("Qnx.QnxToolChain.NDKPath");
    connect(&sdpPath, &BaseAspect::changed, this, &QnxToolChain::toolChainUpdated);

    cpuDir.setSettingsKey("Qnx.QnxToolChain.CpuDir");
    connect(&cpuDir, &BaseAspect::changed, this, &QnxToolChain::toolChainUpdated);

    connect(this, &AspectContainer::fromMapFinished, this, [this] {
        // Make the ABIs QNX specific (if they aren't already).
        setSupportedAbis(QnxUtils::convertAbis(supportedAbis()));
        setTargetAbi(QnxUtils::convertAbi(targetAbi()));
    });
}

std::unique_ptr<ToolChainConfigWidget> QnxToolChain::createConfigurationWidget()
{
    return std::make_unique<QnxToolChainConfigWidget>(this);
}

void QnxToolChain::addToEnvironment(Environment &env) const
{
    if (env.expandedValueForKey("QNX_HOST").isEmpty() ||
        env.expandedValueForKey("QNX_TARGET").isEmpty() ||
        env.expandedValueForKey("QNX_CONFIGURATION_EXCLUSIVE").isEmpty())
        setQnxEnvironment(env, QnxUtils::qnxEnvironment(sdpPath()));

    GccToolChain::addToEnvironment(env);
}

QStringList QnxToolChain::suggestedMkspecList() const
{
    return {
        "qnx-armle-v7-qcc",
        "qnx-x86-qcc",
        "qnx-aarch64le-qcc",
        "qnx-x86-64-qcc"
    };
}

GccToolChain::DetectedAbisResult QnxToolChain::detectSupportedAbis() const
{
    // "unknown-qnx-gnu"is needed to get the "--target=xxx" parameter sent code model,
    // which gets translated as "x86_64-qnx-gnu", which gets Clang to happily parse
    // the QNX code.
    //
    // Without it on Windows Clang defaults to a MSVC mode, which breaks with
    // the QNX code, which is mostly GNU based.
    return GccToolChain::DetectedAbisResult{detectTargetAbis(sdpPath()), "unknown-qnx-gnu"};
}

bool QnxToolChain::operator ==(const ToolChain &other) const
{
    if (!GccToolChain::operator ==(other))
        return false;

    auto qnxTc = static_cast<const QnxToolChain *>(&other);

    return sdpPath() == qnxTc->sdpPath() && cpuDir() == qnxTc->cpuDir();
}

// --------------------------------------------------------------------------
// QnxToolChainFactory
// --------------------------------------------------------------------------

QnxToolChainFactory::QnxToolChainFactory()
{
    setDisplayName(Tr::tr("QCC"));
    setSupportedToolChainType(Constants::QNX_TOOLCHAIN_ID);
    setSupportedLanguages({ProjectExplorer::Constants::C_LANGUAGE_ID,
                           ProjectExplorer::Constants::CXX_LANGUAGE_ID});
    setToolchainConstructor([] { return new QnxToolChain; });
    setUserCreatable(true);
}

Toolchains QnxToolChainFactory::autoDetect(const ToolchainDetector &detector) const
{
    // FIXME: Support detecting toolchains on remote devices
    if (detector.device)
        return {};

    Toolchains tcs = QnxSettingsPage::autoDetect(detector.alreadyKnown);
    return tcs;
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
    m_compilerCommand->setHistoryCompleter("Qnx.ToolChain.History");
    m_compilerCommand->setFilePath(tc->compilerCommand());
    m_compilerCommand->setEnabled(!tc->isAutoDetected());

    m_sdpPath->setExpectedKind(PathChooser::ExistingDirectory);
    m_sdpPath->setHistoryCompleter("Qnx.Sdp.History");
    m_sdpPath->setFilePath(tc->sdpPath());
    m_sdpPath->setEnabled(!tc->isAutoDetected());

    const Abis abiList = detectTargetAbis(m_sdpPath->filePath());
    m_abiWidget->setAbis(abiList, tc->targetAbi());
    m_abiWidget->setEnabled(!tc->isAutoDetected() && !abiList.isEmpty());

    m_mainLayout->addRow(Tr::tr("&Compiler path:"), m_compilerCommand);
    //: SDP refers to 'Software Development Platform'.
    m_mainLayout->addRow(Tr::tr("SDP path:"), m_sdpPath);
    m_mainLayout->addRow(Tr::tr("&ABI:"), m_abiWidget);

    connect(m_compilerCommand, &PathChooser::rawPathChanged, this, &ToolChainConfigWidget::dirty);
    connect(m_sdpPath, &PathChooser::rawPathChanged,
            this, &QnxToolChainConfigWidget::handleSdpPathChange);
    connect(m_abiWidget, &AbiWidget::abiChanged, this, &ToolChainConfigWidget::dirty);
}

void QnxToolChainConfigWidget::applyImpl()
{
    if (toolChain()->isAutoDetected())
        return;

    auto tc = static_cast<QnxToolChain *>(toolChain());
    Q_ASSERT(tc);
    QString displayName = tc->displayName();
    tc->setDisplayName(displayName); // reset display name
    tc->sdpPath.setValue(m_sdpPath->filePath());
    tc->setTargetAbi(m_abiWidget->currentAbi());
    tc->resetToolChain(m_compilerCommand->filePath());
}

void QnxToolChainConfigWidget::discardImpl()
{
    // subwidgets are not yet connected!
    QSignalBlocker blocker(this);
    auto tc = static_cast<const QnxToolChain *>(toolChain());
    m_compilerCommand->setFilePath(tc->compilerCommand());
    m_sdpPath->setFilePath(tc->sdpPath());
    m_abiWidget->setAbis(tc->supportedAbis(), tc->targetAbi());
    if (!m_compilerCommand->filePath().toString().isEmpty())
        m_abiWidget->setEnabled(true);
}

bool QnxToolChainConfigWidget::isDirtyImpl() const
{
    auto tc = static_cast<const QnxToolChain *>(toolChain());
    Q_ASSERT(tc);
    return m_compilerCommand->filePath() != tc->compilerCommand()
            || m_sdpPath->filePath() != tc->sdpPath()
            || m_abiWidget->currentAbi() != tc->targetAbi();
}

void QnxToolChainConfigWidget::handleSdpPathChange()
{
    const Abi currentAbi = m_abiWidget->currentAbi();
    const bool customAbi = m_abiWidget->isCustomAbi();
    const Abis abiList = detectTargetAbis(m_sdpPath->filePath());

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

} // Qnx::Internal
