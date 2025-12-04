// Copyright (C) 2025 David M. Cotter
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpcommands.h"
#include "mcpserver.h"
#include "pluginconstants.h"
#include "plugintr.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <extensionsystem/iplugin.h>

#include <utils/icon.h>

#include <QAction>
#include <QDebug>
#include <QDialog>
#include <QFont>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLoggingCategory>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

#include "mcpservertest.h"

using namespace Core;

// Define logging category for the plugin
Q_LOGGING_CATEGORY(mcpPlugin, "qtc.mcpserver.plugin", QtWarningMsg)

namespace Mcp::Internal {

class McpServerStatusDialog : public QDialog
{
    Q_OBJECT

public:
    explicit McpServerStatusDialog(McpServer *server, QWidget *parent = nullptr)
        : QDialog(parent)
        , m_serverP(server)
    {
        setWindowTitle(QString("MCP Server Status"));
        setModal(true);
        resize(400, 200);

        QVBoxLayout *layout = new QVBoxLayout(this);

        // Title with icon
        QHBoxLayout *titleLayout = new QHBoxLayout();

        QLabel *iconLabel = new QLabel();
        iconLabel->setPixmap(icon().pixmap());
        titleLayout->addWidget(iconLabel);

        QLabel *titleLabel = new QLabel(QString("QtCreator MCP Server"));
        QFont titleFont = titleLabel->font();
        titleFont.setPointSize(14);
        titleFont.setBold(true);
        titleLabel->setFont(titleFont);
        titleLayout->addWidget(titleLabel);
        titleLayout->addStretch();

        layout->addLayout(titleLayout);

        // Status section
        QHBoxLayout *statusLayout = new QHBoxLayout();

        m_statusIcon = new QLabel();
        m_statusIcon->setFixedSize(24, 24);
        m_statusIcon->setAlignment(Qt::AlignCenter);

        m_statusLabel = new QLabel();
        QFont statusFont = m_statusLabel->font();
        statusFont.setPointSize(12);
        m_statusLabel->setFont(statusFont);

        statusLayout->addWidget(m_statusIcon);
        statusLayout->addWidget(m_statusLabel);
        statusLayout->addStretch();

        layout->addLayout(statusLayout);

        // Details
        m_detailsLabel = new QLabel();
        m_detailsLabel->setWordWrap(true);
        m_detailsLabel->setStyleSheet("QLabel { color: #666; }");
        layout->addWidget(m_detailsLabel);

        layout->addStretch();

        // Buttons
        QHBoxLayout *buttonLayout = new QHBoxLayout();
        buttonLayout->addStretch();

        m_restartButton = new QPushButton(Tr::tr("Restart Server"));
        m_restartButton->setEnabled(false);
        connect(m_restartButton, &QPushButton::clicked, this, &McpServerStatusDialog::restartServer);
        buttonLayout->addWidget(m_restartButton);

        QPushButton *closeButton = new QPushButton(Tr::tr("Close"));
        connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
        buttonLayout->addWidget(closeButton);

        layout->addLayout(buttonLayout);

        updateStatus();
    }

    static const Utils::Icon &icon()
    {
        static const Utils::Icon mcpIcon(
            {{":/mcpserver/images/mcpicon.png", Utils::Theme::PanelTextColorMid}},
            Utils::Icon::MenuTintedStyle);
        return mcpIcon;
    }

private slots:
    void restartServer()
    {
        if (m_serverP) {
            m_serverP->stop();
            if (m_serverP->start()) {
                updateStatus();
            }
        }
    }

private:
    void updateStatus()
    {
        bool isRunning = m_serverP && m_serverP->isRunning();

        if (isRunning) {
            // Green checkmark
            m_statusIcon->setText("✓");
            m_statusIcon->setStyleSheet(
                "QLabel { color: green; font-size: 18px; font-weight: bold; }");
            m_statusLabel->setText(Tr::tr("MCP Server is Running"));
            m_statusLabel->setStyleSheet("QLabel { color: green; }");

            // Get the actual port from the server
            quint16 port = m_serverP->getPort();
            QString portInfo
                = Tr::tr("Server is active on port %1 and accepting connections.").arg(port);
            m_detailsLabel->setText(portInfo);

            m_restartButton->setEnabled(false);
        } else {
            // Red X
            m_statusIcon->setText("✗");
            m_statusIcon->setStyleSheet(
                "QLabel { color: red; font-size: 18px; font-weight: bold; }");
            m_statusLabel->setText(Tr::tr("MCP Server is Not Running"));
            m_statusLabel->setStyleSheet("QLabel { color: red; }");

            m_detailsLabel->setText(
                Tr::tr(
                    "The MCP server is not active. Click 'Restart Server' to try starting it "
                    "again."));

            m_restartButton->setEnabled(true);
        }
    }

