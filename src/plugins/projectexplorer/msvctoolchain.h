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

#include "abi.h"
#include "toolchain.h"
#include "toolchaincache.h"
#include "toolchainconfigwidget.h"

#include <QFutureWatcher>

#include <utils/environment.h>
#include <utils/optional.h>

QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QVersionNumber)

namespace Utils {
class PathChooser;
}

namespace ProjectExplorer {
namespace Internal {

// --------------------------------------------------------------------------
// MsvcToolChain
// --------------------------------------------------------------------------

class MsvcToolChain : public ToolChain
{
public:
    enum Type { WindowsSDK, VS };
    enum Platform { x86, amd64, x86_amd64, ia64, x86_ia64, arm, x86_arm, amd64_arm, amd64_x86 };

    explicit MsvcToolChain(const QString &name,
                           const Abi &abi,
                           const QString &varsBat,
                           const QString &varsBatArg,
                           Core::Id l,
                           Detection d = ManualDetection);
    MsvcToolChain(const MsvcToolChain &other);
    MsvcToolChain();
    ~MsvcToolChain() override;

    Abi targetAbi() const override;

    bool isValid() const override;

    QString originalTargetTriple() const override;

    Utils::FileNameList suggestedMkspecList() const override;

    QString typeDisplayName() const override;

    QVariantMap toMap() const override;
    bool fromMap(const QVariantMap &data) override;

    std::unique_ptr<ToolChainConfigWidget> createConfigurationWidget() override;

    bool canClone() const override;
    ToolChain *clone() const override;

    MacroInspectionRunner createMacroInspectionRunner() const override;
    Macros predefinedMacros(const QStringList &cxxflags) const override;
    Utils::LanguageExtensions languageExtensions(const QStringList &cxxflags) const override;
    WarningFlags warningFlags(const QStringList &cflags) const override;
    BuiltInHeaderPathsRunner createBuiltInHeaderPathsRunner() const override;
    HeaderPaths builtInHeaderPaths(const QStringList &cxxflags,
                                   const Utils::FileName &sysRoot) const override;
    void addToEnvironment(Utils::Environment &env) const override;

    QString makeCommand(const Utils::Environment &environment) const override;
    Utils::FileName compilerCommand() const override;
    IOutputParser *outputParser() const override;

    QString varsBatArg() const { return m_varsBatArg; }
    QString varsBat() const { return m_vcvarsBat; }
    void setVarsBatArg(const QString &varsBA) { m_varsBatArg = varsBA; }

    bool operator==(const ToolChain &) const override;

    static void cancelMsvcToolChainDetection();
    static Utils::optional<QString> generateEnvironmentSettings(const Utils::Environment &env,
                                                                const QString &batchFile,
                                                                const QString &batchArgs,
                                                                QMap<QString, QString> &envPairs);

protected:
    class WarningFlagAdder
    {
        int m_warningCode = 0;
        WarningFlags &m_flags;
        bool m_doesEnable = false;
        bool m_triggered = false;

    public:
        WarningFlagAdder(const QString &flag, WarningFlags &flags);
        void operator()(int warningCode, WarningFlags flagsSet);

        bool triggered() const;
    };

    explicit MsvcToolChain(Core::Id typeId,
                           const QString &name,
                           const Abi &abi,
                           const QString &varsBat,
                           const QString &varsBatArg,
                           Core::Id l,
                           Detection d);
    explicit MsvcToolChain(Core::Id typeId);

    static void inferWarningsForLevel(int warningLevel, WarningFlags &flags);
    void toolChainUpdated() override;

    Utils::Environment readEnvironmentSetting(const Utils::Environment &env) const;
    // Function must be thread-safe!
    virtual Macros msvcPredefinedMacros(const QStringList &cxxflags,
                                        const Utils::Environment &env) const;
    virtual Utils::LanguageVersion msvcLanguageVersion(const QStringList &cxxflags,
                                                       const Core::Id &language,
                                                       const Macros &macros) const;

    struct GenerateEnvResult
    {
        Utils::optional<QString> error;
        QList<Utils::EnvironmentItem> environmentItems;
    };
    static void environmentModifications(QFutureInterface<GenerateEnvResult> &future,
                                         QString vcvarsBat,
                                         QString varsBatArg);
    void initEnvModWatcher(const QFuture<GenerateEnvResult> &future);

protected:
    mutable QMutex *m_headerPathsMutex = nullptr;
    mutable HeaderPaths m_headerPaths;

private:
    void updateEnvironmentModifications(QList<Utils::EnvironmentItem> modifications);

    mutable QList<Utils::EnvironmentItem> m_environmentModifications;
    mutable QFutureWatcher<GenerateEnvResult> m_envModWatcher;

    Utils::FileName m_debuggerCommand;

    mutable std::shared_ptr<Cache<MacroInspectionReport, 64>> m_predefinedMacrosCache;

    mutable Utils::Environment m_lastEnvironment;   // Last checked 'incoming' environment.
    mutable Utils::Environment m_resultEnvironment; // Resulting environment for VC

protected:
    Abi m_abi;

    QString m_vcvarsBat;
    QString m_varsBatArg; // Argument
};

class ClangClToolChain : public MsvcToolChain
{
public:
    ClangClToolChain(const QString &name, const QString &llvmDir, Core::Id language, Detection d);
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
    BuiltInHeaderPathsRunner createBuiltInHeaderPathsRunner() const override;

    const QList<MsvcToolChain *> &msvcToolchains() const;
    QString clangPath() const { return m_clangPath; }
    void setClangPath(const QString &path) { m_clangPath = path; }

    void resetMsvcToolChain(const MsvcToolChain *base = nullptr);
    Macros msvcPredefinedMacros(const QStringList &cxxflags,
                                const Utils::Environment &env) const override;
    Utils::LanguageVersion msvcLanguageVersion(const QStringList &cxxflags,
                                               const Core::Id &language,
                                               const Macros &macros) const override;

    bool operator==(const ToolChain &) const override;

private:
    void toolChainUpdated() override;

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

    static QString vcVarsBatFor(const QString &basePath,
                                MsvcToolChain::Platform platform,
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
    void applyImpl() override {}
    void discardImpl() override { setFromMsvcToolChain(); }
    bool isDirtyImpl() const override { return false; }
    void makeReadOnlyImpl() override {}

    void setFromMsvcToolChain();

protected:
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
    void makeReadOnlyImpl() override;

private:
    void setFromClangClToolChain();

    QLabel *m_llvmDirLabel = nullptr;
    QComboBox *m_varsBatDisplayCombo = nullptr;
    Utils::PathChooser *m_compilerCommand = nullptr;
};

} // namespace Internal
} // namespace ProjectExplorer
