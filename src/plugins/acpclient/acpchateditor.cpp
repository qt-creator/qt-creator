// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acpchateditor.h"

#include "acpchattab.h"
#include "acpclientconstants.h"
#include "acpclienttr.h"

#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <utils/qtcassert.h>
#include <utils/qtcsettings.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QRegion>
#include <QToolButton>

#include <algorithm>
#include <memory>
#include <utility>

using namespace Core;
using namespace Utils;

namespace AcpClient::Internal {

static AcpChatTab *s_pendingTab = nullptr;
static AcpInspector *s_inspector = nullptr;
static std::function<void(AcpChatTab *)> s_moveToSidePanel;

// Nothing to load, save or reload: the chat lives in its tab, not in a file.
class AcpChatDocument final : public IDocument
{
public:
    AcpChatDocument()
    {
        setId(Constants::CHAT_EDITOR_ID);
        // The agent connection does not survive the session, so restoring the
        // editor would only bring back an empty chat.
        setTemporary(true);
    }

    Result<> setContents(const QByteArray &contents) final
    {
        QTC_CHECK(contents.isEmpty());
        return ResultOk;
    }

    ReloadBehavior reloadBehavior(ChangeTrigger, ChangeType) const final
    {
        return BehaviorSilent;
    }
};

const char chatColumnsKey[] = "AcpClient/ChatEditorColumns";
const double defaultChatColumns = 100;
const double minimumChatColumns = 60;

static int chatColumnWidth(const QWidget *chat, double columns)
{
    return qRound(chat->fontMetrics().averageCharWidth() * columns);
}

static int maximumChatWidth(const QWidget *chat)
{
    const double columns = ICore::settings()->value(chatColumnsKey, defaultChatColumns).toDouble();
    return chatColumnWidth(chat, std::max(columns, minimumChatColumns));
}

static void setMaximumChatWidth(QWidget *chat, int width)
{
    chat->setMaximumWidth(width);
    const double columns = double(width) / chat->fontMetrics().averageCharWidth();
    ICore::settings()->setValueWithDefault(chatColumnsKey, columns, defaultChatColumns);
}

// Shows where the sides of the chat column will be while a handle is dragged.
// Laying out a long chat is expensive, so the chat itself is only resized
// once the drag ends.
class ChatColumnPreview final : public QWidget
{
public:
    explicit ChatColumnPreview(QWidget *parent)
        : QWidget(parent)
    {
        setObjectName("chatColumnPreview");
        setAttribute(Qt::WA_TransparentForMouseEvents);
    }

    void showPreview(int centerX, int columnWidth)
    {
        if (!isVisible()) {
            setGeometry(parentWidget()->rect());
            raise();
            show();
        }
        // Only the strips of the lines change. Repainting all of the overlay
        // would repaint all of the chat below it.
        QRegion dirty = lines();
        m_centerX = centerX;
        m_columnWidth = columnWidth;
        dirty += lines();
        update(dirty);
    }

    int columnWidth() const { return m_columnWidth; }

protected:
    void paintEvent(QPaintEvent *) final
    {
        QPainter painter(this);
        const QColor color = creatorColor(Theme::Token_Accent_Default);
        for (const QRect &line : lines())
            painter.fillRect(line, color);
    }

private:
    QRegion lines() const
    {
        const int thickness = StyleHelper::HighlightThickness;
        const int left = m_centerX - m_columnWidth / 2 - thickness;
        const int right = m_centerX + (m_columnWidth + 1) / 2;
        return QRegion(QRect(left, 0, thickness, height()))
               + QRegion(QRect(right, 0, thickness, height()));
    }

    int m_centerX = 0;
    int m_columnWidth = 0;
};

// Sits at one side of the chat column. As the column stays centered, dragging
// the handle moves both of its sides.
class ChatColumnHandle final : public QWidget
{
public:
    ChatColumnHandle(Qt::Edge edge, QWidget *chat, ChatColumnPreview *preview)
        : m_edge(edge)
        , m_chat(chat)
        , m_preview(preview)
    {
        setObjectName(edge == Qt::LeftEdge ? "chatColumnLeftHandle" : "chatColumnRightHandle");
        setCursor(Qt::SplitHCursor);
        setFixedWidth(StyleHelper::SpacingTokens::PaddingHM);
        setAttribute(Qt::WA_Hover);
        setToolTip(Tr::tr("Drag to change the width of the chat. Double-click to reset it."));
    }

