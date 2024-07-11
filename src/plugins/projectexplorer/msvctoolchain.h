// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "abi.h"
#include "toolchain.h"
#include "toolchainconfigwidget.h"

#include <QFutureWatcher>

#include <utils/environment.h>

#include <optional>

namespace ProjectExplorer::Internal {

// --------------------------------------------------------------------------
// MsvcToolChain
// --------------------------------------------------------------------------

class MsvcToolchain : public Toolchain
{
public:
    enum Type { WindowsSDK, VS };
    enum Platform { x86, amd64, x86_amd64, ia64, x86_ia64, arm, x86_arm, amd64_arm, amd64_x86,
                    x86_arm64, amd64_arm64, arm64, arm64_x86, arm64_amd64 };

    explicit MsvcToolchain(Utils::Id typeId);
    ~MsvcToolchain() override;

    bool isValid() const override;

    QString originalTargetTriple() const override;

    QStringList suggestedMkspecList() const override;
    Abis supportedAbis() const override;

    void toMap(Utils::Store &data) const override;
    void fromMap(const Utils::Store &data) override;

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

    bool operator==(const Toolchain &) const override;

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
    bool canShareBundleImpl(const Toolchain &other) const override;

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

class PROJECTEXPLORER_EXPORT ClangClToolchain : public MsvcToolchain
{
public:
    ClangClToolchain();

    bool isValid() const override;
    QStringList suggestedMkspecList() const override;
    void addToEnvironment(Utils::Environment &env) const override;
    Utils::FilePath compilerCommand() const override; // FIXME: Remove
    void setCompilerCommand(const Utils::FilePath &cmd) override { setClangPath(cmd); }
    QList<Utils::OutputLineParser *> createOutputParsers() const override;
    void toMap(Utils::Store &data) const override;
    void fromMap(const Utils::Store &data) override;
    BuiltInHeaderPathsRunner createBuiltInHeaderPathsRunner(
            const Utils::Environment &env) const override;

    const QList<MsvcToolchain *> &msvcToolchains() const;
    Utils::FilePath clangPath() const { return m_clangPath; }
    void setClangPath(const Utils::FilePath &path) { m_clangPath = path; }

    Macros msvcPredefinedMacros(const QStringList &cxxflags,
                                const Utils::Environment &env) const override;
    Utils::LanguageVersion msvcLanguageVersion(const QStringList &cxxflags,
                                               const Utils::Id &language,
                                               const Macros &macros) const override;

    bool operator==(const Toolchain &) const override;

    int priority() const override;

private:
    bool canShareBundleImpl(const Toolchain &other) const override;

    Utils::FilePath m_clangPath;
};

void setupMsvcToolchain();
void setupClangClToolchain();

} // namespace ProjectExplorer::Internal
