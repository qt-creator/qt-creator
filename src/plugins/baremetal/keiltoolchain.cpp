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

#include "keilparser.h"
#include "keiltoolchain.h"

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
#include <QFileInfo>
#include <QFormLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSettings>
#include <QTemporaryFile>
#include <QTextStream>

#include <array>

using namespace ProjectExplorer;
using namespace Utils;

namespace BareMetal {
namespace Internal {

// Helpers:

static const char compilerCommandKeyC[] = "BareMetal.KeilToolchain.CompilerPath";
static const char targetAbiKeyC[] = "BareMetal.KeilToolchain.TargetAbi";

static bool compilerExists(const FilePath &compilerPath)
{
    const QFileInfo fi = compilerPath.toFileInfo();
    return fi.exists() && fi.isExecutable() && fi.isFile();
}

static Abi::Architecture guessArchitecture(const FilePath &compilerPath)
{
    const QFileInfo fi = compilerPath.toFileInfo();
    const QString bn = fi.baseName().toLower();
    if (bn == "c51" || bn == "cx51")
        return Abi::Architecture::Mcs51Architecture;
    if (bn == "armcc")
        return Abi::Architecture::ArmArchitecture;
    return Abi::Architecture::UnknownArchitecture;
}

// Note: The KEIL 8051 compiler does not support the predefined
// macros dumping. So, we do it with following trick where we try
// to compile a temporary file and to parse the console output.
static Macros dumpC51PredefinedMacros(const FilePath &compiler, const QStringList &env)
{
    QTemporaryFile fakeIn;
    if (!fakeIn.open())
        return {};
    fakeIn.write("#define VALUE_TO_STRING(x) #x\n");
    fakeIn.write("#define VALUE(x) VALUE_TO_STRING(x)\n");
    fakeIn.write("#define VAR_NAME_VALUE(var) \"\"\"|\"#var\"|\"VALUE(var)\n");
    fakeIn.write("#ifdef __C51__\n");
    fakeIn.write("#pragma message(VAR_NAME_VALUE(__C51__))\n");
    fakeIn.write("#endif\n");
    fakeIn.write("#ifdef __CX51__\n");
    fakeIn.write("#pragma message(VAR_NAME_VALUE(__CX51__))\n");
    fakeIn.write("#endif\n");
    fakeIn.close();

    SynchronousProcess cpp;
    cpp.setEnvironment(env);
    cpp.setTimeoutS(10);

    const CommandLine cmd(compiler, {fakeIn.fileName()});

    const SynchronousProcessResponse response = cpp.runBlocking(cmd);
    if (response.result != SynchronousProcessResponse::Finished
            || response.exitCode != 0) {
        qWarning() << response.exitMessage(cmd.toUserOutput(), 10);
        return {};
    }

    QString output = response.allOutput();
    Macros macros;
    QTextStream stream(&output);
    QString line;
    while (stream.readLineInto(&line)) {
        const QStringList parts = line.split("\"|\"");
        if (parts.count() != 3)
            continue;
        macros.push_back({parts.at(1).toUtf8(), parts.at(2).toUtf8()});
    }
    return macros;
}

static Macros dumpArmPredefinedMacros(const FilePath &compiler, const QStringList &env)
{
    SynchronousProcess cpp;
    cpp.setEnvironment(env);
    cpp.setTimeoutS(10);

    const CommandLine cmd(compiler, {"-E", "--list-macros"});

    const SynchronousProcessResponse response = cpp.runBlocking(cmd);
    if (response.result != SynchronousProcessResponse::Finished
            || response.exitCode != 0) {
        qWarning() << response.exitMessage(compiler.toString(), 10);
        return {};
    }

    const QByteArray output = response.allOutput().toUtf8();
    return Macro::toMacros(output);
}

static Macros dumpPredefinedMacros(const FilePath &compiler, const QStringList &env)
{
    if (compiler.isEmpty() || !compiler.toFileInfo().isExecutable())
        return {};

    const Abi::Architecture arch = guessArchitecture(compiler);
    switch (arch) {
    case Abi::Architecture::Mcs51Architecture:
        return dumpC51PredefinedMacros(compiler, env);
    case Abi::Architecture::ArmArchitecture:
        return dumpArmPredefinedMacros(compiler, env);
    default:
        return {};
    }
}

static HeaderPaths dumpHeaderPaths(const FilePath &compiler)
{
    if (!compiler.exists())
        return {};

    QDir toolkitDir(compiler.parentDir().toString());
    if (!toolkitDir.cdUp())
        return {};

    HeaderPaths headerPaths;

    const Abi::Architecture arch = guessArchitecture(compiler);
    if (arch == Abi::Architecture::Mcs51Architecture) {
        QDir includeDir(toolkitDir);
        if (includeDir.cd("inc"))
            headerPaths.push_back({includeDir.canonicalPath(), HeaderPathType::BuiltIn});
    } else if (arch == Abi::Architecture::ArmArchitecture) {
        QDir includeDir(toolkitDir);
        if (includeDir.cd("include"))
            headerPaths.push_back({includeDir.canonicalPath(), HeaderPathType::BuiltIn});
    }

    return headerPaths;
}

static Abi::Architecture guessArchitecture(const Macros &macros)
{
    for (const Macro &macro : macros) {
        if (macro.key == "__CC_ARM")
            return Abi::Architecture::ArmArchitecture;
        if (macro.key == "__C51__" || macro.key == "__CX51__")
            return Abi::Architecture::Mcs51Architecture;
    }
    return Abi::Architecture::UnknownArchitecture;
}

static unsigned char guessWordWidth(const Macros &macros, Abi::Architecture arch)
{
    // Check for C51 compiler first.
    if (arch == Abi::Architecture::Mcs51Architecture)
        return 16; // C51 always have 16-bit word width.

    const Macro sizeMacro = Utils::findOrDefault(macros, [](const Macro &m) {
        return m.key == "__sizeof_int";
    });
    if (sizeMacro.isValid() && sizeMacro.type == MacroType::Define)
        return sizeMacro.value.toInt() * 8;
    return 0;
}

static Abi::BinaryFormat guessFormat(Abi::Architecture arch)
{
    if (arch == Abi::Architecture::ArmArchitecture)
        return Abi::BinaryFormat::ElfFormat;
    if (arch == Abi::Architecture::Mcs51Architecture)
        return Abi::BinaryFormat::OmfFormat;
    return Abi::BinaryFormat::UnknownFormat;
}

static Abi guessAbi(const Macros &macros)
{
    const auto arch = guessArchitecture(macros);
    return {arch, Abi::OS::BareMetalOS, Abi::OSFlavor::GenericFlavor,
                guessFormat(arch), guessWordWidth(macros, arch)};
}

static QString buildDisplayName(Abi::Architecture arch, Core::Id language,
                                const QString &version)
{
    const auto archName = Abi::toString(arch);
    const auto langName = ToolChainManager::displayNameOfLanguageId(language);
    return KeilToolchain::tr("KEIL %1 (%2, %3)")
            .arg(version, langName, archName);
}

// KeilToolchain

KeilToolchain::KeilToolchain() :
    ToolChain(Constants::KEIL_TOOLCHAIN_TYPEID)
{
    setTypeDisplayName(Internal::KeilToolchainFactory::tr("KEIL"));
}

void KeilToolchain::setTargetAbi(const Abi &abi)
{
    if (abi == m_targetAbi)
        return;
    m_targetAbi = abi;
    toolChainUpdated();
}

Abi KeilToolchain::targetAbi() const
{
    return m_targetAbi;
}

bool KeilToolchain::isValid() const
{
    return true;
}

ToolChain::MacroInspectionRunner KeilToolchain::createMacroInspectionRunner() const
{
    Environment env = Environment::systemEnvironment();
    addToEnvironment(env);

    const Utils::FilePath compilerCommand = m_compilerCommand;
    const Core::Id lang = language();

    MacrosCache macroCache = predefinedMacrosCache();

    return [env, compilerCommand, macroCache, lang]
            (const QStringList &flags) {
        Q_UNUSED(flags)

        const Macros macros = dumpPredefinedMacros(compilerCommand, env.toStringList());
        const auto report = MacroInspectionReport{macros, languageVersion(lang, macros)};
        macroCache->insert({}, report);

        return report;
    };
}

Macros KeilToolchain::predefinedMacros(const QStringList &cxxflags) const
{
    return createMacroInspectionRunner()(cxxflags).macros;
}

Utils::LanguageExtensions KeilToolchain::languageExtensions(const QStringList &) const
{
    return LanguageExtension::None;
}

WarningFlags KeilToolchain::warningFlags(const QStringList &cxxflags) const
{
    Q_UNUSED(cxxflags)
    return WarningFlags::Default;
}

ToolChain::BuiltInHeaderPathsRunner KeilToolchain::createBuiltInHeaderPathsRunner(
        const Environment &) const
{
    const Utils::FilePath compilerCommand = m_compilerCommand;

    HeaderPathsCache headerPaths = headerPathsCache();

    return [compilerCommand,
            headerPaths](const QStringList &flags, const QString &fileName, const QString &) {
        Q_UNUSED(flags)
        Q_UNUSED(fileName)

        const HeaderPaths paths = dumpHeaderPaths(compilerCommand);
        headerPaths->insert({}, paths);

        return paths;
    };
}

HeaderPaths KeilToolchain::builtInHeaderPaths(const QStringList &cxxFlags,
                                              const FilePath &fileName,
                                              const Environment &env) const
{
    return createBuiltInHeaderPathsRunner(env)(cxxFlags, fileName.toString(), "");
}

void KeilToolchain::addToEnvironment(Environment &env) const
{
    if (!m_compilerCommand.isEmpty()) {
        const FilePath path = m_compilerCommand.parentDir();
        env.prependOrSetPath(path.toString());
    }
}

IOutputParser *KeilToolchain::outputParser() const
{
    return new KeilParser;
}

QVariantMap KeilToolchain::toMap() const
{
    QVariantMap data = ToolChain::toMap();
    data.insert(compilerCommandKeyC, m_compilerCommand.toString());
    data.insert(targetAbiKeyC, m_targetAbi.toString());
    return data;
}

bool KeilToolchain::fromMap(const QVariantMap &data)
{
    if (!ToolChain::fromMap(data))
        return false;
    m_compilerCommand = FilePath::fromString(data.value(compilerCommandKeyC).toString());
    m_targetAbi = Abi::fromString(data.value(targetAbiKeyC).toString());
    return true;
}

std::unique_ptr<ToolChainConfigWidget> KeilToolchain::createConfigurationWidget()
{
    return std::make_unique<KeilToolchainConfigWidget>(this);
}

bool KeilToolchain::operator ==(const ToolChain &other) const
{
    if (!ToolChain::operator ==(other))
        return false;

    const auto customTc = static_cast<const KeilToolchain *>(&other);
    return m_compilerCommand == customTc->m_compilerCommand
            && m_targetAbi == customTc->m_targetAbi
            ;
}

void KeilToolchain::setCompilerCommand(const FilePath &file)
{
    if (file == m_compilerCommand)
        return;
    m_compilerCommand = file;
    toolChainUpdated();
}

FilePath KeilToolchain::compilerCommand() const
{
    return m_compilerCommand;
}

FilePath KeilToolchain::makeCommand(const Environment &env) const
{
    Q_UNUSED(env)
    return {};
}

// KeilToolchainFactory

KeilToolchainFactory::KeilToolchainFactory()
{
    setDisplayName(tr("KEIL"));
    setSupportedToolChainType(Constants::KEIL_TOOLCHAIN_TYPEID);
    setSupportedLanguages({ProjectExplorer::Constants::C_LANGUAGE_ID,
                           ProjectExplorer::Constants::CXX_LANGUAGE_ID});
    setToolchainConstructor([] { return new KeilToolchain; });
    setUserCreatable(true);
}

// Parse the 'tools.ini' file to fetch a toolchain version.
// Note: We can't use QSettings here!
static QString extractVersion(const QString &toolsFile, const QString &section)
{
    QFile f(toolsFile);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    QTextStream in(&f);
    enum State { Enter, Lookup, Exit } state = Enter;
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        // Search for section.
        const int firstBracket = line.indexOf('[');
        const int lastBracket = line.lastIndexOf(']');
        const bool hasSection = (firstBracket == 0 && lastBracket != -1
                && (lastBracket + 1) == line.size());
        switch (state) {
        case Enter:
            if (hasSection) {
                const auto content = line.midRef(firstBracket + 1,
                                                 lastBracket - firstBracket - 1);
                if (content == section)
                    state = Lookup;
            }
            break;
        case Lookup: {
            if (hasSection)
                return {}; // Next section found.
            const int versionIndex = line.indexOf("VERSION=");
            if (versionIndex < 0)
                continue;
            QString version = line.mid(8);
            if (version.startsWith('V'))
                version.remove(0, 1);
            return version;
        }
            break;
        default:
            return {};
        }
    }
    return {};
}

QList<ToolChain *> KeilToolchainFactory::autoDetect(const QList<ToolChain *> &alreadyKnown)
{
#ifdef Q_OS_WIN64
    static const char kRegistryNode[] = "HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\" \
                                        "Windows\\CurrentVersion\\Uninstall\\Keil µVision4";
#else
    static const char kRegistryNode[] = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\" \
                                        "Windows\\CurrentVersion\\Uninstall\\Keil µVision4";
#endif

    Candidates candidates;

    QSettings registry(kRegistryNode, QSettings::NativeFormat);
    const auto productGroups = registry.childGroups();
    for (const QString &productKey : productGroups) {
        if (!productKey.startsWith("App"))
            continue;
        registry.beginGroup(productKey);
        const FilePath productPath(FilePath::fromString(registry.value("ProductDir")
                                                        .toString()));
        // Fetch the toolchain executable path.
        FilePath compilerPath;
        if (productPath.endsWith("ARM"))
            compilerPath = productPath.pathAppended("\\ARMCC\\bin\\armcc.exe");
        else if (productPath.endsWith("C51"))
            compilerPath = productPath.pathAppended("\\BIN\\c51.exe");

        if (compilerPath.exists()) {
            // Fetch the toolchain version.
            const QDir rootDir(registry.value("Directory").toString());
            const QString toolsFilePath = rootDir.absoluteFilePath("tools.ini");
            for (auto index = 1; index <= 2; ++index) {
                const QString section = registry.value(
                            QStringLiteral("Section %1").arg(index)).toString();
                const QString version = extractVersion(toolsFilePath, section);
                if (!version.isEmpty()) {
                    candidates.push_back({compilerPath, version});
                    break;
                }
            }
        }
        registry.endGroup();
    }

    return autoDetectToolchains(candidates, alreadyKnown);
}

QList<ToolChain *> KeilToolchainFactory::autoDetectToolchains(
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

QList<ToolChain *> KeilToolchainFactory::autoDetectToolchain(
        const Candidate &candidate, Core::Id language) const
{
    const auto env = Environment::systemEnvironment();
    const Macros macros = dumpPredefinedMacros(candidate.compilerPath, env.toStringList());
    if (macros.isEmpty())
        return {};

    const Abi abi = guessAbi(macros);
    const Abi::Architecture arch = abi.architecture();
    if (arch == Abi::Architecture::Mcs51Architecture
            && language == ProjectExplorer::Constants::CXX_LANGUAGE_ID) {
        // KEIL C51 compiler does not support C++ language.
        return {};
    }

    const auto tc = new KeilToolchain;
    tc->setDetection(ToolChain::AutoDetection);
    tc->setLanguage(language);
    tc->setCompilerCommand(candidate.compilerPath);
    tc->setTargetAbi(abi);
    tc->setDisplayName(buildDisplayName(abi.architecture(), language, candidate.compilerVersion));

    const auto languageVersion = ToolChain::languageVersion(language, macros);
    tc->predefinedMacrosCache()->insert({}, {macros, languageVersion});
    return {tc};
}

// KeilToolchainConfigWidget

KeilToolchainConfigWidget::KeilToolchainConfigWidget(KeilToolchain *tc) :
    ToolChainConfigWidget(tc),
    m_compilerCommand(new PathChooser),
    m_abiWidget(new AbiWidget)
{
    m_compilerCommand->setExpectedKind(PathChooser::ExistingCommand);
    m_compilerCommand->setHistoryCompleter("PE.KEIL.Command.History");
    m_mainLayout->addRow(tr("&Compiler path:"), m_compilerCommand);
    m_mainLayout->addRow(tr("&ABI:"), m_abiWidget);

    m_abiWidget->setEnabled(false);

    addErrorLabel();
    setFromToolchain();

    connect(m_compilerCommand, &PathChooser::rawPathChanged,
            this, &KeilToolchainConfigWidget::handleCompilerCommandChange);
    connect(m_abiWidget, &AbiWidget::abiChanged,
            this, &ToolChainConfigWidget::dirty);
}

void KeilToolchainConfigWidget::applyImpl()
{
    if (toolChain()->isAutoDetected())
        return;

    const auto tc = static_cast<KeilToolchain *>(toolChain());
    const QString displayName = tc->displayName();
    tc->setCompilerCommand(m_compilerCommand->fileName());
    tc->setTargetAbi(m_abiWidget->currentAbi());
    tc->setDisplayName(displayName);

    if (m_macros.isEmpty())
        return;

    const auto languageVersion = ToolChain::languageVersion(tc->language(), m_macros);
    tc->predefinedMacrosCache()->insert({}, {m_macros, languageVersion});

    setFromToolchain();
}

bool KeilToolchainConfigWidget::isDirtyImpl() const
{
    const auto tc = static_cast<KeilToolchain *>(toolChain());
    return m_compilerCommand->fileName() != tc->compilerCommand()
            || m_abiWidget->currentAbi() != tc->targetAbi()
            ;
}

void KeilToolchainConfigWidget::makeReadOnlyImpl()
{
    m_compilerCommand->setReadOnly(true);
    m_abiWidget->setEnabled(false);
}

void KeilToolchainConfigWidget::setFromToolchain()
{
    const QSignalBlocker blocker(this);
    const auto tc = static_cast<KeilToolchain *>(toolChain());
    m_compilerCommand->setFileName(tc->compilerCommand());
    m_abiWidget->setAbis({}, tc->targetAbi());
    const bool haveCompiler = compilerExists(m_compilerCommand->fileName());
    m_abiWidget->setEnabled(haveCompiler && !tc->isAutoDetected());
}

void KeilToolchainConfigWidget::handleCompilerCommandChange()
{
    const FilePath compilerPath = m_compilerCommand->fileName();
    const bool haveCompiler = compilerExists(compilerPath);
    if (haveCompiler) {
        const auto env = Environment::systemEnvironment();
        m_macros = dumpPredefinedMacros(compilerPath, env.toStringList());
        const Abi guessed = guessAbi(m_macros);
        m_abiWidget->setAbis({}, guessed);
    }

    m_abiWidget->setEnabled(haveCompiler);
    emit dirty();
}

} // namespace Internal
} // namespace BareMetal
