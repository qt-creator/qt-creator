// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "chatpanel.h"
#include "acpclienttr.h"
#include "acpmessageview.h"
#include "acpsettings.h"
#include "chatinputedit.h"
#include "sessionpickerwidget.h"

#include <coreplugin/findplaceholder.h>

#include <utils/fileutils.h>
#include <utils/qtdesignwidgets.h>
#include <utils/styledbar.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QApplication>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPaintEvent>
#include <QPainter>
#include <QPushButton>
#include <QStringList>
#include <QTextBlock>
#include <QTextCursor>
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

class ContextItem : public QWidget
{
    Q_OBJECT
public:
    explicit ContextItem(const QString &text, QWidget *parent = nullptr)
        : QWidget(parent)
    {
        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(PaddingHM, 0, 0, 0);
        layout->setSpacing(0);

        auto *label = new QLabel(text);
        layout->addWidget(label);

        auto *closeBtn = new QtcIconButton();
        closeBtn->setIcon(Icons::CLOSE_TOOLBAR.icon());
        closeBtn->setContentsMargins(0, 0, 0, 0);
        closeBtn->setToolTip(Tr::tr("Remove Context"));
        connect(closeBtn, &QToolButton::clicked, this, &ContextItem::removeRequested);
        layout->addWidget(closeBtn);
    }

signals:
    void removeRequested();

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        StyleHelper::drawCardBg(
            &p, rect(), creatorColor(Theme::Token_Foreground_Subtle), QPen(Qt::NoPen), RadiusS);
    }
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
    toolbarLayout->setContentsMargins(PaddingHM, 0, PaddingHM, 0);
    toolbarLayout->setSpacing(GapHM);

    m_agentLabel = new QLabel;
    m_agentLabel->setTextFormat(Qt::RichText);
    toolbarLayout->addWidget(m_agentLabel);

    m_configButton = new QToolButton;
    m_configButton->setIcon(Icons::SETTINGS_TOOLBAR.icon());
    m_configButton->setToolTip(Tr::tr("Configuration Options"));
    m_configButton->setAutoRaise(true);
    m_configButton->hide();
    connect(m_configButton, &QToolButton::clicked, this, &ChatPanel::showConfigMenu);
    toolbarLayout->addStretch(1);
    toolbarLayout->addWidget(m_configButton);

    // --- Message view ---
    m_messageView = new AcpMessageView;
    m_messageView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_messageView, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        QMenu menu(m_messageView);
        QAction *action = menu.addAction(Tr::tr("Show Thoughts"));
        action->setCheckable(true);
        action->setChecked(m_messageView->thoughtsVisible());
        connect(action, &QAction::toggled, m_messageView, &AcpMessageView::setThoughtsVisible);
        menu.exec(m_messageView->mapToGlobal(pos));
    });
    layout->addWidget(m_messageView, 1);
    layout->addWidget(new Core::FindToolBarPlaceHolder(m_messageView));
    layout->addWidget(toolbar);

    // --- Input bar (rounded container) ---
    auto *inputOuter = new QWidget;
    auto *inputOuterLayout = new QVBoxLayout(inputOuter);
    inputOuterLayout->setContentsMargins(PaddingHM, PaddingVXs, PaddingHM, PaddingVM);
    inputOuterLayout->setSpacing(GapVXs);

    auto *inputContainer = new InputContainerWidget;
    auto *inputContainerLayout = new QVBoxLayout(inputContainer);
    inputContainerLayout->setContentsMargins(PaddingHM, PaddingVXs, PaddingHM, PaddingVXs);
    inputContainerLayout->setSpacing(GapVXs);

    m_inputEdit = new ChatInputEdit;
    inputContainerLayout->addWidget(m_inputEdit);

    auto *bottomRow = new QWidget;
    auto *bottomRowLayout = new QHBoxLayout(bottomRow);
    bottomRowLayout->setContentsMargins(0, 0, 0, 0);
    bottomRowLayout->setSpacing(GapHS);

    m_contextBar = new QWidget;
    Layouting::Flow{}.attachTo(m_contextBar);
    m_contextBarLayout = m_contextBar->layout();
    m_contextBarLayout->setContentsMargins(0, 0, 0, 0);
    m_contextBarLayout->setSpacing(GapHS);

    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged, this, [this] {
        updateContextBar();
    });

    QVBoxLayout *verticalCenter = new QVBoxLayout;
    verticalCenter->addStretch();
    verticalCenter->addWidget(m_contextBar);
    verticalCenter->addStretch();
    bottomRowLayout->addLayout(verticalCenter, 1);

    auto addContextButton = new QtcIconButton;
    addContextButton->setIcon(Utils::Icons::PAPERCLIP.icon());
    addContextButton->setToolTip(Tr::tr("Add Context"));
    connect(addContextButton, &QtcIconButton::released, this, [this, addContextButton] {
        auto menu = new QMenu(addContextButton);
        menu->setAttribute(Qt::WA_DeleteOnClose);

        menu->clear();

        auto *addFileAction = menu->addAction(Tr::tr("Add File..."));
        connect(addFileAction, &QAction::triggered, this, [this] {
            const FilePath fp = Utils::FileUtils::getOpenFilePath(
                Tr::tr("Add Context File"), {}, {}, nullptr, {}, false, false);
            if (fp.isEmpty())
                return;
            if (!m_manualContextFiles.contains(fp)) {
                m_manualContextFiles.append(fp);
                updateContextBar();
            }
        });
        auto *addRemoteFileAction = menu->addAction(Tr::tr("Add Remote File..."));
        connect(addRemoteFileAction, &QAction::triggered, this, [this] {
            const FilePath fp = Utils::FileUtils::getOpenFilePath(
                Tr::tr("Add Context File"), {}, {}, nullptr, {}, false, true);
            if (fp.isEmpty())
                return;
            if (!m_manualContextFiles.contains(fp)) {
                m_manualContextFiles.append(fp);
                updateContextBar();
            }
        });

        if (!m_includeCurrentEditorContext) {
            menu->addSeparator();
            auto *action = menu->addAction(Tr::tr("Current Editor"));
            connect(action, &QAction::triggered, this, [this] {
                m_includeCurrentEditorContext = true;
                updateContextBar();
            });
        }

        menu->popup(QCursor::pos());
    });

    auto bottomRowButtonLayout = new QHBoxLayout;
    bottomRowButtonLayout->addWidget(addContextButton);

    m_commandsButton = new QtcIconButton;
    m_commandsButton->setIcon(Icons::SLASH.icon());
    m_commandsButton->setToolTip(Tr::tr("Insert Command"));
    m_commandsButton->hide();
    connect(m_commandsButton, &QAbstractButton::clicked, this, [this] {
        if (!m_commandsMenu)
            return;
        const QPoint topLeft = m_commandsButton->mapToGlobal(QPoint(0, 0));
        const QSize menuSize = m_commandsMenu->sizeHint();
        m_commandsMenu->popup(QPoint(topLeft.x(), topLeft.y() - menuSize.height()));
    });
    bottomRowButtonLayout->addWidget(m_commandsButton);

    m_sendButton = new QtcButton(Tr::tr("Send"), QtcButton::MediumPrimary);
    m_sendButton->setEnabled(false);
    bottomRowButtonLayout->addWidget(m_sendButton);
    auto alignTop = new QVBoxLayout;
    alignTop->addLayout(bottomRowButtonLayout);
    alignTop->addStretch();
    bottomRowLayout->addLayout(alignTop);

    inputOuterLayout->addWidget(inputContainer);
    inputOuterLayout->addWidget(bottomRow);
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

    updateContextBar();
}