    void setChat(QWidget *chat) { m_chat = chat; }

protected:
    void mousePressEvent(QMouseEvent *event) final
    {
        if (event->button() != Qt::LeftButton || !m_chat)
            return QWidget::mousePressEvent(event);
        m_dragStartX = event->globalPosition().x();
        m_dragStartWidth = m_chat->width();
        m_dragging = true;
        update();
    }

    void mouseMoveEvent(QMouseEvent *event) final
    {
        if (!m_dragging || !m_chat)
            return QWidget::mouseMoveEvent(event);
        const qreal dx = event->globalPosition().x() - m_dragStartX;
        const int delta = qRound(m_edge == Qt::RightEdge ? dx : -dx);
        const int available = m_chat->parentWidget()->width();
        const int width = std::max(std::min(m_dragStartWidth + 2 * delta, available),
                                   chatColumnWidth(m_chat, minimumChatColumns));
        m_preview->showPreview(m_chat->geometry().center().x(), width);
    }

    void mouseReleaseEvent(QMouseEvent *event) final
    {
        if (!m_dragging || event->button() != Qt::LeftButton)
            return QWidget::mouseReleaseEvent(event);
        m_dragging = false;
        if (m_preview->isVisible()) {
            m_preview->hide();
            // A chat that the editor keeps narrower than its column only grows
            // within that column, which must not shrink it.
            const int width = m_preview->columnWidth();
            if (m_chat && (width < m_dragStartWidth || width > m_chat->maximumWidth()))
                setMaximumChatWidth(m_chat, width);
        }
        update();
    }

    void mouseDoubleClickEvent(QMouseEvent *event) final
    {
        if (event->button() != Qt::LeftButton || !m_chat)
            return QWidget::mouseDoubleClickEvent(event);
        setMaximumChatWidth(m_chat, chatColumnWidth(m_chat, defaultChatColumns));
    }

    void paintEvent(QPaintEvent *) final
    {
        QPainter painter(this);
        // While dragging, the preview shows the new sides, and the handle
        // marks the current one.
        if (m_dragging || !underMouse()) {
            painter.setPen(creatorColor(Theme::Token_Foreground_Subtle));
            const int x = width() / 2;
            painter.drawLine(x, 0, x, height() - 1);
            return;
        }
        const int x = (width() - StyleHelper::HighlightThickness) / 2;
        painter.fillRect(QRect(x, 0, StyleHelper::HighlightThickness, height()),
                         creatorColor(Theme::Token_Foreground_Subtle));
    }

    bool event(QEvent *event) final
    {
        if (event->type() == QEvent::HoverEnter || event->type() == QEvent::HoverLeave)
            update();
        return QWidget::event(event);
    }

private:
    const Qt::Edge m_edge;
    QPointer<QWidget> m_chat;
    ChatColumnPreview *const m_preview;
    qreal m_dragStartX = 0;
    int m_dragStartWidth = 0;
    bool m_dragging = false;
};

// Gets the focus when the editor is activated and passes it on to the chat.
class ChatEditorWidget final : public QWidget
{
public:
    void setChat(AcpChatTab *chat) { m_chat = chat; }

protected:
    void focusInEvent(QFocusEvent *event) final
    {
        if (m_chat)
            m_chat->setFocus();
        else
            QWidget::focusInEvent(event);
    }

private:
    QPointer<AcpChatTab> m_chat;
};

class AcpChatEditor final : public IEditor
{
public:
    explicit AcpChatEditor(AcpChatTab *tab)
        : m_document(std::make_unique<AcpChatDocument>())
        , m_container(new ChatEditorWidget)
        , m_toolBar(new QWidget)
        , m_tab(tab)
    {
        // An editor is much wider than the side panel. Lines of that length are
        // hard to read and put the user's messages far from the agent's, so the
        // chat keeps to a column in the middle.
        auto containerLayout = new QHBoxLayout(m_container);
        containerLayout->setContentsMargins(0, 0, 0, 0);
        containerLayout->setSpacing(0);
        auto preview = new ChatColumnPreview(m_container);
        preview->hide();
        m_leftHandle = new ChatColumnHandle(Qt::LeftEdge, tab, preview);
        m_rightHandle = new ChatColumnHandle(Qt::RightEdge, tab, preview);
        containerLayout->addStretch(0);
        containerLayout->addWidget(m_leftHandle);
        containerLayout->addWidget(tab, 1);
        containerLayout->addWidget(m_rightHandle);
        containerLayout->addStretch(0);
        tab->setMaximumWidth(maximumChatWidth(tab));
        tab->show();
        m_container->setChat(tab);

        setWidget(m_container);
        setContext(Context(Constants::CHAT_EDITOR_ID));
        setDuplicateSupported(false);

        auto toolBarLayout = new QHBoxLayout(m_toolBar);
        toolBarLayout->setContentsMargins(0, 0, 0, 0);
        toolBarLayout->setSpacing(0);
        QWidget *tabToolBar = tab->toolBarWidget();
        toolBarLayout->addWidget(tabToolBar);
        tabToolBar->show();
        toolBarLayout->addStretch();

        auto moveButton = new QToolButton;
        moveButton->setObjectName("moveChatToSidePanelButton");
        moveButton->setIcon(Icons::TOGGLE_RIGHT_SIDEBAR_TOOLBAR.icon());
        moveButton->setToolTip(Tr::tr("Move Chat to Side Panel"));
        // Queued: moving closes the editor, which deletes the button.
        connect(moveButton, &QToolButton::clicked, this, &AcpChatEditor::moveToSidePanel,
                Qt::QueuedConnection);
        toolBarLayout->addWidget(moveButton);

        connect(tab, &AcpChatTab::titleChanged, this, &AcpChatEditor::updateTitle);
        updateTitle();
    }

