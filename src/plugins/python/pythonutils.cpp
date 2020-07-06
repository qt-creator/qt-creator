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

#include "pythonconstants.h"
#include "pythonplugin.h"
#include "pythonproject.h"
#include "pythonrunconfiguration.h"
#include "pythonsettings.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <languageclient/languageclientmanager.h>
#include <languageclient/languageclientsettings.h>

#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <texteditor/textdocument.h>

#include <utils/consoleprocess.h>
#include <utils/infobar.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>
#include <utils/synchronousprocess.h>

#include <QDir>
#include <QFutureWatcher>
#include <QRegularExpression>
#include <QTimer>

using namespace LanguageClient;
using namespace Utils;

namespace Python {
namespace Internal {

static constexpr char startPylsInfoBarId[] = "Python::StartPyls";
static constexpr char installPylsInfoBarId[] = "Python::InstallPyls";
static constexpr char enablePylsInfoBarId[] = "Python::EnablePyls";
static constexpr char installPylsTaskId[] = "Python::InstallPylsTask";

struct PythonLanguageServerState
{
    enum {
        CanNotBeInstalled,
        CanBeInstalled,
        AlreadyInstalled,
        AlreadyConfigured,
        ConfiguredButDisabled
    } state;
    FilePath pylsModulePath;
};

static QString pythonName(const FilePath &pythonPath)
{
    static QHash<FilePath, QString> nameForPython;
    if (!pythonPath.exists())
        return {};
    QString name = nameForPython.value(pythonPath);
    if (name.isEmpty()) {
        SynchronousProcess pythonProcess;
        pythonProcess.setTimeoutS(2);
        const CommandLine pythonVersionCommand(pythonPath, {"--version"});
        const SynchronousProcessResponse response = pythonProcess.runBlocking(pythonVersionCommand);
        if (response.result != SynchronousProcessResponse::Finished)
            return {};
        name = response.allOutput().trimmed();
        nameForPython[pythonPath] = name;
    }
    return name;
}

FilePath getPylsModulePath(CommandLine pylsCommand)
{
    static QMap<FilePath, FilePath> cache;
    const FilePath &modulePath = cache.value(pylsCommand.executable());
    if (!modulePath.isEmpty())
        return modulePath;

    pylsCommand.addArg("-h");
    SynchronousProcess pythonProcess;
    pythonProcess.setEnvironment(pythonProcess.environment() + QStringList("PYTHONVERBOSE=x"));
    SynchronousProcessResponse response = pythonProcess.runBlocking(pylsCommand);

    static const QString pylsInitPattern = "(.*)"
                                           + QRegularExpression::escape(
                                               QDir::toNativeSeparators("/pyls/__init__.py"))
                                           + '$';
    static const QRegularExpression regexCached(" matches " + pylsInitPattern,
                                                QRegularExpression::MultilineOption);
    static const QRegularExpression regexNotCached(" code object from " + pylsInitPattern,
                                                   QRegularExpression::MultilineOption);

    const QString &output = response.allOutput();
    for (const auto &regex : {regexCached, regexNotCached}) {
        const QRegularExpressionMatch result = regex.match(output);
        if (result.hasMatch()) {
            const FilePath &modulePath = FilePath::fromUserInput(result.captured(1));
            cache[pylsCommand.executable()] = modulePath;
            return modulePath;
        }
    }
    return {};
}

QList<const StdIOSettings *> configuredPythonLanguageServer()
{
    using namespace LanguageClient;
    QList<const StdIOSettings *> result;
    for (const BaseSettings *setting : LanguageClientManager::currentSettings()) {
        if (setting->m_languageFilter.isSupported(FilePath::fromString("foo.py"),
                                                  Constants::C_PY_MIMETYPE)) {
            result << dynamic_cast<const StdIOSettings *>(setting);
        }
    }
    return result;
}

static PythonLanguageServerState checkPythonLanguageServer(const FilePath &python)
{
    using namespace LanguageClient;
    SynchronousProcess pythonProcess;
    const CommandLine pythonLShelpCommand(python, {"-m", "pyls", "-h"});
    const FilePath &modulePath = getPylsModulePath(pythonLShelpCommand);
    for (const StdIOSettings *serverSetting : configuredPythonLanguageServer()) {
        if (modulePath == getPylsModulePath(serverSetting->command())) {
            return {serverSetting->m_enabled ? PythonLanguageServerState::AlreadyConfigured
                                             : PythonLanguageServerState::ConfiguredButDisabled,
                    FilePath()};
        }
    }

    SynchronousProcessResponse response = pythonProcess.runBlocking(pythonLShelpCommand);
    if (response.allOutput().contains("Python Language Server"))
        return {PythonLanguageServerState::AlreadyInstalled, modulePath};

    const CommandLine pythonPipVersionCommand(python, {"-m", "pip", "-V"});
    response = pythonProcess.runBlocking(pythonPipVersionCommand);
    if (response.allOutput().startsWith("pip "))
        return {PythonLanguageServerState::CanBeInstalled, FilePath()};
    else
        return {PythonLanguageServerState::CanNotBeInstalled, FilePath()};
}

static FilePath detectPython(const FilePath &documentPath)
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
        python = PythonSettings::interpreters().first().command;

