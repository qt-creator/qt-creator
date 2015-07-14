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

#include "coregdbadapter.h"

#include <coreplugin/messagebox.h>

#include <debugger/debuggercore.h>
#include <debugger/debuggerprotocol.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggerstringutils.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QTemporaryFile>

using namespace Utils;

namespace Debugger {
namespace Internal {

#define CB(callback) [this](const DebuggerResponse &r) { callback(r); }

///////////////////////////////////////////////////////////////////////
//
// CoreGdbAdapter
//
///////////////////////////////////////////////////////////////////////

GdbCoreEngine::GdbCoreEngine(const DebuggerRunParameters &startParameters)
    : GdbEngine(startParameters),
      m_coreUnpackProcess(0)
{}

GdbCoreEngine::~GdbCoreEngine()
{
    if (m_coreUnpackProcess) {
        m_coreUnpackProcess->blockSignals(true);
        m_coreUnpackProcess->terminate();
        m_coreUnpackProcess->deleteLater();
        m_coreUnpackProcess = 0;
        if (m_tempCoreFile.isOpen())
            m_tempCoreFile.close();
    }
    if (!m_tempCoreName.isEmpty()) {
        QFile tmpFile(m_tempCoreName);
        tmpFile.remove();
    }
}

void GdbCoreEngine::setupEngine()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    showMessage(_("TRYING TO START ADAPTER"));

    const DebuggerRunParameters &rp = runParameters();
    m_executable = rp.executable;
    QFileInfo fi(rp.coreFile);
    m_coreName = fi.absoluteFilePath();

    unpackCoreIfNeeded();
}

static QString findExecutableFromName(const QString &fileNameFromCore, const QString &coreFile)
{
    if (QFileInfo(fileNameFromCore).isFile())
        return fileNameFromCore;
    if (fileNameFromCore.isEmpty())
        return QString();

    // turn the filename into an absolute path, using the location of the core as a hint
    QString absPath;
    QFileInfo fi(fileNameFromCore);
    if (fi.isAbsolute()) {
        absPath = fileNameFromCore;
    } else {
        QFileInfo coreInfo(coreFile);
        QDir coreDir = coreInfo.dir();
        absPath = FileUtils::resolvePath(coreDir.absolutePath(), fileNameFromCore);
    }
    if (QFileInfo(absPath).isFile() || absPath.isEmpty())
        return absPath;

    // remove possible trailing arguments
    QLatin1Char sep(' ');
    QStringList pathFragments = absPath.split(sep);
    while (pathFragments.size() > 0) {
        QString joined_path = pathFragments.join(sep);
        if (QFileInfo(joined_path).isFile()) {
            return joined_path;
        }
        pathFragments.pop_back();
    }

    return QString();
}

GdbCoreEngine::CoreInfo
GdbCoreEngine::readExecutableNameFromCore(const QString &debuggerCommand, const QString &coreFile)
{
    CoreInfo cinfo;
#if 0
    ElfReader reader(coreFile);
    cinfo.rawStringFromCore = QString::fromLocal8Bit(reader.readCoreName(&cinfo.isCore));
    cinfo.foundExecutableName = findExecutableFromName(cinfo.rawStringFromCore, coreFile);
#else
    QStringList args;
    args.append(QLatin1String("-nx"));
    args.append(QLatin1String("-batch"));
    args.append(QLatin1String("-c"));
    args.append(coreFile);

    QProcess proc;
    QStringList envLang = QProcess::systemEnvironment();
    envLang.replaceInStrings(QRegExp(QLatin1String("^LC_ALL=.*")), QLatin1String("LC_ALL=C"));
    proc.setEnvironment(envLang);
    proc.start(debuggerCommand, args);

    if (proc.waitForFinished()) {
        QByteArray ba = proc.readAllStandardOutput();
        // Core was generated by `/data/dev/creator-2.6/bin/qtcreator'.
        // Program terminated with signal 11, Segmentation fault.
        int pos1 = ba.indexOf("Core was generated by");
        if (pos1 != -1) {
            pos1 += 23;
            int pos2 = ba.indexOf('\'', pos1);
            if (pos2 != -1) {
                cinfo.isCore = true;
                cinfo.rawStringFromCore = QString::fromLocal8Bit(ba.mid(pos1, pos2 - pos1));
                cinfo.foundExecutableName = findExecutableFromName(cinfo.rawStringFromCore, coreFile);
            }
        }
    }
#endif
    return cinfo;
}

void GdbCoreEngine::continueSetupEngine()
{
    bool isCore = true;
    if (m_coreUnpackProcess) {
        isCore = m_coreUnpackProcess->exitCode() == 0;
        m_coreUnpackProcess->deleteLater();
        m_coreUnpackProcess = 0;
        if (m_tempCoreFile.isOpen())
            m_tempCoreFile.close();
    }
    if (isCore && m_executable.isEmpty()) {
        GdbCoreEngine::CoreInfo cinfo = readExecutableNameFromCore(
                                            runParameters().debuggerCommand,
                                            coreFileName());

        if (cinfo.isCore) {
            m_executable = cinfo.foundExecutableName;
            if (m_executable.isEmpty()) {
                Core::AsynchronousMessageBox::warning(
                    tr("Error Loading Symbols"),
                    tr("No executable to load symbols from specified core."));
                notifyEngineSetupFailed();
                return;
            }
        }
    }
    if (isCore) {
        startGdb();
    } else {
        Core::AsynchronousMessageBox::warning(
            tr("Error Loading Core File"),
            tr("The specified file does not appear to be a core file."));
        notifyEngineSetupFailed();
    }
}

void GdbCoreEngine::writeCoreChunk()
{
    m_tempCoreFile.write(m_coreUnpackProcess->readAll());
}

void GdbCoreEngine::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    // Do that first, otherwise no symbols are loaded.
    QFileInfo fi(m_executable);
    QByteArray path = fi.absoluteFilePath().toLocal8Bit();
    postCommand("-file-exec-and-symbols \"" + path + '"', NoFlags,
         CB(handleFileExecAndSymbols));
}

