/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

#include "nimtoolchain.h"
#include "nimconstants.h"
#include "nimtoolchainfactory.h"

#include <projectexplorer/abi.h>
#include <utils/environment.h>
#include <utils/qtcprocess.h>

#include <QFileInfo>
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
    setTypeDisplayName(tr("Nim"));
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