    ~AcpChatEditor() final
    {
        // The tab's panel owns the tool bar widget inside m_toolBar, so the
        // tab goes first.
        delete m_container;
        delete m_toolBar;
    }

    IDocument *document() const final { return m_document.get(); }
    QWidget *toolBar() final { return m_toolBar; }

private:
    void updateTitle()
    {
        if (m_tab)
            m_document->setPreferredDisplayName(m_tab->title());
    }

    void moveToSidePanel()
    {
        QTC_ASSERT(m_tab, return);
        AcpChatTab *tab = m_tab;
        m_tab = nullptr;
        disconnect(tab, nullptr, this, nullptr);

        QWidget *tabToolBar = tab->toolBarWidget();
        m_toolBar->layout()->removeWidget(tabToolBar);
        tabToolBar->setParent(nullptr);
        m_container->layout()->removeWidget(tab);
        tab->setParent(nullptr);
        tab->setMaximumWidth(QWIDGETSIZE_MAX);
        m_leftHandle->setChat(nullptr);
        m_rightHandle->setChat(nullptr);
        m_container->setChat(nullptr);

        if (QTC_GUARD(s_moveToSidePanel))
            s_moveToSidePanel(tab);
        else
            delete tab;
        EditorManager::closeEditors({this}, false);
    }

    std::unique_ptr<AcpChatDocument> m_document;
    ChatEditorWidget *m_container = nullptr;
    QWidget *m_toolBar = nullptr;
    ChatColumnHandle *m_leftHandle = nullptr;
    ChatColumnHandle *m_rightHandle = nullptr;
    QPointer<AcpChatTab> m_tab;
};

class AcpChatEditorFactory final : public IEditorFactory
{
public:
    AcpChatEditorFactory()
    {
        setId(Constants::CHAT_EDITOR_ID);
        setDisplayName(Tr::tr("Agentic AI Chat"));
        setEditorCreator([] {
            AcpChatTab *tab = std::exchange(s_pendingTab, nullptr);
            if (!tab) {
                tab = new AcpChatTab;
                if (s_inspector)
                    tab->setInspector(s_inspector);
            }
            return new AcpChatEditor(tab);
        });
    }
};

/*!
    \internal

    Registers the editor that shows a chat in the editor area. \a moveToSidePanel
    takes over a chat tab whose editor is moved back to the side panel.
*/
void setupAcpChatEditor(AcpInspector *inspector,
                        const std::function<void(AcpChatTab *)> &moveToSidePanel)
{
    static AcpChatEditorFactory theAcpChatEditorFactory;
    s_inspector = inspector;
    s_moveToSidePanel = moveToSidePanel;
}

/*!
    \internal

    Opens an editor that takes over \a tab. Returns \c false if no editor was
    created, in which case the caller still owns \a tab.
*/
bool openChatInEditor(AcpChatTab *tab)
{
    QTC_ASSERT(tab, return false);
    s_pendingTab = tab;
    IEditor *editor = EditorManager::openEditorWithContents(Constants::CHAT_EDITOR_ID);
    if (std::exchange(s_pendingTab, nullptr))
        return false;
    QTC_CHECK(editor);
    tab->setFocus();
    return true;
}

/*!
    \internal

    Closes the chat editors, which ends their agent connections.
*/
void closeAllChatEditors()
{
    QList<IDocument *> documents = DocumentModel::openedDocuments();
    documents.removeIf([](IDocument *document) {
        return document->id() != Constants::CHAT_EDITOR_ID;
    });
    if (!documents.isEmpty())
        EditorManager::closeDocuments(documents, false);
}

} // namespace AcpClient::Internal
