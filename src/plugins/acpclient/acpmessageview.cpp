// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acpmessageview.h"
#include "acpsettings.h"
#include "collapsibleframe.h"
#include "toolcalldetailwidget.h"

#include <utils/async.h>
#include <utils/markdownbrowser.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

#include <QAbstractTextDocumentLayout>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QtMath>
#include <QPushButton>
#include <QResizeEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QScrollBar>
#include <QTextDocument>
#include <QTimer>
#include <QVBoxLayout>

using namespace Acp;
using namespace Utils::StyleHelper::SpacingTokens;

namespace AcpClient::Internal {

// ---------------------------------------------------------------------------
// AvatarWidget — circular avatar for agent/user messages
// ---------------------------------------------------------------------------

class AvatarWidget : public QWidget
{
public:
    enum AvatarType { Agent, User };

    explicit AvatarWidget(AvatarType type, QWidget *parent = nullptr)
        : QWidget(parent), m_type(type)
    {
        setFixedSize(28, 28);
    }

    void setIcon(const QIcon &icon) { m_icon = icon; update(); }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QRectF r = QRectF(rect()).adjusted(2, 2, -2, -2);

        if (m_type == Agent) {
            if (!m_icon.isNull()) {
                m_icon.paint(&p, r.toAlignedRect());
            } else {
                QRadialGradient gradient(r.center(), r.width() / 2.0);
                gradient.setColorAt(0, QColor(0x5e, 0xea, 0x8d));
                gradient.setColorAt(0.6, QColor(0x22, 0xc5, 0x5e));
                gradient.setColorAt(1.0, QColor(0x16, 0xa3, 0x4a));
                p.setBrush(gradient);
                p.setPen(Qt::NoPen);
                p.drawEllipse(r);
            }
        } else {
            p.setBrush(QColor(0x52, 0x52, 0x5e));
            p.setPen(Qt::NoPen);
            p.drawEllipse(r);
            p.setPen(Qt::white);
            QFont f = font();
            f.setPixelSize(11);
            f.setBold(true);
            p.setFont(f);
            p.drawText(r, Qt::AlignCenter, QStringLiteral("U"));
        }
    }

private:
    AvatarType m_type;
    QIcon m_icon;
};

// ---------------------------------------------------------------------------
// MessageWidget — unified bubble for both user and agent messages
// ---------------------------------------------------------------------------

class MessageWidget : public CollapsibleFrame
{
public:
    enum Role { User, Agent };

    explicit MessageWidget(Role role, QWidget *parent = nullptr)
        : CollapsibleFrame(parent)
        , m_role(role)
    {
        setFrameShape(QFrame::NoFrame);
        setCollapsible(false);
        setHeaderVisible(false);

        if (role == Agent)
            m_bodyLayout->setContentsMargins(0, 0, 0, 0);

        m_browser = new Utils::MarkdownBrowser(this);
        m_browser->setFrameShape(QFrame::NoFrame);
        m_browser->setOpenExternalLinks(true);
        m_browser->setEnableCodeCopyButton(true);
        m_browser->setMargins({0, 0, 0, 0});
        m_browser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_browser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        QPalette bodyPal = m_browser->palette();
        bodyPal.setColor(QPalette::Base, bgColor());
        m_browser->setPalette(bodyPal);
        m_browser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_browser->setMinimumWidth(1);
        m_bodyLayout->addWidget(m_browser);

        // Debounce markdown rendering to coalesce streamed chunks
        m_renderTimer = new QTimer(this);
        m_renderTimer->setSingleShot(true);
        m_renderTimer->setInterval(100);
        connect(m_renderTimer, &QTimer::timeout, this, [this] {
            m_browser->setMarkdown(m_rawText);
        });

        // Update height when document size changes (content change OR reflow due to width change)
        connect(m_browser->document()->documentLayout(),
                &QAbstractTextDocumentLayout::documentSizeChanged,
                this, [this] {
            if (!m_heightUpdatePending) {
                m_heightUpdatePending = true;
                QTimer::singleShot(0, this, [this] {
                    m_heightUpdatePending = false;
                    updateBrowserHeight();
                    updateGeometry();
                });
            }
        });
    }

    void setText(const QString &text)
    {
        m_rawText = text;
        m_browser->setMarkdown(m_rawText);
    }

    void appendText(const QString &text)
    {
        m_rawText += text;
        m_renderTimer->start();
    }

    void flush()
    {
        if (m_renderTimer->isActive()) {
            m_renderTimer->stop();
            m_browser->setMarkdown(m_rawText);
        }
    }

