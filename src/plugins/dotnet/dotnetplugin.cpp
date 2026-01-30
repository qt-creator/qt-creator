// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dotnetproject.h"
#include "dotnettr.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/processprogress.h>
#include <extensionsystem/iplugin.h>
#include <languageclient/languageclientmanager.h>
#include <utils/environment.h>
#include <utils/globaltasktree.h>
#include <utils/infobar.h>
#include <utils/qtcprocess.h>

using namespace Core;
using namespace LanguageClient;
using namespace QtTaskTree;
using namespace Utils;

namespace Dotnet::Internal {

class DotnetPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Dotnet.json")

public:
    DotnetPlugin() = default;
    ~DotnetPlugin() final = default;

    void initialize() final;

    void editorOpened(Core::IEditor *editor);
};

void DotnetPlugin::initialize()
{
    setupDotnetProject();
    connect(EditorManager::instance(), &EditorManager::editorOpened,
            this, &DotnetPlugin::editorOpened);

}

static QStringList csharpMimeTypes()
{
    static QStringList mimeTypes = {
        "application/x-csh",
        "text/x-csharp"
    };
    return mimeTypes;
}

constexpr char setupCSharpLsInfoBarId[] = "LanguageClient::setupCSharpLs";

static void setupCSharpLanguageServer(IDocument *document)
{

    InfoBar *infoBar = document->infoBar();
    if (!infoBar->canInfoBeAdded(setupCSharpLsInfoBarId))
        return;

    // check if it is already configured
    const QList<BaseSettings *> settings = LanguageClientManager::currentSettings();
    for (BaseSettings *setting : settings) {
        if (setting->isValid() && setting->m_languageFilter.isSupported(document))
            return;
    }

    FilePath dotnet;
    bool install = false;

    const FilePath csharpLs = Environment::systemEnvironment().searchInPath("csharp-ls");
    if (!csharpLs.isExecutableFile()) {
        // check for dotnet
        dotnet = Environment::systemEnvironment().searchInPath("dotnet");
        if (!dotnet.isExecutableFile())
            return;
        install = true;
    }

    const QString message
        = install ? Tr::tr("Install C# language server via dotnet.")
                  : Tr::tr("Set up C# language server (%1).").arg(csharpLs.toUserOutput());
    InfoBarEntry info(setupCSharpLsInfoBarId, message, InfoBarEntry::GlobalSuppression::Enabled);
    info.addCustomButton(install ? Tr::tr("Install") : Tr::tr("Set Up"), [=]() {
        const QList<IDocument *> &openedDocuments = DocumentModel::openedDocuments();
        for (IDocument *doc : openedDocuments)
            doc->infoBar()->removeInfo(setupCSharpLsInfoBarId);

        auto setupStdIOSettings = [=](const FilePath &executable) {
            auto settings = new StdIOSettings();

            settings->executable.setValue(executable);
            settings->name.setValue(Tr::tr("C# Language Server"));
            settings->m_languageFilter.mimeTypes = csharpMimeTypes();
            settings->m_startBehavior = BaseSettings::RequiresProject;
            LanguageClientSettings::addSettings(settings);
            LanguageClientManager::applySettings();
        };

        const QString languageServer = "csharp-ls";
        if (install) {
            const FilePath lsPath = ICore::userResourcePath(languageServer);
            if (!lsPath.ensureWritableDir())
                return;

            const auto onInstallSetup = [dotnet, lsPath, languageServer](Process &process) {
                CommandLine command(dotnet);
                command.addArgs(
                    {"tool", "install", "--tool-path", lsPath.toUserOutput(), languageServer});
                process.setCommand(command);
                process.setWorkingDirectory(lsPath);
                process.setTerminalMode(TerminalMode::Run);
                auto progress = new ProcessProgress(&process);
                progress->setDisplayName(Tr::tr("Install C# Language Server"));
                MessageManager::writeSilently(Tr::tr("Running \"%1\" to install %2.")
                                                  .arg(process.commandLine().toUserOutput(), languageServer));
            };
            const auto onInstallDone = [languageServer, lsPath, setupStdIOSettings](
                                           const Process &process) {
                if (process.result() == ProcessResult::FinishedWithSuccess) {
                    const FilePath lsExecutable = lsPath.resolvePath(languageServer).withExecutableSuffix();
                    if (lsExecutable.isExecutableFile()) {
                        setupStdIOSettings(lsExecutable);
                        return DoneResult::Success;
                    }
                    MessageManager::writeFlashing(
                        Tr::tr("Installing \"%1\" failed. Expected language server (%2) is either "
                               "not found or not executable.")
                            .arg(languageServer)
                            .arg(lsExecutable.toUserOutput()));
                } else {
                    MessageManager::writeFlashing(
                        Tr::tr("Installing \"%1\" failed with exit code %2.")
                            .arg(languageServer)
                            .arg(process.exitCode()));
                }
                return DoneResult::Error;
            };
            const auto onInstallTimeout = [languageServer] {
                MessageManager::writeFlashing(
                    Tr::tr("The installation of \"%1\" was canceled by timeout.").arg(languageServer));
            };

            using namespace std::literals::chrono_literals;
            const Group recipe {
                ProcessTask(onInstallSetup, onInstallDone, CallDone::Always)
                .withTimeout(5min, onInstallTimeout)
            };
            GlobalTaskTree::start(recipe);
        } else {
            setupStdIOSettings(csharpLs);
        }
    });
    infoBar->addInfo(info);
}


void DotnetPlugin::editorOpened(IEditor *editor)
{
    if (!editor)
        return;

    IDocument *doc = editor->document();
    if (!doc)
        return;

    const QString mimeType = doc->mimeType();
    if (!csharpMimeTypes().contains(mimeType))
        return;

    setupCSharpLanguageServer(doc);
}

} // namespace Dotnet::Internal

#include "dotnetplugin.moc"
