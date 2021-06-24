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
#include <utils/qtcprocess.h>

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

static const char compilerPlatformCodeGenFlagsKeyC[] = "PlatformCodeGenFlags";

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
    if (bn == "c251")
        return Abi::Architecture::Mcs251Architecture;
    if (bn == "c166")
        return Abi::Architecture::C166Architecture;
    if (bn == "armcc")
        return Abi::Architecture::ArmArchitecture;
    return Abi::Architecture::UnknownArchitecture;
}

static Macros dumpMcsPredefinedMacros(const FilePath &compiler, const Environment &env)
{
    // Note: The KEIL C51 or C251 compiler does not support the predefined
    // macros dumping. So, we do it with the following trick, where we try
    // to create and compile a special temporary file and to parse the console
    // output with the own magic pattern: (""|"key"|"value"|"").

    QTemporaryFile fakeIn;
    if (!fakeIn.open())
        return {};

    fakeIn.write("#define VALUE_TO_STRING(x) #x\n");
    fakeIn.write("#define VALUE(x) VALUE_TO_STRING(x)\n");

    // Prepare for C51 compiler.
    fakeIn.write("#if defined(__C51__) || defined(__CX51__)\n");
    fakeIn.write("#  define VAR_NAME_VALUE(var) \"(\"\"\"\"|\"#var\"|\"VALUE(var)\"|\"\"\"\")\"\n");
    fakeIn.write("#  if defined (__C51__)\n");
    fakeIn.write("#    pragma message (VAR_NAME_VALUE(__C51__))\n");
    fakeIn.write("#  endif\n");
    fakeIn.write("#  if defined(__CX51__)\n");
    fakeIn.write("#    pragma message (VAR_NAME_VALUE(__CX51__))\n");
    fakeIn.write("#  endif\n");
    fakeIn.write("#  if defined(__MODEL__)\n");
    fakeIn.write("#    pragma message (VAR_NAME_VALUE(__MODEL__))\n");
    fakeIn.write("#  endif\n");
    fakeIn.write("#  if defined(__STDC__)\n");
    fakeIn.write("#    pragma message (VAR_NAME_VALUE(__STDC__))\n");
    fakeIn.write("#  endif\n");
    fakeIn.write("#endif\n");

    // Prepare for C251 compiler.
    fakeIn.write("#if defined(__C251__)\n");
    fakeIn.write("#  define VAR_NAME_VALUE(var) \"\"|#var|VALUE(var)|\"\"\n");
    fakeIn.write("#  if defined(__C251__)\n");
    fakeIn.write("#    warning (VAR_NAME_VALUE(__C251__))\n");
    fakeIn.write("#  endif\n");
    fakeIn.write("#  if defined(__MODEL__)\n");
    fakeIn.write("#    warning (VAR_NAME_VALUE(__MODEL__))\n");
    fakeIn.write("#  endif\n");
    fakeIn.write("#  if defined(__STDC__)\n");
    fakeIn.write("#    warning (VAR_NAME_VALUE(__STDC__))\n");
    fakeIn.write("#  endif\n");
    fakeIn.write("#  if defined(__FLOAT64__)\n");
    fakeIn.write("#    warning (VAR_NAME_VALUE(__FLOAT64__))\n");
    fakeIn.write("#  endif\n");
    fakeIn.write("#  if defined(__MODSRC__)\n");
    fakeIn.write("#    warning (VAR_NAME_VALUE(__MODSRC__))\n");
    fakeIn.write("#  endif\n");
    fakeIn.write("#endif\n");

    fakeIn.close();

    QtcProcess cpp;
    cpp.setEnvironment(env);
    cpp.setTimeoutS(10);
    cpp.setCommand({compiler, {fakeIn.fileName()}});

    cpp.runBlocking();
    QString output = cpp.allOutput();
    Macros macros;
    QTextStream stream(&output);
    QString line;
    while (stream.readLineInto(&line)) {
        enum { KEY_INDEX = 1, VALUE_INDEX = 2, ALL_PARTS = 4 };
        const QStringList parts = line.split("\"|\"");
        if (parts.count() != ALL_PARTS)
            continue;
        macros.push_back({parts.at(KEY_INDEX).toUtf8(), parts.at(VALUE_INDEX).toUtf8()});
    }
    return macros;
}

