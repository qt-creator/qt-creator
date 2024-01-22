// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gcctoolchain.h"

#include "abiwidget.h"
#include "clangparser.h"
#include "devicesupport/idevice.h"
#include "gccparser.h"
#include "linuxiccparser.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "projectmacro.h"
#include "toolchainconfigwidget.h"
#include "toolchainmanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/pathchooser.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/scopedtimer.h>

#include <QBuffer>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
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
namespace Internal {

static const QStringList languageOption(Id languageId)
{
    if (languageId == Constants::C_LANGUAGE_ID)
        return {"-x", "c"};
    return {"-x", "c++"};
}

const QStringList gccPredefinedMacrosOptions(Id languageId)
{
    return languageOption(languageId) + QStringList({"-E", "-dM"});
}

class TargetTripleWidget;
class GccToolChainConfigWidget : public ToolChainConfigWidget
{
public:
    explicit GccToolChainConfigWidget(GccToolChain *tc);

private:
    void handleCompilerCommandChange();
    void handlePlatformCodeGenFlagsChange();
    void handlePlatformLinkerFlagsChange();

    void applyImpl() override;
    void discardImpl() override { setFromToolchain(); }
    bool isDirtyImpl() const override;
    void makeReadOnlyImpl() override;

    void setFromToolchain();

    void updateParentToolChainComboBox(); // Clang

    AbiWidget *m_abiWidget;

    GccToolChain::SubType m_subType = GccToolChain::RealGcc;

    PathChooser *m_compilerCommand;
    QLineEdit *m_platformCodeGenFlagsLineEdit;
    QLineEdit *m_platformLinkerFlagsLineEdit;
    TargetTripleWidget * const m_targetTripleWidget;

    bool m_isReadOnly = false;
    ProjectExplorer::Macros m_macros;

    // Clang only
    QList<QMetaObject::Connection> m_parentToolChainConnections;
    QComboBox *m_parentToolchainCombo = nullptr;
};

} // namespace Internal

using namespace Internal;

// --------------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------------

const char compilerPlatformCodeGenFlagsKeyC[] = "ProjectExplorer.GccToolChain.PlatformCodeGenFlags";
const char compilerPlatformLinkerFlagsKeyC[] = "ProjectExplorer.GccToolChain.PlatformLinkerFlags";
const char targetAbiKeyC[] = "ProjectExplorer.GccToolChain.TargetAbi";
const char originalTargetTripleKeyC[] = "ProjectExplorer.GccToolChain.OriginalTargetTriple";
const char supportedAbisKeyC[] = "ProjectExplorer.GccToolChain.SupportedAbis";
const char parentToolChainIdKeyC[] = "ProjectExplorer.ClangToolChain.ParentToolChainId";
const char priorityKeyC[] = "ProjectExplorer.ClangToolChain.Priority";
const char binaryRegexp[] = "(?:^|-|\\b)(?:gcc|g\\+\\+|clang(?:\\+\\+)?)(?:-([\\d.]+))?$";

static QString runGcc(const FilePath &gcc, const QStringList &arguments, const Environment &env)
{
    if (!gcc.isExecutableFile())
        return {};

    Process cpp;
    Environment environment(env);
    environment.setupEnglishOutput();

    cpp.setEnvironment(environment);
    cpp.setTimeoutS(10);
    cpp.setCommand({gcc, arguments});
    cpp.runBlocking();
    if (cpp.result() != ProcessResult::FinishedWithSuccess || cpp.exitCode() != 0) {
        Core::MessageManager::writeFlashing({"Compiler feature detection failure!",
                                             cpp.exitMessage(),
                                             cpp.allOutput()});
        return {};
    }

    return cpp.allOutput();
}

