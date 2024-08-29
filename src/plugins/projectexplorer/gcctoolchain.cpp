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
#include <utils/qtcprocess.h>
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

class WarningFlagAdder
{
public:
    WarningFlagAdder(const QString &flag, WarningFlags &flags) :
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

    void operator()(const char name[], WarningFlags flagsSet)
    {
        if (m_triggered)
            return;
        if (0 == strcmp(m_flagUtf8.data(), name)) {
            m_triggered = true;
            if (m_doesEnable)
                m_flags |= flagsSet;
            else
                m_flags &= ~flagsSet;
        }
    }

    bool triggered() const
    {
        return m_triggered;
    }

private:
    QByteArray m_flagUtf8;
    WarningFlags &m_flags;
    bool m_doesEnable = false;
    bool m_triggered = false;
};

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
class GccToolchainConfigWidget : public ToolchainConfigWidget
{
public:
    explicit GccToolchainConfigWidget(const ToolchainBundle &bundle);

private:
    void handleCompilerCommandChange(Id language);
    void handlePlatformCodeGenFlagsChange();
    void handlePlatformLinkerFlagsChange();

    void applyImpl() override;
    void discardImpl() override { setFromToolchain(); }
    bool isDirtyImpl() const override;
    void makeReadOnlyImpl() override;

    void setFromToolchain();

    void updateParentToolchainComboBox(); // Clang
    Id bundleIdFromId(const QByteArray &parentId);
    Toolchain *toolchainFromBundleId(Id bundleId, Id language);

    AbiWidget *m_abiWidget;

    GccToolchain::SubType m_subType = GccToolchain::RealGcc;

    QLineEdit *m_platformCodeGenFlagsLineEdit;
    QLineEdit *m_platformLinkerFlagsLineEdit;
    TargetTripleWidget * const m_targetTripleWidget;

    bool m_isReadOnly = false;
    ProjectExplorer::Macros m_macros;

    // Clang only
    QList<QMetaObject::Connection> m_parentToolchainConnections;
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
const char parentToolchainIdKeyC[] = "ProjectExplorer.ClangToolChain.ParentToolChainId";
const char priorityKeyC[] = "ProjectExplorer.ClangToolChain.Priority";
const char binaryRegexp[] = "(?:^|-|\\b)(?:gcc|g\\+\\+|clang(?:\\+\\+)?)(?:-([\\d.]+))?$";

static expected_str<QString> runGcc(
    const FilePath &gcc, const QStringList &arguments, const Environment &env)
{
    if (!gcc.isExecutableFile())
        return make_unexpected(QString("Compiler '%1' not found.").arg(gcc.toUserOutput()));

    Process cpp;
    Environment environment(env);
    environment.setupEnglishOutput();

    cpp.setEnvironment(environment);
    cpp.setCommand({gcc, arguments});
    cpp.runBlocking();
    if (cpp.result() != ProcessResult::FinishedWithSuccess || cpp.exitCode() != 0) {
        return make_unexpected(QString("Compiler feature detection failure.\n%1\n%2")
                                   .arg(cpp.exitMessage())
                                   .arg(cpp.allOutput()));
    }

    return cpp.allOutput().trimmed();
}

static expected_str<ProjectExplorer::Macros> gccPredefinedMacros(
    const FilePath &gcc, const QStringList &args, const Environment &env)
{
    QStringList arguments = args;
    arguments << "-";

    expected_str<QString> result = runGcc(gcc, arguments, env);
    if (!result)
        return make_unexpected(result.error());

    ProjectExplorer::Macros predefinedMacros = Macro::toMacros(result->toUtf8());
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

static HeaderPaths gccHeaderPaths(const FilePath &gcc,
                                  const QStringList &arguments,
                                  const Environment &env)
{
    expected_str<QString> result = runGcc(gcc, arguments, env);
    QTC_ASSERT_EXPECTED(result, return {});

    HeaderPaths builtInHeaderPaths;
    QByteArray line;
    QByteArray data = result->toUtf8();
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
                            && ToolchainManager::detectionSettings().detectX64AsX32)) {
            abiList << Abi(arch, os, flavor, format, 32);
        }
    } else {
        abiList << Abi(arch, os, flavor, format, width);
    }
    return abiList;
}


static GccToolchain::DetectedAbisResult guessGccAbi(const FilePath &path,
                                                    const Environment &env,
                                                    const Macros &macros,
                                                    const QStringList &extraArgs = {})
{
    if (path.isEmpty())
        return GccToolchain::DetectedAbisResult();

    QStringList arguments = extraArgs;
    arguments << "-dumpmachine";

    expected_str<QString> result = runGcc(path, arguments, env);
    QTC_ASSERT_EXPECTED(result, return {});

    QString machine = result->section('\n', 0, 0, QString::SectionSkipEmpty);
    if (machine.isEmpty()) {
        // ICC does not implement the -dumpmachine option on macOS.
        if (HostOsInfo::isMacHost() && (path.fileName() == "icc" || path.fileName() == "icpc"))
            return GccToolchain::DetectedAbisResult({Abi::hostAbi()});
        return GccToolchain::DetectedAbisResult(); // no need to continue if running failed once...
    }
    return GccToolchain::DetectedAbisResult(guessGccAbi(machine, macros), machine);
}

static QString gccVersion(const FilePath &path,
                          const Environment &env,
                          const QStringList &extraArgs)
{
    QStringList arguments = extraArgs;
    arguments << "-dumpversion";
    expected_str<QString> result = runGcc(path, arguments, env);
    QTC_ASSERT_EXPECTED(result, return {});
    return *result;
}

