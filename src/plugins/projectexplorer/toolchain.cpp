/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "toolchain.h"
#include "project.h"
#include "cesdkhandler.h"
#include "projectexplorersettings.h"
#include "gccparser.h"
#include "msvcparser.h"

#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QSettings>
#include <QtCore/QDir>
#include <QtCore/QTemporaryFile>
#include <QtCore/QString>
#include <QtCore/QCoreApplication>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

enum { debug = 0 };

#ifdef Q_OS_WIN64
static const char MSVC_RegKey[] = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\VisualStudio\\SxS\\VC7";
#else
static const char MSVC_RegKey[] = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\SxS\\VC7";
#endif

bool ToolChain::equals(ToolChain *a, ToolChain *b)
{
    if (a == b)
        return true;
    if (a == 0 || b == 0)
        return false;
    if (a->type() == b->type())
        return a->equals(b);
    return false;
}

ToolChain::ToolChain()
{
}

ToolChain::~ToolChain()
{
}

ToolChain *ToolChain::createGccToolChain(const QString &gcc)
{
    return new GccToolChain(gcc);
}

ToolChain *ToolChain::createMinGWToolChain(const QString &gcc, const QString &mingwPath)
{
    return new MinGWToolChain(gcc, mingwPath);
}

ToolChain *ToolChain::createMSVCToolChain(const QString &name, bool amd64)
{
    return MSVCToolChain::create(name, amd64);
}

ToolChain *ToolChain::createWinCEToolChain(const QString &name, const QString &platform)
{
    return WinCEToolChain::create(name, platform);
}

QStringList ToolChain::availableMSVCVersions()
{
    QStringList rc;
    foreach(const MSVCToolChain::Installation &i, MSVCToolChain::installations())
        rc.push_back(i.name);
    return rc;
}

QStringList ToolChain::availableMSVCVersions(bool amd64)
{
    QStringList rc;
    foreach(const MSVCToolChain::Installation &i, MSVCToolChain::installations())
        if (i.is64bit() == amd64)
            rc.push_back(i.name);
    return rc;
}

QList<ToolChain::ToolChainType> ToolChain::supportedToolChains()
{
    QList<ToolChain::ToolChainType> toolChains;
    for (int i = 0; i < LAST_VALID; ++i) {
        toolChains.append(ToolChainType(i));
    }
    return toolChains;
}

QString ToolChain::toolChainName(ToolChainType tc)
{
    switch (tc) {
    case GCC:
        return QCoreApplication::translate("ToolChain", "GCC");
//    case LinuxICC:
//        return QCoreApplication::translate("ToolChain", "Intel C++ Compiler (Linux)");
    case MinGW:
        return QString::fromLatin1("MinGW");
    case MSVC:
        return QCoreApplication::translate("ToolChain", "Microsoft Visual C++");
    case WINCE:
        return QCoreApplication::translate("ToolChain", "Windows CE");
    case WINSCW:
        return QCoreApplication::translate("ToolChain", "WINSCW");
    case GCCE:
        return QCoreApplication::translate("ToolChain", "GCCE");
    case GCCE_GNUPOC:
        return QCoreApplication::translate("ToolChain", "GCCE/GnuPoc");
    case RVCT_ARMV5_GNUPOC:
        return QCoreApplication::translate("ToolChain", "RVCT (ARMV6)/GnuPoc");
    case RVCT_ARMV5:
        return QCoreApplication::translate("ToolChain", "RVCT (ARMV5)");
    case RVCT_ARMV6:
        return QCoreApplication::translate("ToolChain", "RVCT (ARMV6)");
    case GCC_MAEMO:
        return QCoreApplication::translate("ToolChain", "GCC for Maemo");
    case OTHER:
        return QCoreApplication::translate("ToolChain", "Other");
    case INVALID:
        return QCoreApplication::translate("ToolChain", "<Invalid>");
    case UNKNOWN:
        break;
    default:
        Q_ASSERT("Missing name for Toolchaintype");
    };
    return QCoreApplication::translate("ToolChain", "<Unknown>");
}

