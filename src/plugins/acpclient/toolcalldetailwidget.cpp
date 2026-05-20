// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolcalldetailwidget.h"

#include "acpclienttr.h"

#include <coreplugin/editormanager/editormanager.h>

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>
#include <utils/link.h>
#include <utils/markdownbrowser.h>
#include <utils/progressindicator.h>
#include <utils/qtcwidgets.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/elidinglabel.h>
#include <utils/utilsicons.h>

#include <QAbstractButton>
#include <QAbstractTextDocumentLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QtMath>

using namespace Acp;
using namespace Utils::StyleHelper::SpacingTokens;

namespace AcpClient::Internal {

QWidget *toolCallStatusWidget(ToolCallStatus status, QWidget *parent)
{
    if (status == ToolCallStatus::in_progress)
        return new Utils::ProgressIndicator(Utils::ProgressIndicatorSize::Small, parent);

    Utils::Icon icon;
    switch (status) {
    case ToolCallStatus::pending:   icon = Utils::Icons::DOWNLOAD; break;
    case ToolCallStatus::completed: icon = Utils::Icons::OK; break;
    case ToolCallStatus::failed:    icon = Utils::Icons::CRITICAL; break;
    default: break;
    }
    auto *display = new Utils::QtcIconDisplay(parent);
    display->setIcon(icon);
    return display;
}

std::optional<Utils::Icon> iconForToolKind(std::optional<ToolKind> kind)
{
    if (kind) {
        switch (*kind) {
        case ToolKind::read:
            return Utils::Icons::SAVEFILE;
        case ToolKind::edit:
            return Utils::Icons::PASTE;
        case ToolKind::delete_:
            return Utils::Icons::CLEAN;
        case ToolKind::move:
            return Utils::Icons::COPY;
        case ToolKind::search:
            return Utils::Icons::ZOOM;
        case ToolKind::execute:
            return Utils::Icons::RUN_SMALL;
        case ToolKind::think:
            return Utils::Icons::EYE_OPEN;
        case ToolKind::fetch:
            return Utils::Icons::DOWNLOAD;
        case ToolKind::switch_mode:
            return Utils::Icons::RELOAD;
        case ToolKind::other:
            return std::nullopt;
        }
    }
    return std::nullopt;
}

QColor toolCallBorderColor(ToolCallStatus status)
{
    switch (status) {
    case ToolCallStatus::pending:     return Qt::gray;
    case ToolCallStatus::in_progress: return QColor{0x1e, 0x90, 0xff}; // dodgerblue
    case ToolCallStatus::completed:   return Qt::green;
    case ToolCallStatus::failed:      return Qt::red;
    }
    return Qt::gray;
}

ToolCallDetailWidget::ToolCallDetailWidget(const ToolCall &toolCall, QWidget *parent)
    : CollapsibleFrame(parent)
{
    setFrameShape(QFrame::NoFrame);
    setCollapsible(false);

    const ToolCallStatus status = toolCall.status().value_or(ToolCallStatus::in_progress);

    // Header: status widget + title + kind badge
    m_statusWidget = toolCallStatusWidget(status, this);
    m_headerLayout->addWidget(m_statusWidget);

    if (const auto icon = iconForToolKind(toolCall.kind())) {
        auto *kindIcon = new Utils::QtcIconDisplay(this);
        kindIcon->setIcon(*icon);
        m_headerLayout->addWidget(kindIcon);
    }

    m_titleLabel = new Utils::ElidingLabel(toolCall.title(), this);
    m_titleLabel->setElideMode(Qt::ElideRight);
    m_headerLayout->addWidget(m_titleLabel, 1);

    m_status = status;
    populateContent(toolCall);
}

void ToolCallDetailWidget::applyStatus(ToolCallStatus status)
{
    auto *newWidget = toolCallStatusWidget(status, this);
    delete m_headerLayout->replaceWidget(m_statusWidget, newWidget);
    delete m_statusWidget;
    m_statusWidget = newWidget;
    m_status = status;
    update();
}

void ToolCallDetailWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    Utils::StyleHelper::drawCardBg(&p, rect(),
        Utils::creatorColor(Utils::Theme::ChatToolCallBackground),
        QPen(palette().color(QPalette::Mid), 1),
        RadiusM);
    QPainterPath clip;
    clip.addRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5),
                        RadiusM - 0.5, RadiusM - 0.5);
    p.setClipPath(clip);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.fillRect(QRect(0, 0, 3, height()), toolCallBorderColor(m_status));
    p.setClipping(false);
}

void ToolCallDetailWidget::setContentMaxWidth(int width)
{
    m_contentMaxWidth = width - BodyMarginLeft - BodyMarginRight;
    for (int i = 0; i < m_bodyLayout->count(); ++i) {
        if (QWidget *w = m_bodyLayout->itemAt(i)->widget())
            w->setMaximumWidth(m_contentMaxWidth);
    }
}

