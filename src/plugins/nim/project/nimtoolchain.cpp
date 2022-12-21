// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimtoolchain.h"

#include "nimconstants.h"
#include "nimtoolchainfactory.h"
#include "nimtr.h"

#include <projectexplorer/abi.h>
#include <utils/environment.h>
#include <utils/qtcprocess.h>

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

std::unique_ptr<ProjectExplorer::ToolChainConfigWidget> NimToolChain::createConfigurationWidget()
{
    return std::make_unique<NimToolChainConfigWidget>(this);
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

bool NimToolChain::fromMap(const QVariantMap &data)
{
    if (!ToolChain::fromMap(data))
        return false;
    parseVersion(compilerCommand(), m_version);
    return true;
}

bool NimToolChain::parseVersion(const FilePath &path, std::tuple<int, int, int> &result)
{
    QtcProcess process;
    process.setCommand({path, {"--version"}});
    process.start();
    if (!process.waitForFinished())
        return false;
    const QString version = QString::fromUtf8(process.readAllStandardOutput()).section('\n', 0, 0);
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

} // Nim