    QSize sizeHint() const override
    {
        if (!m_browser)
            return CollapsibleFrame::sizeHint();
        // Temporarily remove the text width constraint so idealWidth() returns
        // the true single-line (unwrapped) width of the document.
        QTextDocument *doc = m_browser->document();
        const qreal savedTextWidth = doc->textWidth();
        doc->setTextWidth(-1);
        const qreal iw = doc->idealWidth();
        doc->setTextWidth(savedTextWidth);
        if (iw <= 0)
            return CollapsibleFrame::sizeHint();
        const QMargins bm = m_bodyLayout->contentsMargins();
        const QMargins bcm = m_browser->contentsMargins();
        const int docMargin = static_cast<int>(doc->documentMargin());
        const int contentWidth = qCeil(iw) + bm.left() + bm.right()
                                 + bcm.left() + bcm.right() + 2 * docMargin;
        return QSize(contentWidth, CollapsibleFrame::sizeHint().height());
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        if (m_role != User)
            return;
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        Utils::StyleHelper::drawCardBg(&p, rect(), bgColor());
    }

private:
    QColor bgColor() const
    {
        return m_role == User ? Utils::creatorColor(Utils::Theme::ChatUserMessageBackground)
                              : Qt::transparent;
    }

    void updateBrowserHeight()
    {
        const int docHeight = qCeil(m_browser->document()->size().height())
                              + m_browser->contentsMargins().top()
                              + m_browser->contentsMargins().bottom();
        m_browser->setFixedHeight(qMax(docHeight, m_browser->fontMetrics().height()));
    }

    Role m_role;
    Utils::MarkdownBrowser *m_browser = nullptr;
    QTimer *m_renderTimer = nullptr;
    QString m_rawText;
    bool m_heightUpdatePending = false;
};

// ---------------------------------------------------------------------------
// ThoughtWidget
// ---------------------------------------------------------------------------

class ThoughtWidget : public CollapsibleFrame
{
public:
    explicit ThoughtWidget(const QString &text, QWidget *parent = nullptr)
        : CollapsibleFrame(parent)
    {
        setFrameShape(QFrame::NoFrame);
        setCollapsible(false);

        auto *header = new QLabel(QStringLiteral("<i>Thought</i>"), this);
        QPalette hpal = header->palette();
        hpal.setColor(QPalette::WindowText,
                      Utils::creatorColor(Utils::Theme::PaletteTextDisabled));
        header->setPalette(hpal);
        m_headerLayout->addWidget(header, 1);

        m_label = new QLabel(this);
        m_label->setWordWrap(true);
        m_label->setTextFormat(Qt::PlainText);
        m_label->setText(text);
        QPalette pal = m_label->palette();
        pal.setColor(QPalette::WindowText,
                     Utils::creatorColor(Utils::Theme::PaletteTextDisabled));
        m_label->setPalette(pal);
        QFont f = m_label->font();
        f.setItalic(true);
        f.setPointSizeF(f.pointSizeF() * 0.9);
        m_label->setFont(f);
        m_bodyLayout->addWidget(m_label);
    }

    void appendText(const QString &text)
    {
        m_label->setText(m_label->text() + text);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, false);
        QPen pen(palette().color(QPalette::Mid), 2, Qt::DashLine);
        pen.setCapStyle(Qt::FlatCap);
        p.setPen(pen);
        p.drawLine(QPoint(1, 0), QPoint(1, height()));
    }

private:
    QLabel *m_label = nullptr;
};

// ---------------------------------------------------------------------------
// Tool call helpers
// ---------------------------------------------------------------------------

static QString toolCallStatusIcon(ToolCallStatus status)
{
    switch (status) {
    case ToolCallStatus::pending:
        return QStringLiteral("\u25CB");   // ○
    case ToolCallStatus::in_progress:
        return QStringLiteral("\u23F3");   // ⏳
    case ToolCallStatus::completed:
        return QStringLiteral("\u2713");   // ✓
    case ToolCallStatus::failed:
        return QStringLiteral("\u2717");   // ✗
    }
    return QStringLiteral("?");
}

static QColor toolCallBorderColor(ToolCallStatus status)
{
    switch (status) {
    case ToolCallStatus::pending:     return Qt::gray;
    case ToolCallStatus::in_progress: return QColor{0x1e, 0x90, 0xff}; // dodgerblue
    case ToolCallStatus::completed:   return Qt::green;
    case ToolCallStatus::failed:      return Qt::red;
    }
    return Qt::gray;
}

// ---------------------------------------------------------------------------
// ToolCallWidget
// ---------------------------------------------------------------------------

class ToolCallWidget : public CollapsibleFrame
{
public:
    explicit ToolCallWidget(ToolCallStatus status, const QString &title,
                            const QString &kindText = {}, QWidget *parent = nullptr)
        : CollapsibleFrame(parent)
    {
        setFrameShape(QFrame::NoFrame);

        m_statusLabel = new QLabel(this);
        m_headerLayout->addWidget(m_statusLabel);

        QString labelHtml = QStringLiteral("<b>%1</b>").arg(title.toHtmlEscaped());
        if (!kindText.isEmpty())
            labelHtml += QStringLiteral(" <small>[%1]</small>").arg(kindText.toHtmlEscaped());
        m_titleLabel = new QLabel(labelHtml, this);
        m_titleLabel->setTextFormat(Qt::RichText);
        m_headerLayout->addWidget(m_titleLabel, 1);

        applyStatus(status);
    }

