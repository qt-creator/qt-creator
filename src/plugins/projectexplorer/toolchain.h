// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include "abi.h"
#include "devicesupport/idevicefwd.h"
#include "headerpath.h"
#include "projectmacro.h"
#include "task.h"
#include "toolchaincache.h"

#include <utils/aspects.h>
#include <utils/cpplanguage_details.h>
#include <utils/environment.h>
#include <utils/store.h>

#include <QDateTime>

#include <functional>
#include <memory>

namespace Utils { class OutputLineParser; }

namespace ProjectExplorer {

namespace Internal { class ToolChainPrivate; }

namespace Deprecated {
// Deprecated in 4.3:
namespace Toolchain {
enum Language {
    None = 0,
    C,
    Cxx
};
QString languageId(Language l);
} // namespace Toolchain
} // namespace Deprecated

class GccToolChain;
class ToolChainConfigWidget;
class ToolChainFactory;
class Kit;

namespace Internal { class ToolChainSettingsAccessor; }

class PROJECTEXPLORER_EXPORT ToolChainDescription
{
public:
    Utils::FilePath compilerPath;
    Utils::Id language;
};

// --------------------------------------------------------------------------
// ToolChain (documentation inside)
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT ToolChain : public Utils::AspectContainer
{
public:
    enum Detection {
        ManualDetection,
        AutoDetection,
        AutoDetectionFromSdk,
        UninitializedDetection,
    };

    using Predicate = std::function<bool(const ToolChain *)>;

    virtual ~ToolChain();

    QString displayName() const;
    void setDisplayName(const QString &name);

    bool isAutoDetected() const;
    bool isSdkProvided() const { return detection() == AutoDetectionFromSdk; }
    Detection detection() const;
    QString detectionSource() const;

    QByteArray id() const;

    virtual QStringList suggestedMkspecList() const;

    Utils::Id typeId() const;
    QString typeDisplayName() const;

    Abi targetAbi() const;
    void setTargetAbi(const Abi &abi);

    virtual ProjectExplorer::Abis supportedAbis() const;
    virtual QString originalTargetTriple() const { return {}; }
    virtual QStringList extraCodeModelFlags() const { return {}; }
    virtual Utils::FilePath installDir() const { return {}; }
    virtual bool hostPrefersToolchain() const { return true; }

    virtual bool isValid() const;

    virtual Utils::LanguageExtensions languageExtensions(const QStringList &cxxflags) const = 0;
    virtual Utils::WarningFlags warningFlags(const QStringList &cflags) const = 0;
    virtual Utils::FilePaths includedFiles(const QStringList &flags, const Utils::FilePath &directory) const;
    virtual QString sysRoot() const;

    QString explicitCodeModelTargetTriple() const;
    QString effectiveCodeModelTargetTriple() const;
    void setExplicitCodeModelTargetTriple(const QString &triple);

    class MacroInspectionReport
    {
    public:
        Macros macros;
        Utils::LanguageVersion languageVersion;
    };

    using MacrosCache = std::shared_ptr<Cache<QStringList, ToolChain::MacroInspectionReport, 64>>;
    using HeaderPathsCache = std::shared_ptr<Cache<QPair<Utils::Environment, QStringList>, HeaderPaths>>;

    // A MacroInspectionRunner is created in the ui thread and runs in another thread.
    using MacroInspectionRunner = std::function<MacroInspectionReport(const QStringList &cxxflags)>;
    virtual MacroInspectionRunner createMacroInspectionRunner() const = 0;

    // A BuiltInHeaderPathsRunner is created in the ui thread and runs in another thread.
    using BuiltInHeaderPathsRunner = std::function<HeaderPaths(
        const QStringList &cxxflags, const Utils::FilePath &sysRoot, const QString &originalTargetTriple)>;
    virtual BuiltInHeaderPathsRunner createBuiltInHeaderPathsRunner(const Utils::Environment &env) const = 0;
    virtual void addToEnvironment(Utils::Environment &env) const = 0;
    virtual Utils::FilePath makeCommand(const Utils::Environment &env) const = 0;

    Utils::Id language() const;

    virtual Utils::FilePath compilerCommand() const; // FIXME: De-virtualize.
    void setCompilerCommand(const Utils::FilePath &command);
    virtual bool matchesCompilerCommand(const Utils::FilePath &command) const;

    virtual QList<Utils::OutputLineParser *> createOutputParsers() const = 0;

    virtual bool operator ==(const ToolChain &) const;

    virtual std::unique_ptr<ToolChainConfigWidget> createConfigurationWidget() = 0;
    ToolChain *clone() const;

    // Used by the toolchainmanager to save user-generated tool chains.
    // Make sure to call this function when deriving!
    virtual void toMap(Utils::Store &map) const;
    virtual Tasks validateKit(const Kit *k) const;

    virtual bool isJobCountSupported() const { return true; }

    void setLanguage(Utils::Id language);
    void setDetection(Detection d);
    void setDetectionSource(const QString &source);

    static Utils::LanguageVersion cxxLanguageVersion(const QByteArray &cplusplusMacroValue);
    static Utils::LanguageVersion languageVersion(const Utils::Id &language, const Macros &macros);

    enum Priority {
        PriorityLow = 0,
        PriorityNormal = 10,
        PriorityHigh = 20,
    };

    virtual int priority() const { return PriorityNormal; }
    virtual GccToolChain *asGccToolChain() { return nullptr; }

protected:
    explicit ToolChain(Utils::Id typeId);

    void setTypeDisplayName(const QString &typeName);

    void setTargetAbiNoSignal(const Abi &abi);
    void setTargetAbiKey(const Utils::Key &abiKey);

    void setCompilerCommandKey(const Utils::Key &commandKey);

    const MacrosCache &predefinedMacrosCache() const;
    const HeaderPathsCache &headerPathsCache() const;

    void toolChainUpdated();

    // Make sure to call this function when deriving!
    virtual void fromMap(const Utils::Store &data);

    void reportError();
    bool hasError() const;

    enum class PossiblyConcatenatedFlag { No, Yes };
    static Utils::FilePaths includedFiles(const QString &option,
                                          const QStringList &flags,
                                          const Utils::FilePath &directoryPath,
                                          PossiblyConcatenatedFlag possiblyConcatenated);

private:
    ToolChain(const ToolChain &) = delete;
    ToolChain &operator=(const ToolChain &) = delete;

    const std::unique_ptr<Internal::ToolChainPrivate> d;

    friend class Internal::ToolChainSettingsAccessor;
    friend class ToolChainFactory;
};

using Toolchains = QList<ToolChain *>;

class PROJECTEXPLORER_EXPORT BadToolchain
{
public:
    BadToolchain(const Utils::FilePath &filePath);
    BadToolchain(const Utils::FilePath &filePath, const Utils::FilePath &symlinkTarget,
                 const QDateTime &timestamp);

