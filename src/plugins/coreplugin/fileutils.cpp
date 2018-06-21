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

#include "fileutils.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>
#include <utils/consoleprocess.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/unixutils.h>

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QWidget>

using namespace Utils;

namespace Core {

// Show error with option to open settings.
static void showGraphicalShellError(QWidget *parent, const QString &app, const QString &error)
{
    const QString title = QApplication::translate("Core::Internal",
                                                  "Launching a file browser failed");
    const QString msg = QApplication::translate("Core::Internal",
                                                "Unable to start the file manager:\n\n%1\n\n").arg(app);
    QMessageBox mbox(QMessageBox::Warning, title, msg, QMessageBox::Close, parent);
    if (!error.isEmpty())
        mbox.setDetailedText(QApplication::translate("Core::Internal",
                                                     "\"%1\" returned the following error:\n\n%2").arg(app, error));
    QAbstractButton *settingsButton = mbox.addButton(Core::ICore::msgShowOptionsDialog(),
                                                     QMessageBox::ActionRole);
    mbox.exec();
    if (mbox.clickedButton() == settingsButton)
        ICore::showOptionsDialog(Constants::SETTINGS_ID_INTERFACE, parent);
}

void FileUtils::showInGraphicalShell(QWidget *parent, const QString &pathIn)
{
    const QFileInfo fileInfo(pathIn);
    // Mac, Windows support folder or file.
    if (HostOsInfo::isWindowsHost()) {
        const FileName explorer = Environment::systemEnvironment().searchInPath(QLatin1String("explorer.exe"));
        if (explorer.isEmpty()) {
            QMessageBox::warning(parent,
                                 QApplication::translate("Core::Internal",
                                                         "Launching Windows Explorer Failed"),
                                 QApplication::translate("Core::Internal",
                                                         "Could not find explorer.exe in path to launch Windows Explorer."));
            return;
        }
        QStringList param;
        if (!fileInfo.isDir())
            param += QLatin1String("/select,");
        param += QDir::toNativeSeparators(fileInfo.canonicalFilePath());
        QProcess::startDetached(explorer.toString(), param);
    } else if (HostOsInfo::isMacHost()) {
        QStringList scriptArgs;
        scriptArgs << QLatin1String("-e")
                   << QString::fromLatin1("tell application \"Finder\" to reveal POSIX file \"%1\"")
                                         .arg(fileInfo.canonicalFilePath());
        QProcess::execute(QLatin1String("/usr/bin/osascript"), scriptArgs);
        scriptArgs.clear();
        scriptArgs << QLatin1String("-e")
                   << QLatin1String("tell application \"Finder\" to activate");
        QProcess::execute(QLatin1String("/usr/bin/osascript"), scriptArgs);
    } else {
        // we cannot select a file here, because no file browser really supports it...
        const QString folder = fileInfo.isDir() ? fileInfo.absoluteFilePath() : fileInfo.filePath();
        const QString app = UnixUtils::fileBrowser(ICore::settings());
        QProcess browserProc;
        const QString browserArgs = UnixUtils::substituteFileBrowserParameters(app, folder);
        bool success = browserProc.startDetached(browserArgs);
        const QString error = QString::fromLocal8Bit(browserProc.readAllStandardError());
        success = success && error.isEmpty();
        if (!success)
            showGraphicalShellError(parent, app, error);
    }
}

void FileUtils::openTerminal(const QString &path)
{
    const QFileInfo fileInfo(path);
    const QString pwd = QDir::toNativeSeparators(fileInfo.isDir() ?
                                                 fileInfo.absoluteFilePath() :
                                                 fileInfo.absolutePath());
    ConsoleProcess::startTerminalEmulator(ICore::settings(), pwd);
}

QString FileUtils::msgFindInDirectory()
{
    return QApplication::translate("Core::Internal", "Find in This Directory...");
}

QString FileUtils::msgGraphicalShellAction()
{
    if (HostOsInfo::isWindowsHost())
        return QApplication::translate("Core::Internal", "Show in Explorer");
    if (HostOsInfo::isMacHost())
        return QApplication::translate("Core::Internal", "Show in Finder");
    return QApplication::translate("Core::Internal", "Show Containing Folder");
}

QString FileUtils::msgTerminalAction()
{
    if (HostOsInfo::isWindowsHost())
        return QApplication::translate("Core::Internal", "Open Command Prompt Here");
    return QApplication::translate("Core::Internal", "Open Terminal Here");
}

void FileUtils::removeFile(const QString &filePath, bool deleteFromFS)
{
    // remove from version control
    VcsManager::promptToDelete(filePath);

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
    // QTBUG-3570 is also valid for Qt 5 but QAbstractFileEngine is now in a private header file and
    // the symbol is not exported.
    return QFile::rename(orgFilePath, newFilePath);
}

bool FileUtils::renameFile(const QString &orgFilePath, const QString &newFilePath)
{
    if (orgFilePath == newFilePath)
        return false;

    QString dir = QFileInfo(orgFilePath).absolutePath();
    IVersionControl *vc = VcsManager::findVersionControlForDirectory(dir);

    bool result = false;
    if (vc && vc->supportsOperation(IVersionControl::MoveOperation))
        result = vc->vcsMove(orgFilePath, newFilePath);
    if (!result) // The moving via vcs failed or the vcs does not support moving, fall back
        result = fileSystemRenameFile(orgFilePath, newFilePath);
    if (result) {
        // yeah we moved, tell the filemanager about it
        DocumentManager::renamedFile(orgFilePath, newFilePath);
    }
    return result;
}

} // namespace Core