    void applyStatus(ToolCallStatus status)
    {
        m_status = status;
        m_statusLabel->setText(toolCallStatusIcon(status));
        update();
    }

    ToolCallStatus status() const { return m_status; }

    void updateTitle(const QString &title)
    {
        m_titleLabel->setText(QStringLiteral("<b>%1</b>").arg(title.toHtmlEscaped()));
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        Utils::StyleHelper::drawCardBg(&p, rect(),
            Utils::creatorColor(Utils::Theme::ChatToolCallBackground),
            QPen(Qt::NoPen), RadiusS);
        QPainterPath clip;
        clip.addRoundedRect(rect(), RadiusS, RadiusS);
        p.setClipPath(clip);
        p.setRenderHint(QPainter::Antialiasing, false);
        p.fillRect(QRect(0, 0, 3, height()), toolCallBorderColor(m_status));
        p.setClipping(false);
    }

private:
    QLabel *m_statusLabel = nullptr;
    QLabel *m_titleLabel = nullptr;
    ToolCallStatus m_status = ToolCallStatus::pending;
};

// ---------------------------------------------------------------------------
// ToolCallGroupWidget — groups consecutive tool calls into one collapsible
// ---------------------------------------------------------------------------

class ToolCallGroupWidget : public CollapsibleFrame
{
public:
    explicit ToolCallGroupWidget(QWidget *parent = nullptr)
        : CollapsibleFrame(parent)
    {
        setFrameShape(QFrame::NoFrame);
        setCollapsed(true);

        // Use a vertical layout inside the header so the running-tools line
        // stacks below the counts line — QLabel word-wrap inside QHBoxLayout
        // has a known Qt issue where height-for-width negotiation fails.
        auto *headerVBox = new QVBoxLayout;
        headerVBox->setContentsMargins(0, 0, 0, 0);
        headerVBox->setSpacing(0);

        m_summaryLabel = new QLabel(this);
        m_summaryLabel->setTextFormat(Qt::RichText);
        headerVBox->addWidget(m_summaryLabel);

        m_runningLabel = new QLabel(this);
        m_runningLabel->setTextFormat(Qt::RichText);
        m_runningLabel->setWordWrap(true);
        m_runningLabel->setVisible(false);
        QFont smallFont = m_runningLabel->font();
        smallFont.setPointSizeF(smallFont.pointSizeF() * 0.9);
        m_runningLabel->setFont(smallFont);
        headerVBox->addWidget(m_runningLabel);

        m_headerLayout->addLayout(headerVBox, 1);

        m_innerLayout = new QVBoxLayout;
        m_innerLayout->setContentsMargins(0, 0, 0, 0);
        m_innerLayout->setSpacing(GapVXs);
        m_bodyLayout->addLayout(m_innerLayout);

        updateSummary();
    }

    void addChildWidget(QWidget *widget)
    {
        m_innerLayout->addWidget(widget);
        updateSummary();
    }

    void trackStatus(const QString &toolCallId, ToolCallStatus status)
    {
        m_statuses[toolCallId] = status;
        updateSummary();
    }

    void trackTitle(const QString &toolCallId, const QString &title)
    {
        m_titles[toolCallId] = title;
        updateSummary();
    }

