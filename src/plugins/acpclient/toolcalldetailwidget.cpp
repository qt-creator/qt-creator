// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolcalldetailwidget.h"

#include <coreplugin/editormanager/editormanager.h>

#include <utils/link.h>
#include <utils/markdownbrowser.h>
#include <utils/progressindicator.h>
#include <utils/qtcwidgets.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/elidinglabel.h>
#include <utils/utilsicons.h>

#include <QAbstractTextDocumentLayout>
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
    const int browserMax = width - BodyMarginLeft - BodyMarginRight;
    for (Utils::MarkdownBrowser *browser : m_browsers)
        browser->setMaximumWidth(browserMax);
}

void ToolCallDetailWidget::updateContent(const ToolCallUpdate &update)
{
    if (const auto status = update.status())
        applyStatus(*status);
    if (const auto title = update.title())
        m_titleLabel->setText(*title);

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

    m_browsers.append(browser);
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

void ToolCallDetailWidget::addBodyWidget(QWidget *widget)
{
    if (!isCollapsible()) {
        setCollapsible(true);
        setCollapsed(true);
    }
    m_bodyLayout->addWidget(widget);
}

} // namespace AcpClient::Internal
