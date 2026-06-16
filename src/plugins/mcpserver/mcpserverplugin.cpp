// Copyright (C) 2025 David M. Cotter
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpcommands.h"
#include "mcpserverconstants.h"
#include "mcpserverinspector.h"
#include "mcpservertr.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/mcp/mcpmanager.h>
#include <coreplugin/messagemanager.h>

#include <debugger/debuggerconstants.h>

#include <extensionsystem/iplugin.h>

#include <mcp/server/toolregistry.h>

#include <texteditor/textdocument.h>

#include <utils/algorithm.h>
#include <utils/aspects.h>
#include <utils/async.h>
#include <utils/icon.h>
#include <utils/layoutbuilder.h>
#include <utils/mimeutils.h>
#include <utils/stringutils.h>
#include <utils/stylehelper.h>

#include <projectexplorer/devicesupport/devicemanager.h>

#include <QApplication>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QTcpServer>
#include <QToolTip>

#include <QtTaskTree/QParallelTaskTreeRunner>

using namespace Core;
using namespace QtTaskTree;
using namespace Utils;

// Define logging category for the plugin
Q_LOGGING_CATEGORY(mcpPlugin, "qtc.mcpserver.plugin", QtInfoMsg)

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
                return ResultError(Tr::tr("Invalid IP address."));
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

// Calls resizeRowsToContents() deferred whenever the viewport is resized,
// so row heights are computed after the columns have their final widths.
class ResizeRowsOnViewportResize : public QObject
{
public:
    explicit ResizeRowsOnViewportResize(QTableView *view)
        : QObject(view)
        , m_view(view)
    {}

protected:
    bool eventFilter(QObject *, QEvent *e) override
    {
        if (e->type() == QEvent::Resize)
            QMetaObject::invokeMethod(m_view, &QTableView::resizeRowsToContents, Qt::QueuedConnection);
        return false;
    }

private:
    QTableView *m_view;
};

class PaddedItemDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QSize s = QStyledItemDelegate::sizeHint(option, index);
        s.rheight() += 12;
        return s;
    }
};

class ToolFilterProxyModel : public QSortFilterProxyModel
{
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        const QRegularExpression re = filterRegularExpression();
        if (!re.isValid() || re.pattern().isEmpty())
            return true;
        const QAbstractItemModel *m = sourceModel();
        const QString name = m->index(sourceRow, 0, sourceParent).data(Qt::UserRole).toString();
        const QString title = m->index(sourceRow, 1, sourceParent).data().toString();
        const QString desc = m->index(sourceRow, 2, sourceParent).data().toString();
        return name.contains(re) || title.contains(re) || desc.contains(re);
    }
};

class ToolNameDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QSize s = QStyledItemDelegate::sizeHint(option, index);
        const QString name = index.data(Qt::UserRole).toString();
        const QString title = index.data(Qt::DisplayRole).toString();
        if (!name.isEmpty() && name != title)
            s.rheight() += option.fontMetrics.height() + 2;
        s.rheight() += 12;
        return s;
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        const QString name = index.data(Qt::UserRole).toString();
        const QString title = opt.text;
        const bool showName = !name.isEmpty() && name != title;

        opt.text.clear();
        const QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

        const QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, opt.widget);
        const QRect r = textRect.adjusted(2, 4, -2, -4);

        const QColor baseColor = (opt.state & QStyle::State_Selected)
                                     ? opt.palette.highlightedText().color()
                                     : opt.palette.text().color();

        painter->save();
        painter->setPen(baseColor);

        if (!showName) {
            painter->drawText(r, Qt::AlignVCenter | Qt::AlignLeft, title);
        } else {
            const int lineH = opt.fontMetrics.height();
            const int blockH = lineH * 2 + 2;
            const int top = r.top() + (r.height() - blockH) / 2;

            painter->drawText(QRect(r.left(), top, r.width(), lineH),
                              Qt::AlignLeft, title);

            QColor muted = baseColor;
            muted.setAlphaF(0.55f);
            painter->setPen(muted);
            painter
                ->drawText(QRect(r.left(), top + lineH + 2, r.width(), lineH), Qt::AlignLeft, name);
        }

        painter->restore();
    }
};

class ToolEnablerAspect : public AspectContainer
{
public:
    ToolEnablerAspect(AspectContainer *parent)
        : AspectContainer(parent)
    {
        connect(
            &ToolRegistry::instance(),
            &ToolRegistry::toolRegistered,
            this,
            &ToolEnablerAspect::onToolRegistered);
        setLayouter([this]() { return buildLayout(); });
    }

