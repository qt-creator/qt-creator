// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "chatpanel.h"
#include "acpclienttr.h"
#include "acpmessageview.h"
#include "chatinputedit.h"

#include <utils/elidinglabel.h>
#include <utils/progressindicator.h>
#include <utils/styledbar.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

#include <QComboBox>
#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPaintEvent>
#include <QPainter>
#include <QPushButton>
#include <QTextBlock>
#include <QTextCursor>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

using namespace Acp;
using namespace Utils;
using namespace Utils::StyleHelper::SpacingTokens;

namespace AcpClient::Internal {

class InputContainerWidget : public QWidget
{
public:
    explicit InputContainerWidget(QWidget *parent = nullptr) : QWidget(parent) {}

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        StyleHelper::drawCardBg(
            &p,
            rect(),
            palette().color(QPalette::Base),
            QPen(palette().color(QPalette::Mid), 1),
            RadiusM);
    }
};

class SendButton : public QPushButton
{
public:
    explicit SendButton(const QString &text, QWidget *parent = nullptr)
        : QPushButton(text, parent)
    {
        setContentsMargins(12, 4, 12, 4);
    }

    void setPrompting(bool prompting)
    {
        m_prompting = prompting;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QColor bg, fg;
        if (m_prompting) {
            bg = underMouse() ? QColor{0xaa, 0x22, 0x22} : QColor{0xcc, 0x33, 0x33};
            fg = Qt::white;
        } else if (!isEnabled()) {
            bg = palette().color(QPalette::Disabled, QPalette::Mid);
            fg = palette().color(QPalette::Disabled, QPalette::Midlight);
        } else {
            bg = palette().color(QPalette::Highlight);
            fg = palette().color(QPalette::HighlightedText);
        }
        StyleHelper::drawCardBg(&p, rect(), bg, QPen(Qt::NoPen), RadiusS);
        p.setPen(fg);
        p.drawText(rect(), Qt::AlignCenter, text());
    }

private:
    bool m_prompting = false;
};

ChatPanel::ChatPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // --- Session toolbar (StyledBar) ---
    auto *toolbar = new StyledBar;
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(PaddingHM, PaddingVXs, PaddingHM, PaddingVXs);
    toolbarLayout->setSpacing(GapHM);

    m_progressIndicator = new ProgressIndicator(ProgressIndicatorSize::Small, toolbar);
    m_progressIndicator->hide();
    m_progressIndicator->setSizePolicy(QSizePolicy::Fixed,
                                       m_progressIndicator->sizePolicy().verticalPolicy());
    toolbarLayout->addWidget(m_progressIndicator);

    m_agentLabel = new QLabel;
    m_agentLabel->setTextFormat(Qt::RichText);
    toolbarLayout->addWidget(m_agentLabel);

    m_configOptionsLayout = new QHBoxLayout;
    m_configOptionsLayout->setSpacing(GapHS);
    toolbarLayout->addLayout(m_configOptionsLayout);

    toolbarLayout->addStretch(1);

    m_elapsedLabel = new QLabel;
    m_elapsedLabel->setVisible(false);
    QFont elapsedFont = m_elapsedLabel->font();
    elapsedFont.setPointSizeF(elapsedFont.pointSizeF() * 0.9);
    elapsedFont.setFamily(QStringLiteral("monospace"));
    m_elapsedLabel->setFont(elapsedFont);
    toolbarLayout->addWidget(m_elapsedLabel);

    auto *disconnectButton = new QPushButton;
    disconnectButton->setText(Tr::tr("Disconnect"));
    connect(disconnectButton, &QPushButton::clicked, this, &ChatPanel::disconnectRequested);
    toolbarLayout->addWidget(disconnectButton);

    // --- Message view ---
    m_messageView = new AcpMessageView;
    m_messageView->setDetailedMode(true);
    layout->addWidget(m_messageView, 1);

    layout->addWidget(toolbar);

    // Elapsed time timer
    m_elapsedTimer = new QTimer(this);
    m_elapsedTimer->setInterval(100);
    connect(m_elapsedTimer, &QTimer::timeout, this, [this] {
        const qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - m_promptStartTime;
        const int secs = static_cast<int>(elapsed / 1000);
        const int tenths = static_cast<int>((elapsed % 1000) / 100);
        m_elapsedLabel->setText(QStringLiteral("%1.%2s").arg(secs).arg(tenths));
    });

    // --- Input bar (rounded container) ---
    auto *inputOuter = new QWidget;
    auto *inputOuterLayout = new QHBoxLayout(inputOuter);
    inputOuterLayout->setContentsMargins(PaddingHM, PaddingVXs, PaddingHM, PaddingVM);

    auto *inputContainer = new InputContainerWidget;
    auto *inputLayout = new QHBoxLayout(inputContainer);
    inputLayout->setContentsMargins(PaddingHM, PaddingVXs, PaddingVXs, PaddingVXs);
    inputLayout->setSpacing(GapHS);

    m_inputEdit = new ChatInputEdit;
    inputLayout->addWidget(m_inputEdit, 1);

    m_commandsButton = new QToolButton;
    m_commandsButton->setText(Tr::tr("/"));
    m_commandsButton->setToolTip(Tr::tr("Insert Command"));
    m_commandsButton->setPopupMode(QToolButton::InstantPopup);
    m_commandsButton->setAutoRaise(true);
    m_commandsButton->hide();
    inputLayout->addWidget(m_commandsButton);

    m_sendButton = new SendButton(Tr::tr("Send"));
    m_sendButton->setEnabled(false);
    inputLayout->addWidget(m_sendButton);

    inputOuterLayout->addWidget(inputContainer);
    layout->addWidget(inputOuter);

    // --- Connections ---
    connect(m_inputEdit, &ChatInputEdit::sendRequested, this, [this] {
        const QString text = m_inputEdit->toPlainText().trimmed();
        if (!text.isEmpty()) {
            m_inputEdit->clear();
            emit sendRequested(text);
        }
    });

    connect(m_sendButton, &QPushButton::clicked, this, [this] {
        if (m_prompting) {
            emit cancelRequested();
        } else {
            const QString text = m_inputEdit->toPlainText().trimmed();
            if (!text.isEmpty()) {
                m_inputEdit->clear();
                emit sendRequested(text);
            }
        }
    });

    // Config option combo connections are made dynamically in updateConfigOptions()
}

