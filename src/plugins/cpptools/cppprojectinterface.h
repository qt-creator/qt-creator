/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <projectexplorer/toolchain.h>

#include <memory>

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

namespace Core {
class Id;
}

namespace ProjectExplorer {
class Project;
class ToolChain;
}

namespace CppTools {

class ToolChainInterface
{
public:
    virtual ~ToolChainInterface() {}

    virtual Core::Id type() const = 0;
    virtual bool isMsvc2015Toolchain() const = 0;

    virtual unsigned wordWidth() const = 0;
    virtual QString targetTriple() const = 0;

    virtual QByteArray predefinedMacros() const = 0;
    virtual QList<ProjectExplorer::HeaderPath> systemHeaderPaths() const = 0;

    virtual ProjectExplorer::WarningFlags warningFlags() const = 0;
    virtual ProjectExplorer::ToolChain::CompilerFlags compilerFlags() const = 0;
};

using ToolChainInterfacePtr = std::unique_ptr<ToolChainInterface>;

class ProjectInterface
{
public:
    virtual ~ProjectInterface() {}

    virtual QString displayName() const = 0;
    virtual QString projectFilePath() const = 0;

    virtual ToolChainInterfacePtr toolChain(Core::Id language,
                                            const QStringList &commandLineFlags) const = 0;
};

} // namespace CppTools
