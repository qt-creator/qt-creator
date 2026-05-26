// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acpmessageview.h"

#include "acpclienttr.h"
#include "acpsettings.h"
#include "collapsibleframe.h"
#include "sessionpickerwidget.h"
#include "toolcalldetailwidget.h"

#include <utils/async.h>
#include <aggregation/aggregate.h>

#include <coreplugin/find/ifindsupport.h>
#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>
#include <utils/markdownbrowser.h>
#include <utils/progressindicator.h>
#include <utils/qtcassert.h>
#include <utils/qtcwidgets.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/elidinglabel.h>
#include <utils/utilsicons.h>

#include <limits>

#include <QAbstractTextDocumentLayout>
#include <QComboBox>
#include <QDateTime>
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
#include <QElapsedTimer>

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

        connect(m_browser->document(), &QTextDocument::contentsChanged,
                this, [this] { m_cachedUnwrappedIdealWidth = -1; });
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
        QTextDocument *doc = m_browser->document();
        if (m_cachedUnwrappedIdealWidth < 0) {
            // Temporarily remove the text width constraint so idealWidth() returns
            // the true single-line (unwrapped) width of the document. Each toggle
            // triggers a full relayout, so cache the result until contents change.
            const qreal savedTextWidth = doc->textWidth();
            doc->setTextWidth(-1);
            m_cachedUnwrappedIdealWidth = doc->idealWidth();
            doc->setTextWidth(savedTextWidth);
        }
        if (m_cachedUnwrappedIdealWidth <= 0)
            return CollapsibleFrame::sizeHint();
        const QMargins bm = m_bodyLayout->contentsMargins();
        const QMargins bcm = m_browser->contentsMargins();
        const int docMargin = static_cast<int>(doc->documentMargin());
        const int contentWidth = qCeil(m_cachedUnwrappedIdealWidth) + bm.left() + bm.right()
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
    mutable qreal m_cachedUnwrappedIdealWidth = -1;
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
        setCollapsible(true);

        auto *header = new QLabel(QStringLiteral("<i>Thought</i>"), this);
        QPalette hpal = header->palette();
        hpal.setColor(QPalette::WindowText,
                      Utils::creatorColor(Utils::Theme::Token_Text_Subtle));
        setIndicatorColor(Utils::Theme::Token_Text_Subtle);
        header->setPalette(hpal);
        m_headerLayout->addWidget(header, 1);

        m_label = new QLabel(this);
        m_label->setWordWrap(true);
        m_label->setTextFormat(Qt::PlainText);
        m_label->setText(text);
        QPalette pal = m_label->palette();
        pal.setColor(QPalette::WindowText,
                     Utils::creatorColor(Utils::Theme::Token_Text_Subtle));
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
        QPen pen(Utils::creatorColor(Utils::Theme::Token_Stroke_Subtle), 2, Qt::SolidLine);
        pen.setCapStyle(Qt::FlatCap);
        p.setPen(pen);
        p.drawLine(QPointF(1, 0), QPointF(1, height()));
    }