    void updateSummary()
    {
        const int total = m_statuses.size();

        int completed = 0, failed = 0, inProgress = 0, pending = 0;
        QStringList runningNames;
        for (auto it = m_statuses.cbegin(); it != m_statuses.cend(); ++it) {
            switch (it.value()) {
            case ToolCallStatus::completed:   ++completed; break;
            case ToolCallStatus::failed:      ++failed; break;
            case ToolCallStatus::in_progress:
                ++inProgress;
                if (const QString title = m_titles.value(it.key()); !title.isEmpty())
                    runningNames << title;
                break;
            case ToolCallStatus::pending:
                ++pending;
                if (const QString title = m_titles.value(it.key()); !title.isEmpty())
                    runningNames << title;
                break;
            }
        }

        QString text = QStringLiteral("\u2699 <b>%1 tool call%2</b>")
                            .arg(total)
                            .arg(total != 1 ? QStringLiteral("s") : QString());

        QStringList parts;
        if (completed > 0)
            parts << QStringLiteral("<span style='color:green'>\u2713%1</span>").arg(completed);
        if (inProgress > 0)
            parts << QStringLiteral("<span style='color:dodgerblue'>\u23F3%1</span>").arg(inProgress);
        if (failed > 0)
            parts << QStringLiteral("<span style='color:red'>\u2717%1</span>").arg(failed);
        if (pending > 0)
            parts << QStringLiteral("<span style='color:gray'>\u25CB%1</span>").arg(pending);

        if (!parts.isEmpty())
            text += QStringLiteral("  ") + parts.join(QStringLiteral(" "));

        m_summaryLabel->setText(text);

        // Show currently running / pending tool names on a separate line
        if (!runningNames.isEmpty()) {
            m_runningLabel->setText(
                QStringLiteral("<span style='color:dodgerblue'>\u23F3 %1</span>")
                    .arg(runningNames.join(QStringLiteral(", ")).toHtmlEscaped()));
            m_runningLabel->setVisible(true);
        } else {
            m_runningLabel->setVisible(false);
        }
    }

private:
    QLabel *m_summaryLabel = nullptr;
    QLabel *m_runningLabel = nullptr;
    QVBoxLayout *m_innerLayout = nullptr;
    QHash<QString, ToolCallStatus> m_statuses;
    QHash<QString, QString> m_titles;

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        Utils::StyleHelper::drawCardBg(&p, rect(),
            Utils::creatorColor(Utils::Theme::ChatToolCallBackground));
    }
};

// ---------------------------------------------------------------------------
// PlanWidget
// ---------------------------------------------------------------------------

class PlanWidget : public CollapsibleFrame
{
public:
    explicit PlanWidget(const Plan &plan, QWidget *parent = nullptr)
        : CollapsibleFrame(parent)
    {
        setFrameShape(QFrame::NoFrame);

        auto *header = new QLabel(QStringLiteral("<b>Approach</b>"), this);
        m_headerLayout->addWidget(header, 1);

        QString listHtml = QStringLiteral(
            "<ul style='margin-top:2px; margin-bottom:2px; list-style-type:none; padding-left:4px;'>");
        for (const PlanEntry &entry : plan.entries()) {
            QString icon;
            switch (entry.status()) {
            case PlanEntryStatus::completed:
                icon = QStringLiteral("<span style='color:green'>\u2713</span>");
                break;
            case PlanEntryStatus::in_progress:
                icon = QStringLiteral("<span style='color:dodgerblue'>\u25D4</span>");
                break;
            case PlanEntryStatus::pending:
                icon = QStringLiteral("<span style='color:gray'>\u25CB</span>");
                break;
            }
            listHtml += QStringLiteral("<li style='margin-bottom:3px;'>%1 %2</li>")
                            .arg(icon, entry.content().toHtmlEscaped());
        }
        listHtml += QStringLiteral("</ul>");

        auto *list = new QLabel(listHtml, this);
        list->setTextFormat(Qt::RichText);
        list->setWordWrap(true);
        m_bodyLayout->addWidget(list);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        Utils::StyleHelper::drawCardBg(&p, rect(),
            Utils::creatorColor(Utils::Theme::ChatPlanBackground));
    }
};

// ---------------------------------------------------------------------------
// PermissionRequestWidget — inline widget for permission requests
// ---------------------------------------------------------------------------

class PermissionRequestWidget : public CollapsibleFrame
{
public:
    explicit PermissionRequestWidget(const QString &title, const QString &kindText,
                                     QWidget *parent = nullptr)
        : CollapsibleFrame(parent)
    {
        setFrameShape(QFrame::NoFrame);
        setCollapsible(false);

        auto *icon = new QLabel(QStringLiteral("\u26A0"), this); // ⚠
        m_headerLayout->addWidget(icon);

        QString labelHtml = QStringLiteral("<b>Permission Request</b>");
        if (!title.isEmpty())
            labelHtml += QStringLiteral(": %1").arg(title.toHtmlEscaped());
        if (!kindText.isEmpty())
            labelHtml += QStringLiteral(" <small>[%1]</small>").arg(kindText.toHtmlEscaped());
        auto *label = new QLabel(labelHtml, this);
        label->setTextFormat(Qt::RichText);
        label->setWordWrap(true);
        m_headerLayout->addWidget(label, 1);

        m_buttonLayout = new QHBoxLayout;
        m_buttonLayout->setSpacing(6);
        m_bodyLayout->addLayout(m_buttonLayout);

        m_statusLabel = new QLabel(this);
        m_statusLabel->setTextFormat(Qt::RichText);
        m_statusLabel->hide();
        m_bodyLayout->addWidget(m_statusLabel);
    }

    QPushButton *addOptionButton(const QString &text, Acp::PermissionOptionKind kind)
    {
        auto *button = new QPushButton(text, this);
        // Style reject options with a muted appearance
        if (kind == Acp::PermissionOptionKind::reject_once
            || kind == Acp::PermissionOptionKind::reject_always) {
            button->setFlat(true);
        }
        m_buttonLayout->addWidget(button);
        m_buttons.append(button);
        return button;
    }

