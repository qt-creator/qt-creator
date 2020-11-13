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
    Q_DECLARE_TR_FUNCTIONS(Nim::NimToolChain)

public:
    NimToolChain();
    explicit NimToolChain(Utils::Id typeId);

    MacroInspectionRunner createMacroInspectionRunner() const override;
    Utils::LanguageExtensions languageExtensions(const QStringList &flags) const final;
    Utils::WarningFlags warningFlags(const QStringList &flags) const final;

    BuiltInHeaderPathsRunner createBuiltInHeaderPathsRunner(
            const Utils::Environment &) const override;
    void addToEnvironment(Utils::Environment &env) const final;
    Utils::FilePath makeCommand(const Utils::Environment &env) const final;
    QString compilerVersion() const;
    QList<Utils::OutputLineParser *> createOutputParsers() const final;
    std::unique_ptr<ProjectExplorer::ToolChainConfigWidget> createConfigurationWidget() final;

    bool fromMap(const QVariantMap &data) final;

    static bool parseVersion(const Utils::FilePath &path, std::tuple<int, int, int> &version);

private:
    std::tuple<int, int, int> m_version;
};

} // Nim
