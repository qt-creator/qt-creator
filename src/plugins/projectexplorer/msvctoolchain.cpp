/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "msvctoolchain.h"

#include "gcctoolchain.h"
#include "msvcparser.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "taskhub.h"
#include "toolchainmanager.h"

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/optional.h>
#include <utils/qtcassert.h>
#include <utils/synchronousprocess.h>
#include <utils/runextensions.h>
#include <utils/temporarydirectory.h>
#include <utils/pathchooser.h>
#include <utils/winutils.h>

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QVector>
#include <QVersionNumber>

#include <QLabel>
#include <QFormLayout>

#define KEY_ROOT "ProjectExplorer.MsvcToolChain."
static const char varsBatKeyC[] = KEY_ROOT"VarsBat";
static const char varsBatArgKeyC[] = KEY_ROOT"VarsBatArg";
static const char supportedAbiKeyC[] = KEY_ROOT"SupportedAbi";
static const char environModsKeyC[] = KEY_ROOT"environmentModifications";

enum { debug = 0 };

namespace ProjectExplorer {
namespace Internal {

// --------------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------------

static QThreadPool *envModThreadPool()
{
    static QThreadPool *pool = nullptr;
    if (!pool) {
        pool = new QThreadPool(ProjectExplorerPlugin::instance());
        pool->setMaxThreadCount(1);
    }
    return pool;
}

struct MsvcPlatform {
    MsvcToolChain::Platform platform;
    const char *name;
    const char *prefix; // VS up until 14.0 (MSVC2015)
    const char *bat;
};

const MsvcPlatform platforms[] =
{
    {MsvcToolChain::x86, "x86", "/bin", "vcvars32.bat"},
    {MsvcToolChain::amd64, "amd64", "/bin/amd64", "vcvars64.bat"},
    {MsvcToolChain::x86_amd64, "x86_amd64", "/bin/x86_amd64", "vcvarsx86_amd64.bat"},
    {MsvcToolChain::ia64, "ia64", "/bin/ia64", "vcvars64.bat"},
    {MsvcToolChain::x86_ia64, "x86_ia64", "/bin/x86_ia64", "vcvarsx86_ia64.bat"},
    {MsvcToolChain::arm, "arm", "/bin/arm", "vcvarsarm.bat"},
    {MsvcToolChain::x86_arm, "x86_arm", "/bin/x86_arm", "vcvarsx86_arm.bat"},
    {MsvcToolChain::amd64_arm, "amd64_arm", "/bin/amd64_arm", "vcvarsamd64_arm.bat"},
    {MsvcToolChain::amd64_x86, "amd64_x86", "/bin/amd64_x86", "vcvarsamd64_x86.bat"}
};

static QList<const MsvcToolChain *> g_availableMsvcToolchains;

static const MsvcPlatform *platformEntry(MsvcToolChain::Platform t)
{
    for (const MsvcPlatform &p : platforms) {
        if (p.platform == t)
            return &p;
    }
    return nullptr;
}

static QString platformName(MsvcToolChain::Platform t)
{
    if (const MsvcPlatform *p = platformEntry(t))
        return QLatin1String(p->name);
    return QString();
}

static bool hostSupportsPlatform(MsvcToolChain::Platform platform)
{
    switch (Utils::HostOsInfo::hostArchitecture()) {
    case Utils::HostOsInfo::HostArchitectureAMD64:
        if (platform == MsvcToolChain::amd64 || platform == MsvcToolChain::amd64_arm
            || platform == MsvcToolChain::amd64_x86)
            return true;
        Q_FALLTHROUGH(); // all x86 toolchains are also working on an amd64 host
    case Utils::HostOsInfo::HostArchitectureX86:
        return platform == MsvcToolChain::x86 || platform == MsvcToolChain::x86_amd64
                || platform == MsvcToolChain::x86_ia64 || platform == MsvcToolChain::x86_arm;
    case Utils::HostOsInfo::HostArchitectureArm:
        return platform == MsvcToolChain::arm;
    case Utils::HostOsInfo::HostArchitectureItanium:
        return platform == MsvcToolChain::ia64;
    default:
        return false;
    }
}

static QString fixRegistryPath(const QString &path)
{
    QString result = QDir::fromNativeSeparators(path);
    if (result.endsWith(QLatin1Char('/')))
        result.chop(1);
    return result;
}

struct VisualStudioInstallation
{
    QString vsName;
    QVersionNumber version;
    QString path; // Main installation path
    QString vcVarsPath; // Path under which the various vc..bat are to be found
    QString vcVarsAll;
};

QDebug operator<<(QDebug d, const VisualStudioInstallation &i)
{
    QDebugStateSaver saver(d);
    d.noquote();
    d.nospace();
    d << "VisualStudioInstallation(\"" << i.vsName << "\", v=" << i.version
      << ", path=\"" << QDir::toNativeSeparators(i.path)
      << "\", vcVarsPath=\"" << QDir::toNativeSeparators(i.vcVarsPath)
      << "\", vcVarsAll=\"" << QDir::toNativeSeparators(i.vcVarsAll) << "\")";
    return d;
}

static QString windowsProgramFilesDir()
{
#ifdef Q_OS_WIN64
    const char programFilesC[] = "ProgramFiles(x86)";
#else
    const char programFilesC[] = "ProgramFiles";
#endif
    return QDir::fromNativeSeparators(QFile::decodeName(qgetenv(programFilesC)));
}

// Detect build tools introduced with MSVC2017
static Utils::optional<VisualStudioInstallation> detectCppBuildTools2017()
{
    const QString installPath = windowsProgramFilesDir()
                                + "/Microsoft Visual Studio/2017/BuildTools";
    const QString vcVarsPath = installPath + "/VC/Auxiliary/Build";
    const QString vcVarsAllPath = vcVarsPath + "/vcvarsall.bat";

    if (!QFileInfo::exists(vcVarsAllPath))
        return Utils::nullopt;

    VisualStudioInstallation installation;
    installation.path = installPath;
    installation.vcVarsAll = vcVarsAllPath;
    installation.vcVarsPath = vcVarsPath;
    installation.version = QVersionNumber(15);
    installation.vsName = "15.0";

    return installation;
}

static QVector<VisualStudioInstallation> detectVisualStudio()
{
    QVector<VisualStudioInstallation> result;
#ifdef Q_OS_WIN64
    const QString keyRoot = QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\VisualStudio\\SxS\\");
#else
    const QString keyRoot = QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\SxS\\");
#endif
    QSettings vsRegistry(keyRoot + QStringLiteral("VS7"), QSettings::NativeFormat);
    QScopedPointer<QSettings> vcRegistry;
    const QString vcvarsall = QStringLiteral("/vcvarsall.bat");
    foreach (const QString &vsName, vsRegistry.allKeys()) {
        const QVersionNumber version = QVersionNumber::fromString(vsName);
        if (!version.isNull()) {
            VisualStudioInstallation installation;
            installation.vsName = vsName;
            installation.version = version;
            if (version.majorVersion() > 14) {
                // Starting with 15 (MSVC2017): There are no more VC entries,
                // build path from VS installation
                installation.path = fixRegistryPath(vsRegistry.value(vsName).toString());
                installation.vcVarsPath = installation.path + QStringLiteral("/VC/Auxiliary/Build");
                installation.vcVarsAll =  installation.vcVarsPath + vcvarsall;
            } else {
                // Up to 14 (MSVC2015): Look up via matching VC entry
                if (vcRegistry.isNull())
                    vcRegistry.reset(new QSettings(keyRoot + QStringLiteral("VC7"), QSettings::NativeFormat));
                installation.path = fixRegistryPath(vcRegistry->value(vsName).toString());
                installation.vcVarsPath = installation.path;
                installation.vcVarsAll = installation.vcVarsPath + vcvarsall;
            }
            if (QFileInfo(installation.vcVarsAll).isFile()) {
                result.append(installation);
            } else {
                qWarning().noquote() << "Unable to find MSVC setup script "
                    << QDir::toNativeSeparators(installation.vcVarsPath) << " in version "
                    << version;
            }
        }
    }

    // Detect VS 2017 Build Tools
    auto installation = detectCppBuildTools2017();
    if (installation)
        result.append(*installation);

    return result;
}

static Abi findAbiOfMsvc(MsvcToolChain::Type type, MsvcToolChain::Platform platform, const QString &version)
{
    Abi::Architecture arch = Abi::X86Architecture;
    Abi::OSFlavor flavor = Abi::UnknownFlavor;
    int wordWidth = 64;

    switch (platform)
    {
    case MsvcToolChain::x86:
    case MsvcToolChain::amd64_x86:
        wordWidth = 32;
        break;
    case MsvcToolChain::ia64:
    case MsvcToolChain::x86_ia64:
        arch = Abi::ItaniumArchitecture;
        break;
    case MsvcToolChain::amd64:
    case MsvcToolChain::x86_amd64:
        break;
    case MsvcToolChain::arm:
    case MsvcToolChain::x86_arm:
    case MsvcToolChain::amd64_arm:
        arch = Abi::ArmArchitecture;
        wordWidth = 32;
        break;
    };

    QString msvcVersionString = version;
    if (type == MsvcToolChain::WindowsSDK) {
        if (version == QLatin1String("v7.0") || version.startsWith(QLatin1String("6.")))
            msvcVersionString = QLatin1String("9.0");
        else if (version == QLatin1String("v7.0A") || version == QLatin1String("v7.1"))
            msvcVersionString = QLatin1String("10.0");
    }
    if (msvcVersionString.startsWith(QLatin1String("15.")))
        flavor = Abi::WindowsMsvc2017Flavor;
    else if (msvcVersionString.startsWith(QLatin1String("14.")))
        flavor = Abi::WindowsMsvc2015Flavor;
    else if (msvcVersionString.startsWith(QLatin1String("12.")))
        flavor = Abi::WindowsMsvc2013Flavor;
    else if (msvcVersionString.startsWith(QLatin1String("11.")))
        flavor = Abi::WindowsMsvc2012Flavor;
    else if (msvcVersionString.startsWith(QLatin1String("10.")))
        flavor = Abi::WindowsMsvc2010Flavor;
    else if (msvcVersionString.startsWith(QLatin1String("9.")))
        flavor = Abi::WindowsMsvc2008Flavor;
    else
        flavor = Abi::WindowsMsvc2005Flavor;
    const Abi result = Abi(arch, Abi::WindowsOS, flavor, Abi::PEFormat, wordWidth);
    if (!result.isValid())
        qWarning("Unable to completely determine the ABI of MSVC version %s (%s).",
                 qPrintable(version), qPrintable(result.toString()));
    return result;
}

static QString generateDisplayName(const QString &name,
                                   MsvcToolChain::Type t,
                                   MsvcToolChain::Platform p)
{
    if (t == MsvcToolChain::WindowsSDK) {
        QString sdkName = name;
        sdkName += QString::fromLatin1(" (%1)").arg(platformName(p));
        return sdkName;
    }
    // Comes as "9.0" from the registry
    QString vcName = QLatin1String("Microsoft Visual C++ Compiler ");
    vcName += name;
    vcName += QString::fromLatin1(" (%1)").arg(platformName(p));
    return vcName;
}

static QByteArray msvcCompilationDefine(const char *def)
{
    const QByteArray macro(def);
    return "#if defined(" + macro + ")\n__PPOUT__(" + macro + ")\n#endif\n";
}

static QByteArray msvcCompilationFile()
{
    static const char* macros[] = {
        "_ATL_VER",
        "__ATOM__",
        "__AVX__",
        "__AVX2__",
        "_CHAR_UNSIGNED",
        "__CLR_VER",
        "_CMMN_INTRIN_FUNC",
        "_CONTROL_FLOW_GUARD",
        "__cplusplus",
        "__cplusplus_cli",
        "__cplusplus_winrt",
        "_CPPLIB_VER",
        "_CPPRTTI",
        "_CPPUNWIND",
        "_DEBUG",
        "_DLL",
        "_INTEGRAL_MAX_BITS",
        "__INTELLISENSE__",
        "_ISO_VOLATILE",
        "_KERNEL_MODE",
        "_M_AAMD64",
        "_M_ALPHA",
        "_M_AMD64",
        "_MANAGED",
        "_M_ARM",
        "_M_ARM64",
        "_M_ARM_ARMV7VE",
        "_M_ARM_FP",
        "_M_ARM_NT",
        "_M_ARMT",
        "_M_CEE",
        "_M_CEE_PURE",
        "_M_CEE_SAFE",
        "_MFC_VER",
        "_M_FP_EXCEPT",
        "_M_FP_FAST",
        "_M_FP_PRECISE",
        "_M_FP_STRICT",
        "_M_IA64",
        "_M_IX86",
        "_M_IX86_FP",
        "_M_MPPC",
        "_M_MRX000",
        "_M_PPC",
        "_MSC_BUILD",
        "_MSC_EXTENSIONS",
        "_MSC_FULL_VER",
        "_MSC_VER",
        "_MSVC_LANG",
        "__MSVC_RUNTIME_CHECKS",
        "_MT",
        "_M_THUMB",
        "_M_X64",
        "_NATIVE_WCHAR_T_DEFINED",
        "_OPENMP",
        "_PREFAST_",
        "__STDC__",
        "__STDC_HOSTED__",
        "__STDCPP_THREADS__",
        "_VC_NODEFAULTLIB",
        "_WCHAR_T_DEFINED",
        "_WIN32",
        "_WIN32_WCE",
        "_WIN64",
        "_WINRT_DLL",
        "_Wp64",
        nullptr
    };
    QByteArray file = "#define __PPOUT__(x) V##x=x\n\n";
    for (int i = 0; macros[i] != nullptr; ++i)
        file += msvcCompilationDefine(macros[i]);
    file += "\nvoid main(){}\n\n";
    return file;
}

// Run MSVC 'cl' compiler to obtain #defines.
// This function must be thread-safe!
//
// Some notes regarding the used approach:
//
// It seems that there is no reliable way to get all the
// predefined macros for a cl invocation. The following two
// approaches are unfortunately limited since both lead to an
// incomplete list of actually predefined macros and come with
// other problems, too.
//
// 1) Maintain a list of predefined macros from the official
//    documentation (for MSVC2015, e.g. [1]). Feed cl with a
//    temporary file that queries the values of those macros.
//
//    Problems:
//     * Maintaining that list.
//     * The documentation is incomplete, we do not get all
//       predefined macros. E.g. the cl from MSVC2015, set up
//       with "vcvars.bat x86_arm", predefines among others
//       _M_ARMT, but that's not reflected in the
//       documentation.
//
// 2) Run cl with the undocumented options /B1 and /Bx, as
//    described in [2].
//
//    Note: With qmake from Qt >= 5.8 it's possible to print
//    the macros formatted as preprocessor code in an easy to
//    read/compare/diff way:
//
//      > cl /nologo /c /TC /B1 qmake NUL
//      > cl /nologo /c /TP /Bx qmake NUL
//
//    Problems:
//     * Using undocumented options.
//     * Resulting macros are incomplete.
//       For example, the nowadays default option /Zc:wchar_t
//       predefines _WCHAR_T_DEFINED, but this is not reflected
//       with this approach.
//
//       To work around this we would need extra cl invocations
//       to get the actual values of the missing macros
//       (approach 1).
//
// Currently we combine both approaches in this way:
//  * As base, maintain the list from the documentation and
//    update it once a new MSVC version is released.
//  * Enrich it with macros that we discover with approach 2
//    once a new MSVC version is released.
//  * Enrich it further with macros that are not covered with
//    the above points.
//
// TODO: Update the predefined macros for MSVC 2017 once the
//       page exists.
//
// [1] https://msdn.microsoft.com/en-us/library/b0084kay.aspx
// [2] http://stackoverflow.com/questions/3665537/how-to-find-out-cl-exes-built-in-macros
Macros MsvcToolChain::msvcPredefinedMacros(const QStringList cxxflags,
                                           const Utils::Environment &env) const
{
    Macros predefinedMacros;

    QStringList toProcess;
    for (const QString &arg : cxxflags) {
        if (arg.startsWith(QLatin1String("/D"))) {
            const QString define = arg.mid(2);
            predefinedMacros.append(Macro::fromKeyValue(define));
        } else if (arg.startsWith(QLatin1String("/U"))) {
            predefinedMacros.append({arg.mid(2).toLocal8Bit(), ProjectExplorer::MacroType::Undefine});
        } else {
            toProcess.append(arg);
        }
    }

    Utils::TempFileSaver saver(Utils::TemporaryDirectory::masterDirectoryPath() + "/envtestXXXXXX.cpp");
    saver.write(msvcCompilationFile());
    if (!saver.finalize()) {
        qWarning("%s: %s", Q_FUNC_INFO, qPrintable(saver.errorString()));
        return predefinedMacros;
    }
    Utils::SynchronousProcess cpp;
    cpp.setEnvironment(env.toStringList());
    cpp.setWorkingDirectory(Utils::TemporaryDirectory::masterDirectoryPath());
    QStringList arguments;
    const Utils::FileName binary = env.searchInPath(QLatin1String("cl.exe"));
    if (binary.isEmpty()) {
        qWarning("%s: The compiler binary cl.exe could not be found in the path.", Q_FUNC_INFO);
        return predefinedMacros;
    }

    if (language() == ProjectExplorer::Constants::C_LANGUAGE_ID)
        arguments << QLatin1String("/TC");
    arguments << toProcess << QLatin1String("/EP") << QDir::toNativeSeparators(saver.fileName());
    Utils::SynchronousProcessResponse response = cpp.runBlocking(binary.toString(), arguments);
    if (response.result != Utils::SynchronousProcessResponse::Finished ||
            response.exitCode != 0)
        return predefinedMacros;

    const QStringList output = Utils::filtered(response.stdOut().split('\n'),
                                               [](const QString &s) { return s.startsWith('V'); });
    for (const QString &line : output)
        predefinedMacros.append(Macro::fromKeyValue(line.mid(1)));
    return predefinedMacros;
}

//
// We want to detect the language version based on the predefined macros.
// Unfortunately MSVC does not conform to standard when it comes to the predefined
// __cplusplus macro - it reports "199711L", even for newer language versions.
//
// However:
//   * For >= Visual Studio 2015 Update 3 predefines _MSVC_LANG which has the proper value
//     of __cplusplus.
//     See https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros?view=vs-2017
//   * For >= Visual Studio 2017 Version 15.7 __cplusplus is correct once /Zc:__cplusplus
//     is provided on the command line. Then __cplusplus == _MSVC_LANG.
//     See https://blogs.msdn.microsoft.com/vcblog/2018/04/09/msvc-now-correctly-reports-__cplusplus
//
// We rely on _MSVC_LANG if possible, otherwise on some hard coded language versions
// depending on _MSC_VER.
//
// For _MSV_VER values, see https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros?view=vs-2017.
//
LanguageVersion MsvcToolChain::msvcLanguageVersion(const QStringList /*cxxflags*/,
                                                   const Core::Id &language,
                                                   const Macros &macros) const
{
    int mscVer = -1;
    QByteArray msvcLang;
    for (const ProjectExplorer::Macro &macro : macros) {
        if (macro.key == "_MSVC_LANG")
            msvcLang = macro.value;
        if (macro.key == "_MSC_VER")
            mscVer = macro.value.toInt(nullptr);
    }
    QTC_CHECK(mscVer > 0);

    if (language == Constants::CXX_LANGUAGE_ID) {
        if (!msvcLang.isEmpty()) // >= Visual Studio 2015 Update 3
            return ToolChain::cxxLanguageVersion(msvcLang);
        if (mscVer >= 1800) // >= Visual Studio 2013 (12.0)
            return LanguageVersion::CXX14;
        if (mscVer >= 1600) // >= Visual Studio 2010 (10.0)
            return LanguageVersion::CXX11;
        return LanguageVersion::CXX98;
    } else if (language == Constants::C_LANGUAGE_ID) {
        if (mscVer >= 1910) // >= Visual Studio 2017 RTW (15.0)
            return LanguageVersion::C11;
        return LanguageVersion::C99;
    } else {
        QTC_CHECK(false && "Unexpected toolchain language, assuming latest C++ we support.");
        return LanguageVersion::LatestCxx;
    }
}

// Windows: Expand the delayed evaluation references returned by the
// SDK setup scripts: "PATH=!Path!;foo". Some values might expand
// to empty and should not be added
static QString winExpandDelayedEnvReferences(QString in, const Utils::Environment &env)
{
    const QChar exclamationMark = QLatin1Char('!');
    for (int pos = 0; pos < in.size(); ) {
        // Replace "!REF!" by its value in process environment
        pos = in.indexOf(exclamationMark, pos);
        if (pos == -1)
            break;
        const int nextPos = in.indexOf(exclamationMark, pos + 1);
        if (nextPos == -1)
            break;
        const QString var = in.mid(pos + 1, nextPos - pos - 1);
        const QString replacement = env.value(var.toUpper());
        in.replace(pos, nextPos + 1 - pos, replacement);
        pos += replacement.size();
    }
    return in;
}

void MsvcToolChain::environmentModifications(
        QFutureInterface<MsvcToolChain::GenerateEnvResult> &future,
        QString vcvarsBat, QString varsBatArg)
{
    const Utils::Environment inEnv = Utils::Environment::systemEnvironment();
    Utils::Environment outEnv;
    QMap<QString, QString> envPairs;
    QList<Utils::EnvironmentItem> diff;
    Utils::optional<QString> error = generateEnvironmentSettings(inEnv, vcvarsBat,
                                                                 varsBatArg, envPairs);
    if (!error) {

        // Now loop through and process them
        for (auto envIter = envPairs.cbegin(), end = envPairs.cend(); envIter != end; ++envIter) {
            const QString expandedValue = winExpandDelayedEnvReferences(envIter.value(), inEnv);
            if (!expandedValue.isEmpty())
                outEnv.set(envIter.key(), expandedValue);
        }

        if (debug) {
            const QStringList newVars = outEnv.toStringList();
            const QStringList oldVars = inEnv.toStringList();
            QDebug nsp = qDebug().nospace();
            foreach (const QString &n, newVars) {
                if (!oldVars.contains(n))
                    nsp << n << '\n';
            }
        }

        diff = inEnv.diff(outEnv, true);
        for (int i = diff.size() - 1; i >= 0; --i) {
            if (diff.at(i).name.startsWith(QLatin1Char('='))) { // Exclude "=C:", "=EXITCODE"
                diff.removeAt(i);
            }
        }
    }

    future.reportResult({error, diff});
}

void MsvcToolChain::initEnvModWatcher(const QFuture<GenerateEnvResult> &future)
{
    QObject::connect(&m_envModWatcher, &QFutureWatcher<GenerateEnvResult>::resultReadyAt, [&]() {
        const GenerateEnvResult &result = m_envModWatcher.result();
        if (result.error) {
            const QString &errorMessage = *result.error;
            if (!errorMessage.isEmpty())
                TaskHub::addTask(Task::Error, errorMessage, Constants::TASK_CATEGORY_COMPILE);
        } else {
            updateEnvironmentModifications(result.environmentItems);
        }
    });
    m_envModWatcher.setFuture(future);
}

void MsvcToolChain::updateEnvironmentModifications(QList<Utils::EnvironmentItem> modifications)
{
    Utils::EnvironmentItem::sort(&modifications);
    if (modifications != m_environmentModifications) {
        m_environmentModifications = modifications;
        toolChainUpdated();
    }
}

Utils::Environment MsvcToolChain::readEnvironmentSetting(const Utils::Environment& env) const
{
    Utils::Environment resultEnv = env;
    if (m_environmentModifications.isEmpty()) {
        m_envModWatcher.waitForFinished();
        if (m_envModWatcher.future().isFinished() && !m_envModWatcher.future().isCanceled()) {
            const GenerateEnvResult &result = m_envModWatcher.result();
            if (result.error) {
                const QString &errorMessage = *result.error;
                if (!errorMessage.isEmpty())
                    TaskHub::addTask(Task::Error, errorMessage, Constants::TASK_CATEGORY_COMPILE);
            } else {
                resultEnv.modify(result.environmentItems);
            }
        }
    } else {
        resultEnv.modify(m_environmentModifications);
    }
    return resultEnv;
}

// --------------------------------------------------------------------------
// MsvcToolChain
// --------------------------------------------------------------------------

MsvcToolChain::MsvcToolChain(const QString &name, const Abi &abi,
                             const QString &varsBat, const QString &varsBatArg, Core::Id l,
                             Detection d) :
    MsvcToolChain(Constants::MSVC_TOOLCHAIN_TYPEID, name, abi, varsBat, varsBatArg, l, d)
{ }

MsvcToolChain::MsvcToolChain(const MsvcToolChain &other)
    : AbstractMsvcToolChain(other.typeId(), other.language(), other.detection(), other.targetAbi(), other.varsBat())
    , m_environmentModifications(other.m_environmentModifications)
    , m_varsBatArg(other.m_varsBatArg)
{
    if (other.m_envModWatcher.isRunning()) {
        initEnvModWatcher(other.m_envModWatcher.future());
    } else if (m_environmentModifications.isEmpty() && other.m_envModWatcher.future().isFinished()
               && !other.m_envModWatcher.future().isCanceled()) {
        const GenerateEnvResult &result = m_envModWatcher.result();
        if (result.error) {
            const QString &errorMessage = *result.error;
            if (!errorMessage.isEmpty())
                TaskHub::addTask(Task::Error, errorMessage, Constants::TASK_CATEGORY_COMPILE);
        } else {
            updateEnvironmentModifications(result.environmentItems);
        }
    }

    setDisplayName(other.displayName());
}

MsvcToolChain::MsvcToolChain(Core::Id typeId, const QString &name, const Abi &abi,
                             const QString &varsBat, const QString &varsBatArg, Core::Id l,
                             Detection d)
    : AbstractMsvcToolChain(typeId, l, d, abi, varsBat)
    , m_varsBatArg(varsBatArg)
{
    initEnvModWatcher(Utils::runAsync(envModThreadPool(),
                                      &MsvcToolChain::environmentModifications,
                                      varsBat, varsBatArg));

    Q_ASSERT(!name.isEmpty());

    setDisplayName(name);
}

MsvcToolChain::MsvcToolChain(Core::Id typeId) : AbstractMsvcToolChain(typeId, ManualDetection)
{ }

MsvcToolChain::MsvcToolChain() : MsvcToolChain(Constants::MSVC_TOOLCHAIN_TYPEID)
{ }

QString MsvcToolChain::typeDisplayName() const
{
    return MsvcToolChainFactory::tr("MSVC");
}

Utils::FileNameList MsvcToolChain::suggestedMkspecList() const
{
    Utils::FileNameList result;
    result << Utils::FileName::fromLatin1("win32-msvc"); // Common MSVC mkspec introduced in 5.8.1
    switch (m_abi.osFlavor()) {
    case Abi::WindowsMsvc2005Flavor:
        result << Utils::FileName::fromLatin1("win32-msvc2005");
        break;
    case Abi::WindowsMsvc2008Flavor:
        result << Utils::FileName::fromLatin1("win32-msvc2008");
        break;
    case Abi::WindowsMsvc2010Flavor:
        result << Utils::FileName::fromLatin1("win32-msvc2010");
        break;
    case Abi::WindowsMsvc2012Flavor:
        result << Utils::FileName::fromLatin1("win32-msvc2012")
            << Utils::FileName::fromLatin1("win32-msvc2010");
        break;
    case Abi::WindowsMsvc2013Flavor:
        result << Utils::FileName::fromLatin1("win32-msvc2013")
            << Utils::FileName::fromLatin1("winphone-arm-msvc2013")
            << Utils::FileName::fromLatin1("winphone-x86-msvc2013")
            << Utils::FileName::fromLatin1("winrt-arm-msvc2013")
            << Utils::FileName::fromLatin1("winrt-x86-msvc2013")
            << Utils::FileName::fromLatin1("winrt-x64-msvc2013")
            << Utils::FileName::fromLatin1("win32-msvc2012")
            << Utils::FileName::fromLatin1("win32-msvc2010");
        break;
    case Abi::WindowsMsvc2015Flavor:
        result << Utils::FileName::fromLatin1("win32-msvc2015")
            << Utils::FileName::fromLatin1("winphone-arm-msvc2015")
            << Utils::FileName::fromLatin1("winphone-x86-msvc2015")
            << Utils::FileName::fromLatin1("winrt-arm-msvc2015")
            << Utils::FileName::fromLatin1("winrt-x86-msvc2015")
            << Utils::FileName::fromLatin1("winrt-x64-msvc2015");
        break;
    case Abi::WindowsMsvc2017Flavor:
        result << Utils::FileName::fromLatin1("win32-msvc2017")
               << Utils::FileName::fromLatin1("winrt-arm-msvc2017")
               << Utils::FileName::fromLatin1("winrt-x86-msvc2017")
               << Utils::FileName::fromLatin1("winrt-x64-msvc2017");
        break;
    default:
        result.clear();
        break;
    }
    return result;
}

QVariantMap MsvcToolChain::toMap() const
{
    QVariantMap data = AbstractMsvcToolChain::toMap();
    data.insert(QLatin1String(varsBatKeyC), m_vcvarsBat);
    if (!m_varsBatArg.isEmpty())
        data.insert(QLatin1String(varsBatArgKeyC), m_varsBatArg);
    data.insert(QLatin1String(supportedAbiKeyC), m_abi.toString());
    Utils::EnvironmentItem::sort(&m_environmentModifications);
    data.insert(QLatin1String(environModsKeyC),
                Utils::EnvironmentItem::toVariantList(m_environmentModifications));
    return data;
}

bool MsvcToolChain::fromMap(const QVariantMap &data)
{
    if (!AbstractMsvcToolChain::fromMap(data))
        return false;
    m_vcvarsBat = QDir::fromNativeSeparators(data.value(QLatin1String(varsBatKeyC)).toString());
    m_varsBatArg = data.value(QLatin1String(varsBatArgKeyC)).toString();
    const QString abiString = data.value(QLatin1String(supportedAbiKeyC)).toString();
    m_abi = Abi::fromString(abiString);
    m_environmentModifications = Utils::EnvironmentItem::itemsFromVariantList(
                data.value(QLatin1String(environModsKeyC)).toList());

    initEnvModWatcher(Utils::runAsync(envModThreadPool(),
                                      &MsvcToolChain::environmentModifications,
                                      m_vcvarsBat, m_varsBatArg));

    return !m_vcvarsBat.isEmpty() && m_abi.isValid();
}


std::unique_ptr<ToolChainConfigWidget> MsvcToolChain::createConfigurationWidget()
{
    return std::make_unique<MsvcToolChainConfigWidget>(this);
}

ToolChain *MsvcToolChain::clone() const
{
    return new MsvcToolChain(*this);
}

// --------------------------------------------------------------------------
// MsvcBasedToolChainConfigWidget: Creates a simple GUI without error label
// to display name and varsBat. Derived classes should add the error label and
// call setFromMsvcToolChain().
// --------------------------------------------------------------------------

MsvcBasedToolChainConfigWidget::MsvcBasedToolChainConfigWidget(ToolChain *tc) :
    ToolChainConfigWidget(tc),
    m_nameDisplayLabel(new QLabel(this)),
    m_varsBatDisplayLabel(new QLabel(this))
{
    m_nameDisplayLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_mainLayout->addRow(m_nameDisplayLabel);
    m_varsBatDisplayLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_mainLayout->addRow(tr("Initialization:"), m_varsBatDisplayLabel);
}

void MsvcBasedToolChainConfigWidget::setFromMsvcToolChain()
{
    const auto *tc = static_cast<const MsvcToolChain *>(toolChain());
    QTC_ASSERT(tc, return);
    m_nameDisplayLabel->setText(tc->displayName());
    QString varsBatDisplay = QDir::toNativeSeparators(tc->varsBat());
    if (!tc->varsBatArg().isEmpty()) {
        varsBatDisplay += QLatin1Char(' ');
        varsBatDisplay += tc->varsBatArg();
    }
    m_varsBatDisplayLabel->setText(varsBatDisplay);
}

// --------------------------------------------------------------------------
// MsvcToolChainConfigWidget
// --------------------------------------------------------------------------

MsvcToolChainConfigWidget::MsvcToolChainConfigWidget(ToolChain *tc) :
    MsvcBasedToolChainConfigWidget(tc)
{
    addErrorLabel();
    setFromMsvcToolChain();
}

// --------------------------------------------------------------------------
// ClangClToolChainConfigWidget
// --------------------------------------------------------------------------

ClangClToolChainConfigWidget::ClangClToolChainConfigWidget(ToolChain *tc)
    : MsvcBasedToolChainConfigWidget(tc)
{
    if (tc->isAutoDetected()) {
        m_llvmDirLabel = new QLabel(this);
        m_llvmDirLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        m_mainLayout->addRow(tr("&Compiler path:"), m_llvmDirLabel);
    } else {
        const QStringList gnuVersionArgs = QStringList("--version");
        m_compilerCommand = new Utils::PathChooser(this);
        m_compilerCommand->setExpectedKind(Utils::PathChooser::ExistingCommand);
        m_compilerCommand->setCommandVersionArguments(gnuVersionArgs);
        m_compilerCommand->setHistoryCompleter("PE.Clang.Command.History");
        m_mainLayout->addRow(tr("&Compiler path:"), m_compilerCommand);
    }
    addErrorLabel();
    setFromClangClToolChain();

    if (m_compilerCommand) {
        connect(m_compilerCommand, &Utils::PathChooser::rawPathChanged,
                this, &ClangClToolChainConfigWidget::dirty);
    }
}

void ClangClToolChainConfigWidget::setFromClangClToolChain()
{
    setFromMsvcToolChain();
    const auto *clangClToolChain = static_cast<const ClangClToolChain *>(toolChain());
    if (clangClToolChain->isAutoDetected())
        m_llvmDirLabel->setText(QDir::toNativeSeparators(clangClToolChain->clangPath()));
    else
        m_compilerCommand->setFileName(Utils::FileName::fromString(clangClToolChain->clangPath()));
}

static const MsvcToolChain *findMsvcToolChain(unsigned char wordWidth, Abi::OSFlavor flavor)
{
    return Utils::findOrDefault(g_availableMsvcToolchains,
                                [wordWidth, flavor] (const MsvcToolChain *tc)
    { const Abi abi = tc->targetAbi();
        return abi.osFlavor() == flavor
                && wordWidth == abi.wordWidth();} );
}

static QVersionNumber clangClVersion(const QString& clangClPath)
{
    Utils::SynchronousProcess clangClProcess;
    const Utils::SynchronousProcessResponse response = clangClProcess.runBlocking(
                clangClPath, {QStringLiteral("--version")});
    if (response.result != Utils::SynchronousProcessResponse::Finished || response.exitCode != 0)
        return {};
    const QRegularExpressionMatch match = QRegularExpression(
                QStringLiteral("clang version (\\d+(\\.\\d+)+)")).match(response.stdOut());
    if (!match.hasMatch())
        return {};
    return QVersionNumber::fromString(match.captured(1));
}

static const MsvcToolChain *selectMsvcToolChain(const QString &clangClPath,
                                                unsigned char wordWidth)
{
    const MsvcToolChain *toolChain = nullptr;
    const QVersionNumber version = clangClVersion(clangClPath);
    if (version.majorVersion() >= 6)
        toolChain = findMsvcToolChain(wordWidth, Abi::WindowsMsvc2017Flavor);
    if (!toolChain) {
        toolChain = findMsvcToolChain(wordWidth, Abi::WindowsMsvc2015Flavor);
        if (!toolChain)
            toolChain = findMsvcToolChain(wordWidth, Abi::WindowsMsvc2013Flavor);
    }
    return toolChain;
}

static QList<ToolChain *> detectClangClToolChainInPath(
        const QString &clangClPath, const QList<ToolChain *> &alreadyKnown, bool isDefault = false)
{
    QList<ToolChain *> res;
    const unsigned char wordWidth = Utils::is64BitWindowsBinary(clangClPath) ? 64 : 32;
    const MsvcToolChain *toolChain = selectMsvcToolChain(clangClPath, wordWidth);

    if (!toolChain) {
        qWarning("Unable to find a suitable MSVC version for \"%s\".",
                 qPrintable(QDir::toNativeSeparators(clangClPath)));
        return res;
    }

    Utils::Environment systemEnvironment = Utils::Environment::systemEnvironment();
    const Abi targetAbi = toolChain->targetAbi();
    const QString name = QString("%1LLVM %2 bit based on %3")
              .arg(QLatin1String(isDefault ? "Default " : ""))
              .arg(wordWidth)
              .arg(Abi::toString(targetAbi.osFlavor()).toUpper());
    for (auto language: {Constants::C_LANGUAGE_ID, Constants::CXX_LANGUAGE_ID}) {
        ClangClToolChain *tc = static_cast<ClangClToolChain *>(
                    Utils::findOrDefault(
                        alreadyKnown,
                        [&targetAbi, &language, &clangClPath, &systemEnvironment](ToolChain *tc) -> bool {
            if (tc->typeId() != Constants::CLANG_CL_TOOLCHAIN_TYPEID)
                return false;
            if (tc->targetAbi() != targetAbi)
                return false;
            if (tc->language() != language)
                return false;
            return systemEnvironment.isSameExecutable(tc->compilerCommand().toString(), clangClPath);
        }));
        if (!tc) {
            tc = new ClangClToolChain(name, clangClPath, language, ToolChain::AutoDetection);
            tc->resetMsvcToolChain(toolChain);
        }
        res << tc;
    }
    return res;
}

static QString compilerFromPath(const QString &path)
{
    return path + "/bin/clang-cl.exe";
}

void ClangClToolChainConfigWidget::applyImpl()
{
    Utils::FileName clangClPath = m_compilerCommand->fileName();
        auto clangClToolChain = static_cast<ClangClToolChain *>(toolChain());
    clangClToolChain->setClangPath(clangClPath.toString());

    if (clangClPath.fileName() != "clang-cl.exe") {
        clangClToolChain->resetMsvcToolChain();
        setFromClangClToolChain();
        return;
    }

    QList<ToolChain *> results = detectClangClToolChainInPath(clangClPath.toString(), {});

    if (results.isEmpty()) {
        clangClToolChain->resetMsvcToolChain();
    } else {
        for (const ToolChain *toolchain : results) {
            if (toolchain->language() == clangClToolChain->language()) {
                clangClToolChain->resetMsvcToolChain(static_cast<const MsvcToolChain *>(toolchain));
                break;
            }
        }

        qDeleteAll(results);
    }
    setFromClangClToolChain();
}

void ClangClToolChainConfigWidget::discardImpl()
{
    setFromClangClToolChain();
}

// --------------------------------------------------------------------------
// ClangClToolChain, piggy-backing on MSVC2015 and providing the compiler
// clang-cl.exe as a [to some extent] compatible drop-in replacement for cl.
// --------------------------------------------------------------------------

ClangClToolChain::ClangClToolChain(const QString &name, const QString &clangPath,
                                   Core::Id language,
                                   Detection d)
    : MsvcToolChain(Constants::CLANG_CL_TOOLCHAIN_TYPEID, name, Abi(), "", "", language, d)
    , m_clangPath(clangPath)
{
}

ClangClToolChain::ClangClToolChain() : MsvcToolChain(Constants::CLANG_CL_TOOLCHAIN_TYPEID)
{ }

bool ClangClToolChain::isValid() const
{
    return MsvcToolChain::isValid() && compilerCommand().exists()
            && compilerCommand().fileName() == "clang-cl.exe";
}

void ClangClToolChain::addToEnvironment(Utils::Environment &env) const
{
    MsvcToolChain::addToEnvironment(env);
    QDir path = QFileInfo(m_clangPath).absoluteDir(); // bin folder
    env.prependOrSetPath(path.canonicalPath());
}

Utils::FileName ClangClToolChain::compilerCommand() const
{
    return Utils::FileName::fromString(m_clangPath);
}

QString ClangClToolChain::typeDisplayName() const
{
    return QCoreApplication::translate("ProjectExplorer::ClangToolChainFactory", "Clang");
}

QList<Utils::FileName> ClangClToolChain::suggestedMkspecList() const
{
    const QString mkspec = QLatin1String("win32-clang-") + Abi::toString(targetAbi().osFlavor());
    return QList<Utils::FileName>{Utils::FileName::fromString(mkspec),
                                  Utils::FileName::fromString("win32-clang-msvc")};
}

IOutputParser *ClangClToolChain::outputParser() const
{
    return new ClangClParser;
}

ToolChain *ClangClToolChain::clone() const
{
    return new ClangClToolChain(*this);
}

static inline QString llvmDirKey() { return QStringLiteral("ProjectExplorer.ClangClToolChain.LlvmDir"); }

QVariantMap ClangClToolChain::toMap() const
{
    QVariantMap result = MsvcToolChain::toMap();
    result.insert(llvmDirKey(), m_clangPath);
    return result;
}

bool ClangClToolChain::fromMap(const QVariantMap &data)
{
    if (!MsvcToolChain::fromMap(data))
        return false;
    const QString clangPath = data.value(llvmDirKey()).toString();
    if (clangPath.isEmpty())
        return false;
    m_clangPath = clangPath;

    return true;
}

std::unique_ptr<ToolChainConfigWidget> ClangClToolChain::createConfigurationWidget()
{
    return std::make_unique<ClangClToolChainConfigWidget>(this);
}

void ClangClToolChain::resetMsvcToolChain(const MsvcToolChain *base)
{
    if (!base) {
        m_abi = Abi();
        m_vcvarsBat.clear();
        setVarsBatArg("");
        return;
    }
    m_abi = base->targetAbi();
    m_vcvarsBat = base->varsBat();
    setVarsBatArg(base->varsBatArg());

    initEnvModWatcher(Utils::runAsync(envModThreadPool(),
                                      &ClangClToolChain::environmentModifications,
                                      m_vcvarsBat, base->varsBatArg()));
}

bool ClangClToolChain::operator ==(const ToolChain &other) const
{
    if (!MsvcToolChain::operator ==(other))
        return false;

    const auto *clangClTc = static_cast<const ClangClToolChain *>(&other);
    return m_clangPath == clangClTc->m_clangPath;
}

Macros ClangClToolChain::msvcPredefinedMacros(const QStringList cxxflags,
                                              const Utils::Environment &env) const
{
    if (!cxxflags.contains("--driver-mode=g++"))
        return MsvcToolChain::msvcPredefinedMacros(cxxflags, env);

    Utils::SynchronousProcess cpp;
    cpp.setEnvironment(env.toStringList());
    cpp.setWorkingDirectory(Utils::TemporaryDirectory::masterDirectoryPath());

    QStringList arguments = cxxflags;
    arguments.append(gccPredefinedMacrosOptions(language()));
    arguments.append("-");
    Utils::SynchronousProcessResponse response = cpp.runBlocking(compilerCommand().toString(),
                                                                 arguments);
    if (response.result != Utils::SynchronousProcessResponse::Finished ||
            response.exitCode != 0) {
        // Show the warning but still parse the output.
        QTC_CHECK(false && "clang-cl exited with non-zero code.");
    }

    return Macro::toMacros(response.allRawOutput());
}

LanguageVersion ClangClToolChain::msvcLanguageVersion(const QStringList cxxflags,
                                                      const Core::Id &language,
                                                      const Macros &macros) const
{
    if (cxxflags.contains("--driver-mode=g++"))
        return ToolChain::languageVersion(language, macros);
    return MsvcToolChain::msvcLanguageVersion(cxxflags, language, macros);
}

// --------------------------------------------------------------------------
// MsvcToolChainFactory
// --------------------------------------------------------------------------

MsvcToolChainFactory::MsvcToolChainFactory()
{
    setDisplayName(tr("MSVC"));
}

QSet<Core::Id> MsvcToolChainFactory::supportedLanguages() const
{
    return {Constants::C_LANGUAGE_ID, Constants::CXX_LANGUAGE_ID};
}

QString MsvcToolChainFactory::vcVarsBatFor(const QString &basePath, MsvcToolChain::Platform platform,
                                           const QVersionNumber &v)
{
    QString result;
    if (const MsvcPlatform *p = platformEntry(platform)) {
        result += basePath;
        // Starting with 15.0 (MSVC2017), the .bat are in one folder.
        if (v.majorVersion() <= 14)
            result += QLatin1String(p->prefix);
        result += QLatin1Char('/');
        result += QLatin1String(p->bat);
    }
    return result;
}

static QList<ToolChain *> findOrCreateToolChain(
        const QList<ToolChain *> &alreadyKnown,
        const QString &name, const Abi &abi,
        const QString &varsBat, const QString &varsBatArg,
        ToolChain::Detection d = ToolChain::ManualDetection)
{
    QList<ToolChain *> res;
    for (auto language: {Constants::C_LANGUAGE_ID, Constants::CXX_LANGUAGE_ID}) {
        ToolChain *tc = Utils::findOrDefault(
                    alreadyKnown,
                    [&varsBat, &varsBatArg, &abi, &language](ToolChain *tc) -> bool {
            if (tc->typeId() != Constants::MSVC_TOOLCHAIN_TYPEID)
                return false;
            if (tc->targetAbi() != abi)
                return false;
            if (tc->language() != language)
                return false;
            auto mtc = static_cast<MsvcToolChain *>(tc);
            return mtc->varsBat() == varsBat
                    && mtc->varsBatArg() == varsBatArg;
        });
        if (!tc)
            tc = new MsvcToolChain(name, abi, varsBat, varsBatArg, language, d);
        res << tc;
    }
    return res;
}

// Detect build tools introduced with MSVC2015
static void detectCppBuildTools2015(QList<ToolChain *> *list)
{
    struct Entry {
        const char *postFix;
        const char *varsBatArg;
        Abi::Architecture architecture;
        Abi::BinaryFormat format;
        unsigned char wordSize;
    };

    const Entry entries[] = {
        {" (x86)", "x86", Abi::X86Architecture, Abi::PEFormat, 32},
        {" (x64)", "amd64", Abi::X86Architecture, Abi::PEFormat, 64},
        {" (x86_arm)", "x86_arm", Abi::ArmArchitecture, Abi::PEFormat, 32},
        {" (x64_arm)", "amd64_arm", Abi::ArmArchitecture, Abi::PEFormat, 64}
    };

    const QString name = QStringLiteral("Microsoft Visual C++ Build Tools");
    const QString vcVarsBat = windowsProgramFilesDir()
            + QLatin1Char('/') + name + QStringLiteral("/vcbuildtools.bat");
    if (!QFileInfo(vcVarsBat).isFile())
        return;
    const size_t count = sizeof(entries) / sizeof(entries[0]);
    for (size_t i = 0; i < count; ++i) {
        const Entry &e = entries[i];
        const Abi abi(e.architecture, Abi::WindowsOS, Abi::WindowsMsvc2015Flavor,
                      e.format, e.wordSize);
        for (auto language: {Constants::C_LANGUAGE_ID, Constants::CXX_LANGUAGE_ID}) {
            list->append(new MsvcToolChain(name + QLatin1String(e.postFix), abi,
                                           vcVarsBat, QLatin1String(e.varsBatArg),
                                           language, ToolChain::AutoDetection));
        }
    }
}

QList<ToolChain *> MsvcToolChainFactory::autoDetect(const QList<ToolChain *> &alreadyKnown)
{
    QList<ToolChain *> results;

    // 1) Installed SDKs preferred over standalone Visual studio
    const QSettings sdkRegistry(QLatin1String("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows"),
                                QSettings::NativeFormat);
    const QString defaultSdkPath = sdkRegistry.value(QLatin1String("CurrentInstallFolder")).toString();
    if (!defaultSdkPath.isEmpty()) {
        foreach (const QString &sdkKey, sdkRegistry.childGroups()) {
            const QString name = sdkRegistry.value(sdkKey + QLatin1String("/ProductName")).toString();
            const QString folder = sdkRegistry.value(sdkKey + QLatin1String("/InstallationFolder")).toString();
            if (folder.isEmpty())
                continue;

            QDir dir(folder);
            if (!dir.cd(QLatin1String("bin")))
                continue;
            QFileInfo fi(dir, QLatin1String("SetEnv.cmd"));
            if (!fi.exists())
                continue;

            QList<ToolChain *> tmp;
            const QVector<QPair<MsvcToolChain::Platform, QString> > platforms = {
                {MsvcToolChain::x86, "x86"},
                {MsvcToolChain::amd64, "x64"},
                {MsvcToolChain::ia64, "ia64"},
            };
            for (auto platform: platforms) {
                tmp.append(findOrCreateToolChain(
                               alreadyKnown,
                               generateDisplayName(name, MsvcToolChain::WindowsSDK, platform.first),
                               findAbiOfMsvc(MsvcToolChain::WindowsSDK, platform.first, sdkKey),
                               fi.absoluteFilePath(), "/" + platform.second, ToolChain::AutoDetection));
            }
            // Make sure the default is front.
            if (folder == defaultSdkPath)
                results = tmp + results;
            else
                results += tmp;
        } // foreach
    }

    // 2) Installed MSVCs
    // prioritized list.
    // x86_arm was put before amd64_arm as a workaround for auto detected windows phone
    // toolchains. As soon as windows phone builds support x64 cross builds, this change
    // can be reverted.
    const MsvcToolChain::Platform platforms[] = {
        MsvcToolChain::x86, MsvcToolChain::amd64_x86,
        MsvcToolChain::amd64, MsvcToolChain::x86_amd64,
        MsvcToolChain::arm, MsvcToolChain::x86_arm, MsvcToolChain::amd64_arm,
        MsvcToolChain::ia64, MsvcToolChain::x86_ia64
    };

    foreach (const VisualStudioInstallation &i, detectVisualStudio()) {
        for (MsvcToolChain::Platform platform : platforms) {
            const bool toolchainInstalled =
                QFileInfo(vcVarsBatFor(i.vcVarsPath, platform, i.version)).isFile();
            if (hostSupportsPlatform(platform) && toolchainInstalled) {
                results.append(findOrCreateToolChain(
                                   alreadyKnown,
                                   generateDisplayName(i.vsName, MsvcToolChain::VS, platform),
                                   findAbiOfMsvc(MsvcToolChain::VS, platform, i.vsName),
                                   i.vcVarsAll, platformName(platform),
                                   ToolChain::AutoDetection));
            }
        }
    }

    detectCppBuildTools2015(&results);

    for (const ToolChain *toolchain : results)
        g_availableMsvcToolchains.append(static_cast<const MsvcToolChain *>(toolchain));

    return results;
}

ClangClToolChainFactory::ClangClToolChainFactory()
{
    setDisplayName(tr("clang-cl"));
}

bool ClangClToolChainFactory::canCreate()
{
    return !g_availableMsvcToolchains.isEmpty();
}

QList<ToolChain *> ClangClToolChainFactory::autoDetect(const QList<ToolChain *> &alreadyKnown)
{
    Q_UNUSED(alreadyKnown)

#ifdef Q_OS_WIN64
    const char registryNode[] = "HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\LLVM\\LLVM";
#else
    const char registryNode[] = "HKEY_LOCAL_MACHINE\\SOFTWARE\\LLVM\\LLVM";
#endif

    QList<ToolChain *> results;
    QList<ToolChain *> known = alreadyKnown;

    QString qtCreatorsClang = Core::ICore::clangExecutable(CLANG_BINDIR);
    if (!qtCreatorsClang.isEmpty()) {
        qtCreatorsClang = Utils::FileName::fromString(qtCreatorsClang).parentDir()
                .appendPath("clang-cl.exe").toString();
        results.append(detectClangClToolChainInPath(qtCreatorsClang, alreadyKnown, true));
        known.append(results);
    }

    const QSettings registry(QLatin1String(registryNode), QSettings::NativeFormat);
    if (registry.status() == QSettings::NoError) {
        const QString path = QDir::cleanPath(registry.value(QStringLiteral(".")).toString());
        const QString clangClPath = compilerFromPath(path);
        if (!path.isEmpty()) {
            results.append(detectClangClToolChainInPath(clangClPath, known));
            known.append(results);
        }
    }

    const Utils::Environment systemEnvironment = Utils::Environment::systemEnvironment();
    const Utils::FileName clangClPath = systemEnvironment.searchInPath("clang-cl");
    if (!clangClPath.isEmpty())
        results.append(detectClangClToolChainInPath(clangClPath.toString(), known));

    return results;
}

ToolChain *ClangClToolChainFactory::create(Core::Id l)
{
    return new ClangClToolChain("clang-cl", "", l, ToolChain::ManualDetection);
}

bool MsvcToolChain::operator ==(const ToolChain &other) const
{
    if (!AbstractMsvcToolChain::operator ==(other))
        return false;
    const auto *msvcTc = static_cast<const MsvcToolChain *>(&other);
    return m_varsBatArg == msvcTc->m_varsBatArg;
}

void MsvcToolChain::cancelMsvcToolChainDetection()
{
    envModThreadPool()->clear();
}

bool MsvcToolChainFactory::canRestore(const QVariantMap &data)
{
    const Core::Id id = typeIdFromMap(data);
    return id == Constants::MSVC_TOOLCHAIN_TYPEID;
}

template <class ToolChainType>
ToolChainType *readFromMap(const QVariantMap &data)
{
    auto result = new ToolChainType;
    if (result->fromMap(data))
        return result;
    delete result;
    return nullptr;
}

ToolChain *MsvcToolChainFactory::restore(const QVariantMap &data)
{
    return readFromMap<MsvcToolChain>(data);
}

bool ClangClToolChainFactory::canRestore(const QVariantMap &data)
{
    const Core::Id id = typeIdFromMap(data);
    return id == Constants::CLANG_CL_TOOLCHAIN_TYPEID;
}

ToolChain *ClangClToolChainFactory::restore(const QVariantMap &data)
{
    return readFromMap<ClangClToolChain>(data);
}

} // namespace Internal
} // namespace ProjectExplorer
