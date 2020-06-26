/****************************************************************************
**
** Copyright (C) 2019 Denis Shienkov <denis.shienkov@gmail.com>
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

#include "baremetalconstants.h"

#include "iarewparser.h"
#include "iarewtoolchain.h"

#include <projectexplorer/abiwidget.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmacro.h>
#include <projectexplorer/toolchainmanager.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/synchronousprocess.h>

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

namespace BareMetal {
namespace Internal {

// Helpers:

static const char compilerCommandKeyC[] = "BareMetal.IarToolChain.CompilerPath";
static const char compilerPlatformCodeGenFlagsKeyC[] = "BareMetal.IarToolChain.PlatformCodeGenFlags";
static const char targetAbiKeyC[] = "BareMetal.IarToolChain.TargetAbi";

static bool compilerExists(const FilePath &compilerPath)
{
    const QFileInfo fi = compilerPath.toFileInfo();
    return fi.exists() && fi.isExecutable() && fi.isFile();
}

static QString cppLanguageOption(const FilePath &compiler)
{
    const QString baseName = compiler.toFileInfo().baseName();
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
                                   const Utils::Id languageId, const QStringList &env)
{
    if (compiler.isEmpty() || !compiler.toFileInfo().isExecutable())
        return {};

    // IAR compiler requires an input and output files.

    QTemporaryFile fakeIn;
    if (!fakeIn.open())
        return {};
    fakeIn.close();

    const QString outpath = fakeIn.fileName() + ".tmp";

    SynchronousProcess cpp;
    cpp.setEnvironment(env);
    cpp.setTimeoutS(10);

    CommandLine cmd(compiler, {fakeIn.fileName()});
    if (languageId == ProjectExplorer::Constants::CXX_LANGUAGE_ID)
        cmd.addArg(cppLanguageOption(compiler));
    cmd.addArgs(extraArgs);
    cmd.addArg("--predef_macros");
    cmd.addArg(outpath);

    const SynchronousProcessResponse response = cpp.runBlocking(cmd);
    if (response.result != SynchronousProcessResponse::Finished
            || response.exitCode != 0) {
        qWarning() << response.exitMessage(cmd.toUserOutput(), 10);
        return {};
    }

    QByteArray output;
    QFile fakeOut(outpath);
    if (fakeOut.open(QIODevice::ReadOnly))
        output = fakeOut.readAll();
    fakeOut.remove();

    return Macro::toMacros(output);
}

static HeaderPaths dumpHeaderPaths(const FilePath &compiler, const Utils::Id languageId,
                                   const QStringList &env)
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

    SynchronousProcess cpp;
    cpp.setEnvironment(env);
    cpp.setTimeoutS(10);

    CommandLine cmd(compiler, {fakeIn.fileName()});
    if (languageId == ProjectExplorer::Constants::CXX_LANGUAGE_ID)
        cmd.addArg(cppLanguageOption(compiler));
    cmd.addArg("--preinclude");
    cmd.addArg(".");

    // Note: Response should retutn an error, just don't check on errors.
    const SynchronousProcessResponse response = cpp.runBlocking(cmd);

    HeaderPaths headerPaths;

    const QByteArray output = response.allOutput().toUtf8();
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
            headerPaths.append({headerPath, HeaderPathType::BuiltIn});

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
    const auto langName = ToolChainManager::displayNameOfLanguageId(language);
    return IarToolChain::tr("IAREW %1 (%2, %3)").arg(version, langName, archName);
}

// IarToolChain

IarToolChain::IarToolChain() :
    ToolChain(Constants::IAREW_TOOLCHAIN_TYPEID)
{
    setTypeDisplayName(Internal::IarToolChain::tr("IAREW"));
}

void IarToolChain::setTargetAbi(const Abi &abi)
{
    if (abi == m_targetAbi)
        return;
    m_targetAbi = abi;
    toolChainUpdated();
}

Abi IarToolChain::targetAbi() const
{
    return m_targetAbi;
}

bool IarToolChain::isValid() const
{
    return true;
}

ToolChain::MacroInspectionRunner IarToolChain::createMacroInspectionRunner() const
{
    Environment env = Environment::systemEnvironment();
    addToEnvironment(env);

    const Utils::FilePath compilerCommand = m_compilerCommand;
    const Utils::Id languageId = language();
    const QStringList extraArgs = m_extraCodeModelFlags;
    MacrosCache macrosCache = predefinedMacrosCache();

    return [env, compilerCommand, extraArgs, macrosCache, languageId]
            (const QStringList &flags) {
        Q_UNUSED(flags)

        Macros macros = dumpPredefinedMacros(compilerCommand, extraArgs, languageId, env.toStringList());
        macros.append({"__intrinsic", "", MacroType::Define});
        macros.append({"__nounwind", "", MacroType::Define});
        macros.append({"__noreturn", "", MacroType::Define});
        macros.append({"__packed", "", MacroType::Define});
        macros.append({"__spec_string", "", MacroType::Define});
        macros.append({"__constrange(__a,__b)", "", MacroType::Define});

        const auto languageVersion = ToolChain::languageVersion(languageId, macros);
        const auto report = MacroInspectionReport{macros, languageVersion};
        macrosCache->insert({}, report);

        return report;
    };
}

Macros IarToolChain::predefinedMacros(const QStringList &cxxflags) const
{
    return createMacroInspectionRunner()(cxxflags).macros;
}

Utils::LanguageExtensions IarToolChain::languageExtensions(const QStringList &) const
{
    return LanguageExtension::None;
}

WarningFlags IarToolChain::warningFlags(const QStringList &cxxflags) const
{
    Q_UNUSED(cxxflags)
    return WarningFlags::Default;
}

ToolChain::BuiltInHeaderPathsRunner IarToolChain::createBuiltInHeaderPathsRunner(
        const Environment &) const
{
    Environment env = Environment::systemEnvironment();
    addToEnvironment(env);

    const Utils::FilePath compilerCommand = m_compilerCommand;
    const Utils::Id languageId = language();

    HeaderPathsCache headerPaths = headerPathsCache();

    return [env, compilerCommand, headerPaths, languageId](const QStringList &flags,
                                                                const QString &fileName,
                                                                const QString &) {
        Q_UNUSED(flags)
        Q_UNUSED(fileName)

        const HeaderPaths paths = dumpHeaderPaths(compilerCommand, languageId, env.toStringList());
        headerPaths->insert({}, paths);

        return paths;
    };
}

HeaderPaths IarToolChain::builtInHeaderPaths(const QStringList &cxxFlags,
                                             const FilePath &fileName,
                                             const Environment &env) const
{
    return createBuiltInHeaderPathsRunner(env)(cxxFlags, fileName.toString(), "");
}

void IarToolChain::addToEnvironment(Environment &env) const
{
    if (!m_compilerCommand.isEmpty()) {
        const FilePath path = m_compilerCommand.parentDir();
        env.prependOrSetPath(path.toString());
    }
}

QList<Utils::OutputLineParser *> IarToolChain::createOutputParsers() const
{
    return {new IarParser()};
}

QVariantMap IarToolChain::toMap() const
{
    QVariantMap data = ToolChain::toMap();
    data.insert(compilerCommandKeyC, m_compilerCommand.toString());
    data.insert(compilerPlatformCodeGenFlagsKeyC, m_extraCodeModelFlags);
    data.insert(targetAbiKeyC, m_targetAbi.toString());
    return data;
}

bool IarToolChain::fromMap(const QVariantMap &data)
{
    if (!ToolChain::fromMap(data))
        return false;
    m_compilerCommand = FilePath::fromString(data.value(compilerCommandKeyC).toString());
    m_extraCodeModelFlags = data.value(compilerPlatformCodeGenFlagsKeyC).toStringList();
    m_targetAbi = Abi::fromString(data.value(targetAbiKeyC).toString());
    return true;
}

std::unique_ptr<ToolChainConfigWidget> IarToolChain::createConfigurationWidget()
{
    return std::make_unique<IarToolChainConfigWidget>(this);
}

bool IarToolChain::operator==(const ToolChain &other) const
{
    if (!ToolChain::operator==(other))
        return false;

    const auto customTc = static_cast<const IarToolChain *>(&other);
    return m_compilerCommand == customTc->m_compilerCommand
            && m_targetAbi == customTc->m_targetAbi
            && m_extraCodeModelFlags == customTc->m_extraCodeModelFlags
            ;
}

void IarToolChain::setCompilerCommand(const FilePath &file)
{
    if (file == m_compilerCommand)
        return;
    m_compilerCommand = file;
    toolChainUpdated();
}

FilePath IarToolChain::compilerCommand() const
{
    return m_compilerCommand;
}

void IarToolChain::setExtraCodeModelFlags(const QStringList &flags)
{
    if (flags == m_extraCodeModelFlags)
        return;
    m_extraCodeModelFlags = flags;
    toolChainUpdated();
}

QStringList IarToolChain::extraCodeModelFlags() const
{
    return m_extraCodeModelFlags;
}

FilePath IarToolChain::makeCommand(const Environment &env) const
{
    Q_UNUSED(env)
    return {};
}

// IarToolChainFactory

IarToolChainFactory::IarToolChainFactory()
{
    setDisplayName(IarToolChain::tr("IAREW"));
    setSupportedToolChainType(Constants::IAREW_TOOLCHAIN_TYPEID);
    setSupportedLanguages({ProjectExplorer::Constants::C_LANGUAGE_ID,
                           ProjectExplorer::Constants::CXX_LANGUAGE_ID});
    setToolchainConstructor([] { return new IarToolChain; });
    setUserCreatable(true);
}

QList<ToolChain *> IarToolChainFactory::autoDetect(const QList<ToolChain *> &alreadyKnown)
{
    Candidates candidates;

#ifdef Q_OS_WIN

#ifdef Q_OS_WIN64
    static const char kRegistryNode[] = "HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\IAR Systems\\Embedded Workbench";
#else
    static const char kRegistryNode[] = "HKEY_LOCAL_MACHINE\\SOFTWARE\\IAR Systems\\Embedded Workbench";
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

    QSettings registry(kRegistryNode, QSettings::NativeFormat);
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
                        const FilePath fn = FilePath::fromString(compilerPath);
                        if (compilerExists(fn)) {
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

#endif // Q_OS_WIN

    return autoDetectToolchains(candidates, alreadyKnown);
}

QList<ToolChain *> IarToolChainFactory::autoDetectToolchains(
        const Candidates &candidates, const QList<ToolChain *> &alreadyKnown) const
{
    QList<ToolChain *> result;

    for (const Candidate &candidate : qAsConst(candidates)) {
        const QList<ToolChain *> filtered = Utils::filtered(
                    alreadyKnown, [candidate](ToolChain *tc) {
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

QList<ToolChain *> IarToolChainFactory::autoDetectToolchain(
        const Candidate &candidate, Utils::Id languageId) const
{
    const auto env = Environment::systemEnvironment();
    const Macros macros = dumpPredefinedMacros(candidate.compilerPath, {}, languageId,
                                               env.toStringList());
    if (macros.isEmpty())
        return {};
    const Abi abi = guessAbi(macros);

    const auto tc = new IarToolChain;
    tc->setDetection(ToolChain::AutoDetection);
    tc->setLanguage(languageId);
    tc->setCompilerCommand(candidate.compilerPath);
    tc->setTargetAbi(abi);
    tc->setDisplayName(buildDisplayName(abi.architecture(), languageId,
                                        candidate.compilerVersion));

    const auto languageVersion = ToolChain::languageVersion(languageId, macros);
    tc->predefinedMacrosCache()->insert({}, {macros, languageVersion});
    return {tc};
}

// IarToolChainConfigWidget

IarToolChainConfigWidget::IarToolChainConfigWidget(IarToolChain *tc) :
    ToolChainConfigWidget(tc),
    m_compilerCommand(new PathChooser),
    m_abiWidget(new AbiWidget)
{
    m_compilerCommand->setExpectedKind(PathChooser::ExistingCommand);
    m_compilerCommand->setHistoryCompleter("PE.IAREW.Command.History");
    m_mainLayout->addRow(tr("&Compiler path:"), m_compilerCommand);
    m_platformCodeGenFlagsLineEdit = new QLineEdit(this);
    m_platformCodeGenFlagsLineEdit->setText(QtcProcess::joinArgs(tc->extraCodeModelFlags()));
    m_mainLayout->addRow(tr("Platform codegen flags:"), m_platformCodeGenFlagsLineEdit);
    m_mainLayout->addRow(tr("&ABI:"), m_abiWidget);

    m_abiWidget->setEnabled(false);

    addErrorLabel();
    setFromToolchain();

    connect(m_compilerCommand, &PathChooser::rawPathChanged,
            this, &IarToolChainConfigWidget::handleCompilerCommandChange);
    connect(m_platformCodeGenFlagsLineEdit, &QLineEdit::editingFinished,
            this, &IarToolChainConfigWidget::handlePlatformCodeGenFlagsChange);
    connect(m_abiWidget, &AbiWidget::abiChanged,
            this, &ToolChainConfigWidget::dirty);
}

void IarToolChainConfigWidget::applyImpl()
{
    if (toolChain()->isAutoDetected())
        return;

    const auto tc = static_cast<IarToolChain *>(toolChain());
    const QString displayName = tc->displayName();
    tc->setCompilerCommand(m_compilerCommand->filePath());
    tc->setExtraCodeModelFlags(splitString(m_platformCodeGenFlagsLineEdit->text()));
    tc->setTargetAbi(m_abiWidget->currentAbi());
    tc->setDisplayName(displayName);

    if (m_macros.isEmpty())
        return;

    const auto languageVersion = ToolChain::languageVersion(tc->language(), m_macros);
    tc->predefinedMacrosCache()->insert({}, {m_macros, languageVersion});

    setFromToolchain();
}

bool IarToolChainConfigWidget::isDirtyImpl() const
{
    const auto tc = static_cast<IarToolChain *>(toolChain());
    return m_compilerCommand->filePath() != tc->compilerCommand()
            || m_platformCodeGenFlagsLineEdit->text() != QtcProcess::joinArgs(tc->extraCodeModelFlags())
            || m_abiWidget->currentAbi() != tc->targetAbi()
            ;
}

void IarToolChainConfigWidget::makeReadOnlyImpl()
{
    m_compilerCommand->setReadOnly(true);
    m_platformCodeGenFlagsLineEdit->setEnabled(false);
    m_abiWidget->setEnabled(false);
}

void IarToolChainConfigWidget::setFromToolchain()
{
    const QSignalBlocker blocker(this);
    const auto tc = static_cast<IarToolChain *>(toolChain());
    m_compilerCommand->setFilePath(tc->compilerCommand());
    m_platformCodeGenFlagsLineEdit->setText(QtcProcess::joinArgs(tc->extraCodeModelFlags()));
    m_abiWidget->setAbis({}, tc->targetAbi());
    const bool haveCompiler = compilerExists(m_compilerCommand->filePath());
    m_abiWidget->setEnabled(haveCompiler && !tc->isAutoDetected());
}

void IarToolChainConfigWidget::handleCompilerCommandChange()
{
    const FilePath compilerPath = m_compilerCommand->filePath();
    const bool haveCompiler = compilerExists(compilerPath);
    if (haveCompiler) {
        const auto env = Environment::systemEnvironment();
        const QStringList extraArgs = splitString(m_platformCodeGenFlagsLineEdit->text());
        const auto languageId = toolChain()->language();
        m_macros = dumpPredefinedMacros(compilerPath, extraArgs, languageId,
                                        env.toStringList());
        const Abi guessed = guessAbi(m_macros);
        m_abiWidget->setAbis({}, guessed);
    }

    m_abiWidget->setEnabled(haveCompiler);
    emit dirty();
}

void IarToolChainConfigWidget::handlePlatformCodeGenFlagsChange()
{
    const QString str1 = m_platformCodeGenFlagsLineEdit->text();
    const QString str2 = QtcProcess::joinArgs(splitString(str1));
    if (str1 != str2)
        m_platformCodeGenFlagsLineEdit->setText(str2);
    else
        handleCompilerCommandChange();
}

} // namespace Internal
} // namespace BareMetal