GccToolChain::GccToolChain(const QString &gcc)
    : m_gcc(gcc)
{

}

ToolChain::ToolChainType GccToolChain::type() const
{
    return ToolChain::GCC;
}

QByteArray GccToolChain::predefinedMacros()
{
    if (m_predefinedMacros.isEmpty()) {
        QStringList arguments;
        arguments << QLatin1String("-xc++")
                  << QLatin1String("-E")
                  << QLatin1String("-dM")
                  << QLatin1String("-");

        QProcess cpp;
        ProjectExplorer::Environment env = ProjectExplorer::Environment::systemEnvironment();
        addToEnvironment(env);
        cpp.setEnvironment(env.toStringList());
        cpp.start(m_gcc, arguments);
        cpp.closeWriteChannel();
        cpp.waitForFinished();
        m_predefinedMacros = cpp.readAllStandardOutput();

#ifdef Q_OS_MAC
        // Turn off flag indicating Apple's blocks support
        const QByteArray blocksDefine("#define __BLOCKS__ 1");
        const QByteArray blocksUndefine("#undef __BLOCKS__");
        int idx = m_predefinedMacros.indexOf(blocksDefine);
        if (idx != -1) {
            m_predefinedMacros.replace(idx, blocksDefine.length(), blocksUndefine);
        }

        // Define __strong and __weak (used for Apple's GC extension of C) to be empty
        m_predefinedMacros.append("#define __strong\n");
        m_predefinedMacros.append("#define __weak\n");
#endif // Q_OS_MAC
    }
    return m_predefinedMacros;
}

QList<HeaderPath> GccToolChain::systemHeaderPaths()
{
    if (m_systemHeaderPaths.isEmpty()) {
        QStringList arguments;
        arguments << QLatin1String("-xc++")
                  << QLatin1String("-E")
                  << QLatin1String("-v")
                  << QLatin1String("-");

        QProcess cpp;
        ProjectExplorer::Environment env = ProjectExplorer::Environment::systemEnvironment();
        addToEnvironment(env);
        env.set(QLatin1String("LC_ALL"), QLatin1String("C"));   //override current locale settings
        cpp.setEnvironment(env.toStringList());
        cpp.setReadChannelMode(QProcess::MergedChannels);
        cpp.start(m_gcc, arguments);
        cpp.closeWriteChannel();
        cpp.waitForFinished();

        QByteArray line;
        while (cpp.canReadLine()) {
            line = cpp.readLine();
            if (line.startsWith("#include"))
                break;
        }

        if (! line.isEmpty() && line.startsWith("#include")) {
            HeaderPath::Kind kind = HeaderPath::UserHeaderPath;
            while (cpp.canReadLine()) {
                line = cpp.readLine();
                if (line.startsWith("#include")) {
                    kind = HeaderPath::GlobalHeaderPath;
                } else if (! line.isEmpty() && QChar(line.at(0)).isSpace()) {
                    HeaderPath::Kind thisHeaderKind = kind;

                    line = line.trimmed();
                    if (line.endsWith('\n'))
                        line.chop(1);

                    int index = line.indexOf(" (framework directory)");
                    if (index != -1) {
                        line = line.left(index);
                        thisHeaderKind = HeaderPath::FrameworkHeaderPath;
                    }

                    m_systemHeaderPaths.append(HeaderPath(QFile::decodeName(line), thisHeaderKind));
                } else if (line.startsWith("End of search list.")) {
                    break;
                } else {
                    qWarning() << "ignore line:" << line;
                }
            }
        }
    }
    return m_systemHeaderPaths;
}

void GccToolChain::addToEnvironment(ProjectExplorer::Environment &env)
{
    Q_UNUSED(env)
}

QString GccToolChain::makeCommand() const
{
    return QLatin1String("make");
}