    void finishButtonLayout()
    {
        m_buttonLayout->addStretch();
    }

    void setResolved(const QString &text)
    {
        for (QPushButton *button : m_buttons)
            button->setEnabled(false);
        m_statusLabel->setText(QStringLiteral("<i>%1</i>").arg(text.toHtmlEscaped()));
        m_statusLabel->show();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QColor bg = Utils::creatorColor(Utils::Theme::ChatToolCallBackground);
        Utils::StyleHelper::drawCardBg(&p, rect(), bg, QPen(Qt::NoPen),
                                       Utils::StyleHelper::SpacingTokens::RadiusS);
        // Orange left border to distinguish from regular tool calls
        QPainterPath clip;
        clip.addRoundedRect(rect(), Utils::StyleHelper::SpacingTokens::RadiusS,
                            Utils::StyleHelper::SpacingTokens::RadiusS);
        p.setClipPath(clip);
        p.setRenderHint(QPainter::Antialiasing, false);
        p.fillRect(QRect(0, 0, 3, height()), QColor{0xe6, 0x9c, 0x24}); // amber
        p.setClipping(false);
    }

private:
    QHBoxLayout *m_buttonLayout = nullptr;
    QLabel *m_statusLabel = nullptr;
    QList<QPushButton *> m_buttons;
};

// ---------------------------------------------------------------------------
// AuthenticationWidget — inline widget for authentication flow
// ---------------------------------------------------------------------------

class AuthenticationWidget : public CollapsibleFrame
{
    Q_OBJECT

public:
    explicit AuthenticationWidget(const QList<Acp::AuthMethod> &methods,
                                  QWidget *parent = nullptr)
        : CollapsibleFrame(parent)
    {
        setFrameShape(QFrame::NoFrame);
        setCollapsible(false);

        auto *icon = new QLabel(QStringLiteral("\U0001F512"), this); // 🔒
        m_headerLayout->addWidget(icon);

        auto *label = new QLabel(QStringLiteral("<b>Authentication Required</b>"), this);
        label->setTextFormat(Qt::RichText);
        m_headerLayout->addWidget(label, 1);

        // Method selector (only shown if multiple methods)
        m_methodCombo = new QComboBox(this);
        for (const Acp::AuthMethod &method : methods)
            m_methodCombo->addItem(method.name(), method.id());
        m_methodCombo->setVisible(methods.size() > 1);
        m_bodyLayout->addWidget(m_methodCombo);

        // Description label
        m_descriptionLabel = new QLabel(this);
        m_descriptionLabel->setWordWrap(true);
        QPalette descPal = m_descriptionLabel->palette();
        descPal.setColor(QPalette::WindowText, descPal.color(QPalette::PlaceholderText));
        m_descriptionLabel->setPalette(descPal);
        m_descriptionLabel->hide();
        m_bodyLayout->addWidget(m_descriptionLabel);

        // Error label
        m_errorLabel = new QLabel(this);
        m_errorLabel->setWordWrap(true);
        QPalette errorPal = m_errorLabel->palette();
        errorPal.setColor(QPalette::WindowText, QColor(0xfc, 0x8c, 0x8c));
        m_errorLabel->setPalette(errorPal);
        m_errorLabel->hide();
        m_bodyLayout->addWidget(m_errorLabel);

        // Buttons
        auto *buttonLayout = new QHBoxLayout;
        buttonLayout->setSpacing(6);

        m_authButton = new QPushButton(
            AcpMessageView::tr("Authenticate"), this);
        buttonLayout->addWidget(m_authButton);
        buttonLayout->addStretch();
        m_bodyLayout->addLayout(buttonLayout);

        // Status label (shown after resolution)
        m_statusLabel = new QLabel(this);
        m_statusLabel->setTextFormat(Qt::RichText);
        m_statusLabel->hide();
        m_bodyLayout->addWidget(m_statusLabel);

        // Update description when method changes
        auto updateDescription = [this, methods] {
            const int idx = m_methodCombo->currentIndex();
            if (idx >= 0 && idx < methods.size()) {
                const auto &desc = methods.at(idx).description();
                if (desc.has_value() && !desc->isEmpty()) {
                    m_descriptionLabel->setText(*desc);
                    m_descriptionLabel->show();
                    return;
                }
            }
            m_descriptionLabel->hide();
        };
        updateDescription();
        QObject::connect(m_methodCombo, &QComboBox::currentIndexChanged,
                         this, updateDescription);

        QObject::connect(m_authButton, &QPushButton::clicked, this, [this] {
            m_errorLabel->hide();
            const QString methodId = m_methodCombo->currentData().toString();
            if (!methodId.isEmpty())
                emit authenticateRequested(methodId);
        });
    }

