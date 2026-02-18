// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "collapsibleframe.h"

#include <acp/acp.h>

class QLabel;

namespace Utils { class MarkdownBrowser; }

namespace AcpClient::Internal {

class ToolCallDetailWidget : public CollapsibleFrame
{
    Q_OBJECT

public:
    explicit ToolCallDetailWidget(const Acp::ToolCall &toolCall, QWidget *parent = nullptr);

    void applyStatus(Acp::ToolCallStatus status);
    void updateTitle(const QString &title);
    void updateContent(const Acp::ToolCallUpdate &update);
    void setContentMaxWidth(int width);

protected:
    void paintEvent(QPaintEvent *e) override;

private:
    void populateContent(const Acp::ToolCall &toolCall);
    void addDiffContent(const Acp::Diff &diff);
    void addMarkdownContent(const QString &markdown);
    void addTerminalContent(const Acp::Terminal &terminal);
    void addTextContent(const Acp::Content &content);
    void addLocations(const QList<Acp::ToolCallLocation> &locations);
    void addBodyWidget(QWidget *widget);

    QLabel *m_statusLabel = nullptr;
    QLabel *m_titleLabel = nullptr;
    Acp::ToolCallStatus m_status = Acp::ToolCallStatus::in_progress;
    QList<Utils::MarkdownBrowser *> m_browsers;
};

} // namespace AcpClient::Internal