static Macros dumpC166PredefinedMacros(const FilePath &compiler, const Environment &env)
{
    // Note: The KEIL C166 compiler does not support the predefined
    // macros dumping. Also, it does not support the '#pragma' and
    // '#message|warning|error' directives properly (it is impossible
    // to print to console the value of macro).
    // So, we do it with the following trick, where we try
    // to create and compile a special temporary file and to parse the console
    // output with the own magic pattern, e.g:
    //
    // *** WARNING C320 IN LINE 41 OF c51.c: __C166__
    // *** WARNING C2 IN LINE 42 OF c51.c: '757': unknown #pragma/control, line ignored
    //
    // where the '__C166__' is a key, and the '757' is a value.

    QTemporaryFile fakeIn;
    if (!fakeIn.open())
        return {};

    // Prepare for C166 compiler.
    fakeIn.write("#if defined(__C166__)\n");
    fakeIn.write("#  if defined(__C166__)\n");
    fakeIn.write("#   warning __C166__\n");
    fakeIn.write("#   pragma __C166__\n");
    fakeIn.write("#  endif\n");
    fakeIn.write("#  if defined(__DUS__)\n");
    fakeIn.write("#   warning __DUS__\n");
    fakeIn.write("#   pragma __DUS__\n");
    fakeIn.write("#  endif\n");
    fakeIn.write("#  if defined(__MAC__)\n");
    fakeIn.write("#   warning __MAC__\n");
    fakeIn.write("#   pragma __MAC__\n");
    fakeIn.write("#  endif\n");
    fakeIn.write("#  if defined(__MOD167__)\n");
    fakeIn.write("#   warning __MOD167__\n");
    fakeIn.write("#   pragma __MOD167__\n");
    fakeIn.write("#  endif\n");
    fakeIn.write("#  if defined(__MODEL__)\n");
    fakeIn.write("#   warning __MODEL__\n");
    fakeIn.write("#   pragma __MODEL__\n");
    fakeIn.write("#  endif\n");
    fakeIn.write("#  if defined(__MODV2__)\n");
    fakeIn.write("#   warning __MODV2__\n");
    fakeIn.write("#   pragma __MODV2__\n");
    fakeIn.write("#  endif\n");
    fakeIn.write("#  if defined(__SAVEMAC__)\n");
    fakeIn.write("#   warning __SAVEMAC__\n");
    fakeIn.write("#   pragma __SAVEMAC__\n");
    fakeIn.write("#  endif\n");
    fakeIn.write("#  if defined(__STDC__)\n");
    fakeIn.write("#   warning __STDC__\n");
    fakeIn.write("#   pragma __STDC__\n");
    fakeIn.write("#  endif\n");
    fakeIn.write("#endif\n");

    fakeIn.close();

    QtcProcess cpp;
    cpp.setEnvironment(env);
    cpp.setTimeoutS(10);

    Macros macros;
    auto extractMacros = [&macros](const QString &output) {
        const QStringList lines = output.split('\n');
        for (auto it = lines.cbegin(); it != lines.cend();) {
            if (!it->startsWith("***")) {
                ++it;
                continue;
            }

            // Search for the key at a first line.
            QByteArray key;
            if (it->endsWith("__C166__"))
                key = "__C166__";
            else if (it->endsWith("__DUS__"))
                key = "__DUS__";
            else if (it->endsWith("__MAC__"))
                key = "__MAC__";
            else if (it->endsWith("__MOD167__"))
                key = "__MOD167__";
            else if (it->endsWith("__MODEL__"))
                key = "__MODEL__";
            else if (it->endsWith("__MODV2__"))
                key = "__MODV2__";
            else if (it->endsWith("__SAVEMAC__"))
                key = "__SAVEMAC__";
            else if (it->endsWith("__STDC__"))
                key = "__STDC__";

            if (key.isEmpty()) {
                ++it;
                continue;
            }

            ++it;
            if (it == lines.cend() || !it->startsWith("***"))
                break;

            // Search for the value at a second line.
            const int startIndex = it->indexOf('\'');
            if (startIndex == -1)
                break;
            const int stopIndex = it->indexOf('\'', startIndex + 1);
            if (stopIndex == -1)
                break;
            const QByteArray value = it->mid(startIndex + 1, stopIndex - startIndex - 1).toLatin1();

            macros.append(Macro{key, value});

            ++it;
        }
    };

    cpp.setCommand({compiler, {fakeIn.fileName()}});
    cpp.runBlocking();
    const QString output = cpp.allOutput();
    extractMacros(output);
    return macros;
}