    return python;
}

PyLSConfigureAssistant *PyLSConfigureAssistant::instance()
{
    static auto *instance = new PyLSConfigureAssistant(PythonPlugin::instance());
    return instance;
}

const StdIOSettings *PyLSConfigureAssistant::languageServerForPython(const FilePath &python)
{
    return findOrDefault(configuredPythonLanguageServer(),
                         [pythonModulePath = getPylsModulePath(
                              CommandLine(python, {"-m", "pyls"}))](const StdIOSettings *setting) {
                             return getPylsModulePath(setting->command()) == pythonModulePath;
                         });
}

static Client *registerLanguageServer(const FilePath &python)
{
    auto *settings = new StdIOSettings();
    settings->m_executable = python.toString();
    settings->m_arguments = "-m pyls";
    settings->m_name = PyLSConfigureAssistant::tr("Python Language Server (%1)")
                           .arg(pythonName(python));
    settings->m_languageFilter.mimeTypes = QStringList(Constants::C_PY_MIMETYPE);
    LanguageClientManager::registerClientSettings(settings);
    Client *client = LanguageClientManager::clientForSetting(settings).value(0);
    PyLSConfigureAssistant::updateEditorInfoBars(python, client);
    return client;
}

class PythonLSInstallHelper : public QObject
{
    Q_OBJECT
public:
    PythonLSInstallHelper(const FilePath &python, QPointer<TextEditor::TextDocument> document)
        : m_python(python)
        , m_document(document)
    {
        m_watcher.setFuture(m_future.future());
    }

    void run()
    {
        Core::ProgressManager::addTask(m_future.future(), "Install PyLS", installPylsTaskId);
        connect(&m_process,
                QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this,
                &PythonLSInstallHelper::installFinished);
        connect(&m_process,
                &QProcess::readyReadStandardError,
                this,
                &PythonLSInstallHelper::errorAvailable);
        connect(&m_process,
                &QProcess::readyReadStandardOutput,
                this,
                &PythonLSInstallHelper::outputAvailable);

        connect(&m_killTimer, &QTimer::timeout, this, &PythonLSInstallHelper::cancel);
        connect(&m_watcher, &QFutureWatcher<void>::canceled, this, &PythonLSInstallHelper::cancel);

        QStringList arguments = {"-m", "pip", "install", "python-language-server[all]"};

        // add --user to global pythons, but skip it for venv pythons
        if (!QDir(m_python.parentDir().toString()).exists("activate"))
            arguments << "--user";

        m_process.start(m_python.toString(), arguments);

        Core::MessageManager::write(tr("Running \"%1 %2\" to install Python language server")
                                        .arg(m_process.program(), m_process.arguments().join(' ')));

        m_killTimer.setSingleShot(true);
        m_killTimer.start(5 /*minutes*/ * 60 * 1000);
    }

private:
    void cancel()
    {
        SynchronousProcess::stopProcess(m_process);
        Core::MessageManager::write(tr("The Python language server installation was canceled by %1.")
                                        .arg(m_killTimer.isActive() ? tr("user") : tr("time out")));
    }

    void installFinished(int exitCode, QProcess::ExitStatus exitStatus)
    {
        m_future.reportFinished();
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            if (Client *client = registerLanguageServer(m_python))
                LanguageClientManager::openDocumentWithClient(m_document, client);
        } else {
            Core::MessageManager::write(
                tr("Installing the Python language server failed with exit code %1").arg(exitCode));
        }
        deleteLater();
    }

    void outputAvailable()
    {
        const QString &stdOut = QString::fromLocal8Bit(m_process.readAllStandardOutput().trimmed());
        if (!stdOut.isEmpty())
            Core::MessageManager::write(stdOut);
    }

