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

#pragma once

#include <projectexplorer/toolchain.h>
#include <projectexplorer/headerpath.h>

namespace Nim {

class NimToolChain : public ProjectExplorer::ToolChain
{
public:
    NimToolChain(Detection d);
    NimToolChain(Core::Id typeId, Detection d);

    QString typeDisplayName() const override;
    ProjectExplorer::Abi targetAbi() const override;
    bool isValid() const override;

    PredefinedMacrosRunner createPredefinedMacrosRunner() const override;
    QByteArray predefinedMacros(const QStringList &flags) const final;
    CompilerFlags compilerFlags(const QStringList &flags) const final;
    ProjectExplorer::WarningFlags warningFlags(const QStringList &flags) const final;

    SystemHeaderPathsRunner createSystemHeaderPathsRunner() const override;
    QList<ProjectExplorer::HeaderPath> systemHeaderPaths(const QStringList &flags,
                                                         const Utils::FileName &sysRoot) const final;
    void addToEnvironment(Utils::Environment &env) const final;
    QString makeCommand(const Utils::Environment &env) const final;
    Utils::FileName compilerCommand() const final;
    QString compilerVersion() const;
    void setCompilerCommand(const Utils::FileName &compilerCommand);
    ProjectExplorer::IOutputParser *outputParser() const final;
    ProjectExplorer::ToolChainConfigWidget *configurationWidget() final;
    ProjectExplorer::ToolChain *clone() const final;

    QVariantMap toMap() const final;
    bool fromMap(const QVariantMap &data) final;

    static bool parseVersion(const Utils::FileName &path, std::tuple<int, int, int> &version);

private:
    NimToolChain(const NimToolChain &other);

    Utils::FileName m_compilerCommand;
    std::tuple<int, int, int> m_version;
};

}
