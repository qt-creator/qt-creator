// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "chatpanel.h"
#include "acpclienttr.h"
#include "acpmessageview.h"
#include "chatinputedit.h"
#include "sessionpickerwidget.h"

#include <coreplugin/coreicons.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/findplaceholder.h>

#include <texteditor/displaysettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/marginsettings.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditorconstants.h>

#include <utils/fileutils.h>
#include <utils/fsengine/fileiconprovider.h>
#include <utils/plaintextedit/texteditorlayout.h>
#include <utils/settingsdatabase.h>
#include <utils/layoutbuilder.h>
#include <utils/qtdesignwidgets.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>
#include <utils/widgets.h>

#include <QApplication>
#include <QCheckBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeData>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPushButton>
#include <QStringList>
#include <QTextBlock>
#include <QTextCursor>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>

using namespace Acp;
using namespace Utils;
using namespace Utils::StyleHelper::SpacingTokens;

namespace AcpClient::Internal {

static QList<FilePath> localFilesFromMime(const QMimeData *mime)
{
    QList<FilePath> files;
    if (!mime || !mime->hasUrls())
        return files;
    for (const QUrl &url : mime->urls()) {
        if (url.isLocalFile())
            files.append(FilePath::fromUserInput(url.toLocalFile()));
    }
    return files;
}

class InputContainerWidget : public QWidget
{
public:
    explicit InputContainerWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        setAcceptDrops(true);
    }

    std::function<void(const QList<FilePath> &)> onFilesDropped;

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (QWidget *proxy = focusProxy())
            proxy->setFocus(Qt::MouseFocusReason);
        QWidget::mousePressEvent(event);
    }

    void dragEnterEvent(QDragEnterEvent *event) override
    {
        if (!localFilesFromMime(event->mimeData()).isEmpty())
            event->acceptProposedAction();
        else
            QWidget::dragEnterEvent(event);
    }

    void dragMoveEvent(QDragMoveEvent *event) override
    {
        if (!localFilesFromMime(event->mimeData()).isEmpty())
            event->acceptProposedAction();
        else
            QWidget::dragMoveEvent(event);
    }

    void dropEvent(QDropEvent *event) override
    {
        const QList<FilePath> files = localFilesFromMime(event->mimeData());
        if (!files.isEmpty()) {
            event->acceptProposedAction();
            if (onFilesDropped)
                onFilesDropped(files);
            return;
        }
        QWidget::dropEvent(event);
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        StyleHelper::drawCardBg(
            &p,
            rect(),
            creatorColor(Theme::Token_Background_Subtle),
            creatorColor(Theme::Token_Foreground_Muted),
            RadiusM);
    }
};

class ContextItem : public QWidget
{
    Q_OBJECT
public:
    explicit ContextItem(const QString &text, const QIcon &icon, QWidget *parent = nullptr,
                         bool withBorder = true)
        : QWidget(parent)
        , m_withBorder(withBorder)
    {
        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(PaddingHM, PaddingVXs, PaddingHXs, PaddingVXs);
        layout->setSpacing(PaddingHXs);

        if (!icon.isNull()) {
            auto iconLabel = new QLabel;
            iconLabel->setPixmap(icon.pixmap(16, 16));
            layout->addWidget(iconLabel);
        }

        auto label = new QLabel(text);
        layout->addWidget(label);

        auto closeBtn = new QtcIconButton();
        closeBtn->setMaximumSize(20, 20);
        closeBtn->setIcon(Icons::CLOSE_TOOLBAR.icon());
        closeBtn->setContentsMargins(0, 0, 0, 0);
        closeBtn->setToolTip(Tr::tr("Remove Context"));
        connect(closeBtn, &QToolButton::clicked, this, &ContextItem::removeRequested);
        layout->addWidget(closeBtn);
        setCursor(Qt::PointingHandCursor);
    }

signals:
    void removeRequested();
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        // Emit queued: a handler may rebuild the context bar and delete this
        // widget, so the click must not run while we are still inside the
        // event (which would return into a deleted 'this').
        if (event->button() == Qt::LeftButton)
            QMetaObject::invokeMethod(this, &ContextItem::clicked, Qt::QueuedConnection);
        QWidget::mousePressEvent(event);
    }

    void paintEvent(QPaintEvent *) override
    {
        if (!m_withBorder)
            return;
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        StyleHelper::drawCardBg(
            &p, rect(), QBrush(), creatorColor(Theme::Token_Foreground_Muted), RadiusS);
    }