IOutputParser *GccToolChain::outputParser() const
{
    return new GccParser;
}

bool GccToolChain::equals(ToolChain *other) const
{
    return (m_gcc == static_cast<GccToolChain *>(other)->m_gcc);
}

MinGWToolChain::MinGWToolChain(const QString &gcc, const QString &mingwPath)
    : GccToolChain(gcc), m_mingwPath(mingwPath)
{

}

ToolChain::ToolChainType MinGWToolChain::type() const
{
    return ToolChain::MinGW;
}

bool MinGWToolChain::equals(ToolChain *other) const
{
    MinGWToolChain *o = static_cast<MinGWToolChain *>(other);
    return (m_mingwPath == o->m_mingwPath && this->GccToolChain::equals(other));
}

void MinGWToolChain::addToEnvironment(ProjectExplorer::Environment &env)
{
    if (debug)
        qDebug() << "MinGWToolChain::addToEnvironment" << m_mingwPath;
    if (m_mingwPath.isEmpty())
        return;
    const QString binDir = m_mingwPath + "/bin";
    if (QFileInfo(binDir).exists())
        env.prependOrSetPath(binDir);
}

QString MinGWToolChain::makeCommand() const
{
    return QLatin1String("mingw32-make.exe");
}

IOutputParser *MinGWToolChain::outputParser() const
{
    return new GccParser;
}

// ---------------- MSVC installation location code

// Format the name of an SDK or VC installation version with platform
static inline QString installationName(const QString &name,
                                       MSVCToolChain::Installation::Type t,
                                       MSVCToolChain::Installation::Platform p)
{
    if (t == MSVCToolChain::Installation::WindowsSDK) {
        QString sdkName = name;
        sdkName += QLatin1String(" (");
        sdkName += MSVCToolChain::Installation::platformName(p);
        sdkName += QLatin1Char(')');
        return sdkName;
    }
    // Comes as "9.0" from the registry
    QString vcName = QLatin1String("Microsoft Visual C++ Compilers ");
    vcName += name;
    vcName+= QLatin1String(" (");
    vcName += MSVCToolChain::Installation::platformName(p);
    vcName += QLatin1Char(')');
    return vcName;
}

MSVCToolChain::Installation::Installation(Type t, const QString &n, Platform p,
                                          const QString &v, const QString &a) :
    type(t), name(installationName(n, t, p)), platform(p), varsBat(v), varsBatArg(a)
{
}

MSVCToolChain::Installation::Installation() : platform(s32)
{
}

QString MSVCToolChain::Installation::platformName(Platform t)
{
    switch (t) {
    case s32:
        return QLatin1String("x86");
    case s64:
        return QLatin1String("x64");
    case ia64:
        return QLatin1String("ia64");
    case amd64:
        return QLatin1String("amd64");
    }
    return QString();
}

bool MSVCToolChain::Installation::is64bit() const
{
    return platform != s32;
}

