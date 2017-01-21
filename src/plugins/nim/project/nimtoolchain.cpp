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

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

NimToolChain::NimToolChain(ToolChain::Detection d)
    : NimToolChain(Constants::C_NIMTOOLCHAIN_TYPEID, d)
{}

NimToolChain::NimToolChain(Core::Id typeId, ToolChain::Detection d)
    : ToolChain(typeId, d)
    , m_compilerCommand(FileName())
    , m_version(std::make_tuple(-1,-1,-1))
{
    setLanguage(Constants::C_NIMLANGUAGE_ID);
}

NimToolChain::NimToolChain(const NimToolChain &other)
    : ToolChain(other.typeId(), other.detection())
    , m_compilerCommand(other.m_compilerCommand)
    , m_version(other.m_version)
{
    setLanguage(Constants::C_NIMLANGUAGE_ID);
}

QString NimToolChain::typeDisplayName() const
{
    return NimToolChainFactory::tr("Nim");
}

Abi NimToolChain::targetAbi() const
{
    return Abi();
}

bool NimToolChain::isValid() const
{
    if (m_compilerCommand.isNull())
        return false;
    QFileInfo fi = compilerCommand().toFileInfo();
    return fi.isExecutable();
}

QByteArray NimToolChain::predefinedMacros(const QStringList &) const
{
    return QByteArray();
}

ToolChain::CompilerFlags NimToolChain::compilerFlags(const QStringList &) const
{
    return CompilerFlag::NoFlags;
}

WarningFlags NimToolChain::warningFlags(const QStringList &) const
{
    return WarningFlags::NoWarnings;
}

QList<HeaderPath> NimToolChain::systemHeaderPaths(const QStringList &, const FileName &) const
{
    return QList<HeaderPath>();
}

void NimToolChain::addToEnvironment(Environment &env) const
{
    if (isValid())
        env.prependOrSetPath(compilerCommand().parentDir().toString());
}

QString NimToolChain::makeCommand(const Environment &env) const
{
    QString make = "make";
    FileName tmp = env.searchInPath(make);
    return tmp.isEmpty() ? make : tmp.toString();
}

FileName NimToolChain::compilerCommand() const
{
    return m_compilerCommand;
}

void NimToolChain::setCompilerCommand(const FileName &compilerCommand)
{
    m_compilerCommand = compilerCommand;
    parseVersion(compilerCommand, m_version);
}

IOutputParser *NimToolChain::outputParser() const
{
    return nullptr;
}

ToolChainConfigWidget *NimToolChain::configurationWidget()
{
    return new NimToolChainConfigWidget(this);
}

ToolChain *NimToolChain::clone() const
{
    return new NimToolChain(*this);
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
    setCompilerCommand(FileName::fromString(data.value(Constants::C_NIMTOOLCHAIN_COMPILER_COMMAND_KEY).toString()));
    return true;
}

bool NimToolChain::parseVersion(const FileName &path, std::tuple<int, int, int> &result)
{
    QProcess process;
    process.setReadChannel(QProcess::StandardError);
    process.start(path.toString(), {"--version"});
    if (!process.waitForFinished())
        return false;
    const QString version = QString::fromUtf8(process.readLine());
    if (version.isEmpty())
        return false;
    const QRegExp regex("(\\d+)\\.(\\d+)\\.(\\d+)");
    if (regex.indexIn(version) == -1)
        return false;
    const QStringList text = regex.capturedTexts();
    if (text.length() != 4)
        return false;
    result = std::make_tuple(text[1].toInt(), text[2].toInt(), text[3].toInt());
    return true;
}

}
