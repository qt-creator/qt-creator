// Copyright (C) 2025 David M. Cotter
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpcommands.h"
#include "mcpserver.h"
#include "mcpserverconstants.h"
#include "mcpservertr.h"

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
    }

    void extensionsInitialized() final
    {
#ifdef WITH_TESTS
        addTestCreator(setupMCPServerTest);
#endif
    }

    ShutdownFlag aboutToShutdown() final
    {
        if (m_serverP)
            m_serverP->stop();

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

    McpServer *m_serverP = nullptr;
};

} // namespace Mcp::Internal

#include "mcpserverplugin.moc"