MSVCToolChain::InstallationList MSVCToolChain::installations()
{
    static InstallationList installs;
    static bool firstTime = true;
    if (firstTime) {
        firstTime = false;
        // 1) Installed SDKs preferred over standalone Visual studio
        const char sdk_RegKeyC[] = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows";
        const QSettings sdkRegistry(sdk_RegKeyC, QSettings::NativeFormat);
        const QString defaultSdkPath = sdkRegistry.value(QLatin1String("CurrentInstallFolder")).toString();
        if (!defaultSdkPath.isEmpty()) {
            foreach(const QString &sdkKey, sdkRegistry.childGroups()) {
                const QString name = sdkRegistry.value(sdkKey + QLatin1String("/ProductName")).toString();
                const QString folder = sdkRegistry.value(sdkKey + QLatin1String("/InstallationFolder")).toString();
                if (!folder.isEmpty()) {
                    const QString sdkVcVarsBat = folder + QLatin1String("bin\\SetEnv.cmd");
                    if (QFileInfo(sdkVcVarsBat).exists()) {
                        // Add all platforms
                        InstallationList newInstalls;
                        newInstalls.push_back(Installation(Installation::WindowsSDK, name, Installation::s32, sdkVcVarsBat, QLatin1String("/x86")));
#ifdef Q_OS_WIN64
                        newInstalls.push_back(Installation(Installation::WindowsSDK, name, Installation::s64, sdkVcVarsBat, QLatin1String("/x64")));
                        newInstalls.push_back(Installation(Installation::WindowsSDK, name, Installation::ia64, sdkVcVarsBat, QLatin1String("/ia64")));
#endif
                        // Make sure the default is front.
                        if (folder == defaultSdkPath && !installs.empty()) {
                            const InstallationList old = installs;
                            installs = newInstalls + old;
                        } else {
                            installs.append(newInstalls);
                        }
                    } // bat exists
                } // folder
            } // foreach
        }
        // 2) Installed MSVCs
        const QSettings vsRegistry(MSVC_RegKey, QSettings::NativeFormat);
        foreach(const QString &vsName, vsRegistry.allKeys()) {
            if (vsName.contains(QLatin1Char('.'))) { // Scan for version major.minor
                const QString path = vsRegistry.value(vsName).toString();
                // Check existence of various install scripts
                const QString vcvars32bat = path + QLatin1String("bin\\vcvars32.bat");
                if (QFileInfo(vcvars32bat).isFile())
                    installs.push_back(Installation(Installation::VS, vsName, Installation::s32, vcvars32bat));
                // Amd 64 is the preferred 64bit platform
                const QString vcvarsAmd64bat = path + QLatin1String("bin\\amd64\\vcvarsamd64.bat");
                if (QFileInfo(vcvarsAmd64bat).isFile())
                    installs.push_back(Installation(Installation::VS, vsName, Installation::amd64, vcvarsAmd64bat));
                const QString vcvarsAmd64bat2 = path + QLatin1String("bin\\vcvarsx86_amd64.bat");
                if (QFileInfo(vcvarsAmd64bat2).isFile())
                    installs.push_back(Installation(Installation::VS, vsName, Installation::amd64, vcvarsAmd64bat2));
                const QString vcvars64bat = path + QLatin1String("bin\\vcvars64.bat");
                if (QFileInfo(vcvars64bat).isFile())
                    installs.push_back(Installation(Installation::VS, vsName, Installation::s64, vcvars64bat));
                const QString vcvarsIA64bat = path + QLatin1String("bin\\vcvarsx86_ia64.bat");
                if (QFileInfo(vcvarsIA64bat).isFile())
                    installs.push_back(Installation(Installation::VS, vsName, Installation::ia64, vcvarsIA64bat));
            }
        }
    }
    if (debug)
        foreach(const Installation &i, installs)
            qDebug() << i;
    return installs;
}

MSVCToolChain::Installation MSVCToolChain::findInstallation(bool is64Bit,
                                                            const QString &name,
                                                            bool excludeSDK)
{
    if (debug)
        qDebug() << "find" << (is64Bit ? 64 : 32) << name << excludeSDK;
    foreach(const Installation &i, installations()) {
        if (i.type != Installation::WindowsSDK || !excludeSDK) {
            if ((i.is64bit() == is64Bit) && (name.isEmpty() || name == i.name))
                return i;
        }
    }
    return Installation();
}

namespace ProjectExplorer {
PROJECTEXPLORER_EXPORT QDebug operator<<(QDebug in, const MSVCToolChain::Installation &i)
{
    QDebug nsp = in.nospace();
    nsp << "Type: " << i.type << " Platform: " << i.platform << " Name: " << i.name
        << "\nSetup: " << i.varsBat;
    if (!i.varsBatArg.isEmpty())
        nsp << "\nSetup argument: " << i.varsBatArg;
    return in;
}
}