private:
    QLabel *m_label = nullptr;
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

        auto *headerVBox = new QVBoxLayout;
        headerVBox->setContentsMargins(0, 0, 0, 0);
        headerVBox->setSpacing(0);

        // Summary row: main icon + "N tool calls" + per-status icon+count pairs
        auto *summaryRow = new QHBoxLayout;
        summaryRow->setContentsMargins(0, 0, 0, 0);
        summaryRow->setSpacing(GapHXs);

        m_summaryIcon = new Utils::QtcIconDisplay(this);
        summaryRow->addWidget(m_summaryIcon);

        m_summaryLabel = new QLabel(this);
        summaryRow->addWidget(m_summaryLabel);

        // Per-status icon + count widgets (hidden by default)
        static constexpr ToolCallStatus statusMap[StatusCount] = {
            ToolCallStatus::completed,
            ToolCallStatus::in_progress,
            ToolCallStatus::failed,
            ToolCallStatus::pending,
        };
        for (int i = 0; i < StatusCount; ++i) {
            m_statusIcons[i] = toolCallStatusWidget(statusMap[i], this);
            m_statusIcons[i]->setVisible(false);
            summaryRow->addWidget(m_statusIcons[i]);

            m_statusLabels[i] = new QLabel(this);
            m_statusLabels[i]->setVisible(false);
            summaryRow->addWidget(m_statusLabels[i]);
        }

        summaryRow->addStretch(1);
        headerVBox->addLayout(summaryRow);

        // Running-tools row
        auto *runningRow = new QHBoxLayout;
        runningRow->setContentsMargins(0, 0, 0, 0);
        runningRow->setSpacing(GapHXs);

        m_runningIcon = toolCallStatusWidget(ToolCallStatus::in_progress, this);
        m_runningIcon->setVisible(false);
        runningRow->addWidget(m_runningIcon);

        m_runningLabel = new QLabel(this);
        m_runningLabel->setWordWrap(true);
        m_runningLabel->setVisible(false);
        QFont smallFont = m_runningLabel->font();
        smallFont.setPointSizeF(smallFont.pointSizeF() * 0.9);
        m_runningLabel->setFont(smallFont);
        runningRow->addWidget(m_runningLabel, 1);
        headerVBox->addLayout(runningRow);

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

        int counts[StatusCount] = {};
        QStringList runningNames;
        for (auto it = m_statuses.cbegin(); it != m_statuses.cend(); ++it) {
            switch (it.value()) {
            case ToolCallStatus::completed:   ++counts[Completed]; break;
            case ToolCallStatus::failed:      ++counts[Failed]; break;
            case ToolCallStatus::in_progress:
                ++counts[InProgress];
                if (const QString title = m_titles.value(it.key()); !title.isEmpty())
                    runningNames << title;
                break;
            case ToolCallStatus::pending:
                ++counts[Pending];
                if (const QString title = m_titles.value(it.key()); !title.isEmpty())
                    runningNames << title;
                break;
            }
        }

        m_summaryIcon->setIcon(Utils::Icons::PROJECT); // wrench icon
        m_summaryLabel->setText(QStringLiteral("<b>%1 tool call%2</b>")
                                    .arg(total)
                                    .arg(total != 1 ? QStringLiteral("s") : QString()));

        for (int i = 0; i < StatusCount; ++i) {
            const bool visible = counts[i] > 0;
            m_statusIcons[i]->setVisible(visible);
            m_statusLabels[i]->setVisible(visible);
            if (visible)
                m_statusLabels[i]->setText(QString::number(counts[i]));
        }

        // Show currently running / pending tool names on a separate line
        const bool hasRunning = !runningNames.isEmpty();
        m_runningIcon->setVisible(hasRunning);
        m_runningLabel->setVisible(hasRunning);
        if (hasRunning)
            m_runningLabel->setText(runningNames.join(QStringLiteral(", ")));
    }

private:
    enum StatusIndex { Completed, InProgress, Failed, Pending, StatusCount };

    Utils::QtcIconDisplay *m_summaryIcon = nullptr;
    QLabel *m_summaryLabel = nullptr;
    QWidget *m_statusIcons[StatusCount] = {};
    QLabel *m_statusLabels[StatusCount] = {};
    QWidget *m_runningIcon = nullptr;
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

static QString optionKindToUiString(Acp::PermissionOptionKind kind)
{
    switch (kind) {
    case Acp::PermissionOptionKind::allow_once:
        return Tr::tr("Allow");
    case Acp::PermissionOptionKind::allow_always:
        return Tr::tr("Allow Always");
    case Acp::PermissionOptionKind::reject_once:
        return Tr::tr("Reject");
    case Acp::PermissionOptionKind::reject_always:
        return Tr::tr("Reject Always");
    }
    return {};
}

// ---------------------------------------------------------------------------
// PermissionRequestWidget — inline widget for permission requests
// ---------------------------------------------------------------------------

