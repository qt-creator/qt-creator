// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "lspinspector.h"

#include "client.h"
#include "languageclientmanager.h"
#include "languageclientsettings.h"
#include "languageclienttr.h"

#include <coreplugin/icore.h>

#include <texteditor/texteditor.h>

#include <utils/jsontreeitem.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>
#include <utils/variablechooser.h>

#include <QAction>
#include <QBuffer>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QTreeView>
#include <QVBoxLayout>

using namespace LanguageServerProtocol;
using namespace Utils;

namespace LanguageClient {

// --- JSON tree helpers (for the capabilities tab) ---

class JsonTreeItemDelegate : public QStyledItemDelegate
{
public:
    QString displayText(const QVariant &value, const QLocale &) const override
    {
        QString result = value.toString();
        if (result.size() == 1) {
            switch (result.at(0).toLatin1()) {
            case '\n':
                return QString("\\n");
            case '\t':
                return QString("\\t");
            case '\r':
                return QString("\\r");
            }
        }
        return result;
    }
};

using JsonModel = TreeModel<JsonTreeItem>;

static JsonModel *createJsonModel(const QString &displayName, const QJsonValue &value)
{
    if (value.isNull())
        return nullptr;
    auto root = new JsonTreeItem(displayName, value);
    if (root->canFetchMore())
        root->fetchMore();

    auto model = new JsonModel(root);
    model->setHeader({{"Name"}, {"Value"}, {"Type"}});
    return model;
}

static QTreeView *createJsonTreeView()
{
    auto view = new QTreeView;
    view->setContextMenuPolicy(Qt::ActionsContextMenu);
    auto action = new QAction(Tr::tr("Expand All"), view);
    QObject::connect(action, &QAction::triggered, view, &QTreeView::expandAll);
    view->addAction(action);
    view->setAlternatingRowColors(true);
    view->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    view->setItemDelegate(new JsonTreeItemDelegate);
    return view;
}

// --- LspCapabilitiesWidget ---

class LspCapabilitiesWidget : public QWidget
{
public:
    LspCapabilitiesWidget();
    void setCapabilities(const Capabilities &serverCapabilities);

private:
    void updateOptionsView(const QString &method);

    DynamicCapabilities m_dynamicCapabilities;
    QTreeView *m_capabilitiesView = nullptr;
    QListWidget *m_dynamicCapabilitiesView = nullptr;
    QTreeView *m_dynamicOptionsView = nullptr;
    QGroupBox *m_dynamicCapabilitiesGroup = nullptr;
};

LspCapabilitiesWidget::LspCapabilitiesWidget()
{
    auto mainLayout = new QHBoxLayout;

    auto group = new QGroupBox(Tr::tr("Capabilities:"));
    QLayout *layout = new QHBoxLayout;
    m_capabilitiesView = createJsonTreeView();
    layout->addWidget(m_capabilitiesView);
    group->setLayout(layout);
    mainLayout->addWidget(group);

    m_dynamicCapabilitiesGroup = new QGroupBox(Tr::tr("Dynamic Capabilities:"));
    layout = new QVBoxLayout;
    auto label = new QLabel(Tr::tr("Method:"));
    layout->addWidget(label);
    m_dynamicCapabilitiesView = new QListWidget();
    layout->addWidget(m_dynamicCapabilitiesView);
    label = new QLabel(Tr::tr("Options:"));
    layout->addWidget(label);
    m_dynamicOptionsView = createJsonTreeView();
    layout->addWidget(m_dynamicOptionsView);
    m_dynamicCapabilitiesGroup->setLayout(layout);
    mainLayout->addWidget(m_dynamicCapabilitiesGroup);

    setLayout(mainLayout);

    connect(m_dynamicCapabilitiesView,
            &QListWidget::currentTextChanged,
            this,
            &LspCapabilitiesWidget::updateOptionsView);
}

void LspCapabilitiesWidget::setCapabilities(const Capabilities &serverCapabilities)
{
    if (m_capabilitiesView->model())
        m_capabilitiesView->model()->deleteLater();
    m_capabilitiesView->setModel(
        createJsonModel(Tr::tr("Server Capabilities"), QJsonObject(serverCapabilities.capabilities)));

    m_dynamicCapabilities = serverCapabilities.dynamicCapabilities;
    const QStringList &methods = m_dynamicCapabilities.registeredMethods();
    if (methods.isEmpty()) {
        m_dynamicCapabilitiesGroup->hide();
        return;
    }
    m_dynamicCapabilitiesGroup->show();
    m_dynamicCapabilitiesView->clear();
    m_dynamicCapabilitiesView->addItems(methods);
}

void LspCapabilitiesWidget::updateOptionsView(const QString &method)
{
    QAbstractItemModel *oldModel = m_dynamicOptionsView->model();
    m_dynamicOptionsView->setModel(createJsonModel(method, m_dynamicCapabilities.option(method)));
    delete oldModel;
}

// --- message editor (header widget) ---

static QString sendMessage(Client *client, const QString &msg)
{
    if (!client)
        return Tr::tr("No client selected");

    QString parseError;
    BaseMessage baseMsg;
    QByteArray asUtf8 = msg.toUtf8();
    QBuffer buf;
    buf.open(QIODevice::WriteOnly);
    buf.write(QString("Content-Length: %1\r\n\r\n").arg(asUtf8.size()).toUtf8());
    buf.write(asUtf8);
    buf.close();

    buf.open(QIODevice::ReadOnly);
    BaseMessage::parse(&buf, parseError, baseMsg);

    if (!parseError.isEmpty())
        return parseError;

    auto rpcMessage = JsonRpcMessage(baseMsg);
    if (!rpcMessage.parseError().isEmpty())
        return rpcMessage.parseError();

    client->sendMessage(rpcMessage, Client::SendDocUpdates::Send, LanguageClient::Schedule::Delayed);

    return {};
}

static QWidget *createMessageEditor(QComboBox *clients)
{
    auto container = new QWidget;

    TextEditor::BaseTextEditor *messageEditor = LanguageClient::createJsonEditor(container);
    messageEditor->editorWidget()->setVisible(false);
    messageEditor->document()->setContents(R"({
    "jsonrpc": "2.0",
    "id": "%{UUID}",
    "method": "checkStatus",
    "params": { "options": {"localChecksOnly": true} }
})");

