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

/* A debug dispatcher for Windows that can be registered for calls with crashed
 * processes. It offers debugging using either Qt Creator or
 * the previously registered default debugger.
 * See usage() on how to install/use.
 * Installs itself in the bin directory of Qt Creator. */

#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <QTextStream>
#include <QFileInfo>
#include <QByteArray>
#include <QString>
#include <QDir>
#include <QTime>
#include <QProcess>
#include <QPushButton>

#include "registryaccess.h"

#include <windows.h>
#include <psapi.h>

#include <app/app_version.h>

using namespace RegistryAccess;

enum { debug = 0 };

static const char titleC[] = "Qt Creator Debugger";

// Optional
static const WCHAR debuggerWow32RegistryKeyC[] = L"Software\\Wow6432Node\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug";

static const WCHAR debuggerRegistryDefaultValueNameC[] = L"Debugger.Default";

static const char linkC[] = "http://msdn.microsoft.com/en-us/library/cc266343.aspx";
static const char creatorBinaryC[] = "qtcreator.exe";

#ifdef __GNUC__
#define RRF_RT_ANY             0x0000ffff  // no type restriction
#endif


enum Mode { HelpMode, RegisterMode, UnregisterMode, PromptMode, ForceCreatorMode, ForceDefaultMode };

Mode optMode = PromptMode;
bool optIsWow = false;
bool noguiMode = false;
unsigned long argProcessId = 0;
quint64 argWinCrashEvent = 0;

static bool parseArguments(const QStringList &args, QString *errorMessage)
{
    int argNumber = 0;
    const int count = args.size();
    const QChar dash = QLatin1Char('-');
    const QChar slash = QLatin1Char('/');
    for (int i = 1; i < count; i++) {
        QString arg = args.at(i);
        if (arg.startsWith(dash) || arg.startsWith(slash)) { // option
            arg.remove(0, 1);
            if (arg == QLatin1String("help") || arg == QLatin1String("?")) {
                optMode = HelpMode;
            } else if (arg == QLatin1String("qtcreator")) {
                optMode = ForceCreatorMode;
            } else if (arg == QLatin1String("default")) {
                optMode = ForceDefaultMode;
            } else if (arg == QLatin1String("register")) {
                optMode = RegisterMode;
            } else if (arg == QLatin1String("unregister")) {
                optMode = UnregisterMode;
            } else if (arg == QLatin1String("wow")) {
                optIsWow = true;
            } else if (arg == QLatin1String("nogui")) {
                noguiMode = true;
            } else {
                *errorMessage = QString::fromLatin1("Unexpected option: %1").arg(arg);
                return false;
            }
            } else { // argument
                bool ok = false;
                switch (argNumber++) {
                case 0:
                    argProcessId = arg.toULong(&ok);
                    break;
                case 1:
                    argWinCrashEvent = arg.toULongLong(&ok);
                    break;
                }
                if (!ok) {
                    *errorMessage = QString::fromLatin1("Invalid argument: %1").arg(arg);
                    return false;
                }
            }
        }
    switch (optMode) {
    case HelpMode:
    case RegisterMode:
    case UnregisterMode:
        break;
    default:
        if (argProcessId == 0) {
            *errorMessage = QString::fromLatin1("Please specify the process-id.");
            return false;
        }
    }
    return true;
}

static void usage(const QString &binary, const QString &message = QString())
{
    QString msg;
    QTextStream str(&msg);
    str << "<html><body>";
    if (message.isEmpty()) {
        str << "<h1>" <<  titleC << "</h1>"
            << "<p>Dispatcher that launches the desired debugger for a crashed process"
            " according to <a href=\"" << linkC << "\">Enabling Postmortem Debugging</a>.</p>";
    } else {
        str << "<b>" << message << "</b>";
    }
    str << "<pre>"
        << "Usage: " << QFileInfo(binary).baseName() << "[-wow] [-help|-?|qtcreator|default|register|unregister] &lt;process-id> &lt;event-id>\n"
        << "Options: -help, -?   Display this help\n"
        << "         -qtcreator  Launch Qt Creator without prompting\n"
        << "         -default    Launch Default handler without prompting\n"
        << "         -register   Register as post mortem debugger (requires administrative privileges)\n"
        << "         -unregister Unregister as post mortem debugger (requires administrative privileges)\n"
        << "         -wow        Indicates Wow32 call\n"
        << "         -nogui      Do not show error messages in popup windows\n"
        << "</pre>"
        << "<p>To install, modify the registry key <i>HKEY_LOCAL_MACHINE\\" << wCharToQString(debuggerRegistryKeyC)
        << "</i>:</p><ul>"
        << "<li>Create a copy of the string value <i>" << wCharToQString(debuggerRegistryValueNameC)
        << "</i> as <i>" << wCharToQString(debuggerRegistryDefaultValueNameC) << "</i>"
        << "<li>Change the value of <i>" << wCharToQString(debuggerRegistryValueNameC) << "</i> to "
        << "<pre>\"" << QDir::toNativeSeparators(binary) << "\" %ld %ld</pre>"
        << "</ul>"
        << "<p>On 64-bit systems, do the same for the key <i>HKEY_LOCAL_MACHINE\\" << wCharToQString(debuggerWow32RegistryKeyC) << "</i>, "
        << "setting the new value to <pre>\"" << QDir::toNativeSeparators(binary) << "\" -wow %ld %ld</pre></p>"
        << "<p>How to run a command with administrative privileges:</p>"
        << "<pre>runas /env /noprofile /user:Administrator \"command arguments\"</pre>"
        << "</body></html>";

    QMessageBox msgBox(QMessageBox::Information, QLatin1String(titleC), msg, QMessageBox::Ok);
    msgBox.exec();
}

