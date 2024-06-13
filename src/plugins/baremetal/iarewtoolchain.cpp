// Copyright (C) 2019 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iarewtoolchain.h"

#include "baremetalconstants.h"
#include "baremetaltr.h"
#include "iarewparser.h"

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

using namespace ProjectExplorer;
using namespace Utils;

namespace BareMetal::Internal {

static QString cppLanguageOption(const FilePath &compiler)
{
    const QString baseName = compiler.baseName();
    if (baseName == "iccarm" || baseName == "iccrl78"
            || baseName == "iccrh850" || baseName == "iccrx"
            || baseName == "iccriscv") {
        return QString("--c++");
    }
    if (baseName == "icc8051" || baseName == "iccavr"
            || baseName == "iccstm8" || baseName == "icc430"
            || baseName == "iccv850" || baseName == "icc78k"
            || baseName == "iccavr32" || baseName == "iccsh"
            || baseName == "icccf" || baseName == "iccm32c"
            || baseName == "iccm16c" || baseName == "iccr32c"
            || baseName == "icccr16c") {
        return QString("--ec++");
    }
    return {};
}

static Macros dumpPredefinedMacros(const FilePath &compiler, const QStringList &extraArgs,
                                   const Id languageId, const Environment &env)
{
    if (compiler.isEmpty() || !compiler.toFileInfo().isExecutable())
        return {};

    // IAR compiler requires an input and output files.

    QTemporaryFile fakeIn;
    if (!fakeIn.open())
        return {};
    fakeIn.close();

    const QString outpath = fakeIn.fileName() + ".tmp";

    Process cpp;
    cpp.setEnvironment(env);

    CommandLine cmd(compiler, {fakeIn.fileName()});
    if (languageId == ProjectExplorer::Constants::CXX_LANGUAGE_ID)
        cmd.addArg(cppLanguageOption(compiler));
    cmd.addArgs(extraArgs);
    cmd.addArg("--predef_macros");
    cmd.addArg(outpath);

    cpp.setCommand(cmd);
    cpp.runBlocking();
    if (cpp.result() != ProcessResult::FinishedWithSuccess) {
        qWarning() << cpp.exitMessage();
        return {};
    }

    QByteArray output;
    QFile fakeOut(outpath);
    if (fakeOut.open(QIODevice::ReadOnly))
        output = fakeOut.readAll();
    fakeOut.remove();

    return Macro::toMacros(output);
}

static HeaderPaths dumpHeaderPaths(const FilePath &compiler, const Id languageId,
                                   const Environment &env)
{
    if (!compiler.exists())
        return {};

    // Seems, that IAR compiler has not options to show a list of system
    // include directories. But, we can use the following trick to enumerate
    // this directories. We need to specify the '--preinclude' option with
    // the wrong value (e.g. a dot). In this case the compiler fails and its
    // error output will contains a mention about the using search directories
    // in a form of tokens, like: ' searched: "<path/to/include>" '. Where are
    // the resulting paths are escaped with a quotes.

    QTemporaryFile fakeIn;
    if (!fakeIn.open())
        return {};
    fakeIn.close();

    CommandLine cmd(compiler, {fakeIn.fileName()});
    if (languageId == ProjectExplorer::Constants::CXX_LANGUAGE_ID)
        cmd.addArg(cppLanguageOption(compiler));
    cmd.addArg("--preinclude");
    cmd.addArg(".");

    Process cpp;
    cpp.setEnvironment(env);
    cpp.setCommand(cmd);
    cpp.runBlocking();

    HeaderPaths headerPaths;

    const QByteArray output = cpp.allOutput().toUtf8();
    for (auto pos = 0; pos < output.size(); ++pos) {
        const int searchIndex = output.indexOf("searched:", pos);
        if (searchIndex == -1)
            break;
        const int startQuoteIndex = output.indexOf('"', searchIndex + 1);
        if (startQuoteIndex == -1)
            break;
        const int endQuoteIndex = output.indexOf('"', startQuoteIndex + 1);
        if (endQuoteIndex == -1)
            break;

        const QByteArray candidate = output.mid(startQuoteIndex + 1,
                                                endQuoteIndex - startQuoteIndex - 1)
                .simplified();

        const QString headerPath = QFileInfo(QFile::decodeName(candidate))
                .canonicalFilePath();

        // Ignore the QtC binary directory path.
        if (headerPath != QCoreApplication::applicationDirPath())
            headerPaths.append(HeaderPath::makeBuiltIn(headerPath));

        pos = endQuoteIndex + 1;
    }

    return headerPaths;
}

static Abi::Architecture guessArchitecture(const Macros &macros)
{
    for (const Macro &macro : macros) {
        if (macro.key == "__ICCARM__")
            return Abi::Architecture::ArmArchitecture;
        if (macro.key == "__ICC8051__")
            return Abi::Architecture::Mcs51Architecture;
        if (macro.key == "__ICCAVR__")
            return Abi::Architecture::AvrArchitecture;
        if (macro.key == "__ICCAVR32__")
            return Abi::Architecture::Avr32Architecture;
        if (macro.key == "__ICCSTM8__")
            return Abi::Architecture::Stm8Architecture;
        if (macro.key == "__ICC430__")
            return Abi::Architecture::Msp430Architecture;
        if (macro.key == "__ICCRL78__")
            return Abi::Architecture::Rl78Architecture;
        if (macro.key == "__ICCV850__")
            return Abi::Architecture::V850Architecture;
        if (macro.key == "__ICCRH850__")
            return Abi::Architecture::Rh850Architecture;
        if (macro.key == "__ICCRX__")
            return Abi::Architecture::RxArchitecture;
        if (macro.key == "__ICC78K__")
            return Abi::Architecture::K78Architecture;
        if (macro.key == "__ICCSH__")
            return Abi::Architecture::ShArchitecture;
        if (macro.key == "__ICCRISCV__")
            return Abi::Architecture::RiscVArchitecture;
        if (macro.key == "__ICCCF__")
            return Abi::Architecture::M68KArchitecture;
        if (macro.key == "__ICCM32C__")
            return Abi::Architecture::M32CArchitecture;
        if (macro.key == "__ICCM16C__")
            return Abi::Architecture::M16CArchitecture;
        if (macro.key == "__ICCR32C__")
            return Abi::Architecture::R32CArchitecture;
        if (macro.key == "__ICCCR16C__")
            return Abi::Architecture::CR16Architecture;
    }
    return Abi::Architecture::UnknownArchitecture;
}

static unsigned char guessWordWidth(const Macros &macros)
{
    const Macro sizeMacro = Utils::findOrDefault(macros, [](const Macro &m) {
        return m.key == "__INT_SIZE__";
    });
    if (sizeMacro.isValid() && sizeMacro.type == MacroType::Define)
        return sizeMacro.value.toInt() * 8;
    return 0;
}

static Abi::BinaryFormat guessFormat(Abi::Architecture arch)
{
    if (arch == Abi::Architecture::ArmArchitecture
            || arch == Abi::Architecture::Stm8Architecture
            || arch == Abi::Architecture::Rl78Architecture
            || arch == Abi::Architecture::Rh850Architecture
            || arch == Abi::Architecture::RxArchitecture
            || arch == Abi::Architecture::ShArchitecture
            || arch == Abi::Architecture::RiscVArchitecture) {
        return Abi::BinaryFormat::ElfFormat;
    }
    if (arch == Abi::Architecture::Mcs51Architecture
            || arch == Abi::Architecture::AvrArchitecture
            || arch == Abi::Architecture::Avr32Architecture
            || arch == Abi::Architecture::Msp430Architecture
            || arch == Abi::Architecture::V850Architecture
            || arch == Abi::Architecture::K78Architecture
            || arch == Abi::Architecture::M68KArchitecture
            || arch == Abi::Architecture::M32CArchitecture
            || arch == Abi::Architecture::M16CArchitecture
            || arch == Abi::Architecture::R32CArchitecture
            || arch == Abi::Architecture::CR16Architecture) {
        return Abi::BinaryFormat::UbrofFormat;
    }
    return Abi::BinaryFormat::UnknownFormat;
}

static Abi guessAbi(const Macros &macros)
{
    const auto arch = guessArchitecture(macros);
    return {arch, Abi::OS::BareMetalOS, Abi::OSFlavor::GenericFlavor,
            guessFormat(arch), guessWordWidth(macros)};
}

static QString buildDisplayName(Abi::Architecture arch, Utils::Id language,
                                const QString &version)
{
    const auto archName = Abi::toString(arch);
    const auto langName = ToolchainManager::displayNameOfLanguageId(language);
    return Tr::tr("IAREW %1 (%2, %3)").arg(version, langName, archName);
}

// IarToolchainConfigWidget

class IarToolchain;

class IarToolchainConfigWidget final : public ToolchainConfigWidget
{
public:
    explicit IarToolchainConfigWidget(IarToolchain *tc);

private:
    void applyImpl() final;
    void discardImpl() final { setFromToolchain(); }
    bool isDirtyImpl() const final;
    void makeReadOnlyImpl() final;