class PermissionRequestWidget : public CollapsibleFrame
{
public:
    explicit PermissionRequestWidget(const QString &title, const QString &kindText,
                                     const QString &command, QWidget *parent = nullptr)
        : CollapsibleFrame(parent)
    {
        setFrameShape(QFrame::NoFrame);
        setCollapsible(false);

        m_iconDisplay = new Utils::QtcIconDisplay(this);
        m_iconDisplay->setIcon(Utils::Icons::WARNING);
        m_headerLayout->addWidget(m_iconDisplay);

        auto *headerLabel = new QLabel(Tr::tr("Permission Request"), this);
        QFont boldFont = headerLabel->font();
        boldFont.setBold(true);
        headerLabel->setFont(boldFont);
        headerLabel->setWordWrap(true);
        m_headerLayout->addWidget(headerLabel, 1);

        if (!kindText.isEmpty()) {
            auto *kindLabel = new QLabel(QStringLiteral("[%1]").arg(kindText), this);
            QFont smallFont = kindLabel->font();
            smallFont.setPointSizeF(smallFont.pointSizeF() * 0.85);
            kindLabel->setFont(smallFont);
            kindLabel->setWordWrap(true);
            m_headerLayout->addWidget(kindLabel);
        }

        if (!title.isEmpty()) {
            auto *titleLabel = new Utils::ElidingLabel(title, this);
            titleLabel->setWordWrap(true);
            m_bodyLayout->addWidget(titleLabel);
        }
        if (!command.isEmpty()) {
            auto *commandLabel = new QLabel(command, this);
            commandLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
            commandLabel->setWordWrap(true);
            QFont mono = commandLabel->font();
            mono.setStyleHint(QFont::Monospace);
            mono.setFamily(QFont(QStringLiteral("monospace")).defaultFamily());
            commandLabel->setFont(mono);
            m_bodyLayout->addWidget(commandLabel);
        }
        auto *buttonWidget = new QWidget(this);
        Layouting::Flow{}.attachTo(buttonWidget);
        m_buttonLayout = buttonWidget->layout();
        m_bodyLayout->addWidget(buttonWidget);

        m_statusLabel = new Utils::ElidingLabel(this);
        m_statusLabel->hide();
        m_headerLayout->addWidget(m_statusLabel);
    }

    Utils::QtcButton *addOptionButton(const QString &text, Acp::PermissionOptionKind kind)
    {
        const bool reject = kind == Acp::PermissionOptionKind::reject_once
                            || kind == Acp::PermissionOptionKind::reject_always;
        const auto role = reject ? Utils::QtcButton::MediumTertiary
                                 : Utils::QtcButton::MediumPrimary;
        auto *button = new Utils::QtcButton(optionKindToUiString(kind), role, this);
        button->setToolTip(text);
        m_buttonLayout->addWidget(button);
        m_buttons.append(button);
        return button;
    }