private:
    bool m_withBorder = true;
};

// Plain text editor with the same font/color setup as the chat input.
class ContextTextEdit : public TextEditor::TextEditorWidget
{
public:
    explicit ContextTextEdit(const QString &placeholder, QWidget *parent = nullptr)
        : TextEditor::TextEditorWidget(parent)
    {
        using namespace TextEditor;
        setupFallBackEditor(Utils::Id("AcpClient.ContextEdit"));
        setTabChangesFocus(true);
        setPlaceholderText(placeholder);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setFrameShape(QFrame::NoFrame);
        setHighlightCurrentLine(false);
        setLineNumbersVisible(false);
        setMarksVisible(false);
        setMinimapVisible(false);

        auto applyWidgetColors = [this] {
            FontSettingsData fontSettings = textDocument()->fontSettings();
            const QFont appFont = QApplication::font();
            fontSettings.setFamily(appFont.family());
            fontSettings.setFontSize(appFont.pointSize());
            fontSettings.setFontZoom(100);
            fontSettings.formatFor(C_TEXT).setForeground(creatorColor(Theme::Token_Text_Default));
            fontSettings.formatFor(C_TEXT).setBackground(creatorColor(Theme::Token_Background_Subtle));
            fontSettings.formatFor(C_DISABLED_CODE).setForeground(creatorColor(Theme::Token_Text_Subtle));
            fontSettings.formatFor(C_DISABLED_CODE).setBackground(creatorColor(Theme::Token_Background_Subtle));
            textDocument()->setFontSettings(fontSettings);
        };
        applyWidgetColors();
        connect(textDocument(), &TextEditor::TextDocument::fontSettingsChanged, this, applyWidgetColors);
    }