    void setFromToolchain();
    void handleCompilerCommandChange();
    void handlePlatformCodeGenFlagsChange();

    PathChooser *m_compilerCommand = nullptr;
    AbiWidget *m_abiWidget = nullptr;
    QLineEdit *m_platformCodeGenFlagsLineEdit = nullptr;
    Macros m_macros;
};

// IarToolchain

class IarToolchain final : public Toolchain
{
public:
    IarToolchain() : Toolchain(Constants::IAREW_TOOLCHAIN_TYPEID)
    {
        setTypeDisplayName(Tr::tr("IAREW"));
        setTargetAbiKey("TargetAbi");
        setCompilerCommandKey("CompilerPath");

        m_extraCodeModelFlags.setSettingsKey("PlatformCodeGenFlags");
        connect(&m_extraCodeModelFlags, &BaseAspect::changed,
                this, &IarToolchain::toolChainUpdated);
    }

    MacroInspectionRunner createMacroInspectionRunner() const final;

    LanguageExtensions languageExtensions(const QStringList &cxxflags) const final;
    WarningFlags warningFlags(const QStringList &cxxflags) const final;

    BuiltInHeaderPathsRunner createBuiltInHeaderPathsRunner(const Environment &) const final;
    void addToEnvironment(Environment &env) const final;
    QList<OutputLineParser *> createOutputParsers() const final { return {new IarParser()}; }

