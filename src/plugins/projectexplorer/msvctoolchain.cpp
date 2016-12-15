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

#include "msvcparser.h"
#include "projectexplorerconstants.h"

#include <utils/algorithm.h>
#include <utils/synchronousprocess.h>
#include <utils/winutils.h>
#include <utils/qtcassert.h>
#include <utils/hostosinfo.h>
#include <utils/temporarydirectory.h>

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QSettings>
#include <QVector>
#include <QVersionNumber>

#include <QLabel>
#include <QFormLayout>

#define KEY_ROOT "ProjectExplorer.MsvcToolChain."
static const char varsBatKeyC[] = KEY_ROOT"VarsBat";
static const char varsBatArgKeyC[] = KEY_ROOT"VarsBatArg";
static const char supportedAbiKeyC[] = KEY_ROOT"SupportedAbi";

enum { debug = 0 };

namespace ProjectExplorer {
namespace Internal {

// --------------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------------

struct MsvcPlatform {
    MsvcToolChain::Platform platform;
    const char *name;
    const char *prefix; // VS up until 14.0 (MSVC2015)
    const char *bat;
};

const MsvcPlatform platforms[] =
{
    { MsvcToolChain::x86, "x86", "/bin", "vcvars32.bat" },
    { MsvcToolChain::amd64, "amd64", "/bin/amd64", "vcvars64.bat" },
    { MsvcToolChain::x86_amd64, "x86_amd64", "/bin/x86_amd64", "vcvarsx86_amd64.bat" },
    { MsvcToolChain::ia64, "ia64", "/bin/ia64", "vcvars64.bat" },
    { MsvcToolChain::x86_ia64, "x86_ia64", "/bin/x86_ia64", "vcvarsx86_ia64.bat" },
    { MsvcToolChain::arm, "arm", "/bin/arm", "vcvarsarm.bat" },
    { MsvcToolChain::x86_arm, "x86_arm", "/bin/x86_arm", "vcvarsx86_arm.bat" },
    { MsvcToolChain::amd64_arm, "amd64_arm", "/bin/amd64_arm", "vcvarsamd64_arm.bat" },
    { MsvcToolChain::amd64_x86, "amd64_x86", "/bin/amd64_x86", "vcvarsamd64_x86.bat" }
};

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
        // fall through (all x86 toolchains are also working on an amd64 host)
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

static QByteArray msvcCompilationFile()
{
    static const char* macros[] = {"_ATL_VER", "_CHAR_UNSIGNED", "__CLR_VER",
                                   "__cplusplus_cli", "__COUNTER__", "__cplusplus",
                                   "_CPPLIB_VER", "_CPPRTTI", "_CPPUNWIND",
                                   "_DEBUG", "_DLL", "__FUNCDNAME__",
                                   "__FUNCSIG__", "__FUNCTION__", "_INTEGRAL_MAX_BITS",
                                   "_M_ALPHA", "_M_AAMD64", "_M_CEE", "_M_CEE_PURE",
                                   "_M_CEE_SAFE", "_M_IX86", "_M_IA64",
                                   "_M_IX86_FP", "_M_MPPC", "_M_MRX000",
                                   "_M_PPC", "_M_X64", "_MANAGED",
                                   "_MFC_VER", "_MSC_BUILD", "_MSC_EXTENSIONS",
                                   "_MSC_FULL_VER", "_MSC_VER", "__MSVC_RUNTIME_CHECKS",
                                   "_MT", "_NATIVE_WCHAR_T_DEFINED", "_OPENMP",
                                   "_VC_NODEFAULTLIB", "_WCHAR_T_DEFINED", "_WIN32",
                                   "_WIN32_WCE", "_WIN64", "_Wp64",
                                   "__DATE__", "__TIME__", "__TIMESTAMP__",
                                   0};
    QByteArray file = "#define __PPOUT__(x) V##x=x\n\n";
    for (int i = 0; macros[i] != 0; ++i) {
        const QByteArray macro(macros[i]);
        file += "#if defined(" + macro + ")\n__PPOUT__("
                + macro + ")\n#endif\n";
    }
    file += "\nvoid main(){}\n\n";
    return file;
}

// Run MSVC 'cl' compiler to obtain #defines.
QByteArray MsvcToolChain::msvcPredefinedMacros(const QStringList cxxflags,
                                               const Utils::Environment &env) const
{
    QByteArray predefinedMacros = AbstractMsvcToolChain::msvcPredefinedMacros(cxxflags, env);

    QStringList toProcess;
    foreach (const QString &arg, cxxflags) {
        if (arg.startsWith(QLatin1String("/D"))) {
            QString define = arg.mid(2);
            int pos = define.indexOf(QLatin1Char('='));
            if (pos < 0) {
                predefinedMacros += "#define ";
                predefinedMacros += define.toLocal8Bit();
                predefinedMacros += '\n';
            } else {
                predefinedMacros += "#define ";
                predefinedMacros += define.left(pos).toLocal8Bit();
                predefinedMacros += ' ';
                predefinedMacros += define.mid(pos + 1).toLocal8Bit();
                predefinedMacros += '\n';
            }
        } else if (arg.startsWith(QLatin1String("/U"))) {
            predefinedMacros += "#undef ";
            predefinedMacros += arg.mid(2).toLocal8Bit();
            predefinedMacros += '\n';
        } else if (arg.startsWith(QLatin1String("-I"))) {
            // Include paths should not have any effect on defines
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

    arguments << toProcess << QLatin1String("/EP") << QDir::toNativeSeparators(saver.fileName());
    Utils::SynchronousProcessResponse response = cpp.runBlocking(binary.toString(), arguments);
    if (response.result != Utils::SynchronousProcessResponse::Finished ||
            response.exitCode != 0)
        return predefinedMacros;

    const QStringList output = Utils::filtered(response.stdOut().split('\n'),
                                               [](const QString &s) { return s.startsWith('V'); });
    foreach (const QString& line, output) {
        QStringList split = line.split('=');
        const QString key = split.at(0).mid(1);
        QString value = split.at(1);
        predefinedMacros += "#define ";
        predefinedMacros += key.toUtf8();
        predefinedMacros += ' ';
        predefinedMacros += value.toUtf8();
        predefinedMacros += '\n';
    }
    if (debug)
        qDebug() << "msvcPredefinedMacros" << predefinedMacros;
    return predefinedMacros;
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

Utils::Environment MsvcToolChain::readEnvironmentSetting(const Utils::Environment& env) const
{
    Utils::Environment result;
    QMap<QString, QString> envPairs;
    if (!generateEnvironmentSettings(env, m_vcvarsBat, m_varsBatArg, envPairs))
        return env;

    // Now loop through and process them
    QMap<QString,QString>::const_iterator envIter;
    for (envIter = envPairs.constBegin(); envIter!=envPairs.constEnd(); ++envIter) {
        const QString expandedValue = winExpandDelayedEnvReferences(envIter.value(), env);
        if (!expandedValue.isEmpty())
            result.set(envIter.key(), expandedValue);
    }

    if (debug) {
        const QStringList newVars = result.toStringList();
        const QStringList oldVars = env.toStringList();
        QDebug nsp = qDebug().nospace();
        foreach (const QString &n, newVars) {
            if (!oldVars.contains(n))
                nsp << n << '\n';
        }
    }
    return result;
}

// --------------------------------------------------------------------------
// MsvcToolChain
// --------------------------------------------------------------------------

MsvcToolChain::MsvcToolChain(const QString &name, const Abi &abi,
                             const QString &varsBat, const QString &varsBatArg, Core::Id l,
                             Detection d) :
    MsvcToolChain(Constants::MSVC_TOOLCHAIN_TYPEID, name, abi, varsBat, varsBatArg, l, d)
{ }

MsvcToolChain::MsvcToolChain(Core::Id typeId, const QString &name, const Abi &abi,
                             const QString &varsBat, const QString &varsBatArg, Core::Id l,
                             Detection d) : AbstractMsvcToolChain(typeId, l, d, abi, varsBat),
    m_varsBatArg(varsBatArg)
{
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
        result << Utils::FileName::fromLatin1("win32-msvc2017");
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
    return data;
}

bool MsvcToolChain::fromMap(const QVariantMap &data)
{
    if (!AbstractMsvcToolChain::fromMap(data))
        return false;
    m_vcvarsBat = QDir::fromNativeSeparators(data.value(QLatin1String(varsBatKeyC)).toString());
    m_varsBatArg = data.value(QLatin1String(varsBatArgKeyC)).toString();
    const QString abiString = data.value(QLatin1String(supportedAbiKeyC)).toString();
    m_abi = Abi(abiString);

    return !m_vcvarsBat.isEmpty() && m_abi.isValid();
}


ToolChainConfigWidget *MsvcToolChain::configurationWidget()
{
    return new MsvcToolChainConfigWidget(this);
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
    const MsvcToolChain *tc = static_cast<const MsvcToolChain *>(toolChain());
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
    , m_llvmDirLabel(new QLabel(this))
{
    m_llvmDirLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_mainLayout->addRow(tr("LLVM:"), m_llvmDirLabel);
    addErrorLabel();
    setFromClangClToolChain();
}

void ClangClToolChainConfigWidget::setFromClangClToolChain()
{
    setFromMsvcToolChain();
    const ClangClToolChain *tc = static_cast<const ClangClToolChain *>(toolChain());
    QTC_ASSERT(tc, return);
    m_llvmDirLabel->setText(QDir::toNativeSeparators(tc-> llvmDir()));
}

// --------------------------------------------------------------------------
// ClangClToolChain, piggy-backing on MSVC2015 and providing the compiler
// clang-cl.exe as a [to some extent] compatible drop-in replacement for cl.
// --------------------------------------------------------------------------

static const char clangClBinary[] = "clang-cl.exe";

ClangClToolChain::ClangClToolChain(const QString &name, const QString &llvmDir,
                                   const Abi &abi,
                                   const QString &varsBat, const QString &varsBatArg, Core::Id language,
                                   Detection d)
    : MsvcToolChain(Constants::CLANG_CL_TOOLCHAIN_TYPEID, name, abi, varsBat, varsBatArg, language, d)
    , m_llvmDir(llvmDir)
    , m_compiler(Utils::FileName::fromString(m_llvmDir + QStringLiteral("/bin/") + QLatin1String(clangClBinary)))
{ }

ClangClToolChain::ClangClToolChain() : MsvcToolChain(Constants::CLANG_CL_TOOLCHAIN_TYPEID)
{ }

bool ClangClToolChain::isValid() const
{
    return MsvcToolChain::isValid() && m_compiler.exists();
}

void ClangClToolChain::addToEnvironment(Utils::Environment &env) const
{
    MsvcToolChain::addToEnvironment(env);
    env.prependOrSetPath(m_llvmDir + QStringLiteral("/bin"));
}

QString ClangClToolChain::typeDisplayName() const
{
    return QCoreApplication::translate("ProjectExplorer::ClangToolChainFactory", "Clang");
}

QList<Utils::FileName> ClangClToolChain::suggestedMkspecList() const
{
    const QString mkspec = QLatin1String("win32-clang-") + Abi::toString(targetAbi().osFlavor());
    return QList<Utils::FileName>{Utils::FileName::fromString(mkspec)};
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
    result.insert(llvmDirKey(), m_llvmDir);
    return result;
}

bool ClangClToolChain::fromMap(const QVariantMap &data)
{
    if (!MsvcToolChain::fromMap(data))
        return false;
    const QString llvmDir = data.value(llvmDirKey()).toString();
    if (llvmDir.isEmpty())
        return false;
    m_llvmDir = llvmDir;
    return true;
}

ToolChainConfigWidget *ClangClToolChain::configurationWidget()
{
    return new ClangClToolChainConfigWidget(this);
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
    return { Constants::C_LANGUAGE_ID, Constants::CXX_LANGUAGE_ID };
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
    for (auto language: { Constants::C_LANGUAGE_ID, Constants::CXX_LANGUAGE_ID }) {
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
static void detectCppBuildTools(QList<ToolChain *> *list)
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

#ifdef Q_OS_WIN64
    const char programFilesC[] = "ProgramFiles(x86)";
#else
    const char programFilesC[] = "ProgramFiles";
#endif
    const QString name = QStringLiteral("Microsoft Visual C++ Build Tools");
    const QString vcVarsBat = QFile::decodeName(qgetenv(programFilesC))
        + QLatin1Char('/') + name + QStringLiteral("/vcbuildtools.bat");
    if (!QFileInfo(vcVarsBat).isFile())
        return;
    const size_t count = sizeof(entries) / sizeof(entries[0]);
    for (size_t i = 0; i < count; ++i) {
        const Entry &e = entries[i];
        const Abi abi(e.architecture, Abi::WindowsOS, Abi::WindowsMsvc2015Flavor,
                      e.format, e.wordSize);
        for (auto language: { Constants::C_LANGUAGE_ID, Constants::CXX_LANGUAGE_ID }) {
            list->append(new MsvcToolChain(name + QLatin1String(e.postFix), abi,
                                           vcVarsBat, QLatin1String(e.varsBatArg),
                                           language, ToolChain::AutoDetection));
        }
    }
}

static ToolChain *findMsvcToolChain(const QList<ToolChain *> &list,
                                    unsigned char wordWidth, Abi::OSFlavor flavor)
{
    return Utils::findOrDefault(list, [wordWidth, flavor] (const ToolChain *tc)
        { const Abi abi = tc->targetAbi();
          return abi.osFlavor() == flavor
              && wordWidth == abi.wordWidth();} );
}

// Detect Clang-cl on top of MSVC2015 or MSVC2013.
static void detectClangClToolChain(QList<ToolChain *> *list)
{
#ifdef Q_OS_WIN64
    const char registryNode[] = "HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\LLVM\\LLVM";
#else
    const char registryNode[] = "HKEY_LOCAL_MACHINE\\SOFTWARE\\LLVM\\LLVM";
#endif

    const QSettings registry(QLatin1String(registryNode), QSettings::NativeFormat);
    if (registry.status() != QSettings::NoError)
        return;
    const QString path = QDir::cleanPath(registry.value(QStringLiteral(".")).toString());
    if (path.isEmpty())
        return;
    const unsigned char wordWidth = Utils::is64BitWindowsBinary(path + QStringLiteral("/bin/") + QLatin1String(clangClBinary))
        ? 64 : 32;
    const ToolChain *toolChain = findMsvcToolChain(*list, wordWidth, Abi::WindowsMsvc2015Flavor);
    if (!toolChain)
        toolChain = findMsvcToolChain(*list, wordWidth, Abi::WindowsMsvc2013Flavor);
    if (!toolChain) {
        qWarning("Unable to find a suitable MSVC version for \"%s\".", qPrintable(QDir::toNativeSeparators(path)));
        return;
    }
    const MsvcToolChain *msvcToolChain = static_cast<const MsvcToolChain *>(toolChain);
    const Abi targetAbi = msvcToolChain->targetAbi();
    const QString name = QStringLiteral("LLVM ") + QString::number(wordWidth)
        + QStringLiteral("bit based on ")
        + Abi::toString(targetAbi.osFlavor()).toUpper();
    list->append(new ClangClToolChain(name, path, targetAbi,
                                      msvcToolChain->varsBat(), msvcToolChain->varsBatArg(),
                                      Constants::CXX_LANGUAGE_ID, ToolChain::AutoDetection));
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

    detectCppBuildTools(&results);

    detectClangClToolChain(&results);

    return results;
}

bool MsvcToolChain::operator ==(const ToolChain &other) const
{
    if (!AbstractMsvcToolChain::operator ==(other))
        return false;
    const MsvcToolChain *msvcTc = static_cast<const MsvcToolChain *>(&other);
    return m_varsBatArg == msvcTc->m_varsBatArg;
}

bool MsvcToolChainFactory::canRestore(const QVariantMap &data)
{
    const Core::Id id = typeIdFromMap(data);
    return id == Constants::MSVC_TOOLCHAIN_TYPEID || id == Constants::CLANG_CL_TOOLCHAIN_TYPEID;
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
    const Core::Id id = typeIdFromMap(data);
    if (id == Constants::CLANG_CL_TOOLCHAIN_TYPEID)
        return readFromMap<ClangClToolChain>(data);
    return readFromMap<MsvcToolChain>(data);
}

} // namespace Internal
} // namespace ProjectExplorer