    // Pin the widget to a fixed number of text lines (used for the name editor).
    void setFixedLineCount(int lines)
    {
        const int docMargin = static_cast<int>(document()->documentMargin());
        setFixedHeight(fontMetrics().lineSpacing() * lines + 2 * docMargin);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    // Grow with the content between [minLines, maxLines] visible text lines.
    void setAutoHeightLineRange(int minLines, int maxLines)
    {
        m_minLines = minLines;
        m_maxLines = maxLines;
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        connect(this, &TextEditor::TextEditorWidget::textChanged, this, [this] { updateHeight(); });
        updateHeight();
    }

    void updateHeight()
    {
        if (m_maxLines <= 0)
            return;

        auto block = document()->firstBlock();
        int lineCount = 0;
        while (block.isValid() && lineCount < m_maxLines) {
            editorLayout()->ensureBlockLayout(block);
            lineCount += editorLayout()->blockLineCount(block);
            block = block.next();
        }
        lineCount = std::clamp(lineCount, m_minLines, m_maxLines);
        const bool limitHeight = lineCount >= m_maxLines;
        setVerticalScrollBarPolicy(limitHeight ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);

        const QMargins cm = contentsMargins();
        const int docMargin = static_cast<int>(document()->documentMargin());
        const int h = fontMetrics().lineSpacing() * lineCount
                      + cm.top() + cm.bottom() + 2 * docMargin + 2 * frameWidth();
        setFixedHeight(h);
    }

    void setDisplaySettings(const TextEditor::DisplaySettingsData &settings) override
    {
        TextEditor::DisplaySettingsData overridden = settings;
        overridden.m_visualizeWhitespace = false;
        overridden.m_visualizeIndent = false;
        overridden.m_textWrapping = true;
        overridden.m_scrollBarHighlights = false;
        overridden.m_centerCursorOnScroll = false;
        TextEditorWidget::setDisplaySettings(overridden);
    }

    void setMarginSettings(const TextEditor::MarginSettingsData &settings) override
    {
        TextEditor::MarginSettingsData overridden = settings;
        overridden.m_showMargin = false;
        overridden.m_useIndenter = false;
        TextEditorWidget::setMarginSettings(overridden);
    }

protected:
    int extraAreaWidth(int * = nullptr) const override { return 0; }

private:
    int m_minLines = 0;
    int m_maxLines = 0;
};

// Inline editor for a named plain-text context. Styled like the input
// container and shown above it while adding or modifying a text context.
class TextContextEditor : public QWidget
{
    Q_OBJECT
public:
    explicit TextContextEditor(QWidget *parent = nullptr) : QWidget(parent)
    {
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(PaddingHM, PaddingVM, PaddingHM, PaddingVM);
        layout->setSpacing(GapVXs);

        auto *nameRow = new QWidget;
        auto *nameLayout = new QHBoxLayout(nameRow);
        nameLayout->setContentsMargins(0, 0, 0, 0);
        nameLayout->setSpacing(GapHS);

        m_nameEdit = new ContextTextEdit(Tr::tr("Name"));
        m_nameEdit->setFixedLineCount(1);
        nameLayout->addWidget(m_nameEdit, 1);

        m_historyButton = new QtcIconButton;
        m_historyButton->setIcon(Utils::Icons::CLOCK_TOOLBAR.icon());
        m_historyButton->setToolTip(Tr::tr("Previously Entered Contexts"));
        connect(m_historyButton, &QAbstractButton::clicked, this, [this] { showHistoryMenu(); });
        nameLayout->addWidget(m_historyButton, 0, Qt::AlignTop);

        layout->addWidget(nameRow);

        auto *hr = new QFrame;
        hr->setFrameShape(QFrame::HLine);
        hr->setFrameShadow(QFrame::Plain);
        hr->setFixedHeight(1);
        QPalette pal = hr->palette();
        pal.setColor(QPalette::WindowText, creatorColor(Theme::Token_Foreground_Muted));
        hr->setPalette(pal);
        layout->addWidget(hr);

        m_textEdit = new ContextTextEdit(Tr::tr("Context"));
        m_textEdit->setAutoHeightLineRange(4, 12);
        layout->addWidget(m_textEdit);

        // Tab from the name field should reach the context text, not the
        // history button that follows the name field in the layout.
        setTabOrder(m_nameEdit, m_textEdit);

        auto *buttonRow = new QWidget;
        auto *buttonLayout = new QHBoxLayout(buttonRow);
        buttonLayout->setContentsMargins(0, 0, 0, 0);
        buttonLayout->setSpacing(GapHS);

        m_rememberCombo = new QtcComboBox;
        m_rememberCombo->setToolTip(Tr::tr("Controls whether and where this context is remembered."));
        const auto addScope = [this](TextContextScope scope) {
            m_rememberCombo->addItem(scopeName(scope), int(scope));
            m_rememberCombo->setItemData(
                m_rememberCombo->count() - 1, scopeTooltip(scope), Qt::ToolTipRole);
        };
        addScope(TextContextScope::None);
        addScope(TextContextScope::Session);
        addScope(TextContextScope::Agent);
        addScope(TextContextScope::AllAgents);
        buttonLayout->addWidget(m_rememberCombo);

        buttonLayout->addStretch(1);

        auto *cancelButton = new QtcButton(Tr::tr("Cancel"), QtcButton::MediumTertiary);
        buttonLayout->addWidget(cancelButton);
        auto *saveButton = new QtcButton(Tr::tr("Save"), QtcButton::MediumPrimary);
        buttonLayout->addWidget(saveButton);
        layout->addWidget(buttonRow);

        connect(cancelButton, &QAbstractButton::clicked, this, &TextContextEditor::cancelled);
        connect(saveButton, &QAbstractButton::clicked, this, [this] {
            const auto scope = TextContextScope(m_rememberCombo->currentData().toInt());
            emit saved(m_nameEdit->toPlainText().trimmed(), m_textEdit->toPlainText(), scope);
        });
    }

