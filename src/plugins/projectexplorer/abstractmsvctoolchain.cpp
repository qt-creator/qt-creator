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

#include "abstractmsvctoolchain.h"

#include "msvcparser.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorersettings.h>
#include <projectexplorer/gcctoolchain.h>


#include <utils/fileutils.h>
#include <utils/qtcprocess.h>
#include <utils/synchronousprocess.h>

#include <QFileInfo>
#include <QDir>
#include <QTemporaryFile>

enum { debug = 0 };

namespace ProjectExplorer {
namespace Internal {


AbstractMsvcToolChain::AbstractMsvcToolChain(const QString &id,
                                             bool autodetect,
                                             const Abi &abi,
                                             const QString& vcvarsBat) :
    ToolChain(id, autodetect),
    m_lastEnvironment(Utils::Environment::systemEnvironment()),
    m_abi(abi),
    m_vcvarsBat(vcvarsBat)
{
    Q_ASSERT(abi.os() == Abi::WindowsOS);
    Q_ASSERT(abi.binaryFormat() == Abi::PEFormat);
    Q_ASSERT(abi.osFlavor() != Abi::WindowsMSysFlavor);
    Q_ASSERT(!m_vcvarsBat.isEmpty());
}

AbstractMsvcToolChain::AbstractMsvcToolChain(const QString &id, bool autodetect) :
    ToolChain(id, autodetect),
    m_lastEnvironment(Utils::Environment::systemEnvironment())
{

}

Abi AbstractMsvcToolChain::targetAbi() const
{
    return m_abi;
}

bool AbstractMsvcToolChain::isValid() const
{
    return !m_vcvarsBat.isEmpty();
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
    Q_UNUSED(cxxflags);

    switch (m_abi.osFlavor()) {
    case ProjectExplorer::Abi::WindowsMsvc2010Flavor:
    case ProjectExplorer::Abi::WindowsMsvc2012Flavor:
        return STD_CXX11;
    default:
        return NO_FLAGS;
    }
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
    bool useJom = ProjectExplorerPlugin::instance()->projectExplorerSettings().useJom;
    const QString jom = QLatin1String("jom.exe");
    const QString nmake = QLatin1String("nmake.exe");
    QString tmp;

    if (useJom) {
        tmp = environment.searchInPath(jom, QStringList()
                                       << QCoreApplication::applicationDirPath());
        if (!tmp.isEmpty())
            return tmp;
    }
    tmp = environment.searchInPath(nmake);
    if (!tmp.isEmpty())
        return tmp;

    // Nothing found :(
    return useJom ? jom : nmake;
}

Utils::FileName AbstractMsvcToolChain::compilerCommand() const
{
    Utils::Environment env = Utils::Environment::systemEnvironment();
    addToEnvironment(env);
    return Utils::FileName::fromString(env.searchInPath(QLatin1String("cl.exe")));
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

    QByteArray predefinedMacros = "#define __MSVCRT__\n"
            "#define __w64\n"
            "#define __int64 long long\n"
            "#define __int32 long\n"
            "#define __int16 short\n"
            "#define __int8 char\n"
            "#define __ptr32\n"
            "#define __ptr64\n";

    return predefinedMacros;
}


bool AbstractMsvcToolChain::generateEnvironmentSettings(Utils::Environment &env,
                                                        const QString& batchFile,
                                                        const QString& batchArgs,
                                                        QMap<QString, QString>& envPairs) const
{
    // Create a temporary file name for the output. Use a temporary file here
    // as I don't know another way to do this in Qt...
    // Note, can't just use a QTemporaryFile all the way through as it remains open
    // internally so it can't be streamed to later.
    QString tempOutFile;
    QTemporaryFile* pVarsTempFile = new QTemporaryFile(QDir::tempPath() + QLatin1String("/XXXXXX.txt"));
    pVarsTempFile->setAutoRemove(false);
    pVarsTempFile->open();
    pVarsTempFile->close();
    tempOutFile = pVarsTempFile->fileName();
    delete pVarsTempFile;

    // Create a batch file to create and save the env settings
    Utils::TempFileSaver saver(QDir::tempPath() + QLatin1String("/XXXXXX.bat"));

    QByteArray call = "call ";
    call += Utils::QtcProcess::quoteArg(batchFile).toLocal8Bit();
    if (!batchArgs.isEmpty()) {
        call += ' ';
        call += batchArgs.toLocal8Bit();
    }
    saver.write(call + "\r\n");

    const QByteArray redirect = "set > " + Utils::QtcProcess::quoteArg(
                                    QDir::toNativeSeparators(tempOutFile)).toLocal8Bit() + "\r\n";
    saver.write(redirect);
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
    const QString cmdPath = QString::fromLocal8Bit(qgetenv("COMSPEC"));
    // Windows SDK setup scripts require command line switches for environment expansion.
    QString cmdArguments = QLatin1String(" /E:ON /V:ON /c \"");
    cmdArguments += QDir::toNativeSeparators(saver.fileName());
    cmdArguments += QLatin1Char('"');
    run.setCommand(cmdPath, cmdArguments);
    if (debug)
        qDebug() << "readEnvironmentSetting: " << call << cmdPath << cmdArguments
                 << " Env: " << env.size();
    run.start();

    if (!run.waitForStarted()) {
        qWarning("%s: Unable to run '%s': %s", Q_FUNC_INFO, qPrintable(m_vcvarsBat),
                 qPrintable(run.errorString()));
        return false;
    }
    if (!run.waitForFinished()) {
        qWarning("%s: Timeout running '%s'", Q_FUNC_INFO, qPrintable(m_vcvarsBat));
        Utils::SynchronousProcess::stopProcess(run);
        return false;
    }
    // The SDK/MSVC scripts do not return exit codes != 0. Check on stdout.
    const QByteArray stdOut = run.readAllStandardOutput();
    if (!stdOut.isEmpty() && (stdOut.contains("Unknown") || stdOut.contains("Error")))
        qWarning("%s: '%s' reports:\n%s", Q_FUNC_INFO, call.constData(), stdOut.constData());

    //
    // Now parse the file to get the environment settings
    QFile varsFile(tempOutFile);
    if (!varsFile.open(QIODevice::ReadOnly))
        return false;

    QRegExp regexp(QLatin1String("(\\w*)=(.*)"));
    while (!varsFile.atEnd()) {
        const QString line = QString::fromLocal8Bit(varsFile.readLine()).trimmed();
        if (regexp.exactMatch(line)) {
            const QString varName = regexp.cap(1);
            const QString varValue = regexp.cap(2);

            if (!varValue.isEmpty())
                envPairs.insert(varName, varValue);
        }
    }

    // Tidy up and remove the file
    varsFile.close();
    varsFile.remove();

    return true;
}

bool AbstractMsvcToolChain::operator ==(const ToolChain &other) const
{
    if (!ToolChain::operator ==(other))
        return false;

    const AbstractMsvcToolChain *msvcTc = static_cast<const AbstractMsvcToolChain *>(&other);
    return targetAbi() == msvcTc->targetAbi()
            && m_vcvarsBat == msvcTc->m_vcvarsBat;
}


} // namespace Internal
} // namespace ProjectExplorer