    Utils::Store toMap() const;
    static BadToolchain fromMap(const Utils::Store &map);

    Utils::FilePath filePath;
    Utils::FilePath symlinkTarget;
    QDateTime timestamp;
};

class PROJECTEXPLORER_EXPORT BadToolchains
{
public:
    BadToolchains(const QList<BadToolchain> &toolchains = {});
    bool isBadToolchain(const Utils::FilePath &toolchain) const;

    QVariant toVariant() const;
    static BadToolchains fromVariant(const QVariant &v);

    QList<BadToolchain> toolchains;
};

class PROJECTEXPLORER_EXPORT ToolchainDetector
{
public:
    ToolchainDetector(const Toolchains &alreadyKnown,
                      const IDeviceConstPtr &device,
                      const Utils::FilePaths &searchPaths);

    const Toolchains alreadyKnown;
    const IDeviceConstPtr device;
    const Utils::FilePaths searchPaths; // If empty use device path and/or magic.
};

class PROJECTEXPLORER_EXPORT ToolChainFactory
{
    ToolChainFactory(const ToolChainFactory &) = delete;
    ToolChainFactory &operator=(const ToolChainFactory &) = delete;

public:
    ToolChainFactory();
    virtual ~ToolChainFactory();

    static const QList<ToolChainFactory *> allToolChainFactories();

    QString displayName() const { return m_displayName; }
    Utils::Id supportedToolChainType() const;

    virtual Toolchains autoDetect(const ToolchainDetector &detector) const;
    virtual Toolchains detectForImport(const ToolChainDescription &tcd) const;

    virtual bool canCreate() const;
    ToolChain *create() const;

    ToolChain *restore(const Utils::Store &data);

    static QByteArray idFromMap(const Utils::Store &data);
    static Utils::Id typeIdFromMap(const Utils::Store &data);
    static void autoDetectionToMap(Utils::Store &data, bool detected);

    static ToolChain *createToolChain(Utils::Id toolChainType);

    QList<Utils::Id> supportedLanguages() const;

    void setUserCreatable(bool userCreatable);

protected:
    void setDisplayName(const QString &name) { m_displayName = name; }
    void setSupportedToolChainType(const Utils::Id &supportedToolChainType);
    void setSupportedLanguages(const QList<Utils::Id> &supportedLanguages);
    void setSupportsAllLanguages(bool supportsAllLanguages);
    using ToolChainConstructor = std::function<ToolChain *()>;
    void setToolchainConstructor(const ToolChainConstructor &constructor);
    ToolChainConstructor toolchainConstructor() const;

    class Candidate {
    public:
        Utils::FilePath compilerPath;
        QString compilerVersion;

        bool operator==(const ToolChainFactory::Candidate &other) const {
            return compilerPath == other.compilerPath
                    && compilerVersion == other.compilerVersion;
        }
    };

    using Candidates = QVector<Candidate>;

private:
    QString m_displayName;
    Utils::Id m_supportedToolChainType;
    QList<Utils::Id> m_supportedLanguages;
    bool m_supportsAllLanguages = false;
    bool m_userCreatable = false;
    ToolChainConstructor m_toolchainConstructor;
};

} // namespace ProjectExplorer