    std::unique_ptr<ToolchainConfigWidget> createConfigurationWidget() final;

    bool operator==(const Toolchain &other) const final;

    QStringList extraCodeModelFlags() const final { return m_extraCodeModelFlags(); }

    FilePath makeCommand(const Environment &) const final { return {}; }

private:
    StringListAspect m_extraCodeModelFlags{this};

    friend class IarToolchainFactory;
    friend class IarToolchainConfigWidget;
};

Toolchain::MacroInspectionRunner IarToolchain::createMacroInspectionRunner() const
{
    Environment env = Environment::systemEnvironment();
    addToEnvironment(env);

    const FilePath compiler = compilerCommand();
    const Id languageId = language();
    const QStringList extraArgs = m_extraCodeModelFlags();
    MacrosCache macrosCache = predefinedMacrosCache();

    return [env, compiler, extraArgs, macrosCache, languageId]
            (const QStringList &flags) {
        Q_UNUSED(flags)

        Macros macros = dumpPredefinedMacros(compiler, extraArgs, languageId, env);
        macros.append({"__intrinsic", "", MacroType::Define});
        macros.append({"__nounwind", "", MacroType::Define});
        macros.append({"__noreturn", "", MacroType::Define});
        macros.append({"__no_init", "", MacroType::Define});
        macros.append({"__packed", "", MacroType::Define});
        macros.append({"__spec_string", "", MacroType::Define});
        macros.append({"__constrange(__a,__b)", "", MacroType::Define});

        const auto languageVersion = Toolchain::languageVersion(languageId, macros);
        const auto report = MacroInspectionReport{macros, languageVersion};
        macrosCache->insert({}, report);

        return report;
    };
}

Utils::LanguageExtensions IarToolchain::languageExtensions(const QStringList &) const
{
    return LanguageExtension::None;
}

WarningFlags IarToolchain::warningFlags(const QStringList &cxxflags) const
{
    Q_UNUSED(cxxflags)
    return WarningFlags::Default;
}

Toolchain::BuiltInHeaderPathsRunner IarToolchain::createBuiltInHeaderPathsRunner(
        const Environment &) const
{
    Environment env = Environment::systemEnvironment();
    addToEnvironment(env);

    const FilePath compiler = compilerCommand();
    const Id languageId = language();

    HeaderPathsCache headerPaths = headerPathsCache();

    return [env, compiler, headerPaths, languageId](const QStringList &flags,
                                                    const FilePath &sysRoot,
                                                    const QString &) {
        Q_UNUSED(flags)
        Q_UNUSED(sysRoot)

        const HeaderPaths paths = dumpHeaderPaths(compiler, languageId, env);
        headerPaths->insert({}, paths);

        return paths;
    };
}

void IarToolchain::addToEnvironment(Environment &env) const
{
    if (!compilerCommand().isEmpty())
        env.prependOrSetPath(compilerCommand().parentDir());
}

std::unique_ptr<ToolchainConfigWidget> IarToolchain::createConfigurationWidget()
{
    return std::make_unique<IarToolchainConfigWidget>(this);
}

bool IarToolchain::operator==(const Toolchain &other) const
{
    if (!Toolchain::operator==(other))
        return false;

    const auto customTc = static_cast<const IarToolchain *>(&other);
    return compilerCommand() == customTc->compilerCommand()
            && m_extraCodeModelFlags() == customTc->m_extraCodeModelFlags();
}


// IarToolchainFactory

class IarToolchainFactory final : public ToolchainFactory
{
public:
    IarToolchainFactory()
    {
        setDisplayName(Tr::tr("IAREW"));
        setSupportedToolchainType(Constants::IAREW_TOOLCHAIN_TYPEID);
        setSupportedLanguages({ProjectExplorer::Constants::C_LANGUAGE_ID,
                               ProjectExplorer::Constants::CXX_LANGUAGE_ID});
        setToolchainConstructor([] { return new IarToolchain; });
        setUserCreatable(true);
    }