static Macros dumpArmPredefinedMacros(const FilePath &compiler, const QStringList &extraArgs, const Environment &env)
{
    QtcProcess cpp;
    cpp.setEnvironment(env);
    cpp.setTimeoutS(10);

    QStringList args = extraArgs;
    args.push_back("-E");
    args.push_back("--list-macros");
    cpp.setCommand({compiler, args});

    cpp.runBlocking();
    if (cpp.result() != QtcProcess::FinishedWithSuccess) {
        qWarning() << cpp.exitMessage();
        return {};
    }

    const QByteArray output = cpp.allOutput().toUtf8();
    return Macro::toMacros(output);
}

static bool isMcsArchitecture(Abi::Architecture arch)
{
    return arch == Abi::Architecture::Mcs51Architecture
            || arch == Abi::Architecture::Mcs251Architecture;
}

static bool isC166Architecture(Abi::Architecture arch)
{
    return arch == Abi::Architecture::C166Architecture;
}

static bool isArmArchitecture(Abi::Architecture arch)
{
    return arch == Abi::Architecture::ArmArchitecture;
}

static Macros dumpPredefinedMacros(const FilePath &compiler, const QStringList &args, const Environment &env)
{
    if (compiler.isEmpty() || !compiler.toFileInfo().isExecutable())
        return {};

    const Abi::Architecture arch = guessArchitecture(compiler);
    if (isMcsArchitecture(arch))
        return dumpMcsPredefinedMacros(compiler, env);
    if (isC166Architecture(arch))
        return dumpC166PredefinedMacros(compiler, env);
    if (isArmArchitecture(arch))
        return dumpArmPredefinedMacros(compiler, args, env);
    return {};
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
    if (isMcsArchitecture(arch) || isC166Architecture(arch)) {
        QDir includeDir(toolkitDir);
        if (includeDir.cd("inc"))
            headerPaths.push_back({includeDir.canonicalPath(), HeaderPathType::BuiltIn});
    } else if (isArmArchitecture(arch)) {
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
        if (macro.key == "__C251__")
            return Abi::Architecture::Mcs251Architecture;
        if (macro.key == "__C166__")
            return Abi::Architecture::C166Architecture;
    }
    return Abi::Architecture::UnknownArchitecture;
}

static unsigned char guessWordWidth(const Macros &macros, Abi::Architecture arch)
{
    // Check for C51 or C251 compiler first, which are always have 16-bit word width:
    // * http://www.keil.com/support/man/docs/c51/c51_le_datatypes.htm
    // * http://www.keil.com/support/man/docs/c251/c251_le_datatypes.htm
    // * http://www.keil.com/support/man/docs/c166/c166_le_datatypes.htm
    if (isMcsArchitecture(arch) || isC166Architecture(arch))
        return 16;

    const Macro sizeMacro = Utils::findOrDefault(macros, [](const Macro &m) {
        return m.key == "__sizeof_int";
    });
    if (sizeMacro.isValid() && sizeMacro.type == MacroType::Define)
        return sizeMacro.value.toInt() * 8;
    return 0;
}

static Abi::BinaryFormat guessFormat(Abi::Architecture arch)
{
    if (isArmArchitecture(arch))
        return Abi::BinaryFormat::ElfFormat;
    if (isMcsArchitecture(arch) || isC166Architecture(arch))
        return Abi::BinaryFormat::OmfFormat;
    return Abi::BinaryFormat::UnknownFormat;
}

