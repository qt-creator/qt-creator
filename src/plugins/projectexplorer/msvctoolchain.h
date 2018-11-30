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

#include "abstractmsvctoolchain.h"
#include "abi.h"
#include "toolchainconfigwidget.h"

#include <QFutureWatcher>

#include <utils/optional.h>

QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QVersionNumber)

namespace Utils { class PathChooser; }

namespace ProjectExplorer {
namespace Internal {

// --------------------------------------------------------------------------
// MsvcToolChain
// --------------------------------------------------------------------------

class MsvcToolChain : public AbstractMsvcToolChain
{
public:
    enum Type { WindowsSDK, VS };
    enum Platform { x86,
                    amd64,
                    x86_amd64,
                    ia64,
                    x86_ia64,
                    arm,
                    x86_arm,
                    amd64_arm,
                    amd64_x86
                  };

    explicit MsvcToolChain(const QString &name, const Abi &abi,
                           const QString &varsBat, const QString &varsBatArg,
                           Core::Id l, Detection d = ManualDetection);
    MsvcToolChain(const MsvcToolChain &other);
    MsvcToolChain();

    Utils::FileNameList suggestedMkspecList() const override;

    QString typeDisplayName() const override;

    QVariantMap toMap() const override;
    bool fromMap(const QVariantMap &data) override;

    std::unique_ptr<ToolChainConfigWidget> createConfigurationWidget() override;

    ToolChain *clone() const override;

    QString varsBatArg() const { return m_varsBatArg; }
    void setVarsBatArg(const QString &varsBA) { m_varsBatArg = varsBA; }

    bool operator == (const ToolChain &) const override;

    static void cancelMsvcToolChainDetection();

protected:
    explicit MsvcToolChain(Core::Id typeId, const QString &name, const Abi &abi,
                           const QString &varsBat, const QString &varsBatArg,
                           Core::Id l, Detection d);
    explicit MsvcToolChain(Core::Id typeId);

    Utils::Environment readEnvironmentSetting(const Utils::Environment& env) const final;
    // Function must be thread-safe!
    Macros msvcPredefinedMacros(const QStringList cxxflags,
                                const Utils::Environment &env) const override;
    LanguageVersion msvcLanguageVersion(const QStringList cxxflags,
                                        const Core::Id &language,
                                        const Macros &macros) const override;

    struct GenerateEnvResult
    {
        Utils::optional<QString> error;
        QList<Utils::EnvironmentItem> environmentItems;
    };
    static void environmentModifications(QFutureInterface<GenerateEnvResult> &future,
                                         QString vcvarsBat, QString varsBatArg);
    void initEnvModWatcher(const QFuture<GenerateEnvResult> &future);

private:
    void updateEnvironmentModifications(QList<Utils::EnvironmentItem> modifications);

    mutable QList<Utils::EnvironmentItem> m_environmentModifications;
    mutable QFutureWatcher<GenerateEnvResult> m_envModWatcher;

    QString m_varsBatArg; // Argument
};

class ClangClToolChain : public MsvcToolChain
{
public:
    ClangClToolChain(const QString &name, const QString &llvmDir,
                     Core::Id language,
                     Detection d);
    ClangClToolChain();

    bool isValid() const override;
    QString typeDisplayName() const override;
    QList<Utils::FileName> suggestedMkspecList() const override;
    void addToEnvironment(Utils::Environment &env) const override;
    Utils::FileName compilerCommand() const override;
    IOutputParser *outputParser() const override;
    ToolChain *clone() const override;
    QVariantMap toMap() const override;
    bool fromMap(const QVariantMap &data) override;
    std::unique_ptr<ToolChainConfigWidget> createConfigurationWidget() override;

    const QList<MsvcToolChain *> &msvcToolchains() const;
    QString clangPath() const { return m_clangPath; }
    void setClangPath(const QString &path) { m_clangPath = path; }

    void resetMsvcToolChain(const MsvcToolChain *base = nullptr);
    Macros msvcPredefinedMacros(const QStringList cxxflags,
                                const Utils::Environment &env) const override;
    LanguageVersion msvcLanguageVersion(const QStringList cxxflags,
                                        const Core::Id &language,
                                        const Macros &macros) const override;

    bool operator ==(const ToolChain &) const override;
private:
    QString m_clangPath;
};

// --------------------------------------------------------------------------
// MsvcToolChainFactory
// --------------------------------------------------------------------------

class MsvcToolChainFactory : public ToolChainFactory
{
    Q_OBJECT

public:
    MsvcToolChainFactory();
    QSet<Core::Id> supportedLanguages() const override;

    QList<ToolChain *> autoDetect(const QList<ToolChain *> &alreadyKnown) override;

    bool canRestore(const QVariantMap &data) override;
    ToolChain *restore(const QVariantMap &data) override;

    static QString vcVarsBatFor(const QString &basePath, MsvcToolChain::Platform platform,
                                const QVersionNumber &v);
};

class ClangClToolChainFactory : public MsvcToolChainFactory
{
    Q_OBJECT

public:
    ClangClToolChainFactory();

    QList<ToolChain *> autoDetect(const QList<ToolChain *> &alreadyKnown) override;

    bool canRestore(const QVariantMap &data) override;
    ToolChain *restore(const QVariantMap &data) override;

    bool canCreate() override;
    ToolChain *create(Core::Id l) override;
};


// --------------------------------------------------------------------------
// MsvcBasedToolChainConfigWidget
// --------------------------------------------------------------------------

class MsvcBasedToolChainConfigWidget : public ToolChainConfigWidget
{
    Q_OBJECT

public:
    explicit MsvcBasedToolChainConfigWidget(ToolChain *);

protected:
    void applyImpl() override { }
    void discardImpl() override { setFromMsvcToolChain(); }
    bool isDirtyImpl() const override { return false; }
    void makeReadOnlyImpl() override { }

    void setFromMsvcToolChain();

private:
    QLabel *m_nameDisplayLabel;
    QLabel *m_varsBatDisplayLabel;
};

// --------------------------------------------------------------------------
// MsvcToolChainConfigWidget
// --------------------------------------------------------------------------

class MsvcToolChainConfigWidget : public MsvcBasedToolChainConfigWidget
{
    Q_OBJECT

public:
    explicit MsvcToolChainConfigWidget(ToolChain *);
};

// --------------------------------------------------------------------------
// ClangClToolChainConfigWidget
// --------------------------------------------------------------------------

class ClangClToolChainConfigWidget : public MsvcBasedToolChainConfigWidget
{
    Q_OBJECT

public:
    explicit ClangClToolChainConfigWidget(ToolChain *);

protected:
    void applyImpl() override;
    void discardImpl() override;

private:
    void setFromClangClToolChain();

    QLabel *m_llvmDirLabel = nullptr;
    Utils::PathChooser *m_compilerCommand = nullptr;
};

} // namespace Internal
} // namespace ProjectExplorer
