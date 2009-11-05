/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtCore/QDir>
#include <QtCore/QTemporaryFile>
#include <QtCore/QString>
#include <QtCore/QCoreApplication>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

#ifdef Q_OS_WIN64
static const char * MSVC_RegKey = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\VisualStudio\\SxS\\VC7";
#else
static const char * MSVC_RegKey = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\SxS\\VC7";
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

ToolChain *ToolChain::createMSVCToolChain(const QString &name, bool amd64 = false)
{
    return new MSVCToolChain(name, amd64);
}

ToolChain *ToolChain::createWinCEToolChain(const QString &name, const QString &platform)
{
    return new WinCEToolChain(name, platform);
}

QStringList ToolChain::availableMSVCVersions()
{
    QSettings registry(MSVC_RegKey, QSettings::NativeFormat);
    QStringList versions = registry.allKeys();
    // qDebug() << "AVAILABLE MSVC VERSIONS:" << versions << "at" << MSVC_RegKey;
    return versions;
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
    return "make";
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
    //qDebug()<<"MinGWToolChain::addToEnvironment";
    if (m_mingwPath.isEmpty())
        return;
    QString binDir = m_mingwPath + "/bin";
    if (QFileInfo(binDir).exists())
        env.prependOrSetPath(binDir);
//    if (QFileInfo(binDir).exists())
//        qDebug()<<"Adding "<<binDir<<" to the PATH";
}

QString MinGWToolChain::makeCommand() const
{
    return "mingw32-make.exe";
}


MSVCToolChain::MSVCToolChain(const QString &name, bool amd64)
    : m_name(name), m_valuesSet(false), m_amd64(amd64)
{
    if (m_name.isEmpty()) { // Could be because system qt doesn't set this
        QSettings registry(MSVC_RegKey, QSettings::NativeFormat);
        QStringList keys = registry.allKeys();
        if (keys.count())
            m_name = keys.first();
    }
}

ToolChain::ToolChainType MSVCToolChain::type() const
{
    return ToolChain::MSVC;
}

bool MSVCToolChain::equals(ToolChain *other) const
{
    MSVCToolChain *o = static_cast<MSVCToolChain *>(other);
    return (m_name == o->m_name);
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
                                   "_MFC_VER","_MSC_BUILD","_MSC_EXTENSIONS",
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
                QByteArray newDefine = "#define " + key + " " + value + '\n';
                m_predefinedMacros.append(newDefine);
            }            
        }
        QFile::remove(tmpFilePath);
    }
    //qDebug() << m_predefinedMacros;
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

void MSVCToolChain::addToEnvironment(ProjectExplorer::Environment &env)
{
    if (!m_valuesSet || env != m_lastEnvironment) {
        m_lastEnvironment = env;
        QSettings registry(MSVC_RegKey, QSettings::NativeFormat);
        if (m_name.isEmpty())
            return;
        QString path = registry.value(m_name).toString();
        QString desc;
        QString varsbat;
        if (m_amd64)
            varsbat = path + "bin\\amd64\\vcvarsamd64.bat";
        else
            varsbat = path + "bin\\vcvars32.bat";
        //        qDebug() << varsbat;
        if (QFileInfo(varsbat).exists()) {
            QTemporaryFile tf(QDir::tempPath() + "\\XXXXXX.bat");
            if (!tf.open())
                return;
            QString filename = tf.fileName();
            tf.write("call \"" + varsbat.toLocal8Bit()+"\"\r\n");
            tf.write(("set > \"" + QDir::tempPath() + "\\qtcreator-msvc-environment.txt\"\r\n").toLocal8Bit());
            tf.flush();
            tf.waitForBytesWritten(30000);

            QProcess run;
            run.setEnvironment(env.toStringList());
            QString cmdPath = env.searchInPath("cmd");
            run.start(cmdPath, QStringList()<<"/c"<<filename);
            run.waitForFinished();
            tf.close();

            QFile vars(QDir::tempPath() + "\\qtcreator-msvc-environment.txt");
            if (vars.exists() && vars.open(QIODevice::ReadOnly)) {
                while (!vars.atEnd()) {
                    QByteArray line = vars.readLine();
                    QString line2 = QString::fromLocal8Bit(line);
                    line2 = line2.trimmed();
                    QRegExp regexp("(\\w*)=(.*)");
                    if (regexp.exactMatch(line2)) {
                        QString variable = regexp.cap(1);
                        QString value = regexp.cap(2);
                        m_values.append(QPair<QString, QString>(variable, value));
                    }
                }
                vars.close();
                vars.remove();
            }
        }
        m_valuesSet = true;
    }

    QList< QPair<QString, QString> >::const_iterator it, end;
    end = m_values.constEnd();
    for (it = m_values.constBegin(); it != end; ++it) {
        env.set((*it).first, (*it).second);
    }
}

QString MSVCToolChain::makeCommand() const
{
    if (ProjectExplorerPlugin::instance()->projectExplorerSettings().useJom) {
        // We want jom! Try to find it.
        QString jom = QCoreApplication::applicationDirPath() + QLatin1String("/jom.exe");
        if (QFileInfo(jom).exists())
            return jom;
        else
            return "jom.exe";
    }
    return "nmake.exe";
}

WinCEToolChain::WinCEToolChain(const QString &name, const QString &platform)
    : MSVCToolChain(name), m_platform(platform)
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
    QString path = registry.value(m_name).toString();

    // Find MSVC path

    path += "/";

//        qDebug()<<"MSVC path"<<path;
//        qDebug()<<"looking for platform name in"<< path() + "/mkspecs/" + mkspec() +"/qmake.conf";
    // Find Platform name
//        qDebug()<<"Platform Name"<<m_platform;

    CeSdkHandler cesdkhandler;
    cesdkhandler.parse(path);
    cesdkhandler.find(m_platform).addToEnvironment(env);
    //qDebug()<<"WinCE Final Environment:";
    //qDebug()<<env.toStringList();
}
