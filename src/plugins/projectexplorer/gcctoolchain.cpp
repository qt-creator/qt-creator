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

#include "gcctoolchain.h"
#include "clangparser.h"
#include "gcctoolchainfactories.h"
#include "gccparser.h"
#include "linuxiccparser.h"
#include "projectmacro.h"
#include "toolchainmanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/synchronousprocess.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/synchronousprocess.h>

#include <QBuffer>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QLineEdit>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QTimer>

#include <memory>

namespace {
static Q_LOGGING_CATEGORY(gccLog, "qtc.projectexplorer.toolchain.gcc", QtWarningMsg);
} // namespace

using namespace Utils;

namespace ProjectExplorer {

using namespace Internal;

// --------------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------------

static const char compilerCommandKeyC[] = "ProjectExplorer.GccToolChain.Path";
static const char compilerPlatformCodeGenFlagsKeyC[] = "ProjectExplorer.GccToolChain.PlatformCodeGenFlags";
static const char compilerPlatformLinkerFlagsKeyC[] = "ProjectExplorer.GccToolChain.PlatformLinkerFlags";
static const char targetAbiKeyC[] = "ProjectExplorer.GccToolChain.TargetAbi";
static const char originalTargetTripleKeyC[] = "ProjectExplorer.GccToolChain.OriginalTargetTriple";
static const char supportedAbisKeyC[] = "ProjectExplorer.GccToolChain.SupportedAbis";
static const char parentToolChainIdKeyC[] = "ProjectExplorer.ClangToolChain.ParentToolChainId";
static const char binaryRegexp[] = "(?:^|-|\\b)(?:gcc|g\\+\\+|clang(?:\\+\\+)?)(?:-([\\d.]+))?$";

static QByteArray runGcc(const FilePath &gcc, const QStringList &arguments, const QStringList &env)
{
    if (gcc.isEmpty() || !gcc.toFileInfo().isExecutable())
        return QByteArray();

    SynchronousProcess cpp;
    QStringList environment(env);
    Utils::Environment::setupEnglishOutput(&environment);

    cpp.setEnvironment(environment);
    cpp.setTimeoutS(10);
    CommandLine cmdLine(gcc, arguments);
    SynchronousProcessResponse response =  cpp.runBlocking(cmdLine);
    if (response.result != SynchronousProcessResponse::Finished ||
            response.exitCode != 0) {
        Core::MessageManager::writeMessages({"Compiler feature detection failure!",
                                             response.exitMessage(cmdLine.toUserOutput(), 10),
                                             QString::fromUtf8(response.allRawOutput())});
        return QByteArray();
    }

    return response.allOutput().toUtf8();
}

static ProjectExplorer::Macros gccPredefinedMacros(const FilePath &gcc,
                                                   const QStringList &args,
                                                   const QStringList &env)
{
    QStringList arguments = args;
    arguments << "-";

    ProjectExplorer::Macros  predefinedMacros = Macro::toMacros(runGcc(gcc, arguments, env));
    // Sanity check in case we get an error message instead of real output:
    QTC_CHECK(predefinedMacros.isEmpty()
              || predefinedMacros.front().type == ProjectExplorer::MacroType::Define);
    if (HostOsInfo::isMacHost()) {
        // Turn off flag indicating Apple's blocks support
        const ProjectExplorer::Macro blocksDefine("__BLOCKS__", "1");
        const ProjectExplorer::Macro blocksUndefine("__BLOCKS__", ProjectExplorer::MacroType::Undefine);
        const int idx = predefinedMacros.indexOf(blocksDefine);
        if (idx != -1)
            predefinedMacros[idx] = blocksUndefine;

        // Define __strong and __weak (used for Apple's GC extension of C) to be empty
        predefinedMacros.append({"__strong"});
        predefinedMacros.append({"__weak"});
    }
    return predefinedMacros;
}

HeaderPaths GccToolChain::gccHeaderPaths(const FilePath &gcc, const QStringList &arguments,
                                         const QStringList &env)
{
    HeaderPaths builtInHeaderPaths;
    QByteArray line;
    QByteArray data = runGcc(gcc, arguments, env);
    QBuffer cpp(&data);
    cpp.open(QIODevice::ReadOnly);
    while (cpp.canReadLine()) {
        line = cpp.readLine();
        if (line.startsWith("#include"))
            break;
    }

    if (!line.isEmpty() && line.startsWith("#include")) {
        auto kind = HeaderPathType::User;
        while (cpp.canReadLine()) {
            line = cpp.readLine();
            if (line.startsWith("#include")) {
                kind = HeaderPathType::BuiltIn;
            } else if (! line.isEmpty() && QChar(line.at(0)).isSpace()) {
                HeaderPathType thisHeaderKind = kind;

                line = line.trimmed();

                const int index = line.indexOf(" (framework directory)");
                if (index != -1) {
                    line.truncate(index);
                    thisHeaderKind = HeaderPathType::Framework;
                }

                const QString headerPath = QFileInfo(QFile::decodeName(line)).canonicalFilePath();
                builtInHeaderPaths.append({headerPath, thisHeaderKind});
            } else if (line.startsWith("End of search list.")) {
                break;
            } else {
                qWarning("%s: Ignoring line: %s", __FUNCTION__, line.constData());
            }
        }
    }
    return builtInHeaderPaths;
}

static Abis guessGccAbi(const QString &m, const ProjectExplorer::Macros &macros)
{
    Abis abiList;

    Abi guessed = Abi::abiFromTargetTriplet(m);
    if (guessed.isNull())
        return abiList;

    Abi::Architecture arch = guessed.architecture();
    Abi::OS os = guessed.os();
    Abi::OSFlavor flavor = guessed.osFlavor();
    Abi::BinaryFormat format = guessed.binaryFormat();
    int width = guessed.wordWidth();

    const Macro sizeOfMacro = Utils::findOrDefault(macros, [](const Macro &m) { return m.key == "__SIZEOF_SIZE_T__"; });
    if (sizeOfMacro.isValid() && sizeOfMacro.type == MacroType::Define)
        width = sizeOfMacro.value.toInt() * 8;
    const Macro &mscVerMacro = Utils::findOrDefault(macros, [](const Macro &m) { return m.key == "_MSC_VER"; });
    if (mscVerMacro.type == MacroType::Define) {
        const int msvcVersion = mscVerMacro.value.toInt();
        flavor = Abi::flavorForMsvcVersion(msvcVersion);
    }

    if (os == Abi::DarwinOS) {
        // Apple does PPC and x86!
        abiList << Abi(arch, os, flavor, format, width);
        abiList << Abi(arch, os, flavor, format, width == 64 ? 32 : 64);
    } else if (arch == Abi::X86Architecture && (width == 0 || width == 64)) {
        abiList << Abi(arch, os, flavor, format, 64);
        if (width != 64 || (!m.contains("mingw")
                            && ToolChainManager::detectionSettings().detectX64AsX32)) {
            abiList << Abi(arch, os, flavor, format, 32);
        }
    } else {
        abiList << Abi(arch, os, flavor, format, width);
    }
    return abiList;
}


static GccToolChain::DetectedAbisResult guessGccAbi(const FilePath &path, const QStringList &env,
                                                   const ProjectExplorer::Macros &macros,
                                                   const QStringList &extraArgs = QStringList())
{
    if (path.isEmpty())
        return GccToolChain::DetectedAbisResult();

    QStringList arguments = extraArgs;
    arguments << "-dumpmachine";
    QString machine = QString::fromLocal8Bit(runGcc(path, arguments, env)).trimmed();
    if (machine.isEmpty()) {
        // ICC does not implement the -dumpmachine option on macOS.
        if (HostOsInfo::isMacHost() && (path.fileName() == "icc" || path.fileName() == "icpc"))
            return GccToolChain::DetectedAbisResult({Abi::hostAbi()});
        return GccToolChain::DetectedAbisResult(); // no need to continue if running failed once...
    }
    return GccToolChain::DetectedAbisResult(guessGccAbi(machine, macros), machine);
}

static QString gccVersion(const FilePath &path, const QStringList &env,
                          const QStringList &extraArgs)
{
    QStringList arguments = extraArgs;
    arguments << "-dumpversion";
    return QString::fromLocal8Bit(runGcc(path, arguments, env)).trimmed();
}

static Utils::FilePath gccInstallDir(const FilePath &path, const QStringList &env,
                                     const QStringList &extraArgs = {})
{
    QStringList arguments = extraArgs;
    arguments << "-print-search-dirs";
    QString output = QString::fromLocal8Bit(runGcc(path, arguments, env)).trimmed();
    // Expected output looks like this:
    //   install: /usr/lib/gcc/x86_64-linux-gnu/7/
    //   ...
    // Note that clang also supports "-print-search-dirs". However, the
    // install dir is not part of the output (tested with clang-8/clang-9).

    const QString prefix = "install: ";
    const QString line = QTextStream(&output).readLine();
    if (!line.startsWith(prefix))
        return {};
    return Utils::FilePath::fromString(QDir::cleanPath(line.mid(prefix.size())));
}

// --------------------------------------------------------------------------
// GccToolChain
// --------------------------------------------------------------------------

GccToolChain::GccToolChain(Utils::Id typeId) :
    ToolChain(typeId)
{
    setTypeDisplayName(tr("GCC"));
}

void GccToolChain::setCompilerCommand(const FilePath &path)
{
    if (path == m_compilerCommand)
        return;

    m_compilerCommand = path;
    toolChainUpdated();
}

void GccToolChain::setSupportedAbis(const Abis &abis)
{
    if (m_supportedAbis == abis)
        return;

    m_supportedAbis = abis;
    toolChainUpdated();
}

void GccToolChain::setOriginalTargetTriple(const QString &targetTriple)
{
    if (m_originalTargetTriple == targetTriple)
        return;

    m_originalTargetTriple = targetTriple;
    toolChainUpdated();
}

void GccToolChain::setInstallDir(const Utils::FilePath &installDir)
{
    if (m_installDir == installDir)
        return;

    m_installDir = installDir;
    toolChainUpdated();
}

QString GccToolChain::defaultDisplayName() const
{
    QString type = typeDisplayName();
    const QRegularExpression regexp(binaryRegexp);
    const QRegularExpressionMatch match = regexp.match(m_compilerCommand.fileName());
    if (match.lastCapturedIndex() >= 1)
        type += ' ' + match.captured(1);
    if (m_targetAbi.architecture() == Abi::UnknownArchitecture || m_targetAbi.wordWidth() == 0)
        return type;
    return QCoreApplication::translate("ProjectExplorer::GccToolChain",
                                       "%1 (%2, %3 %4 in %5)").arg(type,
                                                                  ToolChainManager::displayNameOfLanguageId(language()),
                                                                  Abi::toString(m_targetAbi.architecture()),
                                                                  Abi::toString(m_targetAbi.wordWidth()),
                                                                  compilerCommand().parentDir().toUserOutput());
}

LanguageExtensions GccToolChain::defaultLanguageExtensions() const
{
    return LanguageExtension::Gnu;
}

Abi GccToolChain::targetAbi() const
{
    return m_targetAbi;
}

QString GccToolChain::originalTargetTriple() const
{
    if (m_originalTargetTriple.isEmpty())
        m_originalTargetTriple = detectSupportedAbis().originalTargetTriple;
    return m_originalTargetTriple;
}

QString GccToolChain::version() const
{
    if (m_version.isEmpty())
        m_version = detectVersion();
    return m_version;
}

FilePath GccToolChain::installDir() const
{
    if (m_installDir.isEmpty())
        m_installDir = detectInstallDir();
    return m_installDir;
}

void GccToolChain::setTargetAbi(const Abi &abi)
{
    if (abi == m_targetAbi)
        return;

    m_targetAbi = abi;
    toolChainUpdated();
}

Abis GccToolChain::supportedAbis() const
{
    return m_supportedAbis;
}

bool GccToolChain::isValid() const
{
    if (m_compilerCommand.isEmpty())
        return false;

    QFileInfo fi = compilerCommand().toFileInfo();
    return fi.isExecutable();
}

static bool isNetworkCompiler(const QString &dirPath)
{
    return dirPath.contains("icecc") || dirPath.contains("distcc");
}

static Utils::FilePath findLocalCompiler(const Utils::FilePath &compilerPath,
                                         const Environment &env)
{
    // Find the "real" compiler if icecc, distcc or similar are in use. Ignore ccache, since that
    // is local already.

    // Get the path to the compiler, ignoring direct calls to icecc and distcc as we cannot
    // do anything about those.
    if (!isNetworkCompiler(compilerPath.parentDir().toString()))
        return compilerPath;

    // Filter out network compilers
    const FilePaths pathComponents = Utils::filtered(env.path(), [] (const FilePath &dirPath) {
        return !isNetworkCompiler(dirPath.toString());
    });

    // This effectively searches the PATH twice, once via pathComponents and once via PATH itself:
    // searchInPath filters duplicates, so that will not hurt.
    const Utils::FilePath path = env.searchInPath(compilerPath.fileName(), pathComponents);

    return path.isEmpty() ? compilerPath : path;
}

// For querying operations such as -dM
static QStringList filteredFlags(const QStringList &allFlags, bool considerSysroot)
{
    QStringList filtered;
    for (int i = 0; i < allFlags.size(); ++i) {
        const QString &a = allFlags.at(i);
        if (a.startsWith("--gcc-toolchain=")) {
            filtered << a;
        } else if (a == "-arch") {
            if (++i < allFlags.length() && !filtered.contains(a))
                filtered << a << allFlags.at(i);
        } else if ((considerSysroot && (a == "--sysroot" || a == "-isysroot"))
                   || a == "-D" || a == "-U"
                   || a == "-gcc-toolchain" || a == "-target" || a == "-mllvm" || a == "-isystem") {
            if (++i < allFlags.length())
                filtered << a << allFlags.at(i);
        } else if (a.startsWith("-m") || a == "-Os" || a == "-O0" || a == "-O1" || a == "-O2"
                   || a == "-O3" || a == "-ffinite-math-only" || a == "-fshort-double"
                   || a == "-fshort-wchar" || a == "-fsignaling-nans" || a == "-fno-inline"
                   || a == "-fno-exceptions" || a == "-fstack-protector"
                   || a == "-fstack-protector-all" || a == "-fsanitize=address"
                   || a == "-fno-rtti" || a.startsWith("-std=") || a.startsWith("-stdlib=")
                   || a.startsWith("-specs=") || a == "-ansi" || a == "-undef"
                   || a.startsWith("-D") || a.startsWith("-U") || a == "-fopenmp"
                   || a == "-Wno-deprecated" || a == "-fPIC" || a == "-fpic" || a == "-fPIE"
                   || a == "-fpie" || a.startsWith("-stdlib=") || a.startsWith("-B")
                   || a.startsWith("--target=")
                   || (a.startsWith("-isystem") && a.length() > 8)
                   || a == "-nostdinc" || a == "-nostdinc++") {
            filtered << a;
        }
    }
    return filtered;
}

ToolChain::MacroInspectionRunner GccToolChain::createMacroInspectionRunner() const
{
    // Using a clean environment breaks ccache/distcc/etc.
    Environment env = Environment::systemEnvironment();
    addToEnvironment(env);
    const Utils::FilePath compilerCommand = m_compilerCommand;
    const QStringList platformCodeGenFlags = m_platformCodeGenFlags;
    OptionsReinterpreter reinterpretOptions = m_optionsReinterpreter;
    QTC_CHECK(reinterpretOptions);
    MacrosCache macroCache = predefinedMacrosCache();
    Utils::Id lang = language();

    // This runner must be thread-safe!
    return [env, compilerCommand, platformCodeGenFlags, reinterpretOptions, macroCache, lang]
            (const QStringList &flags) {
        QStringList allFlags = platformCodeGenFlags + flags;  // add only cxxflags is empty?
        QStringList arguments = gccPredefinedMacrosOptions(lang) + filteredFlags(allFlags, true);
        arguments = reinterpretOptions(arguments);
        const Utils::optional<MacroInspectionReport> cachedMacros = macroCache->check(arguments);
        if (cachedMacros)
            return cachedMacros.value();

        const Macros macros = gccPredefinedMacros(findLocalCompiler(compilerCommand, env),
                                                  arguments,
                                                  env.toStringList());

        const auto report = MacroInspectionReport{macros, languageVersion(lang, macros)};
        macroCache->insert(arguments, report);

        qCDebug(gccLog) << "MacroInspectionReport for code model:";
        qCDebug(gccLog) << "Language version:" << static_cast<int>(report.languageVersion);
        for (const Macro &m : macros) {
            qCDebug(gccLog) << compilerCommand.toUserOutput()
                            << (lang == Constants::CXX_LANGUAGE_ID ? ": C++ [" : ": C [")
                            << arguments.join(", ") << "]"
                            << QString::fromUtf8(m.toByteArray());
        }

        return report;
    };
}

/**
 * @brief Asks compiler for set of predefined macros
 * @param cxxflags - compiler flags collected from project settings
 * @return defines list, one per line, e.g. "#define __GXX_WEAK__ 1"
 *
 * @note changing compiler flags sometimes changes macros set, e.g. -fopenmp
 * adds _OPENMP macro, for full list of macro search by word "when" on this page:
 * http://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
 */
ProjectExplorer::Macros GccToolChain::predefinedMacros(const QStringList &cxxflags) const
{
    return createMacroInspectionRunner()(cxxflags).macros;
}

/**
 * @brief Parses gcc flags -std=*, -fopenmp, -fms-extensions.
 * @see http://gcc.gnu.org/onlinedocs/gcc/C-Dialect-Options.html
 */
Utils::LanguageExtensions GccToolChain::languageExtensions(const QStringList &cxxflags) const
{
    LanguageExtensions extensions = defaultLanguageExtensions();

    const QStringList allCxxflags = m_platformCodeGenFlags + cxxflags; // add only cxxflags is empty?
    foreach (const QString &flag, allCxxflags) {
        if (flag.startsWith("-std=")) {
            const QByteArray std = flag.mid(5).toLatin1();
            if (std.startsWith("gnu"))
                extensions |= LanguageExtension::Gnu;
            else
                extensions &= ~LanguageExtensions(LanguageExtension::Gnu);
        } else if (flag == "-fopenmp") {
            extensions |= LanguageExtension::OpenMP;
        } else if (flag == "-fms-extensions") {
            extensions |= LanguageExtension::Microsoft;
        }
    }

    return extensions;
}

WarningFlags GccToolChain::warningFlags(const QStringList &cflags) const
{
    // based on 'LC_ALL="en" gcc -Q --help=warnings | grep enabled'
    WarningFlags flags(WarningFlags::Deprecated | WarningFlags::IgnoredQualifiers
                       | WarningFlags::SignedComparison | WarningFlags::UninitializedVars);
    WarningFlags groupWall(WarningFlags::All | WarningFlags::UnknownPragma | WarningFlags::UnusedFunctions
                           | WarningFlags::UnusedLocals | WarningFlags::UnusedResult | WarningFlags::UnusedValue
                           | WarningFlags::SignedComparison | WarningFlags::UninitializedVars);
    WarningFlags groupWextra(WarningFlags::Extra | WarningFlags::IgnoredQualifiers | WarningFlags::UnusedParams);

    foreach (const QString &flag, cflags) {
        if (flag == "--all-warnings")
            flags |= groupWall;
        else if (flag == "--extra-warnings")
            flags |= groupWextra;

        WarningFlagAdder add(flag, flags);
        if (add.triggered())
            continue;

        // supported by clang too
        add("error", WarningFlags::AsErrors);
        add("all", groupWall);
        add("extra", groupWextra);
        add("deprecated", WarningFlags::Deprecated);
        add("effc++", WarningFlags::EffectiveCxx);
        add("ignored-qualifiers", WarningFlags::IgnoredQualifiers);
        add("non-virtual-dtor", WarningFlags::NonVirtualDestructor);
        add("overloaded-virtual", WarningFlags::OverloadedVirtual);
        add("shadow", WarningFlags::HiddenLocals);
        add("sign-compare", WarningFlags::SignedComparison);
        add("unknown-pragmas", WarningFlags::UnknownPragma);
        add("unused", WarningFlags::UnusedFunctions | WarningFlags::UnusedLocals | WarningFlags::UnusedParams
                | WarningFlags::UnusedResult | WarningFlags::UnusedValue);
        add("unused-function", WarningFlags::UnusedFunctions);
        add("unused-variable", WarningFlags::UnusedLocals);
        add("unused-parameter", WarningFlags::UnusedParams);
        add("unused-result", WarningFlags::UnusedResult);
        add("unused-value", WarningFlags::UnusedValue);
        add("uninitialized", WarningFlags::UninitializedVars);
    }
    return flags;
}

QStringList GccToolChain::gccPrepareArguments(const QStringList &flags,
                                              const QString &sysRoot,
                                              const QStringList &platformCodeGenFlags,
                                              Utils::Id languageId,
                                              OptionsReinterpreter reinterpretOptions)
{
    QStringList arguments;
    const bool hasKitSysroot = !sysRoot.isEmpty();
    if (hasKitSysroot)
        arguments.append(QString::fromLatin1("--sysroot=%1").arg(sysRoot));

    QStringList allFlags;
    allFlags << platformCodeGenFlags << flags;
    arguments += filteredFlags(allFlags, !hasKitSysroot);
    arguments << languageOption(languageId) << "-E" << "-v" << "-";
    arguments = reinterpretOptions(arguments);

    return arguments;
}

// NOTE: extraHeaderPathsFunction must NOT capture this or it's members!!!
void GccToolChain::initExtraHeaderPathsFunction(ExtraHeaderPathsFunction &&extraHeaderPathsFunction) const
{
    m_extraHeaderPathsFunction = std::move(extraHeaderPathsFunction);
}

HeaderPaths GccToolChain::builtInHeaderPaths(const Utils::Environment &env,
                                             const Utils::FilePath &compilerCommand,
                                             const QStringList &platformCodeGenFlags,
                                             OptionsReinterpreter reinterpretOptions,
                                             HeaderPathsCache headerCache,
                                             Utils::Id languageId,
                                             ExtraHeaderPathsFunction extraHeaderPathsFunction,
                                             const QStringList &flags,
                                             const QString &sysRoot,
                                             const QString &originalTargetTriple)
{
    QStringList arguments = gccPrepareArguments(flags,
                                                sysRoot,
                                                platformCodeGenFlags,
                                                languageId,
                                                reinterpretOptions);

    // Must be clang case only.
    if (!originalTargetTriple.isEmpty())
        arguments << "-target" << originalTargetTriple;

    const Utils::optional<HeaderPaths> cachedPaths = headerCache->check(qMakePair(env, arguments));
    if (cachedPaths)
        return cachedPaths.value();

    HeaderPaths paths = gccHeaderPaths(findLocalCompiler(compilerCommand, env),
                                       arguments,
                                       env.toStringList());
    extraHeaderPathsFunction(paths);
    headerCache->insert(qMakePair(env, arguments), paths);

    qCDebug(gccLog) << "Reporting header paths to code model:";
    for (const HeaderPath &hp : paths) {
        qCDebug(gccLog) << compilerCommand.toUserOutput()
                        << (languageId == Constants::CXX_LANGUAGE_ID ? ": C++ [" : ": C [")
                        << arguments.join(", ") << "]" << hp.path;
    }

    return paths;
}

ToolChain::BuiltInHeaderPathsRunner GccToolChain::createBuiltInHeaderPathsRunner(
        const Environment &env) const
{
    // Using a clean environment breaks ccache/distcc/etc.
    Environment fullEnv = env;
    addToEnvironment(fullEnv);

    // This runner must be thread-safe!
    return [this,
            fullEnv,
            compilerCommand = m_compilerCommand,
            platformCodeGenFlags = m_platformCodeGenFlags,
            reinterpretOptions = m_optionsReinterpreter,
            headerCache = headerPathsCache(),
            languageId = language(),
            extraHeaderPathsFunction = m_extraHeaderPathsFunction](const QStringList &flags,
                                                                   const QString &sysRoot,
                                                                   const QString &) {
        return builtInHeaderPaths(fullEnv,
                                  compilerCommand,
                                  platformCodeGenFlags,
                                  reinterpretOptions,
                                  headerCache,
                                  languageId,
                                  extraHeaderPathsFunction,
                                  flags,
                                  sysRoot,
                                  /*originalTargetTriple=*/""); // Must be empty for gcc.
    };
}

HeaderPaths GccToolChain::builtInHeaderPaths(const QStringList &flags,
                                             const FilePath &sysRootPath,
                                             const Environment &env) const
{
    return createBuiltInHeaderPathsRunner(env)(flags,
                                               sysRootPath.isEmpty() ? sysRoot()
                                                                     : sysRootPath.toString(),
                                               originalTargetTriple());
}

void GccToolChain::addCommandPathToEnvironment(const FilePath &command, Environment &env)
{
    const Utils::FilePath compilerDir = command.parentDir();
    if (!compilerDir.isEmpty())
        env.prependOrSetPath(compilerDir.toString());
}

void GccToolChain::addToEnvironment(Environment &env) const
{
    // On Windows gcc invokes cc1plus which is in libexec directory.
    // cc1plus depends on libwinpthread-1.dll which is in bin, so bin must be in the PATH.
    if (HostOsInfo::isWindowsHost())
        addCommandPathToEnvironment(m_compilerCommand, env);
}

QStringList GccToolChain::suggestedMkspecList() const
{
    const Abi abi = targetAbi();
    const Abi host = Abi::hostAbi();

    // Cross compile: Leave the mkspec alone!
    if (abi.architecture() != host.architecture()
            || abi.os() != host.os()
            || abi.osFlavor() != host.osFlavor()) // Note: This can fail:-(
        return {};

    if (abi.os() == Abi::DarwinOS) {
        QString v = version();
        // prefer versioned g++ on macOS. This is required to enable building for older macOS versions
        if (v.startsWith("4.0") && m_compilerCommand.endsWith("-4.0"))
            return {"macx-g++40"};
        if (v.startsWith("4.2") && m_compilerCommand.endsWith("-4.2"))
            return {"macx-g++42"};
        return {"macx-g++"};
    }

    if (abi.os() == Abi::LinuxOS) {
        if (abi.osFlavor() != Abi::GenericFlavor)
            return {}; // most likely not a desktop, so leave the mkspec alone.
        if (abi.wordWidth() == host.wordWidth()) {
            // no need to explicitly set the word width, but provide that mkspec anyway to make sure
            // that the correct compiler is picked if a mkspec with a wordwidth is given.
            return {"linux-g++", "linux-g++-" + QString::number(m_targetAbi.wordWidth())};
        }
        return {"linux-g++-" + QString::number(m_targetAbi.wordWidth())};
    }

    if (abi.os() == Abi::BsdOS && abi.osFlavor() == Abi::FreeBsdFlavor)
        return {"freebsd-g++"};

    return {};
}

FilePath GccToolChain::makeCommand(const Environment &environment) const
{
    const FilePath tmp = environment.searchInPath("make");
    return tmp.isEmpty() ? FilePath::fromString("make") : tmp;
}

QList<OutputLineParser *> GccToolChain::createOutputParsers() const
{
    return GccParser::gccParserSuite();
}

void GccToolChain::resetToolChain(const FilePath &path)
{
    bool resetDisplayName = (displayName() == defaultDisplayName());

    setCompilerCommand(path);

    Abi currentAbi = m_targetAbi;
    const DetectedAbisResult detectedAbis = detectSupportedAbis();
    m_supportedAbis = detectedAbis.supportedAbis;
    m_originalTargetTriple = detectedAbis.originalTargetTriple;
    m_installDir = installDir();

    m_targetAbi = Abi();
    if (!m_supportedAbis.isEmpty()) {
        if (m_supportedAbis.contains(currentAbi))
            m_targetAbi = currentAbi;
        else
            m_targetAbi = m_supportedAbis.at(0);
    }

    if (resetDisplayName)
        setDisplayName(defaultDisplayName()); // calls toolChainUpdated()!
    else
        toolChainUpdated();
}

FilePath GccToolChain::compilerCommand() const
{
    return m_compilerCommand;
}

void GccToolChain::setPlatformCodeGenFlags(const QStringList &flags)
{
    if (flags != m_platformCodeGenFlags) {
        m_platformCodeGenFlags = flags;
        toolChainUpdated();
    }
}

QStringList GccToolChain::extraCodeModelFlags() const
{
    return platformCodeGenFlags();
}

/*!
    Code gen flags that have to be passed to the compiler.
 */
QStringList GccToolChain::platformCodeGenFlags() const
{
    return m_platformCodeGenFlags;
}

void GccToolChain::setPlatformLinkerFlags(const QStringList &flags)
{
    if (flags != m_platformLinkerFlags) {
        m_platformLinkerFlags = flags;
        toolChainUpdated();
    }
}

/*!
    Flags that have to be passed to the linker.

    For example: \c{-arch armv7}
 */
QStringList GccToolChain::platformLinkerFlags() const
{
    return m_platformLinkerFlags;
}

QVariantMap GccToolChain::toMap() const
{
    QVariantMap data = ToolChain::toMap();
    data.insert(compilerCommandKeyC, m_compilerCommand.toString());
    data.insert(compilerPlatformCodeGenFlagsKeyC, m_platformCodeGenFlags);
    data.insert(compilerPlatformLinkerFlagsKeyC, m_platformLinkerFlags);
    data.insert(targetAbiKeyC, m_targetAbi.toString());
    data.insert(originalTargetTripleKeyC, m_originalTargetTriple);
    data.insert(supportedAbisKeyC, Utils::transform<QStringList>(m_supportedAbis, &Abi::toString));
    return data;
}

bool GccToolChain::fromMap(const QVariantMap &data)
{
    if (!ToolChain::fromMap(data))
        return false;

    m_compilerCommand = FilePath::fromString(data.value(compilerCommandKeyC).toString());
    m_platformCodeGenFlags = data.value(compilerPlatformCodeGenFlagsKeyC).toStringList();
    m_platformLinkerFlags = data.value(compilerPlatformLinkerFlagsKeyC).toStringList();
    const QString targetAbiString = data.value(targetAbiKeyC).toString();
    m_targetAbi = Abi::fromString(targetAbiString);
    m_originalTargetTriple = data.value(originalTargetTripleKeyC).toString();
    const QStringList abiList = data.value(supportedAbisKeyC).toStringList();
    m_supportedAbis.clear();
    for (const QString &a : abiList)
        m_supportedAbis.append(Abi::fromString(a));

    if (targetAbiString.isEmpty())
        resetToolChain(m_compilerCommand);

    return true;
}

bool GccToolChain::operator ==(const ToolChain &other) const
{
    if (!ToolChain::operator ==(other))
        return false;

    auto gccTc = static_cast<const GccToolChain *>(&other);
    return m_compilerCommand == gccTc->m_compilerCommand && m_targetAbi == gccTc->m_targetAbi
            && m_platformCodeGenFlags == gccTc->m_platformCodeGenFlags
            && m_platformLinkerFlags == gccTc->m_platformLinkerFlags;
}

std::unique_ptr<ToolChainConfigWidget> GccToolChain::createConfigurationWidget()
{
    return std::make_unique<GccToolChainConfigWidget>(this);
}

void GccToolChain::updateSupportedAbis() const
{
    if (m_supportedAbis.isEmpty()) {
        const DetectedAbisResult detected = detectSupportedAbis();
        m_supportedAbis = detected.supportedAbis;
        m_originalTargetTriple = detected.originalTargetTriple;
    }
}

void GccToolChain::setOptionsReinterpreter(const OptionsReinterpreter &optionsReinterpreter)
{
    m_optionsReinterpreter = optionsReinterpreter;
}

GccToolChain::DetectedAbisResult GccToolChain::detectSupportedAbis() const
{
    Environment env = Environment::systemEnvironment();
    addToEnvironment(env);
    ProjectExplorer::Macros macros = predefinedMacros(QStringList());
    return guessGccAbi(findLocalCompiler(m_compilerCommand, env),
                       env.toStringList(),
                       macros,
                       platformCodeGenFlags());
}

QString GccToolChain::detectVersion() const
{
    Environment env = Environment::systemEnvironment();
    addToEnvironment(env);
    return gccVersion(findLocalCompiler(m_compilerCommand, env), env.toStringList(),
                      filteredFlags(platformCodeGenFlags(), true));
}

Utils::FilePath GccToolChain::detectInstallDir() const
{
    Environment env = Environment::systemEnvironment();
    addToEnvironment(env);
    return gccInstallDir(findLocalCompiler(m_compilerCommand, env), env.toStringList(),
                         filteredFlags(platformCodeGenFlags(), true));
}

// --------------------------------------------------------------------------
// GccToolChainFactory
// --------------------------------------------------------------------------

static Utils::FilePaths gnuSearchPathsFromRegistry()
{
    if (!HostOsInfo::isWindowsHost())
        return {};

    // Registry token for the "GNU Tools for ARM Embedded Processors".
    static const char kRegistryToken[] = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\" \
                                         "Windows\\CurrentVersion\\Uninstall\\";

    Utils::FilePaths searchPaths;

    QSettings registry(kRegistryToken, QSettings::NativeFormat);
    const auto productGroups = registry.childGroups();
    for (const QString &productKey : productGroups) {
        if (!productKey.startsWith("GNU Tools for ARM Embedded Processors"))
            continue;
        registry.beginGroup(productKey);
        QString uninstallFilePath = registry.value("UninstallString").toString();
        if (uninstallFilePath.startsWith(QLatin1Char('"')))
            uninstallFilePath.remove(0, 1);
        if (uninstallFilePath.endsWith(QLatin1Char('"')))
            uninstallFilePath.remove(uninstallFilePath.size() - 1, 1);
        registry.endGroup();

        const QString toolkitRootPath = QFileInfo(uninstallFilePath).path();
        const QString toolchainPath = toolkitRootPath + QLatin1String("/bin");
        searchPaths.push_back(FilePath::fromString(toolchainPath));
    }

    return searchPaths;
}

static Utils::FilePaths atmelSearchPathsFromRegistry()
{
    if (!HostOsInfo::isWindowsHost())
        return {};

    // Registry token for the "Atmel" toolchains, e.g. provided by installed
    // "Atmel Studio" IDE.
    static const char kRegistryToken[] = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Atmel\\";

    Utils::FilePaths searchPaths;
    QSettings registry(kRegistryToken, QSettings::NativeFormat);

    // This code enumerate the installed toolchains provided
    // by the Atmel Studio v6.x.
    const auto toolchainGroups = registry.childGroups();
    for (const QString &toolchainKey : toolchainGroups) {
        if (!toolchainKey.endsWith("GCC"))
            continue;
        registry.beginGroup(toolchainKey);
        const auto entries = registry.childGroups();
        for (const auto &entryKey : entries) {
            registry.beginGroup(entryKey);
            const QString installDir = registry.value("Native/InstallDir").toString();
            const QString version = registry.value("Native/Version").toString();
            registry.endGroup();

            QString toolchainPath = installDir
                    + QLatin1String("/Atmel Toolchain/")
                    + toolchainKey + QLatin1String("/Native/")
                    + version;
            if (toolchainKey.startsWith("ARM"))
                toolchainPath += QLatin1String("/arm-gnu-toolchain");
            else if (toolchainKey.startsWith("AVR32"))
                toolchainPath += QLatin1String("/avr32-gnu-toolchain");
            else if (toolchainKey.startsWith("AVR8"))
                toolchainPath += QLatin1String("/avr8-gnu-toolchain");
            else
                break;

            toolchainPath += QLatin1String("/bin");

            const FilePath path = FilePath::fromString(toolchainPath);
            if (path.exists()) {
                searchPaths.push_back(FilePath::fromString(toolchainPath));
                break;
            }
        }
        registry.endGroup();
    }

    // This code enumerate the installed toolchains provided
    // by the Atmel Studio v7.
    registry.beginGroup("AtmelStudio");
    const auto productVersions = registry.childGroups();
    for (const auto &productVersionKey : productVersions) {
        registry.beginGroup(productVersionKey);
        const QString installDir = registry.value("InstallDir").toString();
        registry.endGroup();

        const QStringList knownToolchainSubdirs = {
            "/toolchain/arm/arm-gnu-toolchain/bin/",
            "/toolchain/avr8/avr8-gnu-toolchain/bin/",
            "/toolchain/avr32/avr32-gnu-toolchain/bin/",
        };

        for (const auto &subdir : knownToolchainSubdirs) {
            const QString toolchainPath = installDir + subdir;
            const FilePath path = FilePath::fromString(toolchainPath);
            if (!path.exists())
                continue;
            searchPaths.push_back(path);
        }
    }
    registry.endGroup();

    return searchPaths;
}

static Utils::FilePaths renesasRl78SearchPathsFromRegistry()
{
    if (!HostOsInfo::isWindowsHost())
        return {};

    // Registry token for the "Renesas RL78" toolchain.
    static const char kRegistryToken[] = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\" \
                                         "Windows\\CurrentVersion\\Uninstall";

    Utils::FilePaths searchPaths;

    QSettings registry(QLatin1String(kRegistryToken), QSettings::NativeFormat);
    const auto productGroups = registry.childGroups();
    for (const QString &productKey : productGroups) {
        if (!productKey.startsWith("GCC for Renesas RL78"))
            continue;
        registry.beginGroup(productKey);
        const QString installLocation = registry.value("InstallLocation").toString();
        registry.endGroup();
        if (installLocation.isEmpty())
            continue;

        const FilePath toolchainPath = FilePath::fromUserInput(installLocation)
                .pathAppended("rl78-elf/rl78-elf/bin");
        if (!toolchainPath.exists())
            continue;
        searchPaths.push_back(toolchainPath);
    }

    return searchPaths;
}

GccToolChainFactory::GccToolChainFactory()
{
    setDisplayName(GccToolChain::tr("GCC"));
    setSupportedToolChainType(Constants::GCC_TOOLCHAIN_TYPEID);
    setSupportedLanguages({Constants::C_LANGUAGE_ID, Constants::CXX_LANGUAGE_ID});
    setToolchainConstructor([] { return new GccToolChain(Constants::GCC_TOOLCHAIN_TYPEID); });
    setUserCreatable(true);
}

QList<ToolChain *> GccToolChainFactory::autoDetect(const QList<ToolChain *> &alreadyKnown)
{
    // GCC is almost never what you want on macOS, but it is by default found in /usr/bin
    if (HostOsInfo::isMacHost())
        return {};
    QList<ToolChain *> tcs;
    QList<ToolChain *> known = alreadyKnown;
    static const auto tcChecker = [](const ToolChain *tc) {
        return tc->targetAbi().osFlavor() != Abi::WindowsMSysFlavor
                && tc->compilerCommand().fileName() != "c89-gcc"
                && tc->compilerCommand().fileName() != "c99-gcc";
    };
    tcs.append(autoDetectToolchains("g++", DetectVariants::Yes, Constants::CXX_LANGUAGE_ID,
                                    Constants::GCC_TOOLCHAIN_TYPEID, alreadyKnown, tcChecker));
    tcs.append(autoDetectToolchains("gcc", DetectVariants::Yes, Constants::C_LANGUAGE_ID,
                                    Constants::GCC_TOOLCHAIN_TYPEID, alreadyKnown, tcChecker));
    return tcs;
}

QList<ToolChain *> GccToolChainFactory::detectForImport(const ToolChainDescription &tcd)
{
    const QString fileName = tcd.compilerPath.toFileInfo().completeBaseName();
    if ((tcd.language == Constants::C_LANGUAGE_ID && (fileName.startsWith("gcc")
                                                  || fileName.endsWith("gcc")
                                                  || fileName == "cc"))
            || (tcd.language == Constants::CXX_LANGUAGE_ID && (fileName.startsWith("g++")
                                                           || fileName.endsWith("g++")
                                                           || fileName == "c++")))
        return autoDetectToolChain(tcd, [](const ToolChain *tc) {
            return tc->targetAbi().osFlavor() != Abi::WindowsMSysFlavor;
        });
    return QList<ToolChain *>();
}

QList<ToolChain *> GccToolChainFactory::autoDetectToolchains(
        const QString &compilerName, DetectVariants detectVariants, Utils::Id language,
        const Utils::Id requiredTypeId, const QList<ToolChain *> &alreadyKnown,
        const ToolchainChecker &checker)
{
    FilePaths compilerPaths;
    QFileInfo fi(compilerName);
    if (fi.isAbsolute()) {
        if (fi.isFile())
            compilerPaths << FilePath::fromString(compilerName);
    } else {
        FilePaths searchPaths = Environment::systemEnvironment().path();
        searchPaths << gnuSearchPathsFromRegistry();
        searchPaths << atmelSearchPathsFromRegistry();
        searchPaths << renesasRl78SearchPathsFromRegistry();
        if (HostOsInfo::isAnyUnixHost()) {
            FilePath ccachePath = FilePath::fromString("/usr/lib/ccache/bin");
            if (!ccachePath.exists())
                ccachePath = FilePath::fromString("/usr/lib/ccache");
            if (ccachePath.exists() && !searchPaths.contains(ccachePath))
                searchPaths << ccachePath;
        }
        for (const FilePath &dir : searchPaths) {
            static const QRegularExpression regexp(binaryRegexp);
            QDir binDir(dir.toString());
            QStringList nameFilters(compilerName);
            if (detectVariants == DetectVariants::Yes) {
                nameFilters
                        << compilerName + "-[1-9]*" // "clang-8", "gcc-5"
                        << ("*-" + compilerName) // "avr-gcc", "avr32-gcc"
                        << ("*-" + compilerName + "-[1-9]*")// "avr-gcc-4.8.1", "avr32-gcc-4.4.7"
                        << ("*-*-*-" + compilerName) // "arm-none-eabi-gcc"
                        << ("*-*-*-" + compilerName + "-[1-9]*") // "arm-none-eabi-gcc-9.1.0"
                        << ("*-*-*-*-" + compilerName) // "x86_64-pc-linux-gnu-gcc"
                        << ("*-*-*-*-" + compilerName
                            + "-[1-9]*"); // "x86_64-pc-linux-gnu-gcc-7.4.1"
            }
            nameFilters = transform(nameFilters, [](const QString &baseName) {
                return HostOsInfo::withExecutableSuffix(baseName);
            });
            const QStringList fileNames = binDir.entryList(nameFilters,
                                                           QDir::Files | QDir::Executable);
            for (const QString &fileName : fileNames) {
                if (fileName != compilerName &&
                        !regexp.match(QFileInfo(fileName).completeBaseName()).hasMatch()) {
                    continue;
                }
                compilerPaths << FilePath::fromString(binDir.filePath(fileName));
            }
        }
    }

    QList<ToolChain *> existingCandidates
            = filtered(alreadyKnown, [requiredTypeId, language, &checker](const ToolChain *tc) {
        if (tc->typeId() != requiredTypeId)
            return false;
        if (tc->language() != language)
            return false;
        if (checker && !checker(tc))
            return false;
        return true;
    });
    QList<ToolChain *> result;
    for (const FilePath &compilerPath : compilerPaths) {
        bool alreadyExists = false;
        for (ToolChain * const existingTc : existingCandidates) {
            // We have a match if the existing toolchain ultimately refers to the same file
            // as the candidate path, either directly or via a hard or soft link.
            // Exceptions:
            //   - clang++ is often a soft link to clang, but behaves differently.
            //   - ccache and icecc also create soft links that must not be followed here.
            bool existingTcMatches = false;
            const FilePath existingCommand = existingTc->compilerCommand();
            if ((requiredTypeId == Constants::CLANG_TOOLCHAIN_TYPEID
                 && language == Constants::CXX_LANGUAGE_ID
                 && !existingCommand.fileName().contains("clang++"))
                    || compilerPath.toString().contains("icecc")
                    || compilerPath.toString().contains("ccache")) {
                existingTcMatches = existingCommand == compilerPath;
            } else {
                existingTcMatches = Environment::systemEnvironment().isSameExecutable(
                            existingCommand.toString(), compilerPath.toString())
                        || (HostOsInfo::isWindowsHost() && existingCommand.toFileInfo().size()
                            == compilerPath.toFileInfo().size());
            }
            if (existingTcMatches) {
                if (!result.contains(existingTc))
                    result << existingTc;
                alreadyExists = true;
            }
        }
        if (!alreadyExists) {
            const QList<ToolChain *> newToolchains = autoDetectToolChain({compilerPath, language},
                                                                         checker);
            result << newToolchains;
            existingCandidates << newToolchains;
        }
    }

    return result;
}

QList<ToolChain *> GccToolChainFactory::autoDetectToolChain(const ToolChainDescription &tcd,
                                                            const ToolchainChecker &checker)
{
    QList<ToolChain *> result;

    Environment systemEnvironment = Environment::systemEnvironment();
    GccToolChain::addCommandPathToEnvironment(tcd.compilerPath, systemEnvironment);
    const FilePath localCompilerPath = findLocalCompiler(tcd.compilerPath, systemEnvironment);
    Macros macros
            = gccPredefinedMacros(localCompilerPath, gccPredefinedMacrosOptions(tcd.language),
                                  systemEnvironment.toStringList());
    if (macros.isEmpty())
        return result;
    const GccToolChain::DetectedAbisResult detectedAbis = guessGccAbi(localCompilerPath,
                                                                      systemEnvironment.toStringList(),
                                                                      macros);
    const Utils::FilePath installDir = gccInstallDir(localCompilerPath,
                                                     systemEnvironment.toStringList());

    for (const Abi &abi : detectedAbis.supportedAbis) {
        std::unique_ptr<GccToolChain> tc(dynamic_cast<GccToolChain *>(create()));
        if (!tc)
            return result;

        tc->setLanguage(tcd.language);
        tc->setDetection(ToolChain::AutoDetection);
        tc->predefinedMacrosCache()
            ->insert(QStringList(),
                     ToolChain::MacroInspectionReport{macros,
                                                      ToolChain::languageVersion(tcd.language, macros)});
        tc->setCompilerCommand(tcd.compilerPath);
        tc->setSupportedAbis(detectedAbis.supportedAbis);
        tc->setTargetAbi(abi);
        tc->setOriginalTargetTriple(detectedAbis.originalTargetTriple);
        tc->setInstallDir(installDir);
        tc->setDisplayName(tc->defaultDisplayName()); // reset displayname
        if (!checker || checker(tc.get()))
            result.append(tc.release());
    }
    return result;
}

// --------------------------------------------------------------------------
// GccToolChainConfigWidget
// --------------------------------------------------------------------------

GccToolChainConfigWidget::GccToolChainConfigWidget(GccToolChain *tc) :
    ToolChainConfigWidget(tc),
    m_abiWidget(new AbiWidget),
    m_compilerCommand(new PathChooser)
{
    Q_ASSERT(tc);

    const QStringList gnuVersionArgs = QStringList("--version");
    m_compilerCommand->setExpectedKind(PathChooser::ExistingCommand);
    m_compilerCommand->setCommandVersionArguments(gnuVersionArgs);
    m_compilerCommand->setHistoryCompleter("PE.Gcc.Command.History");
    m_mainLayout->addRow(tr("&Compiler path:"), m_compilerCommand);
    m_platformCodeGenFlagsLineEdit = new QLineEdit(this);
    m_platformCodeGenFlagsLineEdit->setText(QtcProcess::joinArgs(tc->platformCodeGenFlags()));
    m_mainLayout->addRow(tr("Platform codegen flags:"), m_platformCodeGenFlagsLineEdit);
    m_platformLinkerFlagsLineEdit = new QLineEdit(this);
    m_platformLinkerFlagsLineEdit->setText(QtcProcess::joinArgs(tc->platformLinkerFlags()));
    m_mainLayout->addRow(tr("Platform linker flags:"), m_platformLinkerFlagsLineEdit);
    m_mainLayout->addRow(tr("&ABI:"), m_abiWidget);

    m_abiWidget->setEnabled(false);
    addErrorLabel();

    setFromToolchain();

    connect(m_compilerCommand, &PathChooser::rawPathChanged,
            this, &GccToolChainConfigWidget::handleCompilerCommandChange);
    connect(m_platformCodeGenFlagsLineEdit, &QLineEdit::editingFinished,
            this, &GccToolChainConfigWidget::handlePlatformCodeGenFlagsChange);
    connect(m_platformLinkerFlagsLineEdit, &QLineEdit::editingFinished,
            this, &GccToolChainConfigWidget::handlePlatformLinkerFlagsChange);
    connect(m_abiWidget, &AbiWidget::abiChanged, this, &ToolChainConfigWidget::dirty);
}

void GccToolChainConfigWidget::applyImpl()
{
    if (toolChain()->isAutoDetected())
        return;

    auto tc = static_cast<GccToolChain *>(toolChain());
    Q_ASSERT(tc);
    QString displayName = tc->displayName();
    tc->setCompilerCommand(m_compilerCommand->filePath());
    if (m_abiWidget) {
        tc->setSupportedAbis(m_abiWidget->supportedAbis());
        tc->setTargetAbi(m_abiWidget->currentAbi());
    }
    tc->setInstallDir(tc->detectInstallDir());
    tc->setOriginalTargetTriple(tc->detectSupportedAbis().originalTargetTriple);
    tc->setDisplayName(displayName); // reset display name
    tc->setPlatformCodeGenFlags(splitString(m_platformCodeGenFlagsLineEdit->text()));
    tc->setPlatformLinkerFlags(splitString(m_platformLinkerFlagsLineEdit->text()));

    if (m_macros.isEmpty())
        return;

    tc->predefinedMacrosCache()
        ->insert(tc->platformCodeGenFlags(),
                 ToolChain::MacroInspectionReport{m_macros,
                                                  ToolChain::languageVersion(tc->language(),
                                                                             m_macros)});
}

void GccToolChainConfigWidget::setFromToolchain()
{
    // subwidgets are not yet connected!
    QSignalBlocker blocker(this);
    auto tc = static_cast<GccToolChain *>(toolChain());
    m_compilerCommand->setFilePath(tc->compilerCommand());
    m_platformCodeGenFlagsLineEdit->setText(QtcProcess::joinArgs(tc->platformCodeGenFlags()));
    m_platformLinkerFlagsLineEdit->setText(QtcProcess::joinArgs(tc->platformLinkerFlags()));
    if (m_abiWidget) {
        m_abiWidget->setAbis(tc->supportedAbis(), tc->targetAbi());
        if (!m_isReadOnly && !m_compilerCommand->filePath().toString().isEmpty())
            m_abiWidget->setEnabled(true);
    }
}

bool GccToolChainConfigWidget::isDirtyImpl() const
{
    auto tc = static_cast<GccToolChain *>(toolChain());
    Q_ASSERT(tc);
    return m_compilerCommand->filePath() != tc->compilerCommand()
           || m_platformCodeGenFlagsLineEdit->text()
                  != QtcProcess::joinArgs(tc->platformCodeGenFlags())
           || m_platformLinkerFlagsLineEdit->text()
                  != QtcProcess::joinArgs(tc->platformLinkerFlags())
           || (m_abiWidget && m_abiWidget->currentAbi() != tc->targetAbi());
}

void GccToolChainConfigWidget::makeReadOnlyImpl()
{
    m_compilerCommand->setReadOnly(true);
    if (m_abiWidget)
        m_abiWidget->setEnabled(false);
    m_platformCodeGenFlagsLineEdit->setEnabled(false);
    m_platformLinkerFlagsLineEdit->setEnabled(false);
    m_isReadOnly = true;
}

void GccToolChainConfigWidget::handleCompilerCommandChange()
{
    if (!m_abiWidget)
        return;

    bool haveCompiler = false;
    Abi currentAbi = m_abiWidget->currentAbi();
    bool customAbi = m_abiWidget->isCustomAbi() && m_abiWidget->isEnabled();
    FilePath path = m_compilerCommand->filePath();
    Abis abiList;

    if (!path.isEmpty()) {
        QFileInfo fi(path.toFileInfo());
        haveCompiler = fi.isExecutable() && fi.isFile();
    }
    if (haveCompiler) {
        Environment env = Environment::systemEnvironment();
        GccToolChain::addCommandPathToEnvironment(path, env);
        QStringList args = gccPredefinedMacrosOptions(Constants::CXX_LANGUAGE_ID)
                + splitString(m_platformCodeGenFlagsLineEdit->text());
        const FilePath localCompilerPath = findLocalCompiler(path, env);
        m_macros = gccPredefinedMacros(localCompilerPath, args, env.toStringList());
        abiList = guessGccAbi(localCompilerPath, env.toStringList(), m_macros,
                              splitString(m_platformCodeGenFlagsLineEdit->text())).supportedAbis;
    }
    m_abiWidget->setEnabled(haveCompiler);

    // Find a good ABI for the new compiler:
    Abi newAbi;
    if (customAbi || abiList.contains(currentAbi))
        newAbi = currentAbi;

    m_abiWidget->setAbis(abiList, newAbi);
    emit dirty();
}

void GccToolChainConfigWidget::handlePlatformCodeGenFlagsChange()
{
    QString str1 = m_platformCodeGenFlagsLineEdit->text();
    QString str2 = QtcProcess::joinArgs(splitString(str1));
    if (str1 != str2)
        m_platformCodeGenFlagsLineEdit->setText(str2);
    else
        handleCompilerCommandChange();
}

void GccToolChainConfigWidget::handlePlatformLinkerFlagsChange()
{
    QString str1 = m_platformLinkerFlagsLineEdit->text();
    QString str2 = QtcProcess::joinArgs(splitString(str1));
    if (str1 != str2)
        m_platformLinkerFlagsLineEdit->setText(str2);
    else
        emit dirty();
}

// --------------------------------------------------------------------------
// ClangToolChain
// --------------------------------------------------------------------------

static QList<ToolChain *> mingwToolChains()
{
    return ToolChainManager::toolChains([](const ToolChain *tc) -> bool {
        return tc->typeId() == Constants::MINGW_TOOLCHAIN_TYPEID;
    });
}

static const MingwToolChain *mingwToolChainFromId(const QByteArray &id)
{
    if (id.isEmpty())
        return nullptr;

    for (const ToolChain *tc : mingwToolChains()) {
        if (tc->id() == id)
            return static_cast<const MingwToolChain *>(tc);
    }

    return nullptr;
}

void ClangToolChain::syncAutodetectedWithParentToolchains()
{
    if (!HostOsInfo::isWindowsHost() || typeId() != Constants::CLANG_TOOLCHAIN_TYPEID
        || !isAutoDetected()) {
        return;
    }

    QObject::disconnect(m_thisToolchainRemovedConnection);
    QObject::disconnect(m_mingwToolchainAddedConnection);

    if (!mingwToolChainFromId(m_parentToolChainId)) {
        const QList<ToolChain *> mingwTCs = mingwToolChains();
        m_parentToolChainId = mingwTCs.isEmpty() ? QByteArray() : mingwTCs.front()->id();
    }

    // Subscribe only autodetected toolchains.
    ToolChainManager *tcManager = ToolChainManager::instance();
    m_mingwToolchainAddedConnection
        = QObject::connect(tcManager, &ToolChainManager::toolChainAdded, [this](ToolChain *tc) {
              if (tc->typeId() == Constants::MINGW_TOOLCHAIN_TYPEID
                  && !mingwToolChainFromId(m_parentToolChainId)) {
                  m_parentToolChainId = tc->id();
              }
          });
    m_thisToolchainRemovedConnection
        = QObject::connect(tcManager, &ToolChainManager::toolChainRemoved, [this](ToolChain *tc) {
              if (tc == this) {
                  QObject::disconnect(m_thisToolchainRemovedConnection);
                  QObject::disconnect(m_mingwToolchainAddedConnection);
              } else if (m_parentToolChainId == tc->id()) {
                  const QList<ToolChain *> mingwTCs = mingwToolChains();
                  m_parentToolChainId = mingwTCs.isEmpty() ? QByteArray() : mingwTCs.front()->id();
              }
          });
}

ClangToolChain::ClangToolChain() :
    ClangToolChain(Constants::CLANG_TOOLCHAIN_TYPEID)
{
}

ClangToolChain::ClangToolChain(Utils::Id typeId) :
    GccToolChain(typeId)
{
    setTypeDisplayName(tr("Clang"));
    syncAutodetectedWithParentToolchains();
}

ClangToolChain::~ClangToolChain()
{
    QObject::disconnect(m_thisToolchainRemovedConnection);
    QObject::disconnect(m_mingwToolchainAddedConnection);
}

FilePath ClangToolChain::makeCommand(const Environment &environment) const
{
    const QStringList makes
            = HostOsInfo::isWindowsHost() ? QStringList({"mingw32-make.exe", "make.exe"}) : QStringList({"make"});

    FilePath tmp;
    for (const QString &make : makes) {
        tmp = environment.searchInPath(make);
        if (!tmp.isEmpty())
            return tmp;
    }
    return FilePath::fromString(makes.first());
}

/**
 * @brief Similar to \a GccToolchain::languageExtensions, but recognizes
 * "-fborland-extensions".
 */
LanguageExtensions ClangToolChain::languageExtensions(const QStringList &cxxflags) const
{
    LanguageExtensions extensions = GccToolChain::languageExtensions(cxxflags);
    if (cxxflags.contains("-fborland-extensions"))
        extensions |= LanguageExtension::Borland;
    return extensions;
}

WarningFlags ClangToolChain::warningFlags(const QStringList &cflags) const
{
    WarningFlags flags = GccToolChain::warningFlags(cflags);
    foreach (const QString &flag, cflags) {
        if (flag == "-Wdocumentation")
            flags |= WarningFlags::Documentation;
        if (flag == "-Wno-documentation")
            flags &= ~WarningFlags::Documentation;
    }
    return flags;
}

QStringList ClangToolChain::suggestedMkspecList() const
{
    if (const ToolChain * const parentTc = ToolChainManager::findToolChain(m_parentToolChainId))
        return parentTc->suggestedMkspecList();
    const Abi abi = targetAbi();
    if (abi.os() == Abi::DarwinOS)
        return {"macx-clang", "macx-clang-32", "unsupported/macx-clang", "macx-ios-clang"};
    if (abi.os() == Abi::LinuxOS)
        return {"linux-clang", "unsupported/linux-clang"};
    if (abi.os() == Abi::WindowsOS)
        return {"win32-clang-g++"};
    if (abi.architecture() == Abi::AsmJsArchitecture && abi.binaryFormat() == Abi::EmscriptenFormat)
        return {"wasm-emscripten"};
    return {}; // Note: Not supported by Qt yet, so default to the mkspec the Qt was build with
}

void ClangToolChain::addToEnvironment(Environment &env) const
{
    GccToolChain::addToEnvironment(env);

    const QString sysroot = sysRoot();
    if (!sysroot.isEmpty())
        env.prependOrSetPath(sysroot + "/bin");

    // Clang takes PWD as basis for debug info, if set.
    // When running Qt Creator from a shell, PWD is initially set to an "arbitrary" value.
    // Since the tools are not called through a shell, PWD is never changed to the actual cwd,
    // so we better make sure PWD is empty to begin with
    env.unset("PWD");
}

QString ClangToolChain::originalTargetTriple() const
{
    const MingwToolChain *parentTC = mingwToolChainFromId(m_parentToolChainId);
    if (parentTC)
        return parentTC->originalTargetTriple();

    return GccToolChain::originalTargetTriple();
}

QString ClangToolChain::sysRoot() const
{
    const MingwToolChain *parentTC = mingwToolChainFromId(m_parentToolChainId);
    if (!parentTC)
        return QString();

    const FilePath mingwCompiler = parentTC->compilerCommand();
    return mingwCompiler.parentDir().parentDir().toString();
}

ToolChain::BuiltInHeaderPathsRunner ClangToolChain::createBuiltInHeaderPathsRunner(
        const Environment &env) const
{
    // Using a clean environment breaks ccache/distcc/etc.
    Environment fullEnv = env;
    addToEnvironment(fullEnv);

    // This runner must be thread-safe!
    return [this,
            fullEnv,
            compilerCommand = m_compilerCommand,
            platformCodeGenFlags = m_platformCodeGenFlags,
            reinterpretOptions = m_optionsReinterpreter,
            headerCache = headerPathsCache(),
            languageId = language(),
            extraHeaderPathsFunction = m_extraHeaderPathsFunction](const QStringList &flags,
                                                                   const QString &sysRoot,
                                                                   const QString &target) {
        return builtInHeaderPaths(fullEnv,
                                  compilerCommand,
                                  platformCodeGenFlags,
                                  reinterpretOptions,
                                  headerCache,
                                  languageId,
                                  extraHeaderPathsFunction,
                                  flags,
                                  sysRoot,
                                  target);
    };
}

std::unique_ptr<ToolChainConfigWidget> ClangToolChain::createConfigurationWidget()
{
    return std::make_unique<ClangToolChainConfigWidget>(this);
}

QVariantMap ClangToolChain::toMap() const
{
    QVariantMap data = GccToolChain::toMap();
    data.insert(parentToolChainIdKeyC, m_parentToolChainId);
    return data;
}

bool ClangToolChain::fromMap(const QVariantMap &data)
{
    if (!GccToolChain::fromMap(data))
        return false;

    m_parentToolChainId = data.value(parentToolChainIdKeyC).toByteArray();
    syncAutodetectedWithParentToolchains();
    return true;
}

LanguageExtensions ClangToolChain::defaultLanguageExtensions() const
{
    return LanguageExtension::Gnu;
}

QList<OutputLineParser *> ClangToolChain::createOutputParsers() const
{
    return ClangParser::clangParserSuite();
}

// --------------------------------------------------------------------------
// ClangToolChainFactory
// --------------------------------------------------------------------------

ClangToolChainFactory::ClangToolChainFactory()
{
    setDisplayName(ClangToolChain::tr("Clang"));
    setSupportedToolChainType(Constants::CLANG_TOOLCHAIN_TYPEID);
    setSupportedLanguages({Constants::CXX_LANGUAGE_ID, Constants::C_LANGUAGE_ID});
    setToolchainConstructor([] { return new ClangToolChain; });
}

QList<ToolChain *> ClangToolChainFactory::autoDetect(const QList<ToolChain *> &alreadyKnown)
{
    QList<ToolChain *> tcs;
    QList<ToolChain *> known = alreadyKnown;

    tcs.append(autoDetectToolchains("clang++", DetectVariants::Yes, Constants::CXX_LANGUAGE_ID,
                                    Constants::CLANG_TOOLCHAIN_TYPEID, alreadyKnown));
    tcs.append(autoDetectToolchains("clang", DetectVariants::Yes, Constants::C_LANGUAGE_ID,
                                    Constants::CLANG_TOOLCHAIN_TYPEID, alreadyKnown));
    known.append(tcs);

    const FilePath compilerPath = FilePath::fromString(Core::ICore::clangExecutable(CLANG_BINDIR));
    if (!compilerPath.isEmpty()) {
        const FilePath clang = compilerPath.parentDir().pathAppended(
                    HostOsInfo::withExecutableSuffix("clang"));
        tcs.append(autoDetectToolchains(clang.toString(), DetectVariants::No,
                                        Constants::C_LANGUAGE_ID, Constants::CLANG_TOOLCHAIN_TYPEID,
                                        tcs));
    }

    return tcs;
}

QList<ToolChain *> ClangToolChainFactory::detectForImport(const ToolChainDescription &tcd)
{
    const QString fileName = tcd.compilerPath.toString();
    if ((tcd.language == Constants::C_LANGUAGE_ID && fileName.startsWith("clang") && !fileName.startsWith("clang++"))
            || (tcd.language == Constants::CXX_LANGUAGE_ID && fileName.startsWith("clang++")))
        return autoDetectToolChain(tcd);
    return {};
}

ClangToolChainConfigWidget::ClangToolChainConfigWidget(ClangToolChain *tc) :
    GccToolChainConfigWidget(tc)
{
    if (!HostOsInfo::isWindowsHost() || tc->typeId() != Constants::CLANG_TOOLCHAIN_TYPEID)
        return;

    // Remove m_abiWidget row because the parent toolchain abi is going to be used.
    m_mainLayout->removeRow(m_mainLayout->rowCount() - 2);
    m_abiWidget = nullptr;

    m_parentToolchainCombo = new QComboBox(this);
    m_mainLayout->insertRow(m_mainLayout->rowCount() - 1,
                            tr("Parent toolchain:"),
                            m_parentToolchainCombo);

    ToolChainManager *tcManager = ToolChainManager::instance();
    m_parentToolChainConnections.append(
        connect(tcManager, &ToolChainManager::toolChainUpdated, this, [this](ToolChain *tc) {
            if (tc->typeId() == Constants::MINGW_TOOLCHAIN_TYPEID)
                updateParentToolChainComboBox();
        }));
    m_parentToolChainConnections.append(
        connect(tcManager, &ToolChainManager::toolChainAdded, this, [this](ToolChain *tc) {
            if (tc->typeId() == Constants::MINGW_TOOLCHAIN_TYPEID)
                updateParentToolChainComboBox();
        }));
    m_parentToolChainConnections.append(
        connect(tcManager, &ToolChainManager::toolChainRemoved, this, [this](ToolChain *tc) {
            if (tc->id() == toolChain()->id()) {
                for (QMetaObject::Connection &connection : m_parentToolChainConnections)
                    QObject::disconnect(connection);
                return;
            }
            if (tc->typeId() == Constants::MINGW_TOOLCHAIN_TYPEID)
                updateParentToolChainComboBox();
        }));

    setFromClangToolchain();
}

void ClangToolChainConfigWidget::updateParentToolChainComboBox()
{
    auto *tc = static_cast<ClangToolChain *>(toolChain());
    QByteArray parentId = m_parentToolchainCombo->currentData().toByteArray();
    if (tc->isAutoDetected() || m_parentToolchainCombo->count() == 0)
        parentId = tc->m_parentToolChainId;

    const MingwToolChain *parentTC = mingwToolChainFromId(parentId);

    m_parentToolchainCombo->clear();
    m_parentToolchainCombo->addItem(parentTC ? parentTC->displayName() : QString(),
                                    parentTC ? parentId : QByteArray());

    if (tc->isAutoDetected())
        return;

    for (const ToolChain *mingwTC : mingwToolChains()) {
        if (parentId != mingwTC->id())
            m_parentToolchainCombo->addItem(mingwTC->displayName(), mingwTC->id());
    }
}

void ClangToolChainConfigWidget::setFromClangToolchain()
{
    GccToolChainConfigWidget::setFromToolchain();

    if (m_parentToolchainCombo)
        updateParentToolChainComboBox();
}

void ClangToolChainConfigWidget::applyImpl()
{
    GccToolChainConfigWidget::applyImpl();
    if (!m_parentToolchainCombo)
        return;

    auto *tc = static_cast<ClangToolChain *>(toolChain());
    tc->m_parentToolChainId.clear();

    const QByteArray parentId = m_parentToolchainCombo->currentData().toByteArray();
    if (!parentId.isEmpty()) {
        for (const ToolChain *mingwTC : mingwToolChains()) {
            if (parentId == mingwTC->id()) {
                tc->m_parentToolChainId = mingwTC->id();
                tc->setTargetAbi(mingwTC->targetAbi());
                tc->setSupportedAbis(mingwTC->supportedAbis());
                break;
            }
        }
    }
}

bool ClangToolChainConfigWidget::isDirtyImpl() const
{
    if (GccToolChainConfigWidget::isDirtyImpl())
        return true;

    if (!m_parentToolchainCombo)
        return false;

    auto tc = static_cast<ClangToolChain *>(toolChain());
    Q_ASSERT(tc);
    const MingwToolChain *parentTC = mingwToolChainFromId(tc->m_parentToolChainId);
    const QByteArray parentId = parentTC ? parentTC->id() : QByteArray();
    return parentId != m_parentToolchainCombo->currentData();
}

void ClangToolChainConfigWidget::makeReadOnlyImpl()
{
    GccToolChainConfigWidget::makeReadOnlyImpl();
    if (m_parentToolchainCombo)
        m_parentToolchainCombo->setEnabled(false);
}

// --------------------------------------------------------------------------
// MingwToolChain
// --------------------------------------------------------------------------

MingwToolChain::MingwToolChain() :
    GccToolChain(Constants::MINGW_TOOLCHAIN_TYPEID)
{
    setTypeDisplayName(MingwToolChain::tr("MinGW"));
}

QStringList MingwToolChain::suggestedMkspecList() const
{
    if (HostOsInfo::isWindowsHost())
        return {"win32-g++"};
    if (HostOsInfo::isLinuxHost()) {
        if (version().startsWith("4.6."))
            return {"win32-g++-4.6-cross", "unsupported/win32-g++-4.6-cross"};
        return {"win32-g++-cross", "unsupported/win32-g++-cross"};
    }
    return {};
}

FilePath MingwToolChain::makeCommand(const Environment &environment) const
{
    const QStringList makes
            = HostOsInfo::isWindowsHost() ? QStringList({"mingw32-make.exe", "make.exe"}) : QStringList({"make"});

    FilePath tmp;
    foreach (const QString &make, makes) {
        tmp = environment.searchInPath(make);
        if (!tmp.isEmpty())
            return tmp;
    }
    return FilePath::fromString(makes.first());
}

// --------------------------------------------------------------------------
// MingwToolChainFactory
// --------------------------------------------------------------------------

MingwToolChainFactory::MingwToolChainFactory()
{
    setDisplayName(MingwToolChain::tr("MinGW"));
    setSupportedToolChainType(Constants::MINGW_TOOLCHAIN_TYPEID);
    setSupportedLanguages({Constants::CXX_LANGUAGE_ID, Constants::C_LANGUAGE_ID});
    setToolchainConstructor([] { return new MingwToolChain; });
}

QList<ToolChain *> MingwToolChainFactory::autoDetect(const QList<ToolChain *> &alreadyKnown)
{
    static const auto tcChecker = [](const ToolChain *tc) {
        return tc->targetAbi().osFlavor() == Abi::WindowsMSysFlavor;
    };
    QList<ToolChain *> result = autoDetectToolchains(
                "g++", DetectVariants::Yes, Constants::CXX_LANGUAGE_ID,
                Constants::MINGW_TOOLCHAIN_TYPEID, alreadyKnown, tcChecker);
    result += autoDetectToolchains("gcc", DetectVariants::Yes, Constants::C_LANGUAGE_ID,
                                   Constants::MINGW_TOOLCHAIN_TYPEID, alreadyKnown, tcChecker);
    return result;
}

QList<ToolChain *> MingwToolChainFactory::detectForImport(const ToolChainDescription &tcd)
{
    const QString fileName = tcd.compilerPath.toFileInfo().completeBaseName();
    if ((tcd.language == Constants::C_LANGUAGE_ID && (fileName.startsWith("gcc")
                                                      || fileName.endsWith("gcc")))
            || (tcd.language == Constants::CXX_LANGUAGE_ID && (fileName.startsWith("g++")
                                                               || fileName.endsWith("g++"))))
        return autoDetectToolChain(tcd, [](const ToolChain *tc) {
            return tc->targetAbi().osFlavor() == Abi::WindowsMSysFlavor;
        });

    return {};
}

// --------------------------------------------------------------------------
// LinuxIccToolChain
// --------------------------------------------------------------------------

LinuxIccToolChain::LinuxIccToolChain() :
    GccToolChain(Constants::LINUXICC_TOOLCHAIN_TYPEID)
{
    setTypeDisplayName(LinuxIccToolChain::tr("ICC"));
}

/**
 * Similar to \a GccToolchain::languageExtensions, but uses "-openmp" instead of
 * "-fopenmp" and "-fms-dialect[=ver]" instead of "-fms-extensions".
 * @see UNIX manual for "icc"
 */
LanguageExtensions LinuxIccToolChain::languageExtensions(const QStringList &cxxflags) const
{
    QStringList copy = cxxflags;
    copy.removeAll("-fopenmp");
    copy.removeAll("-fms-extensions");

    LanguageExtensions extensions = GccToolChain::languageExtensions(cxxflags);
    if (cxxflags.contains("-openmp"))
        extensions |= LanguageExtension::OpenMP;
    if (cxxflags.contains("-fms-dialect")
            || cxxflags.contains("-fms-dialect=8")
            || cxxflags.contains("-fms-dialect=9")
            || cxxflags.contains("-fms-dialect=10"))
        extensions |= LanguageExtension::Microsoft;
    return extensions;
}

QList<OutputLineParser *> LinuxIccToolChain::createOutputParsers() const
{
    return LinuxIccParser::iccParserSuite();
}

QStringList LinuxIccToolChain::suggestedMkspecList() const
{
    return {QString("linux-icc-%1").arg(targetAbi().wordWidth())};
}

// --------------------------------------------------------------------------
// LinuxIccToolChainFactory
// --------------------------------------------------------------------------

LinuxIccToolChainFactory::LinuxIccToolChainFactory()
{
    setDisplayName(LinuxIccToolChain::tr("ICC"));
    setSupportedToolChainType(Constants::LINUXICC_TOOLCHAIN_TYPEID);
    setSupportedLanguages({Constants::CXX_LANGUAGE_ID, Constants::C_LANGUAGE_ID});
    setToolchainConstructor([] { return new LinuxIccToolChain; });
}

QList<ToolChain *> LinuxIccToolChainFactory::autoDetect(const QList<ToolChain *> &alreadyKnown)
{
    QList<ToolChain *> result
            = autoDetectToolchains("icpc", DetectVariants::No, Constants::CXX_LANGUAGE_ID,
                                   Constants::LINUXICC_TOOLCHAIN_TYPEID, alreadyKnown);
    result += autoDetectToolchains("icc", DetectVariants::Yes, Constants::C_LANGUAGE_ID,
                                   Constants::LINUXICC_TOOLCHAIN_TYPEID, alreadyKnown);
    return result;
}

QList<ToolChain *> LinuxIccToolChainFactory::detectForImport(const ToolChainDescription &tcd)
{
    const QString fileName = tcd.compilerPath.toString();
    if ((tcd.language == Constants::CXX_LANGUAGE_ID && fileName.startsWith("icpc")) ||
        (tcd.language == Constants::C_LANGUAGE_ID && fileName.startsWith("icc")))
        return autoDetectToolChain(tcd);
    return {};
}

GccToolChain::WarningFlagAdder::WarningFlagAdder(const QString &flag, WarningFlags &flags) :
    m_flags(flags)
{
    if (!flag.startsWith("-W")) {
        m_triggered = true;
        return;
    }

    m_doesEnable = !flag.startsWith("-Wno-");
    if (m_doesEnable)
        m_flagUtf8 = flag.mid(2).toUtf8();
    else
        m_flagUtf8 = flag.mid(5).toUtf8();
}

void GccToolChain::WarningFlagAdder::operator ()(const char name[], WarningFlags flagsSet)
{
    if (m_triggered)
        return;
    if (0 == strcmp(m_flagUtf8.data(), name))
    {
        m_triggered = true;
        if (m_doesEnable)
            m_flags |= flagsSet;
        else
            m_flags &= ~flagsSet;
    }
}

bool GccToolChain::WarningFlagAdder::triggered() const
{
    return m_triggered;
}

} // namespace ProjectExplorer

