// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "lspinspector.h"

#include "client.h"
#include "languageclientmanager.h"
#include "languageclienttr.h"

#include <coreplugin/icore.h>
#include <coreplugin/minisplitter.h>

#include <languageserverprotocol/jsonkeys.h>
#include <languageserverprotocol/jsonrpcmessages.h>

#include <utils/jsontreeitem.h>
#include <utils/listmodel.h>

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QJsonDocument>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QStyledItemDelegate>
#include <QTextCodec>
#include <QTreeView>

using namespace LanguageServerProtocol;
using namespace Utils;

namespace LanguageClient {

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

using JsonModel = Utils::TreeModel<Utils::JsonTreeItem>;

JsonModel *createJsonModel(const QString &displayName, const QJsonValue &value)
{
    if (value.isNull())
        return nullptr;
    auto root = new Utils::JsonTreeItem(displayName, value);
    if (root->canFetchMore())
        root->fetchMore();

    auto model = new JsonModel(root);
    model->setHeader({{"Name"}, {"Value"}, {"Type"}});
    return model;
}

QTreeView *createJsonTreeView()
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

QTreeView *createJsonTreeView(const QString &displayName, const QJsonValue &value)
{
    auto view = createJsonTreeView();
    view->setModel(createJsonModel(displayName, value));
    return view;
}

class MessageDetailWidget : public QGroupBox
{
public:
    MessageDetailWidget();

    void setMessage(const LspLogMessage &message);
    void clear();

private:
    QTreeView *m_jsonTree = nullptr;
};

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

class LspLogWidget : public Core::MiniSplitter
{
public:
    LspLogWidget();

    void addMessage(const LspLogMessage &message);
    void setMessages(const std::list<LspLogMessage> &messages);
    void saveLog();

    MessageDetailWidget *m_clientDetails = nullptr;
    QListView *m_messages = nullptr;
    MessageDetailWidget *m_serverDetails = nullptr;
    Utils::ListModel<LspLogMessage> m_model;

private:
    void currentMessageChanged(const QModelIndex &index);
    void selectMatchingMessage(const LspLogMessage &message);
};

static QVariant messageData(const LspLogMessage &message, int, int role)
{
    if (role == Qt::DisplayRole)
        return message.displayText();
    if (role == Qt::TextAlignmentRole)
        return message.sender == LspLogMessage::ClientMessage ? Qt::AlignLeft : Qt::AlignRight;
    return {};
}

LspLogWidget::LspLogWidget()
{
    setOrientation(Qt::Horizontal);

    m_clientDetails = new MessageDetailWidget;
    m_clientDetails->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_clientDetails->setTitle(Tr::tr("Client Message"));
    addWidget(m_clientDetails);
    setStretchFactor(0, 1);

    m_model.setDataAccessor(&messageData);
    m_messages = new QListView;
    m_messages->setModel(&m_model);
    m_messages->setAlternatingRowColors(true);
    m_model.setHeader({Tr::tr("Messages")});
    m_messages->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    m_messages->setSelectionMode(QAbstractItemView::MultiSelection);
    addWidget(m_messages);
    setStretchFactor(1, 0);

    m_serverDetails = new MessageDetailWidget;
    m_serverDetails->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_serverDetails->setTitle(Tr::tr("Server Message"));
    addWidget(m_serverDetails);
    setStretchFactor(2, 1);

    connect(m_messages->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this,
            &LspLogWidget::currentMessageChanged);
}

void LspLogWidget::currentMessageChanged(const QModelIndex &index)
{
    m_messages->clearSelection();
    if (!index.isValid()) {
        m_clientDetails->clear();
        m_serverDetails->clear();
        return;
    }
    LspLogMessage message = m_model.itemAt(index.row())->itemData;
    if (message.sender == LspLogMessage::ClientMessage)
        m_clientDetails->setMessage(message);
    else
        m_serverDetails->setMessage(message);
    selectMatchingMessage(message);
}

static bool matches(LspLogMessage::MessageSender sender,
                    const MessageId &id,
                    const LspLogMessage &message)
{
    if (message.sender != sender)
        return false;
    return message.id() == id;
}

void LspLogWidget::selectMatchingMessage(const LspLogMessage &message)
{
    MessageId id = message.id();
    if (!id.isValid())
        return;
    LspLogMessage::MessageSender sender = message.sender == LspLogMessage::ServerMessage
                                              ? LspLogMessage::ClientMessage
                                              : LspLogMessage::ServerMessage;
    LspLogMessage *matchingMessage = m_model.findData(
        [&](const LspLogMessage &message) { return matches(sender, id, message); });
    if (!matchingMessage)
        return;
    auto index = m_model.findIndex(
        [&](const LspLogMessage &message) { return &message == matchingMessage; });

    m_messages->selectionModel()->select(index, QItemSelectionModel::Select);
    if (matchingMessage->sender == LspLogMessage::ServerMessage)
        m_serverDetails->setMessage(*matchingMessage);
    else
        m_clientDetails->setMessage(*matchingMessage);
}

void LspLogWidget::addMessage(const LspLogMessage &message)
{
    m_model.appendItem(message);
}

void LspLogWidget::setMessages(const std::list<LspLogMessage> &messages)
{
    m_model.clear();
    for (const LspLogMessage &message : messages)
        m_model.appendItem(message);
}

void LspLogWidget::saveLog()
{
    QString contents;
    QTextStream stream(&contents);
    m_model.forAllData([&](const LspLogMessage &message) {
        stream << message.time.toString("hh:mm:ss.zzz") << ' ';
        stream << (message.sender == LspLogMessage::ClientMessage ? QString{"Client"}
                                                                  : QString{"Server"});
        stream << '\n';
        stream << QJsonDocument(message.message.toJsonObject()).toJson();
        stream << "\n\n";
    });

    const FilePath filePath = FileUtils::getSaveFilePath(this, Tr::tr("Log File"));
    if (filePath.isEmpty())
        return;
    FileSaver saver(filePath, QIODevice::Text);
    saver.write(contents.toUtf8());
    if (!saver.finalize(this))
        saveLog();
}

class LspInspectorWidget : public QDialog
{
public:
    explicit LspInspectorWidget(LspInspector *inspector);

