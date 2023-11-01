// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimtoolchain.h"

#include "nimconstants.h"
#include "nimtoolchain.h"
#include "nimtr.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/toolchainconfigwidget.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/pathchooser.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QFormLayout>
#include <QRegularExpression>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

NimToolChain::NimToolChain()
    : NimToolChain(Constants::C_NIMTOOLCHAIN_TYPEID)
{}

NimToolChain::NimToolChain(Utils::Id typeId)
    : ToolChain(typeId)
    , m_version(std::make_tuple(-1,-1,-1))
{
    setLanguage(Constants::C_NIMLANGUAGE_ID);
    setTypeDisplayName(Tr::tr("Nim"));
    setTargetAbiNoSignal(Abi::hostAbi());
    setCompilerCommandKey("Nim.NimToolChain.CompilerCommand");
}

ToolChain::MacroInspectionRunner NimToolChain::createMacroInspectionRunner() const
{
    return ToolChain::MacroInspectionRunner();
}

LanguageExtensions NimToolChain::languageExtensions(const QStringList &) const
{
    return LanguageExtension::None;
}

WarningFlags NimToolChain::warningFlags(const QStringList &) const
{
    return WarningFlags::NoWarnings;
}

ToolChain::BuiltInHeaderPathsRunner NimToolChain::createBuiltInHeaderPathsRunner(
        const Environment &) const
{
    return ToolChain::BuiltInHeaderPathsRunner();
}

void NimToolChain::addToEnvironment(Environment &env) const
{
    if (isValid())
        env.prependOrSetPath(compilerCommand().parentDir());
}

FilePath NimToolChain::makeCommand(const Environment &env) const
{
    const FilePath tmp = env.searchInPath("make");
    return tmp.isEmpty() ? FilePath("make") : tmp;
}

QList<Utils::OutputLineParser *> NimToolChain::createOutputParsers() const
{
    return {};
}

QString NimToolChain::compilerVersion() const
{
    return compilerCommand().isEmpty() || m_version == std::make_tuple(-1,-1,-1)
            ? QString()
            : QString::asprintf("%d.%d.%d",
                                std::get<0>(m_version),
                                std::get<1>(m_version),
                                std::get<2>(m_version));
}

void NimToolChain::fromMap(const Store &data)
{
    ToolChain::fromMap(data);
    if (hasError())
        return;
    parseVersion(compilerCommand(), m_version);
}

bool NimToolChain::parseVersion(const FilePath &path, std::tuple<int, int, int> &result)
{
    Process process;
    process.setCommand({path, {"--version"}});
    process.start();
    if (!process.waitForFinished())
        return false;
    const QString version = process.readAllStandardOutput().section('\n', 0, 0);
    if (version.isEmpty())
        return false;
    const QRegularExpression regex("(\\d+)\\.(\\d+)\\.(\\d+)");
    const QRegularExpressionMatch match = regex.match(version);
    if (!match.hasMatch())
        return false;
    const QStringList text = match.capturedTexts();
    if (text.length() != 4)
        return false;
    result = std::make_tuple(text[1].toInt(), text[2].toInt(), text[3].toInt());
    return true;
}

// NimToolChainConfigWidget

class NimToolChainConfigWidget : public ToolChainConfigWidget
{
public:
    explicit NimToolChainConfigWidget(NimToolChain *tc)
        : ToolChainConfigWidget(tc)
        , m_compilerCommand(new PathChooser)
        , m_compilerVersion(new QLineEdit)
    {
        // Create ui
        const auto gnuVersionArgs = QStringList("--version");
        m_compilerCommand->setExpectedKind(PathChooser::ExistingCommand);
        m_compilerCommand->setCommandVersionArguments(gnuVersionArgs);
        m_mainLayout->addRow(Tr::tr("&Compiler path:"), m_compilerCommand);
        m_compilerVersion->setReadOnly(true);
        m_mainLayout->addRow(Tr::tr("&Compiler version:"), m_compilerVersion);

        // Fill
        fillUI();

        // Connect
        connect(m_compilerCommand, &PathChooser::validChanged, this, [this] {
            const FilePath path = m_compilerCommand->rawFilePath();
            auto tc = static_cast<NimToolChain *>(toolChain());
            QTC_ASSERT(tc, return);
            tc->setCompilerCommand(path);
            fillUI();
        });
    }

protected:
    void applyImpl() final;
    void discardImpl() final;
    bool isDirtyImpl() const final;
    void makeReadOnlyImpl() final;

private:
    void fillUI();

    Utils::PathChooser *m_compilerCommand;
    QLineEdit *m_compilerVersion;
};

void NimToolChainConfigWidget::applyImpl()
{
    auto tc = static_cast<NimToolChain *>(toolChain());
    Q_ASSERT(tc);
    if (tc->isAutoDetected())
        return;
    tc->setCompilerCommand(m_compilerCommand->filePath());
}

void NimToolChainConfigWidget::discardImpl()
{
    fillUI();
}

bool NimToolChainConfigWidget::isDirtyImpl() const
{
    auto tc = static_cast<NimToolChain *>(toolChain());
    Q_ASSERT(tc);
    return tc->compilerCommand() != m_compilerCommand->filePath();
}

void NimToolChainConfigWidget::makeReadOnlyImpl()
{
    m_compilerCommand->setReadOnly(true);
}

void NimToolChainConfigWidget::fillUI()
{
    auto tc = static_cast<NimToolChain *>(toolChain());
    Q_ASSERT(tc);
    m_compilerCommand->setFilePath(tc->compilerCommand());
    m_compilerVersion->setText(tc->compilerVersion());
}

std::unique_ptr<ToolChainConfigWidget> NimToolChain::createConfigurationWidget()
{
    return std::make_unique<NimToolChainConfigWidget>(this);
}

// NimToolChainFactory

NimToolChainFactory::NimToolChainFactory()
{
    setDisplayName(Tr::tr("Nim"));
    setSupportedToolChainType(Constants::C_NIMTOOLCHAIN_TYPEID);
    setSupportedLanguages({Constants::C_NIMLANGUAGE_ID});
    setToolchainConstructor([] { return new NimToolChain; });
    setUserCreatable(true);
}

Toolchains NimToolChainFactory::autoDetect(const ToolchainDetector &detector) const
{
    Toolchains result;

    const FilePath compilerPath = detector.device->searchExecutableInPath("nim");
    if (compilerPath.isEmpty())
        return result;

    result = Utils::filtered(detector.alreadyKnown, [compilerPath](ToolChain *tc) {
        return tc->typeId() == Constants::C_NIMTOOLCHAIN_TYPEID
                && tc->compilerCommand() == compilerPath;
    });

    if (!result.empty())
        return result;

    auto tc = new NimToolChain;
    tc->setDetection(ToolChain::AutoDetection);
    tc->setCompilerCommand(compilerPath);
    result.append(tc);
    return result;
}

Toolchains NimToolChainFactory::detectForImport(const ToolChainDescription &tcd) const
{
    Toolchains result;
    if (tcd.language == Constants::C_NIMLANGUAGE_ID) {
        auto tc = new NimToolChain;
        tc->setDetection(ToolChain::ManualDetection); // FIXME: sure?
        tc->setCompilerCommand(tcd.compilerPath);
        result.append(tc);
    }
    return result;
}

} // Nim
