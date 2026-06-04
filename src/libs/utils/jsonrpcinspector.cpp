// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jsonrpcinspector.h"

#include "fileutils.h"
#include "jsontreeitem.h"
#include "layoutbuilder.h"
#include "listmodel.h"
#include "utilstr.h"

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
#include <QSet>
#include <QSplitter>
#include <QStyledItemDelegate>
#include <QTabWidget>
#include <QTextStream>
#include <QTreeView>
#include <QVBoxLayout>

namespace Utils {

// --- JSON tree helpers ---

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
    explicit MessageDetailWidget(const QStringList &defaultExpandedKeys)
        : m_expandedKeys(defaultExpandedKeys.cbegin(), defaultExpandedKeys.cend())
    {
        auto layout = new QVBoxLayout;
        setLayout(layout);
        m_jsonTree = createJsonTreeView();
        layout->addWidget(m_jsonTree);
    }

    void setMessage(const JsonRpcLogMessage &message)
    {
        if (m_jsonTree->model())
            m_jsonTree->model()->deleteLater();
        m_jsonTree->setModel(createJsonModel("content", message.message));
        if (!m_expandedKeys.isEmpty())
            expandDefault({});
    }

    void clear()
    {
        if (m_jsonTree->model())
            m_jsonTree->model()->deleteLater();
        m_jsonTree->setModel(createJsonModel("", QJsonObject()));
    }

private:
    // Expand the implicit top-level node, then any descendant whose member name is in
    // m_expandedKeys. Expanding an index triggers the model's lazy fetchMore(), so the
    // children of a matched node become available before recursing into them.
    void expandDefault(const QModelIndex &parent)
    {
        QAbstractItemModel *model = m_jsonTree->model();
        if (!model)
            return;
        for (int row = 0, count = model->rowCount(parent); row < count; ++row) {
            const QModelIndex index = model->index(row, 0, parent);
            const bool expand = !parent.isValid() // the implicit "content" wrapper node
                                || m_expandedKeys.contains(index.data(Qt::DisplayRole).toString());
            if (expand) {
                m_jsonTree->expand(index);
                expandDefault(index);
            }
        }
    }

    QTreeView *m_jsonTree = nullptr;
    const QSet<QString> m_expandedKeys;
};

// --- LogWidget ---

static QVariant messageData(const JsonRpcLogMessage &message, int, int role)
{
    if (role == Qt::DisplayRole)
        return message.displayText();
    if (role == Qt::TextAlignmentRole)
        return message.sender == JsonRpcLogMessage::ClientMessage ? Qt::AlignLeft : Qt::AlignRight;
    return {};
}

class LogWidget : public QSplitter
{
public:
    explicit LogWidget(const QStringList &defaultExpandedKeys)
    {
        setOrientation(Qt::Horizontal);

        m_clientDetails = new MessageDetailWidget(defaultExpandedKeys);
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

        m_serverDetails = new MessageDetailWidget(defaultExpandedKeys);
        m_serverDetails->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_serverDetails->setTitle(Tr::tr("Server Message"));
        addWidget(m_serverDetails);
        setStretchFactor(2, 1);

        connect(m_messages->selectionModel(),
                &QItemSelectionModel::currentChanged,
                this,
                &LogWidget::currentMessageChanged);
    }

    void addMessage(const JsonRpcLogMessage &message) { m_model.appendItem(message); }

    void setMessages(const std::list<JsonRpcLogMessage> &messages)
    {
        m_model.clear();
        for (const JsonRpcLogMessage &message : messages)
            m_model.appendItem(message);
    }

