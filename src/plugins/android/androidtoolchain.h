/**************************************************************************
**
** Copyright (C) 2015 BogDan Vatra <bog_dan_ro@yahoo.com>
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef ANDROIDTOOLCHAIN_H
#define ANDROIDTOOLCHAIN_H

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
    QList<Utils::FileName> suggestedMkspecList() const override;
    QString makeCommand(const Utils::Environment &environment) const override;

    QString ndkToolChainVersion() const;

    bool isSecondaryToolChain() const;
    void setSecondaryToolChain(bool b);

protected:
    QList<ProjectExplorer::Abi> detectSupportedAbis() const override;

private:
    explicit AndroidToolChain(const ProjectExplorer::Abi &abi, const QString &ndkToolChainVersion, Detection d);
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

    QList<ProjectExplorer::ToolChain *> autoDetect() override;
    bool canRestore(const QVariantMap &data) override;
    ProjectExplorer::ToolChain *restore(const QVariantMap &data) override;

    class AndroidToolChainInformation
    {
    public:
        Utils::FileName compilerCommand;
        ProjectExplorer::Abi abi;
        QString version;
    };

    static QList<ProjectExplorer::ToolChain *> createToolChainsForNdk(const Utils::FileName &ndkPath);
    static QList<AndroidToolChainInformation> toolchainPathsForNdk(const Utils::FileName &ndkPath);

    static QList<int> versionNumberFromString(const QString &version);
    static bool versionCompareLess(const QList<int> &a, const QList<int> &b);
    static bool versionCompareLess(AndroidToolChain *atc, AndroidToolChain *btc);
    static QList<int> newestToolChainVersionForArch(const ProjectExplorer::Abi &abi);
private:
    static QHash<ProjectExplorer::Abi, QList<int> > m_newestVersionForAbi;
    static Utils::FileName m_ndkLocation;
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDTOOLCHAIN_H