    void apply() override
    {
        AspectContainer::apply();
        for (auto it = m_toolAspects.cbegin(); it != m_toolAspects.cend(); ++it)
            ToolRegistry::enableTool(it.key(), it.value()->value());
    }

private:
    void onToolRegistered()
    {
        for (const Schema::Tool &tool : ToolRegistry::registeredTools()) {
            const QString name = tool.name();
            if (m_toolAspects.contains(name))
                continue;
            auto *aspect = new BoolAspect(this);
            aspect->setSettingsKey(keyFromString(name));
            aspect->setLabelPlacement(BoolAspect::LabelPlacement::Compact);
            aspect->setDefaultValue(true);
            const SettingsGroupNester nester({"McpServer", "EnabledTools"});
            aspect->readSettings();
            ToolRegistry::enableTool(name, aspect->value());
            m_toolAspects[name] = aspect;
            m_toolMetadata[name] = tool;
        }
    }

    Layouting::Column buildLayout()
    {
        using namespace Layouting;

        auto *view = new QTableView;
        auto *model = new QStandardItemModel(0, 3, view);
        model->setHorizontalHeaderLabels({{}, Tr::tr("Name"), Tr::tr("Description")});

        for (const QString &name : Utils::sorted(m_toolAspects.keys())) {
            const Schema::Tool &tool = m_toolMetadata[name];
            BoolAspect *aspect = m_toolAspects[name];

            auto *checkItem = new QStandardItem;
            checkItem->setCheckable(true);
            checkItem->setCheckState(aspect->volatileValue() ? Qt::Checked : Qt::Unchecked);
            checkItem->setEditable(false);
            checkItem->setData(name, Qt::UserRole);
            checkItem->setTextAlignment(Qt::AlignTop | Qt::AlignHCenter);
            checkItem->setToolTip(Tr::tr("Enable or disable this tool for MCP clients."));

            auto *nameItem = new QStandardItem(tool.title().value_or(name));
            nameItem->setEditable(false);
            nameItem->setData(name, Qt::UserRole);

            auto *descItem = new QStandardItem(tool.description().value_or(QString{}));
            descItem->setEditable(false);

            model->appendRow({checkItem, nameItem, descItem});
        }

        auto *proxy = new ToolFilterProxyModel(view);
        proxy->setSourceModel(model);
        proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

        auto *filterEdit = new QLineEdit;
        filterEdit->setPlaceholderText(Tr::tr("Filter by name or description..."));
        filterEdit->setMinimumWidth(250);
        filterEdit->setClearButtonEnabled(true);

        view->setModel(proxy);
        view->setWordWrap(true);
        view->setSelectionBehavior(QAbstractItemView::SelectRows);
        view->setSelectionMode(QAbstractItemView::SingleSelection);
        view->verticalHeader()->setVisible(false);
        view->horizontalHeader()->setStretchLastSection(true);
        view->setShowGrid(false);
        view->resizeColumnToContents(0);
        view->resizeColumnToContents(1);
        view->setItemDelegate(new PaddedItemDelegate(view));
        view->setItemDelegateForColumn(1, new ToolNameDelegate(view));
        view->viewport()->installEventFilter(new ResizeRowsOnViewportResize(view));
        view->setAlternatingRowColors(true);

        QObject::connect(filterEdit, &QLineEdit::textChanged, proxy, [proxy, view](const QString &text) {
            proxy->setFilterFixedString(text);
            QMetaObject::invokeMethod(view, &QTableView::resizeRowsToContents, Qt::QueuedConnection);
        });

        QObject::connect(
            model,
            &QStandardItemModel::dataChanged,
            view,
            [this, model](const QModelIndex &topLeft, const QModelIndex &, const QList<int> &roles) {
                if (topLeft.column() != 0 || !roles.contains(Qt::CheckStateRole))
                    return;
                const QString name = model->data(topLeft, Qt::UserRole).toString();
                if (auto *aspect = m_toolAspects.value(name))
                    aspect->setVolatileValue(
                        model->data(topLeft, Qt::CheckStateRole).toInt() == Qt::Checked);
            });

        return Column{noMargin, Row{st, filterEdit}, view};
    }

    QMap<QString, BoolAspect *> m_toolAspects;
    QMap<QString, Schema::Tool> m_toolMetadata;
};

class McpServerPluginSettings : public AspectContainer
{
public:
    McpServerPluginSettings(McpServerPlugin *plugin);