    void showError(const QString &error)
    {
        m_errorLabel->setText(error);
        m_errorLabel->show();
        m_authButton->setEnabled(true);
    }

    void setResolved()
    {
        m_authButton->setEnabled(false);
        m_methodCombo->setEnabled(false);
        m_errorLabel->hide();
        m_statusLabel->setText(QStringLiteral("<i>%1</i>")
            .arg(AcpMessageView::tr("Authenticated").toHtmlEscaped()));
        m_statusLabel->show();
    }

signals:
    void authenticateRequested(const QString &methodId);

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QColor bg = Utils::creatorColor(Utils::Theme::ChatToolCallBackground);
        Utils::StyleHelper::drawCardBg(&p, rect(), bg, QPen(Qt::NoPen), RadiusS);
        QPainterPath clip;
        clip.addRoundedRect(rect(), RadiusS, RadiusS);
        p.setClipPath(clip);
        p.setRenderHint(QPainter::Antialiasing, false);
        p.fillRect(QRect(0, 0, 3, height()), QColor{0x1e, 0x90, 0xff}); // blue
        p.setClipping(false);
    }

private:
    QComboBox *m_methodCombo = nullptr;
    QLabel *m_descriptionLabel = nullptr;
    QLabel *m_errorLabel = nullptr;
    QPushButton *m_authButton = nullptr;
    QLabel *m_statusLabel = nullptr;
};

// ---------------------------------------------------------------------------
// AcpMessageView
// ---------------------------------------------------------------------------

AcpMessageView::AcpMessageView(QWidget *parent)
    : QScrollArea(parent)
{
    setWidgetResizable(true);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_container = new QWidget(this);
    m_layout = new QVBoxLayout(m_container);
    m_layout->setContentsMargins(PaddingHXl, PaddingHXl, PaddingHXl, PaddingHXl);
    m_layout->setSpacing(GapVL);
    m_layout->addStretch();

    setWidget(m_container);

    // Track whether user has scrolled up to suppress auto-scroll
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int value) {
        m_autoScroll = (value >= verticalScrollBar()->maximum() - 10);
    });
    connect(verticalScrollBar(), &QScrollBar::rangeChanged, this, &AcpMessageView::scrollToBottom);
}

void AcpMessageView::setDetailedMode(bool detailed)
{
    m_detailedMode = detailed;
}

void AcpMessageView::setAgentIconUrl(const QString &iconUrl)
{
    m_agentIconUrl = iconUrl;
}

void AcpMessageView::clear()
{
    // Remove all widgets except the bottom stretch
    while (m_layout->count() > 1) {
        QLayoutItem *item = m_layout->takeAt(0);
        if (item->widget())
            delete item->widget();
        delete item;
    }

    m_currentAgentWidget = nullptr;
    m_currentThoughtWidget = nullptr;
    m_currentToolCallGroup = nullptr;
    m_currentAuthWidget = nullptr;
    m_toolCallWidgets.clear();
    m_toolCallDetailWidgets.clear();
    m_toolCallGroups.clear();
    m_autoScroll = true;
}

void AcpMessageView::addUserMessage(const QString &text)
{
    finishAgentMessage();
    finishToolCallGroup();
    auto *msg = new MessageWidget(MessageWidget::User, m_container);
    msg->setText(text);
    addWidget(wrapWithSpacer(msg, Qt::AlignRight));
    m_autoScroll = true; // reset auto-scroll when user sends a message to ensure they see the response
    scrollToBottom();
}

void AcpMessageView::appendAgentText(const QString &text)
{
    m_currentThoughtWidget = nullptr;
    finishToolCallGroup();
    if (!m_currentAgentWidget) {
        m_currentAgentWidget = new MessageWidget(MessageWidget::Agent, m_container);
        m_currentAgentWidget->appendText(text);
        addWidget(wrapWithSpacer(m_currentAgentWidget, Qt::AlignLeft));
    } else {
        m_currentAgentWidget->appendText(text);
    }
}

void AcpMessageView::appendAgentThought(const QString &text)
{
    if (!m_currentThoughtWidget) {
        finishAgentMessage();
        finishToolCallGroup();
        m_currentThoughtWidget = new ThoughtWidget(text, m_container);
        addWidget(m_currentThoughtWidget);
    } else {
        m_currentThoughtWidget->appendText(text);
    }
}

ToolCallGroupWidget *AcpMessageView::ensureToolCallGroup()
{
    if (!m_currentToolCallGroup) {
        m_currentToolCallGroup = new ToolCallGroupWidget(m_container);
        addWidget(m_currentToolCallGroup);
    }
    return m_currentToolCallGroup;
}

void AcpMessageView::finishToolCallGroup()
{
    if (m_currentToolCallGroup) {
        m_currentToolCallGroup->updateSummary();
        m_currentToolCallGroup = nullptr;
    }
}