    void selectClient(const QString &clientName);
private:
    void addMessage(const QString &clientName, const LspLogMessage &message);
    void updateCapabilities(const QString &clientName);
    void currentClientChanged(const QString &clientName);
    LspLogWidget *log() const;
    LspCapabilitiesWidget *capabilities() const;

    LspInspector * const m_inspector = nullptr;
    QTabWidget * const m_tabWidget;

    enum class TabIndex { Log, Capabilities, Custom };
    QComboBox *m_clients = nullptr;
};

void LspInspector::show(const QString &defaultClient)
{
    if (!m_currentWidget) {
        auto widget = new LspInspectorWidget(this);
        connect(widget, &LspInspectorWidget::finished, this, &LspInspector::onInspectorClosed);
        Core::ICore::registerWindow(widget, Core::Context("LanguageClient.Inspector"));
        m_currentWidget = widget;
    } else {
        QApplication::setActiveWindow(m_currentWidget);
    }
    if (!defaultClient.isEmpty())
        static_cast<LspInspectorWidget *>(m_currentWidget)->selectClient(defaultClient);
    m_currentWidget->show();
}

void LspInspector::log(const LspLogMessage::MessageSender sender,
                       const QString &clientName,
                       const JsonRpcMessage &message)
{
    std::list<LspLogMessage> &clientLog = m_logs[clientName];
    while (clientLog.size() >= static_cast<std::size_t>(m_logSize))
        clientLog.pop_front();
    clientLog.push_back({sender, QTime::currentTime(), message});
    emit newMessage(clientName, clientLog.back());
}

void LspInspector::clientInitialized(const QString &clientName, const ServerCapabilities &capabilities)
{
    m_capabilities[clientName].capabilities = capabilities;
    m_capabilities[clientName].dynamicCapabilities.reset();
    emit capabilitiesUpdated(clientName);
}

void LspInspector::updateCapabilities(const QString &clientName,
                                      const DynamicCapabilities &dynamicCapabilities)
{
    m_capabilities[clientName].dynamicCapabilities = dynamicCapabilities;
    emit capabilitiesUpdated(clientName);
}

std::list<LspLogMessage> LspInspector::messages(const QString &clientName) const
{
    return m_logs.value(clientName);
}

Capabilities LspInspector::capabilities(const QString &clientName) const
{
    return m_capabilities.value(clientName);
}

QList<QString> LspInspector::clients() const
{
    return m_logs.keys();
}

void LspInspector::onInspectorClosed()
{
    m_currentWidget->deleteLater();
    m_currentWidget = nullptr;
}

LspInspectorWidget::LspInspectorWidget(LspInspector *inspector)
    : m_inspector(inspector), m_tabWidget(new QTabWidget(this))
{
    setWindowTitle(Tr::tr("Language Client Inspector"));

    connect(inspector, &LspInspector::newMessage, this, &LspInspectorWidget::addMessage);
    connect(inspector, &LspInspector::capabilitiesUpdated,
            this, &LspInspectorWidget::updateCapabilities);
    connect(Core::ICore::instance(), &Core::ICore::coreAboutToClose, this, &QWidget::close);

    auto mainLayout = new QVBoxLayout;

    m_clients = new QComboBox;
    m_clients->addItem(Tr::tr("<Select>"));
    m_clients->addItems(inspector->clients());
    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->addWidget(new QLabel(Tr::tr("Language Server:")));
    hbox->addWidget(m_clients);
    hbox->addStretch();
    mainLayout->addLayout(hbox);

    m_tabWidget->addTab(new LspLogWidget, Tr::tr("Log"));
    m_tabWidget->addTab(new LspCapabilitiesWidget, Tr::tr("Capabilities"));
    mainLayout->addWidget(m_tabWidget);

    auto buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Save | QDialogButtonBox::Close);
    const auto clearButton = buttonBox->addButton(Tr::tr("Clear"), QDialogButtonBox::ResetRole);
    connect(clearButton, &QPushButton::clicked, this, [this] {
        m_inspector->clear();
        if (m_clients->currentIndex() != 0)
            currentClientChanged(m_clients->currentText());
    });
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    connect(m_clients,
            &QComboBox::currentTextChanged,
            this,
            &LspInspectorWidget::currentClientChanged);

