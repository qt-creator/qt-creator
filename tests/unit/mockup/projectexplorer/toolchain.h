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

#include <projectexplorer/headerpath.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/projectmacro.h>
#include <coreplugin/id.h>

#include <functional>

namespace ProjectExplorer {

class ToolChain
{
public:
    Core::Id typeId() const { return Core::Id(); }

    enum CompilerFlag {
        NoFlags = 0,
        StandardCxx11 = 0x1,
        StandardC99 = 0x2,
        StandardC11 = 0x4,
        GnuExtensions = 0x8,
        MicrosoftExtensions = 0x10,
        BorlandExtensions = 0x20,
        OpenMP = 0x40,
        ObjectiveC = 0x80,
        StandardCxx14 = 0x100,
        StandardCxx17 = 0x200,
        StandardCxx98 = 0x400,
    };
    Q_DECLARE_FLAGS(CompilerFlags, CompilerFlag)

    Abi targetAbi() const { return Abi(); }

    using SystemHeaderPathsRunner = std::function<QList<HeaderPath>(const QStringList &cxxflags, const QString &sysRoot)>;
    virtual SystemHeaderPathsRunner createSystemHeaderPathsRunner() const { return SystemHeaderPathsRunner(); }

    using PredefinedMacrosRunner = std::function<Macros(const QStringList &cxxflags)>;
    virtual PredefinedMacrosRunner createPredefinedMacrosRunner() const { return PredefinedMacrosRunner(); }

    virtual QString originalTargetTriple() const { return QString(); }
};

} // namespace ProjectExplorer
