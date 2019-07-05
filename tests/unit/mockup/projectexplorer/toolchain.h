/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
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

#include <projectexplorer/abi.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/projectmacro.h>
#include <coreplugin/id.h>
#include <utils/cpplanguage_details.h>

#include <functional>

namespace Utils { class Environment; }

namespace ProjectExplorer {

class ToolChain
{
public:
    ToolChain() = default;
    Core::Id typeId() const { return Core::Id(); }

    Abi targetAbi() const { return Abi(); }

    using BuiltInHeaderPathsRunner = std::function<HeaderPaths(
        const QStringList &cxxflags, const QString &sysRoot, const QString &originalTargetTriple)>;
    virtual BuiltInHeaderPathsRunner createBuiltInHeaderPathsRunner(const Utils::Environment &env) const {
        return BuiltInHeaderPathsRunner();
    }

    class MacroInspectionReport
    {
    public:
        Macros macros;
        Utils::LanguageVersion languageVersion; // Derived from macros.
    };
    using MacroInspectionRunner = std::function<MacroInspectionReport(const QStringList &cxxflags)>;
    virtual MacroInspectionRunner createMacroInspectionRunner() const = 0;

    virtual QString originalTargetTriple() const { return QString(); }
    virtual QStringList extraCodeModelFlags() const { return QStringList(); }
};

class ConcreteToolChain : public ToolChain
{
public:
    MacroInspectionRunner createMacroInspectionRunner() const override
    {
        return MacroInspectionRunner();
    }
};

} // namespace ProjectExplorer
