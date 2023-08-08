// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fileutils.h"

#include "coreconstants.h"
#include "coreplugintr.h"
#include "documentmanager.h"
#include "editormanager/editormanager.h"
#include "foldernavigationwidget.h"
#include "icore.h"
#include "iversioncontrol.h"
#include "messagemanager.h"
#include "navigationwidget.h"
#include "vcsmanager.h"

#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/process.h>
#include <utils/terminalcommand.h>
#include <utils/terminalhooks.h>
#include <utils/textfileformat.h>
#include <utils/unixutils.h>

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextStream>
#include <QTextCodec>

using namespace Utils;

namespace Core {

// Show error with option to open settings.
static void showGraphicalShellError(QWidget *parent, const QString &app, const QString &error)
{
    const QString title = Tr::tr("Launching a file browser failed");
    const QString msg = Tr::tr("Unable to start the file manager:\n\n%1\n\n").arg(app);
    QMessageBox mbox(QMessageBox::Warning, title, msg, QMessageBox::Close, parent);
    if (!error.isEmpty())
        mbox.setDetailedText(Tr::tr("\"%1\" returned the following error:\n\n%2").arg(app, error));
    QAbstractButton *settingsButton = mbox.addButton(Core::ICore::msgShowOptionsDialog(),
                                                     QMessageBox::ActionRole);
    mbox.exec();
    if (mbox.clickedButton() == settingsButton)
        ICore::showOptionsDialog(Constants::SETTINGS_ID_INTERFACE, parent);
}

void FileUtils::showInGraphicalShell(QWidget *parent, const FilePath &pathIn)
{
    const QFileInfo fileInfo = pathIn.toFileInfo();
    // Mac, Windows support folder or file.
    if (HostOsInfo::isWindowsHost()) {
        const FilePath explorer = FilePath("explorer.exe").searchInPath();
        if (explorer.isEmpty()) {
            QMessageBox::warning(parent,
                                 Tr::tr("Launching Windows Explorer Failed"),
                                 Tr::tr("Could not find explorer.exe in path to launch Windows Explorer."));
            return;
        }
        QStringList param;
        if (!pathIn.isDir())
            param += QLatin1String("/select,");
        param += QDir::toNativeSeparators(fileInfo.canonicalFilePath());
        Process::startDetached({explorer, param});
    } else if (HostOsInfo::isMacHost()) {
        Process::startDetached({"/usr/bin/open", {"-R", fileInfo.canonicalFilePath()}});
    } else {
        // we cannot select a file here, because no file browser really supports it...
        const QString folder = fileInfo.isDir() ? fileInfo.absoluteFilePath() : fileInfo.filePath();
        const QString app = UnixUtils::fileBrowser(ICore::settings());
        QStringList browserArgs = ProcessArgs::splitArgs(
                    UnixUtils::substituteFileBrowserParameters(app, folder),
                    HostOsInfo::hostOs());
        QString error;
        if (browserArgs.isEmpty()) {
            error = Tr::tr("The command for file browser is not set.");
        } else {
            const QString executable = browserArgs.takeFirst();
            if (!Process::startDetached({FilePath::fromString(executable), browserArgs}))
                error = Tr::tr("Error while starting file browser.");
        }
        if (!error.isEmpty())
            showGraphicalShellError(parent, app, error);
    }
}

void FileUtils::showInFileSystemView(const FilePath &path)
{
    QWidget *widget
        = NavigationWidget::activateSubWidget(FolderNavigationWidgetFactory::instance()->id(),
                                              Side::Left);
    if (auto *navWidget = qobject_cast<FolderNavigationWidget *>(widget))
        navWidget->syncWithFilePath(path);
}

void FileUtils::openTerminal(const FilePath &path, const Environment &env)
{
    Terminal::Hooks::instance().openTerminal({path, env});
}

QString FileUtils::msgFindInDirectory()
{
    return Tr::tr("Find in This Directory...");
}

QString FileUtils::msgFileSystemAction()
{
    return Tr::tr("Show in File System View");
}

QString FileUtils::msgGraphicalShellAction()
{
    if (HostOsInfo::isWindowsHost())
        return Tr::tr("Show in Explorer");
    if (HostOsInfo::isMacHost())
        return Tr::tr("Show in Finder");
    return Tr::tr("Show Containing Folder");
}

QString FileUtils::msgTerminalHereAction()
{
    if (HostOsInfo::isWindowsHost())
        return Tr::tr("Open Command Prompt Here");
    return Tr::tr("Open Terminal Here");
}

QString FileUtils::msgTerminalWithAction()
{
    if (HostOsInfo::isWindowsHost())
        return Tr::tr("Open Command Prompt With",
                      "Opens a submenu for choosing an environment, such as \"Run Environment\"");
    return Tr::tr("Open Terminal With",
                  "Opens a submenu for choosing an environment, such as \"Run Environment\"");
}

void FileUtils::removeFiles(const FilePaths &filePaths, bool deleteFromFS)
{
    // remove from version control
    VcsManager::promptToDelete(filePaths);

    if (!deleteFromFS)
        return;

    // remove from file system
    for (const FilePath &fp : filePaths) {
        QFile file(fp.toString());
        if (!file.exists()) // could have been deleted by vc
            continue;
        if (!file.remove()) {
            MessageManager::writeDisrupting(
                Tr::tr("Failed to remove file \"%1\".").arg(fp.toUserOutput()));
        }
    }
}

bool FileUtils::renameFile(const FilePath &orgFilePath, const FilePath &newFilePath,
                           HandleIncludeGuards handleGuards)
{
    if (orgFilePath == newFilePath)
        return false;

    const FilePath dir = orgFilePath.absolutePath();
    IVersionControl *vc = VcsManager::findVersionControlForDirectory(dir);
    const FilePath newDir = newFilePath.absolutePath();
    if (newDir != dir && !newDir.ensureWritableDir())
        return false;

    bool result = false;
    if (vc && vc->supportsOperation(IVersionControl::MoveOperation))
        result = vc->vcsMove(orgFilePath, newFilePath);
    if (!result) // The moving via vcs failed or the vcs does not support moving, fall back
        result = orgFilePath.renameFile(newFilePath);
    if (result) {
        DocumentManager::renamedFile(orgFilePath, newFilePath);
        updateHeaderFileGuardIfApplicable(orgFilePath, newFilePath, handleGuards);
    }
    return result;
}

void FileUtils::updateHeaderFileGuardIfApplicable(const Utils::FilePath &oldFilePath,
                                                  const Utils::FilePath &newFilePath,
                                                  HandleIncludeGuards handleGuards)
{
    if (handleGuards == HandleIncludeGuards::No)
        return;
    const bool headerUpdateSuccess = updateHeaderFileGuardAfterRename(newFilePath.toString(),
                                                                      oldFilePath.baseName());
    if (headerUpdateSuccess)
        return;
    MessageManager::writeDisrupting(
                Tr::tr("Failed to rename the include guard in file \"%1\".")
                .arg(newFilePath.toUserOutput()));
}

bool FileUtils::updateHeaderFileGuardAfterRename(const QString &headerPath,
                                                 const QString &oldHeaderBaseName)
{
    bool ret = true;
    QFile headerFile(headerPath);
    if (!headerFile.open(QFile::ReadOnly | QFile::Text))
        return false;

    QRegularExpression guardConditionRegExp(
                QString("(#ifndef)(\\s*)(_*)%1_H(_*)(\\s*)").arg(oldHeaderBaseName.toUpper()));
    QRegularExpression guardDefineRegexp, guardCloseRegExp;
    QRegularExpressionMatch guardConditionMatch, guardDefineMatch, guardCloseMatch;
    int guardStartLine = -1;
    int guardCloseLine = -1;

    const QByteArray data = headerFile.readAll();
    headerFile.close();

    auto headerFileTextFormat = Utils::TextFileFormat::detect(data);
    if (!headerFileTextFormat.codec)
        headerFileTextFormat.codec = EditorManager::defaultTextCodec();
    QString stringContent;
    if (!headerFileTextFormat.decode(data, &stringContent))
        return false;
    QTextStream inStream(&stringContent);
    int lineCounter = 0;
    QString line;
    while (!inStream.atEnd()) {
        // use trimmed line to get rid from the maunder leading spaces
        inStream.readLineInto(&line);
        line = line.trimmed();
        if (line == QStringLiteral("#pragma once")) {
            // if pragma based guard found skip reading the whole file
            break;
        }

        if (guardStartLine == -1) {
            // we are still looking for the guard condition
            guardConditionMatch = guardConditionRegExp.match(line);
            if (guardConditionMatch.hasMatch()) {
                guardDefineRegexp.setPattern(QString("(#define\\s*%1)%2(_H%3\\s*)")
                                             .arg(guardConditionMatch.captured(3),
                                                  oldHeaderBaseName.toUpper(),
                                                  guardConditionMatch.captured(4)));
                // read the next line for the guard define
                line = inStream.readLine();
                if (!inStream.atEnd()) {
                    guardDefineMatch = guardDefineRegexp.match(line);
                    if (guardDefineMatch.hasMatch()) {
                        // if a proper guard define present in the next line store the line number
                        guardCloseRegExp
                                .setPattern(
                                    QString("(#endif\\s*)(\\/\\/|\\/\\*)(\\s*%1)%2(_H%3\\s*)((\\*\\/)?)")
                                                    .arg(
                                                        guardConditionMatch.captured(3),
                                                        oldHeaderBaseName.toUpper(),
                                                        guardConditionMatch.captured(4)));
                        guardStartLine = lineCounter;
                        lineCounter++;
                    }
                } else {
                    // it the line after the guard opening is not something what we expect
                    // then skip the whole guard replacing process
                    break;
                }
            }
        } else {
            // guard start found looking for the guard closing endif
            guardCloseMatch = guardCloseRegExp.match(line);
            if (guardCloseMatch.hasMatch()) {
                guardCloseLine = lineCounter;
                break;
            }
        }
        lineCounter++;
    }

    if (guardStartLine != -1) {
        // At least the guard have been found ->
        // copy the contents of the header to a temporary file with the updated guard lines
        inStream.seek(0);

        QFileInfo fi(headerFile);
        const auto guardCondition = QString("#ifndef%1%2%3_H%4%5").arg(
                    guardConditionMatch.captured(2),
                    guardConditionMatch.captured(3),
                    fi.baseName().toUpper(),
                    guardConditionMatch.captured(4),
                    guardConditionMatch.captured(5));
        // guardDefineRegexp.setPattern(QString("(#define\\s*%1)%2(_H%3\\s*)")
        const auto guardDefine = QString("%1%2%3").arg(
                    guardDefineMatch.captured(1),
                    fi.baseName().toUpper(),
                    guardDefineMatch.captured(2));
        const auto guardClose = QString("%1%2%3%4%5%6").arg(
                    guardCloseMatch.captured(1),
                    guardCloseMatch.captured(2),
                    guardCloseMatch.captured(3),
                    fi.baseName().toUpper(),
                    guardCloseMatch.captured(4),
                    guardCloseMatch.captured(5));

        QFile tmpHeader(headerPath + ".tmp");
        if (tmpHeader.open(QFile::WriteOnly)) {
            const auto lineEnd =
                    headerFileTextFormat.lineTerminationMode
                    == Utils::TextFileFormat::LFLineTerminator
                    ? QStringLiteral("\n") : QStringLiteral("\r\n");
            // write into temporary string,
            // after that write with codec into file (QTextStream::setCodec is gone in Qt 6)
            QString outString;
            QTextStream outStream(&outString);
            int lineCounter = 0;
            while (!inStream.atEnd()) {
                inStream.readLineInto(&line);
                if (lineCounter == guardStartLine) {
                    outStream << guardCondition << lineEnd;
                    outStream << guardDefine << lineEnd;
                    inStream.readLine();
                    lineCounter++;
                } else if (lineCounter == guardCloseLine) {
                    outStream << guardClose << lineEnd;
                } else {
                    outStream << line << lineEnd;
                }
                lineCounter++;
            }
            tmpHeader.write(headerFileTextFormat.codec->fromUnicode(outString));
            tmpHeader.close();
        } else {
            // if opening the temp file failed report error
            ret = false;
        }
    }

    if (ret && guardStartLine != -1) {
        // if the guard was found (and updated updated properly) swap the temp and the target file
        ret = QFile::remove(headerPath);
        if (ret)
            ret = QFile::rename(headerPath + ".tmp", headerPath);
    }

    return ret;
}

} // namespace Core