    void errorAvailable()
    {
        const QString &stdErr = QString::fromLocal8Bit(m_process.readAllStandardError().trimmed());
        if (!stdErr.isEmpty())
            Core::MessageManager::write(stdErr);
    }

    QFutureInterface<void> m_future;
    QFutureWatcher<void> m_watcher;
    QProcess m_process;
    QTimer m_killTimer;
    const FilePath m_python;
    QPointer<TextEditor::TextDocument> m_document;
};

void PyLSConfigureAssistant::installPythonLanguageServer(const FilePath &python,
                                                         QPointer<TextEditor::TextDocument> document)
{
    document->infoBar()->removeInfo(installPylsInfoBarId);

    // Hide all install info bar entries for this python, but keep them in the list
    // so the language server will be setup properly after the installation is done.
    for (TextEditor::TextDocument *additionalDocument : m_infoBarEntries[python])
        additionalDocument->infoBar()->removeInfo(installPylsInfoBarId);

    auto install = new PythonLSInstallHelper(python, document);
    install->run();
}

static void setupPythonLanguageServer(const FilePath &python,
                                      QPointer<TextEditor::TextDocument> document)
{
    document->infoBar()->removeInfo(startPylsInfoBarId);
    if (Client *client = registerLanguageServer(python))
        LanguageClientManager::openDocumentWithClient(document, client);
}

static void enablePythonLanguageServer(const FilePath &python,
                                       QPointer<TextEditor::TextDocument> document)
{
    document->infoBar()->removeInfo(enablePylsInfoBarId);
    if (const StdIOSettings *setting = PyLSConfigureAssistant::languageServerForPython(python)) {
        LanguageClientManager::enableClientSettings(setting->m_id);
        if (const StdIOSettings *setting = PyLSConfigureAssistant::languageServerForPython(python)) {
            if (Client *client = LanguageClientManager::clientForSetting(setting).value(0)) {
                LanguageClientManager::openDocumentWithClient(document, client);
                PyLSConfigureAssistant::updateEditorInfoBars(python, client);
            }
        }
    }
}

void PyLSConfigureAssistant::documentOpened(Core::IDocument *document)
{
    auto textDocument = qobject_cast<TextEditor::TextDocument *>(document);
    if (!textDocument || textDocument->mimeType() != Constants::C_PY_MIMETYPE)
        return;

    const FilePath &python = detectPython(textDocument->filePath());
    if (!python.exists())
        return;

    instance()->openDocumentWithPython(python, textDocument);
}

void PyLSConfigureAssistant::openDocumentWithPython(const FilePath &python,
                                                    TextEditor::TextDocument *document)
{
    using CheckPylsWatcher = QFutureWatcher<PythonLanguageServerState>;

    QPointer<CheckPylsWatcher> watcher = new CheckPylsWatcher();
    watcher->setFuture(Utils::runAsync(&checkPythonLanguageServer, python));

    // cancel and delete watcher after a 10 second timeout
    QTimer::singleShot(10000, this, [watcher]() {
        if (watcher) {
            watcher->cancel();
            watcher->deleteLater();
        }
    });

    connect(
        watcher,
        &CheckPylsWatcher::resultReadyAt,
        this,
        [=, document = QPointer<TextEditor::TextDocument>(document)]() {
            if (!document || !watcher)
                return;
            handlePyLSState(python, watcher->result(), document);
            watcher->deleteLater();
        });
}

