// Copyright (C) 2025 David M. Cotter
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "coreplugin/dialogs/ioptionspage.h"
#include "coreplugin/editormanager/editormanager.h"
#include "mcpcommands.h"
#include "mcpserverconstants.h"
#include "mcpservertr.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/mcpmanager.h>
#include <coreplugin/messagemanager.h>

#include <debugger/debuggerconstants.h>

#include <extensionsystem/iplugin.h>

#include <mcp/server/mcpserver.h>

#include <texteditor/textdocument.h>

#include <utils/algorithm.h>
#include <utils/aspects.h>
#include <utils/async.h>
#include <utils/icon.h>
#include <utils/layoutbuilder.h>
#include <utils/mimeutils.h>
#include <utils/stylehelper.h>

#include <projectexplorer/devicesupport/devicemanager.h>

#include <QLabel>
#include <QPushButton>
#include <QTcpServer>

#include <QtTaskTree/qtasktreerunner.h>

using namespace Core;
using namespace QtTaskTree;
using namespace Utils;

// Define logging category for the plugin
Q_LOGGING_CATEGORY(mcpPlugin, "qtc.mcpserver.plugin", QtWarningMsg)

namespace Mcp::Internal {

class McpServerPlugin;

class ListenAddressAspect : public AspectContainer
{
public:
    ListenAddressAspect(AspectContainer *parent)
        : AspectContainer(parent)
    {
        addressType.setSettingsKey("AddressType");
        addressType.addOption("localhost", Tr::tr("Localhost (127.0.0.1)"));
        addressType.addOption("any", Tr::tr("Any (0.0.0.0)"));
        addressType.addOption("custom", Tr::tr("Custom"));
        addressType.setDefaultValue("localhost");
        addressType.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);

        customAddress.setSettingsKey("CustomAddress");
        customAddress.setEnabled(false);
        customAddress.setDefaultValue("127.0.0.1");
        customAddress.setDisplayStyle(StringAspect::DisplayStyle::LineEditDisplay);
        customAddress.setValidationFunction([](const QString &s) -> Result<> {
            QHostAddress addr;
            if (addr.setAddress(s))
                return ResultOk;
            else
                return ResultError(Tr::tr("Invalid IP address"));
        });

        connect(&addressType, &SelectionAspect::volatileValueChanged, this, [this]() {
            customAddress.setEnabled(addressType.volatileValue() == 2);
        });

        connect(this, &BaseAspect::enabledChanged, this, [this]() {
            addressType.setEnabled(this->isEnabled());
            if (!this->isEnabled())
                customAddress.setEnabled(false);
            else
                customAddress.setEnabled(addressType.volatileValue() == 2);
        });

        setLayouter([this]() {
            using namespace Layouting;
            return Row{noMargin, addressType, customAddress};
        });
    }

    QHostAddress hostAddress() const
    {
        if (addressType.stringValue() == "localhost")
            return QHostAddress::LocalHost;
        else if (addressType.stringValue() == "any")
            return QHostAddress::Any;
        else
            return QHostAddress(customAddress.value());
    }

    void readSettings() override
    {
        AspectContainer::readSettings();
        customAddress.setEnabled(addressType.value() == 2);
    }

    SelectionAspect addressType{this};
    StringAspect customAddress{this};
};

class McpServerPluginSettings : public AspectContainer
{
public:
    McpServerPluginSettings(McpServerPlugin *plugin);

    BoolAspect enabled{this};
    ListenAddressAspect listenAddress{this};
    IntegerAspect port{this};
    BoolAspect enableCors{this};
};

class McpServerSettingsPage : public Core::IOptionsPage
{
public:
    McpServerSettingsPage(McpServerPluginSettings *settings)
    {
        setId(Constants::SETTINGS_PAGE_ID);
        setDisplayName("Qt Creator Mcp Server");
        setCategory(Core::Constants::SETTINGS_CATEGORY_AI);
        setSettingsProvider([settings] { return settings; });
    }
};

void setupResources(QObject *guard, Mcp::Server &server, QParallelTaskTreeRunner *runner);

static const char QT_MCPSERVER_MANAGER_ID[] = "QTCREATOR.BUILTIN.MCP.SERVER";

class McpServerPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "mcpserver.json")