    Toolchains autoDetect(const ToolchainDetector &detector) const final;
    Toolchains detectForImport(const ToolchainDescription &tcd) const final;

private:
    Toolchains autoDetectToolchains(const Candidates &candidates,
                                    const Toolchains &alreadyKnown) const;
    Toolchains autoDetectToolchain(const Candidate &candidate, Id languageId) const;
};

void setupIarToolchain()
{
    static IarToolchainFactory theIarToolchainFactory;
}

Toolchains IarToolchainFactory::autoDetect(const ToolchainDetector &detector) const
{
    Candidates candidates;

#ifdef Q_OS_WIN

    QStringList registryNodes;
    registryNodes << "HKEY_LOCAL_MACHINE\\SOFTWARE\\IAR Systems\\Embedded Workbench";
#ifdef Q_OS_WIN64
    registryNodes << "HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\IAR Systems\\Embedded Workbench";
#endif

    // Dictionary for know toolchains.
    static const struct Entry {
        QString registryKey;
        QString subExePath;
    } knowToolchains[] = {
        {{"EWARM"}, {"/arm/bin/iccarm.exe"}},
        {{"EWAVR"}, {"/avr/bin/iccavr.exe"}},
        {{"EWAVR32"}, {"/avr32/bin/iccavr32.exe"}},
        {{"EW8051"}, {"/8051/bin/icc8051.exe"}},
        {{"EWSTM8"}, {"/stm8/bin/iccstm8.exe"}},
        {{"EW430"}, {"/430/bin/icc430.exe"}},
        {{"EWRL78"}, {"/rl78/bin/iccrl78.exe"}},
        {{"EWV850"}, {"/v850/bin/iccv850.exe"}},
        {{"EWRH850"}, {"/rh850/bin/iccrh850.exe"}},
        {{"EWRX"}, {"/rx/bin/iccrx.exe"}},
        {{"EW78K"}, {"/78k/bin/icc78k.exe"}},
        {{"EWSH"}, {"/sh/bin/iccsh.exe"}},
        {{"EWRISCV"}, {"/riscv/bin/iccriscv.exe"}},
        {{"EWCF"}, {"/cf/bin/icccf.exe"}},
        {{"EWM32C"}, {"/m32c/bin/iccm32c.exe"}},
        {{"EWM16C"}, {"/m16c/bin/iccm16c.exe"}},
        {{"EWR32C"}, {"/r32c/bin/iccr32c.exe"}},
        {{"EWCR16C"}, {"/cr16c/bin/icccr16c.exe"}},
    };

    for (const QString &registryNode : registryNodes) {
        QSettings registry(registryNode, QSettings::NativeFormat);
        const auto oneLevelGroups = registry.childGroups();
        for (const QString &oneLevelKey : oneLevelGroups) {
            registry.beginGroup(oneLevelKey);
            const auto twoLevelGroups = registry.childGroups();
            for (const Entry &entry : knowToolchains) {
                if (twoLevelGroups.contains(entry.registryKey)) {
                    registry.beginGroup(entry.registryKey);
                    const auto threeLevelGroups = registry.childGroups();
                    for (const QString &threeLevelKey : threeLevelGroups) {
                        registry.beginGroup(threeLevelKey);
                        QString compilerPath = registry.value("InstallPath").toString();
                        if (!compilerPath.isEmpty()) {
                            // Build full compiler path.
                            compilerPath += entry.subExePath;
                            const FilePath fn = FilePath::fromUserInput(compilerPath);
                            if (fn.isExecutableFile()) {
                                // Note: threeLevelKey is a guessed toolchain version.
                                candidates.push_back({fn, threeLevelKey});
                            }
                        }
                        registry.endGroup();
                    }
                    registry.endGroup();
                }
            }
            registry.endGroup();
        }
    }

#endif // Q_OS_WIN

    return autoDetectToolchains(candidates, detector.alreadyKnown);
}

Toolchains IarToolchainFactory::detectForImport(const ToolchainDescription &tcd) const
{
    return { autoDetectToolchain({tcd.compilerPath, {}}, tcd.language) };
}

Toolchains IarToolchainFactory::autoDetectToolchains(
        const Candidates &candidates, const Toolchains &alreadyKnown) const
{
    Toolchains result;

    for (const Candidate &candidate : std::as_const(candidates)) {
        const Toolchains filtered = Utils::filtered(alreadyKnown, [candidate](Toolchain *tc) {
            return tc->typeId() == Constants::IAREW_TOOLCHAIN_TYPEID
                && tc->compilerCommand() == candidate.compilerPath
                && (tc->language() == ProjectExplorer::Constants::C_LANGUAGE_ID
                    || tc->language() == ProjectExplorer::Constants::CXX_LANGUAGE_ID);
        });

        if (!filtered.isEmpty()) {
            result << filtered;
            continue;
        }

        // Create toolchains for both C and C++ languages.
        result << autoDetectToolchain(candidate, ProjectExplorer::Constants::C_LANGUAGE_ID);
        result << autoDetectToolchain(candidate, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    }

    return result;
}

Toolchains IarToolchainFactory::autoDetectToolchain(const Candidate &candidate, Id languageId) const
{
    if (ToolchainManager::isBadToolchain(candidate.compilerPath))
        return {};
    const auto env = Environment::systemEnvironment();
    const Macros macros = dumpPredefinedMacros(candidate.compilerPath, {}, languageId, env);
    if (macros.isEmpty()) {
        ToolchainManager::addBadToolchain(candidate.compilerPath);
        return {};
    }
    const Abi abi = guessAbi(macros);

    const auto tc = new IarToolchain;
    tc->setDetection(Toolchain::AutoDetection);
    tc->setLanguage(languageId);
    tc->setCompilerCommand(candidate.compilerPath);
    tc->setTargetAbi(abi);
    tc->setDisplayName(buildDisplayName(abi.architecture(), languageId,
                                        candidate.compilerVersion));

    const auto languageVersion = Toolchain::languageVersion(languageId, macros);
    tc->predefinedMacrosCache()->insert({}, {macros, languageVersion});
    return {tc};
}

// IarToolchainConfigWidget

IarToolchainConfigWidget::IarToolchainConfigWidget(IarToolchain *tc) :
    ToolchainConfigWidget(tc),
    m_compilerCommand(new PathChooser),
    m_abiWidget(new AbiWidget)
{
    m_compilerCommand->setExpectedKind(PathChooser::ExistingCommand);
    m_compilerCommand->setHistoryCompleter("PE.IAREW.Command.History");
    m_mainLayout->addRow(Tr::tr("&Compiler path:"), m_compilerCommand);
    m_platformCodeGenFlagsLineEdit = new QLineEdit(this);
    m_platformCodeGenFlagsLineEdit->setText(ProcessArgs::joinArgs(tc->extraCodeModelFlags()));
    m_mainLayout->addRow(Tr::tr("Platform codegen flags:"), m_platformCodeGenFlagsLineEdit);
    m_mainLayout->addRow(Tr::tr("&ABI:"), m_abiWidget);

    m_abiWidget->setEnabled(false);

    addErrorLabel();
    setFromToolchain();

    connect(m_compilerCommand, &PathChooser::rawPathChanged,
            this, &IarToolchainConfigWidget::handleCompilerCommandChange);
    connect(m_platformCodeGenFlagsLineEdit, &QLineEdit::editingFinished,
            this, &IarToolchainConfigWidget::handlePlatformCodeGenFlagsChange);
    connect(m_abiWidget, &AbiWidget::abiChanged,
            this, &ToolchainConfigWidget::dirty);
}

void IarToolchainConfigWidget::applyImpl()
{
    if (toolchain()->isAutoDetected())
        return;

    const auto tc = static_cast<IarToolchain *>(toolchain());
    const QString displayName = tc->displayName();
    tc->setCompilerCommand(m_compilerCommand->filePath());

    tc->m_extraCodeModelFlags.setValue(splitString(m_platformCodeGenFlagsLineEdit->text()));

    tc->setTargetAbi(m_abiWidget->currentAbi());
    tc->setDisplayName(displayName);

    if (m_macros.isEmpty())
        return;

    const auto languageVersion = Toolchain::languageVersion(tc->language(), m_macros);
    tc->predefinedMacrosCache()->insert({}, {m_macros, languageVersion});

    setFromToolchain();
}

bool IarToolchainConfigWidget::isDirtyImpl() const
{
    const auto tc = static_cast<IarToolchain *>(toolchain());
    return m_compilerCommand->filePath() != tc->compilerCommand()
            || m_platformCodeGenFlagsLineEdit->text() != ProcessArgs::joinArgs(tc->extraCodeModelFlags())
            || m_abiWidget->currentAbi() != tc->targetAbi()
            ;
}

void IarToolchainConfigWidget::makeReadOnlyImpl()
{
    m_compilerCommand->setReadOnly(true);
    m_platformCodeGenFlagsLineEdit->setEnabled(false);
    m_abiWidget->setEnabled(false);
}

void IarToolchainConfigWidget::setFromToolchain()
{
    const QSignalBlocker blocker(this);
    const auto tc = static_cast<IarToolchain *>(toolchain());
    m_compilerCommand->setFilePath(tc->compilerCommand());
    m_platformCodeGenFlagsLineEdit->setText(ProcessArgs::joinArgs(tc->extraCodeModelFlags()));
    m_abiWidget->setAbis({}, tc->targetAbi());
    const bool haveCompiler = m_compilerCommand->filePath().isExecutableFile();
    m_abiWidget->setEnabled(haveCompiler && !tc->isAutoDetected());
}

void IarToolchainConfigWidget::handleCompilerCommandChange()
{
    const FilePath compilerPath = m_compilerCommand->filePath();
    const bool haveCompiler = compilerPath.isExecutableFile();
    if (haveCompiler) {
        const auto env = Environment::systemEnvironment();
        const QStringList extraArgs = splitString(m_platformCodeGenFlagsLineEdit->text());
        const Id languageId = toolchain()->language();
        m_macros = dumpPredefinedMacros(compilerPath, extraArgs, languageId, env);
        const Abi guessed = guessAbi(m_macros);
        m_abiWidget->setAbis({}, guessed);
    }

    m_abiWidget->setEnabled(haveCompiler);
    emit dirty();
}

void IarToolchainConfigWidget::handlePlatformCodeGenFlagsChange()
{
    const QString str1 = m_platformCodeGenFlagsLineEdit->text();
    const QString str2 = ProcessArgs::joinArgs(splitString(str1));
    if (str1 != str2)
        m_platformCodeGenFlagsLineEdit->setText(str2);
    else
        handleCompilerCommandChange();
}

} // BareMetal::Internal
