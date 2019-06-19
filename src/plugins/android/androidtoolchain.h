/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include <projectexplorer/gcctoolchain.h>

namespace Android {
namespace Internal {

using ToolChainList = QList<ProjectExplorer::ToolChain *>;

class AndroidToolChain : public ProjectExplorer::ClangToolChain
{
public:
    ~AndroidToolChain() override;

    bool isValid() const override;
    void addToEnvironment(Utils::Environment &env) const override;

    QStringList suggestedMkspecList() const override;
    Utils::FilePath makeCommand(const Utils::Environment &environment) const override;
    bool fromMap(const QVariantMap &data) override;

protected:
    DetectedAbisResult detectSupportedAbis() const override;

private:
    explicit AndroidToolChain();

    friend class AndroidToolChainFactory;
};

class AndroidToolChainFactory : public ProjectExplorer::ToolChainFactory
{
    Q_OBJECT

public:
    AndroidToolChainFactory();

    ToolChainList autoDetect(const ToolChainList &alreadyKnown) override;

    class AndroidToolChainInformation
    {
    public:
        Core::Id language;
        Utils::FilePath compilerCommand;
        ProjectExplorer::Abi abi;
        QString version;
    };

    static ToolChainList autodetectToolChainsForNdk(const ToolChainList &alreadyKnown);
};

} // namespace Internal
} // namespace Android
