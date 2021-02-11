/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "lspinspector.h"

#include <coreplugin/icore.h>
#include <coreplugin/minisplitter.h>
#include <languageserverprotocol/jsonkeys.h>
#include <languageserverprotocol/jsonrpcmessages.h>
#include <utils/jsontreeitem.h>
#include <utils/listmodel.h>

#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QStyledItemDelegate>
#include <QTextCodec>
#include <QTreeView>

using namespace LanguageServerProtocol;

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
    auto action = new QAction(LspInspector::tr("Expand All"), view);
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

    void setMessage(const BaseMessage &message);
    void clear();

private:
    QLabel *m_contentLength = nullptr;
    QLabel *m_mimeType = nullptr;
};

class LspCapabilitiesWidget : public QWidget
{
    Q_DECLARE_TR_FUNCTIONS(LspCapabilitiesWidget)
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

    auto group = new QGroupBox(tr("Capabilities:"));
    QLayout *layout = new QHBoxLayout;
    m_capabilitiesView = createJsonTreeView();
    layout->addWidget(m_capabilitiesView);
    group->setLayout(layout);
    mainLayout->addWidget(group);

    m_dynamicCapabilitiesGroup = new QGroupBox(tr("Dynamic Capabilities:"));
    layout = new QVBoxLayout;
    auto label = new QLabel(tr("Method:"));
    layout->addWidget(label);
    m_dynamicCapabilitiesView = new QListWidget();
    layout->addWidget(m_dynamicCapabilitiesView);
    label = new QLabel(tr("Options:"));
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
        createJsonModel(tr("Server Capabilities"), QJsonObject(serverCapabilities.capabilities)));
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
    void selectMatchingMessage(LspLogMessage::MessageSender sender, const QJsonValue &id);
};

static QVariant messageData(const LspLogMessage &message, int, int role)
{
    if (role == Qt::DisplayRole) {
        QString result = message.time.toString("hh:mm:ss.zzz") + '\n';
        if (message.message.mimeType == JsonRpcMessageHandler::jsonRpcMimeType()) {
            QString error;
            auto json = JsonRpcMessageHandler::toJsonObject(message.message.content,
                                                            message.message.codec,
                                                            error);
            result += json.value(QString{methodKey}).toString(json.value(QString{idKey}).toString());
        } else {
            result += message.message.codec->toUnicode(message.message.content);
        }
        return result;
    }
    if (role == Qt::TextAlignmentRole)
        return message.sender == LspLogMessage::ClientMessage ? Qt::AlignLeft : Qt::AlignRight;
    return {};
}

LspLogWidget::LspLogWidget()
{
    setOrientation(Qt::Horizontal);

    m_clientDetails = new MessageDetailWidget;
    m_clientDetails->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    m_clientDetails->setTitle(tr("Client Message"));
    addWidget(m_clientDetails);
    setStretchFactor(0, 1);

    m_model.setDataAccessor(&messageData);
    m_messages = new QListView;
    m_messages->setModel(&m_model);
    m_messages->setAlternatingRowColors(true);
    m_model.setHeader({tr("Messages")});
    m_messages->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
    m_messages->setSelectionMode(QAbstractItemView::MultiSelection);
    addWidget(m_messages);
    setStretchFactor(1, 0);

    m_serverDetails = new MessageDetailWidget;
    m_serverDetails->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    m_serverDetails->setTitle(tr("Server Message"));
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
    if (!index.isValid())
        return;
    LspLogMessage selectedMessage = m_model.itemAt(index.row())->itemData;
    BaseMessage message = selectedMessage.message;
    if (selectedMessage.sender == LspLogMessage::ClientMessage)
        m_clientDetails->setMessage(message);
    else
        m_serverDetails->setMessage(message);
    if (message.mimeType == JsonRpcMessageHandler::jsonRpcMimeType()) {
        QString error;
        QJsonValue id = JsonRpcMessageHandler::toJsonObject(message.content, message.codec, error)
                            .value(idKey);
        if (!id.isUndefined()) {
            selectMatchingMessage(selectedMessage.sender == LspLogMessage::ClientMessage
                                      ? LspLogMessage::ServerMessage
                                      : LspLogMessage::ClientMessage,
                                  id);
        }
    }
}

static bool matches(LspLogMessage::MessageSender sender,
                    const QJsonValue &id,
                    const LspLogMessage &message)
{
    if (message.sender != sender)
        return false;
    if (message.message.mimeType != JsonRpcMessageHandler::jsonRpcMimeType())
        return false;
    QString error;
    auto json = JsonRpcMessageHandler::toJsonObject(message.message.content,
                                                    message.message.codec,
                                                    error);
    return json.value(QString{idKey}) == id;
}

void LspLogWidget::selectMatchingMessage(LspLogMessage::MessageSender sender, const QJsonValue &id)
{
    LspLogMessage *matchingMessage = m_model.findData(
        [&](const LspLogMessage &message) { return matches(sender, id, message); });
    if (!matchingMessage)
        return;
    auto index = m_model.findIndex(
        [&](const LspLogMessage &message) { return &message == matchingMessage; });

    m_messages->selectionModel()->select(index, QItemSelectionModel::Select);
    if (matchingMessage->sender == LspLogMessage::ServerMessage)
        m_serverDetails->setMessage(matchingMessage->message);
    else
        m_clientDetails->setMessage(matchingMessage->message);
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
        stream << message.message.codec->toUnicode(message.message.content);
        stream << "\n\n";
    });

    const QString fileName = QFileDialog::getSaveFileName(this, tr("Log File"));
    if (fileName.isEmpty())
        return;
    Utils::FileSaver saver(fileName, QIODevice::Text);
    saver.write(contents.toUtf8());
    if (!saver.finalize(this))
        saveLog();
}