    void setResolved(const QString &text, bool accepted)
    {
        for (Utils::QtcButton *button : std::as_const(m_buttons))
            button->hide();
        m_iconDisplay->setIcon(accepted ? Utils::Icons::OK : Utils::Icons::CRITICAL);
        m_statusLabel->setText(QStringLiteral("<i>%1</i>").arg(text.toHtmlEscaped()));
        m_statusLabel->show();
        setCollapsible(true);
        setCollapsed(true);
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
    Utils::QtcIconDisplay *m_iconDisplay = nullptr;
    QLayout *m_buttonLayout = nullptr;
    Utils::ElidingLabel *m_statusLabel = nullptr;
    QList<Utils::QtcButton *> m_buttons;
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
        for (const Acp::AuthMethod &method : methods) {
            const auto *agent = std::get_if<Acp::AuthMethodAgent>(&method);
            QTC_ASSERT(agent, continue);
            m_methodCombo->addItem(agent->name(), agent->id());
        }
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

        m_authButton = new QPushButton(Tr::tr("Authenticate"), this);
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
                const auto *agent = std::get_if<Acp::AuthMethodAgent>(&methods.at(idx));
                QTC_ASSERT(agent, return);
                const auto &desc = agent->description();
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
        m_statusLabel->setText(
            QStringLiteral("<i>%1</i>").arg(Tr::tr("Authenticated").toHtmlEscaped()));
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
// MessageViewFindSupport searches across all MarkdownBrowser widgets
// ---------------------------------------------------------------------------

class MessageViewFindSupport : public Core::IFindSupport
{
public:
    explicit MessageViewFindSupport(AcpMessageView *view)
        : m_view(view)
    {}

    bool supportsReplace() const override { return false; }

    Utils::FindFlags supportedFindFlags() const override
    {
        return Utils::FindCaseSensitively | Utils::FindWholeWords
               | Utils::FindRegularExpression;
    }

    void resetIncrementalSearch() override { m_incrementalStart = {nullptr, {}}; }

    void highlightAll(const QString &txt, Utils::FindFlags findFlags) override
    {
        if (txt.isEmpty()) {
            clearHighlights();
            return;
        }
        const QTextDocument::FindFlags flags = toDocFlags(findFlags);
        QTextCharFormat format;
        format.setBackground(
            Utils::creatorColor(Utils::Theme::OutputPanes_SearchResultBackgroundColor));
        for (auto *browser : browsers()) {
            QList<QTextEdit::ExtraSelection> selections;
            int pos = 0;
            QTextCursor found = findInBrowser(browser, txt, pos, findFlags, flags);
            while (!found.isNull()) {
                selections.append({found, format});
                pos = found.selectionEnd();
                if (found.position() == found.anchor())
                    ++pos; // Avoid infinite loop on zero-length matches
                found = findInBrowser(browser, txt, pos, findFlags, flags);
            }
            browser->setExtraSelections(selections);
        }
    }

    void clearHighlights() override
    {
        for (auto *browser : browsers())
            browser->setExtraSelections({});
    }

    QString currentFindString() const override { return {}; }
    QString completedFindString() const override { return {}; }

    Result findIncremental(const QString &txt, Utils::FindFlags findFlags) override
    {
        if (txt.isEmpty()) {
            clearHighlights();
            return NotFound;
        }

        if (m_incrementalStart.first == nullptr) {
            if (auto *browser = topmostVisibleBrowser()) {
                QWidget *viewport = m_view->viewport();
                const int browserTopInViewport = browser->mapTo(viewport, QPoint(0, 0)).y();
                const QPoint visibleTopInBrowser(0, qMax(0, -browserTopInViewport));
                m_incrementalStart = {browser, browser->cursorForPosition(visibleTopInBrowser)};
            }
        }

        Result result = findText(txt, findFlags, true);
        highlightAll(result == Found ? txt : QString(), findFlags);
        return result;
    }

    Result findStep(const QString &txt, Utils::FindFlags findFlags) override
    {
        if (txt.isEmpty())
            return NotFound;
        return findText(txt, findFlags, false);
    }

private:
    QList<Utils::MarkdownBrowser *> browsers() const
    {
        return m_view->findChildren<Utils::MarkdownBrowser *>();
    }

    Utils::MarkdownBrowser *topmostVisibleBrowser() const
    {
        QWidget *viewport = m_view->viewport();
        const QRect viewportRect = viewport->rect();
        Utils::MarkdownBrowser *topmost = nullptr;
        int topmostY = std::numeric_limits<int>::max();
        for (auto *browser : browsers()) {
            const QRect browserRect(browser->mapTo(viewport, QPoint(0, 0)), browser->size());
            if (browserRect.intersects(viewportRect) && browserRect.top() < topmostY) {
                topmostY = browserRect.top();
                topmost = browser;
            }
        }
        return topmost;
    }

    static QRegularExpression searchRegex(const QString &txt, Utils::FindFlags findFlags)
    {
        return QRegularExpression(
            (findFlags & Utils::FindRegularExpression) ? txt : QRegularExpression::escape(txt),
            (findFlags & Utils::FindCaseSensitively) ? QRegularExpression::NoPatternOption
                                                     : QRegularExpression::CaseInsensitiveOption);
    }

    static QTextDocument::FindFlags toDocFlags(Utils::FindFlags findFlags)
    {
        QTextDocument::FindFlags flags;
        if (findFlags & Utils::FindWholeWords)
            flags |= QTextDocument::FindWholeWords;
        return flags;
    }

    QTextCursor findInBrowser(
        const Utils::MarkdownBrowser *browser,
        const QString &txt,
        int startPos,
        Utils::FindFlags findFlags,
        QTextDocument::FindFlags docFlags)
    {
        if (startPos < 0)
            startPos = findFlags & Utils::FindBackward ? browser->document()->characterCount() - 1 : 0;
        return browser->document()->find(searchRegex(txt, findFlags), startPos, docFlags);
    }

    Result findText(const QString &txt, Utils::FindFlags findFlags, bool incremental)
    {
        const QList<Utils::MarkdownBrowser *> allBrowsers = browsers();
        const int count = allBrowsers.size();
        if (count == 0)
            return NotFound;

        const bool backward = findFlags & Utils::FindBackward;
        QTextDocument::FindFlags docFlags = toDocFlags(findFlags);
        if (backward)
            docFlags |= QTextDocument::FindBackward;

        // Determine starting browser and position
        int startIdx = -1;

        auto findIn = [&](int browserIndex, int startPos = -1) {
            Utils::MarkdownBrowser *browser = allBrowsers[browserIndex];
            const QTextCursor found = findInBrowser(browser, txt, startPos, findFlags, docFlags);

            if (found.isNull())
                return false;

            m_currentResult = {browser, found};
            browser->setTextCursor(found);
            browser->ensureCursorVisible();
            // Scroll the message view to make this browser visible
            m_view->ensureWidgetVisible(browser);
            return true;
        };

        if (incremental && m_incrementalStart.first != nullptr) {
            if (startIdx = allBrowsers.indexOf(m_incrementalStart.first); startIdx >= 0) {
                int startPos = backward ? m_incrementalStart.second.selectionEnd()
                                        : m_incrementalStart.second.selectionStart();
                if (findIn(startIdx, startPos))
                    return Found;
                startIdx = (backward ? startIdx - 1 + count : startIdx + 1) % count;
            }
        } else if (!incremental && m_currentResult.first != nullptr) {
            if (startIdx = allBrowsers.indexOf(m_currentResult.first); startIdx >= 0) {
                int startPos = backward ? m_currentResult.second.selectionStart()
                                        : m_currentResult.second.selectionEnd();
                if (findIn(startIdx, startPos))
                    return Found;
                startIdx = (backward ? startIdx - 1 + count : startIdx + 1) % count;
            }
        }


        if (startIdx < 0)
            startIdx = backward ? allBrowsers.size() - 1 : 0;

        // Search from current position, then wrap around
        for (int i = 0; i < count; ++i) {
            const int idx = backward ? (startIdx - i + count) % count : (startIdx + i) % count;
            if (findIn(idx))
                return Found;
        }

        m_currentResult = {nullptr, QTextCursor()};
        return NotFound;
    }

    AcpMessageView *m_view;
    QPair<Utils::MarkdownBrowser *, QTextCursor> m_currentResult;
    QPair<Utils::MarkdownBrowser *, QTextCursor> m_incrementalStart;
};

// ---------------------------------------------------------------------------
// AcpMessageView
// ---------------------------------------------------------------------------

static const char SETTINGS_THOUGHTS_VISIBLE[] = "acpclient/thoughtsVisible";

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

    auto *trailingRow = new QWidget(m_container);
    auto *trailingLayout = new QHBoxLayout(trailingRow);
    trailingLayout->setContentsMargins(0, 0, 0, 0);
    trailingLayout->setSpacing(GapHS);

    m_progressIndicator = new Utils::ProgressIndicator(
        Utils::ProgressIndicatorSize::Small, trailingRow);
    m_progressIndicator->setVisible(false);
    trailingLayout->addWidget(m_progressIndicator);

    m_elapsedLabel = new QLabel(trailingRow);
    m_elapsedLabel->setVisible(false);
    QFont elapsedFont = m_elapsedLabel->font();
    elapsedFont.setPointSizeF(elapsedFont.pointSizeF() * 0.9);
    elapsedFont.setFamily(QStringLiteral("monospace"));
    m_elapsedLabel->setFont(elapsedFont);
    trailingLayout->addWidget(m_elapsedLabel);

    trailingLayout->addStretch();

    m_layout->addWidget(trailingRow);
    m_layout->addStretch();

    m_elapsedTimer = new QElapsedTimer();
    m_progressUpdateTimer = new QTimer(this);
    m_progressUpdateTimer->setInterval(1000);
    connect(m_progressUpdateTimer, &QTimer::timeout, this, &AcpMessageView::updateElapsedTimeLabel);

    setWidget(m_container);

    Aggregation::aggregate({this, new MessageViewFindSupport(this)});

    // Track whether user has scrolled up to suppress auto-scroll
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int value) {
        m_autoScroll = (value >= verticalScrollBar()->maximum() - 10);
    });
    connect(verticalScrollBar(), &QScrollBar::rangeChanged, this, &AcpMessageView::scrollToBottom);

    m_thoughtsVisible = Core::ICore::settings()->value(SETTINGS_THOUGHTS_VISIBLE, true).toBool();
}

