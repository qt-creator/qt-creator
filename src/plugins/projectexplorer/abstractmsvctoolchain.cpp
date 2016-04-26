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

#include "abstractmsvctoolchain.h"

#include "msvcparser.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorersettings.h>

#include <utils/qtcprocess.h>
#include <utils/synchronousprocess.h>

#include <QDir>
#include <QTemporaryFile>

enum { debug = 0 };

namespace ProjectExplorer {
namespace Internal {


AbstractMsvcToolChain::AbstractMsvcToolChain(Core::Id typeId,
                                             Detection d,
                                             const Abi &abi,
                                             const QString& vcvarsBat) :
    ToolChain(typeId, d),
    m_lastEnvironment(Utils::Environment::systemEnvironment()),
    m_abi(abi),
    m_vcvarsBat(vcvarsBat)
{
    Q_ASSERT(abi.os() == Abi::WindowsOS);
    Q_ASSERT(abi.binaryFormat() == Abi::PEFormat);
    Q_ASSERT(abi.osFlavor() != Abi::WindowsMSysFlavor);
    Q_ASSERT(!m_vcvarsBat.isEmpty());
}

AbstractMsvcToolChain::AbstractMsvcToolChain(Core::Id typeId, Detection d) :
    ToolChain(typeId, d),
    m_lastEnvironment(Utils::Environment::systemEnvironment())
{ }

Abi AbstractMsvcToolChain::targetAbi() const
{
    return m_abi;
}

bool AbstractMsvcToolChain::isValid() const
{
    if (m_vcvarsBat.isEmpty())
        return false;
    QFileInfo fi(m_vcvarsBat);
    return fi.isFile() && fi.isExecutable();
}

QByteArray AbstractMsvcToolChain::predefinedMacros(const QStringList &cxxflags) const
{
    if (m_predefinedMacros.isEmpty()) {
        Utils::Environment env(m_lastEnvironment);
        addToEnvironment(env);
        m_predefinedMacros = msvcPredefinedMacros(cxxflags, env);
    }
    return m_predefinedMacros;
}

ToolChain::CompilerFlags AbstractMsvcToolChain::compilerFlags(const QStringList &cxxflags) const
{
    CompilerFlags flags(MicrosoftExtensions);
    if (cxxflags.contains(QLatin1String("/openmp")))
        flags |= OpenMP;

    // see http://msdn.microsoft.com/en-us/library/0k0w269d%28v=vs.71%29.aspx
    if (cxxflags.contains(QLatin1String("/Za")))
        flags &= ~MicrosoftExtensions;

    if (m_abi.osFlavor() == Abi::WindowsMsvc2010Flavor
            || m_abi.osFlavor() == Abi::WindowsMsvc2012Flavor
            || m_abi.osFlavor() == Abi::WindowsMsvc2013Flavor
            || m_abi.osFlavor() == Abi::WindowsMsvc2015Flavor)
        flags |= StandardCxx11;

    return flags;
}

/**
 * Converts MSVC warning flags to clang flags.
 * @see http://msdn.microsoft.com/en-us/library/thxezb7y.aspx
 */
WarningFlags AbstractMsvcToolChain::warningFlags(const QStringList &cflags) const
{
    WarningFlags flags = WarningFlags::NoWarnings;
    foreach (QString flag, cflags) {
        if (!flag.isEmpty() && flag[0] == QLatin1Char('-'))
            flag[0] = QLatin1Char('/');

        if (flag == QLatin1String("/WX"))
            flags |= WarningFlags::AsErrors;
        else if (flag == QLatin1String("/W0") || flag == QLatin1String("/w"))
            inferWarningsForLevel(0, flags);
        else if (flag == QLatin1String("/W1"))
            inferWarningsForLevel(1, flags);
        else if (flag == QLatin1String("/W2"))
            inferWarningsForLevel(2, flags);
        else if (flag == QLatin1String("/W3") || flag == QLatin1String("/W4") || flag == QLatin1String("/Wall"))
            inferWarningsForLevel(3, flags);

        WarningFlagAdder add(flag, flags);
        if (add.triggered())
            continue;
        // http://msdn.microsoft.com/en-us/library/ay4h0tc9.aspx
        add(4263, WarningFlags::OverloadedVirtual);
        // http://msdn.microsoft.com/en-us/library/ytxde1x7.aspx
        add(4230, WarningFlags::IgnoredQualfiers);
        // not exact match, http://msdn.microsoft.com/en-us/library/0hx5ckb0.aspx
        add(4258, WarningFlags::HiddenLocals);
        // http://msdn.microsoft.com/en-us/library/wzxffy8c.aspx
        add(4265, WarningFlags::NonVirtualDestructor);
        // http://msdn.microsoft.com/en-us/library/y92ktdf2%28v=vs.90%29.aspx
        add(4018, WarningFlags::SignedComparison);
        // http://msdn.microsoft.com/en-us/library/w099eeey%28v=vs.90%29.aspx
        add(4068, WarningFlags::UnknownPragma);
        // http://msdn.microsoft.com/en-us/library/26kb9fy0%28v=vs.80%29.aspx
        add(4100, WarningFlags::UnusedParams);
        // http://msdn.microsoft.com/en-us/library/c733d5h9%28v=vs.90%29.aspx
        add(4101, WarningFlags::UnusedLocals);
        // http://msdn.microsoft.com/en-us/library/xb1db44s%28v=vs.90%29.aspx
        add(4189, WarningFlags::UnusedLocals);
        // http://msdn.microsoft.com/en-us/library/ttcz0bys%28v=vs.90%29.aspx
        add(4996, WarningFlags::Deprecated);
    }
    return flags;
}

QList<HeaderPath> AbstractMsvcToolChain::systemHeaderPaths(const QStringList &cxxflags, const Utils::FileName &sysRoot) const
{
    Q_UNUSED(cxxflags);
    Q_UNUSED(sysRoot);
    if (m_headerPaths.isEmpty()) {
        Utils::Environment env(m_lastEnvironment);
        addToEnvironment(env);
        foreach (const QString &path, env.value(QLatin1String("INCLUDE")).split(QLatin1Char(';')))
            m_headerPaths.append(HeaderPath(path, HeaderPath::GlobalHeaderPath));
    }
    return m_headerPaths;
}

void AbstractMsvcToolChain::addToEnvironment(Utils::Environment &env) const
{
    // We cache the full environment (incoming + modifications by setup script).
    if (!m_resultEnvironment.size() || env != m_lastEnvironment) {
        if (debug)
            qDebug() << "addToEnvironment: " << displayName();
        m_lastEnvironment = env;
        m_resultEnvironment = readEnvironmentSetting(env);
    }
    env = m_resultEnvironment;
}

QString AbstractMsvcToolChain::makeCommand(const Utils::Environment &environment) const
{
    bool useJom = ProjectExplorerPlugin::projectExplorerSettings().useJom;
    const QString jom = QLatin1String("jom.exe");
    const QString nmake = QLatin1String("nmake.exe");
    Utils::FileName tmp;

    if (useJom) {
        tmp = environment.searchInPath(jom, QStringList()
                                       << QCoreApplication::applicationDirPath());
        if (!tmp.isEmpty())
            return tmp.toString();
    }
    tmp = environment.searchInPath(nmake);
    if (!tmp.isEmpty())
        return tmp.toString();

    // Nothing found :(
    return useJom ? jom : nmake;
}

Utils::FileName AbstractMsvcToolChain::compilerCommand() const
{
    Utils::Environment env = Utils::Environment::systemEnvironment();
    addToEnvironment(env);
    return env.searchInPath(QLatin1String("cl.exe"));
}

IOutputParser *AbstractMsvcToolChain::outputParser() const
{
    return new MsvcParser;
}

bool AbstractMsvcToolChain::canClone() const
{
    return true;
}

QByteArray AbstractMsvcToolChain::msvcPredefinedMacros(const QStringList cxxflags,
                                                       const Utils::Environment& env) const
{
    Q_UNUSED(cxxflags);
    Q_UNUSED(env);

    static const QByteArray predefinedMacros(
            "#define __MSVCRT__\n"
            "#define __w64\n"
            "#define __int64 long long\n"
            "#define __int32 long\n"
            "#define __int16 short\n"
            "#define __int8 char\n"
            "#define __ptr32\n"
            "#define __ptr64\n");

    return predefinedMacros;
}


bool AbstractMsvcToolChain::generateEnvironmentSettings(Utils::Environment &env,
                                                        const QString &batchFile,
                                                        const QString &batchArgs,
                                                        QMap<QString, QString> &envPairs)
{
    const QByteArray marker = "####################\r\n";
    // Create a temporary file name for the output. Use a temporary file here
    // as I don't know another way to do this in Qt...
    // Note, can't just use a QTemporaryFile all the way through as it remains open
    // internally so it can't be streamed to later.

    // Create a batch file to create and save the env settings
    Utils::TempFileSaver saver(QDir::tempPath() + QLatin1String("/XXXXXX.bat"));

    QByteArray call = "call ";
    call += Utils::QtcProcess::quoteArg(batchFile).toLocal8Bit();
    if (!batchArgs.isEmpty()) {
        call += ' ';
        call += batchArgs.toLocal8Bit();
    }
    saver.write(call + "\r\n");
    saver.write("@echo " + marker);
    saver.write("set\r\n");
    saver.write("@echo " + marker);
    if (!saver.finalize()) {
        qWarning("%s: %s", Q_FUNC_INFO, qPrintable(saver.errorString()));
        return false;
    }

    Utils::QtcProcess run;
    // As of WinSDK 7.1, there is logic preventing the path from being set
    // correctly if "ORIGINALPATH" is already set. That can cause problems
    // if Creator is launched within a session set up by setenv.cmd.
    env.unset(QLatin1String("ORIGINALPATH"));
    run.setEnvironment(env);
    Utils::FileName cmdPath = Utils::FileName::fromUserInput(QString::fromLocal8Bit(qgetenv("COMSPEC")));
    if (cmdPath.isEmpty())
        cmdPath = env.searchInPath(QLatin1String("cmd.exe"));
    // Windows SDK setup scripts require command line switches for environment expansion.
    QString cmdArguments = QLatin1String(" /E:ON /V:ON /c \"");
    cmdArguments += QDir::toNativeSeparators(saver.fileName());
    cmdArguments += QLatin1Char('"');
    run.setCommand(cmdPath.toString(), cmdArguments);
    if (debug)
        qDebug() << "readEnvironmentSetting: " << call << cmdPath << cmdArguments
                 << " Env: " << env.size();
    run.start();

    if (!run.waitForStarted()) {
        qWarning("%s: Unable to run '%s': %s", Q_FUNC_INFO, qPrintable(batchFile),
                 qPrintable(run.errorString()));
        return false;
    }
    if (!run.waitForFinished()) {
        qWarning("%s: Timeout running '%s'", Q_FUNC_INFO, qPrintable(batchFile));
        Utils::SynchronousProcess::stopProcess(run);
        return false;
    }
    // The SDK/MSVC scripts do not return exit codes != 0. Check on stdout.
    QByteArray stdOut = run.readAllStandardOutput();
    if (!stdOut.isEmpty() && (stdOut.contains("Unknown") || stdOut.contains("Error")))
        qWarning("%s: '%s' reports:\n%s", Q_FUNC_INFO, call.constData(), stdOut.constData());

    //
    // Now parse the file to get the environment settings
    int start = stdOut.indexOf(marker);
    if (start == -1) {
        qWarning("Could not find start marker in stdout output.");
        return false;
    }

    stdOut = stdOut.mid(start + marker.size());

    int end = stdOut.indexOf(marker);
    if (end == -1) {
        qWarning("Could not find end marker in stdout output.");
        return false;
    }

    stdOut = stdOut.left(end);

    QStringList lines = QString::fromLocal8Bit(stdOut).split(QLatin1String("\r\n"));
    QRegExp regexp(QLatin1String("(\\w*)=(.*)"));
    foreach (const QString &line, lines) {
        if (regexp.exactMatch(line)) {
            const QString varName = regexp.cap(1);
            const QString varValue = regexp.cap(2);

            if (!varValue.isEmpty())
                envPairs.insert(varName, varValue);
        }
    }

    return true;
}

/**
 * Infers warnings settings on warning level set
 * @see http://msdn.microsoft.com/en-us/library/23k5d385.aspx
 */
void AbstractMsvcToolChain::inferWarningsForLevel(int warningLevel, WarningFlags &flags)
{
    // reset all except unrelated flag
    flags = flags & WarningFlags::AsErrors;

    if (warningLevel >= 1)
        flags |= WarningFlags(WarningFlags::Default | WarningFlags::IgnoredQualfiers | WarningFlags::HiddenLocals  | WarningFlags::UnknownPragma);
    if (warningLevel >= 2)
        flags |= WarningFlags::All;
    if (warningLevel >= 3) {
        flags |= WarningFlags(WarningFlags::Extra | WarningFlags::NonVirtualDestructor | WarningFlags::SignedComparison
                | WarningFlags::UnusedLocals | WarningFlags::Deprecated);
    }
    if (warningLevel >= 4)
        flags |= WarningFlags::UnusedParams;
}

bool AbstractMsvcToolChain::operator ==(const ToolChain &other) const
{
    if (!ToolChain::operator ==(other))
        return false;

    const AbstractMsvcToolChain *msvcTc = static_cast<const AbstractMsvcToolChain *>(&other);
    return targetAbi() == msvcTc->targetAbi()
            && m_vcvarsBat == msvcTc->m_vcvarsBat;
}

AbstractMsvcToolChain::WarningFlagAdder::WarningFlagAdder(const QString &flag,
                                                          WarningFlags &flags) :
    m_flags(flags),
    m_triggered(false)
{
    if (flag.startsWith(QLatin1String("-wd"))) {
        m_doesEnable = false;
    } else if (flag.startsWith(QLatin1String("-w"))) {
        m_doesEnable = true;
    } else {
        m_triggered = true;
        return;
    }
    bool ok = false;
    if (m_doesEnable)
        m_warningCode = flag.midRef(2).toInt(&ok);
    else
        m_warningCode = flag.midRef(3).toInt(&ok);
    if (!ok)
        m_triggered = true;
}

void AbstractMsvcToolChain::WarningFlagAdder::operator ()(int warningCode, WarningFlags flagsSet)
{
    if (m_triggered)
        return;
    if (warningCode == m_warningCode)
    {
        m_triggered = true;
        if (m_doesEnable)
            m_flags |= flagsSet;
        else
            m_flags &= ~flagsSet;
    }
}

bool AbstractMsvcToolChain::WarningFlagAdder::triggered() const
{
    return m_triggered;
}


} // namespace Internal
} // namespace ProjectExplorer

