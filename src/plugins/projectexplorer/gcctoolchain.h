// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include "toolchain.h"
#include "abi.h"
#include "headerpath.h"

#include <functional>
#include <memory>
#include <optional>

namespace ProjectExplorer {

namespace Internal {
class GccToolChainConfigWidget;
class GccToolChainFactory;

const QStringList gccPredefinedMacrosOptions(Utils::Id languageId);
}

// --------------------------------------------------------------------------
// GccToolChain
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT GccToolChain : public ToolChain
{
public:
    enum SubType { RealGcc, Clang, MinGW, LinuxIcc };

    GccToolChain(Utils::Id typeId, SubType subType = RealGcc);
    ~GccToolChain() override;

    QString originalTargetTriple() const override;
    Utils::FilePath installDir() const override;
    QString version() const;
    Abis supportedAbis() const override;

    Utils::LanguageExtensions languageExtensions(const QStringList &cxxflags) const override;
    Utils::WarningFlags warningFlags(const QStringList &cflags) const override;
    Utils::FilePaths includedFiles(const QStringList &flags,
                                   const Utils::FilePath &directoryPath) const override;

    MacroInspectionRunner createMacroInspectionRunner() const override;
    BuiltInHeaderPathsRunner createBuiltInHeaderPathsRunner(const Utils::Environment &env) const override;

    void addToEnvironment(Utils::Environment &env) const override;
    Utils::FilePath makeCommand(const Utils::Environment &environment) const override;
    QStringList suggestedMkspecList() const override;
    QList<Utils::OutputLineParser *> createOutputParsers() const override;

    void toMap(Utils::Store &data) const override;
    void fromMap(const Utils::Store &data) override;

    std::unique_ptr<ToolChainConfigWidget> createConfigurationWidget() override;

    bool operator ==(const ToolChain &) const override;

    void resetToolChain(const Utils::FilePath &);
    void setPlatformCodeGenFlags(const QStringList &);
    QStringList extraCodeModelFlags() const override;
    QStringList platformCodeGenFlags() const;
    void setPlatformLinkerFlags(const QStringList &);
    QStringList platformLinkerFlags() const;

    static void addCommandPathToEnvironment(const Utils::FilePath &command, Utils::Environment &env);

    class DetectedAbisResult {
    public:
        DetectedAbisResult() = default;
        DetectedAbisResult(const Abis &supportedAbis, const QString &originalTargetTriple = {}) :
            supportedAbis(supportedAbis),
            originalTargetTriple(originalTargetTriple)
        { }

        Abis supportedAbis;
        QString originalTargetTriple;
    };
    GccToolChain *asGccToolChain() final { return this; }

    bool matchesCompilerCommand(const Utils::FilePath &command) const override;

    void setPriority(int priority) { m_priority = priority; }

protected:
    using CacheItem = QPair<QStringList, Macros>;
    using GccCache = QVector<CacheItem>;

    void setSupportedAbis(const Abis &abis);
    void setOriginalTargetTriple(const QString &targetTriple);
    void setInstallDir(const Utils::FilePath &installDir);
    void setMacroCache(const QStringList &allCxxflags, const Macros &macroCache) const;
    Macros macroCache(const QStringList &allCxxflags) const;

    virtual QString defaultDisplayName() const;
    virtual Utils::LanguageExtensions defaultLanguageExtensions() const;

    virtual DetectedAbisResult detectSupportedAbis() const;
    virtual QString detectVersion() const;
    virtual Utils::FilePath detectInstallDir() const;

    // Reinterpret options for compiler drivers inheriting from GccToolChain (e.g qcc) to apply -Wp option
    // that passes the initial options directly down to the gcc compiler
    using OptionsReinterpreter = std::function<QStringList(const QStringList &options)>;
    void setOptionsReinterpreter(const OptionsReinterpreter &optionsReinterpreter);

    using ExtraHeaderPathsFunction = std::function<void(HeaderPaths &)>;
    void initExtraHeaderPathsFunction(ExtraHeaderPathsFunction &&extraHeaderPathsFunction) const;

    static HeaderPaths builtInHeaderPaths(const Utils::Environment &env,
                                          const Utils::FilePath &compilerCommand,
                                          const QStringList &platformCodeGenFlags,
                                          OptionsReinterpreter reinterpretOptions,
                                          HeaderPathsCache headerCache,
                                          Utils::Id languageId,
                                          ExtraHeaderPathsFunction extraHeaderPathsFunction,
                                          const QStringList &flags,
                                          const Utils::FilePath &sysRoot,
                                          const QString &originalTargetTriple);

    static HeaderPaths gccHeaderPaths(const Utils::FilePath &gcc,
                                      const QStringList &args,
                                      const Utils::Environment &env);

    int priority() const override { return m_priority; }

    class WarningFlagAdder
    {
    public:
        WarningFlagAdder(const QString &flag, Utils::WarningFlags &flags);
        void operator ()(const char name[], Utils::WarningFlags flagsSet);

        bool triggered() const;
    private:
        QByteArray m_flagUtf8;
        Utils::WarningFlags &m_flags;
        bool m_doesEnable = false;
        bool m_triggered = false;
    };

    QString sysRoot() const override;

private:
    void syncAutodetectedWithParentToolchains();
    void updateSupportedAbis() const;

protected:
    QStringList m_platformCodeGenFlags;
    QStringList m_platformLinkerFlags;

    OptionsReinterpreter m_optionsReinterpreter = [](const QStringList &v) { return v; };
    mutable ExtraHeaderPathsFunction m_extraHeaderPathsFunction = [](HeaderPaths &) {};

private:
    SubType m_subType = RealGcc;
    mutable Abis m_supportedAbis;
    mutable QString m_originalTargetTriple;
    mutable HeaderPaths m_headerPaths;
    mutable QString m_version;
    mutable Utils::FilePath m_installDir;

    friend class Internal::GccToolChainConfigWidget;
    friend class Internal::GccToolChainFactory;
    friend class ToolChainFactory;

    // "resolved" on macOS from /usr/bin/clang(++) etc to <DeveloperDir>/usr/bin/clang(++)
    // which is used for comparison with matchesCompilerCommand
    mutable std::optional<Utils::FilePath> m_resolvedCompilerCommand;
    QByteArray m_parentToolChainId;
    int m_priority = PriorityNormal;
    QMetaObject::Connection m_mingwToolchainAddedConnection;
    QMetaObject::Connection m_thisToolchainRemovedConnection;
};


namespace Internal {

class GccToolChainFactory : public ToolChainFactory
{
public:
    explicit GccToolChainFactory(GccToolChain::SubType subType);

    Toolchains autoDetect(const ToolchainDetector &detector) const final;
    Toolchains detectForImport(const ToolChainDescription &tcd) const final;

private:
    static Toolchains autoDetectToolchains(const Utils::FilePaths &compilerPaths,
                                           const Utils::Id language,
                                           const Utils::Id requiredTypeId,
                                           const Toolchains &known,
                                           const GccToolChain::SubType subType);
    static Toolchains autoDetectToolChain(const ToolChainDescription &tcd,
                                          const GccToolChain::SubType subType);
    static Toolchains autoDetectSdkClangToolchain(const Toolchains &known);

    const bool m_autoDetecting;
};

} // namespace Internal
} // namespace ProjectExplorer