public:
    ~McpServerPlugin() final {}

    void initialize() final
    {
        settings.readSettings();
        restartServer();
    }

    void extensionsInitialized() final
    {
        // Initialize the server
        McpCommands::registerCommands(m_server);
        setupResources(this, m_server, &m_taskRunner);
    }

    ShutdownFlag aboutToShutdown() final
    {
        qDeleteAll(m_server.boundTcpServers());
        return SynchronousShutdown;
    }

    void restartServer()
    {
        qCDebug(mcpPlugin) << "(Re)-starting Mcp server...";
        qDeleteAll(m_server.boundTcpServers());

        m_server.setCorsEnabled(settings.enableCors());

        McpManager::instance().removeMcpServer(QT_MCPSERVER_MANAGER_ID);

        if (!settings.enabled()) {
            qCInfo(mcpPlugin) << "Mcp server is disabled in settings, not starting.";
            return;
        }

        QTcpServer *tcpServer = new QTcpServer(this);
        if (!tcpServer->listen(settings.listenAddress.hostAddress(), settings.port())
            || !m_server.bind(tcpServer)) {
            delete tcpServer;
            MessageManager::writeFlashing(
                Tr::tr("Failed to start Mcp server on \"%1:%2\".")
                    .arg(settings.listenAddress.hostAddress().toString())
                    .arg(settings.port()));
        } else {
            qCInfo(mcpPlugin) << "Mcp server started successfully on" << tcpServer->serverAddress()
                              << ":" << tcpServer->serverPort();
            // Show startup message in General Messages panel
            MessageManager::writeSilently(
                Tr::tr("The Mcp server is listening on \"%1:%2\".")
                    .arg(tcpServer->serverAddress().toString())
                    .arg(tcpServer->serverPort()));

            const auto url = QUrl(QString("http://%1:%2")
                                      .arg(tcpServer->serverAddress().toString())
                                      .arg(tcpServer->serverPort()));

            const auto serverInfo = McpManager::ServerInfo{
                QT_MCPSERVER_MANAGER_ID,
                "Qt Creator builtin",
                McpManager::ConnectionType::Streamable_Http,
                url,
                {}};

            QTC_CHECK(McpManager::instance().registerMcpServer(serverInfo));
        }
    }

    bool isServerRunning() const { return !m_server.boundTcpServers().isEmpty(); }

private:
    QParallelTaskTreeRunner m_taskRunner;

    Mcp::Server m_server{
        Mcp::Schema::Implementation()
            .name("qt-creator-mcp-server")
            .title("Qt Creator Mcp Server")
            .description(
                "Mcp server for Qt Creator to allow external tools to interact with the IDE.")
            .version(QCoreApplication::applicationVersion())};

    McpServerPluginSettings settings{this};
    McpServerSettingsPage settingsPage{&settings};
};

McpServerPluginSettings::McpServerPluginSettings(McpServerPlugin *plugin)
{
    setAutoApply(false);

    setSettingsGroup("McpServer");

    enabled.setLabel(Tr::tr("Enable Mcp Server"));
    enabled.setDefaultValue(true);

    listenAddress.setSettingsGroup("Listen");
    listenAddress.setEnabler(&enabled);
    listenAddress.setToolTip(
        Tr::tr("The address the Mcp Server should listen on for incoming connections"));

    port.setSettingsKey("Port");
    port.setLabel(Tr::tr("Port:"));
    port.setDefaultValue(8249);
    port.setRange(1, 65535);
    port.setToolTip(Tr::tr("The port for the Mcp Server to listen on."));
    port.setEnabler(&enabled);

    enableCors.setSettingsKey("EnableCors");
    enableCors.setEnabler(&enabled);
    enableCors.setLabel(Tr::tr("CORS:"));
    enableCors.setToolTip(
        Tr::tr(
            "Enable Cross-Origin Resource Sharing (CORS) for the Mcp Server. "
            "This is necessary if you want to connect to the server from a web application."));
    enableCors.setDefaultValue(false);

    connect(this, &AspectContainer::applied, plugin, [plugin]() { plugin->restartServer(); });

    setLayouter([this, plugin]() {
        using namespace Layouting;
        auto statusLabel = new QLabel();
        auto statusIcon = new QLabel();

        statusIcon->setFixedSize(24, 24);
        statusIcon->setAlignment(Qt::AlignCenter);

        const auto updateStatus = [plugin, statusIcon, statusLabel]() {
            const bool isRunning = plugin->isServerRunning();
            auto p = statusLabel->palette();

            p.setColor(
                QPalette::WindowText,
                creatorColor(isRunning ? Theme::TextColorNormal : Theme::TextColorError));
            statusLabel->setPalette(p);
            statusIcon->setPalette(p);
            statusIcon->setText(isRunning ? "✓" : "✗");

            if (isRunning)
                statusLabel->setText(Tr::tr("The Mcp Server is running"));
            else
                statusLabel->setText(Tr::tr("The Mcp Server is not running"));
        };

        updateStatus();
        connect(this, &AspectContainer::applied, this, [updateStatus]() { updateStatus(); });

        // clang-format off
        return Form {
            enabled, br,
            Tr::tr("Listen on:"), listenAddress, br,
            port, br,
            statusIcon, statusLabel, br,
        };
        // clang-format on
    });
};