    void setContext(const QString &name, const QString &text, TextContextScope scope)
    {
        m_nameEdit->setPlainText(name);
        m_textEdit->setPlainText(text);
        const int index = m_rememberCombo->findData(int(scope));
        m_rememberCombo->setCurrentIndex(index < 0 ? 0 : index);
    }

    void focusName() { m_nameEdit->setFocus(); }

    // Supplies the list of previously entered contexts for the history menu.
    std::function<QList<TextContext>()> historyProvider;
    // Removes the history entry at the given index from the settings.
    std::function<void(int)> onRemoveHistory;

signals:
    void saved(const QString &name, const QString &text, TextContextScope scope);
    void cancelled();

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        StyleHelper::drawCardBg(
            &p,
            rect(),
            creatorColor(Theme::Token_Background_Subtle),
            creatorColor(Theme::Token_Foreground_Muted),
            RadiusM);
    }

private:
    static QString scopeName(TextContextScope scope)
    {
        switch (scope) {
        case TextContextScope::None:      return Tr::tr("Do Not Remember");
        case TextContextScope::Session:   return Tr::tr("Remember for This Session");
        case TextContextScope::Agent:     return Tr::tr("Remember for This Agent");
        case TextContextScope::AllAgents: return Tr::tr("Remember for All Agents");
        }
        return {};
    }

    static QString scopeTooltip(TextContextScope scope)
    {
        switch (scope) {
        case TextContextScope::None:
            return Tr::tr("Use this context only for the current chat session and do not "
                          "store it.");
        case TextContextScope::Session:
            return Tr::tr("Remember this context for this chat and restore it when the "
                          "session is reopened.");
        case TextContextScope::Agent:
            return Tr::tr("Remember this context for every chat with the current agent.");
        case TextContextScope::AllAgents:
            return Tr::tr("Remember this context for every chat with any agent.");
        }
        return {};
    }

    void showHistoryMenu()
    {
        if (!historyProvider)
            return;
        const QList<TextContext> history = historyProvider();

        auto *menu = new QMenu(m_historyButton);
        menu->setAttribute(Qt::WA_DeleteOnClose);

        if (history.isEmpty()) {
            QAction *a = menu->addAction(Tr::tr("No Previous Contexts"));
            a->setEnabled(false);
        }
        for (int i = 0; i < history.size(); ++i) {
            const TextContext ctx = history.at(i);
            auto *row = new ContextItem(ctx.name, {}, nullptr, /*withBorder=*/false);
            auto *action = new QWidgetAction(menu);
            action->setDefaultWidget(row);
            menu->addAction(action);
            connect(row, &ContextItem::clicked, this, [this, ctx, menu] {
                m_nameEdit->setPlainText(ctx.name);
                m_textEdit->setPlainText(ctx.text);
                menu->close();
                m_nameEdit->setFocus();
            });
            connect(row, &ContextItem::removeRequested, this, [this, i, menu] {
                if (onRemoveHistory)
                    onRemoveHistory(i);
                menu->close();
                // Reopen so the user sees the updated list.
                QMetaObject::invokeMethod(this, [this] { showHistoryMenu(); }, Qt::QueuedConnection);
            });
        }

        const QPoint topLeft = m_historyButton->mapToGlobal(QPoint(0, 0));
        menu->popup(QPoint(topLeft.x(), topLeft.y() - menu->sizeHint().height()));
    }

    ContextTextEdit *m_nameEdit;
    ContextTextEdit *m_textEdit;
    QtcIconButton *m_historyButton = nullptr;
    QtcComboBox *m_rememberCombo = nullptr;
};