void ChatPanel::setAgentInfo(const QString &name, const QString &version)
{
    QString html;
    if (!name.isEmpty()) {
        html = QStringLiteral("<b>%1</b>").arg(name.toHtmlEscaped());
        if (!version.isEmpty())
            html += QStringLiteral(" <small>v%1</small>").arg(version.toHtmlEscaped());
    }
    m_agentLabel->setText(html);
}

void ChatPanel::setPrompting(bool prompting)
{
    m_prompting = prompting;
    m_sendButton->setText(prompting ? Tr::tr("Cancel") : Tr::tr("Send"));
    m_sendButton->setPrompting(prompting);
    m_inputEdit->setEnabled(!prompting);

    if (prompting) {
        m_promptStartTime = QDateTime::currentMSecsSinceEpoch();
        m_progressIndicator->show();
        m_elapsedLabel->setVisible(true);
        m_elapsedTimer->start();
    } else {
        m_progressIndicator->hide();
        m_elapsedTimer->stop();
        if (m_promptStartTime > 0) {
            const qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - m_promptStartTime;
            const int secs = static_cast<int>(elapsed / 1000);
            const int tenths = static_cast<int>((elapsed % 1000) / 100);
            m_elapsedLabel->setText(Tr::tr("Last: %1.%2s").arg(secs).arg(tenths));
        }
    }
}

void ChatPanel::setSendEnabled(bool enabled)
{
    m_sendButton->setEnabled(enabled);
}