// ------- Registry helpers

static inline bool registryWriteBinaryKey(HKEY handle,
                                          const WCHAR *valueName,
                                          DWORD type,
                                          const BYTE *data,
                                          DWORD size,
                                          QString *errorMessage)
{
    const LONG rc = RegSetValueEx(handle, valueName, 0, type, data, size);
    if (rc != ERROR_SUCCESS) {
        *errorMessage = msgRegistryOperationFailed("write", valueName, msgFunctionFailed("RegSetValueEx", rc));
        return false;
    }
    return true;
}

static inline bool registryWriteStringKey(HKEY handle, // HKEY_LOCAL_MACHINE, etc.
                                          const WCHAR *key,
                                          const QString &s,
                                          QString *errorMessage)
{
    const BYTE *data = reinterpret_cast<const BYTE *>(s.utf16());
    const DWORD size = 2 * s.size(); // excluding 0
    return registryWriteBinaryKey(handle, key, REG_SZ, data, size, errorMessage);
}

static inline bool registryReplaceStringKey(HKEY rootHandle, // HKEY_LOCAL_MACHINE, etc.
                                       const WCHAR *key,
                                       const WCHAR *valueName,
                                       const QString &newValue,
                                       QString *oldValue,
                                       QString *errorMessage)
{
    HKEY handle = 0;
    bool rc = false;
    do {
        if (!openRegistryKey(rootHandle, key, true, &handle, errorMessage))
            break;
        if (!registryReadStringKey(handle, valueName, oldValue, errorMessage))
            break;
        if (*oldValue != newValue) {
            if (!registryWriteStringKey(handle, valueName, newValue, errorMessage))
                break;
        }
        rc = true;
    } while (false);
    if (handle)
        RegCloseKey(handle);
    return rc;
}

static inline bool registryDeleteValue(HKEY handle,
                                       const WCHAR *valueName,
                                       QString *errorMessage)
{
    const LONG rc = RegDeleteValue(handle, valueName);
    if (rc != ERROR_SUCCESS) {
        *errorMessage = msgFunctionFailed("RegDeleteValue", rc);
        return false;
    }
    return true;
}

static QString getProcessBaseName(DWORD pid)
{
    QString rc;
    const HANDLE  handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (handle != NULL) {
        WCHAR buffer[MAX_PATH];
        if (GetModuleBaseName(handle, 0, buffer, MAX_PATH))
            rc = wCharToQString(buffer);
        CloseHandle(handle);
    }
    return rc;
}

// ------- main modes

static bool waitForProcess(DWORD pid)
{
    HANDLE handle = OpenProcess(PROCESS_QUERY_INFORMATION|READ_CONTROL|SYNCHRONIZE, false, pid);
    if (handle == NULL)
        return false;
    const DWORD waitResult = WaitForSingleObject(handle, INFINITE);
    CloseHandle(handle);
    return waitResult == WAIT_OBJECT_0;
}