static Result<Mcp::Schema::ReadResourceResult> getOpenEditorContent(
    const Mcp::Schema::ReadResourceRequestParams &params)
{
    FilePath path = FilePath::fromUserInput(params.uri());
    const FilePath docPath = path.scheme() == u"file" ? FilePath::fromUserInput(path.path()) : path;

    // Find editor by URI
    const auto editors = EditorManager::instance()->visibleEditors();
    auto it = std::find_if(editors.begin(), editors.end(), [&docPath](Core::IEditor *e) {
        return e->document()->filePath() == docPath;
    });

    if (it == editors.end())
        return ResultError("Editor not found for URI: " + path.toUserOutput());

    auto textDoc = qobject_cast<TextEditor::TextDocument *>((*it)->document());

    if (!textDoc)
        return ResultError("Document is not a text document");

    return Mcp::Schema::ReadResourceResult().addContent(
        Mcp::Schema::TextResourceContents()
            .text(textDoc->plainText())
            .uri(params.uri())
            .mimeType(textDoc->mimeType()));
}

void setupResources(QObject *guard, Mcp::Server &server, QParallelTaskTreeRunner *runner)
{
    EditorManager *editorManager = EditorManager::instance();
    QObject::connect(
        editorManager, &EditorManager::editorOpened, guard, [&server](Core::IEditor *editor) {
            const FilePath path = editor->document()->filePath();
            const QString mimeType = editor->document()->mimeType();
            const QString strPath = path.isLocal() ? "file://" + path.toUserOutput()
                                                   : path.toUserOutput();
            server.addResource(
                Mcp::Schema::Resource().uri(strPath).mimeType(mimeType).name(path.fileName()),
                getOpenEditorContent);
        });
    QObject::connect(
        editorManager, &EditorManager::editorAboutToClose, guard, [&server](Core::IEditor *editor) {
            const FilePath path = editor->document()->filePath();
            const QString strPath = path.isLocal() ? "file://" + path.toUserOutput()
                                                   : path.toUserOutput();
            server.removeResource(strPath);
        });

    server.setResourceFallbackCallback(
        [](Mcp::Schema::ReadResourceRequestParams params) -> Result<Mcp::Schema::ReadResourceResult> {
            const FilePath filePath = FilePath::fromUserInput(params.uri());
            if (!filePath.exists())
                return ResultError("File does not exist: " + filePath.toUserOutput());

            if (filePath.isDir()) {
                FilePaths entries = filePath.dirEntries(QDir::AllEntries | QDir::NoDotAndDotDot);
                Mcp::Schema::ReadResourceResult result;
                QStringList data = transform(entries, [](const FilePath &entry) {
                    return entry.fileName();
                });
                result.addContent(
                    Mcp::Schema::TextResourceContents()
                        .text(data.join('\n'))
                        .uri(filePath.toUserOutput())
                        .mimeType("inode/directory"));

                return result;
            }

            if (auto doc = TextEditor::TextDocument::textDocumentForFilePath(filePath)) {
                return Mcp::Schema::ReadResourceResult().addContent(
                    Mcp::Schema::TextResourceContents()
                        .text(doc->plainText())
                        .uri(params.uri())
                        .mimeType(doc->mimeType()));
            }
            Result<QByteArray> contents = filePath.fileContents();
            if (!contents)
                return ResultError(contents.error());

            return Mcp::Schema::ReadResourceResult().addContent(
                Mcp::Schema::TextResourceContents()
                    .text(Core::EditorManager::defaultTextEncoding().decode(*contents))
                    .uri(params.uri())
                    .mimeType(mimeTypeForFile(filePath).name()));
        });

    server.addResourceTemplate(
        Mcp::Schema::ResourceTemplate()
            .uriTemplate("file://{path}")
            .name("Local files")
            .description("Access local files by URI, e.g. file:///path/to/file.txt"));

    const auto addTemplate = [&server](const FilePath &root, const QString &name) {
        server.addResourceTemplate(
            Mcp::Schema::ResourceTemplate()
                .uriTemplate(root.toUserOutput() + "{path}")
                .name(name)
                .description(QString("Access files on device %1 by URI, e.g. %2/path/to/file.txt")
                                 .arg(name)
                                 .arg(root.toUserOutput())));
    };

    for (int i = 0; i < ProjectExplorer::DeviceManager::deviceCount(); ++i) {
        const auto device = ProjectExplorer::DeviceManager::deviceAt(i);
        const FilePath root = device->rootPath();
        if (root.isLocal())
            continue; // Local files are already handled by the "file://" resource template

        addTemplate(root, device->displayName());
    }

    QObject::connect(
        ProjectExplorer::DeviceManager::instance(),
        &ProjectExplorer::DeviceManager::deviceAdded,
        guard,
        [addTemplate](Id id) {
            const auto device = ProjectExplorer::DeviceManager::find(id);
            if (!device)
                return;
            const FilePath root = device->rootPath();
            if (root.isLocal())
                return; // Local files are already handled by the "file://" resource template

            addTemplate(root, device->displayName());
        });
    QObject::connect(
        ProjectExplorer::DeviceManager::instance(),
        &ProjectExplorer::DeviceManager::deviceRemoved,
        guard,
        [&server](Id id) {
            const auto device = ProjectExplorer::DeviceManager::find(id);
            if (!device)
                return;
            const FilePath root = device->rootPath();
            if (root.isLocal())
                return; // Local files are already handled by the "file://" resource template

            server.removeResourceTemplate(root.toUserOutput());
        });

    server.setCompletionCallback([runner](
                                     const Mcp::Schema::CompleteRequestParams &params,
                                     const Mcp::Server::CompletionResultCallback &cb) {
        if (std::holds_alternative<Mcp::Schema::ResourceTemplateReference>(params.ref())) {
            auto resourceTemplateRef = std::get<Mcp::Schema::ResourceTemplateReference>(
                params.ref());

            qCDebug(mcpPlugin) << "Received completion request for resource template:"
                               << resourceTemplateRef.uri();

            QString actual = resourceTemplateRef.uri();
            actual.replace("{" + params.argument().name() + "}", params.argument().value());
            QString root = resourceTemplateRef.uri();
            root.replace("{" + params.argument().name() + "}", "");
            FilePath anchorDir = FilePath::fromUserInput(root);

            qCDebug(mcpPlugin) << "Will search in:" << actual;

            const auto onSetup = [actual](Async<FilePaths> &task) {
                task.setConcurrentCallData(
                    [](const FilePath &base) -> FilePaths {
                        FilePath searchDir = base;
                        QString start = "";
                        if (!searchDir.exists()) {
                            start = searchDir.fileName();
                            searchDir = searchDir.parentDir();
                        }
                        qCDebug(mcpPlugin) << "Searching in:" << searchDir;
                        if (!searchDir.exists()) {
                            qCWarning(mcpPlugin) << "Search directory does not exist:" << searchDir;
                            return {};
                        }
                        auto patterns = start.isEmpty() ? QStringList{} : QStringList{start + "*"};
                        return searchDir.dirEntries(
                            {patterns, QDir::AllEntries | QDir::NoDotAndDotDot});
                    },
                    FilePath::fromUserInput(actual));
            };

            const auto onDone = [cb, anchorDir](const Async<FilePaths> &task) {
                const FilePaths paths = task.result().mid(0, 100); // Limit to first 100 results
                qCDebug(mcpPlugin) << "Completion search found" << paths.size() << "results";
                Mcp::Schema::CompleteResult::Completion completions;
                for (const FilePath &path : paths) {
                    completions.addValue(path.relativePathFromDir(anchorDir));
                }
                cb(Mcp::Schema::CompleteResult().completion(completions));
            };

            runner->start(Group{AsyncTask<FilePaths>(onSetup, onDone)});
            return;
        }

        cb(Mcp::Schema::CompleteResult());
    });
}

} // namespace Mcp::Internal

#include "mcpserverplugin.moc"
