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
#include <utils/qtcprocess.h>
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
    const QString langName = ToolchainManager::displayNameOfLanguageId(language);
    return Tr::tr("SDCC %1 (%2, %3)").arg(version, langName, archName);
}

static FilePath compilerPathFromEnvironment(const QString &compilerName)
{
    const Environment systemEnvironment = Environment::systemEnvironment();
    return systemEnvironment.searchInPath(compilerName);
}

// SdccToolchainConfigWidget

class SdccToolchain;

class SdccToolchainConfigWidget final : public ToolchainConfigWidget
{
public:
    explicit SdccToolchainConfigWidget(const ToolchainBundle &bundle);

private:
    void applyImpl() final;
    void discardImpl() final { setFromToolchain(); }
    bool isDirtyImpl() const final;
    void makeReadOnlyImpl() final;

    void setFromToolchain();
    void handleCompilerCommandChange();

    AbiWidget *m_abiWidget = nullptr;
    Macros m_macros;
};

// SdccToolchain

class SdccToolchain final : public Toolchain
{
public:
    SdccToolchain() : Toolchain(Constants::SDCC_TOOLCHAIN_TYPEID)
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

    bool operator==(const Toolchain &other) const final;

    FilePath makeCommand(const Environment &) const final { return {}; }

private:
    friend class SdccToolchainFactory;
    friend class SdccToolchainConfigWidget;
};

Toolchain::MacroInspectionRunner SdccToolchain::createMacroInspectionRunner() const
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

LanguageExtensions SdccToolchain::languageExtensions(const QStringList &) const
{
    return LanguageExtension::None;
}

WarningFlags SdccToolchain::warningFlags(const QStringList &cxxflags) const
{
    Q_UNUSED(cxxflags)
    return WarningFlags::Default;
}

Toolchain::BuiltInHeaderPathsRunner SdccToolchain::createBuiltInHeaderPathsRunner(
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

void SdccToolchain::addToEnvironment(Environment &env) const
{
    if (!compilerCommand().isEmpty())
        env.prependOrSetPath(compilerCommand().parentDir());
}

bool SdccToolchain::operator==(const Toolchain &other) const
{
    if (!Toolchain::operator==(other))
        return false;

    const auto customTc = static_cast<const SdccToolchain *>(&other);
    return compilerCommand() == customTc->compilerCommand()
            && targetAbi() == customTc->targetAbi();
}

// SdccToolchainFactory

class SdccToolchainFactory final : public ToolchainFactory
{
public:
    SdccToolchainFactory()
    {
        setDisplayName(Tr::tr("SDCC"));
        setSupportedToolchainType(Constants::SDCC_TOOLCHAIN_TYPEID);
        setSupportedLanguages({ProjectExplorer::Constants::C_LANGUAGE_ID});
        setToolchainConstructor([] { return new SdccToolchain; });
        setUserCreatable(true);
    }

    Toolchains autoDetect(const ToolchainDetector &detector) const final;
    std::unique_ptr<ToolchainConfigWidget> createConfigurationWidget(
        const ToolchainBundle &bundle) const final
    {
        return std::make_unique<SdccToolchainConfigWidget>(bundle);
    }

private:
    Toolchains autoDetectToolchains(const Candidates &candidates,
                                    const Toolchains &alreadyKnown) const;
    Toolchains autoDetectToolchain(const Candidate &candidate, Id language) const;
};

void setupSdccToolchain()
{
    static SdccToolchainFactory theSdccToolchainFactory;
}

Toolchains SdccToolchainFactory::autoDetect(const ToolchainDetector &detector) const
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

Toolchains SdccToolchainFactory::autoDetectToolchains(
        const Candidates &candidates, const Toolchains &alreadyKnown) const
{
    Toolchains result;

    for (const Candidate &candidate : std::as_const(candidates)) {
        const Toolchains filtered = Utils::filtered(alreadyKnown, [candidate](Toolchain *tc) {
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

Toolchains SdccToolchainFactory::autoDetectToolchain(const Candidate &candidate, Id language) const
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

        const auto tc = new SdccToolchain;
        tc->setDetection(Toolchain::AutoDetection);
        tc->setLanguage(language);
        tc->setCompilerCommand(candidate.compilerPath);
        tc->setTargetAbi(abi);
        tc->setDisplayName(buildDisplayName(abi.architecture(), language,
                                            candidate.compilerVersion));

        const auto languageVersion = Toolchain::languageVersion(language, macros);
        tc->predefinedMacrosCache()->insert({}, {macros, languageVersion});

        tcs.push_back(tc);
    }

    return tcs;
}

// SdccToolchainConfigWidget

SdccToolchainConfigWidget::SdccToolchainConfigWidget(const ToolchainBundle &bundle) :
    ToolchainConfigWidget(bundle),
    m_abiWidget(new AbiWidget)
{
    m_mainLayout->addRow(Tr::tr("&ABI:"), m_abiWidget);

    m_abiWidget->setEnabled(false);

    addErrorLabel();
    setFromToolchain();

    connect(this, &ToolchainConfigWidget::compilerCommandChanged,
            this, &SdccToolchainConfigWidget::handleCompilerCommandChange);
    connect(m_abiWidget, &AbiWidget::abiChanged,
            this, &ToolchainConfigWidget::dirty);
}

void SdccToolchainConfigWidget::applyImpl()
{
    if (bundle().isAutoDetected())
        return;

    bundle().setTargetAbi(m_abiWidget->currentAbi());
    if (m_macros.isEmpty())
        return;

    bundle().forEach<SdccToolchain>([this](SdccToolchain &tc) {
        const auto languageVersion = Toolchain::languageVersion(tc.language(), m_macros);
        tc.predefinedMacrosCache()->insert({}, {m_macros, languageVersion});
    });
    setFromToolchain();
}

bool SdccToolchainConfigWidget::isDirtyImpl() const
{
    return m_abiWidget->currentAbi() != bundle().targetAbi();
}

void SdccToolchainConfigWidget::makeReadOnlyImpl()
{
    m_abiWidget->setEnabled(false);
}

void SdccToolchainConfigWidget::setFromToolchain()
{
    const QSignalBlocker blocker(this);
    m_abiWidget->setAbis({}, bundle().targetAbi());
    const bool haveCompiler
        = compilerCommand(ProjectExplorer::Constants::C_LANGUAGE_ID).isExecutableFile();
    m_abiWidget->setEnabled(haveCompiler && !bundle().isAutoDetected());
}

void SdccToolchainConfigWidget::handleCompilerCommandChange()
{
    const FilePath compilerPath = compilerCommand(ProjectExplorer::Constants::C_LANGUAGE_ID);
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