void ChatPanel::setAgentInfo(const QString &name, const QString &version,
                             const QString &iconUrl)
{
    m_messageView->setAgentIconUrl(iconUrl);

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
    m_sendButton->setRole(prompting ? QtcButton::MediumTertiary : QtcButton::MediumPrimary);
    m_inputEdit->setEnabled(!prompting);
    m_messageView->setPrompting(prompting);
}

void ChatPanel::setSendEnabled(bool enabled)
{
    m_sendButton->setEnabled(enabled);
}

void ChatPanel::showConfigMenu()
{
    auto *menu = new QMenu(m_configButton);
    menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->setToolTipsVisible(true);
    for (const SessionConfigOption &option : std::as_const(m_configOptions)) {
        auto select = fromJson<SessionConfigSelect>(QJsonValue(toJson(option)));
        if (!select)
            continue;

        const QString id = option.id();
        const QString currentValue = select->currentValue();

        QMenu *sub = menu->addMenu(option.name());

        const auto addOption = [&](const SessionConfigSelectOption &opt, const QString &prefix) {
            QAction *a = sub->addAction(prefix + opt.name());
            if (auto tooltip = opt.description())
                a->setToolTip(*tooltip);
            a->setCheckable(true);
            a->setChecked(opt.value() == currentValue);
            const QString value = opt.value();
            connect(a, &QAction::triggered, this, [this, id, value] {
                emit configOptionChanged(id, value);
            });
        };

        const auto &opts = select->options();
        if (auto *flatOptions = std::get_if<QList<SessionConfigSelectOption>>(&opts)) {
            for (const SessionConfigSelectOption &opt : *flatOptions)
                addOption(opt, {});
        } else if (auto *groups = std::get_if<QList<SessionConfigSelectGroup>>(&opts)) {
            for (const SessionConfigSelectGroup &group : *groups) {
                QAction *header = sub->addAction(QStringLiteral("— %1 —").arg(group.name()));
                header->setEnabled(false);
                for (const SessionConfigSelectOption &opt : group.options())
                    addOption(opt, QStringLiteral("  "));
            }
        }
    }
    menu->popup(QCursor::pos());
}

