/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "archive.h"

#include "algorithm.h"
#include "checkablemessagebox.h"
#include "environment.h"
#include "mimetypes/mimedatabase.h"
#include "qtcassert.h"
#include "qtcprocess.h"

#include <QDir>
#include <QPushButton>
#include <QSettings>
#include <QTimer>

namespace Utils {

namespace {

struct Tool
{
    CommandLine command;
    QStringList supportedMimeTypes;
    FilePaths additionalSearchDirs;
};

} // anon

static FilePaths additionalInstallDirs(const QString &registryKey, const QString &valueName)
{
#if defined(Q_OS_WIN)
    const QSettings settings64(registryKey, QSettings::Registry64Format);
    const QSettings settings32(registryKey, QSettings::Registry32Format);
    return {FilePath::fromVariant(settings64.value(valueName)),
            FilePath::fromVariant(settings32.value(valueName))};
#else
    Q_UNUSED(registryKey)
    Q_UNUSED(valueName)
    return {};
#endif
}

static const QVector<Tool> &sTools()
{
    static QVector<Tool> tools;
    if (tools.isEmpty()) {
        if (HostOsInfo::isWindowsHost()) {
            tools << Tool{{"powershell", "-command Expand-Archive -Force '%{src}' '%{dest}'", CommandLine::Raw},
                          {"application/zip"},
                          {}};
        }
        tools << Tool{{"unzip", {"-o", "%{src}", "-d", "%{dest}"}}, {"application/zip"}, {}};
        tools << Tool{{"7z", {"x", "-o%{dest}", "-y", "-bb", "%{src}"}},
                      {"application/zip", "application/x-7z-compressed"},
                      additionalInstallDirs("HKEY_CURRENT_USER\\Software\\7-Zip", "Path")};
        tools << Tool{{"tar", {"xvf", "%{src}"}},
                      {"application/zip", "application/x-tar", "application/x-7z-compressed"},
                      {}};
        tools << Tool{{"tar", {"xvzf", "%{src}"}}, {"application/x-compressed-tar"}, {}};
        tools << Tool{{"tar", {"xvJf", "%{src}"}}, {"application/x-xz-compressed-tar"}, {}};
        tools << Tool{{"tar", {"xvjf", "%{src}"}}, {"application/x-bzip-compressed-tar"}, {}};

        const FilePaths additionalCMakeDirs =
                additionalInstallDirs("HKEY_LOCAL_MACHINE\\SOFTWARE\\Kitware\\CMake",
                                      "InstallDir");
        tools << Tool{{"cmake", {"-E", "tar", "xvf", "%{src}"}},
                      {"application/zip", "application/x-tar", "application/x-7z-compressed"},
                      additionalCMakeDirs};
        tools << Tool{{"cmake", {"-E", "tar", "xvzf", "%{src}"}},
                      {"application/x-compressed-tar"},
                      additionalCMakeDirs};
        tools << Tool{{"cmake", {"-E", "tar", "xvJf", "%{src}"}},
                      {"application/x-xz-compressed-tar"},
                      additionalCMakeDirs};
        tools << Tool{{"cmake", {"-E", "tar", "xvjf", "%{src}"}},
                      {"application/x-bzip-compressed-tar"},
                      additionalCMakeDirs};
    }
    return tools;
}

static QVector<Tool> toolsForMimeType(const MimeType &mimeType)
{
    return Utils::filtered(sTools(), [mimeType](const Tool &tool) {
        return Utils::anyOf(tool.supportedMimeTypes,
                            [mimeType](const QString &mt) { return mimeType.inherits(mt); });
    });
}

static QVector<Tool> toolsForFilePath(const FilePath &fp)
{
    return toolsForMimeType(Utils::mimeTypeForFile(fp));
}

static Utils::optional<Tool> resolveTool(const Tool &tool)
{
    const FilePath executable =
        tool.command.executable().withExecutableSuffix().searchInPath(tool.additionalSearchDirs);
    Tool resolvedTool = tool;
    resolvedTool.command.setExecutable(executable);
    return executable.isEmpty() ? Utils::nullopt : Utils::make_optional(resolvedTool);
}

static Utils::optional<Tool> unzipTool(const FilePath &src, const FilePath &dest)
{
    const QVector<Tool> tools = toolsForFilePath(src);
    for (const Tool &tool : tools) {
        const Utils::optional<Tool> resolvedTool = resolveTool(tool);
        if (resolvedTool) {
            Tool result = *resolvedTool;
            const QString srcStr = src.toString();
            const QString destStr = dest.toString();
            const QString args = result.command.arguments().replace("%{src}", srcStr).replace("%{dest}", destStr);
            result.command.setArguments(args);
            return result;
        }
    }
    return {};
}

bool Archive::supportsFile(const FilePath &filePath, QString *reason)
{
    const QVector<Tool> tools = toolsForFilePath(filePath);
    if (tools.isEmpty()) {
        if (reason)
            *reason = tr("File format not supported.");
        return false;
    }
    if (!anyOf(tools, [tools](const Tool &t) { return resolveTool(t); })) {
        if (reason) {
            const QStringList execs = transform<QStringList>(tools, [](const Tool &tool) {
                return tool.command.executable().toString();
            });
            *reason = tr("Could not find any unarchiving executable in PATH (%1).")
                          .arg(execs.join(", "));
        }
        return false;
    }
    return true;
}

bool Archive::unarchive(const FilePath &src, const FilePath &dest, QWidget *parent)
{
    Archive *archive = unarchive(src, dest);
    QTC_ASSERT(archive, return false);

    CheckableMessageBox box(parent);
    box.setIcon(QMessageBox::Information);
    box.setWindowTitle(tr("Unarchiving File"));
    box.setText(tr("Unzipping \"%1\" to \"%2\".").arg(src.toUserOutput(), dest.toUserOutput()));
    box.setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    box.button(QDialogButtonBox::Ok)->setEnabled(false);
    box.setCheckBoxVisible(false);
    QObject::connect(archive, &Archive::outputReceived, &box, [&box](const QString &output) {
        box.setDetailedText(box.detailedText() + output);
    });
    bool success = false;
    QObject::connect(archive, &Archive::finished, [&box, &success](bool ret) {
        box.button(QDialogButtonBox::Ok)->setEnabled(true);
        box.button(QDialogButtonBox::Cancel)->setEnabled(false);
        success = ret;
    });
    QObject::connect(&box, &QMessageBox::rejected, archive, &Archive::cancel);
    box.exec();
    return success;
}

Archive *Archive::unarchive(const FilePath &src, const FilePath &dest)
{
    const Utils::optional<Tool> tool = unzipTool(src, dest);
    QTC_ASSERT(tool, return nullptr);

    auto archive = new Archive;

    const FilePath workingDirectory = dest.absolutePath();
    workingDirectory.ensureWritableDir();

    archive->m_process = new QtcProcess;
    archive->m_process->setProcessChannelMode(QProcess::MergedChannels);
    QObject::connect(
        archive->m_process,
        &QtcProcess::readyReadStandardOutput,
        archive,
        [archive]() {
            if (!archive->m_process)
                return;
            emit archive->outputReceived(QString::fromUtf8(
                                             archive->m_process->readAllStandardOutput()));
        },
        Qt::QueuedConnection);
    QObject::connect(
        archive->m_process,
        &QtcProcess::finished,
        archive,
        [archive] {
            if (!archive->m_process)
                return;
            emit archive->finished(archive->m_process->result() == QtcProcess::FinishedWithSuccess);
            archive->m_process->deleteLater();
            archive->m_process = nullptr;
            archive->deleteLater();
        },
        Qt::QueuedConnection);
    QObject::connect(
        archive->m_process,
        &QtcProcess::errorOccurred,
        archive,
        [archive](QProcess::ProcessError) {
            if (!archive->m_process)
                return;
            emit archive->outputReceived(tr("Command failed."));
            emit archive->finished(false);
            archive->m_process->deleteLater();
            archive->m_process = nullptr;
            archive->deleteLater();
        },
        Qt::QueuedConnection);

    QTimer::singleShot(0, archive, [archive, tool, workingDirectory] {
        emit archive->outputReceived(
            tr("Running %1\nin \"%2\".\n\n", "Running <cmd> in <workingdirectory>")
                .arg(tool->command.toUserOutput(), workingDirectory.toUserOutput()));
    });

    archive->m_process->setCommand(tool->command);
    archive->m_process->setWorkingDirectory(workingDirectory);
    archive->m_process->start();
    return archive;
}

void Archive::cancel()
{
    if (m_process)
        m_process->stopProcess();
}

} // namespace Utils