static Abi guessAbi(const Macros &macros)
{
    const auto arch = guessArchitecture(macros);
    return {arch, Abi::OS::BareMetalOS, Abi::OSFlavor::GenericFlavor,
                guessFormat(arch), guessWordWidth(macros, arch)};
}

static QString buildDisplayName(Abi::Architecture arch, Utils::Id language,
                                const QString &version)
{
    const auto archName = Abi::toString(arch);
    const auto langName = ToolChainManager::displayNameOfLanguageId(language);
    return KeilToolChain::tr("KEIL %1 (%2, %3)")
            .arg(version, langName, archName);
}

static void addDefaultCpuArgs(const FilePath &compiler, QStringList &extraArgs)
{
    const Abi::Architecture arch = guessArchitecture(compiler);
    if (!isArmArchitecture(arch))
        return;

    const auto extraArgsIt = std::find_if(std::begin(extraArgs), std::end(extraArgs),
                                          [](const QString &extraArg) {
        return extraArg.contains("-cpu") || extraArg.contains("--cpu");
    });
    if (extraArgsIt == std::end(extraArgs))
        extraArgs.push_back("--cpu=cortex-m0");
}

// KeilToolchain

KeilToolChain::KeilToolChain() :
    ToolChain(Constants::KEIL_TOOLCHAIN_TYPEID)
{
    setTypeDisplayName(tr("KEIL"));
    setTargetAbiKey("TargetAbi");
    setCompilerCommandKey("CompilerPath");
}

ToolChain::MacroInspectionRunner KeilToolChain::createMacroInspectionRunner() const
{
    Environment env = Environment::systemEnvironment();
    addToEnvironment(env);

    const FilePath compiler = compilerCommand();
    const Id lang = language();

    MacrosCache macroCache = predefinedMacrosCache();
    const QStringList extraArgs = m_extraCodeModelFlags;

    return [env, compiler, extraArgs, macroCache, lang](const QStringList &flags) {
        Q_UNUSED(flags)

        const Macros macros = dumpPredefinedMacros(compiler, extraArgs, env);
        const auto report = MacroInspectionReport{macros, languageVersion(lang, macros)};
        macroCache->insert({}, report);

        return report;
    };
}

Utils::LanguageExtensions KeilToolChain::languageExtensions(const QStringList &) const
{
    return LanguageExtension::None;
}

WarningFlags KeilToolChain::warningFlags(const QStringList &cxxflags) const
{
    Q_UNUSED(cxxflags)
    return WarningFlags::Default;
}

ToolChain::BuiltInHeaderPathsRunner KeilToolChain::createBuiltInHeaderPathsRunner(
        const Environment &) const
{
    const FilePath compiler = compilerCommand();
    const HeaderPathsCache headerPaths = headerPathsCache();

    return [compiler,
            headerPaths](const QStringList &flags, const QString &fileName, const QString &) {
        Q_UNUSED(flags)
        Q_UNUSED(fileName)

        const HeaderPaths paths = dumpHeaderPaths(compiler);
        headerPaths->insert({}, paths);

        return paths;
    };
}

void KeilToolChain::addToEnvironment(Environment &env) const
{
    if (!compilerCommand().isEmpty()) {
        const FilePath path = compilerCommand().parentDir();
        env.prependOrSetPath(path.toString());
    }
}

QList<OutputLineParser *> KeilToolChain::createOutputParsers() const
{
    return {new KeilParser};
}

QVariantMap KeilToolChain::toMap() const
{
    QVariantMap data = ToolChain::toMap();
    data.insert(compilerPlatformCodeGenFlagsKeyC, m_extraCodeModelFlags);
    return data;
}

bool KeilToolChain::fromMap(const QVariantMap &data)
{
    if (!ToolChain::fromMap(data))
        return false;
    m_extraCodeModelFlags = data.value(compilerPlatformCodeGenFlagsKeyC).toStringList();
    return true;
}

std::unique_ptr<ToolChainConfigWidget> KeilToolChain::createConfigurationWidget()
{
    return std::make_unique<KeilToolChainConfigWidget>(this);
}

