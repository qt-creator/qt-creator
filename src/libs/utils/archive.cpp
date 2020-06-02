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
#include "synchronousprocess.h"

#include <QDir>
#include <QPushButton>

namespace {

struct Tool
{
    QString executable;
    QStringList arguments;
    QStringList supportedMimeTypes;
};

const QVector<Tool> sTools = {
    {{"unzip"}, {"-o", "%{src}", "-d", "%{dest}"}, {"application/zip"}},
    {{"7z"}, {"x", "-o%{dest}", "-y", "%{src}"}, {"application/zip", "application/x-7z-compressed"}},
    {{"tar"},
     {"xvf", "%{src}"},
     {"application/zip", "application/x-tar", "application/x-7z-compressed"}},
    {{"tar"}, {"xvzf", "%{src}"}, {"application/x-compressed-tar"}},
    {{"tar"}, {"xvJf", "%{src}"}, {"application/x-xz-compressed-tar"}},
    {{"tar"}, {"xvjf", "%{src}"}, {"application/x-bzip-compressed-tar"}},
    {{"cmake"},
     {"-E", "tar", "xvf", "%{src}"},
     {"application/zip", "application/x-tar", "application/x-7z-compressed"}},
    {{"cmake"}, {"-E", "tar", "xvzf", "%{src}"}, {"application/x-compressed-tar"}},
    {{"cmake"}, {"-E", "tar", "xvJf", "%{src}"}, {"application/x-xz-compressed-tar"}},
    {{"cmake"}, {"-E", "tar", "xvjf", "%{src}"}, {"application/x-bzip-compressed-tar"}},
};

static QVector<Tool> toolsForMimeType(const Utils::MimeType &mimeType)
{
    return Utils::filtered(sTools, [mimeType](const Tool &tool) {
        return Utils::anyOf(tool.supportedMimeTypes,
                            [mimeType](const QString &mt) { return mimeType.inherits(mt); });
    });
}

static QVector<Tool> toolsForFilePath(const Utils::FilePath &fp)
{
    return toolsForMimeType(Utils::mimeTypeForFile(fp.toString()));
}

static Utils::optional<Tool> resolveTool(const Tool &tool)
{
    const QString executable = Utils::Environment::systemEnvironment()
                                   .searchInPath(
                                       Utils::HostOsInfo::withExecutableSuffix(tool.executable))
                                   .toString();
    Tool resolvedTool = tool;
    resolvedTool.executable = executable;
    return executable.isEmpty() ? Utils::nullopt : Utils::make_optional(resolvedTool);
}

Utils::optional<Tool> unzipTool(const Utils::FilePath &src, const Utils::FilePath &dest)
{
    const QVector<Tool> tools = toolsForFilePath(src);
    for (const Tool &tool : tools) {
        const Utils::optional<Tool> resolvedTool = resolveTool(tool);
        if (resolvedTool) {
            Tool result = *resolvedTool;
            const QString srcStr = src.toString();
            const QString destStr = dest.toString();
            result.arguments
                = Utils::transform(result.arguments, [srcStr, destStr](const QString &a) {
                      QString val = a;
                      return val.replace("%{src}", srcStr).replace("%{dest}", destStr);
                  });
            return result;
        }
    }
    return {};
}

} // namespace

namespace Utils {

bool Archive::supportsFile(const FilePath &filePath, QString *reason)
{
    const QVector<Tool> tools = toolsForFilePath(filePath);
    if (tools.isEmpty()) {
        if (reason)
            *reason = tr("File format not supported.");
        return false;
    }
    if (!anyOf(tools, [](const Tool &t) { return resolveTool(t); })) {
        if (reason)
            *reason = tr("Could not find any unarchiving executable in PATH (%1).")
                          .arg(transform<QStringList>(tools, &Tool::executable).join(", "));
        return false;
    }
    return true;
}

bool Archive::unarchive(const FilePath &src, const FilePath &dest, QWidget *parent)
{
    const Utils::optional<Tool> tool = unzipTool(src, dest);
    QTC_ASSERT(tool, return false);
    const QString workingDirectory = dest.toFileInfo().absoluteFilePath();
    QDir(workingDirectory).mkpath(".");
    CheckableMessageBox box(parent);
    box.setIcon(QMessageBox::Information);
    box.setWindowTitle(tr("Unarchiving File"));
    box.setText(tr("Unzipping \"%1\" to \"%2\".").arg(src.toUserOutput(), dest.toUserOutput()));
    box.setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    box.button(QDialogButtonBox::Ok)->setEnabled(false);
    box.setCheckBoxVisible(false);
    box.setDetailedText(
        tr("Running %1\nin \"%2\".\n\n", "Running <cmd> in <workingdirectory>")
            .arg(CommandLine(tool->executable, tool->arguments).toUserOutput(), workingDirectory));
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    QObject::connect(&process, &QProcess::readyReadStandardOutput, &box, [&box, &process]() {
        box.setDetailedText(box.detailedText() + QString::fromUtf8(process.readAllStandardOutput()));
    });
    QObject::connect(&process,
                     QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     [&box](int, QProcess::ExitStatus) {
                         box.button(QDialogButtonBox::Ok)->setEnabled(true);
                         box.button(QDialogButtonBox::Cancel)->setEnabled(false);
                     });
    QObject::connect(&box, &QMessageBox::rejected, &process, [&process] {
        SynchronousProcess::stopProcess(process);
    });
    process.setProgram(tool->executable);
    process.setArguments(tool->arguments);
    process.setWorkingDirectory(workingDirectory);
    process.start(QProcess::ReadOnly);
    box.exec();
    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

} // namespace Utils
