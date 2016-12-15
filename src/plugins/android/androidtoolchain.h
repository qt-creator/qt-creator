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
#include <projectexplorer/toolchainconfigwidget.h>

namespace Android {
namespace Internal {

class AndroidToolChain : public ProjectExplorer::GccToolChain
{
public:
    ~AndroidToolChain() override;

    QString typeDisplayName() const override;

    bool isValid() const override;

    void addToEnvironment(Utils::Environment &env) const override;

    bool operator ==(const ProjectExplorer::ToolChain &) const override;

    ProjectExplorer::ToolChainConfigWidget *configurationWidget() override;
    virtual Utils::FileName suggestedDebugger() const override;
    Utils::FileName suggestedGdbServer() const;

    QVariantMap toMap() const override;
    bool fromMap(const QVariantMap &data) override;
    Utils::FileNameList suggestedMkspecList() const override;
    QString makeCommand(const Utils::Environment &environment) const override;

    QString ndkToolChainVersion() const;

    bool isSecondaryToolChain() const;
    void setSecondaryToolChain(bool b);

protected:
    DetectedAbisResult detectSupportedAbis() const override;

private:
    explicit AndroidToolChain(const ProjectExplorer::Abi &abi, const QString &ndkToolChainVersion,
                              Core::Id l, Detection d);
    AndroidToolChain();
    AndroidToolChain(const AndroidToolChain &);

    QString m_ndkToolChainVersion;
    bool m_secondaryToolChain;

    friend class AndroidToolChainFactory;
};


class AndroidToolChainConfigWidget : public ProjectExplorer::ToolChainConfigWidget
{
    Q_OBJECT

public:
    AndroidToolChainConfigWidget(AndroidToolChain *);

private:
    void applyImpl() override {}
    void discardImpl() override {}
    bool isDirtyImpl() const override { return false; }
    void makeReadOnlyImpl() override {}
};


class AndroidToolChainFactory : public ProjectExplorer::ToolChainFactory
{
    Q_OBJECT

public:
    AndroidToolChainFactory();
    QSet<Core::Id> supportedLanguages() const override;

    QList<ProjectExplorer::ToolChain *> autoDetect(const QList<ProjectExplorer::ToolChain *> &alreadyKnown) override;
    bool canRestore(const QVariantMap &data) override;
    ProjectExplorer::ToolChain *restore(const QVariantMap &data) override;

    class AndroidToolChainInformation
    {
    public:
        Core::Id language;
        Utils::FileName compilerCommand;
        ProjectExplorer::Abi abi;
        QString version;
    };

    static QList<ProjectExplorer::ToolChain *>
    autodetectToolChainsForNdk(const Utils::FileName &ndkPath,
                               const QList<ProjectExplorer::ToolChain *> &alreadyKnown);
    static QList<AndroidToolChainInformation> toolchainPathsForNdk(const Utils::FileName &ndkPath);

    static QList<int> versionNumberFromString(const QString &version);
    static bool versionCompareLess(const QList<int> &a, const QList<int> &b);
    static bool versionCompareLess(QList<AndroidToolChain *> atc,
                                   QList<AndroidToolChain *> btc);
    static QList<int> newestToolChainVersionForArch(const ProjectExplorer::Abi &abi);
private:
    static QHash<ProjectExplorer::Abi, QList<int> > m_newestVersionForAbi;
    static Utils::FileName m_ndkLocation;
};

} // namespace Internal
} // namespace Android