static FilePath gccInstallDir(const FilePath &compiler,
                              const Environment &env,
                              const QStringList &extraArgs = {})
{
    QStringList arguments = extraArgs;
    arguments << "-print-search-dirs";
    expected_str<QString> result = runGcc(compiler, arguments, env);
    QTC_ASSERT_EXPECTED(result, return {});

    // Expected output looks like this:
    //   install: /usr/lib/gcc/x86_64-linux-gnu/7/
    //   ...
    // Note that clang also supports "-print-search-dirs". However, the
    // install dir is not part of the output (tested with clang-8/clang-9).

    const QString prefix = "install: ";
    const QString line = QTextStream(&(*result)).readLine();
    if (!line.startsWith(prefix))
        return {};
    return compiler.withNewPath(QDir::cleanPath(line.mid(prefix.size())));
}

// --------------------------------------------------------------------------
// GccToolchain
// --------------------------------------------------------------------------

static Id idForSubType(GccToolchain::SubType subType)
{
    switch (subType) {
    case GccToolchain::RealGcc:
        return Constants::GCC_TOOLCHAIN_TYPEID;
    case GccToolchain::Clang:
        return Constants::CLANG_TOOLCHAIN_TYPEID;
    case GccToolchain::MinGW:
        return Constants::MINGW_TOOLCHAIN_TYPEID;
    case GccToolchain::LinuxIcc:
        return Constants::LINUXICC_TOOLCHAIN_TYPEID;
    }
    QTC_CHECK(false);
    return Constants::GCC_TOOLCHAIN_TYPEID;
}

GccToolchain::GccToolchain(Id typeId, SubType subType)
    : Toolchain(typeId.isValid() ? typeId : idForSubType(subType)), m_subType(subType)
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

GccToolchain::~GccToolchain()
{
    if (m_subType == Clang) {
        QObject::disconnect(m_thisToolchainRemovedConnection);
        QObject::disconnect(m_mingwToolchainAddedConnection);
    }
}

std::unique_ptr<ToolchainConfigWidget> GccToolchain::createConfigurationWidget(
    const ToolchainBundle &bundle)
{
    return std::make_unique<GccToolchainConfigWidget>(bundle);
}

void GccToolchain::setSupportedAbis(const Abis &abis)
{
    if (m_supportedAbis == abis)
        return;

    m_supportedAbis = abis;
    toolChainUpdated();
}

void GccToolchain::setOriginalTargetTriple(const QString &targetTriple)
{
    if (m_originalTargetTriple == targetTriple)
        return;

    m_originalTargetTriple = targetTriple;
    toolChainUpdated();
}

FilePath GccToolchain::correspondingCompilerCommand(
    const Utils::FilePath &srcPath,
    Utils::Id targetLang,
    const QString &cPattern,
    const QString &cxxPattern)
{
    QString outFileName = srcPath.fileName();
    if (targetLang == Constants::CXX_LANGUAGE_ID)
        outFileName.replace(cPattern, cxxPattern);
    else
        outFileName.replace(cxxPattern, cPattern);
    return srcPath.parentDir().pathAppended(outFileName);
}

void GccToolchain::setInstallDir(const FilePath &installDir)
{
    if (m_installDir == installDir)
        return;

    m_installDir = installDir;
    toolChainUpdated();
}

QString GccToolchain::defaultDisplayName() const
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
                                          ToolchainManager::displayNameOfLanguageId(language()),
                                          Abi::toString(abi.architecture()),
                                          Abi::toString(abi.wordWidth()),
                                          compilerCommand().toUserOutput());
}

LanguageExtensions GccToolchain::defaultLanguageExtensions() const
{
    return LanguageExtension::Gnu;
}

static const Toolchains mingwToolchains()
{
    return ToolchainManager::toolchains([](const Toolchain *tc) -> bool {
        return tc->typeId() == Constants::MINGW_TOOLCHAIN_TYPEID;
    });
}

static const GccToolchain *mingwToolchainFromId(const QByteArray &id)
{
    if (id.isEmpty())
        return nullptr;

    for (const Toolchain *tc : mingwToolchains()) {
        if (tc->id() == id)
            return static_cast<const GccToolchain *>(tc);
    }

    return nullptr;
}

QString GccToolchain::originalTargetTriple() const
{
    if (m_subType == Clang) {
        if (const GccToolchain *parentTC = mingwToolchainFromId(m_parentToolchainId))
            return parentTC->originalTargetTriple();
    }

    if (m_originalTargetTriple.isEmpty())
        m_originalTargetTriple = detectSupportedAbis().originalTargetTriple;
    return m_originalTargetTriple;
}

QString GccToolchain::version() const
{
    if (m_version.isEmpty())
        m_version = detectVersion();
    return m_version;
}

FilePath GccToolchain::installDir() const
{
    if (m_installDir.isEmpty())
        m_installDir = detectInstallDir();
    return m_installDir;
}

Abis GccToolchain::supportedAbis() const
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

Toolchain::MacroInspectionRunner GccToolchain::createMacroInspectionRunner() const
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

        const expected_str<Macros> macroResult
            = gccPredefinedMacros(findLocalCompiler(compilerCommand, env), arguments, env);
        QTC_CHECK_EXPECTED(macroResult);

        const auto macros = macroResult.value_or(Macros{});

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
LanguageExtensions GccToolchain::languageExtensions(const QStringList &cxxflags) const
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

WarningFlags GccToolchain::warningFlags(const QStringList &cflags) const
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