void PyLSConfigureAssistant::handlePyLSState(const FilePath &python,
                                             const PythonLanguageServerState &state,
                                             TextEditor::TextDocument *document)
{
    if (state.state == PythonLanguageServerState::CanNotBeInstalled)
        return;
    if (state.state == PythonLanguageServerState::AlreadyConfigured) {
        if (const StdIOSettings *setting = languageServerForPython(python)) {
            if (Client *client = LanguageClientManager::clientForSetting(setting).value(0))
                LanguageClientManager::openDocumentWithClient(document, client);
        }
        return;
    }

    resetEditorInfoBar(document);
    Utils::InfoBar *infoBar = document->infoBar();
    if (state.state == PythonLanguageServerState::CanBeInstalled
        && infoBar->canInfoBeAdded(installPylsInfoBarId)) {
        auto message = tr("Install and set up Python language server (PyLS) for %1 (%2). "
                          "The language server provides Python specific completion and annotation.")
                           .arg(pythonName(python), python.toUserOutput());
        Utils::InfoBarEntry info(installPylsInfoBarId,
                                 message,
                                 Utils::InfoBarEntry::GlobalSuppression::Enabled);
        info.setCustomButtonInfo(tr("Install"),
                                 [=]() { installPythonLanguageServer(python, document); });
        infoBar->addInfo(info);
        m_infoBarEntries[python] << document;
    } else if (state.state == PythonLanguageServerState::AlreadyInstalled
               && infoBar->canInfoBeAdded(startPylsInfoBarId)) {
        auto message = tr("Found a Python language server for %1 (%2). "
                          "Set it up for this document?")
                           .arg(pythonName(python), python.toUserOutput());
        Utils::InfoBarEntry info(startPylsInfoBarId,
                                 message,
                                 Utils::InfoBarEntry::GlobalSuppression::Enabled);
        info.setCustomButtonInfo(tr("Set Up"),
                                 [=]() { setupPythonLanguageServer(python, document); });
        infoBar->addInfo(info);
        m_infoBarEntries[python] << document;
    } else if (state.state == PythonLanguageServerState::ConfiguredButDisabled
               && infoBar->canInfoBeAdded(enablePylsInfoBarId)) {
        auto message = tr("Enable Python language server for %1 (%2)?")
                           .arg(pythonName(python), python.toUserOutput());
        Utils::InfoBarEntry info(enablePylsInfoBarId,
                                 message,
                                 Utils::InfoBarEntry::GlobalSuppression::Enabled);
        info.setCustomButtonInfo(tr("Enable"),
                                 [=]() { enablePythonLanguageServer(python, document); });
        infoBar->addInfo(info);
        m_infoBarEntries[python] << document;
    }
}

void PyLSConfigureAssistant::updateEditorInfoBars(const FilePath &python, Client *client)
{
    for (TextEditor::TextDocument *document : instance()->m_infoBarEntries.take(python)) {
        instance()->resetEditorInfoBar(document);
        if (client)
            LanguageClientManager::openDocumentWithClient(document, client);
    }
}

void PyLSConfigureAssistant::resetEditorInfoBar(TextEditor::TextDocument *document)
{
    for (QList<TextEditor::TextDocument *> &documents : m_infoBarEntries)
        documents.removeAll(document);
    Utils::InfoBar *infoBar = document->infoBar();
    infoBar->removeInfo(installPylsInfoBarId);
    infoBar->removeInfo(startPylsInfoBarId);
    infoBar->removeInfo(enablePylsInfoBarId);
}

PyLSConfigureAssistant::PyLSConfigureAssistant(QObject *parent)
    : QObject(parent)
{
    Core::EditorManager::instance();

    connect(Core::EditorManager::instance(),
            &Core::EditorManager::documentClosed,
            this,
            [this](Core::IDocument *document) {
                if (auto textDocument = qobject_cast<TextEditor::TextDocument *>(document))
                    resetEditorInfoBar(textDocument);
            });
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
                            ? QString("import %1").arg(pythonFile.toFileInfo().completeBaseName())
                            : QString("from %1 import *")
                                  .arg(pythonFile.toFileInfo().completeBaseName());
    return {"-c", QString("%1; print('Running \"%1\"')").arg(import)};
}

void openPythonRepl(const FilePath &file, ReplType type)
{
    static const auto workingDir = [](const FilePath &file) {
        if (file.isEmpty()) {
            if (ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject())
                return project->projectDirectory().toFileInfo().filePath();
            return QDir::currentPath();
        }
        return file.toFileInfo().path();
    };

    const auto args = QStringList{"-i"} + replImportArgs(file, type);
    auto process = new ConsoleProcess;
    const FilePath pythonCommand = detectPython(file);
    process->setCommand({pythonCommand, args});
    process->setWorkingDirectory(workingDir(file));
    const QString commandLine = process->command().toUserOutput();
    QObject::connect(process,
                     &ConsoleProcess::processError,
                     process,
                     [process, commandLine](const QString &errorString) {
                         Core::MessageManager::write(
                             QCoreApplication::translate("Python",
                                                         "Failed to run Python (%1): \"%2\".")
                                 .arg(commandLine, errorString));
                         process->deleteLater();
                     });
    QObject::connect(process, &ConsoleProcess::stubStopped, process, &QObject::deleteLater);
    process->start();
}

} // namespace Internal
} // namespace Python

#include "pythonutils.moc"