void ToolCallDetailWidget::updateContent(const ToolCallUpdate &update)
{
    if (const auto status = update.status())
        applyStatus(*status);
    if (const auto title = update.title(); title && !title->isEmpty())
        m_titleLabel->setText(*title);

    if (const auto &rawInput = update.rawInput())
        addRawInputContent(*rawInput);

    // Parse updated content if available
    if (const auto &contentArr = update.content()) {
        for (const QJsonValue &val : *contentArr) {
            auto content = fromJson<ToolCallContent>(val);
            if (!content)
                continue;
            if (auto *diff = std::get_if<Diff>(&*content))
                addDiffContent(*diff);
            else if (auto *terminal = std::get_if<Terminal>(&*content))
                addTerminalContent(*terminal);
            else if (auto *textContent = std::get_if<Content>(&*content))
                addTextContent(*textContent);
        }
    }

    // Parse updated locations
    if (const auto &locsArr = update.locations()) {
        QList<ToolCallLocation> locs;
        for (const QJsonValue &val : *locsArr) {
            auto loc = fromJson<ToolCallLocation>(val);
            if (loc)
                locs.append(*loc);
        }
        if (!locs.isEmpty())
            addLocations(locs);
    }
}

void ToolCallDetailWidget::populateContent(const ToolCall &toolCall)
{
    if (const auto &rawInput = toolCall.rawInput())
        addRawInputContent(*rawInput);

    // Content items
    if (const auto &contentList = toolCall.content()) {
        for (const ToolCallContent &tc : *contentList) {
            if (auto *diff = std::get_if<Diff>(&tc))
                addDiffContent(*diff);
            else if (auto *terminal = std::get_if<Terminal>(&tc))
                addTerminalContent(*terminal);
            else if (auto *textContent = std::get_if<Content>(&tc))
                addTextContent(*textContent);
        }
    }

    // Locations
    if (const auto &locs = toolCall.locations())
        addLocations(*locs);
}

void ToolCallDetailWidget::addDiffContent(const Diff &diff)
{
    // File path as clickable link
    auto *pathLabel = new QLabel(this);
    pathLabel->setTextFormat(Qt::RichText);
    const QColor linkColor = Utils::creatorColor(Utils::Theme::TextColorLink);
    pathLabel->setText(QStringLiteral("<a href=\"file://%1\" style=\"color:%2; text-decoration:none;\">%1</a>")
                           .arg(diff.path().toHtmlEscaped(),
                                linkColor.name()));
    pathLabel->setCursor(Qt::PointingHandCursor);
    connect(pathLabel, &QLabel::linkActivated, this, [](const QString &link) {
        const QString path = QUrl(link).toLocalFile();
        if (!path.isEmpty())
            Core::EditorManager::openEditorAt(Utils::Link::fromString(path));
    });
    addBodyWidget(pathLabel);

    // Render diff as markdown code block
    QString markdown = QStringLiteral("```diff\n");
    if (diff.oldText().has_value()) {
        // Show abbreviated diff context
        const QStringList oldLines = diff.oldText()->split(QLatin1Char('\n'));
        const QStringList newLines = diff.newText().split(QLatin1Char('\n'));
        for (const QString &line : oldLines)
            markdown += QStringLiteral("-%1\n").arg(line);
        for (const QString &line : newLines)
            markdown += QStringLiteral("+%1\n").arg(line);
    } else {
        // New file
        const QStringList lines = diff.newText().split(QLatin1Char('\n'));
        for (const QString &line : lines)
            markdown += QStringLiteral("+%1\n").arg(line);
    }
    markdown += QStringLiteral("```");

    addMarkdownContent(markdown);
}

void ToolCallDetailWidget::addMarkdownContent(const QString &markdown)
{
    if (markdown.isEmpty())
        return;

    auto *browser = new Utils::MarkdownBrowser(this);
    browser->setFrameShape(QFrame::NoFrame);
    browser->setEnableCodeCopyButton(true);
    browser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    browser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    browser->setMargins({0, 0, 0, 0});
    browser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // Auto-size height on document size changes (content change and reflow)
    connect(browser->document()->documentLayout(),
            &QAbstractTextDocumentLayout::documentSizeChanged,
            browser, [browser] {
        const int h = qCeil(browser->document()->size().height())
                      + browser->contentsMargins().top()
                      + browser->contentsMargins().bottom();
        browser->setFixedHeight(qMax(h, browser->fontMetrics().height()));
    });

    addBodyWidget(browser);
    browser->setMarkdown(markdown);
}

