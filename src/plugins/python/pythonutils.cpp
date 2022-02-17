/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "pythonutils.h"

#include "pythonproject.h"
#include "pythonrunconfiguration.h"
#include "pythonsettings.h"

#include <coreplugin/messagemanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <utils/algorithm.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcprocess.h>

using namespace Utils;

namespace Python {
namespace Internal {

FilePath detectPython(const FilePath &documentPath)
{
    FilePath python;

    PythonProject *project = documentPath.isEmpty()
                                 ? nullptr
                                 : qobject_cast<PythonProject *>(
                                     ProjectExplorer::SessionManager::projectForFile(documentPath));
    if (!project)
        project = qobject_cast<PythonProject *>(ProjectExplorer::SessionManager::startupProject());

    if (project) {
        if (auto target = project->activeTarget()) {
            if (auto runConfig = qobject_cast<PythonRunConfiguration *>(
                    target->activeRunConfiguration())) {
                python = FilePath::fromString(runConfig->interpreter());
            }
        }
    }

    // check whether this file is inside a python virtual environment
    QList<Interpreter> venvInterpreters = PythonSettings::detectPythonVenvs(documentPath);
    if (!python.exists())
        python = venvInterpreters.value(0).command;

    if (!python.exists())
        python = PythonSettings::defaultInterpreter().command;

    if (!python.exists() && !PythonSettings::interpreters().isEmpty())
        python = PythonSettings::interpreters().constFirst().command;

    return python;
}

static QStringList replImportArgs(const FilePath &pythonFile, ReplType type)
{
    using MimeTypes = QList<MimeType>;
    const MimeTypes mimeTypes = pythonFile.isEmpty() || type == ReplType::Unmodified
                                    ? MimeTypes()
                                    : mimeTypesForFileName(pythonFile.toString());
    const bool isPython = Utils::anyOf(mimeTypes, [](const MimeType &mt) {
        return mt.inherits("text/x-python") || mt.inherits("text/x-python3");
    });
    if (type == ReplType::Unmodified || !isPython)
        return {};
    const auto import = type == ReplType::Import
                            ? QString("import %1").arg(pythonFile.completeBaseName())
                            : QString("from %1 import *").arg(pythonFile.completeBaseName());
    return {"-c", QString("%1; print('Running \"%1\"')").arg(import)};
}

void openPythonRepl(QObject *parent, const FilePath &file, ReplType type)
{
    static const auto workingDir = [](const FilePath &file) {
        if (file.isEmpty()) {
            if (ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject())
                return project->projectDirectory();
            return FilePath::fromString(QDir::currentPath());
        }
        return file.absolutePath();
    };

    const auto args = QStringList{"-i"} + replImportArgs(file, type);
    auto process = new QtcProcess(parent);
    process->setTerminalMode(TerminalMode::On);
    const FilePath pythonCommand = detectPython(file);
    process->setCommand({pythonCommand, args});
    process->setWorkingDirectory(workingDir(file));
    const QString commandLine = process->commandLine().toUserOutput();
    QObject::connect(process,
                     &QtcProcess::errorOccurred,
                     process,
                     [process, commandLine] {
                         Core::MessageManager::writeDisrupting(
                             QCoreApplication::translate("Python",
                                                         "Failed to run Python (%1): \"%2\".")
                                 .arg(commandLine, process->errorString()));
                         process->deleteLater();
                     });
    QObject::connect(process, &QtcProcess::finished, process, &QObject::deleteLater);
    process->start();
}

} // namespace Internal
} // namespace Python