class LspInspectorWidget : public QDialog
{
    Q_DECLARE_TR_FUNCTIONS(LspInspectorWidget)
public:
    explicit LspInspectorWidget(LspInspector *inspector);

    void selectClient(const QString &clientName);
private:
    void addMessage(const QString &clientName, const LspLogMessage &message);
    void updateCapabilities(const QString &clientName);
    void currentClientChanged(const QString &clientName);

    LspInspector *m_inspector = nullptr;
    LspLogWidget *m_log = nullptr;
    LspCapabilitiesWidget *m_capabilities = nullptr;
    QListWidget *m_clients = nullptr;
};

QWidget *LspInspector::createWidget(const QString &defaultClient)
{
    auto *inspector = new LspInspectorWidget(this);
    inspector->selectClient(defaultClient);
    return inspector;
}

void LspInspector::log(const LspLogMessage::MessageSender sender,
                       const QString &clientName,
                       const BaseMessage &message)
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

LspInspectorWidget::LspInspectorWidget(LspInspector *inspector)
    : m_inspector(inspector)
{
    setWindowTitle(tr("Language Client Inspector"));

    connect(inspector, &LspInspector::newMessage, this, &LspInspectorWidget::addMessage);
    connect(inspector, &LspInspector::capabilitiesUpdated,
            this, &LspInspectorWidget::updateCapabilities);
    connect(Core::ICore::instance(), &Core::ICore::coreAboutToClose, this, &QWidget::close);

    m_clients = new QListWidget;
    m_clients->addItems(inspector->clients());
    m_clients->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::MinimumExpanding);

    auto tabWidget = new QTabWidget;

    auto mainLayout = new QVBoxLayout;
    auto mainSplitter = new Core::MiniSplitter;
    mainSplitter->setOrientation(Qt::Horizontal);
    mainSplitter->addWidget(m_clients);
    mainSplitter->addWidget(tabWidget);
    mainSplitter->setStretchFactor(0, 0);
    mainSplitter->setStretchFactor(1, 1);
    m_log = new LspLogWidget;
    m_capabilities = new LspCapabilitiesWidget;
    tabWidget->addTab(m_log, tr("Log"));
    tabWidget->addTab(m_capabilities, tr("Capabilities"));
    mainLayout->addWidget(mainSplitter);

    auto buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Save | QDialogButtonBox::Close);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    connect(m_clients,
            &QListWidget::currentTextChanged,
            this,
            &LspInspectorWidget::currentClientChanged);

    // save
    connect(buttonBox, &QDialogButtonBox::accepted, m_log, &LspLogWidget::saveLog);

    // close
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    resize(1024, 768);
}

void LspInspectorWidget::selectClient(const QString &clientName)
{
    auto items = m_clients->findItems(clientName, Qt::MatchExactly);
    if (items.isEmpty())
        return;
    m_clients->setCurrentItem(items.first());
}

void LspInspectorWidget::addMessage(const QString &clientName, const LspLogMessage &message)
{
    if (m_clients->findItems(clientName, Qt::MatchExactly).isEmpty())
        m_clients->addItem(clientName);
    if (clientName == m_clients->currentItem()->text())
        m_log->addMessage(message);
}

void LspInspectorWidget::updateCapabilities(const QString &clientName)
{
    if (m_clients->findItems(clientName, Qt::MatchExactly).isEmpty())
        m_clients->addItem(clientName);
    if (clientName != m_clients->currentItem()->text())
        m_capabilities->setCapabilities(m_inspector->capabilities(clientName));
}

void LspInspectorWidget::currentClientChanged(const QString &clientName)
{
    m_log->setMessages(m_inspector->messages(clientName));
    m_capabilities->setCapabilities(m_inspector->capabilities(clientName));
}

MessageDetailWidget::MessageDetailWidget()
{
    auto layout = new QFormLayout;
    setLayout(layout);

    m_contentLength = new QLabel;
    m_mimeType = new QLabel;

    layout->addRow("Content Length:", m_contentLength);
    layout->addRow("MIME Type:", m_mimeType);
}

void MessageDetailWidget::setMessage(const BaseMessage &message)
{
    m_contentLength->setText(QString::number(message.contentLength));
    m_mimeType->setText(QString::fromLatin1(message.mimeType));

    QWidget *newContentWidget = nullptr;
    if (message.mimeType == JsonRpcMessageHandler::jsonRpcMimeType()) {
        QString error;
        auto json = JsonRpcMessageHandler::toJsonObject(message.content, message.codec, error);
        if (json.isEmpty())
            newContentWidget = new QLabel(error);
        else
            newContentWidget = createJsonTreeView("content", json);
    } else {
        auto edit = new QPlainTextEdit();
        edit->setReadOnly(true);
        edit->setPlainText(message.codec->toUnicode(message.content));
        newContentWidget = edit;
    }
    auto formLayout = static_cast<QFormLayout *>(layout());
    if (formLayout->rowCount() > 2)
        formLayout->removeRow(2);
    formLayout->setWidget(2, QFormLayout::SpanningRole, newContentWidget);
}

void MessageDetailWidget::clear()
{
    m_contentLength->setText({});
    m_mimeType->setText({});
    auto formLayout = static_cast<QFormLayout *>(layout());
    if (formLayout->rowCount() > 2)
        formLayout->removeRow(2);
}

} // namespace LanguageClient