    VariableChooser *vc = new VariableChooser(messageEditor->editorWidget());
    vc->addMacroExpanderProvider(MacroExpanderProvider(globalMacroExpander()));
    vc->addSupportedWidget(messageEditor->editorWidget());

    auto errorLabel = new QLabel;
    auto send = [clients, messageEditor, errorLabel] {
        if (messageEditor->editorWidget()->isHidden()) {
            messageEditor->editorWidget()->setVisible(true);
            return;
        }
        const QList<Client *> clientList = LanguageClientManager::clientsByName(
            clients->currentText());
        QString errMsg;
        for (Client *client : clientList) {
            errMsg += sendMessage(
                client,
                globalMacroExpander()->expand(messageEditor->textDocument()->plainText()));
        }
        errorLabel->setText(errMsg);
    };

    // clang-format off
    using namespace Layouting;
    Column {
        messageEditor->editorWidget(),
        Row { st, errorLabel, PushButton { text(Tr::tr("Send message")), onClicked(container, send) } },
        noMargin,
    }.attachTo(container);
    // clang-format on

    return container;
}

// --- LspInspector ---

static JsonRpcInspector::Settings makeSettings(LspInspector *self)
{
    JsonRpcInspector::Settings s;
    s.windowTitle = Tr::tr("Language Client Inspector");
    s.endpointLabel = Tr::tr("Language Server:");
    s.defaultExpandedKeys = {"params", "result", "error"};
    s.registerWindow = [](QWidget *widget) {
        Core::ICore::registerWindow(widget, Core::Context("LanguageClient.Inspector"));
        QObject::connect(
            Core::ICore::instance(), &Core::ICore::coreAboutToClose, widget, &QWidget::close);
    };
    s.headerWidget = [](QComboBox *clients) { return createMessageEditor(clients); };
    s.extraTabs = [self](const QString &clientName) {
        QList<QPair<QWidget *, QString>> tabs;
        auto capWidget = new LspCapabilitiesWidget;
        capWidget->setCapabilities(self->capabilities(clientName));
        tabs.append({capWidget, Tr::tr("Capabilities")});
        for (Client *const c : LanguageClientManager::clientsByName(clientName)) {
            for (const Client::CustomInspectorTab &tab : c->createCustomInspectorTabs())
                tabs.append({tab.first, tab.second});
        }
        return tabs;
    };
    return s;
}

LspInspector::LspInspector()
    : Utils::JsonRpcInspector(makeSettings(this))
{}

void LspInspector::log(LspLogMessage::MessageSender sender,
                       const QString &clientName,
                       const JsonRpcMessage &message)
{
    JsonRpcInspector::log(sender, clientName, message.toJsonObject());
}

void LspInspector::clientInitialized(const QString &clientName,
                                     const ServerCapabilities &capabilities)
{
    m_capabilities[clientName].capabilities = capabilities;
    m_capabilities[clientName].dynamicCapabilities.reset();
    refreshEndpoint(clientName);
}

void LspInspector::updateCapabilities(const QString &clientName,
                                      const DynamicCapabilities &dynamicCapabilities)
{
    m_capabilities[clientName].dynamicCapabilities = dynamicCapabilities;
    refreshEndpoint(clientName);
}

Capabilities LspInspector::capabilities(const QString &clientName) const
{
    return m_capabilities.value(clientName);
}

} // namespace LanguageClient
