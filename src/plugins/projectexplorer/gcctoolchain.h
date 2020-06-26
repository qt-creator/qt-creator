/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "projectexplorer_export.h"

#include "projectexplorerconstants.h"
#include "toolchain.h"
#include "abi.h"
#include "headerpath.h"

#include <utils/fileutils.h>

#include <functional>
#include <memory>

namespace ProjectExplorer {

namespace Internal {
class ClangToolChainFactory;
class ClangToolChainConfigWidget;
class GccToolChainConfigWidget;
class GccToolChainFactory;
class MingwToolChainFactory;
class LinuxIccToolChainFactory;
}

// --------------------------------------------------------------------------
// GccToolChain
// --------------------------------------------------------------------------

inline const QStringList languageOption(Utils::Id languageId)
{
    if (languageId == Constants::C_LANGUAGE_ID)
        return {"-x", "c"};
    return {"-x", "c++"};
}

inline const QStringList gccPredefinedMacrosOptions(Utils::Id languageId)
{
    return languageOption(languageId) + QStringList({"-E", "-dM"});
}

class PROJECTEXPLORER_EXPORT GccToolChain : public ToolChain
{
    Q_DECLARE_TR_FUNCTIONS(ProjectExplorer::GccToolChain)

public:
    GccToolChain(Utils::Id typeId);

    Abi targetAbi() const override;
    QString originalTargetTriple() const override;
    Utils::FilePath installDir() const override;
    QString version() const;
    Abis supportedAbis() const override;
    void setTargetAbi(const Abi &);

    bool isValid() const override;

    Utils::LanguageExtensions languageExtensions(const QStringList &cxxflags) const override;
    Utils::WarningFlags warningFlags(const QStringList &cflags) const override;

    MacroInspectionRunner createMacroInspectionRunner() const override;
    Macros predefinedMacros(const QStringList &cxxflags) const override;

    BuiltInHeaderPathsRunner createBuiltInHeaderPathsRunner(const Utils::Environment &env) const override;
    HeaderPaths builtInHeaderPaths(const QStringList &flags,
                                   const Utils::FilePath &sysRootPath,
                                   const Utils::Environment &env) const override;

    void addToEnvironment(Utils::Environment &env) const override;
    Utils::FilePath makeCommand(const Utils::Environment &environment) const override;
    QStringList suggestedMkspecList() const override;
    QList<Utils::OutputLineParser *> createOutputParsers() const override;

    QVariantMap toMap() const override;
    bool fromMap(const QVariantMap &data) override;

    std::unique_ptr<ToolChainConfigWidget> createConfigurationWidget() override;

    bool operator ==(const ToolChain &) const override;

    void resetToolChain(const Utils::FilePath &);
    Utils::FilePath compilerCommand() const override;
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

protected:
    using CacheItem = QPair<QStringList, Macros>;
    using GccCache = QVector<CacheItem>;

    void setCompilerCommand(const Utils::FilePath &path);
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
                                          const QString &sysRoot,
                                          const QString &originalTargetTriple);

    static HeaderPaths gccHeaderPaths(const Utils::FilePath &gcc, const QStringList &args,
                                      const QStringList &env);

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

private:
    void updateSupportedAbis() const;
    static QStringList gccPrepareArguments(const QStringList &flags,
                                           const QString &sysRoot,
                                           const QStringList &platformCodeGenFlags,
                                           Utils::Id languageId,
                                           OptionsReinterpreter reinterpretOptions);

protected:
    Utils::FilePath m_compilerCommand;
    QStringList m_platformCodeGenFlags;
    QStringList m_platformLinkerFlags;

    OptionsReinterpreter m_optionsReinterpreter = [](const QStringList &v) { return v; };
    mutable ExtraHeaderPathsFunction m_extraHeaderPathsFunction = [](HeaderPaths &) {};

private:
    Abi m_targetAbi;
    mutable Abis m_supportedAbis;
    mutable QString m_originalTargetTriple;
    mutable HeaderPaths m_headerPaths;
    mutable QString m_version;
    mutable Utils::FilePath m_installDir;

    friend class Internal::GccToolChainConfigWidget;
    friend class Internal::GccToolChainFactory;
    friend class ToolChainFactory;
};

// --------------------------------------------------------------------------
// ClangToolChain
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT ClangToolChain : public GccToolChain
{
    Q_DECLARE_TR_FUNCTIONS(ProjectExplorer::ClangToolChain)

public:
    ClangToolChain();
    explicit ClangToolChain(Utils::Id typeId);
    ~ClangToolChain() override;

    Utils::FilePath makeCommand(const Utils::Environment &environment) const override;

    Utils::LanguageExtensions languageExtensions(const QStringList &cxxflags) const override;
    Utils::WarningFlags warningFlags(const QStringList &cflags) const override;

    QList<Utils::OutputLineParser *> createOutputParsers() const override;

    QStringList suggestedMkspecList() const override;
    void addToEnvironment(Utils::Environment &env) const override;

    QString originalTargetTriple() const override;
    QString sysRoot() const override;

    BuiltInHeaderPathsRunner createBuiltInHeaderPathsRunner(
            const Utils::Environment &env) const override;

    std::unique_ptr<ToolChainConfigWidget> createConfigurationWidget() override;

    QVariantMap toMap() const override;
    bool fromMap(const QVariantMap &data) override;

protected:
    Utils::LanguageExtensions defaultLanguageExtensions() const override;
    void syncAutodetectedWithParentToolchains();

private:
    QByteArray m_parentToolChainId;
    QMetaObject::Connection m_mingwToolchainAddedConnection;
    QMetaObject::Connection m_thisToolchainRemovedConnection;

    friend class Internal::ClangToolChainFactory;
    friend class Internal::ClangToolChainConfigWidget;
    friend class ToolChainFactory;
};

// --------------------------------------------------------------------------
// MingwToolChain
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT MingwToolChain : public GccToolChain
{
    Q_DECLARE_TR_FUNCTIONS(ProjectExplorer::MingwToolChain)

public:
    Utils::FilePath makeCommand(const Utils::Environment &environment) const override;

    QStringList suggestedMkspecList() const override;

private:
    MingwToolChain();

    friend class Internal::MingwToolChainFactory;
    friend class ToolChainFactory;
};

// --------------------------------------------------------------------------
// LinuxIccToolChain
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT LinuxIccToolChain : public GccToolChain
{
    Q_DECLARE_TR_FUNCTIONS(ProjectExplorer::LinuxIccToolChain)

public:
    Utils::LanguageExtensions languageExtensions(const QStringList &cxxflags) const override;
    QList<Utils::OutputLineParser *> createOutputParsers() const override;

    QStringList suggestedMkspecList() const override;

private:
    LinuxIccToolChain();

    friend class Internal::LinuxIccToolChainFactory;
    friend class ToolChainFactory;
};

} // namespace ProjectExplorer