MSVCToolChain *MSVCToolChain::create(const QString &name, bool amd64)
{
    return new MSVCToolChain(MSVCToolChain::findInstallation(amd64, name));
}

MSVCToolChain::MSVCToolChain(const Installation &in) :
    m_installation(in),
    m_valuesSet(false)
{
    if (debug)
        qDebug() << "\nMSVCToolChain::CT\n" << m_installation;
}

ToolChain::ToolChainType MSVCToolChain::type() const
{
    return ToolChain::MSVC;
}

bool MSVCToolChain::equals(ToolChain *other) const
{
    MSVCToolChain *o = static_cast<MSVCToolChain *>(other);
    return (m_installation.name == o->m_installation.name);
}

QByteArray msvcCompilationFile() {
    static const char* macros[] = {"_ATL_VER", "_CHAR_UNSIGNED", "__CLR_VER",
                                   "__cplusplus_cli", "__COUNTER__", "__cplusplus",
                                   "_CPPLIB_VER", "_CPPRTTI", "_CPPUNWIND",
                                   "_DEBUG", "_DLL", "__FUNCDNAME__",
                                   "__FUNCSIG__","__FUNCTION__","_INTEGRAL_MAX_BITS",
                                   "_M_ALPHA","_M_CEE","_M_CEE_PURE",
                                   "_M_CEE_SAFE","_M_IX86","_M_IA64",
                                   "_M_IX86_FP","_M_MPPC","_M_MRX000",
                                   "_M_PPC","_M_X64","_MANAGED",
                                   "_MFC_VER","_MSC_BUILD", /* "_MSC_EXTENSIONS", */
                                   "_MSC_FULL_VER","_MSC_VER","__MSVC_RUNTIME_CHECKS",
                                   "_MT", "_NATIVE_WCHAR_T_DEFINED", "_OPENMP",
                                   "_VC_NODEFAULTLIB", "_WCHAR_T_DEFINED", "_WIN32",
                                   "_WIN32_WCE", "_WIN64", "_Wp64", "__DATE__",
                                    "__DATE__", "__TIME__", "__TIMESTAMP__",
                                   0};
    QByteArray file = "#define __PPOUT__(x) V##x=x\n\n";
    int i =0;
    while (macros[i] != 0) {
        const QByteArray macro(macros[i]);
        file += "#if defined(" + macro + ")\n__PPOUT__("
                + macro + ")\n#endif\n";
        ++i;
    }
    file += "\nvoid main(){}\n\n";
    return file;
}

QByteArray MSVCToolChain::predefinedMacros()
{
    if (m_predefinedMacros.isEmpty()) {
        m_predefinedMacros += "#define __MSVCRT__\n"
                              "#define __w64\n"
                              "#define __int64 long long\n"
                              "#define __int32 long\n"
                              "#define __int16 short\n"
                              "#define __int8 char\n"
                              "#define __ptr32\n"
                              "#define __ptr64\n";

        QString tmpFilePath;
        {
            // QTemporaryFile is buggy and will not unlock the file for cl.exe
            QTemporaryFile tmpFile(QDir::tempPath()+"/envtestXXXXXX.cpp");
            tmpFile.setAutoRemove(false);
            if (!tmpFile.open())
                return m_predefinedMacros;
            tmpFilePath = QFileInfo(tmpFile).canonicalFilePath();
            tmpFile.write(msvcCompilationFile());
            tmpFile.close();
        }
        ProjectExplorer::Environment env = ProjectExplorer::Environment::systemEnvironment();
        addToEnvironment(env);
        QProcess cpp;
        cpp.setEnvironment(env.toStringList());
        cpp.setWorkingDirectory(QDir::tempPath());
        QStringList arguments;
        arguments << "/EP" << QDir::toNativeSeparators(tmpFilePath);
        cpp.start(QLatin1String("cl.exe"), arguments);
        cpp.closeWriteChannel();
        cpp.waitForFinished();
        QList<QByteArray> output = cpp.readAllStandardOutput().split('\n');
        foreach (const QByteArray& line, output) {
            if (line.startsWith('V')) {
                QList<QByteArray> split = line.split('=');
                QByteArray key = split.at(0).mid(1);
                QByteArray value = split.at(1);
                if (!value.isEmpty()) {
                    value.chop(1); //remove '\n'
                }
                QByteArray newDefine = "#define " + key + ' ' + value + '\n';
                m_predefinedMacros.append(newDefine);
            }
        }
        QFile::remove(tmpFilePath);
    }
    return m_predefinedMacros;
}