// Unit tests:

#ifdef WITH_TESTS
#   include "projectexplorer.h"

#   include <QTest>
#   include <QUrl>

namespace ProjectExplorer {
void ProjectExplorerPlugin::testGccAbiGuessing_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QByteArray>("macros");
    QTest::addColumn<QStringList>("abiList");

    QTest::newRow("invalid input")
            << QString::fromLatin1("Some text")
            << QByteArray("")
            << (QStringList());
    QTest::newRow("empty input")
            << QString::fromLatin1("")
            << QByteArray("")
            << (QStringList());
    QTest::newRow("empty input (with macros)")
            << QString::fromLatin1("")
            << QByteArray("#define __SIZEOF_SIZE_T__ 8\n#define __Something\n")
            << (QStringList());
    QTest::newRow("broken input -- 64bit")
            << QString::fromLatin1("arm-none-foo-gnueabi")
            << QByteArray("#define __SIZEOF_SIZE_T__ 8\n#define __Something\n")
            << QStringList({"arm-baremetal-generic-elf-64bit"});
    QTest::newRow("broken input -- 32bit")
            << QString::fromLatin1("arm-none-foo-gnueabi")
            << QByteArray("#define __SIZEOF_SIZE_T__ 4\n#define __Something\n")
            << QStringList({"arm-baremetal-generic-elf-32bit"});
    QTest::newRow("totally broken input -- 32bit")
            << QString::fromLatin1("foo-bar-foo")
            << QByteArray("#define __SIZEOF_SIZE_T__ 4\n#define __Something\n")
            << QStringList();