void ChatPanel::updateConfigOptions(const QList<SessionConfigOption> &configOptions)
{
    for (const SessionConfigOption &option : configOptions) {
        auto select = fromJson<SessionConfigSelect>(QJsonValue(toJson(option)));
        if (!select)
            continue;

        const QString configId = option.id();

        // Find or create a combo for this config option
        QComboBox *combo = m_configCombos.value(configId);
        if (!combo) {
            auto *label = new ElidingLabel(option.name() + QLatin1Char(':'));
            label->setSizePolicy(QSizePolicy::Preferred, label->sizePolicy().verticalPolicy());
            label->setMinimumWidth(0);
            m_configOptionsLayout->addWidget(label);
            combo = new QComboBox;
            combo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
            combo->setMinimumContentsLength(3);
            QSizePolicy policy = combo->sizePolicy();
            policy.setHorizontalPolicy(QSizePolicy::Preferred);
            combo->setSizePolicy(policy);

            if (option.description().has_value())
                combo->setToolTip(*option.description());
            m_configOptionsLayout->addWidget(combo);
            m_configCombos.insert(configId, combo);

            connect(combo, &QComboBox::activated, this, [this, configId, combo](int index) {
                const QString value = combo->itemData(index).toString();
                if (!value.isEmpty())
                    emit configOptionChanged(configId, value);
            });
        }

        const QSignalBlocker blocker(combo);
        combo->clear();

        int currentIndex = 0;
        const QString currentValue = select->currentValue();

        const auto &opts = select->options();
        if (auto *flatOptions = std::get_if<QList<SessionConfigSelectOption>>(&opts)) {
            for (const SessionConfigSelectOption &opt : *flatOptions) {
                combo->addItem(opt.name(), opt.value());
                if (opt.value() == currentValue)
                    currentIndex = combo->count() - 1;
                if (auto tooltip = opt.description())
                    combo->setItemData(combo->count() - 1, *tooltip, Qt::ToolTipRole);
            }
        } else if (auto *groups = std::get_if<QList<SessionConfigSelectGroup>>(&opts)) {
            for (const SessionConfigSelectGroup &group : *groups) {
                combo->addItem(QStringLiteral("\u2014 %1 \u2014").arg(group.name()));
                combo->setItemData(combo->count() - 1, 0, Qt::UserRole - 1);
                for (const SessionConfigSelectOption &opt : group.options()) {
                    combo->addItem(QStringLiteral("  %1").arg(opt.name()), opt.value());
                    if (opt.value() == currentValue)
                        currentIndex = combo->count() - 1;
                    if (auto tooltip = opt.description())
                        combo->setItemData(combo->count() - 1, *tooltip, Qt::ToolTipRole);
                }
            }
        }

        combo->setCurrentIndex(currentIndex);
        combo->setVisible(combo->count() > 0);
    }
}

void ChatPanel::clearConfigOptions()
{
    qDeleteAll(m_configCombos);
    m_configCombos.clear();
    // Also remove any labels added to the config options layout
    while (QLayoutItem *item = m_configOptionsLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
}

void ChatPanel::addUserMessage(const QString &text)
{
    m_messageView->addUserMessage(text);
}

void ChatPanel::appendAgentText(const QString &text)
{
    m_messageView->appendAgentText(text);
}

void ChatPanel::appendAgentThought(const QString &text)
{
    m_messageView->appendAgentThought(text);
}

void ChatPanel::addToolCall(const ToolCall &toolCall)
{
    m_messageView->addToolCall(toolCall);
}

void ChatPanel::updateToolCall(const ToolCallUpdate &update)
{
    m_messageView->updateToolCall(update);
}

void ChatPanel::addPlan(const Plan &plan)
{
    m_messageView->addPlan(plan);
}

void ChatPanel::addErrorMessage(const QString &text)
{
    m_messageView->addErrorMessage(text);
}

void ChatPanel::finishAgentMessage()
{
    m_messageView->finishAgentMessage();
}

void ChatPanel::addPermissionRequest(const QJsonValue &id,
                                     const Acp::RequestPermissionRequest &request)
{
    m_messageView->addPermissionRequest(id, request);
    connect(m_messageView, &AcpMessageView::permissionOptionSelected,
            this, &ChatPanel::permissionOptionSelected, Qt::UniqueConnection);
    connect(m_messageView, &AcpMessageView::permissionCancelled,
            this, &ChatPanel::permissionCancelled, Qt::UniqueConnection);
}

void ChatPanel::addAuthenticationRequest(const QList<Acp::AuthMethod> &methods)
{
    m_messageView->addAuthenticationRequest(methods);
    connect(m_messageView, &AcpMessageView::authenticateRequested,
            this, &ChatPanel::authenticateRequested, Qt::UniqueConnection);
}

void ChatPanel::showAuthenticationError(const QString &error)
{
    m_messageView->showAuthenticationError(error);
}

void ChatPanel::resolveAuthentication()
{
    m_messageView->resolveAuthentication();
}

void ChatPanel::updateAvailableCommands(const QList<AvailableCommand> &commands)
{
    delete m_commandsButton->menu();
    m_commandsButton->setVisible(!commands.isEmpty());

    if (commands.isEmpty())
        return;

    auto *menu = new QMenu(m_commandsButton);
    menu->setToolTipsVisible(true);
    for (const AvailableCommand &cmd : commands) {
        QAction *action = menu->addAction(cmd.name());
        action->setToolTip(cmd.description());
        connect(action, &QAction::triggered, this, [this, cmd] {
            m_inputEdit->setPlainText('/' + cmd.name() + QLatin1Char(' '));
            m_inputEdit->moveCursor(QTextCursor::End);
            m_inputEdit->setFocus();
        });
    }
    m_commandsButton->setMenu(menu);
}

} // namespace AcpClient::Internal