QList<HeaderPath> MSVCToolChain::systemHeaderPaths()
{
    //TODO fix this code
    ProjectExplorer::Environment env = ProjectExplorer::Environment::systemEnvironment();
    addToEnvironment(env);
    QList<HeaderPath> headerPaths;
    foreach(const QString &path, env.value("INCLUDE").split(QLatin1Char(';'))) {
        headerPaths.append(HeaderPath(path, HeaderPath::GlobalHeaderPath));
    }
    return headerPaths;
}

MSVCToolChain::StringStringPairList MSVCToolChain::readEnvironmentSetting(const QString &varsBat,
                                                                          const QStringList &args,
                                                                          const ProjectExplorer::Environment &env)
{
    const StringStringPairList rc = readEnvironmentSettingI(varsBat, args, env);
    if (debug) {
        qDebug() << "Running: " << varsBat << args;
        if (debug > 1) {
            qDebug() << "Incoming: " << env.toStringList();
            foreach(const StringStringPair &e, rc)
                qDebug() << e.first << e.second;
        } else {
            qDebug() << "Read: " << rc.size() << " variables.";
        }
    }
    return rc;
}

// Windows: Expand the delayed evaluation references returned by the
// SDK setup scripts: "PATH=!Path!;foo". Some values might expand
// to empty and should not be added
static inline QString winExpandDelayedEnvReferences(QString in, const ProjectExplorer::Environment &env)
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

MSVCToolChain::StringStringPairList MSVCToolChain::readEnvironmentSettingI(const QString &varsBat,
                                                                           const QStringList &args,
                                                                           const ProjectExplorer::Environment &env)
{
    // Run the setup script and extract the variables
    if (!QFileInfo(varsBat).exists())
        return StringStringPairList();
    const QString tempOutputFileName = QDir::tempPath() + QLatin1String("\\qtcreator-msvc-environment.txt");
    QTemporaryFile tf(QDir::tempPath() + "\\XXXXXX.bat");
    tf.setAutoRemove(true);
    if (!tf.open())
        return StringStringPairList();
    const QString filename = tf.fileName();
    QByteArray call = "call \"";
    call +=  varsBat.toLocal8Bit();
    call += '"';
    if (!args.isEmpty()) {
        call += ' ';
        call += args.join(QString(QLatin1Char(' '))).toLocal8Bit();
    }
    call += "\r\n";
    tf.write(call);
    QString redirect = "set > \"" + tempOutputFileName + "\"\r\n";
    tf.write(redirect.toLocal8Bit());
    tf.flush();
    tf.waitForBytesWritten(30000);

    QProcess run;
    run.setEnvironment(env.toStringList());
    const QString cmdPath = QString::fromLocal8Bit(qgetenv("COMSPEC"));
    run.start(cmdPath, QStringList()<< QLatin1String("/c")<<filename);
    if (!run.waitForStarted() || !run.waitForFinished())
        return StringStringPairList();
    tf.close();

    QFile varsFile(tempOutputFileName);
    if (!varsFile.open(QIODevice::ReadOnly|QIODevice::Text))
        return StringStringPairList();

    QRegExp regexp(QLatin1String("(\\w*)=(.*)"));
    StringStringPairList rc;
    while (!varsFile.atEnd()) {
        const QString line = QString::fromLocal8Bit(varsFile.readLine()).trimmed();
        if (regexp.exactMatch(line)) {
            const QString varName = regexp.cap(1);
            const QString expandedValue = winExpandDelayedEnvReferences(regexp.cap(2), env);
            if (!expandedValue.isEmpty())
                rc.append(StringStringPair(varName, expandedValue));
        }
    }
    varsFile.close();
    varsFile.remove();
    return rc;
}

