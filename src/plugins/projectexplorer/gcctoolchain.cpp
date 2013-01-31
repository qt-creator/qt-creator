/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
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

#include <utils/detailswidget.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/synchronousprocess.h>
#include <utils/qtcassert.h>
#include <utils/pathchooser.h>

#include <QBuffer>
#include <QCoreApplication>
#include <QFileInfo>
#include <QProcess>
#include <QScopedPointer>

#include <QComboBox>
#include <QFormLayout>
#include <QLabel>

using namespace Utils;

namespace ProjectExplorer {

// --------------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------------

static const char compilerCommandKeyC[] = "ProjectExplorer.GccToolChain.Path";
static const char targetAbiKeyC[] = "ProjectExplorer.GccToolChain.TargetAbi";
static const char supportedAbisKeyC[] = "ProjectExplorer.GccToolChain.SupportedAbis";

static const char LEGACY_MAEMO_ID[] = "Qt4ProjectManager.ToolChain.Maemo:";

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
    if (!cpp.waitForFinished()) {
        SynchronousProcess::stopProcess(cpp);
        qWarning("%s: Timeout running '%s'.", Q_FUNC_INFO, qPrintable(gcc.toUserOutput()));
        return QByteArray();
    }
    if (cpp.exitStatus() != QProcess::NormalExit) {
        qWarning("%s: '%s' crashed.", Q_FUNC_INFO, qPrintable(gcc.toUserOutput()));
        return QByteArray();
    }

    return cpp.readAllStandardOutput() + '\n' + cpp.readAllStandardError();
}