void GdbCoreEngine::handleFileExecAndSymbols(const DebuggerResponse &response)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    QString core = coreFileName();
    if (response.resultClass == ResultDone) {
        showMessage(tr("Symbols found."), StatusBar);
        handleInferiorPrepared();
    } else {
        QString msg = tr("No symbols found in core file <i>%1</i>.").arg(core)
            + _(" ") + tr("This can be caused by a path length limitation "
                          "in the core file.")
            + _(" ") + tr("Try to specify the binary using the "
                          "<i>Debug->Start Debugging->Attach to Core</i> dialog.");
        notifyInferiorSetupFailed(msg);
    }
}

void GdbCoreEngine::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    postCommand("target core " + coreFileName().toLocal8Bit(), NoFlags, CB(handleTargetCore));
}

void GdbCoreEngine::handleTargetCore(const DebuggerResponse &response)
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    notifyEngineRunOkAndInferiorUnrunnable();
    if (response.resultClass == ResultDone) {
        showMessage(tr("Attached to core."), StatusBar);
        // Due to the auto-solib-add off setting, we don't have any
        // symbols yet. Load them in order of importance.
        reloadStack();
        reloadModulesInternal();
        postCommand("p 5", NoFlags, CB(handleRoundTrip));
        return;
    }
    showStatusMessage(tr("Attach to core \"%1\" failed:").arg(runParameters().coreFile)
        + QLatin1Char('\n') + QString::fromLocal8Bit(response.data["msg"].data()));
    notifyEngineIll();
}

void GdbCoreEngine::handleRoundTrip(const DebuggerResponse &response)
{
    Q_UNUSED(response);
    loadSymbolsForStack();
    handleStop2();
    QTimer::singleShot(1000, this, SLOT(loadAllSymbols()));
}

void GdbCoreEngine::interruptInferior()
{
    // A core never runs, so this cannot be called.
    QTC_CHECK(false);
}

void GdbCoreEngine::shutdownEngine()
{
    notifyAdapterShutdownOk();
}

static QString tempCoreFilename()
{
    QString pattern = QDir::tempPath() + QLatin1String("/tmpcore-XXXXXX");
    QTemporaryFile tmp(pattern);
    tmp.open();
    return tmp.fileName();
}

void GdbCoreEngine::unpackCoreIfNeeded()
{
    QStringList arguments;
    const QString msg = _("Unpacking core file to %1");
    if (m_coreName.endsWith(QLatin1String(".lzo"))) {
        m_tempCoreName = tempCoreFilename();
        showMessage(msg.arg(m_tempCoreName));
        arguments << QLatin1String("-o") << m_tempCoreName << QLatin1String("-x") << m_coreName;
        m_coreUnpackProcess = new QProcess(this);
        m_coreUnpackProcess->setWorkingDirectory(QDir::tempPath());
        m_coreUnpackProcess->start(QLatin1String("lzop"), arguments);
        connect(m_coreUnpackProcess, static_cast<void (QProcess::*)(int)>(&QProcess::finished),
                this, &GdbCoreEngine::continueSetupEngine);
    } else if (m_coreName.endsWith(QLatin1String(".gz"))) {
        m_tempCoreName = tempCoreFilename();
        showMessage(msg.arg(m_tempCoreName));
        m_tempCoreFile.setFileName(m_tempCoreName);
        m_tempCoreFile.open(QFile::WriteOnly);
        arguments << QLatin1String("-c") << QLatin1String("-d") << m_coreName;
        m_coreUnpackProcess = new QProcess(this);
        m_coreUnpackProcess->setWorkingDirectory(QDir::tempPath());
        m_coreUnpackProcess->start(QLatin1String("gzip"), arguments);
        connect(m_coreUnpackProcess, &QProcess::readyRead, this, &GdbCoreEngine::writeCoreChunk);
        connect(m_coreUnpackProcess, static_cast<void (QProcess::*)(int)>(&QProcess::finished),
                this, &GdbCoreEngine::continueSetupEngine);
    } else {
        continueSetupEngine();
    }
}

QString GdbCoreEngine::coreFileName() const
{
    return m_tempCoreName.isEmpty() ? m_coreName : m_tempCoreName;
}

} // namespace Internal
} // namespace Debugger
