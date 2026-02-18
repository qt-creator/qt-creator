// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acpinspector.h"
#include "acpclienttr.h"

#include <coreplugin/icore.h>
#include <coreplugin/minisplitter.h>

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

namespace AcpClient::Internal {

// --- JSON tree helpers (same pattern as LspInspector) ---

class JsonTreeItemDelegate : public QStyledItemDelegate
{
public:
    QString displayText(const QVariant &value, const QLocale &) const override
    {
        QString result = value.toString();
        if (result.size() == 1) {
            switch (result.at(0).toLatin1()) {
            case '\n': return QString("\\n");
            case '\t': return QString("\\t");
            case '\r': return QString("\\r");
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

    void setMessage(const AcpLogMessage &message)
    {
        m_jsonTree->setModel(createJsonModel("content", message.message));
    }

    void clear()
    {
        m_jsonTree->setModel(createJsonModel("", QJsonObject()));
    }

private:
    QTreeView *m_jsonTree = nullptr;
};

// --- AcpLogWidget ---

static QVariant messageData(const AcpLogMessage &message, int, int role)
{
    if (role == Qt::DisplayRole)
        return message.displayText();
    if (role == Qt::TextAlignmentRole)
        return message.sender == AcpLogMessage::ClientMessage ? Qt::AlignLeft : Qt::AlignRight;
    return {};
}

class AcpLogWidget : public Core::MiniSplitter
{
public:
    AcpLogWidget()
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
                &AcpLogWidget::currentMessageChanged);
    }

    void addMessage(const AcpLogMessage &message)
    {
        m_model.appendItem(message);
    }

    void setMessages(const std::list<AcpLogMessage> &messages)
    {
        m_model.clear();
        for (const AcpLogMessage &message : messages)
            m_model.appendItem(message);
    }

    void saveLog()
    {
        QString contents;
        QTextStream stream(&contents);
        m_model.forAllData([&](const AcpLogMessage &message) {
            stream << message.time.toString("hh:mm:ss.zzz") << ' ';
            stream << (message.sender == AcpLogMessage::ClientMessage ? QString{"Client"}
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
        AcpLogMessage message = m_model.itemAt(index.row())->itemData;
        if (message.sender == AcpLogMessage::ClientMessage)
            m_clientDetails->setMessage(message);
        else
            m_serverDetails->setMessage(message);
        selectMatchingMessage(message);
    }

    void selectMatchingMessage(const AcpLogMessage &message)
    {
        QJsonValue msgId = message.id();
        if (msgId.isNull() || msgId.isUndefined())
            return;
        AcpLogMessage::MessageSender otherSender =
            message.sender == AcpLogMessage::ServerMessage
                ? AcpLogMessage::ClientMessage
                : AcpLogMessage::ServerMessage;
        AcpLogMessage *matchingMessage = m_model.findData(
            [&](const AcpLogMessage &msg) {
                return msg.sender == otherSender && msg.id() == msgId;
            });
        if (!matchingMessage)
            return;
        auto matchIndex = m_model.findIndex(
            [&](const AcpLogMessage &msg) { return &msg == matchingMessage; });

        m_messages->selectionModel()->select(matchIndex, QItemSelectionModel::Select);
        if (matchingMessage->sender == AcpLogMessage::ServerMessage)
            m_serverDetails->setMessage(*matchingMessage);
        else
            m_clientDetails->setMessage(*matchingMessage);
    }

    MessageDetailWidget *m_clientDetails = nullptr;
    QListView *m_messages = nullptr;
    MessageDetailWidget *m_serverDetails = nullptr;
    ListModel<AcpLogMessage> m_model;
};

// --- AcpInspectorWidget ---

class AcpInspectorWidget : public QDialog
{
public:
    explicit AcpInspectorWidget(AcpInspector *inspector)
        : m_inspector(inspector)
    {
        setWindowTitle(Tr::tr("ACP Inspector"));

        connect(inspector, &AcpInspector::newMessage, this, &AcpInspectorWidget::addMessage);
        connect(Core::ICore::instance(), &Core::ICore::coreAboutToClose, this, &QWidget::close);

        m_clients = new QComboBox;
        m_clients->addItem(Tr::tr("<Select>"));
        m_clients->addItems(inspector->clients());

        m_logWidget = new AcpLogWidget;

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
            Row { Tr::tr("ACP Client:"), m_clients, st },
            m_logWidget,
            buttonBox,
        }.attachTo(this);
        // clang-format on

        connect(m_clients, &QComboBox::currentTextChanged,
                this, &AcpInspectorWidget::currentClientChanged);

        connect(buttonBox, &QDialogButtonBox::accepted, m_logWidget, &AcpLogWidget::saveLog);
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
    void addMessage(const QString &clientName, const AcpLogMessage &message)
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

    AcpInspector *const m_inspector = nullptr;
    AcpLogWidget *m_logWidget = nullptr;
    QComboBox *m_clients = nullptr;
};

// --- AcpLogMessage ---

AcpLogMessage::AcpLogMessage(MessageSender sender, const QTime &time, const QJsonObject &message)
    : sender(sender)
    , time(time)
    , message(message)
{}

QJsonValue AcpLogMessage::id() const
{
    if (!m_id.has_value())
        m_id = message.value(QLatin1String("id"));
    return *m_id;
}

QString AcpLogMessage::displayText() const
{
    if (!m_displayText.has_value()) {
        m_displayText = time.toString("hh:mm:ss.zzz") + '\n';
        const QString method = message.value(QLatin1String("method")).toString();
        if (!method.isEmpty()) {
            m_displayText->append(method);
        } else {
            const QJsonValue msgId = id();
            if (msgId.isDouble())
                m_displayText->append(QString("Response #%1").arg(static_cast<qint64>(msgId.toDouble())));
            else if (msgId.isString())
                m_displayText->append(QString("Response #%1").arg(msgId.toString()));
            else
                m_displayText->append(Tr::tr("Unknown"));
        }
    }
    return *m_displayText;
}

// --- AcpInspector ---

void AcpInspector::show(const QString &defaultClient)
{
    if (!m_currentWidget) {
        auto widget = new AcpInspectorWidget(this);
        connect(widget, &AcpInspectorWidget::finished, this, &AcpInspector::onInspectorClosed);
        Core::ICore::registerWindow(widget, Core::Context("AcpClient.Inspector"));
        m_currentWidget = widget;
    } else {
        QApplication::setActiveWindow(m_currentWidget);
    }
    if (!defaultClient.isEmpty())
        static_cast<AcpInspectorWidget *>(m_currentWidget)->selectClient(defaultClient);
    m_currentWidget->show();
}

void AcpInspector::log(AcpLogMessage::MessageSender sender,
                       const QString &clientName,
                       const QJsonObject &message)
{
    std::list<AcpLogMessage> &clientLog = m_logs[clientName];
    while (clientLog.size() >= static_cast<std::size_t>(m_logSize))
        clientLog.pop_front();
    clientLog.push_back({sender, QTime::currentTime(), message});
    emit newMessage(clientName, clientLog.back());
}

std::list<AcpLogMessage> AcpInspector::messages(const QString &clientName) const
{
    return m_logs.value(clientName);
}

QStringList AcpInspector::clients() const
{
    return m_logs.keys();
}

void AcpInspector::onInspectorClosed()
{
    m_currentWidget->deleteLater();
    m_currentWidget = nullptr;
}

} // namespace AcpClient::Internal