ChatPanel::ChatPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

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

    // --- Input bar (rounded container) ---
    auto *inputOuter = new QWidget;
    auto *inputOuterLayout = new QVBoxLayout(inputOuter);
    inputOuterLayout->setContentsMargins(PaddingHM, PaddingVXs, PaddingHM, PaddingVM);
    inputOuterLayout->setSpacing(GapVXs);

    auto *inputContainer = new InputContainerWidget;
    inputContainer->onFilesDropped = [this](const QList<FilePath> &files) { addContextFiles(files); };
    auto *inputContainerLayout = new QVBoxLayout(inputContainer);
    inputContainerLayout->setContentsMargins(PaddingHM, PaddingVM, PaddingHM, PaddingVM);
    inputContainerLayout->setSpacing(GapVXs);

    m_contextBar = new QWidget;
    Layouting::Flow{}.attachTo(m_contextBar);
    m_contextBarLayout = m_contextBar->layout();
    m_contextBarLayout->setContentsMargins(0, 0, 0, 0);
    m_contextBarLayout->setSpacing(GapHS);

    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged, this, [this] {
        updateContextBar();
    });

    inputContainerLayout->addWidget(m_contextBar);

    m_inputEdit = new ChatInputEdit;
    inputContainerLayout->addWidget(m_inputEdit);
    inputContainer->setFocusProxy(m_inputEdit);

    auto *bottomRow = new QWidget;
    auto *bottomRowLayout = new QHBoxLayout(bottomRow);
    bottomRowLayout->setContentsMargins(0, 0, 0, 0);
    bottomRowLayout->setSpacing(GapHS);

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

        auto *addTextAction = menu->addAction(Tr::tr("Add Text..."));
        connect(addTextAction, &QAction::triggered, this, [this] {
            showTextContextEditor(-1);
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

    m_modeButton = new QtcButton({}, QtcButton::SmallGhost);
    m_modeButton->setToolTip(Tr::tr("Switch Mode"));
    m_modeButton->hide();
    connect(m_modeButton, &QAbstractButton::clicked, this, &ChatPanel::showModeMenu);
    bottomRowLayout->addWidget(m_modeButton, 0, Qt::AlignVCenter);

    bottomRowLayout->addStretch(1);

    m_usageBar = new QtcProgressBar;
    m_usageBar->setFixedWidth(64);
    m_usageBar->setToolTip(Tr::tr("Context usage"));
    m_usageBar->hide();
    bottomRowLayout->addWidget(m_usageBar, 0, Qt::AlignVCenter);

    bottomRowLayout->addWidget(addContextButton);

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
    bottomRowLayout->addWidget(m_commandsButton);

    m_configButton = new QtcIconButton;
    m_configButton->setIcon(Icons::SETTINGS.icon());
    m_configButton->setToolTip(Tr::tr("Configuration Options"));
    connect(m_configButton, &QAbstractButton::clicked, this, &ChatPanel::showConfigMenu);
    bottomRowLayout->addWidget(m_configButton);

    m_sendButton = new QtcButton(Tr::tr("Send"), QtcButton::MediumPrimary);
    m_sendButton->setEnabled(false);
    bottomRowLayout->addWidget(m_sendButton);

    inputContainerLayout->addWidget(bottomRow);

    m_textContextEditor = new TextContextEditor;
    m_textContextEditor->hide();
    connect(m_textContextEditor, &TextContextEditor::saved, this,
            [this](const QString &name, const QString &text, TextContextScope scope) {
        const bool editing = m_editingTextContextIndex >= 0
                             && m_editingTextContextIndex < m_textContexts.size();
        if (text.isEmpty()) {
            if (editing)
                m_textContexts.removeAt(m_editingTextContextIndex);
        } else {
            const QString n = name.isEmpty() ? Tr::tr("Text") : name;
            if (editing)
                m_textContexts[m_editingTextContextIndex] = {n, text, scope};
            else
                m_textContexts.append({n, text, scope});
            addTextContextToHistory(n, text);
        }
        hideTextContextEditor();
        persistTextContexts();
        updateContextBar();
    });
    m_textContextEditor->historyProvider = [this] { return textContextHistory(); };
    m_textContextEditor->onRemoveHistory = [this](int index) { removeTextContextHistoryAt(index); };
    connect(m_textContextEditor, &TextContextEditor::cancelled, this,
            [this] { hideTextContextEditor(); });

    inputOuterLayout->addWidget(m_textContextEditor);
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

    updateContextBar();
}

void ChatPanel::setAgentIcon(const QString &iconUrl)
{
    m_messageView->setAgentIconUrl(iconUrl);
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

    if (!menu->isEmpty())
        menu->addSeparator();
    QAction *inspect = menu->addAction(Tr::tr("Inspect ACP Client..."));
    connect(inspect, &QAction::triggered, this, &ChatPanel::inspectRequested);

    menu->popup(QCursor::pos());
}

void ChatPanel::setConfigOptions(const QList<SessionConfigOption> &configOptions)
{
    m_configOptions = configOptions;
}

void ChatPanel::setSessionModes(const QList<SessionMode> &modes, const QString &currentModeId)
{
    m_sessionModes = modes;
    m_currentModeId = currentModeId;
    updateModeButton();
}

void ChatPanel::setCurrentMode(const QString &modeId)
{
    m_currentModeId = modeId;
    updateModeButton();
}

void ChatPanel::setUsage(const Acp::UsageUpdate &usage)
{
    const int size = usage.size();
    const int used = usage.used();
    if (size <= 0) {
        m_usageBar->hide();
        return;
    }
    m_usageBar->setRange(0, size);
    m_usageBar->setValue(used);
    const int percent = qRound(qreal(qBound(0, used, size)) / size * 100.0);
    QString tooltip = Tr::tr("Context usage: %1 / %2 tokens (%3%)").arg(used).arg(size).arg(percent);
    if (usage.cost()) {
        const Cost &cost = *usage.cost();
        tooltip += QLatin1Char('\n')
                   + Tr::tr("Cost: %1 %2")
                         .arg(cost.amount(), 0, 'f', 2)
                         .arg(cost.currency());
    }
    m_usageBar->setToolTip(tooltip);
    m_usageBar->show();
}

void ChatPanel::updateModeButton()
{
    if (m_sessionModes.isEmpty()) {
        m_modeButton->hide();
        return;
    }

    QString name = m_currentModeId;
    for (const SessionMode &mode : std::as_const(m_sessionModes)) {
        if (mode.id() == m_currentModeId) {
            name = mode.name();
            break;
        }
    }
    m_modeButton->setText(name);
    m_modeButton->show();
}

void ChatPanel::showModeMenu()
{
    if (m_sessionModes.isEmpty())
        return;

    auto *menu = new QMenu(m_modeButton);
    menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->setToolTipsVisible(true);
    for (const SessionMode &mode : std::as_const(m_sessionModes)) {
        QAction *action = menu->addAction(mode.name());
        if (const auto &description = mode.description())
            action->setToolTip(*description);
        action->setCheckable(true);
        action->setChecked(mode.id() == m_currentModeId);
        const QString id = mode.id();
        connect(action, &QAction::triggered, this, [this, id] { emit modeChanged(id); });
    }

    const QPoint topLeft = m_modeButton->mapToGlobal(QPoint(0, 0));
    const QSize menuSize = menu->sizeHint();
    menu->popup(QPoint(topLeft.x(), topLeft.y() - menuSize.height()));
}

void ChatPanel::clear()
{
    m_messageView->clear();
    if (m_usageBar)
        m_usageBar->hide();
}

void ChatPanel::clearConfigOptions()
{
    m_configOptions.clear();
    m_sessionModes.clear();
    m_currentModeId.clear();
    updateModeButton();
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

void ChatPanel::addContextFiles(const QList<Utils::FilePath> &files)
{
    bool added = false;
    for (const FilePath &fp : files) {
        if (!fp.isEmpty() && !m_manualContextFiles.contains(fp)) {
            m_manualContextFiles.append(fp);
            added = true;
        }
    }
    if (added)
        updateContextBar();
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
                auto *item = new ContextItem(
                    name, FileIconProvider::icon(filePath), m_contextBar);
                connect(item, &ContextItem::removeRequested, this, [this] {
                    m_includeCurrentEditorContext = false;
                    QMetaObject::invokeMethod(this, [this] { updateContextBar(); }, Qt::QueuedConnection);
                });
                connect(item, &ContextItem::clicked, this, [filePath] {
                    Core::EditorManager::openEditor(filePath);
                });
                m_contextBarLayout->addWidget(item);
            }
        }
    }

    for (const Utils::FilePath &file : std::as_const(m_manualContextFiles)) {
        auto *item = new ContextItem(file.fileName(), FileIconProvider::icon(file), m_contextBar);
        connect(item, &ContextItem::removeRequested, this, [this, file] {
            m_manualContextFiles.removeOne(file);
            QMetaObject::invokeMethod(this, [this] { updateContextBar(); }, Qt::QueuedConnection);
        });
        connect(item, &ContextItem::clicked, this, [file] {
            Core::EditorManager::openEditor(file);
        });
        m_contextBarLayout->addWidget(item);
    }

    for (int i = 0; i < m_textContexts.size(); ++i) {
        auto *item = new ContextItem(
            m_textContexts.at(i).name, Core::Icons::MODE_EDIT_FLAT.icon(), m_contextBar);
        connect(item, &ContextItem::removeRequested, this, [this, i] {
            m_textContexts.removeAt(i);
            persistTextContexts();
            QMetaObject::invokeMethod(this, [this] { updateContextBar(); }, Qt::QueuedConnection);
        });
        connect(item, &ContextItem::clicked, this, [this, i] {
            showTextContextEditor(i);
        });
        m_contextBarLayout->addWidget(item);
    }

    m_contextBar->setVisible(m_contextBarLayout->count() > 0);
}