    QTest::newRow("Linux 1 (32bit intel)")
            << QString::fromLatin1("i686-linux-gnu")
            << QByteArray("#define __SIZEOF_SIZE_T__ 4\n")
            << QStringList({"x86-linux-generic-elf-32bit"});
    QTest::newRow("Linux 2 (32bit intel)")
            << QString::fromLatin1("i486-linux-gnu")
            << QByteArray("#define __SIZEOF_SIZE_T__ 4\n")
            << QStringList({"x86-linux-generic-elf-32bit"});
    QTest::newRow("Linux 3 (64bit intel)")
            << QString::fromLatin1("x86_64-linux-gnu")
            << QByteArray("#define __SIZEOF_SIZE_T__ 8\n")
            << QStringList("x86-linux-generic-elf-64bit");
    QTest::newRow("Linux 3 (64bit intel -- non 64bit)")
            << QString::fromLatin1("x86_64-linux-gnu")
            << QByteArray("#define __SIZEOF_SIZE_T__ 4\n")
            << QStringList({"x86-linux-generic-elf-32bit"});
    QTest::newRow("Linux 4 (32bit mips)")
            << QString::fromLatin1("mipsel-linux-uclibc")
            << QByteArray("#define __SIZEOF_SIZE_T__ 4")
            << QStringList({"mips-linux-generic-elf-32bit"});
    QTest::newRow("Linux 5 (QTCREATORBUG-4690)") // from QTCREATORBUG-4690
            << QString::fromLatin1("x86_64-redhat-linux6E")
            << QByteArray("#define __SIZEOF_SIZE_T__ 8\n")
            << QStringList("x86-linux-generic-elf-64bit");
    QTest::newRow("Linux 6 (QTCREATORBUG-4690)") // from QTCREATORBUG-4690
            << QString::fromLatin1("x86_64-redhat-linux")
            << QByteArray("#define __SIZEOF_SIZE_T__ 8\n")
            << QStringList("x86-linux-generic-elf-64bit");
    QTest::newRow("Linux 7 (arm)")
                << QString::fromLatin1("armv5tl-montavista-linux-gnueabi")
                << QByteArray("#define __SIZEOF_SIZE_T__ 4\n")
                << QStringList({"arm-linux-generic-elf-32bit"});
    QTest::newRow("Linux 8 (arm)")
                << QString::fromLatin1("arm-angstrom-linux-gnueabi")
                << QByteArray("#define __SIZEOF_SIZE_T__ 4\n")
                << QStringList({"arm-linux-generic-elf-32bit"});
    QTest::newRow("Linux 9 (ppc)")
                << QString::fromLatin1("powerpc-nsg-linux")
                << QByteArray("#define __SIZEOF_SIZE_T__ 4\n")
                << QStringList({"ppc-linux-generic-elf-32bit"});
    QTest::newRow("Linux 10 (ppc 64bit)")
                << QString::fromLatin1("powerpc64-suse-linux")
                << QByteArray("#define __SIZEOF_SIZE_T__ 8\n")
                << QStringList({"ppc-linux-generic-elf-64bit"});
    QTest::newRow("Linux 11 (64bit mips)")
            << QString::fromLatin1("mips64el-linux-uclibc")
            << QByteArray("#define __SIZEOF_SIZE_T__ 8")
            << QStringList({"mips-linux-generic-elf-64bit"});