void AcpMessageView::setThoughtsVisible(bool visible)
{
    if (m_thoughtsVisible == visible)
        return;
    m_thoughtsVisible = visible;
    Core::ICore::settings()->setValue(SETTINGS_THOUGHTS_VISIBLE, visible);
    for (ThoughtWidget *w : std::as_const(m_thoughtWidgets))
        w->setVisible(visible);
}

void AcpMessageView::setAgentIconUrl(const QString &iconUrl)
{
    m_agentIconUrl = iconUrl;
}

void AcpMessageView::setPrompting(bool prompting)
{
    if (prompting) {
        m_elapsedTimer->restart();
        updateElapsedTimeLabel();
        m_progressUpdateTimer->start();
    } else {
        m_progressUpdateTimer->stop();
        for (auto it = m_toolCallDetailWidgets.constBegin();
             it != m_toolCallDetailWidgets.constEnd(); ++it) {
            qDebug() << "Checking tool call" << it.key() << "status"
                     << toString(it.value()->status());
            if (it.value()->status() == ToolCallStatus::in_progress
                || it.value()->status() == ToolCallStatus::pending) {
                it.value()->applyStatus(ToolCallStatus::failed);
                if (auto *group = m_toolCallGroups.value(it.key()))
                    group->trackStatus(it.key(), ToolCallStatus::failed);
            }
        }
    }
    m_progressIndicator->setVisible(prompting);
    m_elapsedLabel->setVisible(prompting);
}

