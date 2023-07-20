// Copyright (C) 2019 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sdcctoolchain.h"

#include "baremetalconstants.h"
#include "baremetaltr.h"
#include "sdccparser.h"

#include <projectexplorer/abiwidget.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmacro.h>
#include <projectexplorer/toolchainconfigwidget.h>
#include <projectexplorer/toolchainmanager.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/pathchooser.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSettings>
#include <QTemporaryFile>
#include <QTextStream>

using namespace ProjectExplorer;
using namespace Utils;

namespace BareMetal::Internal {

static QString compilerTargetFlag(const Abi &abi)
{
    switch (abi.architecture()) {
    case Abi::Architecture::Mcs51Architecture:
        return QString("-mmcs51");
    case Abi::Architecture::Stm8Architecture:
        return QString("-mstm8");
    default:
        return {};
    }
}

static Macros dumpPredefinedMacros(const FilePath &compiler, const Environment &env,
                                   const Abi &abi)
{
    if (compiler.isEmpty() || !compiler.toFileInfo().isExecutable())
        return {};

    QTemporaryFile fakeIn("XXXXXX.c");
    if (!fakeIn.open())
        return {};
    fakeIn.close();

    Process cpp;
    cpp.setEnvironment(env);
    cpp.setTimeoutS(10);
    cpp.setCommand({compiler, {compilerTargetFlag(abi),  "-dM", "-E", fakeIn.fileName()}});

    cpp.runBlocking();
    if (cpp.result() != ProcessResult::FinishedWithSuccess) {
        qWarning() << cpp.exitMessage();
        return {};
    }

    const QByteArray output = cpp.allOutput().toUtf8();
    return Macro::toMacros(output);
}

static HeaderPaths dumpHeaderPaths(const FilePath &compiler, const Environment &env,
                                   const Abi &abi)
{
    if (!compiler.exists())
        return {};

    Process cpp;
    cpp.setEnvironment(env);
    cpp.setTimeoutS(10);
    cpp.setCommand({compiler, {compilerTargetFlag(abi), "--print-search-dirs"}});

    cpp.runBlocking();
    if (cpp.result() != ProcessResult::FinishedWithSuccess) {
        qWarning() << cpp.exitMessage();
        return {};
    }

    QString output = cpp.allOutput();
    HeaderPaths headerPaths;
    QTextStream in(&output);
    QString line;
    bool synchronized = false;
    while (in.readLineInto(&line)) {
        if (!synchronized) {
            if (line.startsWith("includedir:"))
                synchronized = true;
        } else {
            if (line.startsWith("programs:") || line.startsWith("datadir:")
                    || line.startsWith("libdir:") || line.startsWith("libpath:")) {
                break;
            } else {
                const QString headerPath = QFileInfo(line.trimmed())
                        .canonicalFilePath();
                headerPaths.append(HeaderPath::makeBuiltIn(headerPath));
            }
        }
    }
    return headerPaths;
}

static QString findMacroValue(const Macros &macros, const QByteArray &key)
{
    for (const Macro &macro : macros) {
        if (macro.key == key)
            return QString::fromLocal8Bit(macro.value);
    }
    return {};
}

static QString guessVersion(const Macros &macros)
{
    const QString major = findMacroValue(macros, "__SDCC_VERSION_MAJOR");
    const QString minor = findMacroValue(macros, "__SDCC_VERSION_MINOR");
    const QString patch = findMacroValue(macros, "__SDCC_VERSION_PATCH");
    return QString("%1.%2.%3").arg(major, minor, patch);
}

static Abi::Architecture guessArchitecture(const Macros &macros)
{
    for (const Macro &macro : macros) {
        if (macro.key == "__SDCC_mcs51")
            return Abi::Architecture::Mcs51Architecture;
        if (macro.key == "__SDCC_stm8")
            return Abi::Architecture::Stm8Architecture;
    }
    return Abi::Architecture::UnknownArchitecture;
}

static unsigned char guessWordWidth(const Macros &macros)
{
    Q_UNUSED(macros)
    // SDCC always have 16-bit word width.
    return 16;
}

static Abi::BinaryFormat guessFormat(Abi::Architecture arch)
{
    Q_UNUSED(arch)
    return Abi::BinaryFormat::UnknownFormat;
}

static Abi guessAbi(const Macros &macros)
{
    const Abi::Architecture arch = guessArchitecture(macros);
    return {arch, Abi::OS::BareMetalOS, Abi::OSFlavor::GenericFlavor,
            guessFormat(arch), guessWordWidth(macros)};
}

static QString buildDisplayName(Abi::Architecture arch, Id language, const QString &version)
{
    const QString archName = Abi::toString(arch);
    const QString langName = ToolChainManager::displayNameOfLanguageId(language);
    return Tr::tr("SDCC %1 (%2, %3)").arg(version, langName, archName);
}

static FilePath compilerPathFromEnvironment(const QString &compilerName)
{
    const Environment systemEnvironment = Environment::systemEnvironment();
    return systemEnvironment.searchInPath(compilerName);
}

// SdccToolChainConfigWidget

class SdccToolChain;

class SdccToolChainConfigWidget final : public ToolChainConfigWidget
{
public:
    explicit SdccToolChainConfigWidget(SdccToolChain *tc);

private:
    void applyImpl() final;
    void discardImpl() final { setFromToolchain(); }
    bool isDirtyImpl() const final;
    void makeReadOnlyImpl() final;

