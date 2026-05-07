// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpserverinspector.h"

#include "mcpservertr.h"

#include <coreplugin/icore.h>
#include <coreplugin/minisplitter.h>

#include <mcp/server/mcpserver.h>

#include <utils/fileutils.h>
#include <utils/jsontreeitem.h>
#include <utils/layoutbuilder.h>
#include <utils/listmodel.h>

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHeaderView>
#include <QJsonDocument>
#include <QListView>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QTreeView>
#include <QVBoxLayout>

using namespace Utils;

namespace Mcp::Internal {

// --- JSON tree helpers (same pattern as LspInspector) ---

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

// --- MessageDetailWidget ---

class MessageDetailWidget : public QGroupBox
{
public:
    MessageDetailWidget()
    {
        auto layout = new QVBoxLayout;
        setLayout(layout);
        m_jsonTree = createJsonTreeView();
        layout->addWidget(m_jsonTree);
    }

    void setMessage(const McpLogMessage &message)
    {
        m_jsonTree->setModel(createJsonModel("content", message.message));
    }

    void clear() { m_jsonTree->setModel(createJsonModel("", QJsonObject())); }

private:
    QTreeView *m_jsonTree = nullptr;
};

// --- McpLogWidget ---

static QVariant messageData(const McpLogMessage &message, int, int role)
{
    if (role == Qt::DisplayRole)
        return message.displayText();
    if (role == Qt::TextAlignmentRole)
        return message.sender == McpLogMessage::ClientMessage ? Qt::AlignLeft : Qt::AlignRight;
    return {};
}

class McpLogWidget : public Core::MiniSplitter
{
public:
    McpLogWidget()
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

        connect(
            m_messages->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this,
            &McpLogWidget::currentMessageChanged);
    }

    void addMessage(const McpLogMessage &message) { m_model.appendItem(message); }

    void setMessages(const std::list<McpLogMessage> &messages)
    {
        m_model.clear();
        for (const McpLogMessage &message : messages)
            m_model.appendItem(message);
    }

    void saveLog()
    {
        QString contents;
        QTextStream stream(&contents);
        m_model.forAllData([&](const McpLogMessage &message) {
            stream << message.time.toString("hh:mm:ss.zzz") << ' ';
            stream
                << (message.sender == McpLogMessage::ClientMessage ? QString{"Client"}
                                                                   : QString{"Server"});
            stream << '\n';
            stream << QJsonDocument(message.message).toJson();
            stream << "\n\n";
        });

        const FilePath filePath = FileUtils::getSaveFilePath(Tr::tr("Log File"));
        if (filePath.isEmpty())
            return;
        FileSaver saver(filePath, QIODevice::Text);
        saver.write(contents.toUtf8());
        if (const Result<> res = saver.finalize(); !res) {
            FileUtils::showError(res.error());
            saveLog();
        }
    }

private:
    void currentMessageChanged(const QModelIndex &index)
    {
        m_messages->clearSelection();
        if (!index.isValid()) {
            m_clientDetails->clear();
            m_serverDetails->clear();
            return;
        }
        McpLogMessage message = m_model.itemAt(index.row())->itemData;
        if (message.sender == McpLogMessage::ClientMessage)
            m_clientDetails->setMessage(message);
        else
            m_serverDetails->setMessage(message);
        selectMatchingMessage(message);
    }

    void selectMatchingMessage(const McpLogMessage &message)
    {
        QJsonValue msgId = message.id();
        if (msgId.isNull() || msgId.isUndefined())
            return;
        McpLogMessage::MessageSender otherSender = message.sender == McpLogMessage::ServerMessage
                                                       ? McpLogMessage::ClientMessage
                                                       : McpLogMessage::ServerMessage;
        McpLogMessage *matchingMessage = m_model.findData([&](const McpLogMessage &msg) {
            return msg.sender == otherSender && msg.id() == msgId;
        });
        if (!matchingMessage)
            return;
        auto matchIndex = m_model.findIndex(
            [&](const McpLogMessage &msg) { return &msg == matchingMessage; });

        m_messages->selectionModel()->select(matchIndex, QItemSelectionModel::Select);
        if (matchingMessage->sender == McpLogMessage::ServerMessage)
            m_serverDetails->setMessage(*matchingMessage);
        else
            m_clientDetails->setMessage(*matchingMessage);
    }

    MessageDetailWidget *m_clientDetails = nullptr;
    QListView *m_messages = nullptr;
    MessageDetailWidget *m_serverDetails = nullptr;
    ListModel<McpLogMessage> m_model;
};

// --- McpInspectorWidget ---

