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

namespace Internal { class ToolchainPrivate; }

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

class GccToolchain;
class ToolchainConfigWidget;
class Kit;

namespace Internal { class ToolchainSettingsAccessor; }

class PROJECTEXPLORER_EXPORT ToolchainDescription
{
public:
    Utils::FilePath compilerPath;
    Utils::Id language;
};

// --------------------------------------------------------------------------
// Toolchain (documentation inside)
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT Toolchain : public Utils::AspectContainer
{
public:
    enum Detection {
        ManualDetection,
        AutoDetection,
        AutoDetectionFromSdk,
        UninitializedDetection,
    };

    using Predicate = std::function<bool(const Toolchain *)>;

    virtual ~Toolchain();

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

    using MacrosCache = std::shared_ptr<Cache<QStringList, Toolchain::MacroInspectionReport, 64>>;
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

    virtual bool operator ==(const Toolchain &) const;

    virtual std::unique_ptr<ToolchainConfigWidget> createConfigurationWidget() = 0;
    Toolchain *clone() const;

    // Used by the toolchainmanager to save user-generated tool chains.
    // Make sure to call this function when deriving!
    void toMap(Utils::Store &map) const override;
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
    virtual GccToolchain *asGccToolchain() { return nullptr; }

protected:
    explicit Toolchain(Utils::Id typeId);

    void setTypeDisplayName(const QString &typeName);

    void setTargetAbiNoSignal(const Abi &abi);
    void setTargetAbiKey(const Utils::Key &abiKey);

    void setCompilerCommandKey(const Utils::Key &commandKey);

    const MacrosCache &predefinedMacrosCache() const;
    const HeaderPathsCache &headerPathsCache() const;

    void toolChainUpdated();

    // Make sure to call this function when deriving!
    void fromMap(const Utils::Store &data) override;

    void reportError();
    bool hasError() const;

    enum class PossiblyConcatenatedFlag { No, Yes };
    static Utils::FilePaths includedFiles(const QString &option,
                                          const QStringList &flags,
                                          const Utils::FilePath &directoryPath,
                                          PossiblyConcatenatedFlag possiblyConcatenated);

private:
    Toolchain(const Toolchain &) = delete;
    Toolchain &operator=(const Toolchain &) = delete;

    const std::unique_ptr<Internal::ToolchainPrivate> d;

    friend class Internal::ToolchainSettingsAccessor;
    friend class ToolchainFactory;
};

using Toolchains = QList<Toolchain *>;

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

class PROJECTEXPLORER_EXPORT AsyncToolchainDetector
{
public:
    AsyncToolchainDetector(
        const ToolchainDetector &detector,
        const std::function<Toolchains(const ToolchainDetector &)> &func,
        const std::function<bool(const Toolchain *, const Toolchains &)> &alreadyRegistered);
    void run();
private:
    ToolchainDetector m_detector;
    std::function<Toolchains(const ToolchainDetector &)> m_func;
    std::function<bool(Toolchain *, const Toolchains &)> m_alreadyRegistered;
};

class PROJECTEXPLORER_EXPORT ToolchainFactory
{
    ToolchainFactory(const ToolchainFactory &) = delete;
    ToolchainFactory &operator=(const ToolchainFactory &) = delete;

public:
    ToolchainFactory();
    virtual ~ToolchainFactory();

    static const QList<ToolchainFactory *> allToolchainFactories();

    QString displayName() const { return m_displayName; }
    Utils::Id supportedToolchainType() const;

    virtual std::optional<AsyncToolchainDetector> asyncAutoDetector(
        const ToolchainDetector &detector) const;
    virtual Toolchains autoDetect(const ToolchainDetector &detector) const;
    virtual Toolchains detectForImport(const ToolchainDescription &tcd) const;

    virtual bool canCreate() const;
    Toolchain *create() const;

    Toolchain *restore(const Utils::Store &data);

    static QByteArray idFromMap(const Utils::Store &data);
    static Utils::Id typeIdFromMap(const Utils::Store &data);
    static void autoDetectionToMap(Utils::Store &data, bool detected);

    static Toolchain *createToolchain(Utils::Id toolchainType);

    QList<Utils::Id> supportedLanguages() const;

    void setUserCreatable(bool userCreatable);

protected:
    void setDisplayName(const QString &name) { m_displayName = name; }
    void setSupportedToolchainType(const Utils::Id &supportedToolchainType);
    void setSupportedLanguages(const QList<Utils::Id> &supportedLanguages);
    void setSupportsAllLanguages(bool supportsAllLanguages);
    using ToolchainConstructor = std::function<Toolchain *()>;
    void setToolchainConstructor(const ToolchainConstructor &constructor);
    ToolchainConstructor toolchainConstructor() const;

    class Candidate {
    public:
        Utils::FilePath compilerPath;
        QString compilerVersion;

        bool operator==(const ToolchainFactory::Candidate &other) const {
            return compilerPath == other.compilerPath
                    && compilerVersion == other.compilerVersion;
        }
    };

    using Candidates = QVector<Candidate>;

private:
    QString m_displayName;
    Utils::Id m_supportedToolchainType;
    QList<Utils::Id> m_supportedLanguages;
    bool m_supportsAllLanguages = false;
    bool m_userCreatable = false;
    ToolchainConstructor m_toolchainConstructor;
};

} // namespace ProjectExplorer
