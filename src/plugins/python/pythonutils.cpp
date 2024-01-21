// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pythonutils.h"

#include "pythonbuildconfiguration.h"
#include "pythonconstants.h"
#include "pythonkitaspect.h"
#include "pythonproject.h"
#include "pythonsettings.h"
#include "pythontr.h"

#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/processprogress.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <utils/algorithm.h>
#include <utils/mimeutils.h>
#include <utils/persistentcachestore.h>
#include <utils/process.h>

#include <QReadLocker>

using namespace ProjectExplorer;
using namespace Utils;

namespace Python::Internal {

static QHash<FilePath, FilePath> &userDefinedPythonsForDocument()
{
    static QHash<FilePath, FilePath> userDefines;
    return userDefines;
}

FilePath detectPython(const FilePath &documentPath)
{
    Project *project = documentPath.isEmpty() ? nullptr
                                              : ProjectManager::projectForFile(documentPath);
    if (!project)
        project = ProjectManager::startupProject();

    FilePaths dirs = Environment::systemEnvironment().path();

    if (project && project->mimeType() == Constants::C_PY_PROJECT_MIME_TYPE) {
        if (const Target *target = project->activeTarget()) {
            if (auto bc = qobject_cast<PythonBuildConfiguration *>(target->activeBuildConfiguration()))
                return bc->python();
            if (const std::optional<Interpreter> python = PythonKitAspect::python(target->kit()))
                return python->command;
        }
    }

    const FilePath userDefined = userDefinedPythonsForDocument().value(documentPath);
    if (userDefined.exists())
        return userDefined;

    // check whether this file is inside a python virtual environment
    const QList<Interpreter> venvInterpreters = PythonSettings::detectPythonVenvs(documentPath);
    if (!venvInterpreters.isEmpty())
        return venvInterpreters.first().command;

    auto defaultInterpreter = PythonSettings::defaultInterpreter().command;
    if (defaultInterpreter.exists())
        return defaultInterpreter;

    auto pythonFromPath = [dirs](const FilePath &toCheck) {
        const FilePaths found = toCheck.searchAllInDirectories(dirs);
        for (const FilePath &python : found) {
            // Windows creates empty redirector files that may interfere
            if (python.exists() && python.osType() == OsTypeWindows && python.fileSize() != 0)
                return python;
        }
        return FilePath();
    };

    const FilePath fromPath3 = pythonFromPath("python3");
    if (fromPath3.exists())
        return fromPath3;

    const FilePath fromPath = pythonFromPath("python");
    if (fromPath.exists())
        return fromPath;

    return PythonSettings::interpreters().value(0).command;
}

void definePythonForDocument(const FilePath &documentPath, const FilePath &python)
{
    userDefinedPythonsForDocument()[documentPath] = python;
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
    Q_UNUSED(parent)

    static const auto workingDir = [](const FilePath &file) {
        if (file.isEmpty()) {
            if (Project *project = ProjectManager::startupProject())
                return project->projectDirectory();
            return FilePath::currentWorkingPath();
        }
        return file.absolutePath();
    };

    const auto args = QStringList{"-i"} + replImportArgs(file, type);
    const FilePath pythonCommand = detectPython(file);

    Process process;
    process.setCommand({pythonCommand, args});
    process.setWorkingDirectory(workingDir(file));
    process.setTerminalMode(TerminalMode::Detached);
    process.start();

    if (process.error() != QProcess::UnknownError) {
        Core::MessageManager::writeDisrupting(
            Tr::tr((process.error() == QProcess::FailedToStart)
                       ? "Failed to run Python (%1): \"%2\"."
                       : "Error while running Python (%1): \"%2\".")
                .arg(process.commandLine().toUserOutput(), process.errorString()));
    }
}

QString pythonName(const FilePath &pythonPath)
{
    static QHash<FilePath, QString> nameForPython;
    if (!pythonPath.exists())
        return {};
    QString name = nameForPython.value(pythonPath);
    if (name.isEmpty()) {
        Process pythonProcess;
        pythonProcess.setTimeoutS(2);
        pythonProcess.setCommand({pythonPath, {"--version"}});
        pythonProcess.runBlocking();
        if (pythonProcess.result() != ProcessResult::FinishedWithSuccess)
            return {};
        name = pythonProcess.allOutput().trimmed();
        nameForPython[pythonPath] = name;
    }
    return name;
}

PythonProject *pythonProjectForFile(const FilePath &pythonFile)
{
    for (Project *project : ProjectManager::projects()) {
        if (auto pythonProject = qobject_cast<PythonProject *>(project)) {
            if (pythonProject->isKnownFile(pythonFile))
                return pythonProject;
        }
    }
    return nullptr;
}

void createVenv(const FilePath &python,
                const FilePath &venvPath,
                const std::function<void(bool)> &callback)
{
    QTC_ASSERT(python.isExecutableFile(), callback(false); return);
    QTC_ASSERT(!venvPath.exists() || venvPath.isDir(), callback(false); return);

    const CommandLine command(python, QStringList{"-m", "venv", venvPath.toUserOutput()});

    auto process = new Process;
    auto progress = new Core::ProcessProgress(process);
    progress->setDisplayName(Tr::tr("Create Python venv"));
    QObject::connect(process, &Process::done, [process, callback](){
        callback(process->result() == ProcessResult::FinishedWithSuccess);
        process->deleteLater();
    });
    process->setCommand(command);
    process->start();
}

bool isVenvPython(const FilePath &python)
{
    return python.parentDir().parentDir().pathAppended("pyvenv.cfg").exists();
}

static bool isUsableHelper(QHash<FilePath, bool> *cache, const QString &keyString,
                           const QString &commandArg, const FilePath &python)
{
    auto it = cache->find(python);
    if (it == cache->end()) {
        const Key key = keyFromString(keyString);
        const auto store = PersistentCacheStore::byKey(key);
        if (store && store->value(keyFromString(python.toString())).toBool()) {
            cache->insert(python, true);
            return true;
        }
        Process process;
        process.setCommand({python, QStringList{"-m", commandArg, "-h"}});
        process.runBlocking();
        const bool usable = process.result() == ProcessResult::FinishedWithSuccess;
        if (usable) {
            Store newStore = store.value_or(Store{});
            newStore.insert(keyFromString(python.toString()), true);
            PersistentCacheStore::write(key, newStore);
        }
        it = cache->insert(python, usable);
    }
    return *it;
}

bool venvIsUsable(const FilePath &python)
{
    static QHash<FilePath, bool> cache;
    return isUsableHelper(&cache, "pyVenvIsUsable", "venv", python);
}

bool pipIsUsable(const FilePath &python)
{
    static QHash<FilePath, bool> cache;
    return isUsableHelper(&cache, "pyPipIsUsable", "pip", python);
}

QString pythonVersion(const FilePath &python)
{
    static QReadWriteLock lock;
    static QMap<FilePath, QString> versionCache;

    {
        QReadLocker locker(&lock);
        auto it = versionCache.constFind(python);
        if (it != versionCache.constEnd())
            return *it;
    }

    Process p;
    p.setCommand({python, {"--version"}});
    p.runBlocking();
    if (p.result() == ProcessResult::FinishedWithSuccess) {
        const QString version = p.readAllStandardOutput().trimmed();
        QWriteLocker locker(&lock);
        versionCache.insert(python, version);
        return version;
    }
    return QString();
}

} // Python::Internal
