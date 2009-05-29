/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

/* A debug dispatcher for Windows that can be registered for calls with crashed
 * processes. It offers debugging using either Qt Creator or
 * the previously registered default debugger.
 * See usage() on how to install/use.
 * Installs itself in the bin directory of Qt Creator. */

#include <QtGui/QApplication>
#include <QtGui/QMessageBox>
#include <QtCore/QDebug>
#include <QtCore/QTextStream>
#include <QtCore/QFileInfo>
#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtGui/QPushButton>

#include <windows.h>
#include <psapi.h>

enum { debug = 0 };

static const char *titleC = "Qt Creator Debugger";
static const char *organizationC = "Nokia";

static const WCHAR *debuggerRegistryKeyC = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug";
// Optional
static const WCHAR *debuggerWow32RegistryKeyC = L"Software\\Wow6432Node\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug";

static const WCHAR *debuggerRegistryValueNameC = L"Debugger";
static const WCHAR *debuggerRegistryDefaultValueNameC = L"Debugger.Default";

static const char *linkC = "http://msdn.microsoft.com/en-us/library/cc266343.aspx";

#ifdef __GNUC__
static inline QString wCharToQString(const WCHAR *w) { return QString::fromUtf16(reinterpret_cast<const ushort*>(w)); }
#define RRF_RT_ANY             0x0000ffff  // no type restriction
#else
static inline QString wCharToQString(const WCHAR *w) { return QString::fromUtf16(w); }
#endif


enum Mode { HelpMode, PromptMode, ForceCreatorMode, ForceDefaultMode };

Mode optMode = PromptMode;
bool optIsWow = false;
unsigned long argProcessId = 0;
quint64 argWinCrashEvent = 0;

static QString winErrorMessage(unsigned long error)
{
    QString rc = QString::fromLatin1("#%1: ").arg(error);
    ushort *lpMsgBuf;

    const int len = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, error, 0, (LPTSTR)&lpMsgBuf, 0, NULL);
    if (len) {
       rc = QString::fromUtf16(lpMsgBuf, len);
        LocalFree(lpMsgBuf);
    } else {
        rc += QString::fromLatin1("<unknown error>");
    }
    return rc;
}

static inline QString msgFunctionFailed(const char *f, unsigned long error)
{
    return QString::fromLatin1("'%1' failed: %2").arg(QLatin1String(f), winErrorMessage(error));
}

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
            } else if (arg == QLatin1String("wow")) {
                optIsWow = true;
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
    if (optMode != HelpMode && argProcessId == 0) {
        *errorMessage = QString::fromLatin1("Please specify the process-id.");
        return false;
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
        << "Usage: " << QFileInfo(binary).baseName() << "[-wow] [-help|-?|qtcreator|default] &lt;process-id> &lt;event-id>\n"
        << "Options: -help, -?   Display this help\n"
        << "         -qtcreator  Launch Qt Creator without prompting\n"
        << "         -default    Launch Default handler without prompting\n"
        << "         -wow        Indicates Wow32 call\n"
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
        << "</body></html>";

    QMessageBox msgBox(QMessageBox::Information, QLatin1String(titleC), msg, QMessageBox::Ok);
    msgBox.exec();
}

// ------- Registry helpers

static bool openRegistryKey(HKEY category, // HKEY_LOCAL_MACHINE, etc.
                            const WCHAR *key,
                            bool readWrite,
                            HKEY *keyHandle,
                            QString *errorMessage)
{

    REGSAM accessRights = KEY_READ;
    if (readWrite)
         accessRights |= KEY_SET_VALUE;
    const LONG rc = RegOpenKeyEx(category, key, 0, accessRights, keyHandle);
    if (rc != ERROR_SUCCESS) {
        *errorMessage = msgFunctionFailed("RegOpenKeyEx", rc);
        return false;
    }
    return true;
}

static inline QString msgRegistryOperationFailed(const char *op, const WCHAR *valueName, const QString &why)
{
    QString rc = QLatin1String("Registry ");
    rc += QLatin1String(op);
    rc += QLatin1String(" of ");
    rc += wCharToQString(valueName);
    rc += QLatin1String(" failed: ");
    rc += why;
    return rc;
}

static bool registryReadBinaryKey(HKEY handle, // HKEY_LOCAL_MACHINE, etc.
                                  const WCHAR *valueName,
                                  QByteArray *data,
                                  QString *errorMessage)
{
    data->clear();
    DWORD type;
    DWORD size;
    // get size and retrieve
    LONG rc = RegQueryValueEx(handle, valueName, 0, &type, 0, &size);
    if (rc != ERROR_SUCCESS) {
        *errorMessage = msgRegistryOperationFailed("read", valueName, msgFunctionFailed("RegQueryValueEx1", rc));
        return false;
    }
    BYTE *dataC = new BYTE[size + 1];
    // Will be Utf16 in case of a string
    rc = RegQueryValueEx(handle, valueName, 0, &type, dataC, &size);
    if (rc != ERROR_SUCCESS) {
        *errorMessage = msgRegistryOperationFailed("read", valueName, msgFunctionFailed("RegQueryValueEx2", rc));
        return false;
    }
    *data = QByteArray(reinterpret_cast<const char*>(dataC), size);
    delete [] dataC;
    return true;
}

static bool registryReadStringKey(HKEY handle, // HKEY_LOCAL_MACHINE, etc.
                                  const WCHAR *valueName,
                                  QString *s,
                                  QString *errorMessage)
{
    QByteArray data;
    if (!registryReadBinaryKey(handle, valueName, &data, errorMessage))
        return false;
    data += '\0';
    data += '\0';
    *s = QString::fromUtf16(reinterpret_cast<const unsigned short*>(data.data()));
    return true;
}

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
    } while(false);
    if (handle)
        RegCloseKey(handle);
    return rc;
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