    BoolAspect enabled{this};
    ListenAddressAspect listenAddress{this};
    IntegerAspect port{this};
    BoolAspect enableCors{this};
    ToolEnablerAspect enabledTools{this};
};

class McpServerSettingsPage : public Core::IOptionsPage
{
public:
    McpServerSettingsPage(McpServerPluginSettings *settings)
    {
        setId(Constants::SETTINGS_PAGE_ID);
        setDisplayName("Qt Creator MCP Server");
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
        ActionBuilder inspectAction(this, "McpServer.Inspector");
        inspectAction.setText(Tr::tr("Inspect MCP Server..."));
        inspectAction.addToContainer(Core::Constants::M_TOOLS_DEBUG);
        inspectAction.addOnTriggered([this] { inspector.show(); });

        m_server.setInspector(&inspector);
        settings.readSettings();
        restartServer();
    }

    void extensionsInitialized() final
    {
        // Initialize the server
        McpCommands::registerCommands();
        setupResources(this, m_server, &m_taskRunner);
    }

    ShutdownFlag aboutToShutdown() final
    {
        qDeleteAll(m_server.boundTcpServers());
        return SynchronousShutdown;
    }

    void restartServer()
    {
        qCDebug(mcpPlugin) << "(Re)-starting MCP server...";
        qDeleteAll(m_server.boundTcpServers());

        m_server.setCorsEnabled(settings.enableCors());

        McpManager::removeMcpServer(QT_MCPSERVER_MANAGER_ID);

        if (!settings.enabled()) {
            qCInfo(mcpPlugin) << "MCP server is disabled in settings, not starting.";
            return;
        }

        QTcpServer *tcpServer = new QTcpServer(this);
        if (!tcpServer->listen(settings.listenAddress.hostAddress(), settings.port())
            || !m_server.bind(tcpServer)) {
            delete tcpServer;
            MessageManager::writeFlashing(
                Tr::tr("Failed to start MCP server on \"%1:%2\".")
                    .arg(settings.listenAddress.hostAddress().toString())
                    .arg(settings.port()));
        } else {
            qCInfo(mcpPlugin).noquote()
                << "MCP server started successfully on"
                << QString("%1:%2")
                       .arg(tcpServer->serverAddress().toString())
                       .arg(tcpServer->serverPort());
            // Show startup message in General Messages panel
            MessageManager::writeSilently(
                Tr::tr("The MCP server is listening on \"%1:%2\".")
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
                {},
                {}};

            QTC_CHECK(McpManager::registerMcpServer(serverInfo));
        }
    }

    bool isServerRunning() const { return !m_server.boundTcpServers().isEmpty(); }
    QString listenAddresses() const
    {
        return (Utils::transform<QStringList>(
                    m_server.boundTcpServers(),
                    [](QTcpServer *s) {
                        if (s->serverAddress() == QHostAddress::Any)
                            return QString("[*:%1](%1)").arg(s->serverPort());
                        else if (s->serverAddress() == QHostAddress::LocalHost)
                            return QString("[127.0.0.1:%1](127.0.0.1:%1)").arg(s->serverPort());
                        return QString("[%1:%2](%1:%2)")
                            .arg(s->serverAddress().toString())
                            .arg(s->serverPort());
                    }).join(", "));
    }

private:
    QParallelTaskTreeRunner m_taskRunner;

    Mcp::AutoRegisteringServer m_server{
        Mcp::Schema::Implementation()
            .name("qt-creator-mcp-server")
            .title("Qt Creator MCP Server")
            .description(
                "MCP server for Qt Creator to allow external tools to interact with the IDE.")
            .version(QCoreApplication::applicationVersion())};

    McpServerPluginSettings settings{this};
    McpServerSettingsPage settingsPage{&settings};

    McpInspector inspector;
};