    void setFromToolchain();
    void handleCompilerCommandChange();

    PathChooser *m_compilerCommand = nullptr;
    AbiWidget *m_abiWidget = nullptr;
    Macros m_macros;
};

// SdccToolChain

class SdccToolChain final : public ToolChain
{
public:
    SdccToolChain() : ToolChain(Constants::SDCC_TOOLCHAIN_TYPEID)
    {
        setTypeDisplayName(Tr::tr("SDCC"));
        setTargetAbiKey("TargetAbi");
        setCompilerCommandKey("CompilerPath");
    }

    MacroInspectionRunner createMacroInspectionRunner() const final;

    LanguageExtensions languageExtensions(const QStringList &cxxflags) const final;
    WarningFlags warningFlags(const QStringList &cxxflags) const final;

    BuiltInHeaderPathsRunner createBuiltInHeaderPathsRunner(const Environment &) const final;
    void addToEnvironment(Environment &env) const final;
    QList<OutputLineParser *> createOutputParsers() const final  { return {new SdccParser}; }

    std::unique_ptr<ToolChainConfigWidget> createConfigurationWidget() final;

    bool operator==(const ToolChain &other) const final;

    FilePath makeCommand(const Environment &) const final { return {}; }

private:
    friend class SdccToolChainFactory;
    friend class SdccToolChainConfigWidget;
};

ToolChain::MacroInspectionRunner SdccToolChain::createMacroInspectionRunner() const
{
    Environment env = Environment::systemEnvironment();
    addToEnvironment(env);

    const FilePath compiler = compilerCommand();
    const Id lang = language();
    const Abi abi = targetAbi();

    MacrosCache macrosCache = predefinedMacrosCache();

    return [env, compiler, macrosCache, lang, abi]
            (const QStringList &flags) {
        Q_UNUSED(flags)

        const Macros macros = dumpPredefinedMacros(compiler, env, abi);
        const auto report = MacroInspectionReport{macros, languageVersion(lang, macros)};
        macrosCache->insert({}, report);

        return report;
    };
}

LanguageExtensions SdccToolChain::languageExtensions(const QStringList &) const
{
    return LanguageExtension::None;
}

WarningFlags SdccToolChain::warningFlags(const QStringList &cxxflags) const
{
    Q_UNUSED(cxxflags)
    return WarningFlags::Default;
}

ToolChain::BuiltInHeaderPathsRunner SdccToolChain::createBuiltInHeaderPathsRunner(
        const Environment &) const
{
    Environment env = Environment::systemEnvironment();
    addToEnvironment(env);

    const FilePath compiler = compilerCommand();
    const Abi abi = targetAbi();

    return [env, compiler, abi](const QStringList &, const FilePath &, const QString &) {
        return dumpHeaderPaths(compiler, env, abi);
    };
}

void SdccToolChain::addToEnvironment(Environment &env) const
{
    if (!compilerCommand().isEmpty())
        env.prependOrSetPath(compilerCommand().parentDir());
}

std::unique_ptr<ToolChainConfigWidget> SdccToolChain::createConfigurationWidget()
{
    return std::make_unique<SdccToolChainConfigWidget>(this);
}

bool SdccToolChain::operator==(const ToolChain &other) const
{
    if (!ToolChain::operator==(other))
        return false;

    const auto customTc = static_cast<const SdccToolChain *>(&other);
    return compilerCommand() == customTc->compilerCommand()
            && targetAbi() == customTc->targetAbi();
}

// SdccToolChainFactory

SdccToolChainFactory::SdccToolChainFactory()
{
    setDisplayName(Tr::tr("SDCC"));
    setSupportedToolChainType(Constants::SDCC_TOOLCHAIN_TYPEID);
    setSupportedLanguages({ProjectExplorer::Constants::C_LANGUAGE_ID});
    setToolchainConstructor([] { return new SdccToolChain; });
    setUserCreatable(true);
}

Toolchains SdccToolChainFactory::autoDetect(const ToolchainDetector &detector) const
{
    Candidates candidates;

    if (Utils::HostOsInfo::isWindowsHost()) {

        // Tries to detect the candidate from the 32-bit
        // or 64-bit system registry format.
        auto probeCandidate = [](QSettings::Format format) {
            QSettings registry("HKEY_LOCAL_MACHINE\\SOFTWARE\\SDCC",
                               format);
            QString compilerPath = registry.value("Default").toString();
            if (compilerPath.isEmpty())
                return Candidate{};
            // Build full compiler path.
            compilerPath += "/bin/sdcc.exe";
            const FilePath fn = FilePath::fromString(
                        QFileInfo(compilerPath).absoluteFilePath());
            if (!fn.isExecutableFile())
                return Candidate{};
            // Build compiler version.
            const QString version = QString("%1.%2.%3").arg(
                        registry.value("VersionMajor").toString(),
                        registry.value("VersionMinor").toString(),
                        registry.value("VersionRevision").toString());
            return Candidate{fn, version};
        };

        const QSettings::Format allowedFormats[] = {
            QSettings::NativeFormat,
#ifdef Q_OS_WIN
            QSettings::Registry32Format,
            QSettings::Registry64Format
#endif
        };

        for (const QSettings::Format format : allowedFormats) {
            const auto candidate = probeCandidate(format);
            if (candidate.compilerPath.isEmpty() || candidates.contains(candidate))
                continue;
            candidates.push_back(candidate);
        }
    }

    const FilePath fn = compilerPathFromEnvironment("sdcc");
    if (fn.exists()) {
        const Environment env = Environment::systemEnvironment();
        const Macros macros = dumpPredefinedMacros(fn, env, {});
        const QString version = guessVersion(macros);
        const Candidate candidate = {fn, version};
        if (!candidates.contains(candidate))
            candidates.push_back(candidate);
    }

    return autoDetectToolchains(candidates, detector.alreadyKnown);
}

Toolchains SdccToolChainFactory::autoDetectToolchains(
        const Candidates &candidates, const Toolchains &alreadyKnown) const
{
    Toolchains result;

    for (const Candidate &candidate : std::as_const(candidates)) {
        const Toolchains filtered = Utils::filtered(alreadyKnown, [candidate](ToolChain *tc) {
            return tc->typeId() == Constants::SDCC_TOOLCHAIN_TYPEID
                && tc->compilerCommand() == candidate.compilerPath
                && (tc->language() == ProjectExplorer::Constants::C_LANGUAGE_ID);
        });

        if (!filtered.isEmpty()) {
            result << filtered;
            continue;
        }

        // Create toolchain only for C language (because SDCC does not support C++).
        result << autoDetectToolchain(candidate, ProjectExplorer::Constants::C_LANGUAGE_ID);
    }

    return result;
}

Toolchains SdccToolChainFactory::autoDetectToolchain(const Candidate &candidate, Id language) const
{
    const auto env = Environment::systemEnvironment();

    // Table of supported ABI's by SDCC compiler.
    const Abi knownAbis[] = {
        {Abi::Mcs51Architecture},
        {Abi::Stm8Architecture}
    };

    Toolchains tcs;

    // Probe each ABI from the table, because the SDCC compiler
    // can be compiled with or without the specified architecture.
    for (const auto &knownAbi : knownAbis) {
        const Macros macros = dumpPredefinedMacros(candidate.compilerPath, env, knownAbi);
        if (macros.isEmpty())
            continue;
        const Abi abi = guessAbi(macros);
        if (knownAbi.architecture() != abi.architecture())
            continue;

        const auto tc = new SdccToolChain;
        tc->setDetection(ToolChain::AutoDetection);
        tc->setLanguage(language);
        tc->setCompilerCommand(candidate.compilerPath);
        tc->setTargetAbi(abi);
        tc->setDisplayName(buildDisplayName(abi.architecture(), language,
                                            candidate.compilerVersion));

        const auto languageVersion = ToolChain::languageVersion(language, macros);
        tc->predefinedMacrosCache()->insert({}, {macros, languageVersion});

        tcs.push_back(tc);
    }

    return tcs;
}

// SdccToolChainConfigWidget

SdccToolChainConfigWidget::SdccToolChainConfigWidget(SdccToolChain *tc) :
    ToolChainConfigWidget(tc),
    m_compilerCommand(new PathChooser),
    m_abiWidget(new AbiWidget)
{
    m_compilerCommand->setExpectedKind(PathChooser::ExistingCommand);
    m_compilerCommand->setHistoryCompleter("PE.SDCC.Command.History");
    m_mainLayout->addRow(Tr::tr("&Compiler path:"), m_compilerCommand);
    m_mainLayout->addRow(Tr::tr("&ABI:"), m_abiWidget);

    m_abiWidget->setEnabled(false);

    addErrorLabel();
    setFromToolchain();

    connect(m_compilerCommand, &PathChooser::rawPathChanged,
            this, &SdccToolChainConfigWidget::handleCompilerCommandChange);
    connect(m_abiWidget, &AbiWidget::abiChanged,
            this, &ToolChainConfigWidget::dirty);
}

void SdccToolChainConfigWidget::applyImpl()
{
    if (toolChain()->isAutoDetected())
        return;

    const auto tc = static_cast<SdccToolChain *>(toolChain());
    const QString displayName = tc->displayName();
    tc->setCompilerCommand(m_compilerCommand->filePath());
    tc->setTargetAbi(m_abiWidget->currentAbi());
    tc->setDisplayName(displayName);

    if (m_macros.isEmpty())
        return;

    const auto languageVersion = ToolChain::languageVersion(tc->language(), m_macros);
    tc->predefinedMacrosCache()->insert({}, {m_macros, languageVersion});

    setFromToolchain();
}

bool SdccToolChainConfigWidget::isDirtyImpl() const
{
    const auto tc = static_cast<SdccToolChain *>(toolChain());
    return m_compilerCommand->filePath() != tc->compilerCommand()
            || m_abiWidget->currentAbi() != tc->targetAbi()
            ;
}

void SdccToolChainConfigWidget::makeReadOnlyImpl()
{
    m_compilerCommand->setReadOnly(true);
    m_abiWidget->setEnabled(false);
}

void SdccToolChainConfigWidget::setFromToolchain()
{
    const QSignalBlocker blocker(this);
    const auto tc = static_cast<SdccToolChain *>(toolChain());
    m_compilerCommand->setFilePath(tc->compilerCommand());
    m_abiWidget->setAbis({}, tc->targetAbi());
    const bool haveCompiler = m_compilerCommand->filePath().isExecutableFile();
    m_abiWidget->setEnabled(haveCompiler && !tc->isAutoDetected());
}

void SdccToolChainConfigWidget::handleCompilerCommandChange()
{
    const FilePath compilerPath = m_compilerCommand->filePath();
    const bool haveCompiler = compilerPath.isExecutableFile();
    if (haveCompiler) {
        const auto env = Environment::systemEnvironment();
        m_macros = dumpPredefinedMacros(compilerPath, env, {});
        const Abi guessed = guessAbi(m_macros);
        m_abiWidget->setAbis({}, guessed);
    }

    m_abiWidget->setEnabled(haveCompiler);
    emit dirty();
}

} // BareMetal::Internal
