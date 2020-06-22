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

#include "lsplogger.h"

#include <coreplugin/icore.h>
#include <coreplugin/minisplitter.h>
#include <languageserverprotocol/jsonkeys.h>
#include <languageserverprotocol/jsonrpcmessages.h>
#include <utils/jsontreeitem.h>
#include <utils/listmodel.h>

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

class LspLoggerWidget : public QDialog
{
    Q_DECLARE_TR_FUNCTIONS(LspLoggerWidget)
public:
    explicit LspLoggerWidget(LspLogger *logger);

private:
    void addMessage(const QString &clientName, const LspLogMessage &message);
    void setCurrentClient(const QString &clientName);
    void currentMessageChanged(const QModelIndex &index);
    void selectMatchingMessage(LspLogMessage::MessageSender sender, const QJsonValue &id);
    void saveLog();

    LspLogger *m_logger = nullptr;
    QListWidget *m_clients = nullptr;
    MessageDetailWidget *m_clientDetails = nullptr;
    QListView *m_messages = nullptr;
    MessageDetailWidget *m_serverDetails = nullptr;
    Utils::ListModel<LspLogMessage> m_model;
};

QWidget *LspLogger::createWidget()
{
    return new LspLoggerWidget(this);
}

void LspLogger::log(const LspLogMessage::MessageSender sender,
                    const QString &clientName,
                    const BaseMessage &message)
{
    std::list<LspLogMessage> &clientLog = m_logs[clientName];
    while (clientLog.size() >= static_cast<std::size_t>(m_logSize))
        clientLog.pop_front();
    clientLog.push_back({sender, QTime::currentTime(), message});
    emit newMessage(clientName, clientLog.back());
}

std::list<LspLogMessage> LspLogger::messages(const QString &clientName) const
{
    return m_logs[clientName];
}

QList<QString> LspLogger::clients() const
{
    return m_logs.keys();
}

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

LspLoggerWidget::LspLoggerWidget(LspLogger *logger)
    : m_logger(logger)
{
    setWindowTitle(tr("Language Client Log"));

    connect(logger, &LspLogger::newMessage, this, &LspLoggerWidget::addMessage);
    connect(Core::ICore::instance(), &Core::ICore::coreAboutToClose, this, &QWidget::close);

    m_clients = new QListWidget;
    m_clients->addItems(logger->clients());
    connect(m_clients, &QListWidget::currentTextChanged, this, &LspLoggerWidget::setCurrentClient);
    m_clients->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::MinimumExpanding);

    m_clientDetails = new MessageDetailWidget;
    m_clientDetails->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    m_clientDetails->setTitle(tr("Client Message"));
    m_serverDetails = new MessageDetailWidget;
    m_serverDetails->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    m_serverDetails->setTitle(tr("Server Message"));

    m_model.setDataAccessor(&messageData);
    m_messages = new QListView;
    m_messages->setModel(&m_model);
    m_messages->setAlternatingRowColors(true);
    m_model.setHeader({tr("Messages")});
    connect(m_messages->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this,
            &LspLoggerWidget::currentMessageChanged);
    m_messages->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
    m_messages->setSelectionMode(QAbstractItemView::MultiSelection);

    auto layout = new QVBoxLayout;
    setLayout(layout);
    auto splitter = new Core::MiniSplitter;
    splitter->setOrientation(Qt::Horizontal);
    splitter->addWidget(m_clients);
    splitter->addWidget(m_clientDetails);
    splitter->addWidget(m_messages);
    splitter->addWidget(m_serverDetails);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 1);
    splitter->setStretchFactor(3, 1);
    layout->addWidget(splitter);

    auto buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Save | QDialogButtonBox::Close);
    layout->addWidget(buttonBox);

    // save
    connect(buttonBox, &QDialogButtonBox::accepted, this, &LspLoggerWidget::saveLog);

    // close
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    resize(1024, 768);
}

void LspLoggerWidget::addMessage(const QString &clientName, const LspLogMessage &message)
{
    if (m_clients->findItems(clientName, Qt::MatchExactly).isEmpty())
        m_clients->addItem(clientName);
    if (clientName != m_clients->currentItem()->text())
        return;
    m_model.appendItem(message);
}

void LspLoggerWidget::setCurrentClient(const QString &clientName)
{
    m_model.clear();
    for (const LspLogMessage &message : m_logger->messages(clientName))
        m_model.appendItem(message);
}

void LspLoggerWidget::currentMessageChanged(const QModelIndex &index)
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

void LspLoggerWidget::selectMatchingMessage(LspLogMessage::MessageSender sender,
                                            const QJsonValue &id)
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

void LspLoggerWidget::saveLog()
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

MessageDetailWidget::MessageDetailWidget()
{
    auto layout = new QFormLayout;
    setLayout(layout);

    m_contentLength = new QLabel;
    m_mimeType = new QLabel;

    layout->addRow("Content Length:", m_contentLength);
    layout->addRow("MIME Type:", m_mimeType);
}

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

void MessageDetailWidget::setMessage(const BaseMessage &message)
{
    m_contentLength->setText(QString::number(message.contentLength));
    m_mimeType->setText(QString::fromLatin1(message.mimeType));

    QWidget *newContentWidget = nullptr;
    if (message.mimeType == JsonRpcMessageHandler::jsonRpcMimeType()) {
        QString error;
        auto json = JsonRpcMessageHandler::toJsonObject(message.content, message.codec, error);
        if (json.isEmpty()) {
            newContentWidget = new QLabel(error);
        } else {
            auto root = new Utils::JsonTreeItem("content", json);
            if (root->canFetchMore())
                root->fetchMore();

            auto model = new Utils::TreeModel<Utils::JsonTreeItem>(root);
            model->setHeader({{"Name"}, {"Value"}, {"Type"}});
            auto view = new QTreeView;
            view->setModel(model);
            view->setAlternatingRowColors(true);
            view->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
            view->setItemDelegate(new JsonTreeItemDelegate);
            newContentWidget = view;
        }
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
