/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "fileutils.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/removefiledialog.h>
#include <coreplugin/vcsmanager.h>
#include <utils/environment.h>

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QApplication>
#include <QMessageBox>
#include <QWidget>

#if QT_VERSION < 0x050000
#include <QAbstractFileEngine>
#endif

#ifndef Q_OS_WIN
#include <utils/consoleprocess.h>
#include <utils/qtcprocess.h>
#ifndef Q_OS_MAC
#include <coreplugin/coreconstants.h>
#include <utils/unixutils.h>
#include <QPushButton>
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
        Core::ICore::showOptionsDialog(QLatin1String(Core::Constants::SETTINGS_CATEGORY_CORE),
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
    QStringList param;
    if (!QFileInfo(pathIn).isDir())
        param += QLatin1String("/select,");
    param += QDir::toNativeSeparators(pathIn);
    QProcess::startDetached(explorer, param);
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
    const QString app = Utils::UnixUtils::fileBrowser(Core::ICore::settings());
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
#elif defined(Q_OS_MAC)
    const QString terminalEmulator = Core::ICore::resourcePath()
            + QLatin1String("/scripts/openTerminal.command");
    QStringList args;
#else
    QStringList args = Utils::QtcProcess::splitArgs(
        Utils::ConsoleProcess::terminalEmulator(Core::ICore::settings()));
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
    return QApplication::translate("Core::Internal", "Show in Explorer");
#elif defined(Q_OS_MAC)
    return QApplication::translate("Core::Internal", "Show in Finder");
#else
    return QApplication::translate("Core::Internal", "Show Containing Folder");
#endif
}

QString FileUtils::msgTerminalAction()
{
#ifdef Q_OS_WIN
    return QApplication::translate("Core::Internal", "Open Command Prompt Here");
#else
    return QApplication::translate("Core::Internal", "Open Terminal Here");
#endif
}

void FileUtils::removeFile(const QString &filePath, bool deleteFromFS)
{
    // remove from version control
    ICore::vcsManager()->promptToDelete(filePath);

    // remove from file system
    if (deleteFromFS) {
        QFile file(filePath);

        if (file.exists()) {
            // could have been deleted by vc
            if (!file.remove())
                QMessageBox::warning(ICore::mainWindow(),
                    QApplication::translate("Core::Internal", "Deleting File Failed"),
                    QApplication::translate("Core::Internal", "Could not delete file %1.").arg(filePath));
        }
    }
}

static inline bool fileSystemRenameFile(const QString &orgFilePath,
                                        const QString &newFilePath)
{
#if QT_VERSION < 0x050000
    QAbstractFileEngine *fileEngine = QAbstractFileEngine::create(orgFilePath); // Due to QTBUG-3570
    if (!fileEngine->caseSensitive() && orgFilePath.compare(newFilePath, Qt::CaseInsensitive) == 0)
        return fileEngine->rename(newFilePath);
#endif
    // QTBUG-3570 is also valid for Qt 5 but QAbstractFileEngine is now in a private header file and
    // the symbol is not exported.
    return QFile::rename(orgFilePath, newFilePath);
}

bool FileUtils::renameFile(const QString &orgFilePath, const QString &newFilePath)
{
    if (orgFilePath == newFilePath)
        return false;

    QString dir = QFileInfo(orgFilePath).absolutePath();
    Core::IVersionControl *vc = Core::ICore::vcsManager()->findVersionControlForDirectory(dir);

    bool result = false;
    if (vc && vc->supportsOperation(Core::IVersionControl::MoveOperation))
        result = vc->vcsMove(orgFilePath, newFilePath);
    if (!result) // The moving via vcs failed or the vcs does not support moving, fall back
        result = fileSystemRenameFile(orgFilePath, newFilePath);
    if (result) {
        // yeah we moved, tell the filemanager about it
        Core::DocumentManager::renamedFile(orgFilePath, newFilePath);
    }
    return result;
}
