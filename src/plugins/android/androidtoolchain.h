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

#include <qtsupport/baseqtversion.h>

#include <projectexplorer/gcctoolchain.h>

namespace Android {
namespace Internal {

using ToolChainList = QList<ProjectExplorer::ToolChain *>;

class AndroidToolChain : public ProjectExplorer::ClangToolChain
{
    Q_DECLARE_TR_FUNCTIONS(Android::Internal::AndroidToolChain)

public:
    ~AndroidToolChain() override;

    bool isValid() const override;
    void addToEnvironment(Utils::Environment &env) const override;

    QStringList suggestedMkspecList() const override;
    Utils::FilePath makeCommand(const Utils::Environment &environment) const override;
    bool fromMap(const QVariantMap &data) override;

    void setNdkLocation(const Utils::FilePath &ndkLocation);
    Utils::FilePath ndkLocation() const;

protected:
    DetectedAbisResult detectSupportedAbis() const override;

private:
    explicit AndroidToolChain();

    friend class AndroidToolChainFactory;

    mutable Utils::FilePath m_ndkLocation;
};

class AndroidToolChainFactory : public ProjectExplorer::ToolChainFactory
{
public:
    AndroidToolChainFactory();

    class AndroidToolChainInformation
    {
    public:
        Utils::Id language;
        Utils::FilePath compilerCommand;
        ProjectExplorer::Abi abi;
        QString version;
    };

    static ToolChainList autodetectToolChains(const ToolChainList &alreadyKnown);
    static ToolChainList autodetectToolChainsFromNdks(const ToolChainList &alreadyKnown,
                                                      const QList<Utils::FilePath> &ndkLocations,
                                                      const bool isCustom = false);
};

} // namespace Internal
} // namespace Android