void MSVCToolChain::addToEnvironment(ProjectExplorer::Environment &env)
{
    if (debug)
        qDebug() << "MSVCToolChain::addToEnvironment" << m_installation.name;
    if (m_installation.name.isEmpty() || m_installation.varsBat.isEmpty()) {
        qWarning("Attempt to set up invalid MSVC Toolchain.");
        return;
    }
    // We cache the full environment (incoming + modifications by setup script).
    if (!m_valuesSet || env != m_lastEnvironment) {
        m_lastEnvironment = env;
        const QStringList args = m_installation.varsBatArg.isEmpty() ?
                                 QStringList() :  QStringList(m_installation.varsBatArg);
        m_values = readEnvironmentSetting(m_installation.varsBat, args, env);
        m_valuesSet = true;
    }

    const StringStringPairList::const_iterator end = m_values.constEnd();
    for (StringStringPairList::const_iterator it = m_values.constBegin(); it != end; ++it)
        env.set((*it).first, (*it).second);
}

QString MSVCToolChain::makeCommand() const
{
    if (ProjectExplorerPlugin::instance()->projectExplorerSettings().useJom) {
        // We want jom! Try to find it.
        QString jom = QCoreApplication::applicationDirPath() + QLatin1String("/jom.exe");
        if (QFileInfo(jom).exists())
            return jom;
        else
            return QLatin1String("jom.exe");
    }
    return QLatin1String("nmake.exe");
}

IOutputParser *MSVCToolChain::outputParser() const
{
    return new MsvcParser;
}

WinCEToolChain *WinCEToolChain::create(const QString &name, const QString &platform)
{
    const bool excludeSDK = true;
    return new WinCEToolChain(findInstallation(false, name, excludeSDK), platform);
}

WinCEToolChain::WinCEToolChain(const Installation &in, const QString &platform) :
        MSVCToolChain(in),
        m_platform(platform)
{
}

ToolChain::ToolChainType WinCEToolChain::type() const
{
    return ToolChain::WINCE;
}

bool WinCEToolChain::equals(ToolChain *other) const
{
    WinCEToolChain *o = static_cast<WinCEToolChain *>(other);
    return (m_platform == o->m_platform && this->MSVCToolChain::equals(other));
}

QByteArray WinCEToolChain::predefinedMacros()
{
    //TODO
    return MSVCToolChain::predefinedMacros();
}

QList<HeaderPath> WinCEToolChain::systemHeaderPaths()
{
    //TODO fix this code
    ProjectExplorer::Environment env = ProjectExplorer::Environment::systemEnvironment();
    addToEnvironment(env);

    QList<HeaderPath> headerPaths;

    const QStringList includes = env.value("INCLUDE").split(QLatin1Char(';'));

    foreach (const QString &path, includes) {
        const HeaderPath headerPath(path, HeaderPath::GlobalHeaderPath);
        headerPaths.append(headerPath);
    }

    return headerPaths;
}

void WinCEToolChain::addToEnvironment(ProjectExplorer::Environment &env)
{
    MSVCToolChain::addToEnvironment(env);
    QSettings registry(MSVC_RegKey, QSettings::NativeFormat);
    QString path = registry.value(m_installation.name).toString();

    // Find MSVC path

    path += QLatin1Char('/');

    // Find Platform name
    CeSdkHandler cesdkhandler;
    cesdkhandler.parse(path);
    cesdkhandler.find(m_platform).addToEnvironment(env);
}