void ToolCallDetailWidget::addTerminalContent(const Terminal &terminal)
{
    auto *label = new QLabel(this);
    label->setTextFormat(Qt::RichText);
    label->setText(QStringLiteral("<code>Terminal: %1</code>")
                       .arg(terminal.terminalId().toHtmlEscaped()));
    addBodyWidget(label);
}

void ToolCallDetailWidget::addTextContent(const Content &content)
{
    const auto &block = content.content();
    if (auto *textBlock = std::get_if<TextContent>(&block))
        addMarkdownContent(textBlock->text());
}

void ToolCallDetailWidget::addLocations(const QList<ToolCallLocation> &locations)
{
    for (const ToolCallLocation &loc : locations) {
        auto *label = new QLabel(this);
        label->setTextFormat(Qt::RichText);
        label->setCursor(Qt::PointingHandCursor);

        QString display = loc.path();
        if (const auto line = loc.line())
            display += QStringLiteral(":%1").arg(*line);

        label->setText(QStringLiteral("<a href=\"loc://%1:%2\">%3</a>")
                           .arg(loc.path().toHtmlEscaped())
                           .arg(loc.line().value_or(0))
                           .arg(display.toHtmlEscaped()));

        connect(label, &QLabel::linkActivated, this, [](const QString &link) {
            const QUrl url(link);
            const QString path = url.host() + url.path();
            const int line = url.port(0);
            Core::EditorManager::openEditorAt(
                Utils::Link{Utils::FilePath::fromString(path), line});
        });

        addBodyWidget(label);
    }
}

void ToolCallDetailWidget::addRawInputContent(const QJsonValue &rawInput)
{
    if (rawInput.isUndefined() || rawInput.isNull())
        return;

    QString json;
    QString command;
    if (rawInput.isObject()) {
        const QJsonObject inputObject = rawInput.toObject();
        if (inputObject.isEmpty())
            return;
        json = QString::fromUtf8(QJsonDocument(inputObject).toJson(QJsonDocument::Indented));
        if (inputObject.contains("command") && inputObject["command"].isString())
            command = inputObject["command"].toString();
        if (m_titleLabel->text().contains(command))
            command.clear();
    } else if (rawInput.isArray()) {
        const QJsonArray inputArray = rawInput.toArray();
        if (inputArray.isEmpty())
            return;
        json = QString::fromUtf8(QJsonDocument(inputArray).toJson(QJsonDocument::Indented));
    } else if (rawInput.isString()) {
        const QString s = rawInput.toString();
        if (s.isEmpty())
            return;
        json = '"' + s + '"';
    } else {
        json = rawInput.toVariant().toString();
    }
    json = json.trimmed();
    if (json.isEmpty())
        return;

    const QString markdown = QStringLiteral("```json\n%1\n```").arg(json);

    if (!command.isEmpty()) {
        if (!m_commandLabel) {
            m_commandLabel = new QLabel(this);
            m_commandLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
            m_commandLabel->setWordWrap(true);
            QFont mono = m_commandLabel->font();
            mono.setStyleHint(QFont::Monospace);
            mono.setFamily(QFont(QStringLiteral("monospace")).defaultFamily());
            m_commandLabel->setFont(mono);
            if (m_contentMaxWidth >= 0)
                m_commandLabel->setMaximumWidth(m_contentMaxWidth);
            m_bodyLayout->insertWidget(0, m_commandLabel);
        }
        m_commandLabel->setText(command);
    }

    if (!m_rawInputContent) {
        auto *frame = new CollapsibleFrame(this); frame->setFrameShape(QFrame::NoFrame);
        frame->setCollapsed(true);

        frame->headerLayout()->addWidget(new QLabel(Tr::tr("Raw Input"), frame), 1);

        auto *browser = new Utils::MarkdownBrowser(frame);
        browser->setFrameShape(QFrame::NoFrame);
        browser->setEnableCodeCopyButton(true);
        browser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        browser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        browser->setMargins({0, 0, 0, 0});
        browser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        connect(browser->document()->documentLayout(),
                &QAbstractTextDocumentLayout::documentSizeChanged,
                browser, [browser] {
            const int h = qCeil(browser->document()->size().height())
                          + browser->contentsMargins().top()
                          + browser->contentsMargins().bottom();
            browser->setFixedHeight(qMax(h, browser->fontMetrics().height()));
        });
        frame->bodyLayout()->addWidget(browser);

        if (m_contentMaxWidth >= 0)
            frame->setMaximumWidth(m_contentMaxWidth);
        const int insertIndex = m_commandLabel ? 1 : 0;
        m_bodyLayout->insertWidget(insertIndex, frame);
        m_rawInputFrame = frame;
        m_rawInputContent = browser;
    }
    m_rawInputContent->setMarkdown(markdown);
}