McpServerPluginSettings::McpServerPluginSettings(McpServerPlugin *plugin)
{
    setAutoApply(false);

    setSettingsGroup("McpServer");

    enabled.setLabel(Tr::tr("Enable MCP Server"));
    enabled.setDefaultValue(true);

    listenAddress.setSettingsGroup("Listen");
    listenAddress.setEnabler(&enabled);
    listenAddress.setToolTip(
        Tr::tr("The address the MCP Server should listen on for incoming connections."));

    port.setSettingsKey("Port");
    port.setLabel(Tr::tr("Port:"));
    port.setDefaultValue(0);
    port.setSpecialValueText(Tr::tr("Automatic"));
    port.setRange(0, 65535);
    port.setToolTip(
        Tr::tr("The port for the MCP Server to listen on. Leave on 0 to auto-select a free port."));
    port.setEnabler(&enabled);

    enableCors.setSettingsKey("EnableCors");
    enableCors.setEnabler(&enabled);
    enableCors.setLabel(Tr::tr("Enable Cross Origin access:"));
    enableCors.setToolTip(
        Tr::tr(
            "Enable Cross-Origin Resource Sharing (CORS) for the MCP Server. "
            "This is necessary if you want to connect to the server from a web application."));
    enableCors.setDefaultValue(false);

    enabledTools.setSettingsGroup("EnabledTools");
    enabledTools.setToolTip(Tr::tr("Select which tools to enable or disable"));

    connect(&enabled, &BaseAspect::changed, plugin, &McpServerPlugin::restartServer);
    connect(&listenAddress, &BaseAspect::changed, plugin, &McpServerPlugin::restartServer);
    connect(&port, &BaseAspect::changed, plugin, &McpServerPlugin::restartServer);
    connect(&enableCors, &BaseAspect::changed, plugin, &McpServerPlugin::restartServer);

    setLayouter([this, plugin]() {
        using namespace Layouting;
        auto statusLabel = new QLabel();
        auto statusIcon = new QLabel();

        statusIcon->setFixedSize(24, 24);
        statusIcon->setAlignment(Qt::AlignCenter);

        statusLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        statusLabel->setTextFormat(Qt::MarkdownText);
        statusLabel->setOpenExternalLinks(false);
        connect(statusLabel, &QLabel::linkActivated, [](const QString &link) {
            Utils::setClipboardAndSelection(link);
            QToolTip::showText(QCursor::pos(), Tr::tr("Address copied to clipboard."), nullptr);
        });

        const auto updateStatus = [plugin, statusIcon, statusLabel]() {
            const bool isRunning = plugin->isServerRunning();
            auto p = statusLabel->palette();

            p.setColor(
                QPalette::WindowText,
                creatorColor(isRunning ? Theme::TextColorNormal : Theme::TextColorError));
            statusLabel->setPalette(p);
            statusIcon->setPalette(p);
            statusIcon->setText(isRunning ? "✓" : "✗");

            if (isRunning) {
                statusLabel->setText(
                    Tr::tr("The MCP Server is running, listening on: %1.")
                        .arg(plugin->listenAddresses()));
            } else {
                statusLabel->setText(Tr::tr("The MCP Server is not running."));
            }
        };

        updateStatus();
        connect(this, &AspectContainer::applied, this, [updateStatus]() { updateStatus(); });

        // clang-format off
        return Form {
            enabled, br,
            Tr::tr("Listen on:"), listenAddress, br,
            port, br,
            enableCors, br,
            statusIcon, statusLabel, br,
            enabledTools, br,
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
            const QString strPath = path.toUrl().toString();
            server.addResource(
                Mcp::Schema::Resource().uri(strPath).mimeType(mimeType).name(path.fileName()),
                getOpenEditorContent);
        });
    QObject::connect(
        editorManager, &EditorManager::editorAboutToClose, guard, [&server](Core::IEditor *editor) {
            const FilePath path = editor->document()->filePath();
            const QString strPath = path.toUrl().toString();
            server.removeResource(strPath);
        });

    server.setResourceFallbackCallback(
        [](Mcp::Schema::ReadResourceRequestParams params) -> Result<Mcp::Schema::ReadResourceResult> {
            const FilePath filePath = FilePath::fromUrl(QUrl(params.uri()));
            if (!filePath.exists())
                return ResultError("File does not exist: " + filePath.toUserOutput());

            if (filePath.isDir()) {
                FilePaths entries = filePath.dirEntries(DirFilterFlag::AllEntries | DirFilterFlag::NoDotAndDotDot);
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
            .uriTemplate("file:///{path}")
            .name("Local files")
            .description("Access local files by URI, e.g. file:///path/to/file.txt"));

    const auto addTemplate = [&server](const FilePath &root, const QString &name) {
        server.addResourceTemplate(
            Mcp::Schema::ResourceTemplate()
                .uriTemplate(root.toUrl().toString() + "{path}")
                .name(name)
                .description(QString("Access files on device %1 by URI, e.g. %2/path/to/file.txt")
                                 .arg(name)
                                 .arg(root.toUrl().toString())));
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
                            {patterns, DirFilterFlag::AllEntries | DirFilterFlag::NoDotAndDotDot});
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
