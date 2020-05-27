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
    Utils::FilePath executable;
    QStringList arguments;
};

Utils::optional<Tool> unzipTool(const Utils::FilePath &src, const Utils::FilePath &dest)
{
    const Utils::FilePath unzip = Utils::Environment::systemEnvironment().searchInPath(
        Utils::HostOsInfo::withExecutableSuffix("unzip"));
    if (!unzip.isEmpty())
        return Tool{unzip, {"-o", src.toString(), "-d", dest.toString()}};

    const Utils::FilePath sevenzip = Utils::Environment::systemEnvironment().searchInPath(
        Utils::HostOsInfo::withExecutableSuffix("7z"));
    if (!sevenzip.isEmpty())
        return Tool{sevenzip, {"x", QString("-o") + dest.toString(), "-y", src.toString()}};

    const Utils::FilePath cmake = Utils::Environment::systemEnvironment().searchInPath(
        Utils::HostOsInfo::withExecutableSuffix("cmake"));
    if (!cmake.isEmpty())
        return Tool{cmake, {"-E", "tar", "xvf", src.toString()}};

    return {};
}

} // namespace

namespace Utils {

bool Archive::supportsFile(const FilePath &filePath, QString *reason)
{
    const QList<MimeType> mimeType = mimeTypesForFileName(filePath.toString());
    if (!anyOf(mimeType, [](const MimeType &mt) { return mt.inherits("application/zip"); })) {
        if (reason)
            *reason = tr("File format not supported.");
        return false;
    }
    if (!unzipTool({}, {})) {
        if (reason)
            *reason = tr("Could not find unzip, 7z, or cmake executable in PATH.");
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
    box.setWindowTitle(tr("Unzipping File"));
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
    process.setProgram(tool->executable.toString());
    process.setArguments(tool->arguments);
    process.setWorkingDirectory(workingDirectory);
    process.start(QProcess::ReadOnly);
    box.exec();
    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

} // namespace Utils