    void saveLog()
    {
        QString contents;
        QTextStream stream(&contents);
        m_model.forAllData([&](const JsonRpcLogMessage &message) {
            stream << message.time.toString("hh:mm:ss.zzz") << ' ';
            stream << (message.sender == JsonRpcLogMessage::ClientMessage ? QString{"Client"}
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
        JsonRpcLogMessage message = m_model.itemAt(index.row())->itemData;
        if (message.sender == JsonRpcLogMessage::ClientMessage)
            m_clientDetails->setMessage(message);
        else
            m_serverDetails->setMessage(message);
        selectMatchingMessage(message);
    }

    void selectMatchingMessage(const JsonRpcLogMessage &message)
    {
        const QJsonValue msgId = message.id();
        if (msgId.isNull() || msgId.isUndefined())
            return;
        const JsonRpcLogMessage::MessageSender otherSender
            = message.sender == JsonRpcLogMessage::ServerMessage ? JsonRpcLogMessage::ClientMessage
                                                                 : JsonRpcLogMessage::ServerMessage;
        JsonRpcLogMessage *matchingMessage = m_model.findData([&](const JsonRpcLogMessage &msg) {
            return msg.sender == otherSender && msg.id() == msgId;
        });
        if (!matchingMessage)
            return;
        auto matchIndex = m_model.findIndex(
            [&](const JsonRpcLogMessage &msg) { return &msg == matchingMessage; });

        m_messages->selectionModel()->select(matchIndex, QItemSelectionModel::Select);
        if (matchingMessage->sender == JsonRpcLogMessage::ServerMessage)
            m_serverDetails->setMessage(*matchingMessage);
        else
            m_clientDetails->setMessage(*matchingMessage);
    }

    MessageDetailWidget *m_clientDetails = nullptr;
    QListView *m_messages = nullptr;
    MessageDetailWidget *m_serverDetails = nullptr;
    ListModel<JsonRpcLogMessage> m_model;
};

// --- InspectorWidget ---

class InspectorWidget : public QDialog
{
public:
    explicit InspectorWidget(JsonRpcInspector *inspector)
        : m_inspector(inspector)
    {
        const JsonRpcInspector::Settings &settings = inspector->settings();
        setWindowTitle(settings.windowTitle);

        connect(inspector, &JsonRpcInspector::newMessage, this, &InspectorWidget::addMessage);
        connect(inspector,
                &JsonRpcInspector::endpointRefreshRequested,
                this,
                &InspectorWidget::refreshEndpoint);

        m_endpoints = new QComboBox;
        m_endpoints->addItem(Tr::tr("<Select>"));
        m_endpoints->addItems(inspector->endpoints());

        m_logWidget = new LogWidget(settings.defaultExpandedKeys);

        QWidget *header = settings.headerWidget ? settings.headerWidget(m_endpoints) : nullptr;

        QWidget *logArea = m_logWidget;
        if (settings.extraTabs) {
            m_tabWidget = new QTabWidget;
            m_tabWidget->addTab(m_logWidget, Tr::tr("Log"));
            logArea = m_tabWidget;
        }

        auto buttonBox = new QDialogButtonBox;
        buttonBox->setStandardButtons(QDialogButtonBox::Save | QDialogButtonBox::Close);
        const auto clearButton = buttonBox->addButton(Tr::tr("Clear"), QDialogButtonBox::ResetRole);
        connect(clearButton, &QPushButton::clicked, this, [this] {
            m_inspector->clear();
            if (m_endpoints->currentIndex() != 0)
                currentEndpointChanged(m_endpoints->currentText());
        });

        // clang-format off
        using namespace Layouting;
        if (header) {
            Column {
                Row { settings.endpointLabel, m_endpoints, st },
                header,
                logArea,
                buttonBox,
            }.attachTo(this);
        } else {
            Column {
                Row { settings.endpointLabel, m_endpoints, st },
                logArea,
                buttonBox,
            }.attachTo(this);
        }
        // clang-format on

        connect(m_endpoints,
                &QComboBox::currentTextChanged,
                this,
                &InspectorWidget::currentEndpointChanged);

        connect(buttonBox, &QDialogButtonBox::accepted, m_logWidget, &LogWidget::saveLog);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        resize(1024, 768);
    }

    void selectEndpoint(const QString &endpoint)
    {
        const int index = m_endpoints->findText(endpoint, Qt::MatchExactly);
        if (index >= 0)
            m_endpoints->setCurrentIndex(index);
    }

private:
    void addMessage(const QString &endpoint, const JsonRpcLogMessage &message)
    {
        if (m_endpoints->findText(endpoint, Qt::MatchExactly) < 0)
            m_endpoints->addItem(endpoint);
        if (m_endpoints->currentText() == endpoint)
            m_logWidget->addMessage(message);
    }

    void currentEndpointChanged(const QString &endpoint)
    {
        m_logWidget->setMessages(m_inspector->messages(endpoint));
        rebuildExtraTabs(endpoint);
    }

    void refreshEndpoint(const QString &endpoint)
    {
        if (m_endpoints->currentText() == endpoint)
            rebuildExtraTabs(endpoint);
    }

    void rebuildExtraTabs(const QString &endpoint)
    {
        if (!m_tabWidget)
            return;
        while (m_tabWidget->count() > 1)
            delete m_tabWidget->widget(m_tabWidget->count() - 1);
        const QList<QPair<QWidget *, QString>> tabs = m_inspector->settings().extraTabs(endpoint);
        for (const auto &tab : tabs)
            m_tabWidget->addTab(tab.first, tab.second);
    }

    JsonRpcInspector *const m_inspector = nullptr;
    LogWidget *m_logWidget = nullptr;
    QTabWidget *m_tabWidget = nullptr;
    QComboBox *m_endpoints = nullptr;
};

// --- JsonRpcLogMessage ---

JsonRpcLogMessage::JsonRpcLogMessage(MessageSender sender,
                                     const QTime &time,
                                     const QJsonObject &message)
    : sender(sender)
    , time(time)
    , message(message)
{}

QJsonValue JsonRpcLogMessage::id() const
{
    if (!m_id.has_value())
        m_id = message.value(QLatin1String("id"));
    return *m_id;
}

QString JsonRpcLogMessage::displayText() const
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

// --- JsonRpcInspector ---

JsonRpcInspector::JsonRpcInspector(Settings settings, QObject *parent)
    : QObject(parent)
    , m_settings(std::move(settings))
{}

void JsonRpcInspector::show(const QString &defaultEndpoint)
{
    if (!m_currentWidget) {
        auto widget = new InspectorWidget(this);
        connect(widget, &QDialog::finished, this, &JsonRpcInspector::onInspectorClosed);
        if (m_settings.registerWindow)
            m_settings.registerWindow(widget);
        m_currentWidget = widget;
    } else {
        QApplication::setActiveWindow(m_currentWidget);
    }
    if (!defaultEndpoint.isEmpty())
        static_cast<InspectorWidget *>(m_currentWidget)->selectEndpoint(defaultEndpoint);
    m_currentWidget->show();
}

void JsonRpcInspector::log(JsonRpcLogMessage::MessageSender sender,
                           const QString &endpoint,
                           const QJsonObject &message)
{
    std::list<JsonRpcLogMessage> &endpointLog = m_logs[endpoint];
    while (endpointLog.size() >= static_cast<std::size_t>(m_settings.maxLogSize))
        endpointLog.pop_front();
    endpointLog.emplace_back(sender, QTime::currentTime(), message);
    emit newMessage(endpoint, endpointLog.back());
}

std::list<JsonRpcLogMessage> JsonRpcInspector::messages(const QString &endpoint) const
{
    return m_logs.value(endpoint);
}

QStringList JsonRpcInspector::endpoints() const
{
    return m_logs.keys();
}

void JsonRpcInspector::refreshEndpoint(const QString &endpoint)
{
    emit endpointRefreshRequested(endpoint);
}

void JsonRpcInspector::onInspectorClosed()
{
    m_currentWidget->deleteLater();
    m_currentWidget = nullptr;
}

} // namespace Utils
