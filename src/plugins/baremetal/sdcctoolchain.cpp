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

#include "sdccparser.h"
#include "sdcctoolchain.h"

#include <projectexplorer/abiwidget.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmacro.h>
#include <projectexplorer/toolchainmanager.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
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
#include <QTextStream>

using namespace ProjectExplorer;
using namespace Utils;

namespace BareMetal {
namespace Internal {

// Helpers:

static const char compilerCommandKeyC[] = "BareMetal.SdccToolChain.CompilerPath";
static const char targetAbiKeyC[] = "BareMetal.SdccToolChain.TargetAbi";

static bool compilerExists(const FilePath &compilerPath)
{
    const QFileInfo fi = compilerPath.toFileInfo();
    return fi.exists() && fi.isExecutable() && fi.isFile();
}

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

static Macros dumpPredefinedMacros(const FilePath &compiler, const QStringList &env,
                                   const Abi &abi)
{
    if (compiler.isEmpty() || !compiler.toFileInfo().isExecutable())
        return {};

    QTemporaryFile fakeIn("XXXXXX.c");
    if (!fakeIn.open())
        return {};
    fakeIn.close();

    SynchronousProcess cpp;
    cpp.setEnvironment(env);
    cpp.setTimeoutS(10);

    const CommandLine cmd(compiler, {compilerTargetFlag(abi),  "-dM", "-E", fakeIn.fileName()});

    const SynchronousProcessResponse response = cpp.runBlocking(cmd);
    if (response.result != SynchronousProcessResponse::Finished
            || response.exitCode != 0) {
        qWarning() << response.exitMessage(compiler.toString(), 10);
        return {};
    }

    const QByteArray output = response.allOutput().toUtf8();
    return Macro::toMacros(output);
}

static HeaderPaths dumpHeaderPaths(const FilePath &compiler, const QStringList &env,
                                   const Abi &abi)
{
    if (!compiler.exists())
        return {};

    SynchronousProcess cpp;
    cpp.setEnvironment(env);
    cpp.setTimeoutS(10);

    const CommandLine cmd(compiler, {compilerTargetFlag(abi), "--print-search-dirs"});

    const SynchronousProcessResponse response = cpp.runBlocking(cmd);
    if (response.result != SynchronousProcessResponse::Finished
            || response.exitCode != 0) {
        qWarning() << response.exitMessage(compiler.toString(), 10);
        return {};
    }

    QString output = response.allOutput();
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
                headerPaths.append({headerPath, HeaderPathType::BuiltIn});
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
    const auto arch = guessArchitecture(macros);
    return {arch, Abi::OS::BareMetalOS, Abi::OSFlavor::GenericFlavor,
            guessFormat(arch), guessWordWidth(macros)};
}

static QString buildDisplayName(Abi::Architecture arch, Utils::Id language,
                                const QString &version)
{
    const auto archName = Abi::toString(arch);
    const auto langName = ToolChainManager::displayNameOfLanguageId(language);
    return SdccToolChain::tr("SDCC %1 (%2, %3)")
            .arg(version, langName, archName);
}

static Utils::FilePath compilerPathFromEnvironment(const QString &compilerName)
{
    const Environment systemEnvironment = Environment::systemEnvironment();
    return systemEnvironment.searchInPath(compilerName);
}

// SdccToolChain

SdccToolChain::SdccToolChain() :
    ToolChain(Constants::SDCC_TOOLCHAIN_TYPEID)
{
    setTypeDisplayName(Internal::SdccToolChain::tr("SDCC"));
}

void SdccToolChain::setTargetAbi(const Abi &abi)
{
    if (abi == m_targetAbi)
        return;
    m_targetAbi = abi;
    toolChainUpdated();
}

Abi SdccToolChain::targetAbi() const
{
    return m_targetAbi;
}

bool SdccToolChain::isValid() const
{
    return true;
}

ToolChain::MacroInspectionRunner SdccToolChain::createMacroInspectionRunner() const
{
    Environment env = Environment::systemEnvironment();
    addToEnvironment(env);

    const Utils::FilePath compilerCommand = m_compilerCommand;
    const Utils::Id lang = language();
    const Abi abi = m_targetAbi;

    MacrosCache macrosCache = predefinedMacrosCache();

    return [env, compilerCommand, macrosCache, lang, abi]
            (const QStringList &flags) {
        Q_UNUSED(flags)

        const Macros macros = dumpPredefinedMacros(compilerCommand, env.toStringList(),
                                                   abi);
        const auto report = MacroInspectionReport{macros, languageVersion(lang, macros)};
        macrosCache->insert({}, report);

        return report;
    };
}

Macros SdccToolChain::predefinedMacros(const QStringList &cxxflags) const
{
    return createMacroInspectionRunner()(cxxflags).macros;
}

Utils::LanguageExtensions SdccToolChain::languageExtensions(const QStringList &) const
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

    const Utils::FilePath compilerCommand = m_compilerCommand;
    const Abi abi = m_targetAbi;