void ChatPanel::showTextContextEditor(int index)
{
    m_editingTextContextIndex = index;
    if (index >= 0 && index < m_textContexts.size()) {
        const TextContext &ctx = m_textContexts.at(index);
        m_textContextEditor->setContext(ctx.name, ctx.text, ctx.scope);
    } else {
        m_textContextEditor->setContext({}, {}, TextContextScope::None);
    }
    m_textContextEditor->show();
    m_textContextEditor->focusName();
}

void ChatPanel::hideTextContextEditor()
{
    m_editingTextContextIndex = -1;
    m_textContextEditor->hide();
}

void ChatPanel::setAgentId(const QString &agentId)
{
    if (m_agentId == agentId)
        return;
    m_agentId = agentId;
    reloadPersistedTextContexts();
}

void ChatPanel::setSessionId(const QString &sessionId)
{
    if (m_sessionId == sessionId)
        return;
    m_sessionId = sessionId;
    reloadPersistedTextContexts();
}

// Remembered text contexts are persisted in the settings database (not the ini
// settings), since they can hold large multi-line text.
static QString textContextGlobalKey() { return "AcpClient/TextContexts/Global"; }
static QString textContextAgentKey(const QString &agentId)
{
    return "AcpClient/TextContexts/Agent/" + agentId;
}
static QString textContextSessionKey(const QString &agentId, const QString &sessionId)
{
    return "AcpClient/TextContexts/Session/" + agentId + '/' + sessionId;
}