FilePaths GccToolchain::includedFiles(const QStringList &flags, const FilePath &directoryPath) const
{
    return Toolchain::includedFiles("-include", flags, directoryPath, PossiblyConcatenatedFlag::No);
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
void GccToolchain::initExtraHeaderPathsFunction(ExtraHeaderPathsFunction &&extraHeaderPathsFunction) const
{
    m_extraHeaderPathsFunction = std::move(extraHeaderPathsFunction);
}

static HeaderPaths builtInHeaderPaths(const Environment &env,
                                      const FilePath &compilerCommand,
                                      const QStringList &platformCodeGenFlags,
                                      std::function<QStringList(const QStringList &options)> reinterpretOptions,
                                      GccToolchain::HeaderPathsCache headerCache,
                                      Id languageId,
                                      std::function<void(HeaderPaths &)> extraHeaderPathsFunction,
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

Toolchain::BuiltInHeaderPathsRunner GccToolchain::createBuiltInHeaderPathsRunner(
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

void GccToolchain::addCommandPathToEnvironment(const FilePath &command, Environment &env)
{
    env.prependOrSetPath(command.parentDir());
}

void GccToolchain::addToEnvironment(Environment &env) const
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

QStringList GccToolchain::suggestedMkspecList() const
{
    if (m_subType == LinuxIcc)
        return {QString("linux-icc-%1").arg(targetAbi().wordWidth())};

    if (m_subType == MinGW) {
        if (HostOsInfo::isWindowsHost()) {
            if (compilerCommand().fileName().contains("clang"))
                return {"win32-clang-g++"};
            return {"win32-g++"};
        }
        if (HostOsInfo::isLinuxHost()) {
            if (version().startsWith("4.6."))
                return {"win32-g++-4.6-cross", "unsupported/win32-g++-4.6-cross"};
            return {"win32-g++-cross", "unsupported/win32-g++-cross"};
        }
        return {};
    }

    if (m_subType == Clang) {
        if (const Toolchain * const parentTc = ToolchainManager::findToolchain(m_parentToolchainId))
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

FilePath GccToolchain::makeCommand(const Environment &environment) const
{
    if (m_subType == Clang || m_subType == MinGW)
        return mingwAwareMakeCommand(environment);

    const FilePath tmp = environment.searchInPath("make");
    return tmp.isEmpty() ? "make" : tmp;
}

QList<OutputLineParser *> GccToolchain::createOutputParsers() const
{
    if (m_subType == LinuxIcc)
        return LinuxIccParser::iccParserSuite();

    if (m_subType == Clang)
        return ClangParser::clangParserSuite();

    return GccParser::gccParserSuite();
}

void GccToolchain::resetToolchain(const FilePath &path)
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

void GccToolchain::setPlatformCodeGenFlags(const QStringList &flags)
{
    if (flags != m_platformCodeGenFlags) {
        m_platformCodeGenFlags = flags;
        toolChainUpdated();
    }
}

QStringList GccToolchain::extraCodeModelFlags() const
{
    return platformCodeGenFlags();
}

/*!
    Code gen flags that have to be passed to the compiler.
 */
QStringList GccToolchain::platformCodeGenFlags() const
{
    return m_platformCodeGenFlags;
}

void GccToolchain::setPlatformLinkerFlags(const QStringList &flags)
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
QStringList GccToolchain::platformLinkerFlags() const
{
    return m_platformLinkerFlags;
}

void GccToolchain::toMap(Store &data) const
{
    Toolchain::toMap(data);
    data.insert(compilerPlatformCodeGenFlagsKeyC, m_platformCodeGenFlags);
    data.insert(compilerPlatformLinkerFlagsKeyC, m_platformLinkerFlags);
    data.insert(originalTargetTripleKeyC, m_originalTargetTriple);
    data.insert(supportedAbisKeyC, Utils::transform<QStringList>(m_supportedAbis, &Abi::toString));

    if (m_subType == Clang) {
        data.insert(parentToolchainIdKeyC, m_parentToolchainId);
        data.insert(priorityKeyC, m_priority);
    }
}

void GccToolchain::fromMap(const Store &data)
{
    Toolchain::fromMap(data);
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
        resetToolchain(compilerCommand());

    if (m_subType == Clang) {
        m_parentToolchainId = data.value(parentToolchainIdKeyC).toByteArray();
        m_priority = data.value(priorityKeyC, PriorityNormal).toInt();
        syncAutodetectedWithParentToolchains();
    }
}

bool GccToolchain::operator ==(const Toolchain &other) const
{
    if (!Toolchain::operator ==(other))
        return false;

    auto gccTc = static_cast<const GccToolchain *>(&other);
    return compilerCommand() == gccTc->compilerCommand() && targetAbi() == gccTc->targetAbi()
            && m_platformCodeGenFlags == gccTc->m_platformCodeGenFlags
            && m_platformLinkerFlags == gccTc->m_platformLinkerFlags;
}

void GccToolchain::updateSupportedAbis() const
{
    if (m_supportedAbis.isEmpty()) {
        const DetectedAbisResult detected = detectSupportedAbis();
        m_supportedAbis = detected.supportedAbis;
        m_originalTargetTriple = detected.originalTargetTriple;
    }
}

void GccToolchain::setOptionsReinterpreter(const OptionsReinterpreter &optionsReinterpreter)
{
    m_optionsReinterpreter = optionsReinterpreter;
}

GccToolchain::DetectedAbisResult GccToolchain::detectSupportedAbis() const
{
    Environment env = compilerCommand().deviceEnvironment();
    addToEnvironment(env);
    ProjectExplorer::Macros macros = createMacroInspectionRunner()({}).macros;
    return guessGccAbi(findLocalCompiler(compilerCommand(), env),
                       env,
                       macros,
                       platformCodeGenFlags());
}

QString GccToolchain::detectVersion() const
{
    Environment env = compilerCommand().deviceEnvironment();
    addToEnvironment(env);
    return gccVersion(findLocalCompiler(compilerCommand(), env), env,
                      filteredFlags(platformCodeGenFlags(), true));
}

FilePath GccToolchain::detectInstallDir() const
{
    Environment env = compilerCommand().deviceEnvironment();
    addToEnvironment(env);
    return gccInstallDir(findLocalCompiler(compilerCommand(), env), env,
                         filteredFlags(platformCodeGenFlags(), true));
}

// --------------------------------------------------------------------------
// GccToolchainFactory
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

static Toolchain *constructRealGccToolchain()
{
    return new GccToolchain(Constants::GCC_TOOLCHAIN_TYPEID, GccToolchain::RealGcc);
}

static Toolchain *constructClangToolchain()
{
    return new GccToolchain(Constants::CLANG_TOOLCHAIN_TYPEID, GccToolchain::Clang);
}

static Toolchain *constructMinGWToolchain()
{
    return new GccToolchain(Constants::MINGW_TOOLCHAIN_TYPEID, GccToolchain::MinGW);
}

static Toolchain *constructLinuxIccToolchain()
{
    return new GccToolchain(Constants::LINUXICC_TOOLCHAIN_TYPEID, GccToolchain::LinuxIcc);
}

namespace Internal {

class GccToolchainFactory final : public ToolchainFactory
{
public:
    explicit GccToolchainFactory(GccToolchain::SubType subType)
        : m_autoDetecting(subType == GccToolchain::RealGcc)
    {
        switch (subType) {
        case GccToolchain::RealGcc:
            setDisplayName(Tr::tr("GCC"));
            setSupportedToolchainType(Constants::GCC_TOOLCHAIN_TYPEID);
            setToolchainConstructor(&constructRealGccToolchain);
            break;
        case GccToolchain::Clang:
            setDisplayName(Tr::tr("Clang"));
            setSupportedToolchainType(Constants::CLANG_TOOLCHAIN_TYPEID);
            setToolchainConstructor(&constructClangToolchain);
            break;
        case GccToolchain::MinGW:
            setDisplayName(Tr::tr("MinGW"));
            setSupportedToolchainType(Constants::MINGW_TOOLCHAIN_TYPEID);
            setToolchainConstructor(&constructMinGWToolchain);
            break;
        case GccToolchain::LinuxIcc:
            setDisplayName(Tr::tr("ICC"));
            setSupportedToolchainType(Constants::LINUXICC_TOOLCHAIN_TYPEID);
            setToolchainConstructor(&constructLinuxIccToolchain);
            break;
        }
        setSupportedLanguages({Constants::C_LANGUAGE_ID, Constants::CXX_LANGUAGE_ID});
        setUserCreatable(true);
    }

    Toolchains autoDetect(const ToolchainDetector &detector) const final;
    Toolchains detectForImport(const ToolchainDescription &tcd) const final;
    std::unique_ptr<ToolchainConfigWidget> createConfigurationWidget(
        const ToolchainBundle &bundle) const final;
    FilePath correspondingCompilerCommand(const FilePath &srcPath, Id targetLang) const final;

private:
    static Toolchains autoDetectToolchains(const FilePaths &compilerPaths,
                                           const Id language,
                                           const Id requiredTypeId,
                                           const Toolchains &known,
                                           const GccToolchain::SubType subType);
    static Toolchains autoDetectToolchain(const ToolchainDescription &tcd,
                                          const GccToolchain::SubType subType);
    static Toolchains autoDetectSdkClangToolchain(const Toolchains &known);

    const bool m_autoDetecting;
};

void setupGccToolchains()
{
#ifndef Q_OS_WIN
    static GccToolchainFactory theLinuxIccToolchainFactory{GccToolchain::LinuxIcc};
#endif

#ifndef Q_OS_MACOS
    // Mingw offers cross-compiling to windows
    static GccToolchainFactory theMingwToolchainFactory{GccToolchain::MinGW};
#endif

    static GccToolchainFactory theGccToolchainFactory{GccToolchain::RealGcc};
    static GccToolchainFactory theClangToolchainFactory{GccToolchain::Clang};
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


Toolchains GccToolchainFactory::autoDetect(const ToolchainDetector &detector) const
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

    Utils::sort(executables);

    const OsType os = detector.device->osType();

    Toolchains result;

    // Linux ICC

    result += autoDetectToolchains(findCompilerCandidates(os, executables, "icpc", false),
                                   Constants::CXX_LANGUAGE_ID,
                                   Constants::LINUXICC_TOOLCHAIN_TYPEID,
                                   detector.alreadyKnown,
                                   GccToolchain::LinuxIcc);


    result += autoDetectToolchains(findCompilerCandidates(os, executables, "icc", true),
                                   Constants::C_LANGUAGE_ID,
                                   Constants::LINUXICC_TOOLCHAIN_TYPEID,
                                   detector.alreadyKnown,
                                   GccToolchain::LinuxIcc);

    // Clang

    Toolchains tcs;
    Toolchains known = detector.alreadyKnown;

    tcs.append(autoDetectToolchains(findCompilerCandidates(os, executables, "clang++", true),
                                    Constants::CXX_LANGUAGE_ID,
                                    Constants::CLANG_TOOLCHAIN_TYPEID,
                                    detector.alreadyKnown,
                                    GccToolchain::Clang));
    tcs.append(autoDetectToolchains(findCompilerCandidates(os, executables, "clang", true),
                                    Constants::C_LANGUAGE_ID,
                                    Constants::CLANG_TOOLCHAIN_TYPEID,
                                    detector.alreadyKnown,
                                    GccToolchain::Clang));
    known.append(tcs);

    tcs.append(autoDetectSdkClangToolchain(known));

    result += tcs;

    // Real GCC or MingGW

    result += autoDetectToolchains(findCompilerCandidates(os, executables, "g++", true),
                                   Constants::CXX_LANGUAGE_ID,
                                   Constants::GCC_TOOLCHAIN_TYPEID,
                                   detector.alreadyKnown,
                                   GccToolchain::RealGcc /*sic!*/);
    result += autoDetectToolchains(findCompilerCandidates(os, executables, "gcc", true),
                                   Constants::C_LANGUAGE_ID,
                                   Constants::GCC_TOOLCHAIN_TYPEID,
                                   detector.alreadyKnown,
                                   GccToolchain::RealGcc /*sic!*/);

    return result;
}

Toolchains GccToolchainFactory::detectForImport(const ToolchainDescription &tcd) const
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
        result += autoDetectToolchain(tcd, GccToolchain::LinuxIcc);
    }

    bool isCCompiler = tcd.language == Constants::C_LANGUAGE_ID
                             && ((fileName.startsWith("clang") && !fileName.startsWith("clang++"))
                                 || (fileName == "cc" && resolvedSymlinksFileName.contains("clang")));

    bool isCxxCompiler = tcd.language == Constants::CXX_LANGUAGE_ID
                               && (fileName.startsWith("clang++")
                                   || (fileName == "c++" && resolvedSymlinksFileName.contains("clang")));

    if (isCCompiler || isCxxCompiler)
        result += autoDetectToolchain(tcd, GccToolchain::Clang);

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
        result += autoDetectToolchain(tcd, GccToolchain::RealGcc);

    return result;
}

std::unique_ptr<ToolchainConfigWidget> GccToolchainFactory::createConfigurationWidget(
    const ToolchainBundle &bundle) const
{
    return GccToolchain::createConfigurationWidget(bundle);
}

FilePath GccToolchainFactory::correspondingCompilerCommand(
    const FilePath &srcPath, Id targetLang) const
{
    if (supportedToolchainType() == Constants::GCC_TOOLCHAIN_TYPEID
        || supportedToolchainType() == Constants::MINGW_TOOLCHAIN_TYPEID) {
        return GccToolchain::correspondingCompilerCommand(srcPath, targetLang, "gcc", "g++");
    }
    if (supportedToolchainType() == Constants::CLANG_TOOLCHAIN_TYPEID)
        return GccToolchain::correspondingCompilerCommand(srcPath, targetLang, "clang", "clang++");
    if (supportedToolchainType() == Constants::LINUXICC_TOOLCHAIN_TYPEID)
        return GccToolchain::correspondingCompilerCommand(srcPath, targetLang, "icc", "icpc");
    return {};
}

Toolchains GccToolchainFactory::autoDetectSdkClangToolchain(const Toolchains &known)
{
    const expected_str<FilePath> compilerPath = Core::ICore::clangExecutable(CLANG_BINDIR);
    QTC_CHECK_EXPECTED(compilerPath);
    if (!compilerPath)
        return {};

    for (Toolchain * const existingTc : known) {
        if (existingTc->compilerCommand() == *compilerPath)
            return {existingTc};
    }

    return {autoDetectToolchain({*compilerPath, Constants::C_LANGUAGE_ID}, GccToolchain::Clang)};
}

Toolchains GccToolchainFactory::autoDetectToolchains(const FilePaths &compilerPaths,
                                                     const Id language,
                                                     const Id requiredTypeId,
                                                     const Toolchains &known,
                                                     const GccToolchain::SubType subType)
{
    Toolchains existingCandidates = filtered(known,
            [language](const Toolchain *tc) { return tc->language() == language; });

    Toolchains result;
    for (const FilePath &compilerPath : std::as_const(compilerPaths)) {
        bool alreadyExists = false;
        for (Toolchain * const existingTc : existingCandidates) {
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
            const Toolchains newToolchains = autoDetectToolchain({compilerPath, language}, subType);
            result << newToolchains;
            existingCandidates << newToolchains;
        }
    }

    return result;
}

Toolchains GccToolchainFactory::autoDetectToolchain(const ToolchainDescription &tcd,
                                                    GccToolchain::SubType subType)
{
    Toolchains result;

    Environment systemEnvironment = tcd.compilerPath.deviceEnvironment();
    GccToolchain::addCommandPathToEnvironment(tcd.compilerPath, systemEnvironment);
    const FilePath localCompilerPath = findLocalCompiler(tcd.compilerPath, systemEnvironment);
    if (ToolchainManager::isBadToolchain(localCompilerPath))
        return result;
    expected_str<Macros> macros = gccPredefinedMacros(
        localCompilerPath, gccPredefinedMacrosOptions(tcd.language), systemEnvironment);
    if (!macros) {
        Core::MessageManager::writeFlashing(
            {QString("Compiler %1 is not a valid compiler:").arg(localCompilerPath.toUserOutput()),
             macros.error()});
        ToolchainManager::addBadToolchain(localCompilerPath);
        return result;
    }
    const GccToolchain::DetectedAbisResult detectedAbis
        = guessGccAbi(localCompilerPath, systemEnvironment, *macros);
    for (const Abi &abi : detectedAbis.supportedAbis) {
        GccToolchain::SubType detectedSubType = subType;
        if (detectedSubType == GccToolchain::RealGcc && abi.osFlavor() == Abi::WindowsMSysFlavor)
            detectedSubType = GccToolchain::MinGW;

        auto tc = new GccToolchain({}, detectedSubType);

        tc->setLanguage(tcd.language);
        tc->setDetection(Toolchain::AutoDetection);
        tc->predefinedMacrosCache()
            ->insert(QStringList(),
                     Toolchain::MacroInspectionReport{*macros,
                                                      Toolchain::languageVersion(tcd.language, *macros)});
        tc->setCompilerCommand(tcd.compilerPath);
        tc->setSupportedAbis(detectedAbis.supportedAbis);
        tc->setTargetAbi(abi);
        tc->setOriginalTargetTriple(detectedAbis.originalTargetTriple);
        tc->setDisplayName(tc->defaultDisplayName()); // reset displayname

        // lower priority of g++/gcc on macOS - usually just a frontend to clang
        if (detectedSubType == GccToolchain::RealGcc && abi.binaryFormat() == Abi::MachOFormat)
            tc->setPriority(Toolchain::PriorityLow);

        // GCC is still "more native" than clang on Linux.
        if (detectedSubType == GccToolchain::Clang && abi.binaryFormat() == Abi::ElfFormat
            && abi.os() == Abi::LinuxOS) {
            tc->setPriority(Toolchain::PriorityLow);
        }

        result.append(tc);
    }
    return result;
}

// --------------------------------------------------------------------------
// GccToolchainConfigWidget
// --------------------------------------------------------------------------

class TargetTripleWidget : public QWidget
{
    Q_OBJECT

public:
    TargetTripleWidget(const ToolchainBundle &bundle)
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

        m_tripleLineEdit.setText(bundle.get(&Toolchain::effectiveCodeModelTargetTriple));
        m_overrideCheckBox.setChecked(!bundle.get(&Toolchain::explicitCodeModelTargetTriple).isEmpty());
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

GccToolchainConfigWidget::GccToolchainConfigWidget(const ToolchainBundle &bundle) :
    ToolchainConfigWidget(bundle),
    m_abiWidget(new AbiWidget),
    m_subType(bundle.get(&GccToolchain::subType)),
    m_targetTripleWidget(new TargetTripleWidget(bundle))
{
    setCommandVersionArguments({"--version"});
    m_platformCodeGenFlagsLineEdit = new QLineEdit(this);
    m_platformCodeGenFlagsLineEdit->setText(ProcessArgs::joinArgs(bundle.extraCodeModelFlags()));
    m_mainLayout->addRow(Tr::tr("Platform codegen flags:"), m_platformCodeGenFlagsLineEdit);
    m_platformLinkerFlagsLineEdit = new QLineEdit(this);
    m_platformLinkerFlagsLineEdit->setText(ProcessArgs::joinArgs(bundle.get(&GccToolchain::platformLinkerFlags)));
    m_mainLayout->addRow(Tr::tr("Platform linker flags:"), m_platformLinkerFlagsLineEdit);
    m_mainLayout->addRow(Tr::tr("&ABI:"), m_abiWidget);
    m_mainLayout->addRow(Tr::tr("Target triple:"), m_targetTripleWidget);

    m_abiWidget->setEnabled(false);
    addErrorLabel();

    setFromToolchain();

    connect(this, &ToolchainConfigWidget::compilerCommandChanged,
            this, &GccToolchainConfigWidget::handleCompilerCommandChange);
    connect(m_platformCodeGenFlagsLineEdit, &QLineEdit::editingFinished,
            this, &GccToolchainConfigWidget::handlePlatformCodeGenFlagsChange);
    connect(m_platformLinkerFlagsLineEdit, &QLineEdit::editingFinished,
            this, &GccToolchainConfigWidget::handlePlatformLinkerFlagsChange);
    connect(m_abiWidget, &AbiWidget::abiChanged, this, &ToolchainConfigWidget::dirty);
    connect(m_targetTripleWidget, &TargetTripleWidget::valueChanged,
            this, &ToolchainConfigWidget::dirty);

    if (m_subType == GccToolchain::Clang) {
        if (!HostOsInfo::isWindowsHost() || bundle.type() != Constants::CLANG_TOOLCHAIN_TYPEID)
            return;

        // Remove m_abiWidget row because the parent toolchain abi is going to be used.
        m_mainLayout->removeRow(m_mainLayout->rowCount() - 3); // FIXME: Do something sane instead.
        m_abiWidget = nullptr;

        m_parentToolchainCombo = new QComboBox(this);
        connect(m_parentToolchainCombo, &QComboBox::currentIndexChanged,
                this, &ToolchainConfigWidget::dirty);
        m_mainLayout->insertRow(m_mainLayout->rowCount() - 1,
                                Tr::tr("Parent toolchain:"),
                                m_parentToolchainCombo);

        ToolchainManager *tcManager = ToolchainManager::instance();
        m_parentToolchainConnections.append(
            connect(tcManager, &ToolchainManager::toolchainUpdated, this, [this](Toolchain *tc) {
                if (tc->typeId() == Constants::MINGW_TOOLCHAIN_TYPEID)
                    updateParentToolchainComboBox();
            }));
        m_parentToolchainConnections.append(
            connect(tcManager, &ToolchainManager::toolchainsRegistered,
                    this, [this](const Toolchains &toolchains) {
                if (Utils::contains(
                        toolchains,
                        Utils::equal(&Toolchain::typeId, Id(Constants::MINGW_TOOLCHAIN_TYPEID)))) {
                    updateParentToolchainComboBox();
                }
            }));
        m_parentToolchainConnections.append(connect(
            tcManager, &ToolchainManager::toolchainsDeregistered, this, [this, bundle](const Toolchains &toolchains) {
                bool updateParentComboBox = false;
                for (Toolchain * const tc : toolchains) {
                    if (Utils::contains(bundle.toolchains(), [tc](const Toolchain *elem) {
                            return elem->id() == tc->id();
                        })) {
                        for (QMetaObject::Connection &connection : m_parentToolchainConnections)
                            QObject::disconnect(connection);
                        return;
                    }
                    if (tc->typeId() == Constants::MINGW_TOOLCHAIN_TYPEID)
                        updateParentComboBox = true;
                }
                if (updateParentComboBox)
                    updateParentToolchainComboBox();
            }));
        updateParentToolchainComboBox();
    }
}

void GccToolchainConfigWidget::applyImpl()
{
    if (bundle().isAutoDetected())
        return;

    const Id parentBundleId = m_parentToolchainCombo
        ? Id::fromSetting(m_parentToolchainCombo->currentData())
        : Id();
    bundle().forEach<GccToolchain>([&](GccToolchain &tc) {
        tc.setCompilerCommand(compilerCommand(tc.language()));
        if (m_abiWidget) {
            tc.setSupportedAbis(m_abiWidget->supportedAbis());
            tc.setTargetAbi(m_abiWidget->currentAbi());
        }
        tc.setInstallDir(tc.detectInstallDir());
        tc.setOriginalTargetTriple(tc.detectSupportedAbis().originalTargetTriple);
        tc.setExplicitCodeModelTargetTriple(m_targetTripleWidget->explicitCodeModelTargetTriple());
        tc.setPlatformCodeGenFlags(splitString(m_platformCodeGenFlagsLineEdit->text()));
        tc.setPlatformLinkerFlags(splitString(m_platformLinkerFlagsLineEdit->text()));

        tc.m_parentToolchainId.clear();
        if (parentBundleId.isValid()) {
            if (const Toolchain * const parentTc
                = toolchainFromBundleId(parentBundleId, tc.language())) {
                tc.m_parentToolchainId = parentTc->id();
                tc.setTargetAbi(parentTc->targetAbi());
                tc.setSupportedAbis(parentTc->supportedAbis());
            }
        }

        if (!m_macros.isEmpty()) {
            tc.predefinedMacrosCache()->insert(
                tc.platformCodeGenFlags(),
                Toolchain::MacroInspectionReport{m_macros, Toolchain::languageVersion(
                                                               tc.language(), m_macros)});
        }
    });
}

void GccToolchainConfigWidget::setFromToolchain()
{
    // subwidgets are not yet connected!
    QSignalBlocker blocker(this);
    m_platformCodeGenFlagsLineEdit->setText(ProcessArgs::joinArgs(
        bundle().get(&GccToolchain::platformCodeGenFlags), HostOsInfo::hostOs()));
    m_platformLinkerFlagsLineEdit->setText(
        ProcessArgs::joinArgs(bundle().get(&GccToolchain::platformLinkerFlags), HostOsInfo::hostOs()));
    if (m_abiWidget) {
        m_abiWidget->setAbis(bundle().supportedAbis(), bundle().targetAbi());
        if (!m_isReadOnly && hasAnyCompiler())
            m_abiWidget->setEnabled(true);
    }

    if (m_parentToolchainCombo)
        updateParentToolchainComboBox();
}

bool GccToolchainConfigWidget::isDirtyImpl() const
{
    if (m_platformCodeGenFlagsLineEdit->text() != ProcessArgs::joinArgs(bundle().get(&GccToolchain::platformCodeGenFlags))
        || m_platformLinkerFlagsLineEdit->text() != ProcessArgs::joinArgs(bundle().get(&GccToolchain::platformLinkerFlags))
        || m_targetTripleWidget->explicitCodeModelTargetTriple()
               != bundle().get(&GccToolchain::explicitCodeModelTargetTriple)
        || (m_abiWidget && m_abiWidget->currentAbi() != bundle().targetAbi())) {
        return true;
    }

    if (!m_parentToolchainCombo)
        return false;

    const GccToolchain *parentTC = mingwToolchainFromId(
        bundle().get(&GccToolchain::parentToolchainId));
    const Id parentBundleId = parentTC ? parentTC->bundleId() : Id();
    return parentBundleId.toSetting() != m_parentToolchainCombo->currentData();
}

void GccToolchainConfigWidget::makeReadOnlyImpl()
{
    if (m_abiWidget)
        m_abiWidget->setEnabled(false);
    m_platformCodeGenFlagsLineEdit->setEnabled(false);
    m_platformLinkerFlagsLineEdit->setEnabled(false);
    m_targetTripleWidget->setEnabled(false);
    m_isReadOnly = true;

    if (m_parentToolchainCombo)
        m_parentToolchainCombo->setEnabled(false);
}

void GccToolchainConfigWidget::handleCompilerCommandChange(Id language)
{
    if (!m_abiWidget)
        return;

    bool haveCompiler = false;
    Abi currentAbi = m_abiWidget->currentAbi();
    bool customAbi = m_abiWidget->isCustomAbi() && m_abiWidget->isEnabled();
    FilePath path = compilerCommand(language);
    Abis abiList;

    if (!path.isEmpty()) {
        haveCompiler = path.isExecutableFile();
    }
    if (haveCompiler) {
        Environment env = path.deviceEnvironment();
        GccToolchain::addCommandPathToEnvironment(path, env);
        QStringList args = gccPredefinedMacrosOptions(Constants::CXX_LANGUAGE_ID)
                + splitString(m_platformCodeGenFlagsLineEdit->text());
        const FilePath localCompilerPath = findLocalCompiler(path, env);
        expected_str<ProjectExplorer::Macros> macros
            = gccPredefinedMacros(localCompilerPath, args, env);
        QTC_CHECK_EXPECTED(macros);
        m_macros = macros.value_or(ProjectExplorer::Macros{});
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

void GccToolchainConfigWidget::handlePlatformCodeGenFlagsChange()
{
    QString str1 = m_platformCodeGenFlagsLineEdit->text();
    QString str2 = ProcessArgs::joinArgs(splitString(str1));
    if (str1 != str2)
        m_platformCodeGenFlagsLineEdit->setText(str2);
    else
        handleCompilerCommandChange(Constants::C_LANGUAGE_ID);
}

void GccToolchainConfigWidget::handlePlatformLinkerFlagsChange()
{
    QString str1 = m_platformLinkerFlagsLineEdit->text();
    QString str2 = ProcessArgs::joinArgs(splitString(str1));
    if (str1 != str2)
        m_platformLinkerFlagsLineEdit->setText(str2);
    else
        emit dirty();
}

// --------------------------------------------------------------------------
// ClangToolchain
// --------------------------------------------------------------------------

void GccToolchain::syncAutodetectedWithParentToolchains()
{
    if (!HostOsInfo::isWindowsHost() || typeId() != Constants::CLANG_TOOLCHAIN_TYPEID
        || !isAutoDetected()) {
        return;
    }

    QObject::disconnect(m_thisToolchainRemovedConnection);
    QObject::disconnect(m_mingwToolchainAddedConnection);

    if (!ToolchainManager::isLoaded()) {
        connect(ToolchainManager::instance(), &ToolchainManager::toolchainsLoaded, this,
                [id = id()] {
            if (Toolchain * const tc = ToolchainManager::findToolchain(id)) {
                if (tc->typeId() == Constants::CLANG_TOOLCHAIN_TYPEID)
                    static_cast<GccToolchain *>(tc)->syncAutodetectedWithParentToolchains();
            }
        });
        return;
    }

    if (!mingwToolchainFromId(m_parentToolchainId)) {
        const Toolchains mingwTCs = mingwToolchains();
        m_parentToolchainId = mingwTCs.isEmpty() ? QByteArray() : mingwTCs.front()->id();
    }

    // Subscribe only autodetected toolchains.
    ToolchainManager *tcManager = ToolchainManager::instance();
    m_mingwToolchainAddedConnection
        = connect(tcManager, &ToolchainManager::toolchainsRegistered, this,
                  [this](const Toolchains &toolchains) {
              if (mingwToolchainFromId(m_parentToolchainId))
                  return;
              for (Toolchain * const tc : toolchains) {
                  if (tc->typeId() == Constants::MINGW_TOOLCHAIN_TYPEID) {
                      m_parentToolchainId = tc->id();
                      break;
                  }
              }
          });
    m_thisToolchainRemovedConnection
        = connect(tcManager, &ToolchainManager::toolchainsDeregistered, this,
                  [this](const Toolchains &toolchains) {
              bool updateParentId = false;
              for (Toolchain * const tc : toolchains) {
                  if (tc == this) {
                      QObject::disconnect(m_thisToolchainRemovedConnection);
                      QObject::disconnect(m_mingwToolchainAddedConnection);
                      break;
                  } else if (m_parentToolchainId == tc->id()) {
                      updateParentId = true;
                      break;
                  }
              }
              if (updateParentId) {
                  const Toolchains mingwTCs = mingwToolchains();
                  m_parentToolchainId = mingwTCs.isEmpty() ? QByteArray()
                                                           : mingwTCs.front()->id();
              }
          });
}

bool GccToolchain::matchesCompilerCommand(const FilePath &command) const
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
    return Toolchain::matchesCompilerCommand(command);
}

QString GccToolchain::sysRoot() const
{
    if (m_subType == Clang) {
        if (const GccToolchain *parentTC = mingwToolchainFromId(m_parentToolchainId)) {
            const FilePath mingwCompiler = parentTC->compilerCommand();
            return mingwCompiler.parentDir().parentDir().toString();
        }
    }
    return {};
}

bool GccToolchain::canShareBundleImpl(const Toolchain &other) const
{
    return platformLinkerFlags() == static_cast<const GccToolchain &>(other).platformLinkerFlags();
}

void GccToolchainConfigWidget::updateParentToolchainComboBox()
{
    QTC_ASSERT(m_parentToolchainCombo, return);

    Id parentBundleId = Id::fromSetting(m_parentToolchainCombo->currentData());
    if (bundle().isAutoDetected() || m_parentToolchainCombo->count() == 0)
        parentBundleId = bundleIdFromId(bundle().get(&GccToolchain::parentToolchainId));
    const QList<ToolchainBundle> mingwBundles = Utils::filtered(
        ToolchainBundle::collectBundles(ToolchainBundle::AutoRegister::NotApplicable),
        [](const ToolchainBundle &b) { return b.type() == Constants::MINGW_TOOLCHAIN_TYPEID; });
    const auto parentBundle
        = Utils::findOr(mingwBundles, std::nullopt, [parentBundleId](const ToolchainBundle &b) {
              return b.bundleId() == parentBundleId;
          });

    m_parentToolchainCombo->clear();
    m_parentToolchainCombo->addItem(parentBundle ? parentBundle->displayName() : QString(),
                                    parentBundle ? parentBundleId.toSetting() : QVariant());

    if (bundle().isAutoDetected())
        return;

    for (const ToolchainBundle &mingwBundle : mingwBundles) {
        if (mingwBundle.bundleId() != parentBundleId) {
            m_parentToolchainCombo
                ->addItem(mingwBundle.displayName(), mingwBundle.bundleId().toSetting());
        }
    }
}

Id GccToolchainConfigWidget::bundleIdFromId(const QByteArray &id)
{
    const Toolchain * const tc = ToolchainManager::toolchain(
        [id](const Toolchain *tc) { return tc->id() == id; });
    return tc ? tc->bundleId() : Id();
}

Toolchain *GccToolchainConfigWidget::toolchainFromBundleId(Id bundleId, Id language)
{
    return ToolchainManager::toolchain(
        [bundleId, language](const Toolchain *tc) {
            return tc->bundleId() == bundleId && tc->language() == language;
        });
}

} // namespace ProjectExplorer

// Unit tests:

#ifdef WITH_TESTS
#   include "projectexplorer_test.h"

#   include <QTest>
#   include <QUrl>

namespace ProjectExplorer {
void ProjectExplorerTest::testGccAbiGuessing_data()
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

void ProjectExplorerTest::testGccAbiGuessing()
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