    // save
    connect(buttonBox, &QDialogButtonBox::accepted, log(), &LspLogWidget::saveLog);

    // close
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    resize(1024, 768);
}

void LspInspectorWidget::selectClient(const QString &clientName)
{
    const int index = m_clients->findText(clientName, Qt::MatchExactly);
    if (index >= 0)
        m_clients->setCurrentIndex(index);
}

void LspInspectorWidget::addMessage(const QString &clientName, const LspLogMessage &message)
{
    if (m_clients->findText(clientName, Qt::MatchExactly) < 0)
        m_clients->addItem(clientName);
    if (m_clients->currentText() == clientName)
        log()->addMessage(message);
}

void LspInspectorWidget::updateCapabilities(const QString &clientName)
{
    if (m_clients->findText(clientName, Qt::MatchExactly) < 0)
        m_clients->addItem(clientName);
    if (m_clients->currentText() == clientName)
        capabilities()->setCapabilities(m_inspector->capabilities(clientName));
}

void LspInspectorWidget::currentClientChanged(const QString &clientName)
{
    log()->setMessages(m_inspector->messages(clientName));
    capabilities()->setCapabilities(m_inspector->capabilities(clientName));
    for (int i = m_tabWidget->count() - 1; i >= int(TabIndex::Custom); --i) {
        QWidget * const w = m_tabWidget->widget(i);
        m_tabWidget->removeTab(i);
        delete w;
    }
    for (Client * const c : LanguageClientManager::clients()) {
        if (c->name() != clientName)
            continue;
        for (const Client::CustomInspectorTab &tab : c->createCustomInspectorTabs())
            m_tabWidget->addTab(tab.first, tab.second);
        break;
    }
}

LspLogWidget *LspInspectorWidget::log() const
{
    return static_cast<LspLogWidget *>(m_tabWidget->widget(int(TabIndex::Log)));
}

LspCapabilitiesWidget *LspInspectorWidget::capabilities() const
{
    return static_cast<LspCapabilitiesWidget *>(m_tabWidget->widget(int(TabIndex::Capabilities)));
}

MessageDetailWidget::MessageDetailWidget()
{
    auto layout = new QVBoxLayout;
    setLayout(layout);

    m_jsonTree = createJsonTreeView();

    layout->addWidget(m_jsonTree);
}

void MessageDetailWidget::setMessage(const LspLogMessage &message)
{
    m_jsonTree->setModel(createJsonModel("content", message.message.toJsonObject()));
}

void MessageDetailWidget::clear()
{
    m_jsonTree->setModel(createJsonModel("", QJsonObject()));
}

LspLogMessage::LspLogMessage() = default;

LspLogMessage::LspLogMessage(MessageSender sender, const QTime &time, const JsonRpcMessage &message)
    : sender(sender)
    , time(time)
    , message(message)
{}

MessageId LspLogMessage::id() const
{
    if (!m_id.has_value())
        m_id = MessageId(message.toJsonObject().value(idKey));
    return *m_id;
}

QString LspLogMessage::displayText() const
{
    if (!m_displayText.has_value()) {
        m_displayText = QString(time.toString("hh:mm:ss.zzz") + '\n');
        m_displayText->append(
            message.toJsonObject().value(methodKey).toString(id().toString()));
    }
    return *m_displayText;
}

} // namespace LanguageClient
