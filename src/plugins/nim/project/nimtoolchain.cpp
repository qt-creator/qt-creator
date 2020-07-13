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

#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

NimToolChain::NimToolChain()
    : NimToolChain(Constants::C_NIMTOOLCHAIN_TYPEID)
{}

NimToolChain::NimToolChain(Utils::Id typeId)
    : ToolChain(typeId)
    , m_compilerCommand(FilePath())
    , m_version(std::make_tuple(-1,-1,-1))
{
    setLanguage(Constants::C_NIMLANGUAGE_ID);
    setTypeDisplayName(tr("Nim"));
}

Abi NimToolChain::targetAbi() const
{
    return Abi::hostAbi();
}

bool NimToolChain::isValid() const
{
    if (m_compilerCommand.isEmpty())
        return false;
    QFileInfo fi = compilerCommand().toFileInfo();
    return fi.isExecutable();
}

ToolChain::MacroInspectionRunner NimToolChain::createMacroInspectionRunner() const
{
    return ToolChain::MacroInspectionRunner();
}

Macros NimToolChain::predefinedMacros(const QStringList &) const
{
    return Macros();
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

HeaderPaths NimToolChain::builtInHeaderPaths(const QStringList &, const FilePath &,
                                             const Environment &) const
{
    return {};
}

void NimToolChain::addToEnvironment(Environment &env) const
{
    if (isValid())
        env.prependOrSetPath(compilerCommand().parentDir().toString());
}

FilePath NimToolChain::makeCommand(const Environment &env) const
{
    const FilePath tmp = env.searchInPath("make");
    return tmp.isEmpty() ? FilePath::fromString("make") : tmp;
}

FilePath NimToolChain::compilerCommand() const
{
    return m_compilerCommand;
}

void NimToolChain::setCompilerCommand(const FilePath &compilerCommand)
{
    m_compilerCommand = compilerCommand;
    parseVersion(compilerCommand, m_version);
}

QList<Utils::OutputLineParser *> NimToolChain::createOutputParsers() const
{
    return {};
}

std::unique_ptr<ProjectExplorer::ToolChainConfigWidget> NimToolChain::createConfigurationWidget()
{
    return std::make_unique<NimToolChainConfigWidget>(this);
}

QVariantMap NimToolChain::toMap() const
{
    QVariantMap data = ToolChain::toMap();
    data[Constants::C_NIMTOOLCHAIN_COMPILER_COMMAND_KEY] = m_compilerCommand.toString();
    return data;
}

QString NimToolChain::compilerVersion() const
{
    return m_compilerCommand.isEmpty() || m_version == std::make_tuple(-1,-1,-1)
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
    setCompilerCommand(FilePath::fromString(data.value(Constants::C_NIMTOOLCHAIN_COMPILER_COMMAND_KEY).toString()));
    return true;
}

bool NimToolChain::parseVersion(const FilePath &path, std::tuple<int, int, int> &result)
{
    QProcess process;
    process.start(path.toString(), {"--version"});
    if (!process.waitForFinished())
        return false;
    const QString version = QString::fromUtf8(process.readLine());
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

}