void ChatPanel::setConfigOptions(const QList<SessionConfigOption> &configOptions)
{
    m_configOptions = configOptions;
    m_configButton->setVisible(!m_configOptions.isEmpty());
}

void ChatPanel::clear()
{
    m_messageView->clear();
}

void ChatPanel::clearConfigOptions()
{
    m_configOptions.clear();
    m_configButton->setVisible(false);
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
    QApplication::alert(m_messageView);

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

SessionPickerWidget *ChatPanel::addSessionPicker()
{
    return m_messageView->addSessionPicker();
}

void ChatPanel::updateAvailableCommands(const QList<AvailableCommand> &commands)
{
    QList<CommandInfo> commandInfos;
    commandInfos.reserve(commands.size());
    for (const AvailableCommand &cmd : commands) {
        const QString hint = cmd.input() ? Acp::hint(*cmd.input()) : QString();
        commandInfos.append({cmd.name(), cmd.description(), hint});
    }
    m_inputEdit->setAvailableCommands(commandInfos);

    delete m_commandsMenu;
    m_commandsMenu = nullptr;
    m_commandsButton->setVisible(!commands.isEmpty());

    if (commands.isEmpty())
        return;

    m_commandsMenu = new QMenu(m_commandsButton);
    m_commandsMenu->setToolTipsVisible(true);
    for (const AvailableCommand &cmd : commands) {
        QAction *action = m_commandsMenu->addAction(cmd.name());
        action->setToolTip(cmd.description());
        connect(action, &QAction::triggered, this, [this, cmd] {
            m_inputEdit->setPlainText('/' + cmd.name() + QLatin1Char(' '));
            m_inputEdit->moveCursor(QTextCursor::End);
            m_inputEdit->setFocus();
        });
    }
}

void ChatPanel::updateContextBar()
{
    while (QLayoutItem *item = m_contextBarLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    if (m_includeCurrentEditorContext) {
        if (auto editor = TextEditor::BaseTextEditor::currentTextEditor()) {
            Utils::FilePath filePath = editor->document()->filePath();
            if (!filePath.isEmpty()) {
                const QString name = filePath.fileName();
                auto *item = new ContextItem(name, m_contextBar);
                connect(item, &ContextItem::removeRequested, this, [this, name] {
                    m_includeCurrentEditorContext = false;
                    QMetaObject::invokeMethod(this, [this] { updateContextBar(); }, Qt::QueuedConnection);
                });
                m_contextBarLayout->addWidget(item);
            }
        }
    }

    for (const Utils::FilePath &file : std::as_const(m_manualContextFiles)) {
        auto *item = new ContextItem(file.fileName(), m_contextBar);
        connect(item, &ContextItem::removeRequested, this, [this, file] {
            m_manualContextFiles.removeOne(file);
            QMetaObject::invokeMethod(this, [this] { updateContextBar(); }, Qt::QueuedConnection);
        });
        m_contextBarLayout->addWidget(item);
    }
}

} // namespace AcpClient::Internal

#include "chatpanel.moc"