bool startCreatorAsDebugger(const QApplication &a, QString *errorMessage)
{
    const QString dir = a.applicationDirPath();
    const QString binary = dir + QLatin1String("/qtcreator.exe");
    QStringList args;
    args << QLatin1String("-debug") << QString::number(argProcessId)
            << QLatin1String("-wincrashevent") << QString::number(argWinCrashEvent);
    if (debug)
        qDebug() << binary << args;
    QProcess p;
    p.setWorkingDirectory(dir);
    p.start(binary, args, QIODevice::NotOpen);
    if (!p.waitForStarted()) {
        *errorMessage = QString::fromLatin1("Unable to start %1!").arg(binary);
        return false;
    }
    p.waitForFinished(-1);
    return true;
}

bool startDefaultDebugger(const QApplication &a, QString *errorMessage)
{
    // Read out default value
    HKEY handle;
    if (!openRegistryKey(HKEY_LOCAL_MACHINE, optIsWow ? debuggerWow32RegistryKeyC : debuggerRegistryKeyC,
                         false, &handle, errorMessage))
        return false;
    QString defaultDebugger;
    if (!registryReadStringKey(handle, debuggerRegistryDefaultValueNameC, &defaultDebugger, errorMessage)) {
        RegCloseKey(handle);
        return false;
    }
    RegCloseKey(handle);
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

bool chooseDebugger(const QApplication &a, QString *errorMessage)
{
    const QString msg = QString::fromLatin1("The application \"%1\" (process id %2)  crashed. Would you like to debug it?").arg(getProcessBaseName(argProcessId)).arg(argProcessId);
    QMessageBox msgBox(QMessageBox::Information, QLatin1String(titleC), msg, QMessageBox::Cancel);
    QPushButton *creatorButton = msgBox.addButton(QLatin1String("Debug with Qt Creator"), QMessageBox::AcceptRole);
    QPushButton *defaultButton = msgBox.addButton(QLatin1String("Debug with default debugger"), QMessageBox::AcceptRole);
    msgBox.exec();
    if (msgBox.clickedButton() == creatorButton) {
        // Just in case, default to standard
        if (startCreatorAsDebugger(a, errorMessage))
            return true;
        return startDefaultDebugger(a, errorMessage);
    }
    if (msgBox.clickedButton() == defaultButton)
        return startDefaultDebugger(a, errorMessage);
    return true;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName(QLatin1String(titleC));
    a.setOrganizationName(QLatin1String(organizationC));
    QString errorMessage;

    if (!parseArguments(a.arguments(), &errorMessage)) {
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
        ex = startCreatorAsDebugger(a, &errorMessage) ? 0 : -1;
        break;
    case ForceDefaultMode:
        ex = startDefaultDebugger(a, &errorMessage) ? 0 : -1;
        break;
    case PromptMode:
        ex = chooseDebugger(a, &errorMessage) ? 0 : -1;
        break;
    }
    if (ex && !errorMessage.isEmpty()) {
        qWarning("%s\n", qPrintable(errorMessage));
        QMessageBox::warning(0, QLatin1String(titleC), errorMessage, QMessageBox::Ok);
    }
    return ex;
}