void AcpMessageView::addToolCall(const ToolCall &toolCall)
{
    finishAgentMessage();

    auto *group = ensureToolCallGroup();
    const ToolCallStatus status = toolCall.status().value_or(ToolCallStatus::in_progress);
    group->trackStatus(toolCall.toolCallId(), status);
    group->trackTitle(toolCall.toolCallId(), toolCall.title());
    m_toolCallGroups[toolCall.toolCallId()] = group;

    if (m_detailedMode) {
        auto *detail = new ToolCallDetailWidget(toolCall, group);
        detail->setContentMaxWidth(contentMaxWidth());
        group->addChildWidget(detail);
        m_toolCallDetailWidgets[toolCall.toolCallId()] = detail;
    } else {
        const QString kindText = toolCall.kind() ? toString(*toolCall.kind()) : QString();
        auto *widget = new ToolCallWidget(status, toolCall.title(), kindText, group);
        widget->setCollapsible(false);
        group->addChildWidget(widget);
        m_toolCallWidgets[toolCall.toolCallId()] = widget;
    }
}

void AcpMessageView::updateToolCall(const ToolCallUpdate &update)
{
    auto updateGroup = [&](const QString &id, std::optional<ToolCallStatus> status,
                           std::optional<QString> title) {
        if (auto *group = m_toolCallGroups.value(id)) {
            if (status)
                group->trackStatus(id, *status);
            if (title)
                group->trackTitle(id, *title);
        }
    };

    if (m_detailedMode) {
        ToolCallDetailWidget *detail = m_toolCallDetailWidgets.value(update.toolCallId());
        if (!detail) {
            const QString title = update.title().value_or(QStringLiteral("Tool Call"));
            ToolCall tc;
            tc.toolCallId(update.toolCallId());
            tc.title(title);
            tc.status(update.status().value_or(ToolCallStatus::in_progress));
            tc.kind(update.kind());
            auto *group = ensureToolCallGroup();
            group->trackStatus(update.toolCallId(),
                               update.status().value_or(ToolCallStatus::in_progress));
            group->trackTitle(update.toolCallId(), title);
            m_toolCallGroups[update.toolCallId()] = group;
            detail = new ToolCallDetailWidget(tc, group);
            detail->setContentMaxWidth(contentMaxWidth());
            group->addChildWidget(detail);
            m_toolCallDetailWidgets[update.toolCallId()] = detail;
        }
        detail->updateContent(update);
        updateGroup(update.toolCallId(), update.status(), update.title());
        return;
    }

    ToolCallWidget *widget = m_toolCallWidgets.value(update.toolCallId());
    if (!widget) {
        const ToolCallStatus status = update.status().value_or(ToolCallStatus::in_progress);
        const QString title = update.title().value_or(QStringLiteral("Tool Call"));
        const QString kindText = update.kind() ? toString(*update.kind()) : QString();
        auto *group = ensureToolCallGroup();
        group->trackStatus(update.toolCallId(), status);
        group->trackTitle(update.toolCallId(), title);
        m_toolCallGroups[update.toolCallId()] = group;
        widget = new ToolCallWidget(status, title, kindText, group);
        widget->setCollapsible(false);
        group->addChildWidget(widget);
        m_toolCallWidgets[update.toolCallId()] = widget;
        return;
    }

    if (const auto status = update.status())
        widget->applyStatus(*status);
    if (const auto title = update.title())
        widget->updateTitle(*title);
    updateGroup(update.toolCallId(), update.status(), update.title());
}

void AcpMessageView::addPlan(const Plan &plan)
{
    finishAgentMessage();
    finishToolCallGroup();
    addWidget(new PlanWidget(plan, m_container));
}

void AcpMessageView::addPermissionRequest(const QJsonValue &id,
                                           const RequestPermissionRequest &request)
{
    finishAgentMessage();
    finishToolCallGroup();

    const ToolCallUpdate &toolCall = request.toolCall();
    const QString title = toolCall.title().value_or(QString());
    const QString kindText = toolCall.kind() ? toString(*toolCall.kind()) : QString();

    auto *widget = new PermissionRequestWidget(title, kindText, m_container);

    const QList<PermissionOption> options = request.options();
    for (const PermissionOption &option : options) {
        auto *button = widget->addOptionButton(option.name(), option.kind());
        connect(button, &QPushButton::clicked, this,
                [this, id, optionId = option.optionId(), widget] {
            widget->setResolved(tr("Approved"));
            emit permissionOptionSelected(id, optionId);
        });
    }

    // Add a deny/cancel button
    auto *cancelButton = widget->addOptionButton(tr("Deny"),
                                                  PermissionOptionKind::reject_once);
    widget->finishButtonLayout();
    connect(cancelButton, &QPushButton::clicked, this,
            [this, id, widget] {
        widget->setResolved(tr("Denied"));
        emit permissionCancelled(id);
    });

    addWidget(widget);
}

