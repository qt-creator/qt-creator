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

// QnxToolchainConfigWidget

class QnxToolchainConfigWidget : public ToolchainConfigWidget
{
public:
    QnxToolchainConfigWidget(const ToolchainBundle &bundle);

private:
    void applyImpl() override;
    void discardImpl() override;
    bool isDirtyImpl() const override;
    void makeReadOnlyImpl() override { }

    void handleSdpPathChange();

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

QnxToolchain::QnxToolchain()
    : GccToolchain(Constants::QNX_TOOLCHAIN_ID)
{
    setOptionsReinterpreter(&reinterpretOptions);
    setTypeDisplayName(Tr::tr("QCC"));

    sdpPath.setSettingsKey("Qnx.QnxToolChain.NDKPath");
    connect(&sdpPath, &BaseAspect::changed, this, &QnxToolchain::toolChainUpdated);

    cpuDir.setSettingsKey("Qnx.QnxToolChain.CpuDir");
    connect(&cpuDir, &BaseAspect::changed, this, &QnxToolchain::toolChainUpdated);

    connect(this, &AspectContainer::fromMapFinished, this, [this] {
        // Make the ABIs QNX specific (if they aren't already).
        setSupportedAbis(QnxUtils::convertAbis(supportedAbis()));
        setTargetAbi(QnxUtils::convertAbi(targetAbi()));
    });
}

void QnxToolchain::addToEnvironment(Environment &env) const
{
    if (env.expandedValueForKey("QNX_HOST").isEmpty() ||
        env.expandedValueForKey("QNX_TARGET").isEmpty() ||
        env.expandedValueForKey("QNX_CONFIGURATION_EXCLUSIVE").isEmpty())
        setQnxEnvironment(env, QnxUtils::qnxEnvironment(sdpPath()));

    GccToolchain::addToEnvironment(env);
}

QStringList QnxToolchain::suggestedMkspecList() const
{
    return {
        "qnx-armle-v7-qcc",
        "qnx-x86-qcc",
        "qnx-aarch64le-qcc",
        "qnx-x86-64-qcc"
    };
}

GccToolchain::DetectedAbisResult QnxToolchain::detectSupportedAbis() const
{
    // "unknown-qnx-gnu"is needed to get the "--target=xxx" parameter sent code model,
    // which gets translated as "x86_64-qnx-gnu", which gets Clang to happily parse
    // the QNX code.
    //
    // Without it on Windows Clang defaults to a MSVC mode, which breaks with
    // the QNX code, which is mostly GNU based.
    return GccToolchain::DetectedAbisResult{detectTargetAbis(sdpPath()), "unknown-qnx-gnu"};
}

bool QnxToolchain::operator ==(const Toolchain &other) const
{
    if (!GccToolchain::operator ==(other))
        return false;

    auto qnxTc = static_cast<const QnxToolchain *>(&other);

    return sdpPath() == qnxTc->sdpPath() && cpuDir() == qnxTc->cpuDir();
}

//---------------------------------------------------------------------------------
// QnxToolChainConfigWidget
//---------------------------------------------------------------------------------

QnxToolchainConfigWidget::QnxToolchainConfigWidget(const ToolchainBundle &bundle)
    : ToolchainConfigWidget(bundle)
    , m_sdpPath(new PathChooser)
    , m_abiWidget(new AbiWidget)
{
    m_sdpPath->setExpectedKind(PathChooser::ExistingDirectory);
    m_sdpPath->setHistoryCompleter("Qnx.Sdp.History");
    m_sdpPath->setFilePath(bundle.get<QnxToolchain>(&QnxToolchain::sdpPath)());
    m_sdpPath->setEnabled(!bundle.isAutoDetected());

    const Abis abiList = detectTargetAbis(m_sdpPath->filePath());
    m_abiWidget->setAbis(abiList, bundle.targetAbi());
    m_abiWidget->setEnabled(!bundle.isAutoDetected() && !abiList.isEmpty());

    //: SDP refers to 'Software Development Platform'.
    m_mainLayout->addRow(Tr::tr("SDP path:"), m_sdpPath);
    m_mainLayout->addRow(Tr::tr("&ABI:"), m_abiWidget);

    connect(m_sdpPath, &PathChooser::rawPathChanged,
            this, &QnxToolchainConfigWidget::handleSdpPathChange);
    connect(m_abiWidget, &AbiWidget::abiChanged, this, &ToolchainConfigWidget::dirty);
}

void QnxToolchainConfigWidget::applyImpl()
{
    if (bundle().isAutoDetected())
        return;

    bundle().setTargetAbi(m_abiWidget->currentAbi());
    bundle().forEach<QnxToolchain>([this](QnxToolchain &tc) {
        tc.sdpPath.setValue(m_sdpPath->filePath());
        tc.resetToolchain(compilerCommand(tc.language()));
    });
}

void QnxToolchainConfigWidget::discardImpl()
{
    // subwidgets are not yet connected!
    QSignalBlocker blocker(this);
    m_sdpPath->setFilePath(bundle().get(&QnxToolchain::sdpPath)());
    m_abiWidget->setAbis(bundle().supportedAbis(), bundle().targetAbi());
    if (hasAnyCompiler())
        m_abiWidget->setEnabled(true);
}

bool QnxToolchainConfigWidget::isDirtyImpl() const
{
    return m_sdpPath->filePath() != bundle().get(&QnxToolchain::sdpPath)()
           || m_abiWidget->currentAbi() != bundle().targetAbi();
}

void QnxToolchainConfigWidget::handleSdpPathChange()
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

// QnxToolchainFactory

class QnxToolchainFactory : public ToolchainFactory
{
public:
    QnxToolchainFactory()
    {
        setDisplayName(Tr::tr("QCC"));
        setSupportedToolchainType(Constants::QNX_TOOLCHAIN_ID);
        setSupportedLanguages({ProjectExplorer::Constants::C_LANGUAGE_ID,
                               ProjectExplorer::Constants::CXX_LANGUAGE_ID});
        setToolchainConstructor([] { return new QnxToolchain; });
        setUserCreatable(true);
    }

    Toolchains autoDetect(const ToolchainDetector &detector) const final
    {
        // FIXME: Support detecting toolchains on remote devices
        if (detector.device)
            return {};

        Toolchains tcs = autoDetectHelper(detector.alreadyKnown);
        return tcs;
    }

    std::unique_ptr<ProjectExplorer::ToolchainConfigWidget> createConfigurationWidget(
        const ToolchainBundle &bundle) const override
    {
        return std::make_unique<QnxToolchainConfigWidget>(bundle);
    }
};

void setupQnxToolchain()
{
    static QnxToolchainFactory theQnxToolChainFactory;
}

} // Qnx::Internal