    McpServer *m_serverP;
    QLabel *m_statusIcon;
    QLabel *m_statusLabel;
    QLabel *m_detailsLabel;
    QPushButton *m_restartButton;
};

class McpServerPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "mcpserver.json")

public:
    McpServerPlugin() = default;

    ~McpServerPlugin() final
    {
        delete m_serverP;
        delete m_commandsP;
    }

    void initialize() final
    {
        qCDebug(mcpPlugin) << "Qt Creator MCP Server initializing...";
        qCDebug(mcpPlugin) << "Qt version:" << QT_VERSION_STR;
        qCDebug(mcpPlugin) << "Build type:" <<
#ifdef QT_NO_DEBUG
            "Release"
#else
            "Debug"
#endif
            ;

        // Create the MCP server and commands
        qCDebug(mcpPlugin) << "Creating MCP server and commands...";
        m_serverP = new McpServer(this);
        m_commandsP = new McpCommands(this);

        // Initialize the server
        qCDebug(mcpPlugin) << "Starting MCP server...";
        if (!m_serverP->start()) {
            qCCritical(mcpPlugin) << "Failed to start MCP server";
            QMessageBox::warning(
                ICore::dialogParent(), Tr::tr("MCP Server"), Tr::tr("Failed to start MCP server"));
        } else {
            qCInfo(mcpPlugin) << "MCP server started successfully on port" << m_serverP->getPort();
            // Show startup message in General Messages panel
            outputMessage(
                QString("MCP Server loaded and functioning - MCP server running on port %1")
                    .arg(m_serverP->getPort()));
        }

        // Create the MCP Server menu
        ActionContainer *menu = ActionManager::createMenu(Constants::MENU_ID);
        menu->menu()->setTitle(Tr::tr("MCP Server"));
        menu->menu()->setIcon(McpServerStatusDialog::icon().icon()); // Add icon to the main menu
        ActionManager::actionContainer(Core::Constants::M_TOOLS)->addMenu(menu);

        // Add separator for About
        menu->addSeparator();

        // About action (shows the existing status dialog)
        ActionBuilder(this, Constants::ABOUT_ACTION_ID)
            .addToContainer(Constants::MENU_ID)
            .setText(QString("ℹ️ About MCP Server"))
            .addOnTriggered(this, &McpServerPlugin::showAbout);

        // Add separator between About and commands
        menu->addSeparator();

        // MCP Protocol Discovery section
        ActionBuilder(this, Constants::MCP_INITIALIZE_ACTION_ID)
            .addToContainer(Constants::MENU_ID)
            .setText(Tr::tr("🚀 MCP: Initialize"))
            .addOnTriggered(this, &McpServerPlugin::executeMCPInitialize);

        ActionBuilder(this, Constants::MCP_TOOLS_LIST_ACTION_ID)
            .addToContainer(Constants::MENU_ID)
            .setText(Tr::tr("🛠️ MCP: List Tools"))
            .addOnTriggered(this, &McpServerPlugin::executeMCPToolsList);

        menu->addSeparator();

        // Project Information section
        ActionBuilder(this, Constants::LIST_SESSIONS_ACTION_ID)
            .addToContainer(Constants::MENU_ID)
            .setText(Tr::tr("📁 List Sessions"))
            .addOnTriggered(this, &McpServerPlugin::executeListSessions);

        ActionBuilder(this, Constants::LIST_PROJECTS_ACTION_ID)
            .addToContainer(Constants::MENU_ID)
            .setText(Tr::tr("📂 List Projects"))
            .addOnTriggered(this, &McpServerPlugin::executeListProjects);

        ActionBuilder(this, Constants::LIST_BUILD_CONFIGS_ACTION_ID)
            .addToContainer(Constants::MENU_ID)
            .setText(Tr::tr("⚙️ List Build Configs"))
            .addOnTriggered(this, &McpServerPlugin::executeListBuildConfigs);

        ActionBuilder(this, Constants::GET_CURRENT_PROJECT_ACTION_ID)
            .addToContainer(Constants::MENU_ID)
            .setText(Tr::tr("📋 Get Current Project"))
            .addOnTriggered(this, &McpServerPlugin::executeGetCurrentProject);

        ActionBuilder(this, Constants::GET_CURRENT_BUILD_CONFIG_ACTION_ID)
            .addToContainer(Constants::MENU_ID)
            .setText(Tr::tr("🔧 Get Current Build Config"))
            .addOnTriggered(this, &McpServerPlugin::executeGetCurrentBuildConfig);

        ActionBuilder(this, Constants::GET_CURRENT_SESSION_ACTION_ID)
            .addToContainer(Constants::MENU_ID)
            .setText(Tr::tr("🎯 Get Current Session"))
            .addOnTriggered(this, &McpServerPlugin::executeGetCurrentSession);

        ActionBuilder(this, Constants::LIST_OPEN_FILES_ACTION_ID)
            .addToContainer(Constants::MENU_ID)
            .setText(Tr::tr("📄 List Open Files"))
            .addOnTriggered(this, &McpServerPlugin::executeListOpenFiles);

        ActionBuilder(this, Constants::LIST_ISSUES_ACTION_ID)
            .addToContainer(Constants::MENU_ID)
            .setText(Tr::tr("⚠️ List Issues"))
            .addOnTriggered(this, &McpServerPlugin::executeListIssues);

        ActionBuilder(this, Constants::GET_METHOD_METADATA_ACTION_ID)
            .addToContainer(Constants::MENU_ID)
            .setText(Tr::tr("📊 Get Method Metadata"))
            .addOnTriggered(this, &McpServerPlugin::executeGetMethodMetadata);

        ActionBuilder(this, Constants::SET_METHOD_METADATA_ACTION_ID)
            .addToContainer(Constants::MENU_ID)
            .setText(Tr::tr("⚡ Set Method Metadata"))
            .addOnTriggered(this, &McpServerPlugin::executeSetMethodMetadata);

        menu->addSeparator();

        ActionBuilder(this, Constants::BUILD_ACTION_ID)
            .addToContainer(Constants::MENU_ID)
            .setText(Tr::tr("🔨 Build Project"))
            .addOnTriggered(this, &McpServerPlugin::executeBuild);

        ActionBuilder(this, Constants::RUN_PROJECT_ACTION_ID)
            .addToContainer(Constants::MENU_ID)
            .setText(Tr::tr("▶️ Run Project"))
            .addOnTriggered(this, &McpServerPlugin::executeRunProject);

        ActionBuilder(this, Constants::DEBUG_ACTION_ID)
            .addToContainer(Constants::MENU_ID)
            .setText(Tr::tr("🐛 Debug Project"))
            .addOnTriggered(this, &McpServerPlugin::executeDebug);

        ActionBuilder(this, Constants::STOP_DEBUG_ACTION_ID)
            .addToContainer(Constants::MENU_ID)
            .setText(Tr::tr("⏹️ Stop Debugging"))
            .addOnTriggered(this, &McpServerPlugin::executeStopDebug);

        ActionBuilder(this, Constants::CLEAN_PROJECT_ACTION_ID)
            .addToContainer(Constants::MENU_ID)
            .setText(Tr::tr("🧹 Clean Project"))
            .addOnTriggered(this, &McpServerPlugin::executeCleanProject);

        ActionBuilder(this, Constants::SAVE_SESSION_ACTION_ID)
            .addToContainer(Constants::MENU_ID)
            .setText(Tr::tr("💾 Save Session"))
            .addOnTriggered(this, &McpServerPlugin::executeSaveSession);

        menu->addSeparator();

        ActionBuilder(this, Constants::QUIT_ACTION_ID)
            .addToContainer(Constants::MENU_ID)
            .setText(Tr::tr("🚪 Quit Qt Creator"))
            .addOnTriggered(this, &McpServerPlugin::executeQuit);
    }

    void extensionsInitialized() final
    {
        // Retrieve objects from the plugin manager's object pool, if needed. (rare)
        // In the extensionsInitialized function, a plugin can be sure that all
        // plugins that depend on it have passed their initialize() and
        // extensionsInitialized() phase.
#ifdef WITH_TESTS
        addTestCreator(setupMCPServerTest);
#endif

    }

    ShutdownFlag aboutToShutdown() final
    {
        // Save settings
        // Disconnect from signals that are not needed during shutdown
        // Hide UI (if you add UI that is not in the main window directly)

        if (m_serverP) {
            m_serverP->stop();
        }

        return SynchronousShutdown;
    }