static QByteArray gccPredefinedMacros(const FileName &gcc, const QStringList &args, const QStringList &env)
{
    QStringList arguments;
    arguments << QLatin1String("-xc++")
              << QLatin1String("-E")
              << QLatin1String("-dM");
    foreach (const QString &a, args) {
        if (a == QLatin1String("-m128bit-long-double") || a == QLatin1String("-m32")
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
                || a.startsWith(QLatin1String("-std=")) || a.startsWith(QLatin1String("-stdlib="))
                || a.startsWith(QLatin1String("-specs="))
                || a == QLatin1String("-ansi")
                || a.startsWith(QLatin1String("-D")) || a.startsWith(QLatin1String("-U"))
                || a == QLatin1String("-undef"))
            arguments << a;
    }

    arguments << QLatin1String("-");

    QByteArray predefinedMacros = runGcc(gcc, arguments, env);
    if (Utils::HostOsInfo::isMacHost()) {
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

QList<HeaderPath> GccToolChain::gccHeaderPaths(const FileName &gcc, const QStringList &args,
                                               const QStringList &env, const FileName &sysrootPath)
{
    QList<HeaderPath> systemHeaderPaths;
    QStringList arguments;
    if (!sysrootPath.isEmpty())
        arguments.append(QString::fromLatin1("--sysroot=%1").arg(sysrootPath.toString()));
    foreach (const QString &a, args) {
        if (a.startsWith(QLatin1String("-stdlib=")))
            arguments << a;
    }

    arguments << QLatin1String("-xc++")
              << QLatin1String("-E")
              << QLatin1String("-v")
              << QLatin1String("-");

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

static QList<Abi> guessGccAbi(const QString &m)
{
    QList<Abi> abiList;

    QString machine = m.toLower();
    if (machine.isEmpty())
        return abiList;

    QStringList parts = machine.split(QRegExp(QLatin1String("[ /-]")));

    Abi::Architecture arch = Abi::UnknownArchitecture;
    Abi::OS os = Abi::UnknownOS;
    Abi::OSFlavor flavor = Abi::UnknownFlavor;
    Abi::BinaryFormat format = Abi::UnknownFormat;
    int width = 0;
    int unknownCount = 0;

    foreach (const QString &p, parts) {
        if (p == QLatin1String("unknown") || p == QLatin1String("pc") || p == QLatin1String("none")
            || p == QLatin1String("gnu") || p == QLatin1String("uclibc")
            || p == QLatin1String("86_64") || p == QLatin1String("redhat") || p == QLatin1String("gnueabi")) {
            continue;
        } else if (p == QLatin1String("i386") || p == QLatin1String("i486") || p == QLatin1String("i586")
                   || p == QLatin1String("i686") || p == QLatin1String("x86")) {
            arch = Abi::X86Architecture;
            width = 32;
        } else if (p.startsWith(QLatin1String("arm"))) {
            arch = Abi::ArmArchitecture;
            width = 32;
        } else if (p == QLatin1String("mipsel")) {
            arch = Abi::MipsArchitecture;
            width = 32;
        } else if (p == QLatin1String("x86_64") || p == QLatin1String("amd64")) {
            arch = Abi::X86Architecture;
            width = 64;
        } else if (p == QLatin1String("powerpc")) {
            arch = Abi::PowerPCArchitecture;
        } else if (p == QLatin1String("w64")) {
            width = 64;
        } else if (p == QLatin1String("linux") || p == QLatin1String("linux6e")) {
            os = Abi::LinuxOS;
            if (flavor == Abi::UnknownFlavor)
                flavor = Abi::GenericLinuxFlavor;
            format = Abi::ElfFormat;
        } else if (p.startsWith(QLatin1String("freebsd"))) {
            os = Abi::BsdOS;
            if (flavor == Abi::UnknownFlavor)
                flavor = Abi::FreeBsdFlavor;
            format = Abi::ElfFormat;
        } else if (p == QLatin1String("mingw32") || p == QLatin1String("win32") || p == QLatin1String("mingw32msvc")) {
            arch = Abi::X86Architecture;
            os = Abi::WindowsOS;
            flavor = Abi::WindowsMSysFlavor;
            format = Abi::PEFormat;
            if (width == 0)
                width = 32;
        } else if (p == QLatin1String("apple")) {
            os = Abi::MacOS;
            flavor = Abi::GenericMacFlavor;
            format = Abi::MachOFormat;
        } else if (p == QLatin1String("darwin10")) {
            width = 64;
        } else if (p == QLatin1String("darwin9")) {
            width = 32;
        } else if (p == QLatin1String("gnueabi")) {
            format = Abi::ElfFormat;
        } else {
            ++unknownCount;
        }
    }

    if (unknownCount == parts.count())
        return abiList;

    if (os == Abi::MacOS && arch != Abi::ArmArchitecture) {
        // Apple does PPC and x86!
        abiList << Abi(arch, os, flavor, format, width);
        abiList << Abi(arch, os, flavor, format, width == 64 ? 32 : 64);
        abiList << Abi(arch == Abi::X86Architecture ? Abi::PowerPCArchitecture : Abi::X86Architecture, os, flavor, format, width);
        abiList << Abi(arch == Abi::X86Architecture ? Abi::PowerPCArchitecture : Abi::X86Architecture, os, flavor, format, width == 64 ? 32 : 64);
    } else if (width == 64) {
        abiList << Abi(arch, os, flavor, format, width);
        abiList << Abi(arch, os, flavor, format, 32);
    } else {
        abiList << Abi(arch, os, flavor, format, width);
    }
    return abiList;
}

static QList<Abi> guessGccAbi(const FileName &path, const QStringList &env)
{
    if (path.isEmpty())
        return QList<Abi>();

    QStringList arguments(QLatin1String("-dumpmachine"));
    QString machine = QString::fromLocal8Bit(runGcc(path, arguments, env)).trimmed();
    return guessGccAbi(machine);
}

static QString gccVersion(const FileName &path, const QStringList &env)
{
    QStringList arguments(QLatin1String("-dumpversion"));
    return QString::fromLocal8Bit(runGcc(path, arguments, env)).trimmed();
}

// --------------------------------------------------------------------------
// GccToolChain
// --------------------------------------------------------------------------

GccToolChain::GccToolChain(bool autodetect) :
    ToolChain(QLatin1String(Constants::GCC_TOOLCHAIN_ID), autodetect)
{ }

GccToolChain::GccToolChain(const QString &id, bool autodetect) :
    ToolChain(id, autodetect)
{ }

GccToolChain::GccToolChain(const GccToolChain &tc) :
    ToolChain(tc),
    m_predefinedMacros(tc.m_predefinedMacros),
    m_compilerCommand(tc.compilerCommand()),
    m_targetAbi(tc.m_targetAbi),
    m_supportedAbis(tc.m_supportedAbis),
    m_headerPaths(tc.m_headerPaths),
    m_version(tc.m_version)
{ }

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

QString GccToolChain::type() const
{
    return QLatin1String("gcc");
}

QString GccToolChain::typeDisplayName() const
{
    return Internal::GccToolChainFactory::tr("GCC");
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
    typedef QPair<QStringList, QByteArray> CacheItem;

    for (GccCache::iterator it = m_predefinedMacros.begin(); it != m_predefinedMacros.end(); ++it)
        if (it->first == cxxflags) {
            // Increase cached item priority
            CacheItem pair = *it;
            m_predefinedMacros.erase(it);
            m_predefinedMacros.push_back(pair);

            return pair.second;
        }

    CacheItem runResults;
    runResults.first = cxxflags;

    // Using a clean environment breaks ccache/distcc/etc.
    Environment env = Environment::systemEnvironment();
    addToEnvironment(env);
    runResults.second = gccPredefinedMacros(m_compilerCommand, cxxflags, env.toStringList());

    m_predefinedMacros.push_back(runResults);
    if (m_predefinedMacros.size() > PREDEFINED_MACROS_CACHE_SIZE)
        m_predefinedMacros.pop_front();

    return runResults.second;
}

ToolChain::CompilerFlags GccToolChain::compilerFlags(const QStringList &cxxflags) const
{
    if (cxxflags.contains(QLatin1String("-std=c++0x")) || cxxflags.contains(QLatin1String("-std=gnu++0x")) ||
        cxxflags.contains(QLatin1String("-std=c++11")) || cxxflags.contains(QLatin1String("-std=gnu++11")) ||
        cxxflags.contains(QLatin1String("-std=c++1y")) || cxxflags.contains(QLatin1String("-std=gnu++1y")))
        return STD_CXX11;
    return NO_FLAGS;
}

QList<HeaderPath> GccToolChain::systemHeaderPaths(const QStringList &cxxflags, const Utils::FileName &sysRoot) const
{
    if (m_headerPaths.isEmpty()) {
        // Using a clean environment breaks ccache/distcc/etc.
        Environment env = Environment::systemEnvironment();
        addToEnvironment(env);
        m_headerPaths = gccHeaderPaths(m_compilerCommand, cxxflags, env.toStringList(), sysRoot);
    }
    return m_headerPaths;
}

void GccToolChain::addToEnvironment(Environment &env) const
{
    if (!m_compilerCommand.isEmpty()) {
        FileName path = m_compilerCommand.parentDir();
        env.prependOrSetPath(path.toString());
    }
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
            return QList<FileName>() << FileName::fromString(QLatin1String("macx-g++40"));
        if (v.startsWith(QLatin1String("4.2")) && m_compilerCommand.endsWith(QLatin1String("-4.2")))
            return QList<FileName>() << FileName::fromString(QLatin1String("macx-g++42"));
        return QList<FileName>() << FileName::fromString(QLatin1String("macx-g++"));
    }

    if (abi.os() == Abi::LinuxOS) {
        if (abi.osFlavor() != Abi::GenericLinuxFlavor)
            return QList<FileName>(); // most likely not a desktop, so leave the mkspec alone.
        if (abi.wordWidth() == host.wordWidth()) {
            // no need to explicitly set the word width, but provide that mkspec anyway to make sure
            // that the correct compiler is picked if a mkspec with a wordwidth is given.
            return QList<FileName>() << FileName::fromString(QLatin1String("linux-g++"))
                                            << FileName::fromString(QLatin1String("linux-g++-") + QString::number(m_targetAbi.wordWidth()));
        }
        return QList<FileName>() << FileName::fromString(QLatin1String("linux-g++-") + QString::number(m_targetAbi.wordWidth()));
    }

    if (abi.os() == Abi::BsdOS && abi.osFlavor() == Abi::FreeBsdFlavor)
        return QList<FileName>() << FileName::fromString(QLatin1String("freebsd-g++"));

    return QList<FileName>();
}

QString GccToolChain::makeCommand(const Utils::Environment &environment) const
{
    QString make = QLatin1String("make");
    QString tmp = environment.searchInPath(make);
    return tmp.isEmpty() ? make : tmp;
}

IOutputParser *GccToolChain::outputParser() const
{
    return new GccParser;
}

void GccToolChain::setCompilerCommand(const FileName &path)
{
    if (path == m_compilerCommand)
        return;

    bool resetDisplayName = displayName() == defaultDisplayName();

    m_compilerCommand = path;

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

ToolChain *GccToolChain::clone() const
{
    return new GccToolChain(*this);
}

QVariantMap GccToolChain::toMap() const
{
    QVariantMap data = ToolChain::toMap();
    data.insert(QLatin1String(compilerCommandKeyC), m_compilerCommand.toString());
    data.insert(QLatin1String(targetAbiKeyC), m_targetAbi.toString());
    QStringList abiList;
    foreach (const Abi &a, m_supportedAbis)
        abiList.append(a.toString());
    data.insert(QLatin1String(supportedAbisKeyC), abiList);
    return data;
}

bool GccToolChain::fromMap(const QVariantMap &data)
{
    if (!ToolChain::fromMap(data))
        return false;

    m_compilerCommand = FileName::fromString(data.value(QLatin1String(compilerCommandKeyC)).toString());
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
    return m_compilerCommand == gccTc->m_compilerCommand && m_targetAbi == gccTc->m_targetAbi;
}

ToolChainConfigWidget *GccToolChain::configurationWidget()
{
    return new Internal::GccToolChainConfigWidget(this);
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
    return guessGccAbi(m_compilerCommand, env.toStringList());
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

QString Internal::GccToolChainFactory::displayName() const
{
    return tr("GCC");
}

QString Internal::GccToolChainFactory::id() const
{
    return QLatin1String(Constants::GCC_TOOLCHAIN_ID);
}

bool Internal::GccToolChainFactory::canCreate()
{
    return true;
}

ToolChain *Internal::GccToolChainFactory::create()
{
    return createToolChain(false);
}

QList<ToolChain *> Internal::GccToolChainFactory::autoDetect()
{
    QList<ToolChain *> tcs;
    if (Utils::HostOsInfo::isMacHost()) {
        // Old mac compilers needed to support macx-gccXY mkspecs:
        tcs.append(autoDetectToolchains(QLatin1String("g++-4.0"), Abi::hostAbi()));
        tcs.append(autoDetectToolchains(QLatin1String("g++-4.2"), Abi::hostAbi()));
    }
    tcs.append(autoDetectToolchains(QLatin1String("g++"), Abi::hostAbi()));

    return tcs;
}

// Used by the ToolChainManager to restore user-generated tool chains
bool Internal::GccToolChainFactory::canRestore(const QVariantMap &data)
{
    const QString id = idFromMap(data);
    return id.startsWith(QLatin1String(Constants::GCC_TOOLCHAIN_ID) + QLatin1Char(':'))
            || id.startsWith(QLatin1String(LEGACY_MAEMO_ID));
}

ToolChain *Internal::GccToolChainFactory::restore(const QVariantMap &data)
{
    GccToolChain *tc = new GccToolChain(false);
    // Updating from 2.5:
    QVariantMap updated = data;
    QString id = idFromMap(updated);
    if (id.startsWith(QLatin1String(LEGACY_MAEMO_ID))) {
        id = QString::fromLatin1(Constants::GCC_TOOLCHAIN_ID).append(id.mid(id.indexOf(QLatin1Char(':'))));
        idToMap(updated, id);
        autoDetectionToMap(updated, false);
    }
    if (tc->fromMap(updated))
        return tc;

    delete tc;
    return 0;
}

GccToolChain *Internal::GccToolChainFactory::createToolChain(bool autoDetect)
{
    return new GccToolChain(autoDetect);
}

QList<ToolChain *> Internal::GccToolChainFactory::autoDetectToolchains(const QString &compiler,
                                                                       const Abi &requiredAbi)
{
    QList<ToolChain *> result;

    const Environment systemEnvironment = Environment::systemEnvironment();
    const FileName compilerPath = FileName::fromString(systemEnvironment.searchInPath(compiler));
    if (compilerPath.isEmpty())
        return result;

    QList<Abi> abiList = guessGccAbi(compilerPath, systemEnvironment.toStringList());
    if (!abiList.contains(requiredAbi)) {
        if (requiredAbi.wordWidth() != 64
                || !abiList.contains(Abi(requiredAbi.architecture(), requiredAbi.os(), requiredAbi.osFlavor(),
                                         requiredAbi.binaryFormat(), 32)))
            return result;
    }

    foreach (const Abi &abi, abiList) {
        QScopedPointer<GccToolChain> tc(createToolChain(true));
        if (tc.isNull())
            return result;

        tc->setCompilerCommand(compilerPath);
        tc->setTargetAbi(abi);
        tc->setDisplayName(tc->defaultDisplayName()); // reset displayname

        result.append(tc.take());
    }

    return result;
}

// --------------------------------------------------------------------------
// GccToolChainConfigWidget
// --------------------------------------------------------------------------

Internal::GccToolChainConfigWidget::GccToolChainConfigWidget(GccToolChain *tc) :
    ToolChainConfigWidget(tc),
    m_compilerCommand(new PathChooser),
    m_abiWidget(new AbiWidget),
    m_isReadOnly(false)
{
    Q_ASSERT(tc);

    const QStringList gnuVersionArgs = QStringList(QLatin1String("--version"));
    m_compilerCommand->setExpectedKind(PathChooser::ExistingCommand);
    m_compilerCommand->setCommandVersionArguments(gnuVersionArgs);
    m_mainLayout->addRow(tr("&Compiler path:"), m_compilerCommand);
    m_mainLayout->addRow(tr("&ABI:"), m_abiWidget);
    m_abiWidget->setEnabled(false);
    addErrorLabel();

    setFromToolchain();

    connect(m_compilerCommand, SIGNAL(changed(QString)), this, SLOT(handleCompilerCommandChange()));
    connect(m_abiWidget, SIGNAL(abiChanged()), this, SIGNAL(dirty()));
}

void Internal::GccToolChainConfigWidget::applyImpl()
{
    if (toolChain()->isAutoDetected())
        return;

    GccToolChain *tc = static_cast<GccToolChain *>(toolChain());
    Q_ASSERT(tc);
    QString displayName = tc->displayName();
    tc->setCompilerCommand(m_compilerCommand->fileName());
    tc->setTargetAbi(m_abiWidget->currentAbi());
    tc->setDisplayName(displayName); // reset display name
}

void Internal::GccToolChainConfigWidget::setFromToolchain()
{
    // subwidgets are not yet connected!
    bool blocked = blockSignals(true);
    GccToolChain *tc = static_cast<GccToolChain *>(toolChain());
    m_compilerCommand->setFileName(tc->compilerCommand());
    m_abiWidget->setAbis(tc->supportedAbis(), tc->targetAbi());
    if (!m_isReadOnly && !m_compilerCommand->path().isEmpty())
        m_abiWidget->setEnabled(true);
    blockSignals(blocked);
}

bool Internal::GccToolChainConfigWidget::isDirtyImpl() const
{
    GccToolChain *tc = static_cast<GccToolChain *>(toolChain());
    Q_ASSERT(tc);
    return m_compilerCommand->fileName() != tc->compilerCommand()
            || m_abiWidget->currentAbi() != tc->targetAbi();
}

void Internal::GccToolChainConfigWidget::makeReadOnlyImpl()
{
    m_compilerCommand->setEnabled(false);
    m_abiWidget->setEnabled(false);
    m_isReadOnly = true;
}

void Internal::GccToolChainConfigWidget::handleCompilerCommandChange()
{
    FileName path = m_compilerCommand->fileName();
    QList<Abi> abiList;
    bool haveCompiler = false;
    if (!path.isEmpty()) {
        QFileInfo fi(path.toFileInfo());
        haveCompiler = fi.isExecutable() && fi.isFile();
    }
    if (haveCompiler)
        abiList = guessGccAbi(path, Environment::systemEnvironment().toStringList());
    m_abiWidget->setEnabled(haveCompiler);
    Abi currentAbi = m_abiWidget->currentAbi();
    m_abiWidget->setAbis(abiList, abiList.contains(currentAbi) ? currentAbi : Abi());
    emit dirty();
}

// --------------------------------------------------------------------------
// ClangToolChain
// --------------------------------------------------------------------------

ClangToolChain::ClangToolChain(bool autodetect) :
    GccToolChain(QLatin1String(Constants::CLANG_TOOLCHAIN_ID), autodetect)
{ }

QString ClangToolChain::type() const
{
    return QLatin1String("clang");
}

QString ClangToolChain::typeDisplayName() const
{
    return Internal::ClangToolChainFactory::tr("Clang");
}

QString ClangToolChain::makeCommand(const Utils::Environment &environment) const
{
    QStringList makes;
    if (Utils::HostOsInfo::isWindowsHost()) {
        makes << QLatin1String("mingw32-make.exe");
        makes << QLatin1String("make.exe");
    } else {
        makes << QLatin1String("make");
    }

    QString tmp;
    foreach (const QString &make, makes) {
        tmp = environment.searchInPath(make);
        if (!tmp.isEmpty())
            return tmp;
    }
    return makes.first();
}

QList<FileName> ClangToolChain::suggestedMkspecList() const
{
    Abi abi = targetAbi();
    if (abi.os() == Abi::MacOS)
        return QList<FileName>()
                << FileName::fromString(QLatin1String("macx-clang"))
                << FileName::fromString(QLatin1String("unsupported/macx-clang"));
    else if (abi.os() == Abi::LinuxOS)
        return QList<FileName>()
                << FileName::fromString(QLatin1String("linux-clang"))
                << FileName::fromString(QLatin1String("unsupported/linux-clang"));
    return QList<FileName>(); // Note: Not supported by Qt yet, so default to the mkspec the Qt was build with
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

QString Internal::ClangToolChainFactory::displayName() const
{
    return tr("Clang");
}

QString Internal::ClangToolChainFactory::id() const
{
    return QLatin1String(Constants::CLANG_TOOLCHAIN_ID);
}

QList<ToolChain *> Internal::ClangToolChainFactory::autoDetect()
{
    Abi ha = Abi::hostAbi();
    return autoDetectToolchains(QLatin1String("clang++"), ha);
}

bool Internal::ClangToolChainFactory::canCreate()
{
    return true;
}

ToolChain *Internal::ClangToolChainFactory::create()
{
    return createToolChain(false);
}

bool Internal::ClangToolChainFactory::canRestore(const QVariantMap &data)
{
    return idFromMap(data).startsWith(QLatin1String(Constants::CLANG_TOOLCHAIN_ID) + QLatin1Char(':'));
}

ToolChain *Internal::ClangToolChainFactory::restore(const QVariantMap &data)
{
    ClangToolChain *tc = new ClangToolChain(false);
    if (tc->fromMap(data))
        return tc;

    delete tc;
    return 0;
}

GccToolChain *Internal::ClangToolChainFactory::createToolChain(bool autoDetect)
{
    return new ClangToolChain(autoDetect);
}

// --------------------------------------------------------------------------
// MingwToolChain
// --------------------------------------------------------------------------

MingwToolChain::MingwToolChain(bool autodetect) :
    GccToolChain(QLatin1String(Constants::MINGW_TOOLCHAIN_ID), autodetect)
{ }

QString MingwToolChain::type() const
{
    return QLatin1String("mingw");
}

QString MingwToolChain::typeDisplayName() const
{
    return Internal::MingwToolChainFactory::tr("MinGW");
}

QList<FileName> MingwToolChain::suggestedMkspecList() const
{
    if (Utils::HostOsInfo::isWindowsHost())
        return QList<FileName>() << FileName::fromString(QLatin1String("win32-g++"));
    if (Utils::HostOsInfo::isLinuxHost()) {
        if (version().startsWith(QLatin1String("4.6.")))
            return QList<FileName>()
                    << FileName::fromString(QLatin1String("win32-g++-4.6-cross"))
                    << FileName::fromString(QLatin1String("unsupported/win32-g++-4.6-cross"));
        else
            return QList<FileName>()
                    << FileName::fromString(QLatin1String("win32-g++-cross"))
                    << FileName::fromString(QLatin1String("unsupported/win32-g++-cross"));
    }
    return QList<FileName>();
}

QString MingwToolChain::makeCommand(const Utils::Environment &environment) const
{
    QStringList makes;
    if (Utils::HostOsInfo::isWindowsHost()) {
        makes << QLatin1String("mingw32-make.exe");
        makes << QLatin1String("make.exe");
    } else {
        makes << QLatin1String("make");
    }

    QString tmp;
    foreach (const QString &make, makes) {
        tmp = environment.searchInPath(make);
        if (!tmp.isEmpty())
            return tmp;
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

QString Internal::MingwToolChainFactory::displayName() const
{
    return tr("MinGW");
}

QString Internal::MingwToolChainFactory::id() const
{
    return QLatin1String(Constants::MINGW_TOOLCHAIN_ID);
}

QList<ToolChain *> Internal::MingwToolChainFactory::autoDetect()
{
    Abi ha = Abi::hostAbi();
    return autoDetectToolchains(QLatin1String("g++"),
                                Abi(ha.architecture(), Abi::WindowsOS, Abi::WindowsMSysFlavor, Abi::PEFormat, ha.wordWidth()));
}

bool Internal::MingwToolChainFactory::canCreate()
{
    return true;
}

ToolChain *Internal::MingwToolChainFactory::create()
{
    return createToolChain(false);
}

bool Internal::MingwToolChainFactory::canRestore(const QVariantMap &data)
{
    return idFromMap(data).startsWith(QLatin1String(Constants::MINGW_TOOLCHAIN_ID) + QLatin1Char(':'));
}

ToolChain *Internal::MingwToolChainFactory::restore(const QVariantMap &data)
{
    MingwToolChain *tc = new MingwToolChain(false);
    if (tc->fromMap(data))
        return tc;

    delete tc;
    return 0;
}

GccToolChain *Internal::MingwToolChainFactory::createToolChain(bool autoDetect)
{
    return new MingwToolChain(autoDetect);
}

// --------------------------------------------------------------------------
// LinuxIccToolChain
// --------------------------------------------------------------------------

LinuxIccToolChain::LinuxIccToolChain(bool autodetect) :
    GccToolChain(QLatin1String(Constants::LINUXICC_TOOLCHAIN_ID), autodetect)
{ }

QString LinuxIccToolChain::type() const
{
    return QLatin1String("icc");
}

QString LinuxIccToolChain::typeDisplayName() const
{
    return Internal::LinuxIccToolChainFactory::tr("Linux ICC");
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

QString Internal::LinuxIccToolChainFactory::displayName() const
{
    return tr("Linux ICC");
}

QString Internal::LinuxIccToolChainFactory::id() const
{
    return QLatin1String(Constants::LINUXICC_TOOLCHAIN_ID);
}

QList<ToolChain *> Internal::LinuxIccToolChainFactory::autoDetect()
{
    return autoDetectToolchains(QLatin1String("icpc"), Abi::hostAbi());
}

ToolChain *Internal::LinuxIccToolChainFactory::create()
{
    return createToolChain(false);
}

bool Internal::LinuxIccToolChainFactory::canRestore(const QVariantMap &data)
{
    return idFromMap(data).startsWith(QLatin1String(Constants::LINUXICC_TOOLCHAIN_ID) + QLatin1Char(':'));
}

ToolChain *Internal::LinuxIccToolChainFactory::restore(const QVariantMap &data)
{
    LinuxIccToolChain *tc = new LinuxIccToolChain(false);
    if (tc->fromMap(data))
        return tc;

    delete tc;
    return 0;
}

GccToolChain *Internal::LinuxIccToolChainFactory::createToolChain(bool autoDetect)
{
    return new LinuxIccToolChain(autoDetect);
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
    QTest::addColumn<QStringList>("abiList");

    QTest::newRow("invalid input")
            << QString::fromLatin1("Some text")
            << (QStringList());
    QTest::newRow("empty input")
            << QString::fromLatin1("")
            << (QStringList());
    QTest::newRow("broken input")
            << QString::fromLatin1("arm-none-foo-gnueabi")
            << (QStringList() << QLatin1String("arm-unknown-unknown-unknown-32bit"));
    QTest::newRow("totally broken input")
            << QString::fromLatin1("foo-bar-foo")
            << (QStringList());

    QTest::newRow("Maemo 1")
            << QString::fromLatin1("arm-none-linux-gnueabi")
            << (QStringList() << QLatin1String("arm-linux-generic-elf-32bit"));
    QTest::newRow("Linux 1")
            << QString::fromLatin1("i686-linux-gnu")
            << (QStringList() << QLatin1String("x86-linux-generic-elf-32bit"));
    QTest::newRow("Linux 2")
            << QString::fromLatin1("i486-linux-gnu")
            << (QStringList() << QLatin1String("x86-linux-generic-elf-32bit"));
    QTest::newRow("Linux 3")
            << QString::fromLatin1("x86_64-linux-gnu")
            << (QStringList() << QLatin1String("x86-linux-generic-elf-64bit")
                              << QLatin1String("x86-linux-generic-elf-32bit"));
    QTest::newRow("Linux 4")
            << QString::fromLatin1("mipsel-linux-uclibc")
            << (QStringList() << QLatin1String("mips-linux-generic-elf-32bit"));
    QTest::newRow("Linux 5") // from QTCREATORBUG-4690
            << QString::fromLatin1("x86_64-redhat-linux6E")
            << (QStringList() << QLatin1String("x86-linux-generic-elf-64bit")
                              << QLatin1String("x86-linux-generic-elf-32bit"));
    QTest::newRow("Linux 6") // from QTCREATORBUG-4690
            << QString::fromLatin1("x86_64-redhat-linux")
            << (QStringList() << QLatin1String("x86-linux-generic-elf-64bit")
                              << QLatin1String("x86-linux-generic-elf-32bit"));
    QTest::newRow("Linux 7")
                << QString::fromLatin1("armv5tl-montavista-linux-gnueabi")
                << (QStringList() << QLatin1String("arm-linux-generic-elf-32bit"));
    QTest::newRow("Linux 8")
                << QString::fromLatin1("arm-angstrom-linux-gnueabi")
                << (QStringList() << QLatin1String("arm-linux-generic-elf-32bit"));

    QTest::newRow("Mingw 1")
            << QString::fromLatin1("i686-w64-mingw32")
            << (QStringList() << QLatin1String("x86-windows-msys-pe-64bit")
                              << QLatin1String("x86-windows-msys-pe-32bit"));
    QTest::newRow("Mingw 2")
            << QString::fromLatin1("mingw32")
            << (QStringList() << QLatin1String("x86-windows-msys-pe-32bit"));
    QTest::newRow("Cross Mingw 1")
            << QString::fromLatin1("amd64-mingw32msvc")
            << (QStringList() << QLatin1String("x86-windows-msys-pe-64bit")
                              << QLatin1String("x86-windows-msys-pe-32bit"));
    QTest::newRow("Cross Mingw 2")
            << QString::fromLatin1("i586-mingw32msvc")
            << (QStringList() << QLatin1String("x86-windows-msys-pe-32bit"));
    QTest::newRow("Clang 1: windows")
            << QString::fromLatin1("x86_64-pc-win32")
            << (QStringList() << QLatin1String("x86-windows-msys-pe-64bit")
                              << QLatin1String("x86-windows-msys-pe-32bit"));
    QTest::newRow("Clang 1: linux")
            << QString::fromLatin1("x86_64-unknown-linux-gnu")
            << (QStringList() << QLatin1String("x86-linux-generic-elf-64bit")
                              << QLatin1String("x86-linux-generic-elf-32bit"));
    QTest::newRow("Mac 1")
            << QString::fromLatin1("i686-apple-darwin10")
            << (QStringList() << QLatin1String("x86-macos-generic-mach_o-64bit")
                              << QLatin1String("x86-macos-generic-mach_o-32bit")
                              << QLatin1String("ppc-macos-generic-mach_o-64bit")
                              << QLatin1String("ppc-macos-generic-mach_o-32bit"));
    QTest::newRow("Mac 2")
            << QString::fromLatin1("powerpc-apple-darwin10")
            << (QStringList() << QLatin1String("ppc-macos-generic-mach_o-64bit")
                              << QLatin1String("ppc-macos-generic-mach_o-32bit")
                              << QLatin1String("x86-macos-generic-mach_o-64bit")
                              << QLatin1String("x86-macos-generic-mach_o-32bit"));
    QTest::newRow("Mac 3")
            << QString::fromLatin1("i686-apple-darwin9")
            << (QStringList() << QLatin1String("x86-macos-generic-mach_o-32bit")
                              << QLatin1String("x86-macos-generic-mach_o-64bit")
                              << QLatin1String("ppc-macos-generic-mach_o-32bit")
                              << QLatin1String("ppc-macos-generic-mach_o-64bit"));
    QTest::newRow("Mac IOS")
            << QString::fromLatin1("arm-apple-darwin9")
            << (QStringList() << QLatin1String("arm-macos-generic-mach_o-32bit"));
    QTest::newRow("Intel 1")
            << QString::fromLatin1("86_64 x86_64 GNU/Linux")
            << (QStringList() << QLatin1String("x86-linux-generic-elf-64bit")
                              << QLatin1String("x86-linux-generic-elf-32bit"));
    QTest::newRow("FreeBSD 1")
            << QString::fromLatin1("i386-portbld-freebsd9.0")
            << (QStringList() << QLatin1String("x86-bsd-freebsd-elf-32bit"));
    QTest::newRow("FreeBSD 2")
            << QString::fromLatin1("i386-undermydesk-freebsd")
            << (QStringList() << QLatin1String("x86-bsd-freebsd-elf-32bit"));
}

void ProjectExplorerPlugin::testGccAbiGuessing()
{
    QFETCH(QString, input);
    QFETCH(QStringList, abiList);

    QList<Abi> al = guessGccAbi(input);
    QCOMPARE(al.count(), abiList.count());
    for (int i = 0; i < al.count(); ++i) {
        QCOMPARE(al.at(i).toString(), abiList.at(i));
    }
}

} // namespace ProjectExplorer

#endif