void AcpMessageView::addAuthenticationRequest(const QList<Acp::AuthMethod> &methods)
{
    finishAgentMessage();
    finishToolCallGroup();

    m_currentAuthWidget = new AuthenticationWidget(methods, m_container);
    connect(m_currentAuthWidget, &AuthenticationWidget::authenticateRequested,
            this, &AcpMessageView::authenticateRequested);
    addWidget(m_currentAuthWidget);
}

void AcpMessageView::showAuthenticationError(const QString &error)
{
    if (m_currentAuthWidget)
        m_currentAuthWidget->showError(error);
}

void AcpMessageView::resolveAuthentication()
{
    if (m_currentAuthWidget) {
        m_currentAuthWidget->setResolved();
        m_currentAuthWidget = nullptr;
    }
}

void AcpMessageView::addErrorMessage(const QString &text)
{
    finishAgentMessage();
    finishToolCallGroup();

    auto *widget = new CollapsibleFrame(m_container);
    widget->setFrameShape(QFrame::NoFrame);
    widget->setCollapsible(false);

    auto *icon = new QLabel(QStringLiteral("\u26A0"), widget); // ⚠
    QPalette iconPal = icon->palette();
    iconPal.setColor(QPalette::WindowText, QColor(0xef, 0x44, 0x44));
    icon->setPalette(iconPal);
    QFont iconFont = icon->font();
    iconFont.setPointSizeF(iconFont.pointSizeF() * 1.2);
    icon->setFont(iconFont);
    widget->headerLayout()->addWidget(icon);

    auto *label = new QLabel(QStringLiteral("<b>Error:</b> %1").arg(text.toHtmlEscaped()), widget);
    label->setTextFormat(Qt::RichText);
    label->setWordWrap(true);
    QPalette pal = label->palette();
    pal.setColor(QPalette::WindowText, QColor(0xfc, 0x8c, 0x8c));
    label->setPalette(pal);
    widget->headerLayout()->addWidget(label, 1);

    addWidget(widget);
}

void AcpMessageView::finishAgentMessage()
{
    if (m_currentAgentWidget)
        m_currentAgentWidget->flush();
    m_currentAgentWidget = nullptr;
    m_currentThoughtWidget = nullptr;
}

void AcpMessageView::scrollToBottom()
{
    if (m_autoScroll)
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}

QWidget *AcpMessageView::wrapWithSpacer(QWidget *widget, Qt::Alignment side)
{
    auto *row = new QWidget(m_container);
    auto *hbox = new QHBoxLayout(row);
    hbox->setContentsMargins(0, 0, 0, 0);
    hbox->setSpacing(GapHM);
    // Widget takes as much space as it needs (Preferred) but won't grow
    // beyond its sizeHint (Maximum). The expanding spacer absorbs the rest.
    widget->setSizePolicy(QSizePolicy::Maximum, widget->sizePolicy().verticalPolicy());
    if (side == Qt::AlignRight) {
        hbox->addSpacerItem(new QSpacerItem(60, 0, QSizePolicy::MinimumExpanding));
        hbox->addWidget(widget);
        auto *avatar = new AvatarWidget(AvatarWidget::User, row);
        hbox->addWidget(avatar, 0, Qt::AlignTop);
    } else {
        auto *avatar = new AvatarWidget(AvatarWidget::Agent, row);
        Utils::onResultReady(AcpSettings::iconForUrl(m_agentIconUrl), this, [avatar](const QIcon &icon){
            avatar->setIcon(icon);
        });
        hbox->addWidget(avatar, 0, Qt::AlignTop);
        hbox->addWidget(widget);
        hbox->addSpacerItem(new QSpacerItem(60, 0, QSizePolicy::MinimumExpanding));
    }
    return row;
}

void AcpMessageView::addWidget(QWidget *widget)
{
    // Insert before the bottom stretch item
    m_layout->insertWidget(m_layout->count() - 1, widget);
}

int AcpMessageView::contentMaxWidth() const
{
    const QMargins m = m_layout->contentsMargins();
    // Subtract ToolCallGroupWidget body margins (one CollapsibleFrame level);
    // ToolCallDetailWidget subtracts its own body margins internally.
    constexpr int groupMargins = CollapsibleFrame::BodyMarginLeft
                                 + CollapsibleFrame::BodyMarginRight;
    return viewport()->width() - m.left() - m.right() - groupMargins;
}

void AcpMessageView::resizeEvent(QResizeEvent *event)
{
    QScrollArea::resizeEvent(event);
    const int maxW = contentMaxWidth();
    for (ToolCallDetailWidget *detail : m_toolCallDetailWidgets)
        detail->setContentMaxWidth(maxW);
}

} // namespace AcpClient::Internal

#include "acpmessageview.moc"
