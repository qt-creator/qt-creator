/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "gcctoolchain.h"
#include "clangparser.h"
#include "gcctoolchainfactories.h"
#include "gccparser.h"
#include "linuxiccparser.h"
#include "projectexplorerconstants.h"
#include "toolchainmanager.h"

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/synchronousprocess.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QBuffer>
#include <QCoreApplication>
#include <QFileInfo>
#include <QProcess>
#include <QScopedPointer>

#include <QLineEdit>
#include <QFormLayout>

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
static const char supportedAbisKeyC[] = "ProjectExplorer.GccToolChain.SupportedAbis";

static QByteArray runGcc(const FileName &gcc, const QStringList &arguments, const QStringList &env)
{
    if (gcc.isEmpty() || !gcc.toFileInfo().isExecutable())
        return QByteArray();

    QProcess cpp;
    // Force locale: This function is used only to detect settings inside the tool chain, so this is save.
    QStringList environment(env);
    environment.append(QLatin1String("LC_ALL=C"));

    cpp.setEnvironment(environment);
    cpp.start(gcc.toString(), arguments);
    if (!cpp.waitForStarted()) {
        qWarning("%s: Cannot start '%s': %s", Q_FUNC_INFO, qPrintable(gcc.toUserOutput()),
            qPrintable(cpp.errorString()));
        return QByteArray();
    }
    cpp.closeWriteChannel();
    if (!cpp.waitForFinished(10000)) {
        SynchronousProcess::stopProcess(cpp);
        qWarning("%s: Timeout running '%s'.", Q_FUNC_INFO, qPrintable(gcc.toUserOutput()));
        return QByteArray();
    }
    if (cpp.exitStatus() != QProcess::NormalExit) {
        qWarning("%s: '%s' crashed.", Q_FUNC_INFO, qPrintable(gcc.toUserOutput()));
        return QByteArray();
    }

    const QByteArray stdErr = SynchronousProcess::normalizeNewlines(
                QString::fromLocal8Bit(cpp.readAllStandardError())).toLocal8Bit();
    if (cpp.exitCode() != 0) {
        qWarning().nospace()
            << Q_FUNC_INFO << ": " << gcc.toUserOutput() << ' '
            << arguments.join(QLatin1Char(' ')) << " returned exit code "
            << cpp.exitCode() << ": " << stdErr;
        return QByteArray();
    }

    QByteArray data = SynchronousProcess::normalizeNewlines(
                QString::fromLocal8Bit(cpp.readAllStandardOutput())).toLocal8Bit();
    if (!data.isEmpty() && !data.endsWith('\n'))
        data.append('\n');
    data.append(stdErr);
    return data;
}

static const QStringList gccPredefinedMacrosOptions()
{
    return QStringList() << QLatin1String("-xc++") << QLatin1String("-E") << QLatin1String("-dM");
}

static QByteArray gccPredefinedMacros(const FileName &gcc, const QStringList &args, const QStringList &env)
{
    QStringList arguments = args;
    arguments << QLatin1String("-");

    QByteArray predefinedMacros = runGcc(gcc, arguments, env);
    // Sanity check in case we get an error message instead of real output:
    QTC_CHECK(predefinedMacros.isNull() || predefinedMacros.startsWith("#define "));
    if (HostOsInfo::isMacHost()) {
        // Turn off flag indicating Apple's blocks support
        const QByteArray blocksDefine("#define __BLOCKS__ 1");
        const QByteArray blocksUndefine("#undef __BLOCKS__");
        const int idx = predefinedMacros.indexOf(blocksDefine);
        if (idx != -1)
            predefinedMacros.replace(idx, blocksDefine.length(), blocksUndefine);

        // Define __strong and __weak (used for Apple's GC extension of C) to be empty
        predefinedMacros.append("#define __strong\n");
        predefinedMacros.append("#define __weak\n");
    }
    return predefinedMacros;
}

const int GccToolChain::PREDEFINED_MACROS_CACHE_SIZE = 16;

QList<HeaderPath> GccToolChain::gccHeaderPaths(const FileName &gcc, const QStringList &arguments,
                                               const QStringList &env)
{
    QList<HeaderPath> systemHeaderPaths;
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
        HeaderPath::Kind kind = HeaderPath::UserHeaderPath;
        while (cpp.canReadLine()) {
            line = cpp.readLine();
            if (line.startsWith("#include")) {
                kind = HeaderPath::GlobalHeaderPath;
            } else if (! line.isEmpty() && QChar(QLatin1Char(line.at(0))).isSpace()) {
                HeaderPath::Kind thisHeaderKind = kind;

                line = line.trimmed();

                const int index = line.indexOf(" (framework directory)");
                if (index != -1) {
                    line.truncate(index);
                    thisHeaderKind = HeaderPath::FrameworkHeaderPath;
                }

                systemHeaderPaths.append(HeaderPath(QFile::decodeName(line), thisHeaderKind));
            } else if (line.startsWith("End of search list.")) {
                break;
            } else {
                qWarning("%s: Ignoring line: %s", __FUNCTION__, line.constData());
            }
        }
    }
    return systemHeaderPaths;
}

