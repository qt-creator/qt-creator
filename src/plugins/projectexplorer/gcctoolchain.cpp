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

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/synchronousprocess.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/synchronousprocess.h>

#include <QBuffer>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QLineEdit>
#include <QLoggingCategory>
#include <QRegularExpression>

#include <algorithm>
#include <memory>

namespace {
Q_LOGGING_CATEGORY(gccLog, "qtc.projectexplorer.toolchain.gcc", QtWarningMsg);
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
static const char binaryRegexp[] = "(?:^|-|\\b)(?:gcc|g\\+\\+|clang(?:\\+\\+)?)(?:-([\\d.]+))?$";

static QByteArray runGcc(const FileName &gcc, const QStringList &arguments, const QStringList &env)
{
    if (gcc.isEmpty() || !gcc.toFileInfo().isExecutable())
        return QByteArray();

    SynchronousProcess cpp;
    QStringList environment(env);
    Utils::Environment::setupEnglishOutput(&environment);

    cpp.setEnvironment(environment);
    cpp.setTimeoutS(10);
    SynchronousProcessResponse response =  cpp.runBlocking(gcc.toString(), arguments);
    if (response.result != SynchronousProcessResponse::Finished ||
            response.exitCode != 0) {
        qWarning() << response.exitMessage(gcc.toString(), 10);
        return QByteArray();
    }

    return response.allOutput().toUtf8();
}

static ProjectExplorer::Macros gccPredefinedMacros(const FileName &gcc,
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

HeaderPaths GccToolChain::gccHeaderPaths(const FileName &gcc, const QStringList &arguments,
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

void GccToolChain::toolChainUpdated()
{
    m_predefinedMacrosCache->invalidate();
    m_headerPathsCache->invalidate();
    ToolChain::toolChainUpdated();
}

static QList<Abi> guessGccAbi(const QString &m, const ProjectExplorer::Macros &macros)
{
    QList<Abi> abiList;

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
        abiList << Abi(arch, os, flavor, format, 32);
    } else {
        abiList << Abi(arch, os, flavor, format, width);
    }
    return abiList;
}


static GccToolChain::DetectedAbisResult guessGccAbi(const FileName &path, const QStringList &env,
                                                   const ProjectExplorer::Macros &macros,
                                                   const QStringList &extraArgs = QStringList())
{
    if (path.isEmpty())
        return GccToolChain::DetectedAbisResult();

    QStringList arguments = extraArgs;
    arguments << "-dumpmachine";
    QString machine = QString::fromLocal8Bit(runGcc(path, arguments, env)).trimmed();
    if (machine.isEmpty())
        return GccToolChain::DetectedAbisResult(); // no need to continue if running failed once...
    return GccToolChain::DetectedAbisResult(guessGccAbi(machine, macros), machine);
}

static QString gccVersion(const FileName &path, const QStringList &env)
{
    QStringList arguments("-dumpversion");
    return QString::fromLocal8Bit(runGcc(path, arguments, env)).trimmed();
}

// --------------------------------------------------------------------------
// GccToolChain
// --------------------------------------------------------------------------

GccToolChain::GccToolChain(Detection d) :
    GccToolChain(Constants::GCC_TOOLCHAIN_TYPEID, d)
{ }

GccToolChain::GccToolChain(Core::Id typeId, Detection d) :
    ToolChain(typeId, d),
    m_predefinedMacrosCache(std::make_shared<Cache<MacroInspectionReport, 64>>()),
    m_headerPathsCache(std::make_shared<Cache<HeaderPaths>>())
{ }

void GccToolChain::setCompilerCommand(const FileName &path)
{
    if (path == m_compilerCommand)
        return;

    m_compilerCommand = path;
    toolChainUpdated();
}

void GccToolChain::setSupportedAbis(const QList<Abi> &m_abis)
{
    if (m_supportedAbis == m_abis)
        return;

    m_supportedAbis = m_abis;
    toolChainUpdated();
}

void GccToolChain::setOriginalTargetTriple(const QString &targetTriple)
{
    if (m_originalTargetTriple == targetTriple)
        return;

    m_originalTargetTriple = targetTriple;
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

QString GccToolChain::typeDisplayName() const
{
    return GccToolChainFactory::tr("GCC");
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

void GccToolChain::setTargetAbi(const Abi &abi)
{
    if (abi == m_targetAbi)
        return;

    m_targetAbi = abi;
    toolChainUpdated();
}

QList<Abi> GccToolChain::supportedAbis() const
{
    return m_supportedAbis;
}

bool GccToolChain::isValid() const
{
    if (m_compilerCommand.isNull())
        return false;

    QFileInfo fi = compilerCommand().toFileInfo();
    return fi.isExecutable();
}

static Utils::FileName findLocalCompiler(const Utils::FileName &compilerPath,
                                         const Environment &env)
{
    // Find the "real" compiler if icecc, distcc or similar are in use. Ignore ccache, since that
    // is local already.

    // Get the path to the compiler, ignoring direct calls to icecc and distcc as we cannot
    // do anything about those.
    const Utils::FileName compilerDir = compilerPath.parentDir();
    const QString compilerDirString = compilerDir.toString();
    if (!compilerDirString.contains("icecc") && !compilerDirString.contains("distcc"))
        return compilerPath;

    FileNameList pathComponents = env.path();
    auto it = std::find_if(pathComponents.begin(), pathComponents.end(),
                           [compilerDir](const FileName &p) {
        return p == compilerDir;
    });
    if (it != pathComponents.end()) {
        std::rotate(pathComponents.begin(), it, pathComponents.end());
        pathComponents.removeFirst(); // remove directory of compilerPath
                                      // No need to put it at the end again, it is in PATH anyway...
    }

    // This effectively searches the PATH twice, once via pathComponents and once via PATH itself:
    // searchInPath filters duplicates, so that will not hurt.
    const Utils::FileName path = env.searchInPath(compilerPath.fileName(), pathComponents);

    return path.isEmpty() ? compilerPath : path;
}

ToolChain::MacroInspectionRunner GccToolChain::createMacroInspectionRunner() const
{
    // Using a clean environment breaks ccache/distcc/etc.
    Environment env = Environment::systemEnvironment();
    addToEnvironment(env);
    const Utils::FileName compilerCommand = m_compilerCommand;
    const QStringList platformCodeGenFlags = m_platformCodeGenFlags;
    OptionsReinterpreter reinterpretOptions = m_optionsReinterpreter;
    QTC_CHECK(reinterpretOptions);
    std::shared_ptr<Cache<MacroInspectionReport, 64>> macroCache = m_predefinedMacrosCache;
    Core::Id lang = language();

    // This runner must be thread-safe!
    return [env, compilerCommand, platformCodeGenFlags, reinterpretOptions, macroCache, lang]
            (const QStringList &flags) {
        QStringList allFlags = platformCodeGenFlags + flags;  // add only cxxflags is empty?
        QStringList arguments = gccPredefinedMacrosOptions(lang);
        for (int iArg = 0; iArg < allFlags.length(); ++iArg) {
            const QString &a = allFlags.at(iArg);
            if (a.startsWith("--gcc-toolchain=")) {
                arguments << a;
            } else if (a == "-arch") {
                if (++iArg < allFlags.length() && !arguments.contains(a))
                    arguments << a << allFlags.at(iArg);
            } else if (a == "--sysroot" || a == "-isysroot" || a == "-D" || a == "-U"
                       || a == "-gcc-toolchain" || a == "-target") {
                if (++iArg < allFlags.length())
                    arguments << a << allFlags.at(iArg);
            } else if (a == "-m128bit-long-double" || a == "-m32" || a == "-m3dnow"
                       || a == "-m3dnowa" || a == "-m64" || a == "-m96bit-long-double"
                       || a == "-mabm" || a == "-maes" || a.startsWith("-march=") || a == "-mavx"
                       || a.startsWith("-masm=") || a.startsWith("-mfloat-abi") || a == "-mcx16"
                       || a == "-mfma" || a == "-mfma4" || a == "-mlwp" || a == "-mpclmul"
                       || a == "-mpopcnt" || a == "-msse" || a == "-msse2" || a == "-msse2avx"
                       || a == "-msse3" || a == "-msse4" || a == "-msse4.1" || a == "-msse4.2"
                       || a == "-msse4a" || a == "-mssse3" || a.startsWith("-mtune=")
                       || a == "-mxop" || a == "-Os" || a == "-O0" || a == "-O1" || a == "-O2"
                       || a == "-O3" || a == "-ffinite-math-only" || a == "-fshort-double"
                       || a == "-fshort-wchar" || a == "-fsignaling-nans" || a == "-fno-inline"
                       || a == "-fno-exceptions" || a == "-fstack-protector"
                       || a == "-fstack-protector-all" || a == "-fsanitize=address"
                       || a == "-fno-rtti" || a.startsWith("-std=") || a.startsWith("-stdlib=")
                       || a.startsWith("-specs=") || a == "-ansi" || a == "-undef"
                       || a.startsWith("-D") || a.startsWith("-U") || a == "-fopenmp"
                       || a == "-Wno-deprecated" || a == "-fPIC" || a == "-fpic" || a == "-fPIE"
                       || a == "-fpie")
                arguments << a;
        }

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
LanguageExtensions GccToolChain::languageExtensions(const QStringList &cxxflags) const
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
    WarningFlags flags(WarningFlags::Deprecated | WarningFlags::IgnoredQualfiers
                       | WarningFlags::SignedComparison | WarningFlags::UninitializedVars);
    WarningFlags groupWall(WarningFlags::All | WarningFlags::UnknownPragma | WarningFlags::UnusedFunctions
                           | WarningFlags::UnusedLocals | WarningFlags::UnusedResult | WarningFlags::UnusedValue
                           | WarningFlags::SignedComparison | WarningFlags::UninitializedVars);
    WarningFlags groupWextra(WarningFlags::Extra | WarningFlags::IgnoredQualfiers | WarningFlags::UnusedParams);

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
        add("ignored-qualifiers", WarningFlags::IgnoredQualfiers);
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
                                              Core::Id languageId,
                                              OptionsReinterpreter reinterpretOptions)
{
    QStringList arguments;
    const bool hasKitSysroot = !sysRoot.isEmpty();
    if (hasKitSysroot)
        arguments.append(QString::fromLatin1("--sysroot=%1").arg(sysRoot));

    QStringList allFlags;
    allFlags << platformCodeGenFlags << flags;
    for (int i = 0; i < allFlags.size(); ++i) {
        const QString &flag = allFlags.at(i);
        if (flag.startsWith("-stdlib=") || flag.startsWith("--gcctoolchain=")
            || flag.startsWith("-B") || (flag.startsWith("-isystem") && flag.length() > 8)) {
            arguments << flag;
        } else if (!hasKitSysroot) {
            // pass build system's sysroot to compiler, if we didn't pass one from kit
            if (flag.startsWith("--sysroot=")) {
                arguments << flag;
            } else if ((flag.startsWith("-isysroot") || flag.startsWith("--sysroot")
                        || flag == "-target" || flag == "-gcc-toolchain" || flag == "-isystem")
                       && i < flags.size() - 1) {
                arguments << flag << allFlags.at(i + 1);
                ++i;
            }
        }
    }
    arguments << languageOption(languageId) << "-E" << "-v" << "-";
    arguments = reinterpretOptions(arguments);

    return arguments;
}

// NOTE: extraHeaderPathsFunction must NOT capture this or it's members!!!
void GccToolChain::initExtraHeaderPathsFunction(ExtraHeaderPathsFunction &&extraHeaderPathsFunction) const
{
    m_extraHeaderPathsFunction = std::move(extraHeaderPathsFunction);
}

ToolChain::BuiltInHeaderPathsRunner GccToolChain::createBuiltInHeaderPathsRunner() const
{
    // Using a clean environment breaks ccache/distcc/etc.
    Environment env = Environment::systemEnvironment();
    addToEnvironment(env);

    const Utils::FileName compilerCommand = m_compilerCommand;
    const QStringList platformCodeGenFlags = m_platformCodeGenFlags;
    OptionsReinterpreter reinterpretOptions = m_optionsReinterpreter;
    QTC_CHECK(reinterpretOptions);
    std::shared_ptr<Cache<HeaderPaths>> headerCache = m_headerPathsCache;
    Core::Id languageId = language();

    // This runner must be thread-safe!
    return [env, compilerCommand, platformCodeGenFlags, reinterpretOptions, headerCache, languageId,
            extraHeaderPathsFunction = m_extraHeaderPathsFunction]
            (const QStringList &flags, const QString &sysRoot) {

        QStringList arguments = gccPrepareArguments(flags, sysRoot, platformCodeGenFlags,
                                                    languageId, reinterpretOptions);

        const Utils::optional<HeaderPaths> cachedPaths = headerCache->check(arguments);
        if (cachedPaths)
            return cachedPaths.value();

        HeaderPaths paths = gccHeaderPaths(findLocalCompiler(compilerCommand, env),
                                           arguments, env.toStringList());
        extraHeaderPathsFunction(paths);
        headerCache->insert(arguments, paths);

        qCDebug(gccLog) << "Reporting header paths to code model:";
        for (const HeaderPath &hp : paths) {
            qCDebug(gccLog) << compilerCommand.toUserOutput()
                            << (languageId == Constants::CXX_LANGUAGE_ID ? ": C++ [" : ": C [")
                            << arguments.join(", ") << "]"
                            << hp.path;
        }

        return paths;
    };
}

HeaderPaths GccToolChain::builtInHeaderPaths(const QStringList &flags,
                                             const FileName &sysRoot) const
{
    return createBuiltInHeaderPathsRunner()(flags, sysRoot.toString());
}

void GccToolChain::addCommandPathToEnvironment(const FileName &command, Environment &env)
{
    const Utils::FileName compilerDir = command.parentDir();
    if (!compilerDir.isEmpty())
        env.prependOrSetPath(compilerDir.toString());
}

GccToolChain::GccToolChain(const GccToolChain &) = default;

void GccToolChain::addToEnvironment(Environment &env) const
{
    // On Windows gcc invokes cc1plus which is in libexec directory.
    // cc1plus depends on libwinpthread-1.dll which is in bin, so bin must be in the PATH.
    if (HostOsInfo::isWindowsHost())
        addCommandPathToEnvironment(m_compilerCommand, env);
}

FileNameList GccToolChain::suggestedMkspecList() const
{
    Abi abi = targetAbi();
    Abi host = Abi::hostAbi();

    // Cross compile: Leave the mkspec alone!
    if (abi.architecture() != host.architecture()
            || abi.os() != host.os()
            || abi.osFlavor() != host.osFlavor()) // Note: This can fail:-(
        return FileNameList();

    if (abi.os() == Abi::DarwinOS) {
        QString v = version();
        // prefer versioned g++ on macOS. This is required to enable building for older macOS versions
        if (v.startsWith("4.0") && m_compilerCommand.endsWith("-4.0"))
            return FileNameList() << FileName::fromLatin1("macx-g++40");
        if (v.startsWith("4.2") && m_compilerCommand.endsWith("-4.2"))
            return FileNameList() << FileName::fromLatin1("macx-g++42");
        return FileNameList() << FileName::fromLatin1("macx-g++");
    }

    if (abi.os() == Abi::LinuxOS) {
        if (abi.osFlavor() != Abi::GenericFlavor)
            return FileNameList(); // most likely not a desktop, so leave the mkspec alone.
        if (abi.wordWidth() == host.wordWidth()) {
            // no need to explicitly set the word width, but provide that mkspec anyway to make sure
            // that the correct compiler is picked if a mkspec with a wordwidth is given.
            return FileNameList() << FileName::fromLatin1("linux-g++")
                                  << FileName::fromString(QString::fromLatin1("linux-g++-") + QString::number(m_targetAbi.wordWidth()));
        }
        return FileNameList() << FileName::fromString(QString::fromLatin1("linux-g++-") + QString::number(m_targetAbi.wordWidth()));
    }

    if (abi.os() == Abi::BsdOS && abi.osFlavor() == Abi::FreeBsdFlavor)
        return FileNameList() << FileName::fromLatin1("freebsd-g++");

    return FileNameList();
}

QString GccToolChain::makeCommand(const Environment &environment) const
{
    QString make = "make";
    FileName tmp = environment.searchInPath(make);
    return tmp.isEmpty() ? make : tmp.toString();
}

IOutputParser *GccToolChain::outputParser() const
{
    return new GccParser;
}

void GccToolChain::resetToolChain(const FileName &path)
{
    bool resetDisplayName = (displayName() == defaultDisplayName());

    setCompilerCommand(path);

    Abi currentAbi = m_targetAbi;
    const DetectedAbisResult detectedAbis = detectSupportedAbis();
    m_supportedAbis = detectedAbis.supportedAbis;
    m_originalTargetTriple = detectedAbis.originalTargetTriple;

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

FileName GccToolChain::compilerCommand() const
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

ToolChain *GccToolChain::clone() const
{
    return new GccToolChain(*this);
}

QVariantMap GccToolChain::toMap() const
{
    QVariantMap data = ToolChain::toMap();
    data.insert(compilerCommandKeyC, m_compilerCommand.toString());
    data.insert(compilerPlatformCodeGenFlagsKeyC, m_platformCodeGenFlags);
    data.insert(compilerPlatformLinkerFlagsKeyC, m_platformLinkerFlags);
    data.insert(targetAbiKeyC, m_targetAbi.toString());
    data.insert(originalTargetTripleKeyC, m_originalTargetTriple);
    QStringList abiList = Utils::transform(m_supportedAbis, &Abi::toString);
    data.insert(supportedAbisKeyC, abiList);
    return data;
}

bool GccToolChain::fromMap(const QVariantMap &data)
{
    if (!ToolChain::fromMap(data))
        return false;

    m_compilerCommand = FileName::fromString(data.value(compilerCommandKeyC).toString());
    m_platformCodeGenFlags = data.value(compilerPlatformCodeGenFlagsKeyC).toStringList();
    m_platformLinkerFlags = data.value(compilerPlatformLinkerFlagsKeyC).toStringList();
    m_targetAbi = Abi::fromString(data.value(targetAbiKeyC).toString());
    m_originalTargetTriple = data.value(originalTargetTripleKeyC).toString();
    const QStringList abiList = data.value(supportedAbisKeyC).toStringList();
    m_supportedAbis.clear();
    for (const QString &a : abiList) {
        Abi abi = Abi::fromString(a);
        if (!abi.isValid())
            continue;
        m_supportedAbis.append(abi);
    }

    if (!m_targetAbi.isValid())
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
    return gccVersion(findLocalCompiler(m_compilerCommand, env), env.toStringList());
}

// --------------------------------------------------------------------------
// GccToolChainFactory
// --------------------------------------------------------------------------

GccToolChainFactory::GccToolChainFactory()
{
    setDisplayName(tr("GCC"));
}

QSet<Core::Id> GccToolChainFactory::supportedLanguages() const
{
    return {Constants::C_LANGUAGE_ID, Constants::CXX_LANGUAGE_ID};
}

bool GccToolChainFactory::canCreate()
{
    return true;
}

ToolChain *GccToolChainFactory::create(Core::Id language)
{
    ToolChain *tc = createToolChain(false);
    tc->setLanguage(language);
    return tc;
}

void GccToolChainFactory::versionProbe(const QString &name, Core::Id language, Core::Id type,
                                       QList<ToolChain *> &tcs, QList<ToolChain *> &known,
                                       const QSet<QString> &filteredNames)
{
    if (!HostOsInfo::isLinuxHost())
        return;
    const QRegularExpression regexp(binaryRegexp);
    for (const QString &dir : QStringList({ "/usr/bin", "/usr/local/bin" })) {
        QDir binDir(dir);
        for (const QString &entry : binDir.entryList(
             {"*-" + name, name + "-*", "*-" + name + "-*"},
                 QDir::Files | QDir::Executable)) {
            const QString fileName = FileName::fromString(entry).fileName();
            if (filteredNames.contains(fileName))
                continue;
            const QRegularExpressionMatch match = regexp.match(fileName);
            if (!match.hasMatch())
                continue;
            const bool isNative = fileName.startsWith(name);
            const Abi abi = isNative ? Abi::hostAbi() : Abi();
            tcs.append(autoDetectToolchains(compilerPathFromEnvironment(entry), abi, language, type,
                                            known));
            known.append(tcs);
        }
    }
}

QList<ToolChain *> GccToolChainFactory::autoDetect(const QList<ToolChain *> &alreadyKnown)
{
    QList<ToolChain *> tcs;
    QList<ToolChain *> known = alreadyKnown;
    tcs.append(autoDetectToolchains(compilerPathFromEnvironment("g++"), Abi::hostAbi(),
                                    Constants::CXX_LANGUAGE_ID, Constants::GCC_TOOLCHAIN_TYPEID,
                                    alreadyKnown));
    tcs.append(autoDetectToolchains(compilerPathFromEnvironment("gcc"), Abi::hostAbi(),
                                    Constants::C_LANGUAGE_ID, Constants::GCC_TOOLCHAIN_TYPEID,
                                    alreadyKnown));
    known.append(tcs);
    versionProbe("g++", Constants::CXX_LANGUAGE_ID, Constants::GCC_TOOLCHAIN_TYPEID, tcs, known);
    versionProbe("gcc", Constants::C_LANGUAGE_ID, Constants::GCC_TOOLCHAIN_TYPEID, tcs, known,
                 {"c89-gcc", "c99-gcc"});

    return tcs;
}

QList<ToolChain *> GccToolChainFactory::autoDetect(const FileName &compilerPath, const Core::Id &language)
{
    const QString fileName = compilerPath.fileName();
    if ((language == Constants::C_LANGUAGE_ID && (fileName.startsWith("gcc")
                                                  || fileName.endsWith("gcc")))
            || (language == Constants::CXX_LANGUAGE_ID && (fileName.startsWith("g++")
                                                           || fileName.endsWith("g++"))))
        return autoDetectToolChain(compilerPath, language);
    return QList<ToolChain *>();
}

// Used by the ToolChainManager to restore user-generated tool chains
bool GccToolChainFactory::canRestore(const QVariantMap &data)
{
    return typeIdFromMap(data) == Constants::GCC_TOOLCHAIN_TYPEID;
}

ToolChain *GccToolChainFactory::restore(const QVariantMap &data)
{
    GccToolChain *tc = createToolChain(false);
    if (tc->fromMap(data))
        return tc;

    delete tc;
    return nullptr;
}

GccToolChain *GccToolChainFactory::createToolChain(bool autoDetect)
{
    return new GccToolChain(autoDetect ? ToolChain::AutoDetection : ToolChain::ManualDetection);
}

Utils::FileName GccToolChainFactory::compilerPathFromEnvironment(const QString &compilerName)
{
    Environment systemEnvironment = Environment::systemEnvironment();
    return systemEnvironment.searchInPath(compilerName);
}

QList<ToolChain *> GccToolChainFactory::autoDetectToolchains(const FileName &compilerPath,
                                                             const Abi &requiredAbi,
                                                             Core::Id language,
                                                             const Core::Id requiredTypeId,
                                                             const QList<ToolChain *> &alreadyKnown)
{
    QList<ToolChain *> result;

    if (compilerPath.isEmpty())
        return result;

    result = Utils::filtered(alreadyKnown, [requiredTypeId, compilerPath](ToolChain *tc) {
        return tc->typeId() == requiredTypeId && tc->compilerCommand() == compilerPath;
    });
    if (!result.isEmpty()) {
        for (ToolChain *tc : result) {
            if (tc->isAutoDetected())
                tc->setLanguage(language);
        }
        return result;
    }

    result = autoDetectToolChain(compilerPath, language, requiredAbi);

    return result;
}

QList<ToolChain *> GccToolChainFactory::autoDetectToolChain(const FileName &compilerPath,
                                                            const Core::Id language,
                                                            const Abi &requiredAbi)
{
    QList<ToolChain *> result;

    Environment systemEnvironment = Environment::systemEnvironment();
    GccToolChain::addCommandPathToEnvironment(compilerPath, systemEnvironment);
    const FileName localCompilerPath = findLocalCompiler(compilerPath, systemEnvironment);
    Macros macros
            = gccPredefinedMacros(localCompilerPath, gccPredefinedMacrosOptions(language),
                                  systemEnvironment.toStringList());
    const GccToolChain::DetectedAbisResult detectedAbis = guessGccAbi(localCompilerPath,
                                                                      systemEnvironment.toStringList(),
                                                                      macros);

    const QList<Abi> abiList = detectedAbis.supportedAbis;
    if (!requiredAbi.isNull() && !abiList.contains(requiredAbi)) {
        if (requiredAbi.wordWidth() != 64
                || !abiList.contains(Abi(requiredAbi.architecture(), requiredAbi.os(), requiredAbi.osFlavor(),
                                         requiredAbi.binaryFormat(), 32))) {
            return result;
        }
    }

    for (const Abi &abi : abiList) {
        std::unique_ptr<GccToolChain> tc(createToolChain(true));
        if (!tc)
            return result;

        tc->setLanguage(language);
        tc->m_predefinedMacrosCache
            ->insert(QStringList(),
                     ToolChain::MacroInspectionReport{macros,
                                                      ToolChain::languageVersion(language, macros)});
        tc->setCompilerCommand(compilerPath);
        tc->setSupportedAbis(detectedAbis.supportedAbis);
        tc->setTargetAbi(abi);
        tc->setOriginalTargetTriple(detectedAbis.originalTargetTriple);
        tc->setDisplayName(tc->defaultDisplayName()); // reset displayname

        result.append(tc.release());
    }
    return result;
}

// --------------------------------------------------------------------------
// GccToolChainConfigWidget
// --------------------------------------------------------------------------

GccToolChainConfigWidget::GccToolChainConfigWidget(GccToolChain *tc) :
    ToolChainConfigWidget(tc),
    m_compilerCommand(new PathChooser),
    m_abiWidget(new AbiWidget)
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
    tc->setCompilerCommand(m_compilerCommand->fileName());
    tc->setSupportedAbis(m_abiWidget->supportedAbis());
    tc->setTargetAbi(m_abiWidget->currentAbi());
    tc->setOriginalTargetTriple(tc->detectSupportedAbis().originalTargetTriple);
    tc->setDisplayName(displayName); // reset display name
    tc->setPlatformCodeGenFlags(splitString(m_platformCodeGenFlagsLineEdit->text()));
    tc->setPlatformLinkerFlags(splitString(m_platformLinkerFlagsLineEdit->text()));

    if (m_macros.isEmpty())
        return;

    tc->m_predefinedMacrosCache
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
    m_compilerCommand->setFileName(tc->compilerCommand());
    m_platformCodeGenFlagsLineEdit->setText(QtcProcess::joinArgs(tc->platformCodeGenFlags()));
    m_platformLinkerFlagsLineEdit->setText(QtcProcess::joinArgs(tc->platformLinkerFlags()));
    m_abiWidget->setAbis(tc->supportedAbis(), tc->targetAbi());
    if (!m_isReadOnly && !m_compilerCommand->path().isEmpty())
        m_abiWidget->setEnabled(true);
}

bool GccToolChainConfigWidget::isDirtyImpl() const
{
    auto tc = static_cast<GccToolChain *>(toolChain());
    Q_ASSERT(tc);
    return m_compilerCommand->fileName() != tc->compilerCommand()
            || m_platformCodeGenFlagsLineEdit->text() != QtcProcess::joinArgs(tc->platformCodeGenFlags())
            || m_platformLinkerFlagsLineEdit->text() != QtcProcess::joinArgs(tc->platformLinkerFlags())
            || m_abiWidget->currentAbi() != tc->targetAbi();
}

void GccToolChainConfigWidget::makeReadOnlyImpl()
{
    m_compilerCommand->setReadOnly(true);
    m_abiWidget->setEnabled(false);
    m_platformCodeGenFlagsLineEdit->setEnabled(false);
    m_platformLinkerFlagsLineEdit->setEnabled(false);
    m_isReadOnly = true;
}

QStringList GccToolChainConfigWidget::splitString(const QString &s)
{
    QtcProcess::SplitError splitError;
    const OsType osType = HostOsInfo::hostOs();
    QStringList res = QtcProcess::splitArgs(s, osType, false, &splitError);
    if (splitError != QtcProcess::SplitOk){
        res = QtcProcess::splitArgs(s + '\\', osType, false, &splitError);
        if (splitError != QtcProcess::SplitOk){
            res = QtcProcess::splitArgs(s + '"', osType, false, &splitError);
            if (splitError != QtcProcess::SplitOk)
                res = QtcProcess::splitArgs(s + '\'', osType, false, &splitError);
        }
    }
    return res;
}

void GccToolChainConfigWidget::handleCompilerCommandChange()
{
    bool haveCompiler = false;
    Abi currentAbi = m_abiWidget->currentAbi();
    bool customAbi = m_abiWidget->isCustomAbi() && m_abiWidget->isEnabled();
    FileName path = m_compilerCommand->fileName();
    QList<Abi> abiList;

    if (!path.isEmpty()) {
        QFileInfo fi(path.toFileInfo());
        haveCompiler = fi.isExecutable() && fi.isFile();
    }
    if (haveCompiler) {
        Environment env = Environment::systemEnvironment();
        GccToolChain::addCommandPathToEnvironment(path, env);
        QStringList args = gccPredefinedMacrosOptions(Constants::CXX_LANGUAGE_ID)
                + splitString(m_platformCodeGenFlagsLineEdit->text());
        const FileName localCompilerPath = findLocalCompiler(path, env);
        m_macros = gccPredefinedMacros(localCompilerPath, args, env.toStringList());
        abiList = guessGccAbi(localCompilerPath, env.toStringList(), m_macros,
                              splitString(m_platformCodeGenFlagsLineEdit->text())).supportedAbis;
    }
    m_abiWidget->setEnabled(haveCompiler);

    // Find a good ABI for the new compiler:
    Abi newAbi;
    if (customAbi)
        newAbi = currentAbi;
    else if (abiList.contains(currentAbi))
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

ClangToolChain::ClangToolChain(Detection d) :
    GccToolChain(Constants::CLANG_TOOLCHAIN_TYPEID, d)
{ }

ClangToolChain::ClangToolChain(Core::Id typeId, ToolChain::Detection d) :
    GccToolChain(typeId, d)
{ }

QString ClangToolChain::typeDisplayName() const
{
    return ClangToolChainFactory::tr("Clang");
}

QString ClangToolChain::makeCommand(const Environment &environment) const
{
    const QStringList makes
            = HostOsInfo::isWindowsHost() ? QStringList({"mingw32-make.exe", "make.exe"}) : QStringList({"make"});

    FileName tmp;
    for (const QString &make : makes) {
        tmp = environment.searchInPath(make);
        if (!tmp.isEmpty())
            return tmp.toString();
    }
    return makes.first();
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

FileNameList ClangToolChain::suggestedMkspecList() const
{
    Abi abi = targetAbi();
    if (abi.os() == Abi::DarwinOS)
        return FileNameList()
                << FileName::fromLatin1("macx-clang")
                << FileName::fromLatin1("macx-clang-32")
                << FileName::fromLatin1("unsupported/macx-clang")
                << FileName::fromLatin1("macx-ios-clang");
    else if (abi.os() == Abi::LinuxOS)
        return FileNameList()
                << FileName::fromLatin1("linux-clang")
                << FileName::fromLatin1("unsupported/linux-clang");
    return FileNameList(); // Note: Not supported by Qt yet, so default to the mkspec the Qt was build with
}

void ClangToolChain::addToEnvironment(Environment &env) const
{
    GccToolChain::addToEnvironment(env);
    // Clang takes PWD as basis for debug info, if set.
    // When running Qt Creator from a shell, PWD is initially set to an "arbitrary" value.
    // Since the tools are not called through a shell, PWD is never changed to the actual cwd,
    // so we better make sure PWD is empty to begin with
    env.unset("PWD");
}

LanguageExtensions ClangToolChain::defaultLanguageExtensions() const
{
    return LanguageExtension::Gnu;
}

IOutputParser *ClangToolChain::outputParser() const
{
    return new ClangParser;
}

ToolChain *ClangToolChain::clone() const
{
    return new ClangToolChain(*this);
}

// --------------------------------------------------------------------------
// ClangToolChainFactory
// --------------------------------------------------------------------------

ClangToolChainFactory::ClangToolChainFactory()
{
    setDisplayName(tr("Clang"));
}

QSet<Core::Id> ClangToolChainFactory::supportedLanguages() const
{
    return {Constants::CXX_LANGUAGE_ID,
            Constants::C_LANGUAGE_ID};
}

QList<ToolChain *> ClangToolChainFactory::autoDetect(const QList<ToolChain *> &alreadyKnown)
{
    QList<ToolChain *> tcs;
    QList<ToolChain *> known = alreadyKnown;

    const Abi hostAbi = Abi::hostAbi();
    tcs.append(autoDetectToolchains(compilerPathFromEnvironment("clang++"), hostAbi,
                                    Constants::CXX_LANGUAGE_ID, Constants::CLANG_TOOLCHAIN_TYPEID,
                                    alreadyKnown));
    tcs.append(autoDetectToolchains(compilerPathFromEnvironment("clang"), hostAbi,
                                    Constants::C_LANGUAGE_ID, Constants::CLANG_TOOLCHAIN_TYPEID,
                                    alreadyKnown));
    known.append(tcs);
    versionProbe("clang++", Constants::CXX_LANGUAGE_ID, Constants::CLANG_TOOLCHAIN_TYPEID, tcs, known);
    versionProbe("clang", Constants::C_LANGUAGE_ID, Constants::CLANG_TOOLCHAIN_TYPEID, tcs, known);

    const FileName compilerPath = FileName::fromString(Core::ICore::clangExecutable(CLANG_BINDIR));
    if (!compilerPath.isEmpty()) {
        const FileName clang = compilerPath.parentDir().appendPath(
                    HostOsInfo::withExecutableSuffix("clang"));
        tcs.append(autoDetectToolchains(clang,
                                        hostAbi, Constants::CXX_LANGUAGE_ID,
                                        Constants::CLANG_TOOLCHAIN_TYPEID, alreadyKnown));
        tcs.append(autoDetectToolchains(clang,
                                        hostAbi, Constants::C_LANGUAGE_ID,
                                        Constants::CLANG_TOOLCHAIN_TYPEID, alreadyKnown));
    }
    return tcs;
}

QList<ToolChain *> ClangToolChainFactory::autoDetect(const FileName &compilerPath, const Core::Id &language)
{
    const QString fileName = compilerPath.fileName();
    if ((language == Constants::C_LANGUAGE_ID && fileName.startsWith("clang") && !fileName.startsWith("clang++"))
            || (language == Constants::CXX_LANGUAGE_ID && fileName.startsWith("clang++")))
        return autoDetectToolChain(compilerPath, language);
    return QList<ToolChain *>();
}

bool ClangToolChainFactory::canRestore(const QVariantMap &data)
{
    return typeIdFromMap(data) == Constants::CLANG_TOOLCHAIN_TYPEID;
}

GccToolChain *ClangToolChainFactory::createToolChain(bool autoDetect)
{
    return new ClangToolChain(autoDetect ? ToolChain::AutoDetection : ToolChain::ManualDetection);
}

// --------------------------------------------------------------------------
// MingwToolChain
// --------------------------------------------------------------------------

MingwToolChain::MingwToolChain(Detection d) :
    GccToolChain(Constants::MINGW_TOOLCHAIN_TYPEID, d)
{ }

QString MingwToolChain::typeDisplayName() const
{
    return MingwToolChainFactory::tr("MinGW");
}

FileNameList MingwToolChain::suggestedMkspecList() const
{
    if (HostOsInfo::isWindowsHost())
        return FileNameList() << FileName::fromLatin1("win32-g++");
    if (HostOsInfo::isLinuxHost()) {
        if (version().startsWith("4.6."))
            return FileNameList()
                    << FileName::fromLatin1("win32-g++-4.6-cross")
                    << FileName::fromLatin1("unsupported/win32-g++-4.6-cross");
        else
            return FileNameList()
                    << FileName::fromLatin1("win32-g++-cross")
                    << FileName::fromLatin1("unsupported/win32-g++-cross");
    }
    return FileNameList();
}

QString MingwToolChain::makeCommand(const Environment &environment) const
{
    const QStringList makes
            = HostOsInfo::isWindowsHost() ? QStringList({"mingw32-make.exe", "make.exe"}) : QStringList({"make"});

    FileName tmp;
    foreach (const QString &make, makes) {
        tmp = environment.searchInPath(make);
        if (!tmp.isEmpty())
            return tmp.toString();
    }
    return makes.first();
}

ToolChain *MingwToolChain::clone() const
{
    return new MingwToolChain(*this);
}

// --------------------------------------------------------------------------
// MingwToolChainFactory
// --------------------------------------------------------------------------

MingwToolChainFactory::MingwToolChainFactory()
{
    setDisplayName(tr("MinGW"));
}

QSet<Core::Id> MingwToolChainFactory::supportedLanguages() const
{
    return {Constants::CXX_LANGUAGE_ID,
            Constants::C_LANGUAGE_ID};
}

QList<ToolChain *> MingwToolChainFactory::autoDetect(const QList<ToolChain *> &alreadyKnown)
{
    Abi ha = Abi::hostAbi();
    ha = Abi(ha.architecture(), Abi::WindowsOS, Abi::WindowsMSysFlavor, Abi::PEFormat, ha.wordWidth());
    QList<ToolChain *> result = autoDetectToolchains(
                compilerPathFromEnvironment("g++"), ha, Constants::CXX_LANGUAGE_ID,
                Constants::MINGW_TOOLCHAIN_TYPEID, alreadyKnown);
    result += autoDetectToolchains(
                compilerPathFromEnvironment("gcc"), ha, Constants::C_LANGUAGE_ID,
                Constants::MINGW_TOOLCHAIN_TYPEID, alreadyKnown);
    return result;
}

QList<ToolChain *> MingwToolChainFactory::autoDetect(const FileName &compilerPath, const Core::Id &language)
{
    Abi ha = Abi::hostAbi();
    ha = Abi(ha.architecture(), Abi::WindowsOS, Abi::WindowsMSysFlavor, Abi::PEFormat, ha.wordWidth());
    const QString fileName = compilerPath.fileName();
    if ((language == Constants::C_LANGUAGE_ID && (fileName.startsWith("gcc")
                                                  || fileName.endsWith("gcc")))
            || (language == Constants::CXX_LANGUAGE_ID && (fileName.startsWith("g++")
                || fileName.endsWith("g++"))))
        return autoDetectToolChain(compilerPath, language, ha);
    return QList<ToolChain *>();
}

bool MingwToolChainFactory::canRestore(const QVariantMap &data)
{
    return typeIdFromMap(data) == Constants::MINGW_TOOLCHAIN_TYPEID;
}

GccToolChain *MingwToolChainFactory::createToolChain(bool autoDetect)
{
    return new MingwToolChain(autoDetect ? ToolChain::AutoDetection : ToolChain::ManualDetection);
}

// --------------------------------------------------------------------------
// LinuxIccToolChain
// --------------------------------------------------------------------------

LinuxIccToolChain::LinuxIccToolChain(Detection d) :
    GccToolChain(Constants::LINUXICC_TOOLCHAIN_TYPEID, d)
{ }

QString LinuxIccToolChain::typeDisplayName() const
{
    return LinuxIccToolChainFactory::tr("Linux ICC");
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

IOutputParser *LinuxIccToolChain::outputParser() const
{
    return new LinuxIccParser;
}

FileNameList LinuxIccToolChain::suggestedMkspecList() const
{
    return FileNameList()
            << FileName::fromString(QString::fromLatin1("linux-icc-") + QString::number(targetAbi().wordWidth()));
}

ToolChain *LinuxIccToolChain::clone() const
{
    return new LinuxIccToolChain(*this);
}

// --------------------------------------------------------------------------
// LinuxIccToolChainFactory
// --------------------------------------------------------------------------

LinuxIccToolChainFactory::LinuxIccToolChainFactory()
{
    setDisplayName(tr("Linux ICC"));
}

QSet<Core::Id> LinuxIccToolChainFactory::supportedLanguages() const
{
    return {Constants::CXX_LANGUAGE_ID, Constants::C_LANGUAGE_ID};
}

QList<ToolChain *> LinuxIccToolChainFactory::autoDetect(const QList<ToolChain *> &alreadyKnown)
{
    QList<ToolChain *> result
            = autoDetectToolchains(compilerPathFromEnvironment("icpc"),
                                   Abi::hostAbi(), Constants::CXX_LANGUAGE_ID,
                                   Constants::LINUXICC_TOOLCHAIN_TYPEID, alreadyKnown);
    result += autoDetectToolchains(compilerPathFromEnvironment("icc"),
                                   Abi::hostAbi(), Constants::C_LANGUAGE_ID,
                                   Constants::LINUXICC_TOOLCHAIN_TYPEID, alreadyKnown);
    return result;
}

QList<ToolChain *> LinuxIccToolChainFactory::autoDetect(const FileName &compilerPath, const Core::Id &language)
{
    const QString fileName = compilerPath.fileName();
    if ((language == Constants::CXX_LANGUAGE_ID && fileName.startsWith("icpc")) ||
        (language == Constants::C_LANGUAGE_ID && fileName.startsWith("icc")))
        return autoDetectToolChain(compilerPath, language);
    return {};
}

bool LinuxIccToolChainFactory::canRestore(const QVariantMap &data)
{
    return typeIdFromMap(data) == Constants::LINUXICC_TOOLCHAIN_TYPEID;
}

GccToolChain *LinuxIccToolChainFactory::createToolChain(bool autoDetect)
{
    return new LinuxIccToolChain(autoDetect ? ToolChain::AutoDetection : ToolChain::ManualDetection);
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
            << QStringList({"arm-unknown-unknown-elf-64bit"});
    QTest::newRow("broken input -- 32bit")
            << QString::fromLatin1("arm-none-foo-gnueabi")
            << QByteArray("#define __SIZEOF_SIZE_T__ 4\n#define __Something\n")
            << QStringList({"arm-unknown-unknown-elf-32bit"});
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
            << QStringList({"x86-linux-generic-elf-64bit",  "x86-linux-generic-elf-32bit"});
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
            << QStringList({"x86-linux-generic-elf-64bit", "x86-linux-generic-elf-32bit"});
    QTest::newRow("Linux 6 (QTCREATORBUG-4690)") // from QTCREATORBUG-4690
            << QString::fromLatin1("x86_64-redhat-linux")
            << QByteArray("#define __SIZEOF_SIZE_T__ 8\n")
            << QStringList({"x86-linux-generic-elf-64bit", "x86-linux-generic-elf-32bit"});
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
            << QStringList({"x86-windows-msys-pe-64bit", "x86-windows-msys-pe-32bit"});
    QTest::newRow("Mingw 3 (32 bit)")
            << QString::fromLatin1("mingw32")
            << QByteArray("#define __SIZEOF_SIZE_T__ 4\r\n")
            << QStringList({"x86-windows-msys-pe-32bit"});
    QTest::newRow("Cross Mingw 1 (64bit)")
            << QString::fromLatin1("amd64-mingw32msvc")
            << QByteArray("#define __SIZEOF_SIZE_T__ 8\r\n")
            << QStringList({"x86-windows-msys-pe-64bit", "x86-windows-msys-pe-32bit"});
    QTest::newRow("Cross Mingw 2 (32bit)")
            << QString::fromLatin1("i586-mingw32msvc")
            << QByteArray("#define __SIZEOF_SIZE_T__ 4\r\n")
            << QStringList({"x86-windows-msys-pe-32bit"});
    QTest::newRow("Clang 1: windows")
            << QString::fromLatin1("x86_64-pc-win32")
            << QByteArray("#define __SIZEOF_SIZE_T__ 8\r\n")
            << QStringList({"x86-windows-msys-pe-64bit", "x86-windows-msys-pe-32bit"});
    QTest::newRow("Clang 1: linux")
            << QString::fromLatin1("x86_64-unknown-linux-gnu")
            << QByteArray("#define __SIZEOF_SIZE_T__ 8\n")
            << QStringList({"x86-linux-generic-elf-64bit", "x86-linux-generic-elf-32bit"});
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
            << QStringList({"x86-linux-generic-elf-64bit", "x86-linux-generic-elf-32bit"});
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

    QList<Abi> al = guessGccAbi(input, ProjectExplorer::Macro::toMacros(macros));
    QCOMPARE(al.count(), abiList.count());
    for (int i = 0; i < al.count(); ++i)
        QCOMPARE(al.at(i).toString(), abiList.at(i));
}

} // namespace ProjectExplorer

#endif
