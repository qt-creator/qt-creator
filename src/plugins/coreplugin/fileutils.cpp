/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "fileutils.h"

#include <utils/environment.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtGui/QApplication>
#include <QtGui/QMessageBox>
#include <QtGui/QWidget>

#ifndef Q_OS_WIN
#include <coreplugin/icore.h>
#include <utils/consoleprocess.h>
#include <utils/qtcprocess.h>
#ifndef Q_OS_MAC
#include <coreplugin/coreconstants.h>
#include <utils/unixutils.h>
#include <QtGui/QPushButton>
#endif
#endif

using namespace Core;

#if !defined(Q_OS_WIN) && !defined(Q_OS_MAC)
// Show error with option to open settings.
static inline void showGraphicalShellError(QWidget *parent,
                                           const QString &app,
                                           const QString &error)
{
    const QString title = QApplication::translate("Core::Internal",
                                                  "Launching a file browser failed");
    const QString msg = QApplication::translate("Core::Internal",
                                                "Unable to start the file manager:\n\n%1\n\n").arg(app);
    QMessageBox mbox(QMessageBox::Warning, title, msg, QMessageBox::Close, parent);
    if (!error.isEmpty())
        mbox.setDetailedText(QApplication::translate("Core::Internal",
                                                     "'%1' returned the following error:\n\n%2").arg(app, error));
    QAbstractButton *settingsButton = mbox.addButton(QApplication::translate("Core::Internal", "Settings..."),
                                                     QMessageBox::ActionRole);
    mbox.exec();
    if (mbox.clickedButton() == settingsButton)
        Core::ICore::instance()->showOptionsDialog(QLatin1String(Core::Constants::SETTINGS_CATEGORY_CORE),
                                                   QLatin1String(Core::Constants::SETTINGS_ID_ENVIRONMENT));
}
#endif

void FileUtils::showInGraphicalShell(QWidget *parent, const QString &pathIn)
{
    // Mac, Windows support folder or file.
#if defined(Q_OS_WIN)
    const QString explorer = Utils::Environment::systemEnvironment().searchInPath(QLatin1String("explorer.exe"));
    if (explorer.isEmpty()) {
        QMessageBox::warning(parent,
                             QApplication::translate("Core::Internal",
                                                     "Launching Windows Explorer Failed"),
                             QApplication::translate("Core::Internal",
                                                     "Could not find explorer.exe in path to launch Windows Explorer."));
        return;
    }
    QString param;
    if (!QFileInfo(pathIn).isDir())
        param = QLatin1String("/select,");
    param += QDir::toNativeSeparators(pathIn);
    QProcess::startDetached(explorer, QStringList(param));
#elif defined(Q_OS_MAC)
    Q_UNUSED(parent)
    QStringList scriptArgs;
    scriptArgs << QLatin1String("-e")
               << QString::fromLatin1("tell application \"Finder\" to reveal POSIX file \"%1\"")
                                     .arg(pathIn);
    QProcess::execute(QLatin1String("/usr/bin/osascript"), scriptArgs);
    scriptArgs.clear();
    scriptArgs << QLatin1String("-e")
               << QLatin1String("tell application \"Finder\" to activate");
    QProcess::execute("/usr/bin/osascript", scriptArgs);
#else
    // we cannot select a file here, because no file browser really supports it...
    const QFileInfo fileInfo(pathIn);
    const QString folder = fileInfo.isDir() ? fileInfo.absoluteFilePath() : fileInfo.filePath();
    const QString app = Utils::UnixUtils::fileBrowser(Core::ICore::instance()->settings());
    QProcess browserProc;
    const QString browserArgs = Utils::UnixUtils::substituteFileBrowserParameters(app, folder);
    bool success = browserProc.startDetached(browserArgs);
    const QString error = QString::fromLocal8Bit(browserProc.readAllStandardError());
    success = success && error.isEmpty();
    if (!success)
        showGraphicalShellError(parent, app, error);
#endif
}

void FileUtils::openTerminal(const QString &path)
{
    // Get terminal application
#ifdef Q_OS_WIN
    const QString terminalEmulator = QString::fromLocal8Bit(qgetenv("COMSPEC"));
    const QStringList args; // none
#elif defined(Q_WS_MAC)
    const QString terminalEmulator = Core::ICore::instance()->resourcePath()
            + QLatin1String("/scripts/openTerminal.command");
    QStringList args;
#else
    QStringList args = Utils::QtcProcess::splitArgs(
        Utils::ConsoleProcess::terminalEmulator(Core::ICore::instance()->settings()));
    const QString terminalEmulator = args.takeFirst();
    const QString shell = QString::fromLocal8Bit(qgetenv("SHELL"));
    args.append(shell);
#endif
    // Launch terminal with working directory set.
    const QFileInfo fileInfo(path);
    const QString pwd = QDir::toNativeSeparators(fileInfo.isDir() ?
                                                 fileInfo.absoluteFilePath() :
                                                 fileInfo.absolutePath());
    QProcess::startDetached(terminalEmulator, args, pwd);
}

QString FileUtils::msgGraphicalShellAction()
{
#if defined(Q_OS_WIN)
    return QApplication::translate("Core::Internal", "Show in Explorer...");
#elif defined(Q_OS_MAC)
    return QApplication::translate("Core::Internal", "Show in Finder...");
#else
    return QApplication::translate("Core::Internal", "Show Containing Folder...");
#endif
}

QString FileUtils::msgTerminalAction()
{
#ifdef Q_OS_WIN
    return QApplication::translate("Core::Internal", "Open Command Prompt Here...");
#else
    return QApplication::translate("Core::Internal", "Open Terminal Here...");
#endif
}