    return [env, compilerCommand, abi](const QStringList &, const QString &, const QString &) {
        return dumpHeaderPaths(compilerCommand, env.toStringList(), abi);
    };
}

HeaderPaths SdccToolChain::builtInHeaderPaths(const QStringList &cxxFlags,
                                              const FilePath &fileName,
                                              const Environment &env) const
{
    return createBuiltInHeaderPathsRunner(env)(cxxFlags, fileName.toString(), "");
}

void SdccToolChain::addToEnvironment(Environment &env) const
{
    if (!m_compilerCommand.isEmpty()) {
        const FilePath path = m_compilerCommand.parentDir();
        env.prependOrSetPath(path.toString());
    }
}

QList<Utils::OutputLineParser *> SdccToolChain::createOutputParsers() const
{
    return {new SdccParser};
}

QVariantMap SdccToolChain::toMap() const
{
    QVariantMap data = ToolChain::toMap();
    data.insert(compilerCommandKeyC, m_compilerCommand.toString());
    data.insert(targetAbiKeyC, m_targetAbi.toString());
    return data;
}

bool SdccToolChain::fromMap(const QVariantMap &data)
{
    if (!ToolChain::fromMap(data))
        return false;
    m_compilerCommand = FilePath::fromString(data.value(compilerCommandKeyC).toString());
    m_targetAbi = Abi::fromString(data.value(targetAbiKeyC).toString());
    return true;
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
    return m_compilerCommand == customTc->m_compilerCommand
            && m_targetAbi == customTc->m_targetAbi
            ;
}

void SdccToolChain::setCompilerCommand(const FilePath &file)
{
    if (file == m_compilerCommand)
        return;
    m_compilerCommand = file;
    toolChainUpdated();
}

FilePath SdccToolChain::compilerCommand() const
{
    return m_compilerCommand;
}

FilePath SdccToolChain::makeCommand(const Environment &env) const
{
    Q_UNUSED(env)
    return {};
}

// SdccToolChainFactory

SdccToolChainFactory::SdccToolChainFactory()
{
    setDisplayName(SdccToolChain::tr("SDCC"));
    setSupportedToolChainType(Constants::SDCC_TOOLCHAIN_TYPEID);
    setSupportedLanguages({ProjectExplorer::Constants::C_LANGUAGE_ID});
    setToolchainConstructor([] { return new SdccToolChain; });
    setUserCreatable(true);
}

QList<ToolChain *> SdccToolChainFactory::autoDetect(const QList<ToolChain *> &alreadyKnown)
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
            if (!compilerExists(fn))
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
        const auto env = Environment::systemEnvironment();
        const auto macros = dumpPredefinedMacros(fn, env.toStringList(), {});
        const QString version = guessVersion(macros);
        const Candidate candidate = {fn, version};
        if (!candidates.contains(candidate))
            candidates.push_back(candidate);
    }

    return autoDetectToolchains(candidates, alreadyKnown);
}

QList<ToolChain *> SdccToolChainFactory::autoDetectToolchains(
        const Candidates &candidates, const QList<ToolChain *> &alreadyKnown) const
{
    QList<ToolChain *> result;

    for (const Candidate &candidate : qAsConst(candidates)) {
        const QList<ToolChain *> filtered = Utils::filtered(
                    alreadyKnown, [candidate](ToolChain *tc) {
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

QList<ToolChain *> SdccToolChainFactory::autoDetectToolchain(
        const Candidate &candidate, Utils::Id language) const
{
    const auto env = Environment::systemEnvironment();

    // Table of supported ABI's by SDCC compiler.
    const Abi knownAbis[] = {
        {Abi::Mcs51Architecture},
        {Abi::Stm8Architecture}
    };

    QList<ToolChain *> tcs;

    // Probe each ABI from the table, because the SDCC compiler
    // can be compiled with or without the specified architecture.
    for (const auto &knownAbi : knownAbis) {
        const Macros macros = dumpPredefinedMacros(candidate.compilerPath,
                                                   env.toStringList(), knownAbi);
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
    m_mainLayout->addRow(tr("&Compiler path:"), m_compilerCommand);
    m_mainLayout->addRow(tr("&ABI:"), m_abiWidget);

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
    const bool haveCompiler = compilerExists(m_compilerCommand->filePath());
    m_abiWidget->setEnabled(haveCompiler && !tc->isAutoDetected());
}

void SdccToolChainConfigWidget::handleCompilerCommandChange()
{
    const FilePath compilerPath = m_compilerCommand->filePath();
    const bool haveCompiler = compilerExists(compilerPath);
    if (haveCompiler) {
        const auto env = Environment::systemEnvironment();
        m_macros = dumpPredefinedMacros(compilerPath, env.toStringList(), {});
        const Abi guessed = guessAbi(m_macros);
        m_abiWidget->setAbis({}, guessed);
    }

    m_abiWidget->setEnabled(haveCompiler);
    emit dirty();
}

} // namespace Internal
} // namespace BareMetal