void ChatPanel::reloadPersistedTextContexts()
{
    // Drop previously loaded remembered contexts, keep only transient (none) ones.
    m_textContexts.removeIf([](const TextContext &ctx) {
        return ctx.scope != TextContextScope::None;
    });

    const auto load = [this](const QString &key, TextContextScope scope) {
        // Stored as a JSON array string: SQLite cannot round-trip a nested
        // QVariantList/QVariantMap through a single bound value.
        const QByteArray json = SettingsDatabase::value(key).toString().toUtf8();
        const QJsonArray array = QJsonDocument::fromJson(json).array();
        for (const QJsonValue &item : array) {
            const QJsonObject o = item.toObject();
            m_textContexts.append(
                {o.value("name").toString(), o.value("text").toString(), scope});
        }
    };

    load(textContextGlobalKey(), TextContextScope::AllAgents);
    if (!m_agentId.isEmpty())
        load(textContextAgentKey(m_agentId), TextContextScope::Agent);
    if (!m_agentId.isEmpty() && !m_sessionId.isEmpty())
        load(textContextSessionKey(m_agentId, m_sessionId), TextContextScope::Session);

    updateContextBar();
}

void ChatPanel::persistTextContexts()
{
    QJsonArray global;
    QJsonArray agent;
    QJsonArray session;
    for (const TextContext &ctx : std::as_const(m_textContexts)) {
        const QJsonObject o{{"name", ctx.name}, {"text", ctx.text}};
        if (ctx.scope == TextContextScope::AllAgents)
            global.append(o);
        else if (ctx.scope == TextContextScope::Agent)
            agent.append(o);
        else if (ctx.scope == TextContextScope::Session)
            session.append(o);
    }
    // Stored as a JSON array string: SQLite cannot round-trip a nested
    // QVariantList/QVariantMap through a single bound value.
    const auto toJson = [](const QJsonArray &a) {
        return QString::fromUtf8(QJsonDocument(a).toJson(QJsonDocument::Compact));
    };
    SettingsDatabase::setValue(textContextGlobalKey(), toJson(global), std::chrono::seconds::max());
    if (!m_agentId.isEmpty())
        SettingsDatabase::setValue(textContextAgentKey(m_agentId), toJson(agent), std::chrono::years(1));
    if (!m_agentId.isEmpty() && !m_sessionId.isEmpty())
        SettingsDatabase::setValue(
            textContextSessionKey(m_agentId, m_sessionId), toJson(session), std::chrono::months(3));
}