class McpInspectorWidget : public QDialog
{
public:
    explicit McpInspectorWidget(McpInspector *inspector)
        : m_inspector(inspector)
    {
        setWindowTitle(Tr::tr("MCP Server Inspector"));

        connect(inspector, &McpInspector::newMessage, this, &McpInspectorWidget::addMessage);
        connect(Core::ICore::instance(), &Core::ICore::coreAboutToClose, this, &QWidget::close);

        m_clients = new QComboBox;
        m_clients->addItem(Tr::tr("<Select>"));
        m_clients->addItems(inspector->sessions());

        m_logWidget = new McpLogWidget;

        auto buttonBox = new QDialogButtonBox;
        buttonBox->setStandardButtons(QDialogButtonBox::Save | QDialogButtonBox::Close);
        const auto clearButton = buttonBox->addButton(Tr::tr("Clear"), QDialogButtonBox::ResetRole);
        connect(clearButton, &QPushButton::clicked, this, [this] {
            m_inspector->clear();
            if (m_clients->currentIndex() != 0)
                currentClientChanged(m_clients->currentText());
        });

        // clang-format off
        using namespace Layouting;
        Column {
            Row { Tr::tr("Session:"), m_clients, st },
            m_logWidget,
            buttonBox,
        }.attachTo(this);
        // clang-format on

        connect(
            m_clients,
            &QComboBox::currentTextChanged,
            this,
            &McpInspectorWidget::currentClientChanged);

        connect(buttonBox, &QDialogButtonBox::accepted, m_logWidget, &McpLogWidget::saveLog);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        resize(1024, 768);
    }

    void selectClient(const QString &clientName)
    {
        const int index = m_clients->findText(clientName, Qt::MatchExactly);
        if (index >= 0)
            m_clients->setCurrentIndex(index);
    }

private:
    void addMessage(const QString &clientName, const McpLogMessage &message)
    {
        if (m_clients->findText(clientName, Qt::MatchExactly) < 0)
            m_clients->addItem(clientName);
        if (m_clients->currentText() == clientName)
            m_logWidget->addMessage(message);
    }

    void currentClientChanged(const QString &clientName)
    {
        m_logWidget->setMessages(m_inspector->messages(clientName));
    }

    McpInspector *const m_inspector = nullptr;
    McpLogWidget *m_logWidget = nullptr;
    QComboBox *m_clients = nullptr;
};

// --- McpLogMessage ---

McpLogMessage::McpLogMessage(MessageSender sender, const QTime &time, const QJsonObject &message)
    : sender(sender)
    , time(time)
    , message(message)
{}

QJsonValue McpLogMessage::id() const
{
    if (!m_id.has_value())
        m_id = message.value(QLatin1String("id"));
    return *m_id;
}

QString McpLogMessage::displayText() const
{
    if (!m_displayText.has_value()) {
        m_displayText = time.toString("hh:mm:ss.zzz") + '\n';
        const QString method = message.value(QLatin1String("method")).toString();
        if (!method.isEmpty()) {
            m_displayText->append(method);
        } else {
            const QJsonValue msgId = id();
            if (msgId.isDouble())
                m_displayText->append(
                    QString("Response #%1").arg(static_cast<qint64>(msgId.toDouble())));
            else if (msgId.isString())
                m_displayText->append(QString("Response #%1").arg(msgId.toString()));
            else
                m_displayText->append(Tr::tr("Unknown"));
        }
    }
    return *m_displayText;
}

// --- McpInspector ---

void McpInspector::show(const QString &defaultClient)
{
    if (!m_currentWidget) {
        auto widget = new McpInspectorWidget(this);
        connect(widget, &McpInspectorWidget::finished, this, &McpInspector::onInspectorClosed);
        Core::ICore::registerWindow(widget, Core::Context("McpClient.Inspector"));
        m_currentWidget = widget;
    } else {
        QApplication::setActiveWindow(m_currentWidget);
    }
    if (!defaultClient.isEmpty())
        static_cast<McpInspectorWidget *>(m_currentWidget)->selectClient(defaultClient);
    m_currentWidget->show();
}

void McpInspector::log(
    McpLogMessage::MessageSender sender, const QString &clientName, const QJsonObject &message)
{
    std::list<McpLogMessage> &clientLog = m_logs[clientName];
    while (clientLog.size() >= static_cast<std::size_t>(m_logSize))
        clientLog.pop_front();
    clientLog.push_back({sender, QTime::currentTime(), message});
    emit newMessage(clientName, clientLog.back());
}

std::list<McpLogMessage> McpInspector::messages(const QString &clientName) const
{
    return m_logs.value(clientName);
}

QStringList McpInspector::sessions() const
{
    return m_logs.keys();
}

QString McpInspector::sessionIdToDisplayName(const QString &sessionId)
{
    return sessionId.isEmpty() ? Tr::tr("Global") : sessionId;
}

void McpInspector::onInspectorClosed()
{
    m_currentWidget->deleteLater();
    m_currentWidget = nullptr;
}

} // namespace Mcp::Internal