void AcpMessageView::clear()
{
    // Remove all widgets except the trailing elapsed label and bottom stretch
    while (m_layout->count() > 2) {
        QLayoutItem *item = m_layout->takeAt(0);
        if (item->widget())
            delete item->widget();
        delete item;
    }

    m_currentAgentWidget = nullptr;
    m_currentThoughtWidget = nullptr;
    m_currentToolCallGroup = nullptr;
    m_currentAuthWidget = nullptr;
    m_thoughtWidgets.clear();
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
    if (m_currentThoughtWidget)
        m_currentThoughtWidget->setCollapsed(true);
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
    if (text.isEmpty())
        return;
    if (!m_currentThoughtWidget) {
        finishAgentMessage();
        finishToolCallGroup();
        m_currentThoughtWidget = new ThoughtWidget(text, m_container);
        m_currentThoughtWidget->setVisible(m_thoughtsVisible);
        m_thoughtWidgets.append(m_currentThoughtWidget);
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

    auto *detail = new ToolCallDetailWidget(toolCall, group);
    detail->setContentMaxWidth(contentMaxWidth());
    group->addChildWidget(detail);
    m_toolCallDetailWidgets[toolCall.toolCallId()] = detail;
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
    const QString toolCallId = toolCall.toolCallId();
    const QString title = toolCall.title().value_or(QString());
    const QString kindText = toolCall.kind() ? toString(*toolCall.kind()) : QString();

    QString command;
    if (const auto &rawInput = toolCall.rawInput(); rawInput && rawInput->isObject()) {
        const QJsonValue cmd = rawInput->toObject().value("command");
        if (cmd.isString())
            command = cmd.toString();
        if (title.contains(command))
            command.clear();
    }

    auto *widget = new PermissionRequestWidget(title, kindText, command, m_container);

    auto markToolCallFailed = [this, toolCallId] {
        if (auto *detail = m_toolCallDetailWidgets.value(toolCallId))
            detail->applyStatus(ToolCallStatus::failed);
        if (auto *group = m_toolCallGroups.value(toolCallId))
            group->trackStatus(toolCallId, ToolCallStatus::failed);
    };

    const QList<PermissionOption> options = request.options();
    for (const PermissionOption &option : options) {
        auto *button = widget->addOptionButton(option.name(), option.kind());
        connect(button, &QAbstractButton::clicked, this,
                [this, id, option, widget, markToolCallFailed] {
            const bool accepted = option.kind() == PermissionOptionKind::allow_once
                                  || option.kind() == PermissionOptionKind::allow_always;
            widget->setResolved(option.name(), accepted);
            if (!accepted)
                markToolCallFailed();
            emit permissionOptionSelected(id, option.optionId());
        });
    }

    const bool hasRejectOption = Utils::anyOf(
        options, Utils::equal(&PermissionOption::kind, PermissionOptionKind::reject_once));
    if (!hasRejectOption) {
        // Add a deny/cancel button if not already present, to allow user to reject the request without selecting an option
        auto *cancelButton
            = widget->addOptionButton(Tr::tr("Deny"), PermissionOptionKind::reject_once);
        connect(cancelButton, &QAbstractButton::clicked, this, [this, id, widget, markToolCallFailed] {
            widget->setResolved(Tr::tr("Denied"), false);
            markToolCallFailed();
            emit permissionCancelled(id);
        });
    }

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

SessionPickerWidget *AcpMessageView::addSessionPicker()
{
    finishAgentMessage();
    finishToolCallGroup();

    m_autoScroll = false;
    auto *picker = new SessionPickerWidget(m_container);
    addWidget(picker);
    return picker;
}

void AcpMessageView::addErrorMessage(const QString &text)
{
    finishAgentMessage();
    finishToolCallGroup();

    auto *widget = new CollapsibleFrame(m_container);
    widget->setFrameShape(QFrame::NoFrame);
    widget->setCollapsible(false);

    auto *icon = new Utils::QtcIconDisplay(widget);
    icon->setIcon(Utils::Icons::CRITICAL);
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
    if (m_currentThoughtWidget)
        m_currentThoughtWidget->setCollapsed(true);
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
    // Insert before the trailing elapsed label and the bottom stretch
    m_layout->insertWidget(m_layout->count() - 2, widget);
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
    for (ToolCallDetailWidget *detail : std::as_const(m_toolCallDetailWidgets))
        detail->setContentMaxWidth(maxW);
}

void AcpMessageView::updateElapsedTimeLabel()
{
    const qint64 elapsedMS = m_elapsedTimer->elapsed();
    const int secs = elapsedMS / 1000;
    const int minutes = secs / 60;
    const QString text = minutes ? Tr::tr("%1:%2 minutes")
                                       .arg(minutes)
                                       .arg(QString::number(secs % 60).rightJustified(2, '0'))
                                 : Tr::tr("%1 seconds").arg(secs);
    m_elapsedLabel->setText(text);
}

} // namespace AcpClient::Internal

#include "acpmessageview.moc"