bool KeilToolChain::operator ==(const ToolChain &other) const
{
    if (!ToolChain::operator ==(other))
        return false;

    const auto customTc = static_cast<const KeilToolChain *>(&other);
    return compilerCommand() == customTc->compilerCommand()
            && targetAbi() == customTc->targetAbi()
            && m_extraCodeModelFlags == customTc->m_extraCodeModelFlags;
}

void KeilToolChain::setExtraCodeModelFlags(const QStringList &flags)
{
    if (flags == m_extraCodeModelFlags)
        return;
    m_extraCodeModelFlags = flags;
    toolChainUpdated();
}

QStringList KeilToolChain::extraCodeModelFlags() const
{
    return m_extraCodeModelFlags;
}

FilePath KeilToolChain::makeCommand(const Environment &env) const
{
    Q_UNUSED(env)
    return {};
}

// KeilToolchainFactory

KeilToolChainFactory::KeilToolChainFactory()
{
    setDisplayName(KeilToolChain::tr("KEIL"));
    setSupportedToolChainType(Constants::KEIL_TOOLCHAIN_TYPEID);
    setSupportedLanguages({ProjectExplorer::Constants::C_LANGUAGE_ID,
                           ProjectExplorer::Constants::CXX_LANGUAGE_ID});
    setToolchainConstructor([] { return new KeilToolChain; });
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
                const auto content = QStringView(line).mid(firstBracket + 1,
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

QList<ToolChain *> KeilToolChainFactory::autoDetect(const QList<ToolChain *> &alreadyKnown,
                                                    const IDevice::Ptr &device)
{
    Q_UNUSED(device)
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
            compilerPath = productPath.pathAppended("ARMCC/bin/armcc.exe");
        else if (productPath.endsWith("C51"))
            compilerPath = productPath.pathAppended("BIN/c51.exe");
        else if (productPath.endsWith("C251"))
            compilerPath = productPath.pathAppended("BIN/c251.exe");
        else if (productPath.endsWith("C166"))
            compilerPath = productPath.pathAppended("BIN/c166.exe");

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

QList<ToolChain *> KeilToolChainFactory::autoDetectToolchains(
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

QList<ToolChain *> KeilToolChainFactory::autoDetectToolchain(
        const Candidate &candidate, Utils::Id language) const
{
    const auto env = Environment::systemEnvironment();

    QStringList extraArgs;
    addDefaultCpuArgs(candidate.compilerPath, extraArgs);
    const Macros macros = dumpPredefinedMacros(candidate.compilerPath, extraArgs, env);
    if (macros.isEmpty())
        return {};

    const Abi abi = guessAbi(macros);
    const Abi::Architecture arch = abi.architecture();
    if ((isMcsArchitecture(arch) || isC166Architecture(arch))
            && language == ProjectExplorer::Constants::CXX_LANGUAGE_ID) {
        // KEIL C51/C251/C166 compilers does not support C++ language.
        return {};
    }

    const auto tc = new KeilToolChain;
    tc->setDetection(ToolChain::AutoDetection);
    tc->setLanguage(language);
    tc->setCompilerCommand(candidate.compilerPath);
    tc->setExtraCodeModelFlags(extraArgs);
    tc->setTargetAbi(abi);
    tc->setDisplayName(buildDisplayName(abi.architecture(), language, candidate.compilerVersion));

    const auto languageVersion = ToolChain::languageVersion(language, macros);
    tc->predefinedMacrosCache()->insert({}, {macros, languageVersion});
    return {tc};
}

// KeilToolchainConfigWidget

KeilToolChainConfigWidget::KeilToolChainConfigWidget(KeilToolChain *tc) :
    ToolChainConfigWidget(tc),
    m_compilerCommand(new PathChooser),
    m_abiWidget(new AbiWidget)
{
    m_compilerCommand->setExpectedKind(PathChooser::ExistingCommand);
    m_compilerCommand->setHistoryCompleter("PE.KEIL.Command.History");
    m_mainLayout->addRow(tr("&Compiler path:"), m_compilerCommand);
    m_platformCodeGenFlagsLineEdit = new QLineEdit(this);
    m_platformCodeGenFlagsLineEdit->setText(ProcessArgs::joinArgs(tc->extraCodeModelFlags()));
    m_mainLayout->addRow(tr("Platform codegen flags:"), m_platformCodeGenFlagsLineEdit);
    m_mainLayout->addRow(tr("&ABI:"), m_abiWidget);

    m_abiWidget->setEnabled(false);

    addErrorLabel();
    setFromToolChain();

    connect(m_compilerCommand, &PathChooser::rawPathChanged,
            this, &KeilToolChainConfigWidget::handleCompilerCommandChange);
    connect(m_platformCodeGenFlagsLineEdit, &QLineEdit::editingFinished,
            this, &KeilToolChainConfigWidget::handlePlatformCodeGenFlagsChange);
    connect(m_abiWidget, &AbiWidget::abiChanged,
            this, &ToolChainConfigWidget::dirty);
}

void KeilToolChainConfigWidget::applyImpl()
{
    if (toolChain()->isAutoDetected())
        return;

    const auto tc = static_cast<KeilToolChain *>(toolChain());
    const QString displayName = tc->displayName();
    tc->setCompilerCommand(m_compilerCommand->filePath());
    tc->setExtraCodeModelFlags(splitString(m_platformCodeGenFlagsLineEdit->text()));
    tc->setTargetAbi(m_abiWidget->currentAbi());
    tc->setDisplayName(displayName);

    if (m_macros.isEmpty())
        return;

    const auto languageVersion = ToolChain::languageVersion(tc->language(), m_macros);
    tc->predefinedMacrosCache()->insert({}, {m_macros, languageVersion});

    setFromToolChain();
}

bool KeilToolChainConfigWidget::isDirtyImpl() const
{
    const auto tc = static_cast<KeilToolChain *>(toolChain());
    return m_compilerCommand->filePath() != tc->compilerCommand()
            || m_platformCodeGenFlagsLineEdit->text() != ProcessArgs::joinArgs(tc->extraCodeModelFlags())
            || m_abiWidget->currentAbi() != tc->targetAbi()
            ;
}

void KeilToolChainConfigWidget::makeReadOnlyImpl()
{
    m_compilerCommand->setReadOnly(true);
    m_platformCodeGenFlagsLineEdit->setEnabled(false);
    m_abiWidget->setEnabled(false);
}

void KeilToolChainConfigWidget::setFromToolChain()
{
    const QSignalBlocker blocker(this);
    const auto tc = static_cast<KeilToolChain *>(toolChain());
    m_compilerCommand->setFilePath(tc->compilerCommand());
    m_platformCodeGenFlagsLineEdit->setText(ProcessArgs::joinArgs(tc->extraCodeModelFlags()));
    m_abiWidget->setAbis({}, tc->targetAbi());
    const bool haveCompiler = compilerExists(m_compilerCommand->filePath());
    m_abiWidget->setEnabled(haveCompiler && !tc->isAutoDetected());
}

void KeilToolChainConfigWidget::handleCompilerCommandChange()
{
    const FilePath compilerPath = m_compilerCommand->filePath();
    const bool haveCompiler = compilerExists(compilerPath);
    if (haveCompiler) {
        const auto env = Environment::systemEnvironment();
        const QStringList prevExtraArgs = splitString(m_platformCodeGenFlagsLineEdit->text());
        QStringList newExtraArgs = prevExtraArgs;
        addDefaultCpuArgs(compilerPath, newExtraArgs);
        if (prevExtraArgs != newExtraArgs)
            m_platformCodeGenFlagsLineEdit->setText(ProcessArgs::joinArgs(newExtraArgs));
        m_macros = dumpPredefinedMacros(compilerPath, newExtraArgs, env);
        const Abi guessed = guessAbi(m_macros);
        m_abiWidget->setAbis({}, guessed);
    }

    m_abiWidget->setEnabled(haveCompiler);
    emit dirty();
}

void KeilToolChainConfigWidget::handlePlatformCodeGenFlagsChange()
{
    const QString str1 = m_platformCodeGenFlagsLineEdit->text();
    const QString str2 = ProcessArgs::joinArgs(splitString(str1));
    if (str1 != str2)
        m_platformCodeGenFlagsLineEdit->setText(str2);
    else
        handleCompilerCommandChange();
}

} // namespace Internal
} // namespace BareMetal