private:
    void outputMessage(const QString &message)
    {
        // Try different MessageManager methods
        Core::MessageManager::writeFlashing(message);
    }

    void showAbout()
    {
        McpServerStatusDialog dialog(m_serverP, ICore::dialogParent());
        dialog.exec();
    }

    void executeMCPInitialize()
    {
        if (!m_serverP || !m_serverP->isRunning()) {
            outputMessage("❌ MCP Server is not running. Please start Qt Creator first.");
            return;
        }

        // Create JSON-RPC request for MCP initialize
        QJsonObject request;
        request["jsonrpc"] = "2.0";
        request["method"] = "initialize";
        request["id"] = 1;

        QJsonObject params;
        params["protocolVersion"] = "2024-11-05";
        params["capabilities"] = QJsonObject();

        QJsonObject clientInfo;
        clientInfo["name"] = "Qt Creator";
        clientInfo["version"] = "1.0.0";
        params["clientInfo"] = clientInfo;

        request["params"] = params;

        QJsonDocument doc(request);
        QString requestJson = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
        outputMessage(QString("MCP Initialize Request: %1").arg(requestJson));

        // Actually call the MCP server and get the real response
        QJsonObject response = m_serverP->callMCPMethod("initialize", params);

        if (response.contains("error")) {
            outputMessage(
                QString("❌ MCP Error: %1").arg(response["error"].toObject()["message"].toString()));
        } else if (response.contains("result")) {
            QJsonObject result = response["result"].toObject();
            outputMessage("✅ MCP Initialize successful!");

            if (result.contains("protocolVersion")) {
                outputMessage(
                    QString("  Protocol Version: %1").arg(result["protocolVersion"].toString()));
            }
            if (result.contains("serverInfo")) {
                QJsonObject serverInfo = result["serverInfo"].toObject();
                if (serverInfo.contains("name")) {
                    outputMessage(QString("  Server Name: %1").arg(serverInfo["name"].toString()));
                }
                if (serverInfo.contains("version")) {
                    outputMessage(
                        QString("  Server Version: %1").arg(serverInfo["version"].toString()));
                }
            }
            if (result.contains("capabilities")) {
                outputMessage(
                    QString("  Capabilities: %1")
                        .arg(
                            QString::fromUtf8(QJsonDocument(result["capabilities"].toObject())
                                                  .toJson(QJsonDocument::Compact))));
            }
        } else {
            outputMessage("❌ No result in MCP response");
        }
    }

    void executeMCPToolsList()
    {
        if (!m_serverP || !m_serverP->isRunning()) {
            outputMessage("❌ MCP Server is not running. Please start Qt Creator first.");
            return;
        }

        // Create JSON-RPC request for MCP tools/list
        QJsonObject request;
        request["jsonrpc"] = "2.0";
        request["method"] = "tools/list";
        request["id"] = 1;

        QJsonDocument doc(request);
        QString requestJson = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
        outputMessage(QString("MCP Tools List Request: %1").arg(requestJson));

        // Actually call the MCP server and get the real response
        QJsonObject response = m_serverP->callMCPMethod("tools/list");

        if (response.contains("error")) {
            outputMessage(
                QString("❌ MCP Error: %1").arg(response["error"].toObject()["message"].toString()));
        } else if (response.contains("result")) {
            QJsonObject result = response["result"].toObject();
            if (result.contains("tools")) {
                QJsonArray tools = result["tools"].toArray();
                outputMessage(QString("✅ Found %1 MCP tools:").arg(tools.size()));

                for (const QJsonValue &toolValue : tools) {
                    QJsonObject tool = toolValue.toObject();
                    QString name = tool["name"].toString();
                    QString description = tool["description"].toString();
                    outputMessage(QString("  • %1: %2").arg(name, description));
                }
            } else {
                outputMessage("❌ Invalid MCP response format");
            }
        } else {
            outputMessage("❌ No result in MCP response");
        }
    }

    void executeListSessions()
    {
        QStringList sessions = m_commandsP->listSessions();
        outputMessage(QString("Available Sessions: %1").arg(sessions.join(", ")));
    }

    void executeListProjects()
    {
        QStringList projects = m_commandsP->listProjects();
        outputMessage(QString("Loaded Projects: %1").arg(projects.join(", ")));
    }

    void executeListBuildConfigs()
    {
        QStringList configs = m_commandsP->listBuildConfigs();
        outputMessage(QString("Build Configurations: %1").arg(configs.join(", ")));
    }

    void executeGetCurrentProject()
    {
        QString project = m_commandsP->getCurrentProject();
        outputMessage(QString("Current Project: %1").arg(project));
    }

    void executeGetCurrentBuildConfig()
    {
        QString config = m_commandsP->getCurrentBuildConfig();
        outputMessage(QString("Current Build Config: %1").arg(config));
    }

    void executeGetCurrentSession()
    {
        QString session = m_commandsP->getCurrentSession();
        outputMessage(QString("Current Session: %1").arg(session));
    }

    void executeListOpenFiles()
    {
        QStringList files = m_commandsP->listOpenFiles();
        outputMessage(QString("Open Files: %1").arg(files.join(", ")));
    }

    void executeListIssues()
    {
        QStringList issues = m_commandsP->listIssues();
        outputMessage(QString("Build Issues: %1").arg(issues.join(", ")));
    }

    void executeGetMethodMetadata()
    {
        outputMessage("Getting method metadata...");
        // Get metadata for all methods
        QString result = m_commandsP->getMethodMetadata();
        outputMessage(result);
    }

    void executeSetMethodMetadata()
    {
        outputMessage("Setting method metadata...");
        // For demonstration, set debug timeout to 120 seconds
        QString result = m_commandsP->setMethodMetadata("debug", 120);
        outputMessage(result);
    }

    void executeBuild()
    {
        outputMessage("Starting build...");
        bool success = m_commandsP->build();
        QString result = success ? QStringLiteral("Build started successfully")
                                 : QStringLiteral("Build failed to start");
        outputMessage(QString("Build result: %1").arg(result));
    }

    void executeRunProject()
    {
        outputMessage("Running project...");
        bool success = m_commandsP->runProject();
        QString result = success ? QStringLiteral("Project run started successfully")
                                 : QStringLiteral("Project run failed to start");
        outputMessage(QString("Run result: %1").arg(result));
    }

    void executeDebug()
    {
        outputMessage("Starting debug session...");
        QString result = m_commandsP->debug();
        outputMessage(QString("Debug result: %1").arg(result));
    }

    void executeStopDebug()
    {
        outputMessage("Stopping debug session...");
        QString result = m_commandsP->stopDebug();
        outputMessage(QString("Stop debug result: %1").arg(result));
    }

    void executeCleanProject()
    {
        outputMessage("Cleaning project...");
        bool success = m_commandsP->cleanProject();
        QString result = success ? QStringLiteral("Project clean started successfully")
                                 : QStringLiteral("Project clean failed to start");
        outputMessage(QString("Clean result: %1").arg(result));
    }

    void executeSaveSession()
    {
        outputMessage("Saving session...");
        bool success = m_commandsP->saveSession();
        QString result = success ? QStringLiteral("Session saved successfully")
                                 : QStringLiteral("Session save failed");
        outputMessage(QString("Save session result: %1").arg(result));
    }

    void executeQuit()
    {
        outputMessage("Quitting Qt Creator...");
        bool success = m_commandsP->quit();
        QString result = success ? QStringLiteral("Quit initiated successfully")
                                 : QStringLiteral("Quit failed");
        outputMessage(QString("Quit result: %1").arg(result));
    }

    McpServer *m_serverP = nullptr;
    McpCommands *m_commandsP = nullptr;
};

} // namespace Mcp::Internal

#include "plugin.moc"