    QTest::newRow("Mingw 1 (32bit)")
            << QString::fromLatin1("i686-w64-mingw32")
            << QByteArray("#define __SIZEOF_SIZE_T__ 4\r\n")
            << QStringList({"x86-windows-msys-pe-32bit"});
    QTest::newRow("Mingw 2 (64bit)")
            << QString::fromLatin1("i686-w64-mingw32")
            << QByteArray("#define __SIZEOF_SIZE_T__ 8\r\n")
            << QStringList({"x86-windows-msys-pe-64bit"});
    QTest::newRow("Mingw 3 (32 bit)")
            << QString::fromLatin1("mingw32")
            << QByteArray("#define __SIZEOF_SIZE_T__ 4\r\n")
            << QStringList({"x86-windows-msys-pe-32bit"});
    QTest::newRow("Cross Mingw 1 (64bit)")
            << QString::fromLatin1("amd64-mingw32msvc")
            << QByteArray("#define __SIZEOF_SIZE_T__ 8\r\n")
            << QStringList({"x86-windows-msys-pe-64bit"});
    QTest::newRow("Cross Mingw 2 (32bit)")
            << QString::fromLatin1("i586-mingw32msvc")
            << QByteArray("#define __SIZEOF_SIZE_T__ 4\r\n")
            << QStringList({"x86-windows-msys-pe-32bit"});
    QTest::newRow("Clang 1: windows")
            << QString::fromLatin1("x86_64-pc-win32")
            << QByteArray("#define __SIZEOF_SIZE_T__ 8\r\n")
            << QStringList("x86-windows-msys-pe-64bit");
    QTest::newRow("Clang 1: linux")
            << QString::fromLatin1("x86_64-unknown-linux-gnu")
            << QByteArray("#define __SIZEOF_SIZE_T__ 8\n")
            << QStringList("x86-linux-generic-elf-64bit");
    QTest::newRow("Mac 1")
            << QString::fromLatin1("i686-apple-darwin10")
            << QByteArray("#define __SIZEOF_SIZE_T__ 8\n")
            << QStringList({"x86-darwin-generic-mach_o-64bit", "x86-darwin-generic-mach_o-32bit"});
    QTest::newRow("Mac 2")
            << QString::fromLatin1("powerpc-apple-darwin10")
            << QByteArray("#define __SIZEOF_SIZE_T__ 8\n")
            << QStringList({"ppc-darwin-generic-mach_o-64bit", "ppc-darwin-generic-mach_o-32bit"});
    QTest::newRow("Mac 3")
            << QString::fromLatin1("i686-apple-darwin9")
            << QByteArray("#define __SIZEOF_SIZE_T__ 4\n")
            << QStringList({"x86-darwin-generic-mach_o-32bit", "x86-darwin-generic-mach_o-64bit"});
    QTest::newRow("Mac IOS")
            << QString::fromLatin1("arm-apple-darwin9")
            << QByteArray("#define __SIZEOF_SIZE_T__ 4\n")
            << QStringList({"arm-darwin-generic-mach_o-32bit", "arm-darwin-generic-mach_o-64bit"});
    QTest::newRow("Intel 1")
            << QString::fromLatin1("86_64 x86_64 GNU/Linux")
            << QByteArray("#define __SIZEOF_SIZE_T__ 8\n")
            << QStringList("x86-linux-generic-elf-64bit");
    QTest::newRow("FreeBSD 1")
            << QString::fromLatin1("i386-portbld-freebsd9.0")
            << QByteArray("#define __SIZEOF_SIZE_T__ 4\n")
            << QStringList({"x86-bsd-freebsd-elf-32bit"});
    QTest::newRow("FreeBSD 2")
            << QString::fromLatin1("i386-undermydesk-freebsd")
            << QByteArray("#define __SIZEOF_SIZE_T__ 4\n")
            << QStringList({"x86-bsd-freebsd-elf-32bit"});
}

void ProjectExplorerPlugin::testGccAbiGuessing()
{
    QFETCH(QString, input);
    QFETCH(QByteArray, macros);
    QFETCH(QStringList, abiList);

    const Abis al = guessGccAbi(input, ProjectExplorer::Macro::toMacros(macros));
    QCOMPARE(al.count(), abiList.count());
    for (int i = 0; i < al.count(); ++i)
        QCOMPARE(al.at(i).toString(), abiList.at(i));
}

} // namespace ProjectExplorer

#endif
