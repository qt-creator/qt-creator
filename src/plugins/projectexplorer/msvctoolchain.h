// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "abi.h"
#include "abiwidget.h"
#include "toolchain.h"
#include "toolchainconfigwidget.h"

#include <QFutureWatcher>

#include <utils/environment.h>

#include <optional>

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
    enum Platform { x86, amd64, x86_amd64, ia64, x86_ia64, arm, x86_arm, amd64_arm, amd64_x86,
                    x86_arm64, amd64_arm64, arm64, arm64_x86, arm64_amd64 };

    explicit MsvcToolChain(Utils::Id typeId);
    ~MsvcToolChain() override;

    bool isValid() const override;

    QString originalTargetTriple() const override;

    QStringList suggestedMkspecList() const override;
    Abis supportedAbis() const override;

    void toMap(Utils::Store &data) const override;
    void fromMap(const Utils::Store &data) override;

    std::unique_ptr<ToolChainConfigWidget> createConfigurationWidget() override;
    bool hostPrefersToolchain() const override;

    MacroInspectionRunner createMacroInspectionRunner() const override;
    Utils::LanguageExtensions languageExtensions(const QStringList &cxxflags) const override;
    Utils::WarningFlags warningFlags(const QStringList &cflags) const override;
    Utils::FilePaths includedFiles(const QStringList &flags,
                              const Utils::FilePath &directoryPath) const override;
    BuiltInHeaderPathsRunner createBuiltInHeaderPathsRunner(
            const Utils::Environment &env) const override;
    void addToEnvironment(Utils::Environment &env) const override;

    Utils::FilePath makeCommand(const Utils::Environment &environment) const override;
    QList<Utils::OutputLineParser *> createOutputParsers() const override;

    QString varsBatArg() const { return m_varsBatArg; }
    QString varsBat() const { return m_vcvarsBat; }
    void setupVarsBat(const Abi &abi, const QString &varsBat, const QString &varsBatArg);
    void resetVarsBat();
    Platform platform() const;

    bool operator==(const ToolChain &) const override;

    bool isJobCountSupported() const override { return false; }

    int priority() const override;

    static void cancelMsvcToolChainDetection();
    static std::optional<QString> generateEnvironmentSettings(const Utils::Environment &env,
                                                                const QString &batchFile,
                                                                const QString &batchArgs,
                                                                QMap<QString, QString> &envPairs);
    bool environmentInitialized() const { return !m_environmentModifications.isEmpty(); }

protected:
    class WarningFlagAdder
    {
        int m_warningCode = 0;
        Utils::WarningFlags &m_flags;
        bool m_doesEnable = false;
        bool m_triggered = false;

    public:
        WarningFlagAdder(const QString &flag, Utils::WarningFlags &flags);
        void operator()(int warningCode, Utils::WarningFlags flagsSet);

        bool triggered() const;
    };

    static void inferWarningsForLevel(int warningLevel, Utils::WarningFlags &flags);

    Utils::Environment readEnvironmentSetting(const Utils::Environment &env) const;
    // Function must be thread-safe!
    virtual Macros msvcPredefinedMacros(const QStringList &cxxflags,
                                        const Utils::Environment &env) const;
    virtual Utils::LanguageVersion msvcLanguageVersion(const QStringList &cxxflags,
                                                       const Utils::Id &language,
                                                       const Macros &macros) const;

    struct GenerateEnvResult
    {
        std::optional<QString> error;
        Utils::EnvironmentItems environmentItems;
    };
    static void environmentModifications(QPromise<GenerateEnvResult> &future,
                                         QString vcvarsBat, QString varsBatArg);
    void initEnvModWatcher(const QFuture<GenerateEnvResult> &future);

protected:
    mutable QMutex m_headerPathsMutex;
    mutable QHash<QStringList, HeaderPaths> m_headerPathsPerEnv;

private:
    void updateEnvironmentModifications(Utils::EnvironmentItems modifications);
    void rescanForCompiler();

    mutable Utils::EnvironmentItems m_environmentModifications;
    mutable QFutureWatcher<GenerateEnvResult> m_envModWatcher;

    mutable Utils::Environment m_lastEnvironment;   // Last checked 'incoming' environment.
    mutable Utils::Environment m_resultEnvironment; // Resulting environment for VC

protected:
    QString m_vcvarsBat;
    QString m_varsBatArg; // Argument
};

class PROJECTEXPLORER_EXPORT ClangClToolChain : public MsvcToolChain
{
public:
    ClangClToolChain();

    bool isValid() const override;
    QStringList suggestedMkspecList() const override;
    void addToEnvironment(Utils::Environment &env) const override;
    Utils::FilePath compilerCommand() const override; // FIXME: Remove
    QList<Utils::OutputLineParser *> createOutputParsers() const override;
    void toMap(Utils::Store &data) const override;
    void fromMap(const Utils::Store &data) override;
    std::unique_ptr<ToolChainConfigWidget> createConfigurationWidget() override;
    BuiltInHeaderPathsRunner createBuiltInHeaderPathsRunner(
            const Utils::Environment &env) const override;

    const QList<MsvcToolChain *> &msvcToolchains() const;
    Utils::FilePath clangPath() const { return m_clangPath; }
    void setClangPath(const Utils::FilePath &path) { m_clangPath = path; }

    Macros msvcPredefinedMacros(const QStringList &cxxflags,
                                const Utils::Environment &env) const override;
    Utils::LanguageVersion msvcLanguageVersion(const QStringList &cxxflags,
                                               const Utils::Id &language,
                                               const Macros &macros) const override;

    bool operator==(const ToolChain &) const override;

    int priority() const override;

private:
    Utils::FilePath m_clangPath;
};

// --------------------------------------------------------------------------
// MsvcToolChainFactory
// --------------------------------------------------------------------------

class MsvcToolChainFactory : public ToolChainFactory
{
public:
    MsvcToolChainFactory();

    Toolchains autoDetect(const ToolchainDetector &detector) const final;

    bool canCreate() const final;

    static QString vcVarsBatFor(const QString &basePath,
                                MsvcToolChain::Platform platform,
                                const QVersionNumber &v);
};

class ClangClToolChainFactory : public ToolChainFactory
{
public:
    ClangClToolChainFactory();

    Toolchains autoDetect(const ToolchainDetector &detector) const final;

    bool canCreate() const final;
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

private:
    void applyImpl() override;
    void discardImpl() override;
    bool isDirtyImpl() const override;
    void makeReadOnlyImpl() override;

    void setFromMsvcToolChain();

    void updateAbis();
    void handleVcVarsChange(const QString &vcVars);
    void handleVcVarsArchChange(const QString &arch);

    QString vcVarsArguments() const;

    QComboBox *m_varsBatPathCombo;
    QComboBox *m_varsBatArchCombo;
    QLineEdit *m_varsBatArgumentsEdit;
    AbiWidget *m_abiWidget;
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
    bool isDirtyImpl() const override { return false; }
    void makeReadOnlyImpl() override;

private:
    void setFromClangClToolChain();

    QLabel *m_llvmDirLabel = nullptr;
    QComboBox *m_varsBatDisplayCombo = nullptr;
    Utils::PathChooser *m_compilerCommand = nullptr;
};

} // namespace Internal
} // namespace ProjectExplorer