static QList<Abi> guessGccAbi(const QString &m, const QByteArray &macros)
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

    if (macros.contains("#define __SIZEOF_SIZE_T__ 8"))
        width = 64;
    else if (macros.contains("#define __SIZEOF_SIZE_T__ 4"))
        width = 32;

    if (os == Abi::MacOS) {
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

static QList<Abi> guessGccAbi(const FileName &path, const QStringList &env,
                              const QByteArray &macros,
                              const QStringList &extraArgs = QStringList())
{
    if (path.isEmpty())
        return QList<Abi>();

    QStringList arguments = extraArgs;
    arguments << QLatin1String("-dumpmachine");
    QString machine = QString::fromLocal8Bit(runGcc(path, arguments, env)).trimmed();
    if (machine.isEmpty())
        return QList<Abi>(); // no need to continue if running failed once...
    return guessGccAbi(machine, macros);
}

static QString gccVersion(const FileName &path, const QStringList &env)
{
    QStringList arguments(QLatin1String("-dumpversion"));
    return QString::fromLocal8Bit(runGcc(path, arguments, env)).trimmed();
}

// --------------------------------------------------------------------------
// GccToolChain
// --------------------------------------------------------------------------

GccToolChain::GccToolChain(Detection d) :
    ToolChain(Constants::GCC_TOOLCHAIN_TYPEID, d)
{ }

GccToolChain::GccToolChain(Core::Id typeId, Detection d) :
    ToolChain(typeId, d)
{ }

void GccToolChain::setCompilerCommand(const FileName &path)
{
    if (path == m_compilerCommand)
        return;

    m_compilerCommand = path;
}

void GccToolChain::setSupportedAbis(const QList<Abi> &m_abis)
{
    m_supportedAbis = m_abis;
}

void GccToolChain::setMacroCache(const QStringList &allCxxflags, const QByteArray &macros) const
{
    if (macros.isNull())
        return;

    CacheItem runResults;
    QByteArray data = macros;
    runResults.first = allCxxflags;
    if (macros.isNull())
        data = QByteArray("");
    runResults.second = data;

    m_predefinedMacros.push_back(runResults);
    if (m_predefinedMacros.size() > PREDEFINED_MACROS_CACHE_SIZE)
        m_predefinedMacros.pop_front();
}

QByteArray GccToolChain::macroCache(const QStringList &allCxxflags) const
{
    for (GccCache::iterator it = m_predefinedMacros.begin(); it != m_predefinedMacros.end(); ++it) {
        if (it->first == allCxxflags) {
            // Increase cached item priority
            CacheItem pair = *it;
            m_predefinedMacros.erase(it);
            m_predefinedMacros.push_back(pair);

            return pair.second;
        }
    }
    return QByteArray();
}

QString GccToolChain::defaultDisplayName() const
{
    if (!m_targetAbi.isValid())
        return typeDisplayName();
    return QCoreApplication::translate("ProjectExplorer::GccToolChain",
                                       "%1 (%2 %3 in %4)").arg(typeDisplayName(),
                                                               Abi::toString(m_targetAbi.architecture()),
                                                               Abi::toString(m_targetAbi.wordWidth()),
                                                               compilerCommand().parentDir().toUserOutput());
}

ToolChain::CompilerFlags GccToolChain::defaultCompilerFlags() const
{
    return CompilerFlags(GnuExtensions);
}

QString GccToolChain::typeDisplayName() const
{
    return GccToolChainFactory::tr("GCC");
}

Abi GccToolChain::targetAbi() const
{
    return m_targetAbi;
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

/**
 * @brief Asks compiler for set of predefined macros
 * @param cxxflags - compiler flags collected from project settings
 * @return defines list, one per line, e.g. "#define __GXX_WEAK__ 1"
 *
 * @note changing compiler flags sometimes changes macros set, e.g. -fopenmp
 * adds _OPENMP macro, for full list of macro search by word "when" on this page:
 * http://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
 */
QByteArray GccToolChain::predefinedMacros(const QStringList &cxxflags) const
{
    QStringList allCxxflags = m_platformCodeGenFlags + cxxflags;  // add only cxxflags is empty?

    QByteArray macros = macroCache(allCxxflags);
    if (!macros.isNull())
        return macros;

    // Using a clean environment breaks ccache/distcc/etc.
    Environment env = Environment::systemEnvironment();
    addToEnvironment(env);
    QStringList arguments = gccPredefinedMacrosOptions();
    for (int iArg = 0; iArg < allCxxflags.length(); ++iArg) {
        const QString &a = allCxxflags.at(iArg);
        if (a == QLatin1String("-arch")) {
            if (++iArg < allCxxflags.length() && !arguments.contains(a))
                arguments << a << allCxxflags.at(iArg);
        } else if (a == QLatin1String("--sysroot") || a == QLatin1String("-isysroot")) {
            if (++iArg < allCxxflags.length())
                arguments << a << allCxxflags.at(iArg);
        } else if (a == QLatin1String("-m128bit-long-double") || a == QLatin1String("-m32")
                || a == QLatin1String("-m3dnow") || a == QLatin1String("-m3dnowa")
                || a == QLatin1String("-m64") || a == QLatin1String("-m96bit-long-double")
                || a == QLatin1String("-mabm") || a == QLatin1String("-maes")
                || a.startsWith(QLatin1String("-march=")) || a == QLatin1String("-mavx")
                || a.startsWith(QLatin1String("-masm=")) || a == QLatin1String("-mcx16")
                || a == QLatin1String("-mfma") || a == QLatin1String("-mfma4")
                || a == QLatin1String("-mlwp") || a == QLatin1String("-mpclmul")
                || a == QLatin1String("-mpopcnt") || a == QLatin1String("-msse")
                || a == QLatin1String("-msse2") || a == QLatin1String("-msse2avx")
                || a == QLatin1String("-msse3") || a == QLatin1String("-msse4")
                || a == QLatin1String("-msse4.1") || a == QLatin1String("-msse4.2")
                || a == QLatin1String("-msse4a") || a == QLatin1String("-mssse3")
                || a.startsWith(QLatin1String("-mtune=")) || a == QLatin1String("-mxop")
                || a == QLatin1String("-Os") || a == QLatin1String("-O0") || a == QLatin1String("-O1")
                || a == QLatin1String("-O2") || a == QLatin1String("-O3")
                || a == QLatin1String("-ffinite-math-only") || a == QLatin1String("-fshort-double")
                || a == QLatin1String("-fshort-wchar") || a == QLatin1String("-fsignaling-nans")
                || a == QLatin1String("-fno-inline") || a == QLatin1String("-fno-exceptions")
                || a == QLatin1String("-fstack-protector") || a == QLatin1String("-fstack-protector-all")
                || a == QLatin1String("-fsanitize=address") || a == QLatin1String("-fno-rtti")
                || a.startsWith(QLatin1String("-std=")) || a.startsWith(QLatin1String("-stdlib="))
                || a.startsWith(QLatin1String("-specs="))
                || a == QLatin1String("-ansi") || a == QLatin1String("-undef")
                || a.startsWith(QLatin1String("-D")) || a.startsWith(QLatin1String("-U"))
                || a == QLatin1String("-fopenmp") || a == QLatin1String("-Wno-deprecated")
                || a == QLatin1String("-fPIC") || a == QLatin1String("-fpic")
                || a == QLatin1String("-fPIE") || a == QLatin1String("-fpie"))
            arguments << a;
    }
    macros = gccPredefinedMacros(m_compilerCommand, reinterpretOptions(arguments),
                                 env.toStringList());

    setMacroCache(allCxxflags, macros);
    return macros;
}

/**
 * @brief Parses gcc flags -std=*, -fopenmp, -fms-extensions, -ansi.
 * @see http://gcc.gnu.org/onlinedocs/gcc/C-Dialect-Options.html
 */
ToolChain::CompilerFlags GccToolChain::compilerFlags(const QStringList &cxxflags) const
{
    CompilerFlags flags = defaultCompilerFlags();

    const QStringList allCxxflags = m_platformCodeGenFlags + cxxflags; // add only cxxflags is empty?
    foreach (const QString &flag, allCxxflags) {
        if (flag.startsWith(QLatin1String("-std="))) {
            const QByteArray std = flag.mid(5).toLatin1();
            if (std == "c++98" || std == "c++03") {
                flags &= ~CompilerFlags(StandardCxx11 | StandardCxx14 | StandardCxx17 | GnuExtensions);
                flags |= StandardCxx98;
            } else if (std == "gnu++98" || std == "gnu++03") {
                flags &= ~CompilerFlags(StandardCxx11 | StandardCxx14 | StandardCxx17);
                flags |= GnuExtensions;
            } else if (std == "c++0x" || std == "c++11") {
                flags |= StandardCxx11;
                flags &= ~CompilerFlags(StandardCxx14 | StandardCxx17 | GnuExtensions);
            } else if (std == "c++14" || std == "c++1y") {
                flags |= StandardCxx14;
                flags &= ~CompilerFlags(StandardCxx11 | StandardCxx17 | GnuExtensions);
            } else if (std == "c++17" || std == "c++1z") {
                flags |= StandardCxx17;
                flags &= ~CompilerFlags(StandardCxx11 | StandardCxx14 | GnuExtensions);
            } else if (std == "gnu++0x" || std == "gnu++11" || std== "gnu++1y") {
                flags |= CompilerFlags(StandardCxx11 | GnuExtensions);
                flags &= ~CompilerFlags(StandardCxx14 | StandardCxx17);
            } else if (std == "c89" || std == "c90"
                       || std == "iso9899:1990" || std == "iso9899:199409") {
                flags &= ~CompilerFlags(StandardC99 | StandardC11);
            } else if (std == "gnu89" || std == "gnu90") {
                flags &= ~CompilerFlags(StandardC99 | StandardC11);
                flags |= GnuExtensions;
            } else if (std == "c99" || std == "c9x"
                       || std == "iso9899:1999" || std == "iso9899:199x") {
                flags |= StandardC99;
                flags &= ~StandardC11;
            } else if (std == "gnu99" || std == "gnu9x") {
                flags |= CompilerFlags(StandardC99 | GnuExtensions);
                flags &= ~StandardC11;
            } else if (std == "c11" || std == "c1x" || std == "iso9899:2011") {
                flags |= CompilerFlags(StandardC99 | StandardC11);
            } else if (std == "gnu11" || std == "gnu1x") {
                flags |= CompilerFlags(StandardC99 | StandardC11 | GnuExtensions);
            }
        } else if (flag == QLatin1String("-fopenmp")) {
            flags |= OpenMP;
        } else if (flag == QLatin1String("-fms-extensions")) {
            flags |= MicrosoftExtensions;
        } else if (flag == QLatin1String("-ansi")) {
            flags &= ~CompilerFlags(StandardCxx11 | GnuExtensions
                                    | StandardC99 | StandardC11);
        }
    }

    return flags;
}

GccToolChain::WarningFlags GccToolChain::warningFlags(const QStringList &cflags) const
{
    // based on 'LC_ALL="en" gcc -Q --help=warnings | grep enabled'
    WarningFlags flags(WarnDeprecated | WarnIgnoredQualfiers
                       | WarnSignedComparison | WarnUninitializedVars);
    WarningFlags groupWall(WarningsAll | WarnUnknownPragma |WarnUnusedFunctions
                           | WarnUnusedLocals | WarnUnusedResult | WarnUnusedValue
                           | WarnSignedComparison | WarnUninitializedVars);
    WarningFlags groupWextra(WarningsExtra | WarnIgnoredQualfiers | WarnUnusedParams);

    foreach (const QString &flag, cflags) {
        if (flag == QLatin1String("--all-warnings"))
            flags |= groupWall;
        else if (flag == QLatin1String("--extra-warnings"))
            flags |= groupWextra;

        WarningFlagAdder add(flag, flags);
        if (add.triggered())
            continue;

        // supported by clang too
        add("error", WarningsAsErrors);
        add("all", groupWall);
        add("extra", groupWextra);
        add("deprecated", WarnDeprecated);
        add("effc++", WarnEffectiveCxx);
        add("ignored-qualifiers", WarnIgnoredQualfiers);
        add("non-virtual-dtor", WarnNonVirtualDestructor);
        add("overloaded-virtual", WarnOverloadedVirtual);
        add("shadow", WarnHiddenLocals);
        add("sign-compare", WarnSignedComparison);
        add("unknown-pragmas", WarnUnknownPragma);
        add("unused", ToolChain::WarningFlag(
                WarnUnusedFunctions | WarnUnusedLocals | WarnUnusedParams
                | WarnUnusedResult | WarnUnusedValue));
        add("unused-function", WarnUnusedFunctions);
        add("unused-variable", WarnUnusedLocals);
        add("unused-parameter", WarnUnusedParams);
        add("unused-result", WarnUnusedResult);
        add("unused-value", WarnUnusedValue);
        add("uninitialized", WarnUninitializedVars);
    }
    return flags;
}

QList<HeaderPath> GccToolChain::systemHeaderPaths(const QStringList &cxxflags, const FileName &sysRoot) const
{
    if (m_headerPaths.isEmpty()) {
        // Using a clean environment breaks ccache/distcc/etc.
        Environment env = Environment::systemEnvironment();
        addToEnvironment(env);
        // Prepare arguments
        QStringList arguments;
        if (!sysRoot.isEmpty())
            arguments.append(QString::fromLatin1("--sysroot=%1").arg(sysRoot.toString()));

        QStringList flags;
        flags << m_platformCodeGenFlags << cxxflags;
        foreach (const QString &a, flags) {
            if (a.startsWith(QLatin1String("-stdlib=")))
                arguments << a;
        }

        arguments << QLatin1String("-xc++")
                  << QLatin1String("-E")
                  << QLatin1String("-v")
                  << QLatin1String("-");

        m_headerPaths = gccHeaderPaths(m_compilerCommand, reinterpretOptions(arguments), env.toStringList());
    }
    return m_headerPaths;
}

void GccToolChain::addCommandPathToEnvironment(const FileName &command, Environment &env)
{
    if (!command.isEmpty())
        env.prependOrSetPath(command.parentDir().toString());
}

void GccToolChain::addToEnvironment(Environment &env) const
{
    addCommandPathToEnvironment(m_compilerCommand, env);
}

QList<FileName> GccToolChain::suggestedMkspecList() const
{
    Abi abi = targetAbi();
    Abi host = Abi::hostAbi();

    // Cross compile: Leave the mkspec alone!
    if (abi.architecture() != host.architecture()
            || abi.os() != host.os()
            || abi.osFlavor() != host.osFlavor()) // Note: This can fail:-(
        return QList<FileName>();

    if (abi.os() == Abi::MacOS) {
        QString v = version();
        // prefer versioned g++ on mac. This is required to enable building for older Mac OS versions
        if (v.startsWith(QLatin1String("4.0")) && m_compilerCommand.endsWith(QLatin1String("-4.0")))
            return QList<FileName>() << FileName::fromLatin1("macx-g++40");
        if (v.startsWith(QLatin1String("4.2")) && m_compilerCommand.endsWith(QLatin1String("-4.2")))
            return QList<FileName>() << FileName::fromLatin1("macx-g++42");
        return QList<FileName>() << FileName::fromLatin1("macx-g++");
    }

    if (abi.os() == Abi::LinuxOS) {
        if (abi.osFlavor() != Abi::GenericLinuxFlavor)
            return QList<FileName>(); // most likely not a desktop, so leave the mkspec alone.
        if (abi.wordWidth() == host.wordWidth()) {
            // no need to explicitly set the word width, but provide that mkspec anyway to make sure
            // that the correct compiler is picked if a mkspec with a wordwidth is given.
            return QList<FileName>() << FileName::fromLatin1("linux-g++")
                                            << FileName::fromString(QLatin1String("linux-g++-") + QString::number(m_targetAbi.wordWidth()));
        }
        return QList<FileName>() << FileName::fromString(QLatin1String("linux-g++-") + QString::number(m_targetAbi.wordWidth()));
    }

    if (abi.os() == Abi::BsdOS && abi.osFlavor() == Abi::FreeBsdFlavor)
        return QList<FileName>() << FileName::fromLatin1("freebsd-g++");

    return QList<FileName>();
}

QString GccToolChain::makeCommand(const Environment &environment) const
{
    QString make = QLatin1String("make");
    FileName tmp = environment.searchInPath(make);
    return tmp.isEmpty() ? make : tmp.toString();
}

IOutputParser *GccToolChain::outputParser() const
{
    return new GccParser;
}

void GccToolChain::resetToolChain(const FileName &path)
{
    if (path == m_compilerCommand)
        return;

    bool resetDisplayName = displayName() == defaultDisplayName();

    setCompilerCommand(path);

    Abi currentAbi = m_targetAbi;
    m_supportedAbis = detectSupportedAbis();

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
    data.insert(QLatin1String(compilerCommandKeyC), m_compilerCommand.toString());
    data.insert(QLatin1String(compilerPlatformCodeGenFlagsKeyC), m_platformCodeGenFlags);
    data.insert(QLatin1String(compilerPlatformLinkerFlagsKeyC), m_platformLinkerFlags);
    data.insert(QLatin1String(targetAbiKeyC), m_targetAbi.toString());
    QStringList abiList = Utils::transform(m_supportedAbis, &Abi::toString);
    data.insert(QLatin1String(supportedAbisKeyC), abiList);
    return data;
}

bool GccToolChain::fromMap(const QVariantMap &data)
{
    if (!ToolChain::fromMap(data))
        return false;

    m_compilerCommand = FileName::fromString(data.value(QLatin1String(compilerCommandKeyC)).toString());
    m_platformCodeGenFlags = data.value(QLatin1String(compilerPlatformCodeGenFlagsKeyC)).toStringList();
    m_platformLinkerFlags = data.value(QLatin1String(compilerPlatformLinkerFlagsKeyC)).toStringList();
    m_targetAbi = Abi(data.value(QLatin1String(targetAbiKeyC)).toString());
    QStringList abiList = data.value(QLatin1String(supportedAbisKeyC)).toStringList();
    m_supportedAbis.clear();
    foreach (const QString &a, abiList) {
        Abi abi(a);
        if (!abi.isValid())
            continue;
        m_supportedAbis.append(abi);
    }
    return true;
}

bool GccToolChain::operator ==(const ToolChain &other) const
{
    if (!ToolChain::operator ==(other))
        return false;

    const GccToolChain *gccTc = static_cast<const GccToolChain *>(&other);
    return m_compilerCommand == gccTc->m_compilerCommand && m_targetAbi == gccTc->m_targetAbi
            && m_platformCodeGenFlags == gccTc->m_platformCodeGenFlags
            && m_platformLinkerFlags == gccTc->m_platformLinkerFlags;
}

ToolChainConfigWidget *GccToolChain::configurationWidget()
{
    return new GccToolChainConfigWidget(this);
}

void GccToolChain::updateSupportedAbis() const
{
    if (m_supportedAbis.isEmpty())
        m_supportedAbis = detectSupportedAbis();
}

QList<Abi> GccToolChain::detectSupportedAbis() const
{
    Environment env = Environment::systemEnvironment();
    addToEnvironment(env);
    QByteArray macros = predefinedMacros(QStringList());
    return guessGccAbi(m_compilerCommand, env.toStringList(), macros, platformCodeGenFlags());
}

QString GccToolChain::detectVersion() const
{
    Environment env = Environment::systemEnvironment();
    addToEnvironment(env);
    return gccVersion(m_compilerCommand, env.toStringList());
}

// --------------------------------------------------------------------------
// GccToolChainFactory
// --------------------------------------------------------------------------

GccToolChainFactory::GccToolChainFactory()
{
    setTypeId(Constants::GCC_TOOLCHAIN_TYPEID);
    setDisplayName(tr("GCC"));
}

bool GccToolChainFactory::canCreate()
{
    return true;
}

ToolChain *GccToolChainFactory::create()
{
    return createToolChain(false);
}

QList<ToolChain *> GccToolChainFactory::autoDetect()
{
    QList<ToolChain *> tcs;
    if (HostOsInfo::isMacHost()) {
        // Old mac compilers needed to support macx-gccXY mkspecs:
        tcs.append(autoDetectToolchains(QLatin1String("g++-4.0"), Abi::hostAbi()));
        tcs.append(autoDetectToolchains(QLatin1String("g++-4.2"), Abi::hostAbi()));
    }
    tcs.append(autoDetectToolchains(QLatin1String("g++"), Abi::hostAbi()));

    return tcs;
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
    return 0;
}

GccToolChain *GccToolChainFactory::createToolChain(bool autoDetect)
{
    return new GccToolChain(autoDetect ? ToolChain::AutoDetection : ToolChain::ManualDetection);
}

QList<ToolChain *> GccToolChainFactory::autoDetectToolchains(const QString &compiler,
                                                                       const Abi &requiredAbi)
{
    QList<ToolChain *> result;

    Environment systemEnvironment = Environment::systemEnvironment();
    const FileName compilerPath = systemEnvironment.searchInPath(compiler);
    if (compilerPath.isEmpty())
        return result;

    GccToolChain::addCommandPathToEnvironment(compilerPath, systemEnvironment);
    QByteArray macros
            = gccPredefinedMacros(compilerPath, gccPredefinedMacrosOptions(), systemEnvironment.toStringList());
    QList<Abi> abiList = guessGccAbi(compilerPath, systemEnvironment.toStringList(), macros);
    if (!abiList.contains(requiredAbi)) {
        if (requiredAbi.wordWidth() != 64
                || !abiList.contains(Abi(requiredAbi.architecture(), requiredAbi.os(), requiredAbi.osFlavor(),
                                         requiredAbi.binaryFormat(), 32)))
            return result;
    }

    foreach (const Abi &abi, abiList) {
        QScopedPointer<GccToolChain> tc(createToolChain(true));
        tc->setMacroCache(QStringList(), macros);
        if (tc.isNull())
            return result;

        tc->setCompilerCommand(compilerPath);
        tc->setSupportedAbis(abiList);
        tc->setTargetAbi(abi);
        tc->setDisplayName(tc->defaultDisplayName()); // reset displayname

        result.append(tc.take());
    }

    return result;
}

// --------------------------------------------------------------------------
// GccToolChainConfigWidget
// --------------------------------------------------------------------------

GccToolChainConfigWidget::GccToolChainConfigWidget(GccToolChain *tc) :
    ToolChainConfigWidget(tc),
    m_compilerCommand(new PathChooser),
    m_abiWidget(new AbiWidget),
    m_isReadOnly(false)
{
    Q_ASSERT(tc);

    const QStringList gnuVersionArgs = QStringList(QLatin1String("--version"));
    m_compilerCommand->setExpectedKind(PathChooser::ExistingCommand);
    m_compilerCommand->setCommandVersionArguments(gnuVersionArgs);
    m_compilerCommand->setHistoryCompleter(QLatin1String("PE.Gcc.Command.History"));
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

    connect(m_compilerCommand, SIGNAL(changed(QString)), this, SLOT(handleCompilerCommandChange()));
    connect(m_platformCodeGenFlagsLineEdit, SIGNAL(editingFinished()), this, SLOT(handlePlatformCodeGenFlagsChange()));
    connect(m_platformLinkerFlagsLineEdit, SIGNAL(editingFinished()), this, SLOT(handlePlatformLinkerFlagsChange()));
    connect(m_abiWidget, SIGNAL(abiChanged()), this, SIGNAL(dirty()));
}

void GccToolChainConfigWidget::applyImpl()
{
    if (toolChain()->isAutoDetected())
        return;

    GccToolChain *tc = static_cast<GccToolChain *>(toolChain());
    Q_ASSERT(tc);
    QString displayName = tc->displayName();
    tc->setCompilerCommand(m_compilerCommand->fileName());
    tc->setSupportedAbis(m_abiWidget->supportedAbis());
    tc->setTargetAbi(m_abiWidget->currentAbi());
    tc->setDisplayName(displayName); // reset display name
    tc->setPlatformCodeGenFlags(splitString(m_platformCodeGenFlagsLineEdit->text()));
    tc->setPlatformLinkerFlags(splitString(m_platformLinkerFlagsLineEdit->text()));
    tc->setMacroCache(tc->platformCodeGenFlags(), m_macros);
}

void GccToolChainConfigWidget::setFromToolchain()
{
    // subwidgets are not yet connected!
    bool blocked = blockSignals(true);
    GccToolChain *tc = static_cast<GccToolChain *>(toolChain());
    m_compilerCommand->setFileName(tc->compilerCommand());
    m_platformCodeGenFlagsLineEdit->setText(QtcProcess::joinArgs(tc->platformCodeGenFlags()));
    m_platformLinkerFlagsLineEdit->setText(QtcProcess::joinArgs(tc->platformLinkerFlags()));
    m_abiWidget->setAbis(tc->supportedAbis(), tc->targetAbi());
    if (!m_isReadOnly && !m_compilerCommand->path().isEmpty())
        m_abiWidget->setEnabled(true);
    blockSignals(blocked);
}

bool GccToolChainConfigWidget::isDirtyImpl() const
{
    GccToolChain *tc = static_cast<GccToolChain *>(toolChain());
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
        res = QtcProcess::splitArgs(s + QLatin1Char('\\'), osType, false, &splitError);
        if (splitError != QtcProcess::SplitOk){
            res = QtcProcess::splitArgs(s + QLatin1Char('"'), osType, false, &splitError);
            if (splitError != QtcProcess::SplitOk)
                res = QtcProcess::splitArgs(s + QLatin1Char('\''), osType, false, &splitError);
        }
    }
    return res;
}

void GccToolChainConfigWidget::handleCompilerCommandChange()
{
    bool haveCompiler = false;
    Abi currentAbi = m_abiWidget->currentAbi();
    bool customAbi = m_abiWidget->isCustomAbi();
    FileName path = m_compilerCommand->fileName();
    QList<Abi> abiList;

    if (!path.isEmpty()) {
        QFileInfo fi(path.toFileInfo());
        haveCompiler = fi.isExecutable() && fi.isFile();
    }
    if (haveCompiler) {
        Environment env = Environment::systemEnvironment();
        GccToolChain::addCommandPathToEnvironment(path, env);
        QStringList args = gccPredefinedMacrosOptions() + splitString(m_platformCodeGenFlagsLineEdit->text());
        m_macros = gccPredefinedMacros(path, args, env.toStringList());
        abiList = guessGccAbi(path, env.toStringList(), m_macros,
                              splitString(m_platformCodeGenFlagsLineEdit->text()));
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

QString ClangToolChain::typeDisplayName() const
{
    return ClangToolChainFactory::tr("Clang");
}

QString ClangToolChain::makeCommand(const Environment &environment) const
{
    QStringList makes;
    if (HostOsInfo::isWindowsHost()) {
        makes << QLatin1String("mingw32-make.exe");
        makes << QLatin1String("make.exe");
    } else {
        makes << QLatin1String("make");
    }

    FileName tmp;
    foreach (const QString &make, makes) {
        tmp = environment.searchInPath(make);
        if (!tmp.isEmpty())
            return tmp.toString();
    }
    return makes.first();
}

/**
 * @brief Similar to \a GccToolchain::compilerFlags, but recognizes
 * "-fborland-extensions".
 */
ToolChain::CompilerFlags ClangToolChain::compilerFlags(const QStringList &cxxflags) const
{
    CompilerFlags flags = GccToolChain::compilerFlags(cxxflags);
    if (cxxflags.contains(QLatin1String("-fborland-extensions")))
        flags |= BorlandExtensions;
    return flags;
}

ToolChain::WarningFlags ClangToolChain::warningFlags(const QStringList &cflags) const
{
    WarningFlags flags = GccToolChain::warningFlags(cflags);
    foreach (const QString &flag, cflags) {
        if (flag == QLatin1String("-Wdocumentation"))
            flags |= WarnDocumentation;
        if (flag == QLatin1String("-Wno-documentation"))
            flags &= ~WarnDocumentation;
    }
    return flags;
}

QList<FileName> ClangToolChain::suggestedMkspecList() const
{
    Abi abi = targetAbi();
    if (abi.os() == Abi::MacOS)
        return QList<FileName>()
                << FileName::fromLatin1("macx-clang")
                << FileName::fromLatin1("macx-clang-32")
                << FileName::fromLatin1("unsupported/macx-clang")
                << FileName::fromLatin1("macx-ios-clang");
    else if (abi.os() == Abi::LinuxOS)
        return QList<FileName>()
                << FileName::fromLatin1("linux-clang")
                << FileName::fromLatin1("unsupported/linux-clang");
    return QList<FileName>(); // Note: Not supported by Qt yet, so default to the mkspec the Qt was build with
}

void ClangToolChain::addToEnvironment(Environment &env) const
{
    GccToolChain::addToEnvironment(env);
    // Clang takes PWD as basis for debug info, if set.
    // When running Qt Creator from a shell, PWD is initially set to an "arbitrary" value.
    // Since the tools are not called through a shell, PWD is never changed to the actual cwd,
    // so we better make sure PWD is empty to begin with
    env.unset(QLatin1String("PWD"));
}

ToolChain::CompilerFlags ClangToolChain::defaultCompilerFlags() const
{
    return CompilerFlags(GnuExtensions | StandardCxx11);
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
    setTypeId(Constants::CLANG_TOOLCHAIN_TYPEID);
}

QList<ToolChain *> ClangToolChainFactory::autoDetect()
{
    Abi ha = Abi::hostAbi();
    return autoDetectToolchains(QLatin1String("clang++"), ha);
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

QList<FileName> MingwToolChain::suggestedMkspecList() const
{
    if (HostOsInfo::isWindowsHost())
        return QList<FileName>() << FileName::fromLatin1("win32-g++");
    if (HostOsInfo::isLinuxHost()) {
        if (version().startsWith(QLatin1String("4.6.")))
            return QList<FileName>()
                    << FileName::fromLatin1("win32-g++-4.6-cross")
                    << FileName::fromLatin1("unsupported/win32-g++-4.6-cross");
        else
            return QList<FileName>()
                    << FileName::fromLatin1("win32-g++-cross")
                    << FileName::fromLatin1("unsupported/win32-g++-cross");
    }
    return QList<FileName>();
}

QString MingwToolChain::makeCommand(const Environment &environment) const
{
    QStringList makes;
    if (HostOsInfo::isWindowsHost()) {
        makes << QLatin1String("mingw32-make.exe");
        makes << QLatin1String("make.exe");
    } else {
        makes << QLatin1String("make");
    }

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
    setTypeId(Constants::MINGW_TOOLCHAIN_TYPEID);
    setDisplayName(tr("MinGW"));
}

QList<ToolChain *> MingwToolChainFactory::autoDetect()
{
    Abi ha = Abi::hostAbi();
    return autoDetectToolchains(QLatin1String("g++"),
                                Abi(ha.architecture(), Abi::WindowsOS, Abi::WindowsMSysFlavor, Abi::PEFormat, ha.wordWidth()));
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
 * Similar to \a GccToolchain::compilerFlags, but uses "-openmp" instead of
 * "-fopenmp" and "-fms-dialect[=ver]" instead of "-fms-extensions".
 * @see UNIX manual for "icc"
 */
ToolChain::CompilerFlags LinuxIccToolChain::compilerFlags(const QStringList &cxxflags) const
{
    QStringList copy = cxxflags;
    copy.removeAll(QLatin1String("-fopenmp"));
    copy.removeAll(QLatin1String("-fms-extensions"));

    CompilerFlags flags = GccToolChain::compilerFlags(cxxflags);
    if (cxxflags.contains(QLatin1String("-openmp")))
        flags |= OpenMP;
    if (cxxflags.contains(QLatin1String("-fms-dialect"))
            || cxxflags.contains(QLatin1String("-fms-dialect=8"))
            || cxxflags.contains(QLatin1String("-fms-dialect=9"))
            || cxxflags.contains(QLatin1String("-fms-dialect=10")))
        flags |= MicrosoftExtensions;
    return flags;
}

IOutputParser *LinuxIccToolChain::outputParser() const
{
    return new LinuxIccParser;
}

QList<FileName> LinuxIccToolChain::suggestedMkspecList() const
{
    return QList<FileName>()
            << FileName::fromString(QLatin1String("linux-icc-") + QString::number(targetAbi().wordWidth()));
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
    setTypeId(Constants::LINUXICC_TOOLCHAIN_TYPEID);
}

QList<ToolChain *> LinuxIccToolChainFactory::autoDetect()
{
    return autoDetectToolchains(QLatin1String("icpc"), Abi::hostAbi());
}

bool LinuxIccToolChainFactory::canRestore(const QVariantMap &data)
{
    return typeIdFromMap(data) == Constants::LINUXICC_TOOLCHAIN_TYPEID;
}

GccToolChain *LinuxIccToolChainFactory::createToolChain(bool autoDetect)
{
    return new LinuxIccToolChain(autoDetect ? ToolChain::AutoDetection : ToolChain::ManualDetection);
}

GccToolChain::WarningFlagAdder::WarningFlagAdder(const QString &flag, ToolChain::WarningFlags &flags) :
    m_flags(flags),
    m_triggered(false)
{
    if (!flag.startsWith(QLatin1String("-W"))) {
        m_triggered = true;
        return;
    }

    m_doesEnable = !flag.startsWith(QLatin1String("-Wno-"));
    if (m_doesEnable)
        m_flagUtf8 = flag.mid(2).toUtf8();
    else
        m_flagUtf8 = flag.mid(5).toUtf8();
}

void GccToolChain::WarningFlagAdder::operator ()(const char name[], ToolChain::WarningFlags flagsSet)
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

void GccToolChain::WarningFlagAdder::operator ()(const char name[], ToolChain::WarningFlag flag)
{
    (*this)(name, WarningFlags(flag));
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
            << (QStringList() << QLatin1String("arm-unknown-unknown-unknown-64bit"));
    QTest::newRow("broken input -- 32bit")
            << QString::fromLatin1("arm-none-foo-gnueabi")
            << QByteArray("#define __SIZEOF_SIZE_T__ 4\n#define __Something\n")
            << (QStringList() << QLatin1String("arm-unknown-unknown-unknown-32bit"));
    QTest::newRow("totally broken input -- 32bit")
            << QString::fromLatin1("foo-bar-foo")
            << QByteArray("#define __SIZEOF_SIZE_T__ 4\n#define __Something\n")
            << (QStringList());

    QTest::newRow("Linux 1 (32bit intel)")
            << QString::fromLatin1("i686-linux-gnu")
            << QByteArray("#define __SIZEOF_SIZE_T__ 4\n")
            << (QStringList() << QLatin1String("x86-linux-generic-elf-32bit"));
    QTest::newRow("Linux 2 (32bit intel)")
            << QString::fromLatin1("i486-linux-gnu")
            << QByteArray("#define __SIZEOF_SIZE_T__ 4\n")
            << (QStringList() << QLatin1String("x86-linux-generic-elf-32bit"));
    QTest::newRow("Linux 3 (64bit intel)")
            << QString::fromLatin1("x86_64-linux-gnu")
            << QByteArray("#define __SIZEOF_SIZE_T__ 8\n")
            << (QStringList() << QLatin1String("x86-linux-generic-elf-64bit")
                              << QLatin1String("x86-linux-generic-elf-32bit"));
    QTest::newRow("Linux 3 (64bit intel -- non 64bit)")
            << QString::fromLatin1("x86_64-linux-gnu")
            << QByteArray("#define __SIZEOF_SIZE_T__ 4\n")
            << (QStringList() << QLatin1String("x86-linux-generic-elf-32bit"));
    QTest::newRow("Linux 4 (32bit mips)")
            << QString::fromLatin1("mipsel-linux-uclibc")
            << QByteArray("#define __SIZEOF_SIZE_T__ 4")
            << (QStringList() << QLatin1String("mips-linux-generic-elf-32bit"));
    QTest::newRow("Linux 5 (QTCREATORBUG-4690)") // from QTCREATORBUG-4690
            << QString::fromLatin1("x86_64-redhat-linux6E")
            << QByteArray("#define __SIZEOF_SIZE_T__ 8\n")
            << (QStringList() << QLatin1String("x86-linux-generic-elf-64bit")
                              << QLatin1String("x86-linux-generic-elf-32bit"));
    QTest::newRow("Linux 6 (QTCREATORBUG-4690)") // from QTCREATORBUG-4690
            << QString::fromLatin1("x86_64-redhat-linux")
            << QByteArray("#define __SIZEOF_SIZE_T__ 8\n")
            << (QStringList() << QLatin1String("x86-linux-generic-elf-64bit")
                              << QLatin1String("x86-linux-generic-elf-32bit"));
    QTest::newRow("Linux 7 (arm)")
                << QString::fromLatin1("armv5tl-montavista-linux-gnueabi")
                << QByteArray("#define __SIZEOF_SIZE_T__ 4\n")
                << (QStringList() << QLatin1String("arm-linux-generic-elf-32bit"));
    QTest::newRow("Linux 8 (arm)")
                << QString::fromLatin1("arm-angstrom-linux-gnueabi")
                << QByteArray("#define __SIZEOF_SIZE_T__ 4\n")
                << (QStringList() << QLatin1String("arm-linux-generic-elf-32bit"));
    QTest::newRow("Linux 9 (ppc)")
                << QString::fromLatin1("powerpc-nsg-linux")
                << QByteArray("#define __SIZEOF_SIZE_T__ 4\n")
                << (QStringList() << QLatin1String("ppc-linux-generic-elf-32bit"));
    QTest::newRow("Linux 10 (ppc 64bit)")
                << QString::fromLatin1("powerpc64-suse-linux")
                << QByteArray("#define __SIZEOF_SIZE_T__ 8\n")
                << (QStringList() << QLatin1String("ppc-linux-generic-elf-64bit"));

    QTest::newRow("Mingw 1 (32bit)")
            << QString::fromLatin1("i686-w64-mingw32")
            << QByteArray("#define __SIZEOF_SIZE_T__ 4\r\n")
            << (QStringList() << QLatin1String("x86-windows-msys-pe-32bit"));
    QTest::newRow("Mingw 2 (64bit)")
            << QString::fromLatin1("i686-w64-mingw32")
            << QByteArray("#define __SIZEOF_SIZE_T__ 8\r\n")
            << (QStringList() << QLatin1String("x86-windows-msys-pe-64bit")
                              << QLatin1String("x86-windows-msys-pe-32bit"));
    QTest::newRow("Mingw 3 (32 bit)")
            << QString::fromLatin1("mingw32")
            << QByteArray("#define __SIZEOF_SIZE_T__ 4\r\n")
            << (QStringList() << QLatin1String("x86-windows-msys-pe-32bit"));
    QTest::newRow("Cross Mingw 1 (64bit)")
            << QString::fromLatin1("amd64-mingw32msvc")
            << QByteArray("#define __SIZEOF_SIZE_T__ 8\r\n")
            << (QStringList() << QLatin1String("x86-windows-msys-pe-64bit")
                              << QLatin1String("x86-windows-msys-pe-32bit"));
    QTest::newRow("Cross Mingw 2 (32bit)")
            << QString::fromLatin1("i586-mingw32msvc")
            << QByteArray("#define __SIZEOF_SIZE_T__ 4\r\n")
            << (QStringList() << QLatin1String("x86-windows-msys-pe-32bit"));
    QTest::newRow("Clang 1: windows")
            << QString::fromLatin1("x86_64-pc-win32")
            << QByteArray("#define __SIZEOF_SIZE_T__ 8\r\n")
            << (QStringList() << QLatin1String("x86-windows-msys-pe-64bit")
                              << QLatin1String("x86-windows-msys-pe-32bit"));
    QTest::newRow("Clang 1: linux")
            << QString::fromLatin1("x86_64-unknown-linux-gnu")
            << QByteArray("#define __SIZEOF_SIZE_T__ 8\n")
            << (QStringList() << QLatin1String("x86-linux-generic-elf-64bit")
                              << QLatin1String("x86-linux-generic-elf-32bit"));
    QTest::newRow("Mac 1")
            << QString::fromLatin1("i686-apple-darwin10")
            << QByteArray("#define __SIZEOF_SIZE_T__ 8\n")
            << (QStringList() << QLatin1String("x86-macos-generic-mach_o-64bit")
                              << QLatin1String("x86-macos-generic-mach_o-32bit"));
    QTest::newRow("Mac 2")
            << QString::fromLatin1("powerpc-apple-darwin10")
            << QByteArray("#define __SIZEOF_SIZE_T__ 8\n")
            << (QStringList() << QLatin1String("ppc-macos-generic-mach_o-64bit")
                              << QLatin1String("ppc-macos-generic-mach_o-32bit"));
    QTest::newRow("Mac 3")
            << QString::fromLatin1("i686-apple-darwin9")
            << QByteArray("#define __SIZEOF_SIZE_T__ 4\n")
            << (QStringList() << QLatin1String("x86-macos-generic-mach_o-32bit")
                              << QLatin1String("x86-macos-generic-mach_o-64bit"));
    QTest::newRow("Mac IOS")
            << QString::fromLatin1("arm-apple-darwin9")
            << QByteArray("#define __SIZEOF_SIZE_T__ 4\n")
            << (QStringList() << QLatin1String("arm-macos-generic-mach_o-32bit")
                              << QLatin1String("arm-macos-generic-mach_o-64bit"));
    QTest::newRow("Intel 1")
            << QString::fromLatin1("86_64 x86_64 GNU/Linux")
            << QByteArray("#define __SIZEOF_SIZE_T__ 8\n")
            << (QStringList() << QLatin1String("x86-linux-generic-elf-64bit")
                              << QLatin1String("x86-linux-generic-elf-32bit"));
    QTest::newRow("FreeBSD 1")
            << QString::fromLatin1("i386-portbld-freebsd9.0")
            << QByteArray("#define __SIZEOF_SIZE_T__ 4\n")
            << (QStringList() << QLatin1String("x86-bsd-freebsd-elf-32bit"));
    QTest::newRow("FreeBSD 2")
            << QString::fromLatin1("i386-undermydesk-freebsd")
            << QByteArray("#define __SIZEOF_SIZE_T__ 4\n")
            << (QStringList() << QLatin1String("x86-bsd-freebsd-elf-32bit"));
}

void ProjectExplorerPlugin::testGccAbiGuessing()
{
    QFETCH(QString, input);
    QFETCH(QByteArray, macros);
    QFETCH(QStringList, abiList);

    QList<Abi> al = guessGccAbi(input, macros);
    QCOMPARE(al.count(), abiList.count());
    for (int i = 0; i < al.count(); ++i)
        QCOMPARE(al.at(i).toString(), abiList.at(i));
}

} // namespace ProjectExplorer

#endif