bool startCreatorAsDebugger(bool asClient, QString *errorMessage)
{
    const QString dir = QApplication::applicationDirPath();
    const QString binary = dir + QLatin1Char('/') + QLatin1String(creatorBinaryC);
    QStringList args;
    // Send to running Creator: Unstable with directly linked CDB engine.
    if (asClient)
        args << QLatin1String("-client");
    args << QLatin1String("-wincrashevent")
        << QString::fromLatin1("%1:%2").arg(argWinCrashEvent).arg(argProcessId);
    if (debug)
        qDebug() << binary << args;
    QProcess p;
    p.setWorkingDirectory(dir);
    QTime executionTime;
    executionTime.start();
    p.start(binary, args, QIODevice::NotOpen);
    if (!p.waitForStarted()) {
        *errorMessage = QString::fromLatin1("Unable to start %1!").arg(binary);
        return false;
    }
    // Short execution time: indicates that -client was passed on attach to
    // another running instance of Qt Creator. Keep alive as long as user
    // does not close the process. If that fails, try to launch 2nd instance.
    const bool waitResult = p.waitForFinished(-1);
    const bool ranAsClient = asClient && (executionTime.elapsed() < 10000);
    if (waitResult && p.exitStatus() == QProcess::NormalExit && ranAsClient) {
        if (p.exitCode() == 0) {
            waitForProcess(argProcessId);
        } else {
            errorMessage->clear();
            return startCreatorAsDebugger(false, errorMessage);
        }
    }
    return true;
}

bool readDefaultDebugger(QString *defaultDebugger,
                         QString *errorMessage)
{
    bool success = false;
    HKEY handle;
    if (openRegistryKey(HKEY_LOCAL_MACHINE, optIsWow ? debuggerWow32RegistryKeyC : debuggerRegistryKeyC,
                        false, &handle, errorMessage)) {
        success = registryReadStringKey(handle, debuggerRegistryDefaultValueNameC,
                                        defaultDebugger, errorMessage);
        RegCloseKey(handle);
    }
    return success;
}

bool startDefaultDebugger(QString *errorMessage)
{
    QString defaultDebugger;
    if (!readDefaultDebugger(&defaultDebugger, errorMessage))
        return false;
    // binary, replace placeholders by pid/event id
    if (debug)
        qDebug() << "Default" << defaultDebugger;
    const QString placeHolder = QLatin1String("%ld");
    const int pidPlaceHolderPos = defaultDebugger.indexOf(placeHolder);
    if (pidPlaceHolderPos == -1)
        return true; // was empty or sth
    defaultDebugger.replace(pidPlaceHolderPos, placeHolder.size(), QString::number(argProcessId));
    const int evtPlaceHolderPos = defaultDebugger.indexOf(placeHolder);
    if (evtPlaceHolderPos != -1)
        defaultDebugger.replace(evtPlaceHolderPos, placeHolder.size(), QString::number(argWinCrashEvent));
    if (debug)
        qDebug() << "Default" << defaultDebugger;
    QProcess p;
    p.start(defaultDebugger, QIODevice::NotOpen);
    if (!p.waitForStarted()) {
        *errorMessage = QString::fromLatin1("Unable to start %1!").arg(defaultDebugger);
        return false;
    }
    p.waitForFinished(-1);
    return true;
}

bool chooseDebugger(QString *errorMessage)
{
    QString defaultDebugger;
    const QString processName = getProcessBaseName(argProcessId);
    const QString msg = QString::fromLatin1("The application \"%1\" (process id %2)  crashed. Would you like to debug it?").arg(processName).arg(argProcessId);
    QMessageBox msgBox(QMessageBox::Information, QLatin1String(titleC), msg, QMessageBox::Cancel);
    QPushButton *creatorButton = msgBox.addButton(QLatin1String("Debug with Qt Creator"), QMessageBox::AcceptRole);
    QPushButton *defaultButton = msgBox.addButton(QLatin1String("Debug with default debugger"), QMessageBox::AcceptRole);
    defaultButton->setEnabled(readDefaultDebugger(&defaultDebugger, errorMessage)
                              && !defaultDebugger.isEmpty());
    msgBox.exec();
    if (msgBox.clickedButton() == creatorButton) {
        // Just in case, default to standard. Do not run as client in the unlikely case
        // Creator crashed
        // TODO: pass asClient=true for new CDB engine.
        const bool canRunAsClient = !processName.contains(QLatin1String(creatorBinaryC), Qt::CaseInsensitive);
        Q_UNUSED(canRunAsClient)
        if (startCreatorAsDebugger(false, errorMessage))
            return true;
        return startDefaultDebugger(errorMessage);
    }
    if (msgBox.clickedButton() == defaultButton)
        return startDefaultDebugger(errorMessage);
    return true;
}

// registration helper: Register ourselves in a debugger registry key.
// Make a copy of the old value as "Debugger.Default" and have the
// "Debug" key point to us.