static ProjectExplorer::Macros gccPredefinedMacros(const FilePath &gcc,
                                                   const QStringList &args,
                                                   const Environment &env)
{
    QStringList arguments = args;
    arguments << "-";

    ProjectExplorer::Macros  predefinedMacros = Macro::toMacros(runGcc(gcc, arguments, env).toUtf8());
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

HeaderPaths GccToolChain::gccHeaderPaths(const FilePath &gcc,
                                         const QStringList &arguments,
                                         const Environment &env)
{
    HeaderPaths builtInHeaderPaths;
    QByteArray line;
    QByteArray data = runGcc(gcc, arguments, env).toUtf8();
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

                const FilePath headerPath
                    = gcc.withNewPath(QString::fromUtf8(line)).canonicalPath();

                if (!headerPath.isEmpty())
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


static GccToolChain::DetectedAbisResult guessGccAbi(const FilePath &path,
                                                    const Environment &env,
                                                    const Macros &macros,
                                                    const QStringList &extraArgs = {})
{
    if (path.isEmpty())
        return GccToolChain::DetectedAbisResult();

    QStringList arguments = extraArgs;
    arguments << "-dumpmachine";
    QString machine = runGcc(path, arguments, env).trimmed().section('\n', 0, 0, QString::SectionSkipEmpty);
    if (machine.isEmpty()) {
        // ICC does not implement the -dumpmachine option on macOS.
        if (HostOsInfo::isMacHost() && (path.fileName() == "icc" || path.fileName() == "icpc"))
            return GccToolChain::DetectedAbisResult({Abi::hostAbi()});
        return GccToolChain::DetectedAbisResult(); // no need to continue if running failed once...
    }
    return GccToolChain::DetectedAbisResult(guessGccAbi(machine, macros), machine);
}

static QString gccVersion(const FilePath &path,
                          const Environment &env,
                          const QStringList &extraArgs)
{
    QStringList arguments = extraArgs;
    arguments << "-dumpversion";
    return runGcc(path, arguments, env).trimmed();
}

static FilePath gccInstallDir(const FilePath &compiler,
                              const Environment &env,
                              const QStringList &extraArgs = {})
{
    QStringList arguments = extraArgs;
    arguments << "-print-search-dirs";
    QString output = runGcc(compiler, arguments, env).trimmed();
    // Expected output looks like this:
    //   install: /usr/lib/gcc/x86_64-linux-gnu/7/
    //   ...
    // Note that clang also supports "-print-search-dirs". However, the
    // install dir is not part of the output (tested with clang-8/clang-9).

    const QString prefix = "install: ";
    const QString line = QTextStream(&output).readLine();
    if (!line.startsWith(prefix))
        return {};
    return compiler.withNewPath(QDir::cleanPath(line.mid(prefix.size())));
}

// --------------------------------------------------------------------------
// GccToolChain
// --------------------------------------------------------------------------

static Id idForSubType(GccToolChain::SubType subType)
{
    switch (subType) {
    case GccToolChain::RealGcc:
        return Constants::GCC_TOOLCHAIN_TYPEID;
    case GccToolChain::Clang:
        return Constants::CLANG_TOOLCHAIN_TYPEID;
    case GccToolChain::MinGW:
        return Constants::MINGW_TOOLCHAIN_TYPEID;
    case GccToolChain::LinuxIcc:
        return Constants::LINUXICC_TOOLCHAIN_TYPEID;
    }
    QTC_CHECK(false);
    return Constants::GCC_TOOLCHAIN_TYPEID;
}

GccToolChain::GccToolChain(Id typeId, SubType subType)
    : ToolChain(typeId.isValid() ? typeId : idForSubType(subType)), m_subType(subType)
{
    setTypeDisplayName(Tr::tr("GCC"));
    setTargetAbiKey(targetAbiKeyC);
    setCompilerCommandKey("ProjectExplorer.GccToolChain.Path");
    if (m_subType == LinuxIcc)  {
        setTypeDisplayName(Tr::tr("ICC"));
    } else if (m_subType == MinGW) {
        setTypeDisplayName(Tr::tr("MinGW"));
    } else if (m_subType == Clang) {
        setTypeDisplayName(Tr::tr("Clang"));
        syncAutodetectedWithParentToolchains();
    }
}

GccToolChain::~GccToolChain()
{
    if (m_subType == Clang) {
        QObject::disconnect(m_thisToolchainRemovedConnection);
        QObject::disconnect(m_mingwToolchainAddedConnection);
    }
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

void GccToolChain::setInstallDir(const FilePath &installDir)
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
    const QRegularExpressionMatch match = regexp.match(compilerCommand().fileName());
    if (match.lastCapturedIndex() >= 1)
        type += ' ' + match.captured(1);
    const Abi abi = targetAbi();
    if (abi.architecture() == Abi::UnknownArchitecture || abi.wordWidth() == 0)
        return type;
    return Tr::tr("%1 (%2, %3 %4 at %5)").arg(type,
                                          ToolChainManager::displayNameOfLanguageId(language()),
                                          Abi::toString(abi.architecture()),
                                          Abi::toString(abi.wordWidth()),
                                          compilerCommand().toUserOutput());
}

LanguageExtensions GccToolChain::defaultLanguageExtensions() const
{
    return LanguageExtension::Gnu;
}

static const Toolchains mingwToolChains()
{
    return ToolChainManager::toolchains([](const ToolChain *tc) -> bool {
        return tc->typeId() == Constants::MINGW_TOOLCHAIN_TYPEID;
    });
}

static const GccToolChain *mingwToolChainFromId(const QByteArray &id)
{
    if (id.isEmpty())
        return nullptr;

    for (const ToolChain *tc : mingwToolChains()) {
        if (tc->id() == id)
            return static_cast<const GccToolChain *>(tc);
    }

    return nullptr;
}

QString GccToolChain::originalTargetTriple() const
{
    if (m_subType == Clang) {
        if (const GccToolChain *parentTC = mingwToolChainFromId(m_parentToolChainId))
            return parentTC->originalTargetTriple();
    }

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

Abis GccToolChain::supportedAbis() const
{
    return m_supportedAbis;
}

static bool isNetworkCompiler(const QString &dirPath)
{
    return dirPath.contains("icecc") || dirPath.contains("distcc");
}

static FilePath findLocalCompiler(const FilePath &compilerPath, const Environment &env)
{
    // Find the "real" compiler if icecc, distcc or similar are in use. Ignore ccache, since that
    // is local already.

    // Get the path to the compiler, ignoring direct calls to icecc and distcc as we cannot
    // do anything about those.
    if (!isNetworkCompiler(compilerPath.parentDir().path()))
        return compilerPath;

    // Filter out network compilers
    const FilePaths pathComponents = Utils::filtered(env.path(), [] (const FilePath &dirPath) {
        return !isNetworkCompiler(dirPath.path());
    });

    // This effectively searches the PATH twice, once via pathComponents and once via PATH itself:
    // searchInPath filters duplicates, so that will not hurt.
    const FilePath path = env.searchInPath(compilerPath.fileName(), pathComponents);

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
        }  else if (a == "-Xclang") {
            filtered << a;
            continue;
        } else if ((considerSysroot && (a == "--sysroot" || a == "-isysroot"))
                   || a == "-D" || a == "-U"
                   || a == "-gcc-toolchain" || a == "-target" || a == "-mllvm" || a == "-isystem") {
            if (++i < allFlags.length())
                filtered << a << allFlags.at(i);
        } else if (a.startsWith("-m") || a.startsWith("-f") || a.startsWith("-O")
                   || a.startsWith("-std=") || a.startsWith("-stdlib=")
                   || a.startsWith("-specs=") || a == "-ansi" || a == "-undef"
                   || a.startsWith("-D") || a.startsWith("-U")
                   || a.startsWith("-stdlib=") || a.startsWith("-B")
                   || a.startsWith("--target=")
                   || (a.startsWith("-isystem") && a.length() > 8)
                   || a == "-Wno-deprecated"
                   || a == "-nostdinc" || a == "-nostdinc++") {
            filtered << a;
        }
        if (!filtered.isEmpty() && filtered.last() == "-Xclang")
            filtered.removeLast();
    }
    return filtered;
}

ToolChain::MacroInspectionRunner GccToolChain::createMacroInspectionRunner() const
{
    // Using a clean environment breaks ccache/distcc/etc.
    Environment env = compilerCommand().deviceEnvironment();
    addToEnvironment(env);
    const QStringList platformCodeGenFlags = m_platformCodeGenFlags;
    OptionsReinterpreter reinterpretOptions = m_optionsReinterpreter;
    QTC_CHECK(reinterpretOptions);
    MacrosCache macroCache = predefinedMacrosCache();
    Id lang = language();

    /*
     * Asks compiler for set of predefined macros
     * flags are the compiler flags collected from project settings
     * returns the list of defines, one per line, e.g. "#define __GXX_WEAK__ 1"
     * Note: changing compiler flags sometimes changes macros set, e.g. -fopenmp
     * adds _OPENMP macro, for full list of macro search by word "when" on this page:
     * http://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
     *
     * This runner must be thread-safe!
     */
    return [env, compilerCommand = compilerCommand(),
            platformCodeGenFlags, reinterpretOptions, macroCache, lang]
            (const QStringList &flags) {
        QStringList allFlags = platformCodeGenFlags + flags;  // add only cxxflags is empty?
        QStringList arguments = gccPredefinedMacrosOptions(lang) + filteredFlags(allFlags, true);
        arguments = reinterpretOptions(arguments);
        const std::optional<MacroInspectionReport> cachedMacros = macroCache->check(arguments);
        if (cachedMacros)
            return cachedMacros.value();

        const Macros macros = gccPredefinedMacros(findLocalCompiler(compilerCommand, env),
                                                  arguments,
                                                  env);

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
 * @brief Parses gcc flags -std=*, -fopenmp, -fms-extensions.
 * @see http://gcc.gnu.org/onlinedocs/gcc/C-Dialect-Options.html
 */
LanguageExtensions GccToolChain::languageExtensions(const QStringList &cxxflags) const
{
    LanguageExtensions extensions = defaultLanguageExtensions();

    const QStringList allCxxflags = m_platformCodeGenFlags + cxxflags; // add only cxxflags is empty?
    for (const QString &flag : allCxxflags) {
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

    // Clang knows -fborland-extensions".
    if (m_subType == Clang && cxxflags.contains("-fborland-extensions"))
        extensions |= LanguageExtension::Borland;

    if (m_subType == LinuxIcc) {
        // and "-fms-dialect[=ver]" instead of "-fms-extensions".
        // see UNIX manual for "icc"
        // FIXME: This copy seems unneeded.
        QStringList copy = cxxflags;
        copy.removeAll("-fopenmp");
        copy.removeAll("-fms-extensions");

        if (cxxflags.contains("-openmp"))
            extensions |= LanguageExtension::OpenMP;
        if (cxxflags.contains("-fms-dialect")
                || cxxflags.contains("-fms-dialect=8")
                || cxxflags.contains("-fms-dialect=9")
                || cxxflags.contains("-fms-dialect=10"))
            extensions |= LanguageExtension::Microsoft;
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

    for (int end = cflags.size(), i = 0; i != end; ++i) {
        const QString &flag = cflags[i];
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

    if (m_subType == Clang) {
        for (int end = cflags.size(), i = 0; i != end; ++i) {
            const QString &flag = cflags[i];
            if (flag == "-Wdocumentation")
                flags |= WarningFlags::Documentation;
            if (flag == "-Wno-documentation")
                flags &= ~WarningFlags::Documentation;
        }
    }

    return flags;
}

FilePaths GccToolChain::includedFiles(const QStringList &flags, const FilePath &directoryPath) const
{
    return ToolChain::includedFiles("-include", flags, directoryPath, PossiblyConcatenatedFlag::No);
}

static QStringList gccPrepareArguments(const QStringList &flags,
                                       const FilePath &sysRoot,
                                       const QStringList &platformCodeGenFlags,
                                       Id languageId)
{
    QStringList arguments;
    const bool hasKitSysroot = !sysRoot.isEmpty();
    if (hasKitSysroot)
        arguments.append(QString("--sysroot=%1").arg(sysRoot.nativePath()));

    QStringList allFlags;
    allFlags << platformCodeGenFlags << flags;
    arguments += filteredFlags(allFlags, !hasKitSysroot);
    arguments << languageOption(languageId) << "-E" << "-v" << "-";

    return arguments;
}

// NOTE: extraHeaderPathsFunction must NOT capture this or it's members!!!
void GccToolChain::initExtraHeaderPathsFunction(ExtraHeaderPathsFunction &&extraHeaderPathsFunction) const
{
    m_extraHeaderPathsFunction = std::move(extraHeaderPathsFunction);
}

HeaderPaths GccToolChain::builtInHeaderPaths(const Environment &env,
                                             const FilePath &compilerCommand,
                                             const QStringList &platformCodeGenFlags,
                                             OptionsReinterpreter reinterpretOptions,
                                             HeaderPathsCache headerCache,
                                             Id languageId,
                                             ExtraHeaderPathsFunction extraHeaderPathsFunction,
                                             const QStringList &flags,
                                             const FilePath &sysRoot,
                                             const QString &originalTargetTriple)
{
    QStringList arguments = gccPrepareArguments(flags,
                                                sysRoot,
                                                platformCodeGenFlags,
                                                languageId);
    arguments = reinterpretOptions(arguments);

    // Must be clang case only.
    if (!originalTargetTriple.isEmpty())
        arguments << "-target" << originalTargetTriple;

    const std::optional<HeaderPaths> cachedPaths = headerCache->check({env, arguments});
    if (cachedPaths)
        return cachedPaths.value();

    HeaderPaths paths = gccHeaderPaths(findLocalCompiler(compilerCommand, env),
                                       arguments,
                                       env);
    extraHeaderPathsFunction(paths);
    headerCache->insert({env, arguments}, paths);

    qCDebug(gccLog) << "Reporting header paths to code model:";
    for (const HeaderPath &hp : std::as_const(paths)) {
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

    if (m_subType == Clang) {
        // This runner must be thread-safe!
        return [fullEnv,
                compilerCommand = compilerCommand(),
                platformCodeGenFlags = m_platformCodeGenFlags,
                reinterpretOptions = m_optionsReinterpreter,
                headerCache = headerPathsCache(),
                languageId = language(),
                extraHeaderPathsFunction = m_extraHeaderPathsFunction](const QStringList &flags,
                                                                       const FilePath &sysRoot,
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

    // This runner must be thread-safe!
    return [fullEnv,
            compilerCommand = compilerCommand(),
            platformCodeGenFlags = m_platformCodeGenFlags,
            reinterpretOptions = m_optionsReinterpreter,
            headerCache = headerPathsCache(),
            languageId = language(),
            extraHeaderPathsFunction = m_extraHeaderPathsFunction](const QStringList &flags,
                                                                   const FilePath &sysRoot,
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

void GccToolChain::addCommandPathToEnvironment(const FilePath &command, Environment &env)
{
    env.prependOrSetPath(command.parentDir());
}

void GccToolChain::addToEnvironment(Environment &env) const
{
    // On Windows gcc invokes cc1plus which is in libexec directory.
    // cc1plus depends on libwinpthread-1.dll which is in bin, so bin must be in the PATH.
    if (compilerCommand().osType() == OsTypeWindows)
        addCommandPathToEnvironment(compilerCommand(), env);

    if (m_subType == Clang) {
        const QString sysroot = sysRoot();
        if (!sysroot.isEmpty())
            env.prependOrSetPath(FilePath::fromString(sysroot) / "bin");

        // Clang takes PWD as basis for debug info, if set.
        // When running Qt Creator from a shell, PWD is initially set to an "arbitrary" value.
        // Since the tools are not called through a shell, PWD is never changed to the actual cwd,
        // so we better make sure PWD is empty to begin with
        env.unset("PWD");
    }
}

QStringList GccToolChain::suggestedMkspecList() const
{
    if (m_subType == LinuxIcc)
        return {QString("linux-icc-%1").arg(targetAbi().wordWidth())};

    if (m_subType == MinGW) {
        if (HostOsInfo::isWindowsHost())
            return {"win32-g++"};
        if (HostOsInfo::isLinuxHost()) {
            if (version().startsWith("4.6."))
                return {"win32-g++-4.6-cross", "unsupported/win32-g++-4.6-cross"};
            return {"win32-g++-cross", "unsupported/win32-g++-cross"};
        }
        return {};
    }

    if (m_subType == Clang) {
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
        if (v.startsWith("4.0") && compilerCommand().endsWith("-4.0"))
            return {"macx-g++40"};
        if (v.startsWith("4.2") && compilerCommand().endsWith("-4.2"))
            return {"macx-g++42"};
        return {"macx-g++"};
    }

    if (abi.os() == Abi::LinuxOS) {
        if (abi.osFlavor() != Abi::GenericFlavor)
            return {}; // most likely not a desktop, so leave the mkspec alone.
        if (abi.wordWidth() == host.wordWidth()) {
            // no need to explicitly set the word width, but provide that mkspec anyway to make sure
            // that the correct compiler is picked if a mkspec with a wordwidth is given.
            return {"linux-g++", "linux-g++-" + QString::number(targetAbi().wordWidth())};
        }
        return {"linux-g++-" + QString::number(targetAbi().wordWidth())};
    }

    if (abi.os() == Abi::BsdOS && abi.osFlavor() == Abi::FreeBsdFlavor)
        return {"freebsd-g++"};

    return {};
}

static FilePath mingwAwareMakeCommand(const Environment &environment)
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

FilePath GccToolChain::makeCommand(const Environment &environment) const
{
    if (m_subType == Clang || m_subType == MinGW)
        return mingwAwareMakeCommand(environment);

    const FilePath tmp = environment.searchInPath("make");
    return tmp.isEmpty() ? "make" : tmp;
}

QList<OutputLineParser *> GccToolChain::createOutputParsers() const
{
    if (m_subType == LinuxIcc)
        return LinuxIccParser::iccParserSuite();

    if (m_subType == Clang)
        return ClangParser::clangParserSuite();

    return GccParser::gccParserSuite();
}

void GccToolChain::resetToolChain(const FilePath &path)
{
    bool resetDisplayName = (displayName() == defaultDisplayName());

    setCompilerCommand(path);

    const Abi currentAbi = targetAbi();
    const DetectedAbisResult detectedAbis = detectSupportedAbis();
    m_supportedAbis = detectedAbis.supportedAbis;
    m_originalTargetTriple = detectedAbis.originalTargetTriple;
    m_installDir = installDir();

    if (m_supportedAbis.isEmpty())
        setTargetAbiNoSignal(Abi());
    else if (!m_supportedAbis.contains(currentAbi))
        setTargetAbiNoSignal(m_supportedAbis.at(0));

    if (resetDisplayName)
        setDisplayName(defaultDisplayName()); // calls toolChainUpdated()!
    else
        toolChainUpdated();
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

void GccToolChain::toMap(Store &data) const
{
    ToolChain::toMap(data);
    data.insert(compilerPlatformCodeGenFlagsKeyC, m_platformCodeGenFlags);
    data.insert(compilerPlatformLinkerFlagsKeyC, m_platformLinkerFlags);
    data.insert(originalTargetTripleKeyC, m_originalTargetTriple);
    data.insert(supportedAbisKeyC, Utils::transform<QStringList>(m_supportedAbis, &Abi::toString));

    if (m_subType == Clang) {
        data.insert(parentToolChainIdKeyC, m_parentToolChainId);
        data.insert(priorityKeyC, m_priority);
    }
}

void GccToolChain::fromMap(const Store &data)
{
    ToolChain::fromMap(data);
    if (hasError())
        return;

    m_platformCodeGenFlags = data.value(compilerPlatformCodeGenFlagsKeyC).toStringList();
    m_platformLinkerFlags = data.value(compilerPlatformLinkerFlagsKeyC).toStringList();
    m_originalTargetTriple = data.value(originalTargetTripleKeyC).toString();
    const QStringList abiList = data.value(supportedAbisKeyC).toStringList();
    m_supportedAbis.clear();
    for (const QString &a : abiList)
        m_supportedAbis.append(Abi::fromString(a));

    const QString targetAbiString = data.value(targetAbiKeyC).toString();
    if (targetAbiString.isEmpty())
        resetToolChain(compilerCommand());

    if (m_subType == Clang) {
        m_parentToolChainId = data.value(parentToolChainIdKeyC).toByteArray();
        m_priority = data.value(priorityKeyC, PriorityNormal).toInt();
        syncAutodetectedWithParentToolchains();
    }
}

bool GccToolChain::operator ==(const ToolChain &other) const
{
    if (!ToolChain::operator ==(other))
        return false;

    auto gccTc = static_cast<const GccToolChain *>(&other);
    return compilerCommand() == gccTc->compilerCommand() && targetAbi() == gccTc->targetAbi()
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
    Environment env = compilerCommand().deviceEnvironment();
    addToEnvironment(env);
    ProjectExplorer::Macros macros = createMacroInspectionRunner()({}).macros;
    return guessGccAbi(findLocalCompiler(compilerCommand(), env),
                       env,
                       macros,
                       platformCodeGenFlags());
}

QString GccToolChain::detectVersion() const
{
    Environment env = compilerCommand().deviceEnvironment();
    addToEnvironment(env);
    return gccVersion(findLocalCompiler(compilerCommand(), env), env,
                      filteredFlags(platformCodeGenFlags(), true));
}

FilePath GccToolChain::detectInstallDir() const
{
    Environment env = compilerCommand().deviceEnvironment();
    addToEnvironment(env);
    return gccInstallDir(findLocalCompiler(compilerCommand(), env), env,
                         filteredFlags(platformCodeGenFlags(), true));
}

// --------------------------------------------------------------------------
// GccToolChainFactory
// --------------------------------------------------------------------------

static FilePaths gnuSearchPathsFromRegistry()
{
    if (!HostOsInfo::isWindowsHost())
        return {};

    // Registry token for the "GNU Tools for ARM Embedded Processors".
    static const char kRegistryToken[] = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\" \
                                         "Windows\\CurrentVersion\\Uninstall\\";

    FilePaths searchPaths;

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

static ToolChain *constructRealGccToolchain()
{
    return new GccToolChain(Constants::GCC_TOOLCHAIN_TYPEID, GccToolChain::RealGcc);
}

static ToolChain *constructClangToolchain()
{
    return new GccToolChain(Constants::CLANG_TOOLCHAIN_TYPEID, GccToolChain::Clang);
}

static ToolChain *constructMinGWToolchain()
{
    return new GccToolChain(Constants::MINGW_TOOLCHAIN_TYPEID, GccToolChain::MinGW);
}

static ToolChain *constructLinuxIccToolchain()
{
    return new GccToolChain(Constants::LINUXICC_TOOLCHAIN_TYPEID, GccToolChain::LinuxIcc);
}

GccToolChainFactory::GccToolChainFactory(GccToolChain::SubType subType)
    : m_autoDetecting(subType == GccToolChain::RealGcc)
{
    switch (subType) {
    case GccToolChain::RealGcc:
        setDisplayName(Tr::tr("GCC"));
        setSupportedToolChainType(Constants::GCC_TOOLCHAIN_TYPEID);
        setToolchainConstructor(&constructRealGccToolchain);
        break;
    case GccToolChain::Clang:
        setDisplayName(Tr::tr("Clang"));
        setSupportedToolChainType(Constants::CLANG_TOOLCHAIN_TYPEID);
        setToolchainConstructor(&constructClangToolchain);
        break;
    case GccToolChain::MinGW:
        setDisplayName(Tr::tr("MinGW"));
        setSupportedToolChainType(Constants::MINGW_TOOLCHAIN_TYPEID);
        setToolchainConstructor(&constructMinGWToolchain);
        break;
    case GccToolChain::LinuxIcc:
        setDisplayName(Tr::tr("ICC"));
        setSupportedToolChainType(Constants::LINUXICC_TOOLCHAIN_TYPEID);
        setToolchainConstructor(&constructLinuxIccToolchain);
        break;
    }
    setSupportedLanguages({Constants::C_LANGUAGE_ID, Constants::CXX_LANGUAGE_ID});
    setUserCreatable(true);
}

static FilePaths findCompilerCandidates(OsType os,
                                        const FilePaths &executables,
                                        const QString &compilerName,
                                        bool detectVariants)
{
    // We expect the following patterns:
    //   compilerName                            "clang", "gcc"
    //   compilerName + "-[1-9]*"                "clang-8", "gcc-5"
    //   "*-" + compilerName                     "avr-gcc", "avr32-gcc"
    //                                           "arm-none-eabi-gcc"
    //                                           "x86_64-pc-linux-gnu-gcc"
    //   "*-" + compilerName + "-[1-9]*"         "avr-gcc-4.8.1", "avr32-gcc-4.4.7"
    //                                           "arm-none-eabi-gcc-9.1.0"
    //                                           "x86_64-pc-linux-gnu-gcc-7.4.1"
    // but not "c89-gcc" or "c99-gcc"

    FilePaths compilerPaths;
    const int cl = compilerName.size();
    for (const FilePath &executable : executables) {
        QStringView fileName = executable.fileNameView();
        if (os == OsTypeWindows && fileName.endsWith(u".exe", Qt::CaseInsensitive))
            fileName.chop(4);

        if (fileName == compilerName)
            compilerPaths << executable;

        if (!detectVariants)
            continue;

        if (fileName == u"c89-gcc" || fileName == u"c99-gcc")
            continue;

        int pos = fileName.indexOf(compilerName);
        if (pos == -1)
            continue;

        // if not at the beginning, it must be preceded by a hyphen.
        if (pos > 0 && fileName.at(pos - 1) != '-')
            continue;

        // if not at the end, it must by followed by a hyphen and a digit between 1 and 9
        pos += cl;
        if (pos != fileName.size()) {
            if (pos + 2 >= fileName.size())
                continue;
            if (fileName.at(pos) != '-')
                continue;
            const QChar c = fileName.at(pos + 1);
            if (c < '1' || c > '9')
                continue;
        }

        compilerPaths << executable;
    }

    return compilerPaths;
}


Toolchains GccToolChainFactory::autoDetect(const ToolchainDetector &detector) const
{
    QTC_ASSERT(detector.device, return {});

    // Do all autodetection in th 'RealGcc' case, and none in the others.
    if (!m_autoDetecting)
        return {};

    const bool isLocal = detector.device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
    FilePaths searchPaths = detector.searchPaths;
    if (!isLocal) {
        if (searchPaths.isEmpty())
            searchPaths = detector.device->systemEnvironment().path();
        searchPaths = Utils::transform(searchPaths, [&](const FilePath &onDevice) {
            return detector.device->filePath(onDevice.path());
        });
    } else if (searchPaths.isEmpty()) {
        searchPaths = Environment::systemEnvironment().path();
        searchPaths << gnuSearchPathsFromRegistry();
        searchPaths << atmelSearchPathsFromRegistry();
        searchPaths << renesasRl78SearchPathsFromRegistry();
        if (HostOsInfo::isMacHost()) {
            searchPaths << "/opt/homebrew/opt/ccache/libexec" // homebrew arm
                        << "/usr/local/opt/ccache/libexec"    // homebrew intel
                        << "/opt/local/libexec/ccache"; // macports, no links are created automatically though
        }
        if (HostOsInfo::isAnyUnixHost()) {
            FilePath ccachePath = "/usr/lib/ccache/bin";
            if (!ccachePath.exists())
                ccachePath = "/usr/lib/ccache";
            if (ccachePath.exists() && !searchPaths.contains(ccachePath))
                searchPaths << ccachePath;
        }
    }

    FilePaths executables;

    QStringList nameFilters = {"*icpc*", "*icc*", "*clang++*", "*clang*", "*gcc*", "*g++*"};

    FilePath::iterateDirectories(searchPaths, [&executables](const FilePath &path) {
        executables.append(path);
        return IterationPolicy::Continue;
    }, {nameFilters, QDir::Files | QDir::Executable });

    // Gcc is almost never what you want on macOS, but it is by default found in /usr/bin
    if (HostOsInfo::isMacHost() && detector.device->type() == Constants::DESKTOP_DEVICE_TYPE) {
         executables.removeOne(FilePath::fromPathPart(u"/usr/bin/gcc"));
         executables.removeOne(FilePath::fromPathPart(u"/usr/bin/g++"));
    }

    const OsType os = detector.device->osType();

    Toolchains result;

    // Linux ICC

    result += autoDetectToolchains(findCompilerCandidates(os, executables, "icpc", false),
                                   Constants::CXX_LANGUAGE_ID,
                                   Constants::LINUXICC_TOOLCHAIN_TYPEID,
                                   detector.alreadyKnown,
                                   GccToolChain::LinuxIcc);


    result += autoDetectToolchains(findCompilerCandidates(os, executables, "icc", true),
                                   Constants::C_LANGUAGE_ID,
                                   Constants::LINUXICC_TOOLCHAIN_TYPEID,
                                   detector.alreadyKnown,
                                   GccToolChain::LinuxIcc);

    // Clang

    Toolchains tcs;
    Toolchains known = detector.alreadyKnown;

    tcs.append(autoDetectToolchains(findCompilerCandidates(os, executables, "clang++", true),
                                    Constants::CXX_LANGUAGE_ID,
                                    Constants::CLANG_TOOLCHAIN_TYPEID,
                                    detector.alreadyKnown,
                                    GccToolChain::Clang));
    tcs.append(autoDetectToolchains(findCompilerCandidates(os, executables, "clang", true),
                                    Constants::C_LANGUAGE_ID,
                                    Constants::CLANG_TOOLCHAIN_TYPEID,
                                    detector.alreadyKnown,
                                    GccToolChain::Clang));
    known.append(tcs);

    tcs.append(autoDetectSdkClangToolchain(known));

    result += tcs;

    // Real GCC or MingGW

    result += autoDetectToolchains(findCompilerCandidates(os, executables, "g++", true),
                                   Constants::CXX_LANGUAGE_ID,
                                   Constants::GCC_TOOLCHAIN_TYPEID,
                                   detector.alreadyKnown,
                                   GccToolChain::RealGcc /*sic!*/);
    result += autoDetectToolchains(findCompilerCandidates(os, executables, "gcc", true),
                                   Constants::C_LANGUAGE_ID,
                                   Constants::GCC_TOOLCHAIN_TYPEID,
                                   detector.alreadyKnown,
                                   GccToolChain::RealGcc /*sic!*/);

    return result;
}

Toolchains GccToolChainFactory::detectForImport(const ToolChainDescription &tcd) const
{
    Toolchains result;

    // Do all autodetection in th 'RealGcc' case, and none in the others.
    if (!m_autoDetecting)
        return result;

    const QString fileName = tcd.compilerPath.completeBaseName();
    const QString resolvedSymlinksFileName = tcd.compilerPath.resolveSymlinks().completeBaseName();

    // Linux ICC

    if ((tcd.language == Constants::CXX_LANGUAGE_ID && fileName.startsWith("icpc")) ||
        (tcd.language == Constants::C_LANGUAGE_ID && fileName.startsWith("icc"))) {
        result += autoDetectToolChain(tcd, GccToolChain::LinuxIcc);
    }

    bool isCCompiler = tcd.language == Constants::C_LANGUAGE_ID
                             && ((fileName.startsWith("clang") && !fileName.startsWith("clang++"))
                                 || (fileName == "cc" && resolvedSymlinksFileName.contains("clang")));

    bool isCxxCompiler = tcd.language == Constants::CXX_LANGUAGE_ID
                               && (fileName.startsWith("clang++")
                                   || (fileName == "c++" && resolvedSymlinksFileName.contains("clang")));

    if (isCCompiler || isCxxCompiler)
        result += autoDetectToolChain(tcd, GccToolChain::Clang);

    // GCC

    isCCompiler = tcd.language == Constants::C_LANGUAGE_ID
            && (fileName.startsWith("gcc")
                || fileName.endsWith("gcc")
                || (fileName == "cc" && !resolvedSymlinksFileName.contains("clang")));

    isCxxCompiler = tcd.language == Constants::CXX_LANGUAGE_ID
            && (fileName.startsWith("g++")
                || fileName.endsWith("g++")
                || (fileName == "c++" && !resolvedSymlinksFileName.contains("clang")));

    if (isCCompiler || isCxxCompiler)
        result += autoDetectToolChain(tcd, GccToolChain::RealGcc);

    return result;
}

Toolchains GccToolChainFactory::autoDetectSdkClangToolchain(const Toolchains &known)
{
    const FilePath compilerPath = Core::ICore::clangExecutable(CLANG_BINDIR);
    if (compilerPath.isEmpty())
        return {};

    for (ToolChain * const existingTc : known) {
        if (existingTc->compilerCommand() == compilerPath)
            return {existingTc};
    }

    return {autoDetectToolChain({compilerPath, Constants::C_LANGUAGE_ID}, GccToolChain::Clang)};
}

Toolchains GccToolChainFactory::autoDetectToolchains(const FilePaths &compilerPaths,
                                                     const Id language,
                                                     const Id requiredTypeId,
                                                     const Toolchains &known,
                                                     const GccToolChain::SubType subType)
{
    Toolchains existingCandidates = filtered(known,
            [language](const ToolChain *tc) { return tc->language() == language; });

    Toolchains result;
    for (const FilePath &compilerPath : std::as_const(compilerPaths)) {
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
                 && ((language == Constants::CXX_LANGUAGE_ID && !existingCommand.fileName().contains("clang++"))
                     || (language == Constants::C_LANGUAGE_ID && !existingCommand.baseName().endsWith("clang"))))
                    || compilerPath.toString().contains("icecc")
                    || compilerPath.toString().contains("ccache")) {
                existingTcMatches = existingCommand == compilerPath;
            } else {
                existingTcMatches = existingCommand.isSameExecutable(compilerPath);
                if (!existingTcMatches
                        && HostOsInfo::isWindowsHost()
                        && !existingCommand.needsDevice()
                        && !compilerPath.needsDevice()) {
                    existingTcMatches = existingCommand.fileSize() == compilerPath.fileSize();
                }
            }
            if (existingTcMatches) {
                if (existingTc->typeId() == requiredTypeId && !result.contains(existingTc))
                    result << existingTc;
                alreadyExists = true;
            }
        }
        if (!alreadyExists) {
            const Toolchains newToolchains = autoDetectToolChain({compilerPath, language}, subType);
            result << newToolchains;
            existingCandidates << newToolchains;
        }
    }

    return result;
}

Toolchains GccToolChainFactory::autoDetectToolChain(const ToolChainDescription &tcd,
                                                    GccToolChain::SubType subType)
{
    Toolchains result;

    Environment systemEnvironment = tcd.compilerPath.deviceEnvironment();
    GccToolChain::addCommandPathToEnvironment(tcd.compilerPath, systemEnvironment);
    const FilePath localCompilerPath = findLocalCompiler(tcd.compilerPath, systemEnvironment);
    if (ToolChainManager::isBadToolchain(localCompilerPath))
        return result;
    Macros macros
            = gccPredefinedMacros(localCompilerPath, gccPredefinedMacrosOptions(tcd.language),
                                  systemEnvironment);
    if (macros.isEmpty()) {
        ToolChainManager::addBadToolchain(localCompilerPath);
        return result;
    }
    const GccToolChain::DetectedAbisResult detectedAbis = guessGccAbi(localCompilerPath,
                                                                      systemEnvironment,
                                                                      macros);
    for (const Abi &abi : detectedAbis.supportedAbis) {
        GccToolChain::SubType detectedSubType = subType;
        if (detectedSubType == GccToolChain::RealGcc && abi.osFlavor() == Abi::WindowsMSysFlavor)
            detectedSubType = GccToolChain::MinGW;

        auto tc = new GccToolChain({}, detectedSubType);

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
        tc->setDisplayName(tc->defaultDisplayName()); // reset displayname
        // lower priority of g++/gcc on macOS - usually just a frontend to clang
        if (detectedSubType == GccToolChain::RealGcc && abi.binaryFormat() == Abi::MachOFormat)
            tc->setPriority(ToolChain::PriorityLow);
        result.append(tc);
    }
    return result;
}

// --------------------------------------------------------------------------
// GccToolChainConfigWidget
// --------------------------------------------------------------------------

namespace Internal {
class TargetTripleWidget : public QWidget
{
    Q_OBJECT

public:
    TargetTripleWidget(const ToolChain *toolchain)
    {
        const auto layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        m_tripleLineEdit.setEnabled(false);
        m_overrideCheckBox.setText(Tr::tr("Override for code model"));
        m_overrideCheckBox.setToolTip(Tr::tr("Enable in the rare case that the code model\n"
                "fails because Clang does not understand the target architecture."));
        layout->addWidget(&m_tripleLineEdit, 1);
        layout->addWidget(&m_overrideCheckBox);
        layout->addStretch(1);

        connect(&m_tripleLineEdit, &QLineEdit::textEdited, this, &TargetTripleWidget::valueChanged);
        connect(&m_overrideCheckBox, &QCheckBox::toggled,
                &m_tripleLineEdit, &QLineEdit::setEnabled);

        m_tripleLineEdit.setText(toolchain->effectiveCodeModelTargetTriple());
        m_overrideCheckBox.setChecked(!toolchain->explicitCodeModelTargetTriple().isEmpty());
    }

    QString explicitCodeModelTargetTriple() const
    {
        if (m_overrideCheckBox.isChecked())
            return m_tripleLineEdit.text();
        return {};
    }

signals:
    void valueChanged();

private:
    QLineEdit m_tripleLineEdit;
    QCheckBox m_overrideCheckBox;
};
}

GccToolChainConfigWidget::GccToolChainConfigWidget(GccToolChain *tc) :
    ToolChainConfigWidget(tc),
    m_abiWidget(new AbiWidget),
    m_subType(tc->m_subType),
    m_compilerCommand(new PathChooser),
    m_targetTripleWidget(new TargetTripleWidget(tc))
{
    const QStringList gnuVersionArgs = QStringList("--version");
    m_compilerCommand->setExpectedKind(PathChooser::ExistingCommand);
    m_compilerCommand->setCommandVersionArguments(gnuVersionArgs);
    m_compilerCommand->setHistoryCompleter("PE.Gcc.Command.History");
    m_compilerCommand->setAllowPathFromDevice(true);
    m_mainLayout->addRow(Tr::tr("&Compiler path:"), m_compilerCommand);
    m_platformCodeGenFlagsLineEdit = new QLineEdit(this);
    m_platformCodeGenFlagsLineEdit->setText(ProcessArgs::joinArgs(tc->platformCodeGenFlags()));
    m_mainLayout->addRow(Tr::tr("Platform codegen flags:"), m_platformCodeGenFlagsLineEdit);
    m_platformLinkerFlagsLineEdit = new QLineEdit(this);
    m_platformLinkerFlagsLineEdit->setText(ProcessArgs::joinArgs(tc->platformLinkerFlags()));
    m_mainLayout->addRow(Tr::tr("Platform linker flags:"), m_platformLinkerFlagsLineEdit);
    m_mainLayout->addRow(Tr::tr("&ABI:"), m_abiWidget);
    m_mainLayout->addRow(Tr::tr("Target triple:"), m_targetTripleWidget);

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
    connect(m_targetTripleWidget, &TargetTripleWidget::valueChanged,
            this, &ToolChainConfigWidget::dirty);

    if (m_subType == GccToolChain::Clang) {
        if (!HostOsInfo::isWindowsHost() || tc->typeId() != Constants::CLANG_TOOLCHAIN_TYPEID)
            return;

        // Remove m_abiWidget row because the parent toolchain abi is going to be used.
        m_mainLayout->removeRow(m_mainLayout->rowCount() - 3); // FIXME: Do something sane instead.
        m_abiWidget = nullptr;

        m_parentToolchainCombo = new QComboBox(this);
        m_mainLayout->insertRow(m_mainLayout->rowCount() - 1,
                                Tr::tr("Parent toolchain:"),
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

        updateParentToolChainComboBox();
    }
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
    tc->setExplicitCodeModelTargetTriple(m_targetTripleWidget->explicitCodeModelTargetTriple());
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

    if (m_subType == GccToolChain::Clang && m_parentToolchainCombo) {

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
}

void GccToolChainConfigWidget::setFromToolchain()
{
    // subwidgets are not yet connected!
    QSignalBlocker blocker(this);
    auto tc = static_cast<GccToolChain *>(toolChain());
    m_compilerCommand->setFilePath(tc->compilerCommand());
    m_platformCodeGenFlagsLineEdit->setText(ProcessArgs::joinArgs(tc->platformCodeGenFlags(),
                                                                  HostOsInfo::hostOs()));
    m_platformLinkerFlagsLineEdit->setText(ProcessArgs::joinArgs(tc->platformLinkerFlags(),
                                                                 HostOsInfo::hostOs()));
    if (m_abiWidget) {
        m_abiWidget->setAbis(tc->supportedAbis(), tc->targetAbi());
        if (!m_isReadOnly && !m_compilerCommand->filePath().toString().isEmpty())
            m_abiWidget->setEnabled(true);
    }

    if (m_parentToolchainCombo)
        updateParentToolChainComboBox();
}

bool GccToolChainConfigWidget::isDirtyImpl() const
{
    auto tc = static_cast<GccToolChain *>(toolChain());

    if (m_compilerCommand->filePath() != tc->compilerCommand()
           || m_platformCodeGenFlagsLineEdit->text()
                  != ProcessArgs::joinArgs(tc->platformCodeGenFlags())
           || m_platformLinkerFlagsLineEdit->text()
                  != ProcessArgs::joinArgs(tc->platformLinkerFlags())
           || m_targetTripleWidget->explicitCodeModelTargetTriple()
                  != tc->explicitCodeModelTargetTriple()
           || (m_abiWidget && m_abiWidget->currentAbi() != tc->targetAbi())) {
        return true;
    }

    if (!m_parentToolchainCombo)
        return false;

    const GccToolChain *parentTC = mingwToolChainFromId(tc->m_parentToolChainId);
    const QByteArray parentId = parentTC ? parentTC->id() : QByteArray();
    return parentId != m_parentToolchainCombo->currentData();
}

void GccToolChainConfigWidget::makeReadOnlyImpl()
{
    m_compilerCommand->setReadOnly(true);
    if (m_abiWidget)
        m_abiWidget->setEnabled(false);
    m_platformCodeGenFlagsLineEdit->setEnabled(false);
    m_platformLinkerFlagsLineEdit->setEnabled(false);
    m_targetTripleWidget->setEnabled(false);
    m_isReadOnly = true;

    if (m_parentToolchainCombo)
        m_parentToolchainCombo->setEnabled(false);
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
        haveCompiler = path.isExecutableFile();
    }
    if (haveCompiler) {
        Environment env = path.deviceEnvironment();
        GccToolChain::addCommandPathToEnvironment(path, env);
        QStringList args = gccPredefinedMacrosOptions(Constants::CXX_LANGUAGE_ID)
                + splitString(m_platformCodeGenFlagsLineEdit->text());
        const FilePath localCompilerPath = findLocalCompiler(path, env);
        m_macros = gccPredefinedMacros(localCompilerPath, args, env);
        abiList = guessGccAbi(localCompilerPath, env, m_macros,
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
    QString str2 = ProcessArgs::joinArgs(splitString(str1));
    if (str1 != str2)
        m_platformCodeGenFlagsLineEdit->setText(str2);
    else
        handleCompilerCommandChange();
}

void GccToolChainConfigWidget::handlePlatformLinkerFlagsChange()
{
    QString str1 = m_platformLinkerFlagsLineEdit->text();
    QString str2 = ProcessArgs::joinArgs(splitString(str1));
    if (str1 != str2)
        m_platformLinkerFlagsLineEdit->setText(str2);
    else
        emit dirty();
}

// --------------------------------------------------------------------------
// ClangToolChain
// --------------------------------------------------------------------------

void GccToolChain::syncAutodetectedWithParentToolchains()
{
    if (!HostOsInfo::isWindowsHost() || typeId() != Constants::CLANG_TOOLCHAIN_TYPEID
        || !isAutoDetected()) {
        return;
    }

    QObject::disconnect(m_thisToolchainRemovedConnection);
    QObject::disconnect(m_mingwToolchainAddedConnection);

    if (!ToolChainManager::isLoaded()) {
        connect(ToolChainManager::instance(), &ToolChainManager::toolChainsLoaded, this,
                [id = id()] {
            if (ToolChain * const tc = ToolChainManager::findToolChain(id)) {
                if (tc->typeId() == Constants::CLANG_TOOLCHAIN_TYPEID)
                    static_cast<GccToolChain *>(tc)->syncAutodetectedWithParentToolchains();
            }
        });
        return;
    }

    if (!mingwToolChainFromId(m_parentToolChainId)) {
        const Toolchains mingwTCs = mingwToolChains();
        m_parentToolChainId = mingwTCs.isEmpty() ? QByteArray() : mingwTCs.front()->id();
    }

    // Subscribe only autodetected toolchains.
    ToolChainManager *tcManager = ToolChainManager::instance();
    m_mingwToolchainAddedConnection
        = connect(tcManager, &ToolChainManager::toolChainAdded, this, [this](ToolChain *tc) {
              if (tc->typeId() == Constants::MINGW_TOOLCHAIN_TYPEID
                  && !mingwToolChainFromId(m_parentToolChainId)) {
                  m_parentToolChainId = tc->id();
              }
          });
    m_thisToolchainRemovedConnection
        = connect(tcManager, &ToolChainManager::toolChainRemoved, this, [this](ToolChain *tc) {
              if (tc == this) {
                  QObject::disconnect(m_thisToolchainRemovedConnection);
                  QObject::disconnect(m_mingwToolchainAddedConnection);
              } else if (m_parentToolChainId == tc->id()) {
                  const Toolchains mingwTCs = mingwToolChains();
                  m_parentToolChainId = mingwTCs.isEmpty() ? QByteArray() : mingwTCs.front()->id();
              }
          });
}

bool GccToolChain::matchesCompilerCommand(const FilePath &command) const
{
    if (m_subType == Clang) {
        if (!m_resolvedCompilerCommand) {
            m_resolvedCompilerCommand = FilePath();
            if (HostOsInfo::isMacHost()
                && compilerCommand().parentDir() == FilePath::fromString("/usr/bin")) {
                Process xcrun;
                xcrun.setCommand({"/usr/bin/xcrun", {"-f", compilerCommand().fileName()}});
                xcrun.runBlocking();
                const FilePath output = FilePath::fromString(xcrun.cleanedStdOut().trimmed());
                if (output.isExecutableFile() && output != compilerCommand())
                    m_resolvedCompilerCommand = output;
            }
        }
        if (!m_resolvedCompilerCommand->isEmpty()
            && m_resolvedCompilerCommand->isSameExecutable(command))
            return true;
    }
    return ToolChain::matchesCompilerCommand(command);
}

QString GccToolChain::sysRoot() const
{
    if (m_subType == Clang) {
        if (const GccToolChain *parentTC = mingwToolChainFromId(m_parentToolChainId)) {
            const FilePath mingwCompiler = parentTC->compilerCommand();
            return mingwCompiler.parentDir().parentDir().toString();
        }
    }
    return {};
}

void GccToolChainConfigWidget::updateParentToolChainComboBox()
{
    QTC_ASSERT(m_parentToolchainCombo, return);

    auto *tc = static_cast<GccToolChain *>(toolChain());
    QByteArray parentId = m_parentToolchainCombo->currentData().toByteArray();
    if (tc->isAutoDetected() || m_parentToolchainCombo->count() == 0)
        parentId = tc->m_parentToolChainId;

    const GccToolChain *parentTC = mingwToolChainFromId(parentId);

    m_parentToolchainCombo->clear();
    m_parentToolchainCombo->addItem(parentTC ? parentTC->displayName() : QString(),
                                    parentTC ? parentId : QByteArray());

    if (tc->isAutoDetected())
        return;

    for (const ToolChain *mingwTC : mingwToolChains()) {
        if (mingwTC->id() == parentId)
            continue;
        if (mingwTC->language() != tc->language())
            continue;
        m_parentToolchainCombo->addItem(mingwTC->displayName(), mingwTC->id());
    }
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

#include <gcctoolchain.moc>