// History of every text context the user has entered, independent of scope, so
// previously used contexts can be picked again from the editor.
static QString textContextHistoryKey() { return "AcpClient/TextContexts/History"; }
static constexpr int textContextHistoryMax = 50;

QList<TextContext> ChatPanel::textContextHistory() const
{
    const QByteArray json = SettingsDatabase::value(textContextHistoryKey()).toString().toUtf8();
    const QJsonArray array = QJsonDocument::fromJson(json).array();
    QList<TextContext> result;
    for (const QJsonValue &item : array) {
        const QJsonObject o = item.toObject();
        result.append({o.value("name").toString(), o.value("text").toString(),
                       TextContextScope::None});
    }
    return result;
}

void ChatPanel::addTextContextToHistory(const QString &name, const QString &text)
{
    if (text.isEmpty())
        return;

    QList<TextContext> history = textContextHistory();
    // Drop an existing identical entry, then prepend so it becomes most recent.
    history.removeIf([&](const TextContext &c) { return c.name == name && c.text == text; });
    history.prepend({name, text, TextContextScope::None});
    while (history.size() > textContextHistoryMax)
        history.removeLast();

    QJsonArray array;
    for (const TextContext &c : std::as_const(history))
        array.append(QJsonObject{{"name", c.name}, {"text", c.text}});
    SettingsDatabase::setValue(
        textContextHistoryKey(),
        QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact)),
        std::chrono::seconds::max());
}

void ChatPanel::removeTextContextHistoryAt(int index)
{
    QList<TextContext> history = textContextHistory();
    if (index < 0 || index >= history.size())
        return;
    history.removeAt(index);

    QJsonArray array;
    for (const TextContext &c : std::as_const(history))
        array.append(QJsonObject{{"name", c.name}, {"text", c.text}});
    SettingsDatabase::setValue(
        textContextHistoryKey(),
        QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact)),
        std::chrono::seconds::max());
}

} // namespace AcpClient::Internal

#include "chatpanel.moc"
