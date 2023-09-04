// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qtsupport/baseqtversion.h>

#include <projectexplorer/gcctoolchain.h>

namespace Android {
namespace Internal {

using ToolChainList = QList<ProjectExplorer::ToolChain *>;

class AndroidToolChain : public ProjectExplorer::GccToolChain
{
public:
    ~AndroidToolChain() override;

    bool isValid() const override;
    void addToEnvironment(Utils::Environment &env) const override;

    QStringList suggestedMkspecList() const override;
    Utils::FilePath makeCommand(const Utils::Environment &environment) const override;
    void fromMap(const Utils::Store &data) override;

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