void ToolCallDetailWidget::addBodyWidget(QWidget *widget)
{
    if (!isCollapsible()) {
        setCollapsible(true);
        setCollapsed(true);
    }
    if (m_contentMaxWidth >= 0)
        widget->setMaximumWidth(m_contentMaxWidth);
    m_bodyLayout->addWidget(widget);
}

static QString permissionOptionKindLabel(PermissionOptionKind kind)
{
    switch (kind) {
    case PermissionOptionKind::allow_once:    return Tr::tr("Allow Once");
    case PermissionOptionKind::allow_always:  return Tr::tr("Allow Always");
    case PermissionOptionKind::reject_once:   return Tr::tr("Reject Once");
    case PermissionOptionKind::reject_always: return Tr::tr("Reject Always");
    }
    return {};
}

void ToolCallDetailWidget::addPermissionControls(const QList<PermissionOption> &options,
                                                 bool addDenyFallback)
{
    if (m_permissionRow)
        return;

    // Force the body open while a permission decision is pending.
    m_collapsibleBeforePermission = isCollapsible();
    m_collapsedBeforePermission = isCollapsed();
    setCollapsible(false);
    setCollapsed(false);

    m_permissionRow = new QWidget(this);
    auto *rowLayout = new QVBoxLayout(m_permissionRow);
    rowLayout->setContentsMargins(0, 0, 0, 0);

    auto *promptRow = new QHBoxLayout;
    promptRow->setContentsMargins(0, 0, 0, 0);
    auto *warningIcon = new Utils::QtcIconDisplay(m_permissionRow);
    warningIcon->setIcon(Utils::Icons::WARNING);
    promptRow->addWidget(warningIcon);
    auto *promptLabel = new QLabel(Tr::tr("Permission required:"), m_permissionRow);
    promptRow->addWidget(promptLabel, 1);
    rowLayout->addLayout(promptRow);

    auto *buttonHost = new QWidget(m_permissionRow);
    Layouting::Flow{}.attachTo(buttonHost);
    QLayout *buttonLayout = buttonHost->layout();
    rowLayout->addWidget(buttonHost);

    for (const PermissionOption &option : options) {
        const bool reject = option.kind() == PermissionOptionKind::reject_once
                            || option.kind() == PermissionOptionKind::reject_always;
        const auto role = reject ? Utils::QtcButton::MediumTertiary
                                 : Utils::QtcButton::MediumPrimary;
        const QString name = option.name();
        QString buttonText;
        if (name.isEmpty())
            buttonText = permissionOptionKindLabel(option.kind());
        else if (name.size() > 30)
            buttonText = name.left(27) + QStringLiteral("...");
        else
            buttonText = name;
        auto *button = new Utils::QtcButton(buttonText, role, m_permissionRow);
        button->setToolTip(name);
        buttonLayout->addWidget(button);
        m_permissionButtons.append(button);
        const QString optionId = option.optionId();
        connect(button, &QAbstractButton::clicked, this, [this, optionId] {
            emit permissionOptionSelected(optionId);
        });
    }

    const bool hasReject = Utils::anyOf(
        options, Utils::equal(&PermissionOption::kind, PermissionOptionKind::reject_once));
    if (addDenyFallback && !hasReject) {
        auto *cancel = new Utils::QtcButton(
            Tr::tr("Deny"), Utils::QtcButton::MediumTertiary, m_permissionRow);
        buttonLayout->addWidget(cancel);
        m_permissionButtons.append(cancel);
        connect(cancel, &QAbstractButton::clicked, this, [this] {
            emit permissionCancelled();
        });
    }

    m_permissionStatusLabel = new QLabel(m_permissionRow);
    m_permissionStatusLabel->hide();
    promptRow->addWidget(m_permissionStatusLabel);

    if (m_contentMaxWidth >= 0)
        m_permissionRow->setMaximumWidth(m_contentMaxWidth);
    m_bodyLayout->addWidget(m_permissionRow);
}

void ToolCallDetailWidget::resolvePermission(const QString &text, bool accepted)
{
    for (QAbstractButton *button : std::as_const(m_permissionButtons))
        button->hide();
    if (m_permissionStatusLabel) {
        m_permissionStatusLabel->setText(QStringLiteral("<i>%1</i>").arg(text.toHtmlEscaped()));
        m_permissionStatusLabel->show();
    }
    if (!accepted)
        applyStatus(ToolCallStatus::failed);

    // Restore the collapsible state that was in effect before the permission row was shown.
    if (m_collapsibleBeforePermission) {
        setCollapsible(true);
        setCollapsed(m_collapsedBeforePermission);
    }
}

} // namespace AcpClient::Internal
