// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

#include <utils/markdownbrowser.h>
#include <utils/stylehelper.h>

QT_BEGIN_NAMESPACE
class QFont;
class QLabel;
class QWidget;
QT_END_NAMESPACE

namespace AcpClient::Internal {

// Markdown browser for chat content. Never scrolls vertically; its height
// follows the document. Code-only content can scroll horizontally instead
// of wrapping.
class ChatMarkdownBrowser : public Utils::MarkdownBrowser
{
public:
    explicit ChatMarkdownBrowser(QWidget *parent = nullptr);

    // Disables line wrapping and shows a horizontal scroll bar when needed.
    void setScrollHorizontally(bool scroll);
    // Reports the unwrapped content width as size hint width.
    void setContentWidthHint(bool enable);

    QSize sizeHint() const override;

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateHeight();

    bool m_contentWidthHint = false;
    mutable bool m_measuring = false;
    mutable qreal m_cachedUnwrappedIdealWidth = -1;
};

struct MarkdownSegment
{
    QString text;
    bool isCode = false;
};

// Splits markdown into prose and fenced code block segments. An unterminated
// code block (still streaming in) extends to the end of the text.
QList<MarkdownSegment> splitCodeBlocks(const QString &markdown);

// True if the markdown consists of exactly one fenced code block.
bool isSingleCodeBlock(const QString &markdown);

class ChatFontScale : public QObject
{
    Q_OBJECT

public:
    static ChatFontScale &instance();

    static qreal scale();
    static void setScale(qreal scale);
    static void zoomIn();
    static void zoomOut();
    static void resetZoom();

signals:
    void scaleChanged(qreal scale);

private:
    ChatFontScale();

    qreal m_scale;
};

void setupChatZoomActions(QObject *guard);
void setChatFont(QWidget *widget, qreal factor = 1.0);
void setChatFont(QWidget *widget, const QFont &baseFont);
void setChatTextFormat(QLabel *label, const Utils::StyleHelper::TextFormat &format);
void setChatSpacing(QWidget *widget);
qreal chatRadius(qreal radius);
void setupChatBrowser(Utils::MarkdownBrowser *browser);
void enableChatZoom(QWidget *widget);

} // namespace AcpClient::Internal