static bool registerDebuggerKey(const WCHAR *key,
                                const QString &call,
                                QString *errorMessage)
{
    HKEY handle = 0;
    bool success = false;
    do {
        if (!openRegistryKey(HKEY_LOCAL_MACHINE, key, true, &handle, errorMessage))
            break;
        // Save old key, which might be missing
        QString oldDebugger;
        if (isRegistered(handle, call, errorMessage, &oldDebugger)) {
            *errorMessage = QLatin1String("The program is already registered as post mortem debugger.");
            break;
        }
        if (!(oldDebugger.contains(QLatin1String(debuggerApplicationFileC), Qt::CaseInsensitive)
              || registryWriteStringKey(handle, debuggerRegistryDefaultValueNameC, oldDebugger, errorMessage)))
            break;
        if (debug)
            qDebug() << "registering self as " << call;
        if (!registryWriteStringKey(handle, debuggerRegistryValueNameC, call, errorMessage))
            break;
        success = true;
    } while (false);
    if (handle)
        RegCloseKey(handle);
    return success;
}

bool install(QString *errorMessage)
{
    if (!registerDebuggerKey(debuggerRegistryKeyC, debuggerCall(), errorMessage))
        return false;
#ifdef Q_OS_WIN64
    if (!registerDebuggerKey(debuggerWow32RegistryKeyC, debuggerCall(QLatin1String("-wow")), errorMessage))
        return false;
#endif
    return true;
}

// Unregister helper: Restore the original debugger key
static bool unregisterDebuggerKey(const WCHAR *key,
                                  const QString &call,
                                  QString *errorMessage)
{
    HKEY handle = 0;
    bool success = false;
    do {
        if (!openRegistryKey(HKEY_LOCAL_MACHINE, key, true, &handle, errorMessage))
            break;
        QString debugger;
        if (!isRegistered(handle, call, errorMessage, &debugger) && !debugger.isEmpty()) {
            *errorMessage = QLatin1String("The program is not registered as post mortem debugger.");
            break;
        }
        QString oldDebugger;
        if (!registryReadStringKey(handle, debuggerRegistryDefaultValueNameC, &oldDebugger, errorMessage))
            break;
        // Re-register old debugger or delete key if it was empty.
        if (oldDebugger.isEmpty()) {
            if (!registryDeleteValue(handle, debuggerRegistryValueNameC, errorMessage))
                break;
        } else {
            if (!registryWriteStringKey(handle, debuggerRegistryValueNameC, oldDebugger, errorMessage))
                break;
        }
        if (!registryDeleteValue(handle, debuggerRegistryDefaultValueNameC, errorMessage))
            break;
        success = true;
    } while (false);
    if (handle)
        RegCloseKey(handle);
    return success;
}


bool uninstall(QString *errorMessage)
{
    if (!unregisterDebuggerKey(debuggerRegistryKeyC, debuggerCall(), errorMessage))
        return false;
#ifdef Q_OS_WIN64
    if (!unregisterDebuggerKey(debuggerWow32RegistryKeyC, debuggerCall(QLatin1String("-wow")), errorMessage))
        return false;
#endif
    return true;
}


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setApplicationName(QLatin1String(titleC));
    QApplication::setOrganizationName(QLatin1String(Core::Constants::IDE_SETTINGSVARIANT_STR));
    QString errorMessage;

    if (!parseArguments(QApplication::arguments(), &errorMessage)) {
        qWarning("%s\n", qPrintable(errorMessage));
        usage(QCoreApplication::applicationFilePath(), errorMessage);
        return -1;
    }
    if (debug)
        qDebug() << "Mode=" << optMode << " PID=" << argProcessId << " Evt=" << argWinCrashEvent;
    bool ex = 0;
    switch (optMode) {
    case HelpMode:
        usage(QCoreApplication::applicationFilePath(), errorMessage);
        break;
    case ForceCreatorMode:
        // TODO: pass asClient=true for new CDB engine.
        ex = startCreatorAsDebugger(false, &errorMessage) ? 0 : -1;
        break;
    case ForceDefaultMode:
        ex = startDefaultDebugger(&errorMessage) ? 0 : -1;
        break;
    case PromptMode:
        ex = chooseDebugger(&errorMessage) ? 0 : -1;
        break;
    case RegisterMode:
        ex = install(&errorMessage) ? 0 : -1;
        break;
    case UnregisterMode:
        ex = uninstall(&errorMessage) ? 0 : -1;
        break;
    }
    if (ex && !errorMessage.isEmpty()) {
        if (noguiMode)
            qWarning("%s\n", qPrintable(errorMessage));
        else
            QMessageBox::warning(0, QLatin1String(titleC), errorMessage, QMessageBox::Ok);
    }
    return ex;
}
